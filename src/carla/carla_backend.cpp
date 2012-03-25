/*
 * JACK Backend code for Carla
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_backend.h"

// Global variables
CallbackFunc Callback = nullptr;
const char* last_error = nullptr;
const char* carla_client_name = nullptr;

#if 0

#include "audio_plugin.h"
#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <QtGui/QApplication>
#include <QtCore/QMutex>

QMutex carla_proc_lock_var;
QMutex carla_midi_lock_var;
CarlaCheckThread check_thread;

// Global variables (shared)
const char* unique_names[MAX_PLUGINS]  = { nullptr };
AudioPlugin* AudioPlugins[MAX_PLUGINS] = { nullptr };
ExternalMidiNote ExternalMidiNotes[MAX_MIDI_EVENTS];

volatile double ains_peak[MAX_PLUGINS*2]  = { 0.0 };
volatile double aouts_peak[MAX_PLUGINS*2] = { 0.0 };

// Global JACK client
jack_client_t* carla_jack_client = nullptr;
jack_nframes_t carla_buffer_size = 512;
jack_nframes_t carla_sample_rate = 48000;

// Global OSC stuff
lo_server_thread global_osc_server_thread = nullptr;
const char* global_osc_server_path = nullptr;
OscData global_osc_data = { nullptr, nullptr, nullptr };

// Global options
carla_options_t carla_options = {
  /* _initiated */           false,
  /* global_jack_client   */ false,
  /* bridge_path_lv2_gtk2 */ nullptr,
  /* bridge_path_lv2_qt4  */ nullptr,
  /* bridge_path_lv2_x11  */ nullptr,
  /* bridge_path_vst_qt4  */ nullptr,
  /* bridge_path_winvst   */ nullptr
};

// jack.cpp
int carla_jack_process_callback(jack_nframes_t nframes, void* arg);
int carla_jack_bufsize_callback(jack_nframes_t new_buffer_size, void* arg);
int carla_jack_srate_callback(jack_nframes_t new_sample_rate, void* arg);
void carla_jack_shutdown_callback(void* arg);

// plugin specific
short add_plugin_ladspa(const char* filename, const char* label, void* extra_stuff);
short add_plugin_dssi(const char* filename, const char* label, void* extra_stuff);
short add_plugin_lv2(const char* filename, const char* label, void* extra_stuff);
short add_plugin_vst(const char* filename, const char* label);
short add_plugin_winvst(const char* filename, const char* label, void* extra_stuff);
short add_plugin_sf2(const char* filename, const char* label);

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)
bool carla_init(const char* client_name)
{
    qDebug("carla_init(%s)", client_name);

    carla_options._initiated = true;

    bool started = false;
    carla_jack_client = jack_client_open(client_name, JackNullOption, 0x0);

    if (carla_jack_client)
    {
        carla_buffer_size = jack_get_buffer_size(carla_jack_client);
        carla_sample_rate = jack_get_sample_rate(carla_jack_client);

        if (carla_options.global_jack_client)
            jack_set_process_callback(carla_jack_client, carla_jack_process_callback, nullptr);

        jack_set_buffer_size_callback(carla_jack_client, carla_jack_bufsize_callback, nullptr);
        jack_set_sample_rate_callback(carla_jack_client, carla_jack_srate_callback, nullptr);
        jack_on_shutdown(carla_jack_client, carla_jack_shutdown_callback, nullptr);

        if (jack_activate(carla_jack_client))
        {
            set_last_error("Failed to activate the JACK client");
            carla_jack_client = nullptr;
        }
        else
            started = true;
    }
    else
    {
        set_last_error("Failed to create new JACK client");
        carla_jack_client = nullptr;
    }

    if (started)
    {
        client_name = jack_get_client_name(carla_jack_client);

        // Fix name for OSC usage
        char* fixed_name = strdup(client_name);
        for (size_t i=0; i < strlen(fixed_name); i++)
        {
            if (!std::isalpha(fixed_name[i]) && !std::isdigit(fixed_name[i]))
                fixed_name[i] = '_';
        }

        carla_client_name = strdup(fixed_name);
        free((void*)fixed_name);

        for (unsigned short i=0; i<MAX_MIDI_EVENTS; i++)
            ExternalMidiNotes[i].valid = false;

        osc_init();

        check_thread.start();

        set_last_error("no error");
    }

    return started;
}

bool carla_close()
{
    qDebug("carla_close()");

    bool closed = false;

    if (jack_deactivate(carla_jack_client))
        set_last_error("Failed to deactivate the JACK client");
    else
    {
        if (jack_client_close(carla_jack_client))
            set_last_error("Failed to close the JACK client");
        else
            closed = true;
    }

    carla_jack_client = nullptr;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id >= 0)
            remove_plugin(i);
    }

    check_thread.quit();

    if (!check_thread.wait(1000000)) // 1 sec
        qWarning("Failed to properly stop global check thread");

    osc_send_exit(&global_osc_data);
    osc_close();

    if (carla_client_name)
        free((void*)carla_client_name);

    if (last_error)
        free((void*)last_error);

    if (carla_options._initiated)
    {
        if (carla_options.bridge_path_lv2_gtk2)
            free((void*)carla_options.bridge_path_lv2_gtk2);

        if (carla_options.bridge_path_lv2_qt4)
            free((void*)carla_options.bridge_path_lv2_qt4);

        if (carla_options.bridge_path_lv2_x11)
            free((void*)carla_options.bridge_path_lv2_x11);

        if (carla_options.bridge_path_vst_qt4)
            free((void*)carla_options.bridge_path_vst_qt4);

        if (carla_options.bridge_path_winvst)
            free((void*)carla_options.bridge_path_winvst);
    }

    // cleanup static data
    get_plugin_info(0);
    get_parameter_info(0, 0);
    get_scalepoint_info(0, 0, 0);
    get_chunk_data(0);
    get_real_plugin_name(0);

    return closed;
}

bool carla_is_engine_running()
{
    return bool(carla_jack_client);
}

short add_plugin(PluginType ptype, const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin(%s, %s, %i, %p)", filename, label, ptype, extra_stuff);

    switch (ptype)
    {
    case PLUGIN_LADSPA:
        return add_plugin_ladspa(filename, label, extra_stuff);
    case PLUGIN_DSSI:
        return add_plugin_dssi(filename, label, extra_stuff);
    case PLUGIN_LV2:
        return add_plugin_lv2(filename, label, extra_stuff);
    case PLUGIN_VST:
        return add_plugin_vst(filename, label);
    case PLUGIN_WINVST:
        return add_plugin_winvst(filename, label, extra_stuff);
    case PLUGIN_SF2:
        return add_plugin_sf2(filename, label);
    default:
        set_last_error("Unknown plugin type");
        return -1;
    }
}

bool remove_plugin(unsigned short plugin_id)
{
    qDebug("remove_plugin(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            osc_send_remove_plugin(&global_osc_data, plugin->id);

            carla_proc_lock();
            plugin->id = -1;
            carla_proc_unlock();

            plugin->delete_me();

            AudioPlugins[i] = nullptr;
            unique_names[i] = nullptr;

            return true;
        }
    }

    set_last_error("Could not find plugin to remove");
    return false;
}

PluginInfo* get_plugin_info(unsigned short plugin_id)
{
    qDebug("get_plugin_info(%i)", plugin_id);

    static PluginInfo info = { false, PLUGIN_NONE, PLUGIN_CATEGORY_NONE, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 };

    if (info.valid)
    {
        free((void*)info.label);
        free((void*)info.maker);
        free((void*)info.copyright);
    }

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            char buf_str[STR_MAX] = { 0 };

            info.valid     = true;
            info.type      = plugin->type;
            info.category  = plugin->get_category();
            info.hints     = plugin->hints;
            info.binary    = plugin->filename;
            info.name      = plugin->name;
            info.unique_id = plugin->get_unique_id();

            plugin->get_label(buf_str);
            info.label = strdup(buf_str);

            plugin->get_maker(buf_str);
            info.maker = strdup(buf_str);

            plugin->get_copyright(buf_str);
            info.copyright = strdup(buf_str);

            return &info;
        }
    }

    if (carla_is_engine_running())
        qCritical("get_plugin_info(%i) - could not find plugin", plugin_id);

    return &info;
}

PortCountInfo* get_audio_port_count_info(unsigned short plugin_id)
{
    qDebug("get_audio_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { false, 0, 0, 0 };
    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            info.valid = true;
            info.ins   = plugin->ain.count;
            info.outs  = plugin->aout.count;
            info.total = info.ins + info.outs;
            return &info;
        }
    }

    qCritical("get_audio_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

PortCountInfo* get_midi_port_count_info(unsigned short plugin_id)
{
    qDebug("get_midi_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { false, 0, 0, 0 };
    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            info.valid = true;
            info.ins   = plugin->min.count;
            info.outs  = plugin->mout.count;
            info.total = info.ins + info.outs;
            return &info;
        }
    }

    qCritical("get_midi_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

PortCountInfo* get_parameter_count_info(unsigned short plugin_id)
{
    qDebug("get_parameter_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { false, 0, 0, 0 };
    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            info.valid = true;
            info.ins   = 0;
            info.outs  = 0;
            info.total = plugin->param.count;

            for (uint32_t j=0; j < plugin->param.count; j++)
            {
                ParameterType ParamType = plugin->param.data[j].type;
                if (ParamType == PARAMETER_INPUT)
                    info.ins += 1;
                else if (ParamType == PARAMETER_OUTPUT)
                    info.outs += 1;
            }

            return &info;
        }
    }

    qCritical("get_parameter_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

ParameterInfo* get_parameter_info(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_parameter_info(%i, %i)", plugin_id, parameter_id);

    static ParameterInfo info = { false, nullptr, nullptr, nullptr, 0 };

    if (info.valid)
    {
        free((void*)info.name);
        free((void*)info.symbol);
        free((void*)info.label);
    }

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
            {
                char buf_str[STR_MAX] = { 0 };
                int32_t rindex = plugin->param.data[parameter_id].rindex;

                if (rindex >= 0)
                {
                    info.valid = true;
                    info.scalepoint_count = plugin->get_scalepoint_count(rindex);

                    plugin->get_parameter_name(rindex, buf_str);
                    info.name = strdup(buf_str);

                    plugin->get_parameter_symbol(rindex, buf_str);
                    info.symbol = strdup(buf_str);

                    plugin->get_parameter_label(rindex, buf_str);
                    info.label = strdup(buf_str);
                }
                else
                    qCritical("get_parameter_info(%i, %i) - invalid rindex: %i", plugin_id, parameter_id, rindex);
            }
            else
                qCritical("get_parameter_info(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &info;
        }
    }

    if (carla_is_engine_running())
        qCritical("get_parameter_info(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return &info;
}

ScalePointInfo* get_scalepoint_info(unsigned short plugin_id, uint32_t parameter_id, uint32_t scalepoint_id)
{
    qDebug("get_scalepoint_info(%i, %i, %i)", plugin_id, parameter_id, scalepoint_id);

    static ScalePointInfo info = { false, 0.0, nullptr };

    if (info.valid)
    {
        free((void*)info.label);
    }

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
            {
                int32_t rindex = plugin->param.data[parameter_id].rindex;

                if (scalepoint_id < plugin->get_scalepoint_count(rindex))
                {
                    char buf_str[STR_MAX] = { 0 };

                    info.valid = true;
                    info.value = plugin->get_scalepoint_value(rindex, scalepoint_id);

                    plugin->get_scalepoint_label(rindex, scalepoint_id, buf_str);
                    info.label = strdup(buf_str);
                }
                else
                    qCritical("get_scalepoint_info(%i, %i, %i) - scalepoint_id out of bounds", plugin_id, parameter_id, scalepoint_id);
            }
            else
                qCritical("get_scalepoint_info(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, parameter_id);

            return &info;
        }
    }

    if (carla_is_engine_running())
        qCritical("get_scalepoint_info(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, scalepoint_id);

    return &info;
}

MidiProgramInfo* get_midi_program_info(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("get_midi_program_info(%i, %i)", plugin_id, midi_program_id);

    static MidiProgramInfo info = { false, 0, 0, nullptr };
    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (midi_program_id < plugin->midiprog.count)
            {
                info.valid   = true;
                info.bank    = plugin->midiprog.data[midi_program_id].bank;
                info.program = plugin->midiprog.data[midi_program_id].program;
                info.label   = plugin->midiprog.names[midi_program_id];
            }
            else
                qCritical("get_midi_program_info(%i, %i) - midi_program_id out of bounds", plugin_id, midi_program_id);

            return &info;
        }
    }

    qCritical("get_midi_program_info(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return &info;
}

ParameterData* get_parameter_data(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_parameter_data(%i, %i)", plugin_id, parameter_id);

    static ParameterData data = { PARAMETER_UNKNOWN, -1, -1, 0, 0, -1 };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
                return &plugin->param.data[parameter_id];
            else
                qCritical("get_parameter_data(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &data;
        }
    }

    qCritical("get_parameter_data(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &data;
}

ParameterRanges* get_parameter_ranges(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_parameter_ranges(%i, %i)", plugin_id, parameter_id);

    static ParameterRanges ranges = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
                return &plugin->param.ranges[parameter_id];
            else
                qCritical("get_parameter_ranges(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &ranges;
        }
    }

    qCritical("get_parameter_ranges(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &ranges;
}

CustomData* get_custom_data(unsigned short plugin_id, uint32_t custom_data_id)
{
    qDebug("get_custom_data(%i, %i)", plugin_id, custom_data_id);

    static CustomData data = { CUSTOM_DATA_INVALID, nullptr, nullptr };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (custom_data_id < (uint32_t)plugin->custom.count())
                return &plugin->custom[custom_data_id];
            else
                qCritical("get_custom_data(%i, %i) - custom_data_id out of bounds", plugin_id, custom_data_id);

            return &data;
        }
    }

    qCritical("get_custom_data(%i, %i) - could not find plugin", plugin_id, custom_data_id);
    return &data;
}

const char* get_chunk_data(unsigned short plugin_id)
{
    qDebug("get_chunk_data(%i)", plugin_id);

    static const char* chunk_data = nullptr;

    if (chunk_data)
        free((void*)chunk_data);

    chunk_data = nullptr;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (plugin->hints & PLUGIN_USES_CHUNKS)
            {
                void* data = nullptr;
                int32_t data_size = plugin->get_chunk_data(&data);

                if (data && data_size >= 4)
                {
                    QByteArray chunk((const char*)data, data_size);
                    chunk_data = strdup(chunk.toBase64().data());
                }
                else
                    qCritical("get_chunk_data(%i) - got invalid chunk data", plugin_id);
            }
            else
                qCritical("get_chunk_data(%i) - plugin does not support chunks", plugin_id);

            return chunk_data;
        }
    }

    if (carla_is_engine_running())
        qCritical("get_chunk_data(%i) - could not find plugin", plugin_id);

    return chunk_data;
}

GuiData* get_gui_data(unsigned short plugin_id)
{
    qDebug("get_gui_data(%i)", plugin_id);

    static GuiData data = { GUI_NONE, false, false, 0, 0, nullptr, false };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return &plugin->gui;
    }

    qCritical("get_gui_data(%i) - could not find plugin", plugin_id);
    return &data;
}

uint32_t get_parameter_count(unsigned short plugin_id)
{
    qDebug("get_parameter_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->param.count;
    }

    qCritical("get_parameter_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_program_count(unsigned short plugin_id)
{
    qDebug("get_program_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->prog.count;
    }

    qCritical("get_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_midi_program_count(unsigned short plugin_id)
{
    qDebug("get_midi_program_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->midiprog.count;
    }

    qCritical("get_midi_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_custom_data_count(unsigned short plugin_id)
{
    qDebug("get_custom_data_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->custom.count();
    }

    qCritical("get_custom_data_count(%i) - could not find plugin", plugin_id);
    return 0;
}

const char* get_program_name(unsigned short plugin_id, uint32_t program_id)
{
    qDebug("get_program_name(%i, %i)", plugin_id, program_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (program_id < plugin->prog.count)
                return plugin->prog.names[program_id];
            else
                qCritical("get_program_name(%i, %i) - program_id out of bounds", plugin_id, program_id);

            return nullptr;
        }
    }

    qCritical("get_program_name(%i, %i) - could not find plugin", plugin_id, program_id);
    return nullptr;
}

const char* get_midi_program_name(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("get_midi_program_name(%i, %i)", plugin_id, midi_program_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (midi_program_id < plugin->midiprog.count)
                return plugin->midiprog.names[midi_program_id];
            else
                qCritical("get_midi_program_name(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);

            return nullptr;
        }
    }

    qCritical("get_midi_program_name(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return nullptr;
}

const char* get_real_plugin_name(unsigned short plugin_id)
{
    qDebug("get_real_plugin_name(%i)", plugin_id);

    static const char* real_plugin_name = nullptr;

    if (real_plugin_name)
        free((void*)real_plugin_name);

    real_plugin_name = nullptr;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->get_real_name(buf_str);
            real_plugin_name = strdup(buf_str);

            return real_plugin_name;
        }
    }

    if (carla_is_engine_running())
        qCritical("get_real_plugin_name(%i) - could not find plugin", plugin_id);

    return real_plugin_name;
}

int32_t get_current_program_index(unsigned short plugin_id)
{
    qDebug("get_current_program_index(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->prog.current;
    }

    qCritical("get_current_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

int32_t get_current_midi_program_index(unsigned short plugin_id)
{
    qDebug("get_current_midi_program_index(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->midiprog.current;
    }

    qCritical("get_current_midi_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

double get_default_parameter_value(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_default_parameter_value(%i, %i)", plugin_id, parameter_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
                return plugin->param.ranges[parameter_id].def;
            else
                qCritical("get_default_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return 0.0;
        }
    }

    qCritical("get_default_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

double get_current_parameter_value(unsigned short plugin_id, uint32_t parameter_id)
{
    //qDebug("get_current_parameter_value(%i, %i)", plugin_id, parameter_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
            {
                double value = plugin->param.buffers[parameter_id];

                if (plugin->param.data[parameter_id].hints & PARAMETER_HAS_STRICT_BOUNDS)
                    plugin->fix_parameter_value(value, plugin->param.ranges[parameter_id]);

                return value;
            }
            else
                qCritical("get_current_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return 0.0;
        }
    }

    qCritical("get_current_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

double get_input_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (port_id == 1 || port_id == 2)
        return ains_peak[(plugin_id*2)+port_id-1];
    else
        return 0.0;
}

double get_output_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (port_id == 1 || port_id == 2)
        return aouts_peak[(plugin_id*2)+port_id-1];
    else
        return 0.0;
}

void set_active(unsigned short plugin_id, bool onoff)
{
    qDebug("set_active(%i, %s)", plugin_id, bool2str(onoff));

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_active(onoff, true, false);
    }

    qCritical("set_active(%i, %s) - could not find plugin", plugin_id, bool2str(onoff));
}

void set_drywet(unsigned short plugin_id, double value)
{
    qDebug("set_drywet(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_drywet(value, true, false);
    }

    qCritical("set_drywet(%i, %f) - could not find plugin", plugin_id, value);
}

void set_vol(unsigned short plugin_id, double value)
{
    qDebug("set_vol(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_vol(value, true, false);
    }

    qCritical("set_vol(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_left(unsigned short plugin_id, double value)
{
    qDebug("set_balance_left(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_balance_left(value, true, false);
    }

    qCritical("set_balance_left(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_right(unsigned short plugin_id, double value)
{
    qDebug("set_balance_right(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_balance_right(value, true, false);
    }

    qCritical("set_balance_right(%i, %f) - could not find plugin", plugin_id, value);
}

void set_parameter_value(unsigned short plugin_id, uint32_t parameter_id, double value)
{
    qDebug("set_parameter_value(%i, %i, %f)", plugin_id, parameter_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
                plugin->set_parameter_value(parameter_id, value, true, true, false);
            else
                qCritical("set_parameter_value(%i, %i, %f) - parameter_id out of bounds", plugin_id, parameter_id, value);

            return;
        }
    }

    qCritical("set_parameter_value(%i, %i, %f) - could not find plugin", plugin_id, parameter_id, value);
}

void set_parameter_midi_channel(unsigned short plugin_id, uint32_t parameter_id, uint8_t channel)
{
    qDebug("set_parameter_midi_channel(%i, %i, %i)", plugin_id, parameter_id, channel);

    if (channel > 15)
    {
        qCritical("set_parameter_midi_channel(%i, %i, %i) - invalid channel number", plugin_id, parameter_id, channel);
        return;
    }

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
            {
                plugin->param.data[parameter_id].midi_channel = channel;

                if (plugin->hints & PLUGIN_IS_BRIDGE)
                    osc_send_set_parameter_midi_channel(&plugin->osc.data, plugin->id, parameter_id, channel);
            }
            else
                qCritical("set_parameter_midi_channel(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, channel);

            return;
        }
    }

    qCritical("set_parameter_midi_channel(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, channel);
}

void set_parameter_midi_cc(unsigned short plugin_id, uint32_t parameter_id, int16_t midi_cc)
{
    qDebug("set_parameter_midi_cc(%i, %i, %i)", plugin_id, parameter_id, midi_cc);

    if (midi_cc < -1)
    {
        midi_cc = -1;
    }
    else if (midi_cc > 0x5F) // 95
    {
        qCritical("set_parameter_midi_cc(%i, %i, %i) - invalid midi_cc number", plugin_id, parameter_id, midi_cc);
        return;
    }

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (parameter_id < plugin->param.count)
            {
                plugin->param.data[parameter_id].midi_cc = midi_cc;

                if (plugin->hints & PLUGIN_IS_BRIDGE)
                    osc_send_set_parameter_midi_cc(&plugin->osc.data, plugin->id, parameter_id, midi_cc);
            }
            else
                qCritical("set_parameter_midi_cc(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, midi_cc);

            return;
        }
    }

    qCritical("set_parameter_midi_cc(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, midi_cc);
}

void set_program(unsigned short plugin_id, uint32_t program_id)
{
    qDebug("set_program(%i, %i)", plugin_id, program_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (program_id < plugin->prog.count)
                plugin->set_program(program_id, true, true, false, true);
            else
                qCritical("set_program(%i, %i) - program_id out of bounds", plugin_id, program_id);

            return;
        }
    }

    qCritical("set_program(%i, %i) - could not find plugin", plugin_id, program_id);
}

void set_midi_program(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("set_midi_program(%i, %i)", plugin_id, midi_program_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (midi_program_id < plugin->midiprog.count)
                plugin->set_midi_program(midi_program_id, true, true, false, true);
            else
                qCritical("set_midi_program(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);

            return;
        }
    }

    qCritical("set_midi_program(%i, %i) - could not find plugin", plugin_id, midi_program_id);
}

void set_custom_data(unsigned short plugin_id, CustomDataType type, const char* key, const char* value)
{
    qDebug("set_custom_data(%i, %i, %s, %s)", plugin_id, type, key, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->set_custom_data(type, key, value, true);
    }

    qCritical("set_custom_data(%i, %i, %s, %s) - could not find plugin", plugin_id, type, key, value);
}

void set_chunk_data(unsigned short plugin_id, const char* chunk_data)
{
    qDebug("set_chunk_data(%i, %s)", plugin_id, chunk_data);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (plugin->hints & PLUGIN_USES_CHUNKS)
                plugin->set_chunk_data(chunk_data);
            else
                qCritical("set_chunk_data(%i, %s) - plugin does not support chunks", plugin_id, chunk_data);

            return;
        }
    }

    qCritical("set_chunk_data(%i, %s) - could not find plugin", plugin_id, chunk_data);
}

void set_gui_data(unsigned short plugin_id, int data, intptr_t gui_addr)
{
    qDebug("set_gui_data(%i, %i, " P_INTPTR ")", plugin_id, data, gui_addr);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            if (plugin->gui.type != GUI_NONE)
                plugin->set_gui_data(data, get_pointer(gui_addr));
            else
                qCritical("set_gui_data(%i, %i, " P_INTPTR ") - plugin has no UI", plugin_id, data, gui_addr);

            return;
        }
    }

    qCritical("set_gui_data(%i, %i, " P_INTPTR ") - could not find plugin", plugin_id, data, gui_addr);
}

void show_gui(unsigned short plugin_id, bool yesno)
{
    qDebug("show_gui(%i, %s)", plugin_id, bool2str(yesno));

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            plugin->show_gui(yesno);
            return;
        }
    }

    qCritical("show_gui(%i, %s) - could not find plugin", plugin_id, bool2str(yesno));
}

void idle_gui(unsigned short plugin_id)
{
    qDebug("idle_gui(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
        {
            plugin->idle_gui();
            return;
        }
    }

    qCritical("idle_gui(%i) - could not find plugin", plugin_id);
}

void send_midi_note(unsigned short plugin_id, bool onoff, uint8_t note, uint8_t velocity)
{
    qDebug("send_midi_note(%i, %s, %i, %i)", plugin_id, bool2str(onoff), note, velocity);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return send_plugin_midi_note(plugin_id, onoff, note, velocity, true, true, false);
    }

    qCritical("send_midi_note(%i, %s, %i, %i) - could not find plugin", plugin_id, bool2str(onoff), note, velocity);
}

void prepare_for_save(unsigned short plugin_id)
{
    qDebug("prepare_for_save(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        AudioPlugin* plugin = AudioPlugins[i];
        if (plugin && plugin->id == plugin_id)
            return plugin->prepare_for_save();
    }

    qCritical("prepare_for_save(%i) - could not find plugin", plugin_id);
}

void set_callback_function(CallbackFunc func)
{
    qDebug("set_callback_function(%p)", func);
    Callback = func;
}

void set_option(OptionsType option, int value, const char* value_str)
{
    qDebug("set_option(%i, %i, %s)", option, value, value_str);

    if (carla_options._initiated)
        return;

    switch(option)
    {
    case OPTION_GLOBAL_JACK_CLIENT:
        carla_options.global_jack_client = value;
        break;
    case OPTION_BRIDGE_PATH_LV2_GTK2:
        if (value_str) carla_options.bridge_path_lv2_gtk2 = strdup(value_str);
        break;
    case OPTION_BRIDGE_PATH_LV2_QT4:
        if (value_str) carla_options.bridge_path_lv2_qt4 = strdup(value_str);
        break;
    case OPTION_BRIDGE_PATH_LV2_X11:
        if (value_str) carla_options.bridge_path_lv2_x11 = strdup(value_str);
        break;
    case OPTION_BRIDGE_PATH_VST_QT4:
        if (value_str) carla_options.bridge_path_vst_qt4 = strdup(value_str);
        break;
    case OPTION_BRIDGE_PATH_WINVST:
        if (value_str) carla_options.bridge_path_winvst = strdup(value_str);
        break;
    default:
        break;
    }
}

const char* get_last_error()
{
    qDebug("get_last_error()");
    return last_error;
}

const char* get_host_client_name()
{
    qDebug("get_host_client_name()");
    return carla_client_name;
}

const char* get_host_osc_url()
{
    qDebug("get_host_osc_url()");
    return global_osc_server_path;
}

uint32_t get_buffer_size()
{
    qDebug("get_buffer_size()");
    return carla_buffer_size;
}

double get_sample_rate()
{
    qDebug("get_sample_rate()");
    return carla_sample_rate;
}

double get_latency()
{
    qDebug("get_latency()");
    return double(carla_buffer_size)/carla_sample_rate*1000;
}
// End of exported symbols (API)
// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
// Helper functions

short get_new_plugin_id()
{
    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        if (!AudioPlugins[i])
            return i;
    }

    return -1;
}

const char* get_unique_name(const char* name)
{
    QString qname(name);
    qname.replace(":", "."); // ":" is used in JACK to split client/port names

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        // Check if unique name already exists
        if (unique_names[i] && qname == unique_names[i])
        {
            // Check if string has already been modified
            uint len = qname.size();

            if (qname.at(len-3) == QChar('(') && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                int number = qname.at(len-2).toAscii()-'0';

                if (number == 9)
                    // next number is 10, 2 digits
                    qname.replace(" (9)", " (10)");
                else
                    qname[len-2] = QChar('0'+number+1);

                continue;
            }
            else if (qname.at(len-4) == QChar('(') && qname.at(len-3).isDigit() && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                QChar n2 = qname.at(len-2); // (1x)
                QChar n3 = qname.at(len-3); // (x0)

                if (n2 == QChar('9'))
                {
                    n2 = QChar('0');
                    n3 = QChar(n3.toAscii()+1);
                } else
                    n2 = QChar(n2.toAscii()+1);

                qname[len-2] = n2;
                qname[len-3] = n3;

                continue;
            }

            // Modify string if not
            qname += " (2)";
        }
    }

    return strdup(qname.toStdString().data());
}

void* get_pointer(intptr_t ptr_addr)
{
    intptr_t* ptr = (intptr_t*)ptr_addr;
    return (void*)ptr;
}

void set_last_error(const char* error)
{
    if (last_error)
        free((void*)last_error);

    last_error = strdup(error);
}

void callback_action(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3)
{
    if (Callback)
        Callback(action, plugin_id, value1, value2, value3);
}

void carla_proc_lock()
{
    carla_proc_lock_var.lock();
}

bool carla_proc_trylock()
{
    return carla_proc_lock_var.tryLock();
}

void carla_proc_unlock()
{
    carla_proc_lock_var.unlock();
}

void carla_midi_lock()
{
    carla_midi_lock_var.lock();
}

void carla_midi_unlock()
{
    carla_midi_lock_var.unlock();
}

void send_plugin_midi_note(unsigned short plugin_id, bool onoff, uint8_t note, uint8_t velo, bool gui_send, bool osc_send, bool callback_send)
{
    carla_midi_lock();
    for (unsigned int i=0; i<MAX_MIDI_EVENTS; i++)
    {
        if (ExternalMidiNotes[i].valid == false)
        {
            ExternalMidiNotes[i].valid = true;
            ExternalMidiNotes[i].plugin_id = plugin_id;
            ExternalMidiNotes[i].onoff = onoff;
            ExternalMidiNotes[i].note = note;
            ExternalMidiNotes[i].velo = velo;
            break;
        }
    }
    carla_midi_unlock();

    if (gui_send)
    {
        // TODO - send midi note to GUI?
    }

    if (osc_send)
    {
        if (onoff)
            osc_send_note_on(&AudioPlugins[plugin_id]->osc.data, plugin_id, note, velo);
        else
            osc_send_note_off(&AudioPlugins[plugin_id]->osc.data, plugin_id, note, velo);
    }

    if (callback_send)
        callback_action(onoff ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, plugin_id, note, velo, 0.0);
}
// End of helper functions
// -------------------------------------------------------------------------------------------------------------------

#endif

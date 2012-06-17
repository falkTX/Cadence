/*
 * Carla Backend
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
#include "carla_engine.h"
#include "carla_plugin.h"
#include "carla_threads.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// plugin specific
short add_plugin_ladspa(const char* filename, const char* label, const void* extra_stuff);
short add_plugin_dssi(const char* filename, const char* label, const void* extra_stuff);
short add_plugin_lv2(const char* filename, const char* label);
short add_plugin_vst(const char* filename, const char* label);
short add_plugin_gig(const char* filename, const char* label);
short add_plugin_sf2(const char* filename, const char* label);
short add_plugin_sfz(const char* filename, const char* label);
#ifndef BUILD_BRIDGE
short add_plugin_bridge(BinaryType btype, PluginType ptype, const char* filename, const char* label);
#endif

CarlaEngine carla_engine;
CarlaCheckThread carla_check_thread;

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)

bool engine_init(const char* client_name)
{
    qDebug("carla_backend_init(%s)", client_name);

    bool started = carla_engine.init(client_name);

    if (started)
    {
        osc_init(get_host_client_name());
        carla_check_thread.start(QThread::HighPriority);
        set_last_error("no error");
    }

    return started;
}

bool engine_close()
{
    qDebug("carla_backend_close()");

    bool closed = carla_engine.close();

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        if (CarlaPlugins[i])
            remove_plugin(i);
    }

    if (carla_check_thread.isRunning())
        carla_check_thread.stopNow();

    osc_global_send_exit();
    osc_close();

    // cleanup static data
    get_plugin_info(0);
    get_parameter_info(0, 0);
    get_scalepoint_info(0, 0, 0);
    get_chunk_data(0);
    get_program_name(0, 0);
    get_midi_program_name(0, 0);
    get_real_plugin_name(0);
    set_last_error(nullptr);

    if (carla_options.bridge_unix32)
        free((void*)carla_options.bridge_unix32);

    if (carla_options.bridge_unix64)
        free((void*)carla_options.bridge_unix64);

    if (carla_options.bridge_win32)
        free((void*)carla_options.bridge_win32);

    if (carla_options.bridge_win64)
        free((void*)carla_options.bridge_win64);

    if (carla_options.bridge_lv2gtk2)
        free((void*)carla_options.bridge_lv2gtk2);

    if (carla_options.bridge_lv2qt4)
        free((void*)carla_options.bridge_lv2qt4);

    if (carla_options.bridge_lv2x11)
        free((void*)carla_options.bridge_lv2x11);

    carla_options.bridge_unix32  = nullptr;
    carla_options.bridge_unix64  = nullptr;
    carla_options.bridge_win32   = nullptr;
    carla_options.bridge_win64   = nullptr;
    carla_options.bridge_lv2gtk2 = nullptr;
    carla_options.bridge_lv2qt4  = nullptr;
    carla_options.bridge_lv2x11  = nullptr;

    return closed;
}

short add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin(%i, %i, %s, %s, %p)", btype, ptype, filename, label, extra_stuff);

#ifndef BUILD_BRIDGE
    if (btype != BINARY_NATIVE)
    {
        if (carla_options.global_jack_client)
        {
            set_last_error("Cannot use bridged plugins while in global client mode");
            return -1;
        }

        return add_plugin_bridge(btype, ptype, filename, label);
    }
#endif

    switch (ptype)
    {
    case PLUGIN_LADSPA:
        return add_plugin_ladspa(filename, label, extra_stuff);
    case PLUGIN_DSSI:
        return add_plugin_dssi(filename, label, extra_stuff);
    case PLUGIN_LV2:
        return add_plugin_lv2(filename, label);
    case PLUGIN_VST:
        return add_plugin_vst(filename, label);
    case PLUGIN_GIG:
        return add_plugin_gig(filename, label);
    case PLUGIN_SF2:
        return add_plugin_sf2(filename, label);
    case PLUGIN_SFZ:
        return add_plugin_sfz(filename, label);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            osc_global_send_remove_plugin(plugin->id());

            carla_proc_lock();
            plugin->set_enabled(false);
            carla_proc_unlock();

            if (is_engine_running() && carla_check_thread.isRunning())
                carla_check_thread.stopNow();

            delete plugin;

            CarlaPlugins[i] = nullptr;
            unique_names[i] = nullptr;

            if (is_engine_running())
                carla_check_thread.start(QThread::HighPriority);

            return true;
        }
    }

    qCritical("remove_plugin(%i) - could not find plugin", plugin_id);
    set_last_error("Could not find plugin to remove");
    return false;
}

PluginInfo* get_plugin_info(unsigned short plugin_id)
{
    qDebug("get_plugin_info(%i)", plugin_id);

    static PluginInfo info = { false, PLUGIN_NONE, PLUGIN_CATEGORY_NONE, 0x0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 };

    if (info.valid)
    {
        free((void*)info.label);
        free((void*)info.maker);
        free((void*)info.copyright);
    }

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            char buf_str[STR_MAX] = { 0 };

            info.valid     = true;
            info.type      = plugin->type();
            info.category  = plugin->category();
            info.hints     = plugin->hints();
            info.binary    = plugin->filename();
            info.name      = plugin->name();
            info.unique_id = plugin->unique_id();

            plugin->get_label(buf_str);
            info.label = strdup(buf_str);

            plugin->get_maker(buf_str);
            info.maker = strdup(buf_str);

            plugin->get_copyright(buf_str);
            info.copyright = strdup(buf_str);

            return &info;
        }
    }

    if (is_engine_running())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            info.valid = true;
            info.ins   = plugin->ain_count();
            info.outs  = plugin->aout_count();
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            info.valid = true;
            info.ins   = plugin->min_count();
            info.outs  = plugin->mout_count();
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            info.valid = true;
            plugin->get_parameter_count_info(&info);
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
        free((void*)info.unit);
    }

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
            {
                char buf_str[STR_MAX] = { 0 };

                info.valid = true;
                info.scalepoint_count = plugin->param_scalepoint_count(parameter_id);

                plugin->get_parameter_name(parameter_id, buf_str);
                info.name = strdup(buf_str);

                plugin->get_parameter_symbol(parameter_id, buf_str);
                info.symbol = strdup(buf_str);

                plugin->get_parameter_unit(parameter_id, buf_str);
                info.unit = strdup(buf_str);
            }
            else
                qCritical("get_parameter_info(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &info;
        }
    }

    if (is_engine_running())
        qCritical("get_parameter_info(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return &info;
}

ScalePointInfo* get_scalepoint_info(unsigned short plugin_id, uint32_t parameter_id, uint32_t scalepoint_id)
{
    qDebug("get_scalepoint_info(%i, %i, %i)", plugin_id, parameter_id, scalepoint_id);

    static ScalePointInfo info = { false, 0.0, nullptr };

    if (info.valid)
        free((void*)info.label);

    info.valid = false;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
            {
                if (scalepoint_id < plugin->param_scalepoint_count(parameter_id))
                {
                    char buf_str[STR_MAX] = { 0 };

                    info.valid = true;
                    info.value = plugin->get_parameter_scalepoint_value(parameter_id, scalepoint_id);

                    plugin->get_parameter_scalepoint_label(parameter_id, scalepoint_id, buf_str);
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

    if (is_engine_running())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (midi_program_id < plugin->midiprog_count())
            {
                info.valid   = true;
                plugin->get_midi_program_info(&info, midi_program_id);
            }
            else
                qCritical("get_midi_program_info(%i, %i) - midi_program_id out of bounds", plugin_id, midi_program_id);

            return &info;
        }
    }

    qCritical("get_midi_program_info(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return &info;
}

GuiInfo* get_gui_info(unsigned short plugin_id)
{
    qDebug("get_gui_info(%i)", plugin_id);

    static GuiInfo info = { GUI_NONE, false };
    info.type = GUI_NONE;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            plugin->get_gui_info(&info);
            return &info;
        }
    }

    qCritical("get_gui_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const ParameterData* get_parameter_data(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_parameter_data(%i, %i)", plugin_id, parameter_id);

    static ParameterData data = { PARAMETER_UNKNOWN, -1, -1, 0, 0, -1 };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                return plugin->param_data(parameter_id);
            else
                qCritical("get_parameter_data(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &data;
        }
    }

    qCritical("get_parameter_data(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &data;
}

const ParameterRanges* get_parameter_ranges(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_parameter_ranges(%i, %i)", plugin_id, parameter_id);

    static ParameterRanges ranges = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                return plugin->param_ranges(parameter_id);
            else
                qCritical("get_parameter_ranges(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            return &ranges;
        }
    }

    qCritical("get_parameter_ranges(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &ranges;
}

const CustomData* get_custom_data(unsigned short plugin_id, uint32_t custom_data_id)
{
    qDebug("get_custom_data(%i, %i)", plugin_id, custom_data_id);

    static CustomData data = { CUSTOM_DATA_INVALID, nullptr, nullptr };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (custom_data_id < plugin->custom_count())
                return plugin->custom_data(custom_data_id);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (plugin->hints() & PLUGIN_USES_CHUNKS)
            {
                void* data = nullptr;
                int32_t data_size = plugin->chunk_data(&data);

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

    if (is_engine_running())
        qCritical("get_chunk_data(%i) - could not find plugin", plugin_id);

    return chunk_data;
}

uint32_t get_parameter_count(unsigned short plugin_id)
{
    qDebug("get_parameter_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->param_count();
    }

    qCritical("get_parameter_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_program_count(unsigned short plugin_id)
{
    qDebug("get_program_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->prog_count();
    }

    qCritical("get_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_midi_program_count(unsigned short plugin_id)
{
    qDebug("get_midi_program_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->midiprog_count();
    }

    qCritical("get_midi_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_custom_data_count(unsigned short plugin_id)
{
    qDebug("get_custom_data_count(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->custom_count();
    }

    qCritical("get_custom_data_count(%i) - could not find plugin", plugin_id);
    return 0;
}

const char* get_parameter_text(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_parameter_text(%i, %i)", plugin_id, parameter_id);

    static char buf_text[STR_MAX] = { 0 };
    memset(buf_text, 0, sizeof(char)*STR_MAX);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                plugin->get_parameter_text(parameter_id, buf_text);
            else
                qCritical("get_parameter_text(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

            break;
        }
    }

    return buf_text;
}

const char* get_program_name(unsigned short plugin_id, uint32_t program_id)
{
    qDebug("get_program_name(%i, %i)", plugin_id, program_id);

    static const char* program_name = nullptr;

    if (program_name)
        free((void*)program_name);

    program_name = nullptr;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (program_id < plugin->prog_count())
            {
                char buf_str[STR_MAX] = { 0 };

                plugin->get_program_name(program_id, buf_str);
                program_name = strdup(buf_str);

                return program_name;
            }
            else
                qCritical("get_program_name(%i, %i) - program_id out of bounds", plugin_id, program_id);

            return nullptr;
        }
    }

    if (is_engine_running())
        qCritical("get_program_name(%i, %i) - could not find plugin", plugin_id, program_id);

    return nullptr;
}

const char* get_midi_program_name(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("get_midi_program_name(%i, %i)", plugin_id, midi_program_id);

    static const char* midi_program_name = nullptr;

    if (midi_program_name)
        free((void*)midi_program_name);

    midi_program_name = nullptr;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (midi_program_id < plugin->midiprog_count())
            {
                char buf_str[STR_MAX] = { 0 };

                plugin->get_midi_program_name(midi_program_id, buf_str);
                midi_program_name = strdup(buf_str);

                return midi_program_name;
            }
            else
                qCritical("get_midi_program_name(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);

            return nullptr;
        }
    }

    if (is_engine_running())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->get_real_name(buf_str);
            real_plugin_name = strdup(buf_str);

            return real_plugin_name;
        }
    }

    if (is_engine_running())
        qCritical("get_real_plugin_name(%i) - could not find plugin", plugin_id);

    return real_plugin_name;
}

qint32 get_current_program_index(unsigned short plugin_id)
{
    qDebug("get_current_program_index(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->prog_current();
    }

    qCritical("get_current_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

qint32 get_current_midi_program_index(unsigned short plugin_id)
{
    qDebug("get_current_midi_program_index(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->midiprog_current();
    }

    qCritical("get_current_midi_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

double get_default_parameter_value(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("get_default_parameter_value(%i, %i)", plugin_id, parameter_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                return plugin->param_ranges(parameter_id)->def;
            //return plugin->get_default_parameter_value(parameter_id);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                return plugin->get_parameter_value(parameter_id);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_active(onoff, true, false);
    }

    qCritical("set_active(%i, %s) - could not find plugin", plugin_id, bool2str(onoff));
}

void set_drywet(unsigned short plugin_id, double value)
{
    qDebug("set_drywet(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_drywet(value, true, false);
    }

    qCritical("set_drywet(%i, %f) - could not find plugin", plugin_id, value);
}

void set_volume(unsigned short plugin_id, double value)
{
    qDebug("set_vol(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_volume(value, true, false);
    }

    qCritical("set_vol(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_left(unsigned short plugin_id, double value)
{
    qDebug("set_balance_left(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_balance_left(value, true, false);
    }

    qCritical("set_balance_left(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_right(unsigned short plugin_id, double value)
{
    qDebug("set_balance_right(%i, %f)", plugin_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_balance_right(value, true, false);
    }

    qCritical("set_balance_right(%i, %f) - could not find plugin", plugin_id, value);
}

void set_parameter_value(unsigned short plugin_id, uint32_t parameter_id, double value)
{
    qDebug("set_parameter_value(%i, %i, %f)", plugin_id, parameter_id, value);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                plugin->set_parameter_midi_channel(parameter_id, channel);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (parameter_id < plugin->param_count())
                plugin->set_parameter_midi_cc(parameter_id, midi_cc);
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (program_id < plugin->prog_count())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (midi_program_id < plugin->midiprog_count())
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
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->set_custom_data(type, key, value, true);
    }

    qCritical("set_custom_data(%i, %i, %s, %s) - could not find plugin", plugin_id, type, key, value);
}

void set_chunk_data(unsigned short plugin_id, const char* chunk_data)
{
    qDebug("set_chunk_data(%i, %s)", plugin_id, chunk_data);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            if (plugin->hints() & PLUGIN_USES_CHUNKS)
                plugin->set_chunk_data(chunk_data);
            else
                qCritical("set_chunk_data(%i, %s) - plugin does not support chunks", plugin_id, chunk_data);

            return;
        }
    }

    qCritical("set_chunk_data(%i, %s) - could not find plugin", plugin_id, chunk_data);
}

void set_gui_data(unsigned short plugin_id, int data, quintptr gui_addr)
{
    qDebug("set_gui_data(%i, %i, " P_UINTPTR ")", plugin_id, data, gui_addr);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            plugin->set_gui_data(data, (QDialog*)get_pointer(gui_addr));
            return;
        }
    }

    qCritical("set_gui_data(%i, %i, " P_UINTPTR ") - could not find plugin", plugin_id, data, gui_addr);
}

// TESTING
//void set_name(unsigned short plugin_id, const char* name)
//{

//    for (unsigned short i=0; i<MAX_PLUGINS; i++)
//    {
//        CarlaPlugin* plugin = CarlaPlugins[i];
//        if (plugin && plugin->id() == plugin_id)
//        {
//            plugin->set_name(name);
//        }
//    }
//}

void show_gui(unsigned short plugin_id, bool yesno)
{
    qDebug("show_gui(%i, %s)", plugin_id, bool2str(yesno));

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            plugin->show_gui(yesno);
            return;
        }
    }

    qCritical("show_gui(%i, %s) - could not find plugin", plugin_id, bool2str(yesno));
}

void idle_guis()
{
    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->enabled())
            plugin->idle_gui();
    }
}

void send_midi_note(unsigned short plugin_id, uint8_t note, uint8_t velocity)
{
    qDebug("send_midi_note(%i, %i, %i)", plugin_id, note, velocity);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
        {
            plugin->send_midi_note(note, velocity, true, true, false);
            return;
        }
    }

    qCritical("send_midi_note(%i, %i, %i) - could not find plugin", plugin_id, note, velocity);
}

void prepare_for_save(unsigned short plugin_id)
{
    qDebug("prepare_for_save(%i)", plugin_id);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() == plugin_id)
            return plugin->prepare_for_save();
    }

    qCritical("prepare_for_save(%i) - could not find plugin", plugin_id);
}

void set_option(OptionsType option, int value, const char* value_str)
{
    qDebug("set_option(%i, %i, %s)", option, value, value_str);

    switch (option)
    {
    case OPTION_MAX_PARAMETERS:
        carla_options.max_parameters = (value > 0) ? value : MAX_PARAMETERS;
        break;
    case OPTION_GLOBAL_JACK_CLIENT:
        carla_options.global_jack_client = value;
        break;
    case OPTION_PREFER_UI_BRIDGES:
        carla_options.prefer_ui_bridges = value;
        break;
    case OPTION_PROCESS_HQ:
        carla_options.proccess_hq = value;
        break;
    case OPTION_OSC_GUI_TIMEOUT:
        carla_options.osc_gui_timeout = value/100;
        break;
    case OPTION_USE_DSSI_CHUNKS:
        carla_options.use_dssi_chunks = value;
        break;
    case OPTION_PATH_LADSPA:
        carla_setenv("LADSPA_PATH", value_str);
        break;
    case OPTION_PATH_DSSI:
        carla_setenv("DSSI_PATH", value_str);
        break;
    case OPTION_PATH_LV2:
        carla_setenv("LV2_PATH", value_str);
        break;
    case OPTION_PATH_VST:
        carla_setenv("VST_PATH", value_str);
        break;
    case OPTION_PATH_GIG:
        carla_setenv("GIG_PATH", value_str);
        break;
    case OPTION_PATH_SF2:
        carla_setenv("SF2_PATH", value_str);
        break;
    case OPTION_PATH_SFZ:
        carla_setenv("SFZ_PATH", value_str);
        break;
    case OPTION_PATH_BRIDGE_UNIX32:
        carla_options.bridge_unix32 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_UNIX64:
        carla_options.bridge_unix64 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_WIN32:
        carla_options.bridge_win32 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_WIN64:
        carla_options.bridge_win64 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        carla_options.bridge_lv2gtk2 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        carla_options.bridge_lv2qt4 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        carla_options.bridge_lv2x11 = strdup(value_str);
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        carla_options.bridge_vstx11 = strdup(value_str);
        break;
    }
}

// End of exported symbols (API)
// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#ifdef QTCREATOR_TEST

#include <QtGui/QApplication>
#include <QtGui/QDialog>

QDialog* gui;

#ifndef CARLA_BACKEND_NO_NAMESPACE
using namespace CarlaBackend;
#endif

void main_callback(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3)
{
    qDebug("Callback(%i, %u, %i, %i, %f)", action, plugin_id, value1, value2, value3);

    switch (action)
    {
    case CALLBACK_SHOW_GUI:
        if (! value1)
            gui->close();
        break;
    case CALLBACK_RESIZE_GUI:
        gui->setFixedSize(value1, value2);
        break;
    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    gui = new QDialog(nullptr);

    if (engine_init("carla_demo"))
    {
        set_callback_function(main_callback);
        short id = add_plugin_sfz("/home/falktx/Personal/Muzyks/Kits/SFZ/sonatina/Sonatina Symphonic Orchestra/Strings - 1st Violins Sustain.sfz", "xaxaxa");

        if (id >= 0)
        {
            qDebug("Main Initiated, id = %u", id);

            const GuiInfo* guiInfo = get_gui_info(id);

            if (guiInfo->type == GUI_INTERNAL_QT4 || guiInfo->type == GUI_INTERNAL_X11)
            {
                set_gui_data(id, 0, (quintptr)gui);
                gui->show();
            }

            set_active(id, true);
            show_gui(id, true);
            app.exec();

            remove_plugin(id);
        }
        else
            qCritical("failed: %s", get_last_error());

        engine_close();
    }
    else
        qCritical("failed to start backend engine");

    delete gui;
    return 0;
}

#endif

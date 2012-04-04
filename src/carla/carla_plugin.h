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

#ifndef CARLA_PLUGIN_H
#define CARLA_PLUGIN_H

#include "carla_jack.h"
#include "carla_osc.h"
#include "carla_shared.h"

#ifndef BUILD_BRIDGE
#include "carla_threads.h"
#endif

// common includes
#include <cmath>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QString>

// TODO - check and try '#ifndef BUILD_BRIDGE' in all global_jack_client

#define CARLA_PROCESS_CONTINUE_CHECK if (m_id != plugin_id) { return callback_action(CALLBACK_DEBUG, plugin_id, m_id, 0, 0.0); }

const unsigned short MAX_POST_EVENTS = 128;

enum PluginPostEventType {
    PostEventDebug,
    PostEventParameterChange,
    PostEventProgramChange,
    PostEventMidiProgramChange,
    PostEventNoteOn,
    PostEventNoteOff
};

enum PluginBridgeInfoType {
    PluginBridgeAudioCountInfo,
    PluginBridgeMidiCountInfo,
    PluginBridgeParameterCountInfo,
    PluginBridgeProgramCountInfo,
    PluginBridgeMidiProgramCountInfo,
    PluginBridgePluginInfo,
    PluginBridgeParameterInfo,
    PluginBridgeParameterDataInfo,
    PluginBridgeParameterRangesInfo,
    PluginBridgeProgramName,
    PluginBridgeUpdateNow
};

struct midi_program_t {
    uint32_t bank;
    uint32_t program;
    const char* name;
};

struct PluginAudioData {
    uint32_t count;
    jack_port_t** ports;
};

struct PluginMidiData {
    jack_port_t* port_min;
    jack_port_t* port_mout;
};

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    jack_port_t* port_cin;
    jack_port_t* port_cout;
};

struct PluginProgramData {
    uint32_t count;
    int32_t current;
    const char** names;
};

struct PluginMidiProgramData {
    uint32_t count;
    int32_t current;
    midi_program_t* data;
};

struct PluginPostEvent {
    bool valid;
    PluginPostEventType type;
    int32_t index;
    double value;
};

struct ExternalMidiNote {
    bool valid;
    bool onoff;
    uint8_t note;
    uint8_t velo;
};

class CarlaPlugin
{
public:
    CarlaPlugin()
    {
        qDebug("CarlaPlugin::CarlaPlugin()");

        m_type = PLUGIN_NONE;
        m_id   = -1;
        m_hints = 0;

        m_active = false;
        m_active_before = false;

        m_lib  = nullptr;
        m_name = nullptr;
        m_filename = nullptr;

        x_drywet = 1.0;
        x_vol    = 1.0;
        x_bal_left = -1.0;
        x_bal_right = 1.0;

        jack_client = nullptr;

        ain.count = 0;
        ain.ports = nullptr;

        aout.count = 0;
        aout.ports = nullptr;

        midi.port_min  = nullptr;
        midi.port_mout = nullptr;

        param.count  = 0;
        param.data   = nullptr;
        param.ranges = nullptr;
        param.port_cin  = nullptr;
        param.port_cout = nullptr;

        prog.count   = 0;
        prog.current = -1;
        prog.names   = nullptr;

        midiprog.count   = 0;
        midiprog.current = -1;
        midiprog.data    = nullptr;

        custom.clear();

#ifndef BUILD_BRIDGE
        osc.data.path = nullptr;
        osc.data.source = nullptr;
        osc.data.target = nullptr;
        osc.thread = nullptr;
#endif

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            post_events.data[i].valid = false;

        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
            ext_midi_notes[i].valid = false;
    }

    virtual ~CarlaPlugin()
    {
        qDebug("CarlaPlugin::~CarlaPlugin()");

        // FIXME - reorder these calls?

        // Unregister jack client and ports
        remove_from_jack();

        // Delete data
        delete_buffers();

        if (prog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
                free((void*)prog.names[i]);

            delete[] prog.names;
        }

        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
                free((void*)midiprog.data[i].name);

            delete[] midiprog.data;
        }

        if (custom.count() > 0)
        {
            for (int i=0; i < custom.count(); i++)
            {
                if (custom[i].key)
                    free((void*)custom[i].key);

                if (custom[i].value)
                    free((void*)custom[i].value);
            }

            custom.clear();
        }

        lib_close();

        if (m_name)
            free((void*)m_name);

        if (m_filename)
            free((void*)m_filename);

        if (jack_client && carla_options.global_jack_client == false)
            jack_client_close(jack_client);
    }

    PluginType type()
    {
        return m_type;
    }

    short id()
    {
        return m_id;
    }

    unsigned int hints()
    {
        return m_hints;
    }

    const char* name()
    {
        return m_name;
    }

    const char* filename()
    {
        return m_filename;
    }

    virtual PluginCategory category()
    {
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return 0;
    }

    virtual uint32_t ain_count()
    {
        return ain.count;
    }

    virtual uint32_t aout_count()
    {
        return aout.count;
    }

    virtual uint32_t min_count()
    {
        return (midi.port_min) ? 1 : 0;
    }

    virtual uint32_t mout_count()
    {
        return (midi.port_mout) ? 1 : 0;
    }

    uint32_t param_count()
    {
        return param.count;
    }

    virtual uint32_t param_scalepoint_count(uint32_t /*param_id*/)
    {
        return 0;
    }

    uint32_t custom_count()
    {
        return custom.count();
    }

    uint32_t prog_count()
    {
        return prog.count;
    }

    uint32_t midiprog_count()
    {
        return midiprog.count;
    }

    int32_t prog_current()
    {
        return prog.current;
    }

    int32_t midiprog_current()
    {
        return midiprog.current;
    }

    ParameterData* param_data(uint32_t index)
    {
        return &param.data[index];
    }

    ParameterRanges* param_ranges(uint32_t index)
    {
        return &param.ranges[index];
    }

    CustomData* custom_data(uint32_t index)
    {
        return &custom[index];
    }

    virtual int32_t chunk_data(void** /*data_ptr*/)
    {
        return 0;
    }

#ifndef BUILD_BRIDGE
    OscData* osc_data()
    {
        return &osc.data;
    }
#endif

    virtual double get_parameter_value(uint32_t /*param_id*/)
    {
        return 0.0;
    }

    virtual double get_parameter_scalepoint_value(uint32_t /*param_id*/, uint32_t /*scalepoint_id*/)
    {
        return 0.0;
    }

    // FIXME - remove this?
//    double get_default_parameter_value(uint32_t param_id)
//    {
//        return param.ranges[param_id].def;
//    }

    virtual void get_label(char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_maker(char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_copyright(char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_real_name(char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_parameter_name(uint32_t /*param_id*/, char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_parameter_symbol(uint32_t /*param_id*/, char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_parameter_label(uint32_t /*param_id*/, char* buf_str)
    {
        *buf_str = 0;
    }

    virtual void get_parameter_scalepoint_label(uint32_t /*param_id*/, uint32_t /*scalepoint_id*/, char* buf_str)
    {
        *buf_str = 0;
    }

    void get_program_name(uint32_t program_id, char* buf_str)
    {
        strncpy(buf_str, prog.names[program_id], STR_MAX);
    }

    void get_midi_program_name(uint32_t midiprogram_id, char* buf_str)
    {
        strncpy(buf_str, midiprog.data[midiprogram_id].name, STR_MAX);
    }

    void get_parameter_count_info(PortCountInfo* info)
    {
        info->ins   = 0;
        info->outs  = 0;
        info->total = param.count;

        for (uint32_t i=0; i < param.count; i++)
        {
            if (param.data[i].type == PARAMETER_INPUT)
                info->ins += 1;
            else if (param.data[i].type == PARAMETER_OUTPUT)
                info->outs += 1;
        }
    }

    void get_midi_program_info(MidiProgramInfo* info, uint32_t index)
    {
        info->bank    = midiprog.data[index].bank;
        info->program = midiprog.data[index].program;
        info->label   = midiprog.data[index].name;
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        info->type = GUI_NONE;
    }

    void set_id(short id)
    {
        m_id = id;
    }

    void set_active(bool active, bool osc_send, bool callback_send)
    {
        m_active = active;
        double value = active ? 1.0 : 0.0;

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, PARAMETER_ACTIVE, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_ACTIVE, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_ACTIVE, 0, value);
    }

    void set_drywet(double value, bool osc_send, bool callback_send)
    {
        if (value < 0.0)
            value = 0.0;
        else if (value > 1.0)
            value = 1.0;

        x_drywet = value;

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, PARAMETER_DRYWET, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_DRYWET, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_DRYWET, 0, value);
    }

    void set_volume(double value, bool osc_send, bool callback_send)
    {
        if (value < 0.0)
            value = 0.0;
        else if (value > 1.27)
            value = 1.27;

        x_vol = value;

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, PARAMETER_VOLUME, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_VOLUME, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_VOLUME, 0, value);
    }

    void set_balance_left(double value, bool osc_send, bool callback_send)
    {
        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_bal_left = value;

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, PARAMETER_BALANCE_LEFT, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_BALANCE_LEFT, 0, value);
    }

    void set_balance_right(double value, bool osc_send, bool callback_send)
    {
        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_bal_right = value;

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, PARAMETER_BALANCE_RIGHT, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_BALANCE_RIGHT, 0, value);
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool /*gui_send*/, bool osc_send, bool callback_send)
    {
#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, index, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, param_id, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, param_id, 0, value);
    }

    void set_parameter_midi_channel(uint32_t index, uint8_t channel)
    {
        param.data[index].midi_channel = channel;

#ifndef BUILD_BRIDGE
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_channel(&osc.data, m_id, index, channel);
#endif
    }

    void set_parameter_midi_cc(uint32_t index, int16_t midi_cc)
    {
        param.data[index].midi_cc = midi_cc;

#ifndef BUILD_BRIDGE
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_cc(&osc.data, m_id, index, midi_cc);
#endif
    }

    virtual void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool)
    {
        qDebug("set_custom_data(%i, %s, %s)", dtype, key, value);

        bool save_data = true;
        bool already_have = false;

        switch (dtype)
        {
        case CUSTOM_DATA_INVALID:
            save_data = false;
            break;
        case CUSTOM_DATA_STRING:
            // Ignore some keys
            if (strncmp(key, "OSC:", 4) == 0 || strcmp(key, "guiVisible") == 0)
                save_data = false;
            break;
        default:
            break;
        }

        if (save_data)
        {
            // Check if we already have this key
            for (int i=0; i < custom.count(); i++)
            {
                if (strcmp(custom[i].key, key) == 0)
                {
                    free((void*)custom[i].value);
                    custom[i].value = strdup(value);
                    already_have = true;
                    break;
                }
            }

            if (already_have == false)
            {
                CustomData new_data;
                new_data.type  = dtype;
                new_data.key   = strdup(key);
                new_data.value = strdup(value);
                custom.append(new_data);
            }
        }
    }

    virtual void set_chunk_data(const char*)
    {
    }

    virtual void set_program(int32_t index, bool, bool osc_send, bool callback_send, bool)
    {
        prog.current = index;

        // Change default value
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = get_parameter_value(i);

#ifndef BUILD_BRIDGE
            //if (osc_send)
            //    osc_send_set_default_value(&global_osc_data, m_id, i, param.ranges[i].def);
#endif
        }

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_program(&global_osc_data, m_id, prog.current);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_program(&osc.data, prog.current);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PROGRAM_CHANGED, m_id, prog.current, 0, 0.0);
    }

    virtual void set_midi_program(int32_t index, bool, bool osc_send, bool callback_send, bool)
    {
        midiprog.current = index;

        // Change default value
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = get_parameter_value(i);

#ifndef BUILD_BRIDGE
            //if (osc_send)
            //    osc_send_set_default_value(&global_osc_data, m_id, i, param.ranges[i].def);
#endif
        }

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_midi_program(&global_osc_data, m_id, midiprog.current);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_program(&osc.data, midiprog.current);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, midiprog.current, 0, 0.0);
    }

    virtual void set_gui_data(int /*data*/, void* /*ptr*/)
    {
    }

    virtual void show_gui(bool /*yesno*/)
    {
    }

    virtual void idle_gui()
    {
    }

    virtual void reload()
    {
    }

    virtual void reload_programs(bool)
    {
    }

    virtual void prepare_for_save()
    {
    }

    virtual void process(jack_nframes_t)
    {
    }

    virtual void buffer_size_changed(jack_nframes_t)
    {
    }

    void send_midi_note(bool onoff, uint8_t note, uint8_t velo, bool, bool osc_send, bool callback_send)
    {
        carla_midi_lock();
        for (unsigned int i=0; i<MAX_MIDI_EVENTS; i++)
        {
            if (ext_midi_notes[i].valid == false)
            {
                ext_midi_notes[i].valid = true;
                ext_midi_notes[i].onoff = onoff;
                ext_midi_notes[i].note = note;
                ext_midi_notes[i].velo = velo;
                break;
            }
        }
        carla_midi_unlock();

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //if (onoff)
            //    osc_send_note_on(&global_osc_data, m_id, note, velo);
            //else
            //    osc_send_note_off(&global_osc_data, m_id, note);

            //if (m_hints & PLUGIN_IS_BRIDGE)
            //{
            //    if (onoff)
            //        osc_send_note_on(&osc.data, m_id, note, velo);
            //    else
            //        osc_send_note_off(&osc.data, m_id, note);
            //}
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(onoff ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, m_id, note, velo, 0.0);
    }

    void postpone_event(PluginPostEventType type, int32_t index, double value)
    {
        post_events.lock.lock();

        for (unsigned short i=0; i<MAX_POST_EVENTS; i++)
        {
            if (post_events.data[i].valid == false)
            {
                post_events.data[i].valid = true;
                post_events.data[i].type  = type;
                post_events.data[i].index = index;
                post_events.data[i].value = value;
                break;
            }
        }

        post_events.lock.unlock();
    }

    void post_events_copy(PluginPostEvent* post_events_dst)
    {
        post_events.lock.lock();

        // FIXME?
        memcpy(post_events_dst, post_events.data, sizeof(PluginPostEvent)*MAX_POST_EVENTS);

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            post_events.data[i].valid = false;

        post_events.lock.unlock();
    }

    virtual int set_osc_bridge_info(PluginBridgeInfoType, lo_arg**)
    {
        return 1;
    }

#ifndef BUILD_BRIDGE
    void update_osc_data(lo_address source, const char* url)
    {
        const char* host;
        const char* port;

        osc_clear_data(&osc.data);

        host = lo_address_get_hostname(source);
        port = lo_address_get_port(source);
        osc.data.source = lo_address_new(host, port);

        host = lo_url_get_hostname(url);
        port = lo_url_get_port(url);

        osc.data.path = lo_url_get_path(url);
        osc.data.target = lo_address_new(host, port);

        free((void*)host);
        free((void*)port);

        for (int i=0; i < custom.count(); i++)
        {
            //if (plugin->custom[i].type == CUSTOM_DATA_STRING)
            osc_send_configure(&osc.data, custom.at(i).key, custom.at(i).value);
        }

        if (prog.current >= 0)
            osc_send_program(&osc.data, prog.current);

        if (midiprog.current >= 0)
        {
            int32_t midi_id = midiprog.current;

            if (m_type == PLUGIN_DSSI)
                osc_send_program_as_midi(&osc.data, midiprog.data[midi_id].bank, midiprog.data[midi_id].program);
            else
                osc_send_midi_program(&osc.data, midiprog.data[midi_id].bank, midiprog.data[midi_id].program);
        }

        for (uint32_t i=0; i < param.count; i++)
            osc_send_control(&osc.data, param.data[i].rindex, get_parameter_value(i));

        if (m_hints & PLUGIN_IS_BRIDGE)
        {
            osc_send_control(&osc.data, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
            osc_send_control(&osc.data, PARAMETER_DRYWET, x_drywet);
            osc_send_control(&osc.data, PARAMETER_VOLUME, x_vol);
            osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, x_bal_left);
            osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, x_bal_right);
        }
    }

    bool update_osc_gui()
    {
        // wait for UI 'update' call; 40 re-tries, 4 secs
        for (short i=1; i<40; i++)
        {
            if (osc.data.target)
            {
                osc_send_show(&osc.data);
                return true;
            }
            else
                carla_msleep(100);
        }
        return false;
    }
#endif

    void remove_from_jack()
    {
        qDebug("CarlaPlugin::remove_from_jack() - start");

        if (jack_client == nullptr)
            return;

        if (carla_options.global_jack_client == false)
            jack_deactivate(jack_client);

        for (uint32_t i=0; i < ain.count; i++)
            jack_port_unregister(jack_client, ain.ports[i]);

        for (uint32_t i=0; i < aout.count; i++)
            jack_port_unregister(jack_client, aout.ports[i]);

        if (midi.port_min)
            jack_port_unregister(jack_client, midi.port_min);

        if (midi.port_mout)
            jack_port_unregister(jack_client, midi.port_mout);

        if (param.port_cin)
            jack_port_unregister(jack_client, param.port_cin);

        if (param.port_cout)
            jack_port_unregister(jack_client, param.port_cout);

        qDebug("CarlaPlugin::remove_from_jack() - end");
    }

    virtual void delete_buffers()
    {
        qDebug("CarlaPlugin::delete_buffers() - start");

        if (ain.count > 0)
            delete[] ain.ports;

        if (aout.count > 0)
            delete[] aout.ports;

        if (param.count > 0)
        {
            delete[] param.data;
            delete[] param.ranges;
        }

        ain.count = 0;
        ain.ports = nullptr;

        aout.count = 0;
        aout.ports = nullptr;

        midi.port_min  = nullptr;
        midi.port_mout = nullptr;

        param.count    = 0;
        param.data     = nullptr;
        param.ranges   = nullptr;
        param.port_cin  = nullptr;
        param.port_cout = nullptr;

        qDebug("CarlaPlugin::delete_buffers() - end");
    }

    bool lib_open(const char* filename)
    {
#ifdef Q_OS_WIN
        m_lib = LoadLibraryA(filename);
#else
        m_lib = dlopen(filename, RTLD_NOW);
#endif
        return bool(m_lib);
    }

    bool lib_close()
    {
        if (m_lib)
#ifdef Q_OS_WIN
            return FreeLibrary((HMODULE)m_lib) != 0;
#else
            return dlclose(m_lib) != 0;
#endif
        else
            return false;
    }

    void* lib_symbol(const char* symbol)
    {
        if (m_lib)
#ifdef Q_OS_WIN
            return (void*)GetProcAddress((HMODULE)m_lib, symbol);
#else
            return dlsym(m_lib, symbol);
#endif
        else
            return nullptr;
    }

    const char* lib_error()
    {
#ifdef Q_OS_WIN
        static char libError[2048];
        memset(libError, 0, sizeof(char)*2048);

        LPVOID winErrorString;
        DWORD  winErrorCode = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

        snprintf(libError, 2048, "%s: error code %i: %s", m_filename, winErrorCode, (const char*)winErrorString);
        LocalFree(winErrorString);

        return libError;
#else
        return dlerror();
#endif
    }

    // utilities
    static void fix_parameter_value(double& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
    }

    static double abs_d(const double& value)
    {
        return (value < 0.0) ? -value : value;
    }

protected:
    PluginType m_type;
    short m_id;
    unsigned int m_hints;

    bool m_active;
    bool m_active_before;

    void* m_lib;
    const char* m_name;
    const char* m_filename;

    double x_drywet, x_vol, x_bal_left, x_bal_right;
    jack_client_t* jack_client;

    // Storage Data
    PluginAudioData ain;
    PluginAudioData aout;
    PluginMidiData midi;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    QList<CustomData> custom;

    // Extra
#ifndef BUILD_BRIDGE
    struct {
        OscData data;
        CarlaPluginThread* thread;
    } osc;
#endif

    struct {
        QMutex lock;
        PluginPostEvent data[MAX_POST_EVENTS];
    } post_events;

    ExternalMidiNote ext_midi_notes[MAX_MIDI_EVENTS];
};

#endif // CARLA_PLUGIN_H

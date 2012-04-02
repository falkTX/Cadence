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

#include <cmath>
#include <cstring>

#include <QtCore/QList>
#include <QtCore/QMutex>

#define CARLA_PROCESS_CONTINUE_CHECK if (m_id != plugin_id) { return callback_action(CALLBACK_DEBUG, plugin_id, m_id, 0, 0.0); }

class CarlaPluginThread;

const unsigned short MAX_POSTEVENTS = 128;

// Global OSC stuff
extern OscData global_osc_data;

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
};

struct PluginAudioData {
    uint32_t count;
    uint32_t* rindexes;
    jack_port_t** ports;
};

struct PluginMidiData {
    uint32_t count;
    jack_port_t** ports;
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
    const char** names;
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
        ain.rindexes = nullptr;

        aout.count = 0;
        aout.ports = nullptr;
        aout.rindexes = nullptr;

        min.count = 0;
        min.ports = nullptr;

        mout.count = 0;
        mout.ports = nullptr;

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
        midiprog.names   = nullptr;

        custom.clear();

        osc.data.path = nullptr;
        osc.data.source = nullptr;
        osc.data.target = nullptr;
        osc.thread = nullptr;

        for (unsigned short i=0; i < MAX_POSTEVENTS; i++)
            post_events.data[i].valid = false;

        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
            external_midi[i].valid = false;
    }

    virtual ~CarlaPlugin()
    {
        qDebug("CarlaPlugin::~CarlaPlugin()");

        m_type = PLUGIN_NONE;
        m_id   = -1;
        m_hints = 0;

        m_active = false;
        m_active_before = false;

        // Unregister jack ports
        remove_from_jack();

        // Delete data
        delete_buffers();

        osc.data.path = nullptr;
        osc.data.source = nullptr;
        osc.data.target = nullptr;
        osc.thread = nullptr;

        if (prog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
                free((void*)prog.names[i]);

            delete[] prog.names;
        }

        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
                free((void*)midiprog.names[i]);

            delete[] midiprog.data;
            delete[] midiprog.names;
        }

        prog.count   = 0;
        prog.current = -1;
        prog.names   = nullptr;

        midiprog.count   = 0;
        midiprog.current = -1;
        midiprog.data    = nullptr;
        midiprog.names   = nullptr;

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

    virtual PluginCategory category()
    {
        return PLUGIN_CATEGORY_NONE;
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

    uint32_t param_count()
    {
        return param.count;
    }

    uint32_t custom_count()
    {
        return custom.count();
    }

    virtual uint32_t prog_count()
    {
        return prog.count;
    }

    virtual uint32_t midiprog_count()
    {
        return midiprog.count;
    }

    virtual uint32_t param_scalepoint_count(uint32_t)
    {
        return 0;
    }

    virtual double param_scalepoint_value(uint32_t, uint32_t)
    {
        return 0.0;
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

    OscData* osc_data()
    {
        return &osc.data;
    }

    int32_t prog_current()
    {
        return prog.current;
    }

    int32_t midiprog_current()
    {
        return midiprog.current;
    }

    const char* prog_name(uint32_t index)
    {
        return prog.names[index];
    }

    const char* midiprog_name(uint32_t index)
    {
        return midiprog.names[index];
    }

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

    virtual void get_parameter_name(uint32_t index, char* buf_str)
    {
        *buf_str = 0;
        Q_UNUSED(index);
    }

    virtual void get_parameter_symbol(uint32_t index, char* buf_str)
    {
        *buf_str = 0;
        Q_UNUSED(index);
    }

    virtual void get_parameter_label(uint32_t index, char* buf_str)
    {
        *buf_str = 0;
        Q_UNUSED(index);
    }

    virtual void get_parameter_scalepoint_label(uint32_t pindex, uint32_t index, char* buf_str)
    {
        *buf_str = 0;
        Q_UNUSED(index);
        Q_UNUSED(pindex);
    }

    virtual void get_audio_port_count_info(PortCountInfo* info)
    {
        info->ins   = ain.count;
        info->outs  = aout.count;
        info->total = ain.count + aout.count;
    }

    void get_midi_port_count_info(PortCountInfo* info)
    {
        info->ins   = min.count;
        info->outs  = mout.count;
        info->total = min.count + mout.count;
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
        info->label   = midiprog.names[index];
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        info->type = GUI_NONE;
    }

    virtual int32_t get_chunk_data(void**)
    {
        return 0;
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

    double get_default_parameter_value(uint32_t index)
    {
        return param.ranges[index].def;
    }

    virtual double get_current_parameter_value(uint32_t)
    {
        return 0.0;
    }

    virtual void set_parameter_value(uint32_t index, double value, bool gui_send, bool osc_send, bool callback_send)
    {
#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, m_id, index, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, index, value);
        }
#else
        Q_UNUSED(osc_send);
#endif

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, index, 0, value);

        Q_UNUSED(gui_send);
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
        bool save_data = true;
        bool already_have = false;

        qDebug("set_custom_data(%i, %s, %s)", dtype, key, value);

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

    virtual void set_program(uint32_t index, bool, bool osc_send, bool callback_send, bool)
    {
        prog.current = index;

        // Change default value
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = get_current_parameter_value(i);

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

    virtual void set_midi_program(uint32_t index, bool, bool osc_send, bool callback_send, bool)
    {
        midiprog.current = index;

        // Change default value
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = get_current_parameter_value(i);

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

    virtual void send_midi_note(bool onoff, uint8_t note, uint8_t velo, bool, bool osc_send, bool callback_send)
    {
        carla_midi_lock();
        for (unsigned int i=0; i<MAX_MIDI_EVENTS; i++)
        {
            if (external_midi[i].valid == false)
            {
                external_midi[i].valid = true;
                external_midi[i].onoff = onoff;
                external_midi[i].note = note;
                external_midi[i].velo = velo;
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

    virtual void set_gui_data(int, void*)
    {
    }

    virtual void show_gui(bool)
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

    void postpone_event(PluginPostEventType type, int32_t index, double value)
    {
        post_events.lock.lock();

        for (unsigned short i=0; i<MAX_POSTEVENTS; i++)
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

        memcpy(post_events_dst, post_events.data, sizeof(PluginPostEvent)*MAX_POSTEVENTS);

        for (unsigned short i=0; i < MAX_POSTEVENTS; i++)
            post_events.data[i].valid = false;

        post_events.lock.unlock();
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
            osc_send_control(&osc.data, param.data[i].rindex, get_current_parameter_value(i));

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
            //else
                // 100 ms
                // FIXME
                //usleep(100000);
        }
        return false;
    }
#endif

    void remove_from_jack()
    {
        qDebug("CarlaPlugin::remove_from_jack()");

        if (jack_client == nullptr)
            return;

        uint32_t i;

        for (i=0; i < ain.count; i++)
            jack_port_unregister(jack_client, ain.ports[i]);

        for (i=0; i < aout.count; i++)
            jack_port_unregister(jack_client, aout.ports[i]);

        for (i=0; i < min.count; i++)
            jack_port_unregister(jack_client, min.ports[i]);

        for (i=0; i < mout.count; i++)
            jack_port_unregister(jack_client, mout.ports[i]);

        if (param.port_cin)
            jack_port_unregister(jack_client, param.port_cin);

        if (param.port_cout)
            jack_port_unregister(jack_client, param.port_cout);

        qDebug("CarlaPlugin::remove_from_jack() - end");
    }

    void delete_buffers()
    {
        qDebug("CarlaPlugin::delete_buffers()");

        if (ain.count > 0)
        {
            delete[] ain.ports;
            delete[] ain.rindexes;
        }

        if (aout.count > 0)
        {
            delete[] aout.ports;
            delete[] aout.rindexes;
        }

        if (min.count > 0)
        {
            delete[] min.ports;
        }

        if (mout.count > 0)
        {
            delete[] mout.ports;
        }

        if (param.count > 0)
        {
            delete[] param.data;
            delete[] param.ranges;
        }

        ain.count = 0;
        ain.ports = nullptr;
        ain.rindexes = nullptr;

        aout.count = 0;
        aout.ports = nullptr;
        aout.rindexes = nullptr;

        min.count = 0;
        min.ports = nullptr;

        mout.count = 0;
        mout.ports = nullptr;

        param.count    = 0;
        param.data     = nullptr;
        param.ranges   = nullptr;
        param.port_cin  = nullptr;
        param.port_cout = nullptr;

        qDebug("CarlaPlugin::delete_buffers() - end");
    }

    virtual int set_osc_bridge_info(PluginBridgeInfoType, lo_arg**)
    {
        return 1;
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

        snprintf(libError, 2048, "%s: error code " P_INTPTR ": %s", m_filename, winErrorCode, (const char*)winErrorString);
        LocalFree(winErrorString);

        return libError;
#else
        return dlerror();
#endif
    }

    // utilities
    void fix_parameter_value(double& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
    }

    double abs_d(const double& value)
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
    PluginMidiData min;
    PluginMidiData mout;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;

    QList<CustomData> custom;

    // Extra
    struct {
        OscData data;
        CarlaPluginThread* thread;
    } osc;

    struct {
        QMutex lock;
        PluginPostEvent data[MAX_POSTEVENTS];
    } post_events;

    ExternalMidiNote external_midi[MAX_MIDI_EVENTS];
};

#endif // CARLA_PLUGIN_H

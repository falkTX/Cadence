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

#include "carla_backend.h"

#include <cmath>
#include <cstring>

#include <jack/jack.h>
#include <jack/midiport.h>

#define CARLA_PROCESS_CONTINUE_CHECK if (m_id != plugin_id) { return callback_action(CALLBACK_DEBUG, plugin_id, m_id, 0, 0.0f); }

// Global JACK client
extern jack_client_t* carla_jack_client;

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

class CarlaPlugin
{
public:
    CarlaPlugin()
    {
        qDebug("CarlaPlugin::CarlaPlugin()");

        m_type  = PLUGIN_NONE;
        m_id    = -1;
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

        param.count    = 0;
        param.data     = nullptr;
        param.ranges   = nullptr;
        param.port_cin  = nullptr;
        param.port_cout = nullptr;
    }

    virtual ~CarlaPlugin()
    {
        qDebug("CarlaPlugin::~CarlaPlugin()");

        // Unregister jack ports
        remove_from_jack();

        // Delete data
        delete_buffers();

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

    uint32_t param_count()
    {
        return param.count;
    }

    ParameterData* param_data(uint32_t index)
    {
        return &param.data[index];
    }

    ParameterRanges* param_ranges(uint32_t index)
    {
        return &param.ranges[index];
    }

    virtual uint32_t param_scalepoint_count(uint32_t index)
    {
        Q_UNUSED(index);
        return 0;
    }

    virtual double param_scalepoint_value(uint32_t pindex, uint32_t index)
    {
        Q_UNUSED(index);
        Q_UNUSED(pindex);
        return 0.0;
    }

    virtual uint32_t prog_count()
    {
        return 0;
    }

    virtual uint32_t midiprog_count()
    {
        return 0;
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

    void get_audio_port_count_info(PortCountInfo* info)
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

    virtual void get_midi_program_info(MidiProgramInfo* info)
    {
        info->bank = 0;
        info->program = 0;
        info->label = nullptr;
    }

    virtual int32_t get_chunk_data(void** data_ptr)
    {
        Q_UNUSED(data_ptr);
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

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, PARAMETER_ACTIVE, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, PARAMETER_ACTIVE, value);
        }

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

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, PARAMETER_DRYWET, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, PARAMETER_DRYWET, value);
        }

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

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, PARAMETER_VOLUME, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, PARAMETER_VOLUME, value);
        }

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

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, PARAMETER_BALANCE_LEFT, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, value);
        }

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

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, PARAMETER_BALANCE_RIGHT, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, value);
        }

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_BALANCE_RIGHT, 0, value);
    }

    double get_default_parameter_value(uint32_t index)
    {
        return param.ranges[index].def;
    }

    virtual double get_current_parameter_value(uint32_t index)
    {
        //if (plugin->param.data[parameter_id].hints & PARAMETER_HAS_STRICT_BOUNDS)
        //    plugin->fix_parameter_value(value, plugin->param.ranges[parameter_id]);
        Q_UNUSED(index);
        return 0.0;
    }

    virtual void set_parameter_value(uint32_t index, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        //fix_parameter_value(value, param.ranges[parameter_id]);

        if (osc_send)
        {
            //osc_send_set_parameter_value(&global_osc_data, id, parameter_id, value);

            //if (hints & PLUGIN_IS_BRIDGE)
            //    osc_send_control(&osc.data, parameter_id, value);
        }

        if (callback_send)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, index, 0, value);

        Q_UNUSED(gui_send);
        //x_set_parameter_value(parameter_id, value, gui_send);
    }

    virtual void set_chunk_data(const char* string_data)
    {
        Q_UNUSED(string_data);
    }

    virtual void set_gui_data(int data, void* ptr)
    {
        Q_UNUSED(data);
        Q_UNUSED(ptr);
    }

    virtual void show_gui(bool yesno)
    {
        Q_UNUSED(yesno);
    }

    virtual void idle_gui()
    {
    }

    virtual void reload()
    {
    }

    virtual void reload_programs(bool init)
    {
        Q_UNUSED(init);
    }

    virtual void prepare_for_save()
    {
    }

    virtual void process(jack_nframes_t nframes) = 0;

    virtual void buffer_size_changed(jack_nframes_t new_buffer_size)
    {
        Q_UNUSED(new_buffer_size);
    }

//    virtual int set_osc_bridge_info(PluginOscBridgeInfoType, lo_arg**) = 0;

//    virtual void x_set_parameter_value(uint32_t parameter_id, double value, bool gui_send) = 0;
//    virtual void x_set_program(uint32_t program_id, bool gui_send, bool block) = 0;
//    virtual void x_set_midi_program(uint32_t midi_program_id, bool gui_send, bool block) = 0;
//    virtual void x_set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send) = 0;

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

        snprintf(libError, 2048, "%s: error code %li: %s", m_filename, winErrorCode, (const char*)winErrorString);
        LocalFree(winErrorString);

        return libError;
#else
        return dlerror();
#endif
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
};

#endif // CARLA_PLUGIN_H

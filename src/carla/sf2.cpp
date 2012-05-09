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

#ifdef BUILD_BRIDGE
#error You should not use bridge for soundfonts
#endif

#include "carla_plugin.h"

#include <fluidsynth.h>

#if (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 3)
#define FLUIDSYNTH_VERSION_113
#endif

class Sf2Plugin : public CarlaPlugin
{
public:
    Sf2Plugin() : CarlaPlugin()
    {
        qDebug("Sf2Plugin::Sf2Plugin()");
        m_type = PLUGIN_SF2;

        f_settings = new_fluid_settings();
        fluid_settings_setnum(f_settings, "synth.sample-rate", get_sample_rate());

        f_synth = new_fluid_synth(f_settings);
        fluid_synth_set_sample_rate(f_synth, get_sample_rate());
        fluid_synth_set_reverb_on(f_synth, 0);
        fluid_synth_set_chorus_on(f_synth, 0);

        m_label = nullptr;
        param_buffers = nullptr;
    }

    ~Sf2Plugin()
    {
        qDebug("Sf2Plugin::~Sf2Plugin()");

        if (m_label)
            free((void*)m_label);

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

    PluginCategory category()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    uint32_t param_scalepoint_count(uint32_t param_id)
    {
        switch (param_id)
        {
        case Sf2ChorusType:
            return 2;
        case Sf2Interpolation:
            return 4;
        default:
            return 0;
        }
    }

    double get_parameter_value(uint32_t param_id)
    {
        return param_buffers[param_id];
    }

    double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        switch (param_id)
        {
        case Sf2ChorusType:
            switch (scalepoint_id)
            {
            case 0:
                return FLUID_CHORUS_MOD_SINE;
            case 1:
                return FLUID_CHORUS_MOD_TRIANGLE;
            default:
                return FLUID_CHORUS_DEFAULT_TYPE;
            }
        case Sf2Interpolation:
            switch (scalepoint_id)
            {
            case 0:
                return FLUID_INTERP_NONE;
            case 1:
                return FLUID_INTERP_LINEAR;
            case 2:
                return FLUID_INTERP_4THORDER;
            case 3:
                return FLUID_INTERP_7THORDER;
            default:
                return FLUID_INTERP_DEFAULT;
            }
        default:
            return 0.0;
        }
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, m_label, STR_MAX);
    }

//    void get_real_name(char *buf_str)
//    {
//        fluid_sfont_t* f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);
//        strncpy(buf_str, f_sfont->get_name(f_sfont), STR_MAX);
//        //f_sfont->free(f_sfont);
//    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        switch (param_id)
        {
        case Sf2ReverbOnOff:
            strncpy(buf_str, "Reverb On/Off", STR_MAX);
            break;
        case Sf2ReverbRoomSize:
            strncpy(buf_str, "Reverb Room Size", STR_MAX);
            break;
        case Sf2ReverbDamp:
            strncpy(buf_str, "Reverb Damp", STR_MAX);
            break;
        case Sf2ReverbLevel:
            strncpy(buf_str, "Reverb Level", STR_MAX);
            break;
        case Sf2ReverbWidth:
            strncpy(buf_str, "Reverb Width", STR_MAX);
            break;
        case Sf2ChorusOnOff:
            strncpy(buf_str, "Chorus On/Off", STR_MAX);
            break;
        case Sf2ChorusNr:
            strncpy(buf_str, "Chorus Voice Count", STR_MAX);
            break;
        case Sf2ChorusLevel:
            strncpy(buf_str, "Chorus Level", STR_MAX);
            break;
        case Sf2ChorusSpeedHz:
            strncpy(buf_str, "Chorus Speed", STR_MAX);
            break;
        case Sf2ChorusDepthMs:
            strncpy(buf_str, "Chorus Depth", STR_MAX);
            break;
        case Sf2ChorusType:
            strncpy(buf_str, "Chorus Type", STR_MAX);
            break;
        case Sf2Polyphony:
            strncpy(buf_str, "Polyphony", STR_MAX);
            break;
        case Sf2Interpolation:
            strncpy(buf_str, "Interpolation", STR_MAX);
            break;
        case Sf2VoiceCount:
            strncpy(buf_str, "Voice Count", STR_MAX);
            break;
        default:
            *buf_str = 0;
            break;
        }
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        switch (param_id)
        {
        case Sf2ChorusSpeedHz:
            strncpy(buf_str, "Hz", STR_MAX);
            break;
        case Sf2ChorusDepthMs:
            strncpy(buf_str, "ms", STR_MAX);
            break;
        default:
            *buf_str = 0;
            break;
        }
    }

    void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        switch (param_id)
        {
        case Sf2ChorusType:
            switch (scalepoint_id)
            {
            case 0:
                strncpy(buf_str, "Sine wave", STR_MAX);
                return;
            case 1:
                strncpy(buf_str, "Triangle wave", STR_MAX);
                return;
            }
        case Sf2Interpolation:
            switch (scalepoint_id)
            {
            case 0:
                strncpy(buf_str, "None", STR_MAX);
                return;
            case 1:
                strncpy(buf_str, "Straight-line", STR_MAX);
                return;
            case 2:
                strncpy(buf_str, "Fourth-order", STR_MAX);
                return;
            case 3:
                strncpy(buf_str, "Seventh-order", STR_MAX);
                return;
            }
        }
        *buf_str = 0;
    }

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        param_buffers[param_id] = value;

        switch(param_id)
        {
        case Sf2ReverbOnOff:
            value = value > 0.5 ? 1 : 0;
            fluid_synth_set_reverb_on(f_synth, value);
            break;
        case Sf2ReverbRoomSize:
        case Sf2ReverbDamp:
        case Sf2ReverbLevel:
        case Sf2ReverbWidth:
            fluid_synth_set_reverb(f_synth, param_buffers[Sf2ReverbRoomSize], param_buffers[Sf2ReverbDamp], param_buffers[Sf2ReverbWidth], param_buffers[Sf2ReverbLevel]);
            break;
        case Sf2ChorusOnOff:
            value = value > 0.5 ? 1 : 0;
            fluid_synth_set_chorus_on(f_synth, value);
            break;
        case Sf2ChorusNr:
        case Sf2ChorusLevel:
        case Sf2ChorusSpeedHz:
        case Sf2ChorusDepthMs:
        case Sf2ChorusType:
            fluid_synth_set_chorus(f_synth, (int)param_buffers[Sf2ChorusNr], param_buffers[Sf2ChorusLevel], param_buffers[Sf2ChorusSpeedHz], param_buffers[Sf2ChorusDepthMs], (int)param_buffers[Sf2ChorusType]);
            break;
        case Sf2Polyphony:
            fluid_synth_set_polyphony(f_synth, (int)value);
            break;
        case Sf2Interpolation:
            for (int i=0; i < 16; i++)
                fluid_synth_set_interp_method(f_synth, i, (int)value);
            break;
        default:
            break;
        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_midi_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        if (index >= 0)
        {
            if (carla_jack_on_freewheel())
            {
                if (block) carla_proc_lock();
                fluid_synth_program_select(f_synth, 0, f_id, midiprog.data[index].bank, midiprog.data[index].program);
                if (block) carla_proc_unlock();
            }
            else
            {
                short _id = m_id;

                if (block)
                {
                    carla_proc_lock();
                    m_id = -1;
                    carla_proc_unlock();
                }

                fluid_synth_program_select(f_synth, 0, f_id, midiprog.data[index].bank, midiprog.data[index].program);

                if (block)
                {
                    carla_proc_lock();
                    m_id = _id;
                    carla_proc_unlock();
                }
            }
        }

        CarlaPlugin::set_midi_program(index, gui_send, osc_send, callback_send, block);
    }

    void reload()
    {
        qDebug("Sf2AudioPlugin::reload() - start");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        remove_from_jack(bool(_id >= 0));

        // Delete old data
        delete_buffers();

        uint32_t aouts, params, j;
        aouts  = 2;
        params = Sf2ParametersMax;

        aout.ports    = new jack_port_t*[aouts];
        aout.rindexes = new uint32_t[aouts];

        param.data    = new ParameterData[params];
        param.ranges  = new ParameterRanges[params];
        param_buffers = new double[params];

        const int port_name_size = jack_port_name_size() - 1;
        char port_name[port_name_size];

        // ---------------------------------------
        // Audio Outputs

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":out-left");
        }
        else
#endif
            strcpy(port_name, "out-left");

        aout.ports[0]    = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        aout.rindexes[0] = 0;

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":out-right");
        }
        else
#endif
            strcpy(port_name, "out-right");

        aout.ports[1]    = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        aout.rindexes[1] = 1;

        // ---------------------------------------
        // MIDI Input

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":midi-in");
        }
        else
#endif
            strcpy(port_name, "midi-in");

        midi.port_min = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

        // ---------------------------------------
        // Parameters

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":control-in");
        }
        else
#endif
            strcpy(port_name, "control-in");

        param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":control-out");
        }
        else
#endif
            strcpy(port_name, "control-out");

        param.port_cout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

        // ----------------------
        j = Sf2ReverbOnOff;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = 0.0;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 1.0;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbRoomSize;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.2;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_ROOMSIZE;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbDamp;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_DAMP;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbLevel;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = MIDI_CONTROL_REVERB_SEND_LEVEL;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_LEVEL;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbWidth;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 30.0; // should be 100, but that sounds too much
        param.ranges[j].def = FLUID_REVERB_DEFAULT_WIDTH;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusOnOff;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_BOOLEAN;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = 0.0;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 1.0;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusNr;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 99.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_N;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 10.0;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusLevel;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = MIDI_CONTROL_CHORUS_SEND_LEVEL;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 10.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_LEVEL;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusSpeedHz;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.29;
        param.ranges[j].max = 5.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_SPEED;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusDepthMs;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 2048000.0 / get_sample_rate();
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_DEPTH;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusType;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = FLUID_CHORUS_MOD_SINE;
        param.ranges[j].max = FLUID_CHORUS_MOD_TRIANGLE;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_TYPE;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2Polyphony;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 1;
        param.ranges[j].max = 512; // max theoric is 65535
        param.ranges[j].def = fluid_synth_get_polyphony(f_synth);
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 10;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2Interpolation;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER | PARAMETER_USES_SCALEPOINTS;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = FLUID_INTERP_NONE;
        param.ranges[j].max = FLUID_INTERP_HIGHEST;
        param.ranges[j].def = FLUID_INTERP_DEFAULT;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2VoiceCount;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_OUTPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_IS_INTEGER;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0;
        param.ranges[j].max = 65535;
        param.ranges[j].def = 0;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param_buffers[j] = param.ranges[j].def;

        // ---------------------------------------

        aout.count  = aouts;
        param.count = params;

        reload_programs(true);

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        m_hints |= PLUGIN_IS_SYNTH;
        m_hints |= PLUGIN_CAN_VOLUME;
        m_hints |= PLUGIN_CAN_BALANCE;

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client == false)
#endif
            jack_activate(jack_client);

        qDebug("Sf2AudioPlugin::reload() - end");
    }

    virtual void reload_programs(bool init)
    {
        qDebug("Sf2AudioPlugin::reload_programs(%s)", bool2str(init));

        // Delete old programs
        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < midiprog.count; i++)
                free((void*)midiprog.data[i].name);

            delete[] midiprog.data;
        }

        midiprog.count = 0;
        midiprog.data  = nullptr;

        // Query new programs
        fluid_sfont_t* f_sfont;
        fluid_preset_t f_preset;

        f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);

        // initial check to know how much midi-programs we get
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            midiprog.count += 1;

        if (midiprog.count > 0)
            midiprog.data  = new midi_program_t [midiprog.count];

        // Update data
        uint32_t i = 0;
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
        {
            if (i < midiprog.count)
            {
                midiprog.data[i].bank    = f_preset.get_banknum(&f_preset);
                midiprog.data[i].program = f_preset.get_num(&f_preset);
                midiprog.data[i].name    = strdup(f_preset.get_name(&f_preset));
            }
            i++;
        }

        // TODO - for xx(), rest of i < count to null

        //f_sfont->free(f_sfont);

#ifndef BUILD_BRIDGE
        // Update OSC Names
        osc_global_send_set_midi_program_count(m_id, midiprog.count);

        for (i=0; i < midiprog.count; i++)
            osc_global_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (midiprog.count > 0)
            {
                fluid_synth_program_reset(f_synth);

                for (i=0; i < 16 && i != 9; i++)
                {
                    fluid_synth_set_channel_type(f_synth, i, CHANNEL_TYPE_MELODIC);
                    fluid_synth_program_select(f_synth, i, f_id, midiprog.data[0].bank, midiprog.data[0].program);
                }

                fluid_synth_set_channel_type(f_synth, 9, CHANNEL_TYPE_DRUM);
                fluid_synth_program_select(f_synth, 9, f_id, 128, 0);

                set_midi_program(0, false, false, false, true);
            }
        }
    }

    virtual void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        unsigned int midi_event_count = 0;

        double aouts_peak_tmp[2] = { 0.0 };

        jack_audio_sample_t* aouts_buffer[aout.count];
        void* min_buffer;

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        min_buffer = jack_port_get_buffer(midi.port_min, nframes);

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        {
            void* pin_buffer = jack_port_get_buffer(param.port_cin, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            unsigned char next_bank_ids[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0 };

            if (midiprog.current > 0 && midiprog.count > 0)
                next_bank_ids[0] = midiprog.data[midiprog.current].bank;

            for (i=0; i < n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                jack_midi_data_t status  = pin_event.buffer[0];
                jack_midi_data_t channel = status & 0x0F;

                // Control change
                if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    jack_midi_data_t control = pin_event.buffer[1];
                    jack_midi_data_t c_value = pin_event.buffer[2];

                    // Bank Select
                    if (MIDI_IS_CONTROL_BANK_SELECT(control))
                    {
                        next_bank_ids[0] = c_value;
                        continue;
                    }

                    double value;

                    // Control backend stuff
                    if (channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(control) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = double(c_value)/127;
                            set_drywet(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(control) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = double(c_value)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_BALANCE(control) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = (double(c_value)-63.5)/63.5;

                            if (value < 0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0)
                            {
                                left  = (value*2)-1.0;
                                right = 1.0;
                            }
                            else
                            {
                                left  = -1.0;
                                right = 1.0;
                            }

                            set_balance_left(left, false, false);
                            set_balance_right(right, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
#ifdef FLUIDSYNTH_VERSION_113
                            fluid_synth_all_notes_off(f_synth, 0);
                            fluid_synth_all_sounds_off(f_synth, 0);
#endif
                            send_midi_all_notes_off();
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
#ifdef FLUIDSYNTH_VERSION_113
                            fluid_synth_all_notes_off(f_synth, 0);
#endif
                            send_midi_all_notes_off();
                            continue;
                        }
                    }
#ifdef FLUIDSYNTH_VERSION_113
                    else // not channel for backend
                    {
                        if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            fluid_synth_all_notes_off(f_synth, channel);
                            fluid_synth_all_sounds_off(f_synth, channel);
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            fluid_synth_all_notes_off(f_synth, channel);
                            continue;
                        }
                    }
#endif

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midi_channel == channel && param.data[k].midi_cc == control && param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = c_value <= 63 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    if (channel > 15)
                        continue;

                    uint32_t mbank_id = next_bank_ids[channel];
                    uint32_t mprog_id = pin_event.buffer[1];

                    for (k=0; k < midiprog.count; k++)
                    {
                        if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                        {
                            if (channel == cin_channel)
                            {
                                set_midi_program(k, false, false, false, false);
                                postpone_event(PostEventMidiProgramChange, k, 0.0);
                            }
                            else
                                fluid_synth_program_select(f_synth, channel, f_id, mbank_id, mprog_id);

                            break;
                        }
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (ext_midi_notes[i].valid)
                {
                    if (ext_midi_notes[i].onoff)
                        fluid_synth_noteon(f_synth, 0, ext_midi_notes[i].note, ext_midi_notes[i].velo);
                    else
                        fluid_synth_noteoff(f_synth, 0, ext_midi_notes[i].note);

                    ext_midi_notes[i].valid = false;
                    midi_event_count += 1;
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (JACK)

        {
            jack_midi_event_t min_event;
            uint32_t n_min_events = jack_midi_get_event_count(min_buffer);

            for (k=0; k < n_min_events && midi_event_count < MAX_MIDI_EVENTS; k++)
            {
                if (jack_midi_event_get(&min_event, min_buffer, k) != 0)
                    break;

                jack_midi_data_t status  = min_event.buffer[0];
                jack_midi_data_t channel = status & 0x0F;

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && min_event.buffer[2] == 0)
                    status -= 0x10;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    jack_midi_data_t note = min_event.buffer[1];

                    fluid_synth_noteoff(f_synth, channel, note);
                    postpone_event(PostEventNoteOff, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    jack_midi_data_t note = min_event.buffer[1];
                    jack_midi_data_t velo = min_event.buffer[2];

                    fluid_synth_noteon(f_synth, channel, note, velo);
                    postpone_event(PostEventNoteOn, note, velo);
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    jack_midi_data_t pressure = min_event.buffer[1];

                    fluid_synth_channel_pressure(f_synth, channel, pressure);
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    jack_midi_data_t lsb = min_event.buffer[1];
                    jack_midi_data_t msb = min_event.buffer[2];

                    fluid_synth_pitch_bend(f_synth, channel, (msb << 7) | lsb);
                }

                midi_event_count += 1;
            }
        } // End of MIDI Input (JACK)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
            fluid_synth_process(f_synth, nframes, 0, nullptr, 2, aouts_buffer);

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (volume and balance)

        if (m_active)
        {
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_bal_left != -1.0 || x_bal_right != 1.0);

            double bal_rangeL, bal_rangeR;
            jack_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Volume, using fluidsynth internals
                fluid_synth_set_gain(f_synth, x_vol);

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_audio_sample_t)*nframes);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k<nframes; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            aouts_buffer[i][k]  = old_bal_left[k]*(1.0-bal_rangeL);
                            aouts_buffer[i][k] += aouts_buffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            aouts_buffer[i][k]  = aouts_buffer[i][k]*bal_rangeR;
                            aouts_buffer[i][k] += old_bal_left[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; k < nframes && i < 2; k++)
                {
                    if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        {
            void* cout_buffer = jack_port_get_buffer(param.port_cout, nframes);
            jack_midi_clear_buffer(cout_buffer);

            k = Sf2VoiceCount;
            param_buffers[k] = fluid_synth_get_active_voice_count(f_synth);

            if (param.data[k].midi_cc > 0)
            {
                double value_per = (param_buffers[k] - param.ranges[k].min)/(param.ranges[k].max - param.ranges[k].min);

                jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                event_buffer[0] = MIDI_STATUS_CONTROL_CHANGE + param.data[k].midi_channel;
                event_buffer[1] = param.data[k].midi_cc;
                event_buffer[2] = 127*value_per;
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    virtual void delete_buffers()
    {
        qDebug("Sf2Plugin::delete_buffers() - start");

        if (param.count > 0)
            delete[] param_buffers;

        param_buffers = nullptr;

        qDebug("Sf2Plugin::delete_buffers() - end");
    }

    bool init(const char* filename, const char* label)
    {
        f_id = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id >= 0)
        {
            m_filename = strdup(filename);
            m_label = strdup(label);
            m_name  = get_unique_name(label);

            if (carla_jack_register_plugin(this, &jack_client))
                return true;
            else
                set_last_error("Failed to register plugin in JACK");
        }
        else
            set_last_error("Failed to load SoundFont file");

        return false;
    }

private:
    enum Sf2InputParameters {
        Sf2ReverbOnOff    = 0,
        Sf2ReverbRoomSize = 1,
        Sf2ReverbDamp     = 2,
        Sf2ReverbLevel    = 3,
        Sf2ReverbWidth    = 4,
        Sf2ChorusOnOff    = 5,
        Sf2ChorusNr       = 6,
        Sf2ChorusLevel    = 7,
        Sf2ChorusSpeedHz  = 8,
        Sf2ChorusDepthMs  = 9,
        Sf2ChorusType     = 10,
        Sf2Polyphony      = 11,
        Sf2Interpolation  = 12,
        Sf2VoiceCount     = 13,
        Sf2ParametersMax  = 14
    };

    fluid_settings_t* f_settings;
    fluid_synth_t* f_synth;
    int f_id;

    double* param_buffers;
    const char* m_label;
};

short add_plugin_sf2(const char* filename, const char* label)
{
    qDebug("add_plugin_sf2(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        if (fluid_is_soundfont(filename))
        {
            Sf2Plugin* plugin = new Sf2Plugin;

            if (plugin->init(filename, label))
            {
                plugin->reload();
                plugin->set_id(id);

                unique_names[id] = plugin->name();
                CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
                plugin->osc_global_register_new();
#endif
            }
            else
            {
                delete plugin;
                id = -1;
            }
        }
        else
            set_last_error("Requested file is not a valid SoundFont");
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

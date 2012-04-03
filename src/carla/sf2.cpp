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

#include "carla_plugin.h"

#include <fluidsynth.h>

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
        fluid_synth_set_reverb_on(f_synth, 0);
        fluid_synth_set_chorus_on(f_synth, 0);

        m_label = nullptr;
    }

    virtual ~Sf2Plugin()
    {
        qDebug("Sf2Plugin::~Sf2Plugin()");

        if (m_label)
            free((void*)m_label);

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

#if 0
    virtual PluginCategory category()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    virtual uint32_t param_scalepoint_count(uint32_t param_id)
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

    virtual double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
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

    virtual void get_label(char* buf_str)
    {
        strncpy(buf_str, m_label, STR_MAX);
    }

    virtual void get_parameter_name(uint32_t index, char* buf_str)
    {
        switch (index)
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

    virtual void get_parameter_label(uint32_t index, char* buf_str)
    {
        switch (index)
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

    virtual void get_parameter_scalepoint_label(uint32_t pindex, uint32_t index, char* buf_str)
    {
        switch (pindex)
        {
        case Sf2ChorusType:
            switch (index)
            {
            case 0:
                strncpy(buf_str, "Sine wave", STR_MAX);
                return;
            case 1:
                strncpy(buf_str, "Triangle wave", STR_MAX);
                return;
            }
        case Sf2Interpolation:
            switch (index)
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

    virtual void set_parameter_value(uint32_t index, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        switch(index)
        {
        case Sf2ReverbOnOff:
            fluid_synth_set_reverb_on(f_synth, value > 0.5 ? 1 : 0);
            break;
        case Sf2ReverbRoomSize:
        case Sf2ReverbDamp:
        case Sf2ReverbLevel:
        case Sf2ReverbWidth:
            fluid_synth_set_reverb(f_synth, param_buffers[Sf2ReverbRoomSize], param_buffers[Sf2ReverbDamp], param_buffers[Sf2ReverbWidth], param_buffers[Sf2ReverbLevel]);
            break;
        case Sf2ChorusOnOff:
            fluid_synth_set_chorus_on(f_synth, value > 0.5 ? 1 : 0);
            break;
        case Sf2ChorusNr:
        case Sf2ChorusLevel:
        case Sf2ChorusSpeedHz:
        case Sf2ChorusDepthMs:
        case Sf2ChorusType:
            fluid_synth_set_chorus(f_synth, param_buffers[Sf2ChorusNr], param_buffers[Sf2ChorusLevel], param_buffers[Sf2ChorusSpeedHz], param_buffers[Sf2ChorusDepthMs], param_buffers[Sf2ChorusType]);
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

        CarlaPlugin::set_parameter_value(index, value, gui_send, osc_send, callback_send);
    }

    virtual void set_midi_program(uint32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        fluid_synth_program_select(f_synth, 0, f_id, midiprog.data[index].bank, midiprog.data[index].program);

        CarlaPlugin::set_midi_program(index, gui_send, osc_send, callback_send, block);
    }

    virtual void reload()
    {
#if 0
        qDebug("Sf2AudioPlugin::reload()");
        short _id = id;

        // Safely disable plugin for reload
        carla_proc_lock();

        id = -1;
        if (carla_options.global_jack_client == false)
            jack_deactivate(jack_client);

        carla_proc_unlock();

        // Unregister jack ports
        remove_from_jack();

        // Delete old data
        delete_buffers();

        uint32_t aouts, mins, params, j;
        aouts  = 2;
        mins   = 1;
        params = Sf2ParametersMax;

        aout.rindexes = new uint32_t[aouts];
        aout.ports    = new jack_port_t*[aouts];

        min.ports = new jack_port_t*[mins];

        param.buffers = new float[params];
        param.data    = new ParameterData[params];
        param.ranges  = new ParameterRanges[params];

        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];

        // ---------------------------------------
        // Audio Outputs

        if (carla_options.global_jack_client)
        {
            strncpy(port_name, name, (port_name_size/2)-2);
            strcat(port_name, ":");
            strncat(port_name, "out-left", port_name_size/2);
        }
        else
            strncpy(port_name, "out-left", port_name_size);

        aout.ports[0] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        aout.rindexes[0] = 0;

        if (carla_options.global_jack_client)
        {
            strncpy(port_name, name, (port_name_size/2)-2);
            strcat(port_name, ":");
            strncat(port_name, "out-right", port_name_size/2);
        }
        else
            strncpy(port_name, "out-right", port_name_size);

        aout.ports[1] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        aout.rindexes[1] = 1;

        // ---------------------------------------
        // MIDI Input

        if (carla_options.global_jack_client)
        {
            strncpy(port_name, name, (port_name_size/2)-2);
            strcat(port_name, ":midi-in");
        }
        else
            strcpy(port_name, "midi-in");

        min.ports[0] = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

        // ---------------------------------------
        // Parameters

        if (carla_options.global_jack_client)
        {
            strncpy(port_name, name, (port_name_size/2)-2);
            strcat(port_name, ":control-in");
        }
        else
            strcpy(port_name, "control-in");

        param.port_in = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

        if (carla_options.global_jack_client)
        {
            strncpy(port_name, name, (port_name_size/2)-2);
            strcat(port_name, ":control-out");
        }
        else
            strcpy(port_name, "control-out");

        param.port_out = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

        // ----------------------
        j = Sf2ReverbOnOff;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = 0.0;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 1.0;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbRoomSize;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_ROOMSIZE;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

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
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbLevel;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_LEVEL;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ReverbWidth;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_REVERB_DEFAULT_WIDTH;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusOnOff;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = 0.0;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 1.0;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusNr;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0;
        param.ranges[j].max = 64;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_N;
        param.ranges[j].step = 1.0;
        param.ranges[j].step_small = 1.0;
        param.ranges[j].step_large = 1.0;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusLevel;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 2.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_LEVEL;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusSpeedHz;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 1.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_SPEED;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusDepthMs;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 8.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_DEPTH;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2ChorusType;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE | PARAMETER_USES_SCALEPOINTS;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = FLUID_CHORUS_MOD_SINE;
        param.ranges[j].max = FLUID_CHORUS_MOD_TRIANGLE;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_TYPE;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2Polyphony;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 1;
        param.ranges[j].max = 256;
        param.ranges[j].def = fluid_synth_get_polyphony(f_synth);
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 10;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2Interpolation;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_USES_SCALEPOINTS;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = FLUID_INTERP_NONE;
        param.ranges[j].max = FLUID_INTERP_HIGHEST;
        param.ranges[j].def = FLUID_INTERP_DEFAULT;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param.buffers[j] = param.ranges[j].def;

        // ----------------------
        j = Sf2VoiceCount;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_OUTPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0;
        param.ranges[j].max = 256;
        param.ranges[j].def = 0;
        param.ranges[j].step = 1;
        param.ranges[j].step_small = 1;
        param.ranges[j].step_large = 1;
        param.buffers[j] = param.ranges[j].def;

        // ---------------------------------------

        ain.count   = 0;
        aout.count  = aouts;
        min.count   = mins;
        param.count = params;

        reload_programs(true);

        // Other plugin checks
        hints |= PLUGIN_IS_SYNTH;
        hints |= PLUGIN_CAN_VOL;
        hints |= PLUGIN_CAN_BALANCE;

        carla_proc_lock();
        id = _id;
        carla_proc_unlock();

        if (carla_options.global_jack_client == false)
            jack_activate(jack_client);
#endif
    }

    virtual void reload_programs(bool /*init*/)
    {
#if 0
        qDebug("Sf2AudioPlugin::reload_programs(%s)", bool2str(init));

        // Delete old programs
        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < midiprog.count; i++)
                free((void*)midiprog.names[i]);

            delete[] midiprog.data;
            delete[] midiprog.names;
        }
        midiprog.count = 0;

        // Query new programs
        fluid_sfont_t* f_sfont;
        fluid_preset_t f_preset;

        f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);

        // initial check to know how much midi-programs we get
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            midiprog.count += 1;

        // allocate
        if (midiprog.count > 0)
        {
            midiprog.data  = new midi_program_t [midiprog.count];
            midiprog.names = new const char* [midiprog.count];
        }

        // Update data
        uint32_t i = 0;
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
        {
            if (i < midiprog.count)
            {
                midiprog.data[i].bank    = f_preset.get_banknum(&f_preset);
                midiprog.data[i].program = f_preset.get_num(&f_preset);
                midiprog.names[i] = strdup(f_preset.get_name(&f_preset));
            }
            i++;
        }

        // Update OSC Names
        osc_send_set_midi_program_count(&global_osc_data, id, midiprog.count);

        if (midiprog.count > 0)
        {
            for (i=0; i < midiprog.count; i++)
                osc_send_set_midi_program_data(&global_osc_data, id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.names[i]);
        }

        callback_action(CALLBACK_RELOAD_PROGRAMS, id, 0, 0, 0.0);

        if (init)
        {
            if (midiprog.count > 0)
                set_midi_program(0, false, false, false, true);
        }
#endif
    }

    virtual void process(jack_nframes_t /*nframes*/)
    {
#if 0
        uint32_t i, k;
        unsigned short plugin_id = id;
        unsigned int midi_event_count = 0;

        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* aouts_buffer[aout.count];
        jack_default_audio_sample_t* mins_buffer[min.count];

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        for (i=0; i < min.count; i++)
            mins_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(min.ports[i], nframes);

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_in)
        {
            jack_default_audio_sample_t* pin_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(param.port_in, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            for (i=0; i<n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                unsigned char channel = pin_event.buffer[0] & 0x0F;
                unsigned char mode    = pin_event.buffer[0] & 0xF0;

                // Status change
                if (mode == 0xB0)
                {
                    unsigned char status  = pin_event.buffer[1] & 0x7F;
                    unsigned char velo    = pin_event.buffer[2] & 0x7F;
                    double value, velo_per = double(velo)/127;

                    // Control GUI stuff (channel 0 only)
                    if (channel == 0)
                    {
                        if (status == 0x78)
                        {
                            // All Sound Off
                            set_active(false, false, false);
                            postpone_event(PostEventParameter, PARAMETER_ACTIVE, 0.0);

#if (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 2)
                            for (k=0; k < 16; k++)
                            {
                                fluid_synth_all_notes_off(f_synth, k);
                                fluid_synth_all_sounds_off(f_synth, k);
                            }
#endif

                            break;
                        }
                        else if (status == 0x09 && hints & PLUGIN_CAN_DRYWET)
                        {
                            // Dry/Wet (using '0x09', undefined)
                            set_drywet(velo_per, false, false);
                            postpone_event(PostEventParameter, PARAMETER_DRYWET, velo_per);
                        }
                        else if (status == 0x07 && hints & PLUGIN_CAN_VOL)
                        {
                            // Volume
                            value = double(velo)/100;
                            set_vol(value, false, false);
                            postpone_event(PostEventParameter, PARAMETER_VOLUME, value);
                        }
                        else if (status == 0x08 && hints & PLUGIN_CAN_BALANCE)
                        {
                            // Balance
                            double left, right;
                            value = (double(velo)-63.5)/63.5;

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
                            postpone_event(PostEventParameter, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameter, PARAMETER_BALANCE_RIGHT, right);
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0 &&
                                param.data[k].midi_channel == channel && param.data[k].midi_cc == status)
                        {
                            value = (velo_per * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;
                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameter, k, value);
                        }
                    }
                }
                // Program change
                else if (mode == 0xC0)
                {
                    uint32_t mbank_id = 0;
                    uint32_t mprog_id = pin_event.buffer[1] & 0x7F;

                    if (midiprog.current > 0)
                        mbank_id = midiprog.data[midiprog.current].bank;

                    for (k=0; k < midiprog.count; k++)
                    {
                        if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                        {
                            set_midi_program(k, false, false, false, false);
                            postpone_event(PostEventMidiProgram, k, 0.0);
                            break;
                        }
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (min.count > 0)
        {
            carla_midi_lock();

            for (i=0; i<MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (ExternalMidiNotes[i].valid)
                {
                    if (ExternalMidiNotes[i].plugin_id == plugin_id)
                    {
                        ExternalMidiNote* enote = &ExternalMidiNotes[i];
                        enote->valid = false;

                        if (enote->onoff == true)
                            fluid_synth_noteon(f_synth, 0, enote->note, enote->velo);
                        else
                            fluid_synth_noteoff(f_synth, 0, enote->note);

                        fluid_synth_write_float(f_synth, nframes, aouts_buffer[0], 0, 1, aouts_buffer[1], 0, 1);

                        midi_event_count += 1;
                    }
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (JACK), Plugin processing

        if (min.count > 0)
        {
            jack_nframes_t offset = 0;
            jack_midi_event_t min_event;
            uint32_t n_min_events = jack_midi_get_event_count(mins_buffer[0]);

            for (k=0; k<n_min_events && midi_event_count < MAX_MIDI_EVENTS; k++)
            {
                if (jack_midi_event_get(&min_event, mins_buffer[0], k) != 0)
                    break;

                if (min_event.size != 3)
                    continue;

                if (min_event.time > offset)
                {
                    /* generate audio up to event */
                    fluid_synth_write_float(f_synth, min_event.time - offset, aouts_buffer[0] + offset, 0, 1, aouts_buffer[1] + offset, 0, 1);

                    offset = min_event.time;
                }

                unsigned char channel = min_event.buffer[0] & 0x0F;
                unsigned char mode = min_event.buffer[0] & 0xF0;
                unsigned char note = min_event.buffer[1] & 0x7F;
                unsigned char velo = min_event.buffer[2] & 0x7F;

                // fix bad note off
                if (mode == 0x90 && velo == 0)
                {
                    mode = 0x80;
                    velo = 64;
                }

                if (mode == 0x80)
                {
                    fluid_synth_noteoff(f_synth, channel, note);
                    postpone_event(PostEventNoteOff, note, velo);
                }
                else if (mode == 0x90)
                {
                    fluid_synth_noteon(f_synth, channel, note, velo);
                    postpone_event(PostEventNoteOn, note, velo);
                }
                else if (mode == 0xE0)
                {
                    fluid_synth_pitch_bend(f_synth, channel, (min_event.buffer[2] << 7) | min_event.buffer[1]);
                }

                midi_event_count += 1;
            }

            if (nframes > offset)
            {
                fluid_synth_write_float(f_synth, nframes - offset, aouts_buffer[0] + offset, 0, 1, aouts_buffer[1] + offset, 0, 1);
            }
        } // End of MIDI Input (JACK)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (active)
        {
            double bal_rangeL, bal_rangeR;
            jack_default_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Volume, using fluidsynth internals
                fluid_synth_set_gain(f_synth, x_vol);

                // Balance
                if (hints & PLUGIN_CAN_BALANCE)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_default_audio_sample_t)*nframes);

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
                if (i < 2)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (aouts_buffer[i][k] > aouts_peak_tmp[i])
                            aouts_peak_tmp[i] = aouts_buffer[i][k];
                    }
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_default_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.port_out)
        {
            jack_default_audio_sample_t* cout_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(param.port_out, nframes);
            jack_midi_clear_buffer(cout_buffer);

            k = Sf2VoiceCount;
            param.buffers[k] = fluid_synth_get_active_voice_count(f_synth);

            if (param.data[k].midi_cc >= 0)
            {
                double value_per = (param.buffers[k] - param.ranges[k].min)/(param.ranges[k].max - param.ranges[k].min);

                jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                event_buffer[0] = 0xB0 + param.data[k].midi_channel;
                event_buffer[1] = param.data[k].midi_cc;
                event_buffer[2] = 127*value_per;
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        active_before = active;
#endif
    }
#endif

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

                //osc_new_plugin(plugin);
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

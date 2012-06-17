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

#ifdef BUILD_BRIDGE
#error Should not use fluidsynth for bridges!
#endif

#include "carla_plugin.h"

#ifdef WANT_FLUIDSYNTH
#include <fluidsynth.h>

#if (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 4)
#define FLUIDSYNTH_VERSION_NEW_API
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

class FluidSynthPlugin : public CarlaPlugin
{
public:
    FluidSynthPlugin(unsigned short id) : CarlaPlugin(id)
    {
        qDebug("FluidSynthPlugin::FluidSynthPlugin()");
        m_type = PLUGIN_SF2;

        f_settings = new_fluid_settings();

        fluid_settings_setnum(f_settings, "synth.sample-rate", get_sample_rate());
        fluid_settings_setnum(f_settings, "synth.threadsafe-api ", 0.0);

        f_synth = new_fluid_synth(f_settings);
#ifdef FLUIDSYNTH_VERSION_NEW_API
        fluid_synth_set_sample_rate(f_synth, get_sample_rate());
#endif
        fluid_synth_set_reverb_on(f_synth, 0);
        fluid_synth_set_chorus_on(f_synth, 0);

        m_label = nullptr;
    }

    ~FluidSynthPlugin()
    {
        qDebug("Sf2Plugin::~FluidSynthPlugin()");

        if (m_label)
            free((void*)m_label);

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t param_scalepoint_count(uint32_t param_id)
    {
        assert(param_id < param.count);
        switch (param_id)
        {
        case FluidSynthChorusType:
            return 2;
        case FluidSynthInterpolation:
            return 4;
        default:
            return 0;
        }
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double get_parameter_value(uint32_t param_id)
    {
        assert(param_id < param.count);
        return param_buffers[param_id];
    }

    double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        assert(param_id < param.count);
        assert(scalepoint_id < param_scalepoint_count(param_id));
        switch (param_id)
        {
        case FluidSynthChorusType:
            switch (scalepoint_id)
            {
            case 0:
                return FLUID_CHORUS_MOD_SINE;
            case 1:
                return FLUID_CHORUS_MOD_TRIANGLE;
            default:
                return FLUID_CHORUS_DEFAULT_TYPE;
            }
        case FluidSynthInterpolation:
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

    void get_maker(char* buf_str)
    {
        strncpy(buf_str, "FluidSynth SF2 engine", STR_MAX);
    }

    void get_copyright(char* buf_str)
    {
        strncpy(buf_str, "GNU GPL v2+", STR_MAX);
    }

    void get_real_name(char* buf_str)
    {
        strncpy(buf_str, m_label, STR_MAX);
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        assert(param_id < param.count);
        switch (param_id)
        {
        case FluidSynthReverbOnOff:
            strncpy(buf_str, "Reverb On/Off", STR_MAX);
            break;
        case FluidSynthReverbRoomSize:
            strncpy(buf_str, "Reverb Room Size", STR_MAX);
            break;
        case FluidSynthReverbDamp:
            strncpy(buf_str, "Reverb Damp", STR_MAX);
            break;
        case FluidSynthReverbLevel:
            strncpy(buf_str, "Reverb Level", STR_MAX);
            break;
        case FluidSynthReverbWidth:
            strncpy(buf_str, "Reverb Width", STR_MAX);
            break;
        case FluidSynthChorusOnOff:
            strncpy(buf_str, "Chorus On/Off", STR_MAX);
            break;
        case FluidSynthChorusNr:
            strncpy(buf_str, "Chorus Voice Count", STR_MAX);
            break;
        case FluidSynthChorusLevel:
            strncpy(buf_str, "Chorus Level", STR_MAX);
            break;
        case FluidSynthChorusSpeedHz:
            strncpy(buf_str, "Chorus Speed", STR_MAX);
            break;
        case FluidSynthChorusDepthMs:
            strncpy(buf_str, "Chorus Depth", STR_MAX);
            break;
        case FluidSynthChorusType:
            strncpy(buf_str, "Chorus Type", STR_MAX);
            break;
        case FluidSynthPolyphony:
            strncpy(buf_str, "Polyphony", STR_MAX);
            break;
        case FluidSynthInterpolation:
            strncpy(buf_str, "Interpolation", STR_MAX);
            break;
        case FluidSynthVoiceCount:
            strncpy(buf_str, "Voice Count", STR_MAX);
            break;
        default:
            *buf_str = 0;
            break;
        }
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        assert(param_id < param.count);
        switch (param_id)
        {
        case FluidSynthChorusSpeedHz:
            strncpy(buf_str, "Hz", STR_MAX);
            break;
        case FluidSynthChorusDepthMs:
            strncpy(buf_str, "ms", STR_MAX);
            break;
        default:
            *buf_str = 0;
            break;
        }
    }

    void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        assert(param_id < param.count);
        assert(scalepoint_id < param_scalepoint_count(param_id));
        switch (param_id)
        {
        case FluidSynthChorusType:
            switch (scalepoint_id)
            {
            case 0:
                strncpy(buf_str, "Sine wave", STR_MAX);
                return;
            case 1:
                strncpy(buf_str, "Triangle wave", STR_MAX);
                return;
            }
        case FluidSynthInterpolation:
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

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        assert(param_id < param.count);
        param_buffers[param_id] = fix_parameter_value(value, param.ranges[param_id]);

        switch(param_id)
        {
        case FluidSynthReverbOnOff:
            value = value > 0.5 ? 1 : 0;
            fluid_synth_set_reverb_on(f_synth, value);
            break;

        case FluidSynthReverbRoomSize:
        case FluidSynthReverbDamp:
        case FluidSynthReverbLevel:
        case FluidSynthReverbWidth:
            fluid_synth_set_reverb(f_synth, param_buffers[FluidSynthReverbRoomSize], param_buffers[FluidSynthReverbDamp], param_buffers[FluidSynthReverbWidth], param_buffers[FluidSynthReverbLevel]);
            break;

        case FluidSynthChorusOnOff:
        {
            const CarlaPluginScopedDisabler m(this, ! CarlaEngine::isOffline());
            value = value > 0.5 ? 1 : 0;
            fluid_synth_set_chorus_on(f_synth, value);
            break;
        }

        case FluidSynthChorusNr:
        case FluidSynthChorusLevel:
        case FluidSynthChorusSpeedHz:
        case FluidSynthChorusDepthMs:
        case FluidSynthChorusType:
        {
            const CarlaPluginScopedDisabler m(this, ! CarlaEngine::isOffline());
            fluid_synth_set_chorus(f_synth, rint(param_buffers[FluidSynthChorusNr]), param_buffers[FluidSynthChorusLevel], param_buffers[FluidSynthChorusSpeedHz], param_buffers[FluidSynthChorusDepthMs], rint(param_buffers[FluidSynthChorusType]));
            break;
        }

        case FluidSynthPolyphony:
        {
            const CarlaPluginScopedDisabler m(this, ! CarlaEngine::isOffline());
            fluid_synth_set_polyphony(f_synth, rint(value));
            break;
        }

        case FluidSynthInterpolation:
        {
            const CarlaPluginScopedDisabler m(this, ! CarlaEngine::isOffline());
            for (int i=0; i < 16; i++)
                fluid_synth_set_interp_method(f_synth, i, rint(value));
            break;
        }

        default:
            break;
        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_midi_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        if (cin_channel < 0 || cin_channel > 15)
            return;

        if (index >= 0)
        {
            assert(index < (int32_t)midiprog.count);
            if (CarlaEngine::isOffline())
            {
                if (block) carla_proc_lock();
                fluid_synth_program_select(f_synth, cin_channel, f_id, midiprog.data[index].bank, midiprog.data[index].program);
                if (block) carla_proc_unlock();
            }
            else
            {
                const CarlaPluginScopedDisabler m(this, block);
                fluid_synth_program_select(f_synth, cin_channel, f_id, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::set_midi_program(index, gui_send, osc_send, callback_send, block);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("FluidSynthPlugin::reload() - start");

        // Safely disable plugin for reload
        const CarlaPluginScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        remove_client_ports();

        // Delete old data
        delete_buffers();

        uint32_t aouts, params, j;
        aouts  = 2;
        params = FluidSynthParametersMax;

        aout.ports    = new CarlaEngineAudioPort*[aouts];
        aout.rindexes = new uint32_t[aouts];

        param.data    = new ParameterData[params];
        param.ranges  = new ParameterRanges[params];

        const int port_name_size = CarlaEngine::maxPortNameSize() - 1;
        char port_name[port_name_size];

        // ---------------------------------------
        // Audio Outputs

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":out-left");
        }
        else
#endif
            strcpy(port_name, "out-left");

        aout.ports[0]    = (CarlaEngineAudioPort*)x_client->addPort(port_name, CarlaEnginePortTypeAudio, false);
        aout.rindexes[0] = 0;

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":out-right");
        }
        else
#endif
            strcpy(port_name, "out-right");

        aout.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(port_name, CarlaEnginePortTypeAudio, false);
        aout.rindexes[1] = 1;

        // ---------------------------------------
        // MIDI Input

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":midi-in");
        }
        else
#endif
            strcpy(port_name, "midi-in");

        midi.port_min = (CarlaEngineMidiPort*)x_client->addPort(port_name, CarlaEnginePortTypeMIDI, true);

        // ---------------------------------------
        // Parameters

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":control-in");
        }
        else
#endif
            strcpy(port_name, "control-in");

        param.port_cin = (CarlaEngineControlPort*)x_client->addPort(port_name, CarlaEnginePortTypeControl, true);

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(port_name, m_name);
            strcat(port_name, ":control-out");
        }
        else
#endif
            strcpy(port_name, "control-out");

        param.port_cout = (CarlaEngineControlPort*)x_client->addPort(port_name, CarlaEnginePortTypeControl, false);

        // ----------------------
        j = FluidSynthReverbOnOff;
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
        j = FluidSynthReverbRoomSize;
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
        j = FluidSynthReverbDamp;
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
        j = FluidSynthReverbLevel;
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
        j = FluidSynthReverbWidth;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = -1;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 10.0; // should be 100, but that sounds too much
        param.ranges[j].def = FLUID_REVERB_DEFAULT_WIDTH;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusOnOff;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_BOOLEAN;
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
        j = FluidSynthChorusNr;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;
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
        j = FluidSynthChorusLevel;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED;
        param.data[j].midi_channel = 0;
        param.data[j].midi_cc = 0; //MIDI_CONTROL_CHORUS_SEND_LEVEL;
        param.ranges[j].min = 0.0;
        param.ranges[j].max = 10.0;
        param.ranges[j].def = FLUID_CHORUS_DEFAULT_LEVEL;
        param.ranges[j].step = 0.01;
        param.ranges[j].step_small = 0.0001;
        param.ranges[j].step_large = 0.1;
        param_buffers[j] = param.ranges[j].def;

        // ----------------------
        j = FluidSynthChorusSpeedHz;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED;
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
        j = FluidSynthChorusDepthMs;
        param.data[j].index  = j;
        param.data[j].rindex = j;
        param.data[j].type   = PARAMETER_INPUT;
        param.data[j].hints  = PARAMETER_IS_ENABLED;
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
        j = FluidSynthChorusType;
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
        j = FluidSynthPolyphony;
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
        j = FluidSynthInterpolation;
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
        j = FluidSynthVoiceCount;
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

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        m_hints |= PLUGIN_IS_SYNTH;
        m_hints |= PLUGIN_CAN_VOLUME;
        m_hints |= PLUGIN_CAN_BALANCE;

        reload_programs(true);

        x_client->activate();

        qDebug("FluidSynthPlugin::reload() - end");
    }

    void reload_programs(bool init)
    {
        qDebug("FluidSynthPlugin::reload_programs(%s)", bool2str(init));

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

        // initial check to know how much midi-programs we have
        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            midiprog.count += 1;

        if (midiprog.count > 0)
            midiprog.data = new midi_program_t [midiprog.count];

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

        if (init && midiprog.count > 0)
        {
            fluid_synth_program_reset(f_synth);

            for (i=0; i < 16 && i != 9; i++)
            {
#ifdef FLUIDSYNTH_VERSION_NEW_API
                fluid_synth_set_channel_type(f_synth, i, CHANNEL_TYPE_MELODIC);
#endif
                fluid_synth_program_select(f_synth, i, f_id, midiprog.data[0].bank, midiprog.data[0].program);
            }

#ifdef FLUIDSYNTH_VERSION_NEW_API
            fluid_synth_set_channel_type(f_synth, 9, CHANNEL_TYPE_DRUM);
#endif
            fluid_synth_program_select(f_synth, 9, f_id, 128, 0);

            set_midi_program(0, false, false, false, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** ains_buffer, float** aouts_buffer, uint32_t nframes, uint32_t nframesOffset)
    {
        uint32_t i, k;
        uint32_t midi_event_count = 0;

        double aouts_peak_tmp[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (m_active && m_active_before)
        {
            void* cin_buffer = param.port_cin->getBuffer();

            const CarlaEngineControlEvent* cin_event;
            uint32_t time, n_cin_events = param.port_cin->getEventCount(cin_buffer);

            unsigned char next_bank_ids[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0 };

            if (midiprog.current >= 0 && midiprog.count > 0)
                next_bank_ids[0] = midiprog.data[midiprog.current].bank;

            for (i=0; i < n_cin_events; i++)
            {
                cin_event = param.port_cin->getEvent(cin_buffer, i);

                if (! cin_event)
                    continue;

                time = cin_event->time - nframesOffset;

                if (time >= nframes)
                    continue;

                // Control change
                switch (cin_event->type)
                {
                case CarlaEngineEventControlChange:
                {
                    double value;

                    // Control backend stuff
                    if (cin_event->channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cin_event->controller) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cin_event->value;
                            set_drywet(value, false, false);
                            postpone_event(PluginPostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cin_event->controller) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cin_event->value*127/100;
                            set_volume(value, false, false);
                            postpone_event(PluginPostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_BALANCE(cin_event->controller) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cin_event->value/0.5 - 1.0;

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
                            postpone_event(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midi_channel != cin_event->channel)
                            continue;
                        if (param.data[k].midi_cc != cin_event->controller)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cin_event->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cin_event->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PluginPostEventParameterChange, k, value);
                        }
                    }

                    break;
                }

                case CarlaEngineEventMidiBankChange:
                    if (cin_event->channel < 16)
                        next_bank_ids[cin_event->channel] = cin_event->value;
                    break;

                case CarlaEngineEventMidiProgramChange:
                    if (cin_event->channel < 16)
                    {
                        uint32_t mbank_id = next_bank_ids[cin_event->channel];
                        uint32_t mprog_id = cin_event->value;

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                            {
                                if (cin_event->channel == cin_channel)
                                {
                                    set_midi_program(k, false, false, false, false);
                                    postpone_event(PluginPostEventMidiProgramChange, k, 0.0);
                                }
                                else
                                    fluid_synth_program_select(f_synth, cin_event->channel, f_id, mbank_id, mprog_id);

                                break;
                            }
                        }
                    }
                    break;

                case CarlaEngineEventAllSoundOff:
                    if (cin_event->channel == cin_channel)
                    {
                        send_midi_all_notes_off();
                        midi_event_count += 128;

#ifdef FLUIDSYNTH_VERSION_NEW_API
                        fluid_synth_all_notes_off(f_synth, cin_channel);
                        fluid_synth_all_sounds_off(f_synth, cin_channel);
#endif
                    }
#ifdef FLUIDSYNTH_VERSION_NEW_API
                    else if (cin_event->channel < 16)
                    {
                        fluid_synth_all_notes_off(f_synth, cin_event->channel);
                        fluid_synth_all_sounds_off(f_synth, cin_event->channel);
                    }
#endif
                    break;

                case CarlaEngineEventAllNotesOff:
                    if (cin_event->channel == cin_channel)
                    {
                        send_midi_all_notes_off();
                        midi_event_count += 128;

#ifdef FLUIDSYNTH_VERSION_NEW_API
                        fluid_synth_all_notes_off(f_synth, cin_channel);
#endif
                    }
#ifdef FLUIDSYNTH_VERSION_NEW_API
                    else if (cin_event->channel < 16)
                    {
                        fluid_synth_all_notes_off(f_synth, cin_event->channel);
                    }
#endif
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (cin_channel >= 0 && cin_channel < 16 && m_active && m_active_before)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (extMidiNotes[i].valid)
                {
                    if (extMidiNotes[i].velo)
                        fluid_synth_noteon(f_synth, cin_channel, extMidiNotes[i].note, extMidiNotes[i].velo);
                    else
                        fluid_synth_noteoff(f_synth, cin_channel, extMidiNotes[i].note);

                    extMidiNotes[i].valid = false;
                    midi_event_count += 1;
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (System)

        if (m_active && m_active_before)
        {
            void* min_buffer = midi.port_min->getBuffer();

            const CarlaEngineMidiEvent* min_event;
            uint32_t time, n_min_events = midi.port_min->getEventCount(min_buffer);

            for (i=0; i < n_min_events && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                min_event = midi.port_min->getEvent(min_buffer, i);

                if (! min_event)
                    continue;

                time = min_event->time - nframesOffset;

                if (time >= nframes)
                    continue;

                uint8_t status  = min_event->data[0];
                uint8_t channel = status & 0x0F;

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && min_event->data[2] == 0)
                    status -= 0x10;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = min_event->data[1];

                    fluid_synth_noteoff(f_synth, channel, note);

                    if (channel == cin_channel)
                        postpone_event(PluginPostEventNoteOff, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = min_event->data[1];
                    uint8_t velo = min_event->data[2];

                    fluid_synth_noteon(f_synth, channel, note, velo);

                    if (channel == cin_channel)
                        postpone_event(PluginPostEventNoteOn, note, velo);
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    uint8_t pressure = min_event->data[1];

                    fluid_synth_channel_pressure(f_synth, channel, pressure);
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    uint8_t lsb = min_event->data[1];
                    uint8_t msb = min_event->data[2];

                    fluid_synth_pitch_bend(f_synth, channel, (msb << 7) | lsb);
                }
                else
                    continue;

                midi_event_count += 1;
            }
        } // End of MIDI Input (System)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_active_before)
            {
                if (cin_channel >= 0 && cin_channel < 16)
                {
                    fluid_synth_cc(f_synth, cin_channel, MIDI_CONTROL_ALL_SOUND_OFF, 0);
                    fluid_synth_cc(f_synth, cin_channel, MIDI_CONTROL_ALL_NOTES_OFF, 0);
                }

#ifdef FLUIDSYNTH_VERSION_NEW_API
                for (i=0; i < 16; i++)
                {
                    fluid_synth_all_notes_off(f_synth, i);
                    fluid_synth_all_sounds_off(f_synth, i);
                }
#endif
            }

            fluid_synth_process(f_synth, nframes, 0, ains_buffer, 2, aouts_buffer);
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (volume and balance)

        if (m_active)
        {
            bool do_balance = (x_bal_left != -1.0 || x_bal_right != 1.0);

            double bal_rangeL, bal_rangeR;
            float old_bal_left[do_balance ? nframes : 0];

            for (i=0; i < aout.count; i++)
            {
                // Volume, using fluidsynth internals
                fluid_synth_set_gain(f_synth, x_vol);

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(float)*nframes);

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
                memset(aouts_buffer[i], 0.0f, sizeof(float)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (m_active)
        {
            void* cout_buffer = param.port_cout->getBuffer();

            if (nframesOffset == 0 || ! m_active_before)
                param.port_cout->initBuffer(cout_buffer);

            k = FluidSynthVoiceCount;
            param_buffers[k] = rint(fluid_synth_get_active_voice_count(f_synth));
            fix_parameter_value(param_buffers[k], param.ranges[k]);

            if (param.data[k].midi_cc > 0)
            {
                double value = (param_buffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                param.port_cout->writeEvent(cout_buffer, CarlaEngineEventControlChange, nframesOffset, param.data[k].midi_channel, param.data[k].midi_cc, value);
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        aouts_peak[(m_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(m_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    bool init(const char* filename, const char* label)
    {
        // ---------------------------------------------------------------
        // open soundfont

        f_id = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id < 0)
        {
            set_last_error("Failed to load SoundFont file");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);
        m_label    = strdup(label);
        m_name     = get_unique_name(label);

        // ---------------------------------------------------------------
        // register client

        x_client = new CarlaEngineClient(this);

        if (! x_client->isOk())
        {
            set_last_error("Failed to register plugin client");
            return false;
        }

        return true;
    }

private:
    enum FluidSynthInputParameters {
        FluidSynthReverbOnOff    = 0,
        FluidSynthReverbRoomSize = 1,
        FluidSynthReverbDamp     = 2,
        FluidSynthReverbLevel    = 3,
        FluidSynthReverbWidth    = 4,
        FluidSynthChorusOnOff    = 5,
        FluidSynthChorusNr       = 6,
        FluidSynthChorusLevel    = 7,
        FluidSynthChorusSpeedHz  = 8,
        FluidSynthChorusDepthMs  = 9,
        FluidSynthChorusType     = 10,
        FluidSynthPolyphony      = 11,
        FluidSynthInterpolation  = 12,
        FluidSynthVoiceCount     = 13,
        FluidSynthParametersMax  = 14
    };

    fluid_settings_t* f_settings;
    fluid_synth_t* f_synth;
    int f_id;

    double param_buffers[FluidSynthParametersMax];
    const char* m_label;
};
#endif // WANT_FLUIDSYNTH

short add_plugin_sf2(const char* filename, const char* label)
{
    qDebug("add_plugin_sf2(%s, %s)", filename, label);

#ifdef WANT_FLUIDSYNTH
    short id = get_new_plugin_id();

    if (id >= 0)
    {
        if (fluid_is_soundfont(filename))
        {
            FluidSynthPlugin* plugin = new FluidSynthPlugin(id);

            if (plugin->init(filename, label))
            {
                plugin->reload();

                unique_names[id] = plugin->name();
                CarlaPlugins[id] = plugin;

                plugin->osc_register_new();
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
#else
    set_last_error("fluidsynth support not available");
    return -1;
#endif
}

CARLA_BACKEND_END_NAMESPACE

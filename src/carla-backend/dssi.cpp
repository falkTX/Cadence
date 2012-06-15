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

#include "carla_plugin.h"

#include "dssi/dssi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(unsigned short id) : CarlaPlugin(id)
    {
        qDebug("DssiPlugin::DssiPlugin()");
        m_type = PLUGIN_DSSI;

        handle = nullptr;
        descriptor = nullptr;
        ldescriptor = nullptr;

        param_buffers = nullptr;

        memset(midi_events, 0, sizeof(snd_seq_event_t)*MAX_MIDI_EVENTS);
    }

    ~DssiPlugin()
    {
        qDebug("DssiPlugin::~DssiPlugin()");

#ifndef BUILD_BRIDGE
        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            if (osc.data.target)
            {
                osc_send_hide(&osc.data);
                osc_send_quit(&osc.data);
            }

            if (osc.thread)
            {
                // Wait a bit first, try safe quit, then force kill
                if (osc.thread->isRunning())
                {
                    if (! osc.thread->wait(2000))
                        osc.thread->quit();

                    if (osc.thread->isRunning() && ! osc.thread->wait(1000))
                    {
                        qWarning("Failed to properly stop DSSI GUI thread");
                        osc.thread->terminate();
                    }
                }

                delete osc.thread;
            }

            osc_clear_data(&osc.data);
        }
#endif

        if (handle && ldescriptor && ldescriptor->deactivate && m_active_before)
            ldescriptor->deactivate(handle);

        if (handle && ldescriptor && ldescriptor->cleanup)
            ldescriptor->cleanup(handle);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        if (m_hints & PLUGIN_IS_SYNTH)
            return PLUGIN_CATEGORY_SYNTH;
        return get_category_from_name(m_name);
    }

    long unique_id()
    {
        return ldescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunk_data(void** data_ptr)
    {
        unsigned long long_data_size = 0;
        if (descriptor->get_custom_data(handle, data_ptr, &long_data_size))
            return long_data_size;
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double get_parameter_value(uint32_t param_id)
    {
        assert(param_id < param.count);
        return param_buffers[param_id];
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Label, STR_MAX);
    }

    void get_maker(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Maker, STR_MAX);
    }

    void get_copyright(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Copyright, STR_MAX);
    }

    void get_real_name(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Name, STR_MAX);
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        assert(param_id < param.count);
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, ldescriptor->PortNames[rindex], STR_MAX);
    }

    void get_gui_info(GuiInfo* info)
    {
        if (m_hints & PLUGIN_HAS_GUI)
            info->type = GUI_EXTERNAL_OSC;
        else
            info->type = GUI_NONE;
        info->resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        assert(param_id < param.count);
        param_buffers[param_id] = fix_parameter_value(value, param.ranges[param_id]);

#ifndef BUILD_BRIDGE
        if (gui_send)
            osc_send_control(&osc.data, param.data[param_id].rindex, value);
#endif

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send)
    {
        descriptor->configure(handle, key, value);

#ifndef BUILD_BRIDGE
        if (gui_send)
            osc_send_configure(&osc.data, key, value);
#endif

        if (strcmp(key, "reloadprograms") == 0 || strcmp(key, "load") == 0 || strncmp(key, "patches", 7) == 0)
        {
            const CarlaPluginScopedDisabler m(this);
            reload_programs(false);
        }

        CarlaPlugin::set_custom_data(dtype, key, value, gui_send);
    }

    void set_chunk_data(const char* string_data)
    {
        static QByteArray chunk;
        chunk = QByteArray::fromBase64(string_data);
        descriptor->set_custom_data(handle, chunk.data(), chunk.size());
    }

    void set_midi_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        if (index >= 0)
        {
            assert(index < (int32_t)midiprog.count);
            if (CarlaEngine::isOffline())
            {
                if (block) carla_proc_lock();
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (block) carla_proc_unlock();
            }
            else
            {
                const CarlaPluginScopedDisabler m(this, block);
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }

#ifndef BUILD_BRIDGE
            if (gui_send)
                osc_send_midi_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program, true);
#endif
        }

        CarlaPlugin::set_midi_program(index, gui_send, osc_send, callback_send, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

#ifndef BUILD_BRIDGE
    void show_gui(bool yesno)
    {
        if (yesno)
        {
            osc.thread->start();
        }
        else
        {
            osc_send_hide(&osc.data);
            osc_send_quit(&osc.data);
            osc_clear_data(&osc.data);
            osc.thread->quit(); // FIXME - stop thread?
        }
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("DssiPlugin::reload() - start");

        // Safely disable plugin for reload
        const CarlaPluginScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        remove_client_ports();

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, mins, params, j;
        ains = aouts = mins = params = 0;

        const unsigned long PortCount = ldescriptor->PortCount;

        for (unsigned long i=0; i<PortCount; i++)
        {
            const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
                if (LADSPA_IS_PORT_INPUT(PortType))
                    ains += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                    aouts += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortType))
                params += 1;
        }

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            mins = 1;

        if (ains > 0)
        {
            ain.ports    = new CarlaEngineAudioPort*[ains];
            ain.rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new CarlaEngineAudioPort*[aouts];
            aout.rindexes = new uint32_t[aouts];
        }

        if (params > 0)
        {
            param.data    = new ParameterData[params];
            param.ranges  = new ParameterRanges[params];
            param_buffers = new float[params];
        }

        const int port_name_size = CarlaEngine::maxPortNameSize() - 1;
        char port_name[port_name_size];
        bool needs_cin  = false;
        bool needs_cout = false;

        for (unsigned long i=0; i<PortCount; i++)
        {
            const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint PortHint  = ldescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carla_options.global_jack_client)
                {
                    strcpy(port_name, m_name);
                    strcat(port_name, ":");
                    strncat(port_name, ldescriptor->PortNames[i], port_name_size/2);
                }
                else
#endif
                    strncpy(port_name, ldescriptor->PortNames[i], port_name_size);

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(port_name, CarlaEnginePortTypeAudio, true);
                    ain.rindexes[j] = i;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(port_name, CarlaEnginePortTypeAudio, false);
                    aout.rindexes[j] = i;
                    needs_cin = true;
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LADSPA_IS_PORT_CONTROL(PortType))
            {
                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midi_channel = 0;
                param.data[j].midi_cc = -1;

                double min, max, def, step, step_small, step_large;

                // min value
                if (LADSPA_IS_HINT_BOUNDED_BELOW(PortHint.HintDescriptor))
                    min = PortHint.LowerBound;
                else
                    min = 0.0;

                // max value
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(PortHint.HintDescriptor))
                    max = PortHint.UpperBound;
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                // default value
                if (LADSPA_IS_HINT_HAS_DEFAULT(PortHint.HintDescriptor))
                {
                    switch (PortHint.HintDescriptor & LADSPA_HINT_DEFAULT_MASK)
                    {
                    case LADSPA_HINT_DEFAULT_MINIMUM:
                        def = min;
                        break;
                    case LADSPA_HINT_DEFAULT_MAXIMUM:
                        def = max;
                        break;
                    case LADSPA_HINT_DEFAULT_0:
                        def = 0.0;
                        break;
                    case LADSPA_HINT_DEFAULT_1:
                        def = 1.0;
                        break;
                    case LADSPA_HINT_DEFAULT_100:
                        def = 100.0;
                        break;
                    case LADSPA_HINT_DEFAULT_440:
                        def = 440.0;
                        break;
                    case LADSPA_HINT_DEFAULT_LOW:
                        if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                            def = exp((log(min)*0.75) + (log(max)*0.25));
                        else
                            def = (min*0.75) + (max*0.25);
                        break;
                    case LADSPA_HINT_DEFAULT_MIDDLE:
                        if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                            def = sqrt(min*max);
                        else
                            def = (min+max)/2;
                        break;
                    case LADSPA_HINT_DEFAULT_HIGH:
                        if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                            def = exp((log(min)*0.25) + (log(max)*0.75));
                        else
                            def = (min*0.25) + (max*0.75);
                        break;
                    default:
                        if (min < 0.0 && max > 0.0)
                            def = 0.0;
                        else
                            def = min;
                        break;
                    }
                }
                else
                {
                    // no default value
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
                {
                    double sample_rate = get_sample_rate();
                    min *= sample_rate;
                    max *= sample_rate;
                    def *= sample_rate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_TOGGLED(PortHint.HintDescriptor))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LADSPA_IS_HINT_INTEGER(PortHint.HintDescriptor))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    param.data[j].type   = PARAMETER_INPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needs_cin = true;

                    // MIDI CC value
                    if (descriptor->get_midi_controller_for_port)
                    {
                        int controller = descriptor->get_midi_controller_for_port(handle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                param.data[j].midi_cc = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    if (strcmp(ldescriptor->PortNames[i], "latency") == 0 || strcmp(ldescriptor->PortNames[i], "_latency") == 0)
                    {
                        min = 0.0;
                        max = get_sample_rate();
                        def = 0.0;
                        step = 1.0;
                        step_small = 1.0;
                        step_large = 1.0;

                        param.data[j].type  = PARAMETER_LATENCY;
                        param.data[j].hints = 0;
                    }
                    else
                    {
                        param.data[j].type   = PARAMETER_OUTPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needs_cout = true;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LADSPA_IS_HINT_LOGARITHMIC(PortHint.HintDescriptor))
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].step_small = step_small;
                param.ranges[j].step_large = step_large;

                // Start parameters in their default values
                param_buffers[j] = def;

                ldescriptor->connect_port(handle, i, &param_buffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                ldescriptor->connect_port(handle, i, nullptr);
            }
        }

        if (needs_cin)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-in");
            }
            else
#endif
                strcpy(port_name, "control-in");

            param.port_cin = (CarlaEngineControlPort*)x_client->addPort(port_name, CarlaEnginePortTypeControl, true);
        }

        if (needs_cout)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-out");
            }
            else
#endif
                strcpy(port_name, "control-out");

            param.port_cout = (CarlaEngineControlPort*)x_client->addPort(port_name, CarlaEnginePortTypeControl, false);
        }

        if (mins > 0)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":midi-in");
            }
            else
#endif
                strcpy(port_name, "midi-in");

            midi.port_min = (CarlaEngineMidiPort*)x_client->addPort(port_name, CarlaEnginePortTypeMIDI, true);
        }

        ain.count   = ains;
        aout.count  = aouts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (midi.port_min && aout.count > 0)
            m_hints |= PLUGIN_IS_SYNTH;

#ifndef BUILD_BRIDGE
        if (carla_options.use_dssi_chunks && QString(m_filename).endsWith("dssi-vst.so", Qt::CaseInsensitive))
        {
            if (descriptor->get_custom_data && descriptor->set_custom_data)
                m_hints |= PLUGIN_USES_CHUNKS;
        }
#endif

        if (aouts > 0 && (ains == aouts || ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aouts >= 2 && aouts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        reload_programs(true);

        x_client->activate();

        qDebug("DssiPlugin::reload() - end");
    }

    void reload_programs(bool init)
    {
        qDebug("DssiPlugin::reload_programs(%s)", bool2str(init));
        uint32_t i, old_count = midiprog.count;

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
        if (descriptor->get_program && descriptor->select_program)
        {
            while (descriptor->get_program(handle, midiprog.count))
                midiprog.count += 1;
        }

        if (midiprog.count > 0)
            midiprog.data = new midi_program_t [midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const DSSI_Program_Descriptor* pdesc = descriptor->get_program(handle, i);
            assert(pdesc);
            midiprog.data[i].bank    = pdesc->Bank;
            midiprog.data[i].program = pdesc->Program;
            midiprog.data[i].name    = strdup(pdesc->Name);
        }

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
                set_midi_program(0, false, false, false, true);
        }
        else
        {
            callback_action(CALLBACK_UPDATE, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool program_changed = false;

            if (midiprog.count == old_count+1)
            {
                // one midi program added, probably created by user
                midiprog.current = old_count;
                program_changed  = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                program_changed  = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                program_changed  = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                program_changed  = true;
            }

            if (program_changed)
                set_midi_program(midiprog.current, true, true, true, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** ains_buffer, float** aouts_buffer, uint32_t nframes, uint32_t nframesOffset)
    {
        uint32_t i, k;
        unsigned long midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            if (ain.count == 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                }
            }
            else if (ain.count >= 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);

                    if (abs_d(ains_buffer[1][k]) > ains_peak_tmp[1])
                        ains_peak_tmp[1] = abs_d(ains_buffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin && m_active && m_active_before)
        {
            void* cin_buffer = param.port_cin->getBuffer();

            const CarlaEngineControlEvent* cin_event;
            uint32_t time, n_cin_events = param.port_cin->getEventCount(cin_buffer);

            uint32_t next_bank_id = 0;
            if (midiprog.current >= 0 && midiprog.count > 0)
                next_bank_id = midiprog.data[midiprog.current].bank;

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
                    if (cin_event->channel == cin_channel)
                        next_bank_id = cin_event->value;
                    break;

                case CarlaEngineEventMidiProgramChange:
                    if (cin_event->channel == cin_channel)
                    {
                        uint32_t mbank_id = next_bank_id;
                        uint32_t mprog_id = cin_event->value;

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                            {
                                set_midi_program(k, false, false, false, false);
                                postpone_event(PluginPostEventMidiProgramChange, k, 0.0);
                                break;
                            }
                        }
                        break;
                    }

                case CarlaEngineEventAllSoundOff:
                    if (cin_event->channel == cin_channel)
                    {
                        if (midi.port_min)
                        {
                            send_midi_all_notes_off();
                            midi_event_count += 128;
                        }

                        if (ldescriptor->deactivate)
                            ldescriptor->deactivate(handle);

                        if (ldescriptor->activate)
                            ldescriptor->activate(handle);
                    }
                    break;

                case CarlaEngineEventAllNotesOff:
                    if (cin_event->channel == cin_channel)
                    {
                        if (midi.port_min)
                        {
                            send_midi_all_notes_off();
                            midi_event_count += 128;
                        }
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (midi.port_min && cin_channel >= 0 && cin_channel < 16 && m_active && m_active_before)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (extMidiNotes[i].valid)
                {
                    snd_seq_event_t* midi_event = &midi_events[midi_event_count];
                    memset(midi_event, 0, sizeof(snd_seq_event_t));

                    midi_event->type      = extMidiNotes[i].velo ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                    midi_event->time.tick = nframesOffset; // FIXME - other types may also need time-check here
                    midi_event->data.note.channel  = cin_channel;
                    midi_event->data.note.note     = extMidiNotes[i].note;
                    midi_event->data.note.velocity = extMidiNotes[i].velo;

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

        if (midi.port_min && m_active && m_active_before)
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

                snd_seq_event_t* midi_event = &midi_events[midi_event_count];
                memset(midi_event, 0, sizeof(snd_seq_event_t));

                midi_event->time.tick = time;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = min_event->data[1];

                    midi_event->type = SND_SEQ_EVENT_NOTEOFF;
                    midi_event->data.note.channel = channel;
                    midi_event->data.note.note    = note;

                    if (channel == cin_channel)
                        postpone_event(PluginPostEventNoteOff, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = min_event->data[1];
                    uint8_t velo = min_event->data[2];

                    midi_event->type = SND_SEQ_EVENT_NOTEON;
                    midi_event->data.note.channel  = channel;
                    midi_event->data.note.note     = note;
                    midi_event->data.note.velocity = velo;

                    if (channel == cin_channel)
                        postpone_event(PluginPostEventNoteOn, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = min_event->data[1];
                    uint8_t pressure = min_event->data[2];

                    midi_event->type = SND_SEQ_EVENT_KEYPRESS;
                    midi_event->data.note.channel  = channel;
                    midi_event->data.note.note     = note;
                    midi_event->data.note.velocity = pressure;
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    uint8_t pressure = min_event->data[1];

                    midi_event->type = SND_SEQ_EVENT_CHANPRESS;
                    midi_event->data.control.channel = channel;
                    midi_event->data.control.value   = pressure;
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    uint8_t lsb = min_event->data[1];
                    uint8_t msb = min_event->data[2];

                    midi_event->type = SND_SEQ_EVENT_PITCHBEND;
                    midi_event->data.control.channel = channel;
                    midi_event->data.control.value   = ((msb << 7) | lsb) - 8192;
                }
                else
                    continue;

                midi_event_count += 1;
            }
        } // End of MIDI Input (System)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_active_before)
            {
                if (midi.port_min)
                {
                    memset(&midi_events[0], 0, sizeof(snd_seq_event_t));
                    memset(&midi_events[1], 0, sizeof(snd_seq_event_t));

                    midi_events[0].type = SND_SEQ_EVENT_CONTROLLER;
                    midi_events[0].data.control.param = MIDI_CONTROL_ALL_SOUND_OFF;

                    midi_events[1].type = SND_SEQ_EVENT_CONTROLLER;
                    midi_events[1].data.control.param = MIDI_CONTROL_ALL_NOTES_OFF;

                    midi_event_count = 2;
                }

                if (ldescriptor->activate)
                    ldescriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                ldescriptor->connect_port(handle, ain.rindexes[i], ains_buffer[i]);

            for (i=0; i < aout.count; i++)
                ldescriptor->connect_port(handle, aout.rindexes[i], aouts_buffer[i]);

            if (descriptor->run_synth)
            {
                descriptor->run_synth(handle, nframes, midi_events, midi_event_count);
            }
            else if (descriptor->run_multiple_synths)
            {
                snd_seq_event_t* dssi_events_ptr[] = { midi_events, nullptr };
                descriptor->run_multiple_synths(1, &handle, nframes, dssi_events_ptr, &midi_event_count);
            }
            else if (ldescriptor->run)
                ldescriptor->run(handle, nframes);
        }
        else
        {
            if (m_active_before)
            {
                if (ldescriptor->deactivate)
                    ldescriptor->deactivate(handle);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_vol != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_bal_left != -1.0 || x_bal_right != 1.0);

            double bal_rangeL, bal_rangeR;
            float old_bal_left[do_balance ? nframes : 0];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                if (do_drywet || do_volume)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (do_drywet)
                        {
                            if (aout.count == 1)
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                            else
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                        }

                        if (do_volume)
                            aouts_buffer[i][k] *= x_vol;
                    }
                }

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

        if (param.port_cout && m_active)
        {
            void* cout_buffer = param.port_cout->getBuffer();

            if (nframesOffset == 0 || ! m_active_before)
                param.port_cout->initBuffer(cout_buffer);

            double value;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    fix_parameter_value(param_buffers[k], param.ranges[k]);

                    if (param.data[k].midi_cc > 0)
                    {
                        value = (param_buffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.port_cout->writeEvent(cout_buffer, CarlaEngineEventControlChange, nframesOffset, param.data[k].midi_channel, param.data[k].midi_cc, value);
                    }
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        ains_peak[(m_id*2)+0]  = ains_peak_tmp[0];
        ains_peak[(m_id*2)+1]  = ains_peak_tmp[1];
        aouts_peak[(m_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(m_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    // -------------------------------------------------------------------
    // Cleanup

    void delete_buffers()
    {
        qDebug("DssiPlugin::delete_buffers() - start");

        if (param.count > 0)
            delete[] param_buffers;

        param_buffers = nullptr;

        qDebug("DssiPlugin::delete_buffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* filename, const char* label, const char* gui_filename)
    {
        // ---------------------------------------------------------------
        // open DLL

        if (! lib_open(filename))
        {
            set_last_error(lib_error());
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)lib_symbol("dssi_descriptor");

        if (! descfn)
        {
            set_last_error("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((descriptor = descfn(i++)))
        {
            ldescriptor = descriptor->LADSPA_Plugin;
            if (strcmp(ldescriptor->Label, label) == 0)
                break;
        }

        if (! descriptor)
        {
            set_last_error("Could not find the requested plugin Label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        handle = ldescriptor->instantiate(ldescriptor, get_sample_rate());

        if (! handle)
        {
            set_last_error("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);
        m_name = get_unique_name(ldescriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = new CarlaEngineClient(this);

        if (! x_client->isOk())
        {
            set_last_error("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

#ifndef BUILD_BRIDGE
        if (gui_filename)
        {
            osc.thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
            osc.thread->setOscData(gui_filename, ldescriptor->Label);

            m_hints |= PLUGIN_HAS_GUI;
        }
#else
        Q_UNUSED(gui_filename);
#endif
        return true;
    }

private:
    LADSPA_Handle handle;
    const LADSPA_Descriptor* ldescriptor;
    const DSSI_Descriptor* descriptor;
    snd_seq_event_t midi_events[MAX_MIDI_EVENTS];

    float* param_buffers;
};

short add_plugin_dssi(const char* filename, const char* label, const void* extra_stuff)
{
    qDebug("add_plugin_dssi(%s, %s, %p)", filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        DssiPlugin* plugin = new DssiPlugin(id);

        if (plugin->init(filename, label, (const char*)extra_stuff))
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
        set_last_error("Maximum number of plugins reached");

    return id;
}

CARLA_BACKEND_END_NAMESPACE

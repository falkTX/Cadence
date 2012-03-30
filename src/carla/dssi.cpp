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
#include "carla_osc.h"
#include "carla_threads.h"

#include "dssi/dssi.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin() :
        CarlaPlugin()
    {
        qDebug("DssiPlugin::DssiPlugin()");
        m_type = PLUGIN_DSSI;

        handle = nullptr;
        descriptor = nullptr;
        ldescriptor = nullptr;

        memset(&midi_events, 0, sizeof(snd_seq_event_t)*MAX_MIDI_EVENTS);
    }

    virtual ~DssiPlugin()
    {
        qDebug("DssiPlugin::~DssiPlugin()");

        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            if (osc.data.path)
            {
                osc_send_hide(&osc.data);
                osc_send_quit(&osc.data);
            }

            if (osc.thread)
            {
                if (osc.thread->isRunning())
                {
                    osc.thread->quit();

                    if (osc.thread->wait(3000) == false) // 3 sec
                        qWarning("Failed to properly stop DSSI GUI thread");
                }

                delete osc.thread;
            }

            osc_clear_data(&osc.data);
        }

        if (handle && ldescriptor->deactivate && m_active_before)
            ldescriptor->deactivate(handle);

        if (handle && ldescriptor->cleanup)
            ldescriptor->cleanup(handle);

        handle = nullptr;
        descriptor = nullptr;
        ldescriptor = nullptr;
    }

    virtual PluginCategory category()
    {
        if (min.count > 0 && aout.count > 0)
            return PLUGIN_CATEGORY_SYNTH;
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return ldescriptor->UniqueID;
    }

    virtual void get_label(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Label, STR_MAX);
    }

    virtual void get_maker(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Maker, STR_MAX);
    }

    virtual void get_copyright(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Copyright, STR_MAX);
    }

    virtual void get_real_name(char* buf_str)
    {
        strncpy(buf_str, ldescriptor->Name, STR_MAX);
    }

    virtual void get_parameter_name(uint32_t index, char* buf_str)
    {
        int32_t rindex = param.data[index].rindex;
        strncpy(buf_str, ldescriptor->PortNames[rindex], STR_MAX);
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        if (m_hints & PLUGIN_HAS_GUI)
            info->type = GUI_EXTERNAL_OSC;
        else
            info->type = GUI_NONE;
    }

    virtual int32_t get_chunk_data(void** data_ptr)
    {
        unsigned long long_data_size = 0;
        if (descriptor->get_custom_data(handle, data_ptr, &long_data_size))
            return long_data_size;
        return 0;
    }

    virtual double get_current_parameter_value(uint32_t index)
    {
        return param_buffers[index];
    }

    virtual void set_parameter_value(uint32_t index, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        param_buffers[index] = value;

        if (gui_send)
            osc_send_control(&osc.data, param.data[index].rindex, value);

        CarlaPlugin::set_parameter_value(index, value, gui_send, osc_send, callback_send);
    }

    virtual void set_custom_data(const char* type, const char* key, const char* value, bool gui_send)
    {
        descriptor->configure(handle, key, value);

        if (gui_send)
            osc_send_configure(&osc.data, key, value);

        if (strcmp(key, "reloadprograms") == 0 || strcmp(key, "load") == 0 || strncmp(key, "patches", 7) == 0)
        {
            reload_programs(false);
        }
        else if (strcmp(key, "names") == 0) // Not in the API!
        {
            //if (prog.count > 0)
            //{
                //osc_send_set_program_count(&global_osc_data, m_id, prog.count);

//                // Parse names
//                int j, k, last_str_n = 0;
//                int str_len = strlen(value);
//                char name[256];

//                for (uint32_t i=0; i < prog.count; i++)
//                {
//                    for (j=0, k=last_str_n; j<256 && k+j<str_len; j++)
//                    {
//                        name[j] = value[k+j];
//                        if (value[k+j] == ',')
//                        {
//                            name[j] = 0;
//                            last_str_n = k+j+1;
//                            free((void*)prog.names[i]);
//                            prog.names[i] = strdup(name);
//                            break;
//                        }
//                    }

//                    osc_send_set_program_name(&osc.data, id, i, prog.names[i]);
//                }

//                callback_action(CALLBACK_RELOAD_PROGRAMS, id, 0, 0, 0.0f);
            //}
        }

        CarlaPlugin::set_custom_data(type, key, value, gui_send);
    }

    virtual void set_chunk_data(const char* string_data)
    {
        QByteArray chunk = QByteArray::fromBase64(string_data);
        descriptor->set_custom_data(handle, chunk.data(), chunk.size());
    }

    virtual void show_gui(bool yesno)
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
        }
    }

    virtual void reload()
    {
        qDebug("DssiPlugin::reload()");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        if (carla_options.global_jack_client == false && _id >= 0)
            jack_deactivate(jack_client);

        // Unregister previous jack ports
        remove_from_jack();

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, mins, params, j;
        ains = aouts = mins = params = 0;

        const unsigned long PortCount = ldescriptor->PortCount;

        for (unsigned long i=0; i<PortCount; i++)
        {
            LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
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
            ain.rindexes = new uint32_t[ains];
            ain.ports    = new jack_port_t*[ains];
        }

        if (aouts > 0)
        {
            aout.rindexes = new uint32_t[aouts];
            aout.ports    = new jack_port_t*[aouts];
        }

        if (mins > 0)
        {
            min.ports = new jack_port_t*[mins];
        }

        if (params > 0)
        {
            param.data    = new ParameterData[params];
            param.ranges  = new ParameterRanges[params];
            param_buffers = new float[params];
        }

        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];
        bool needs_cin  = false;
        bool needs_cout = false;

        for (unsigned long i=0; i<PortCount; i++)
        {
            LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
            LADSPA_PortRangeHint PortHint  = ldescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
                if (carla_options.global_jack_client)
                {
                    strcpy(port_name, m_name);
                    strcat(port_name, ":");
                    strncat(port_name, ldescriptor->PortNames[i], port_name_size/2);
                }
                else
                    strncpy(port_name, ldescriptor->PortNames[i], port_name_size/2);

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                    ain.rindexes[j] = i;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
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

                if (max - min <= 0.0)
                {
                    qWarning("Broken plugin parameter -> max - min <= 0");
                    max = min + 0.1;
                }

                if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
                {
                    double sample_rate = get_sample_rate();
                    min *= sample_rate;
                    max *= sample_rate;
                    def *= sample_rate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LADSPA_IS_HINT_INTEGER(PortHint.HintDescriptor))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                }
                else if (LADSPA_IS_HINT_TOGGLED(PortHint.HintDescriptor))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
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
                    param.data[j].type = PARAMETER_INPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needs_cin = true;

                    // MIDI CC value
                    if (descriptor->get_midi_controller_for_port)
                    {
                        int cc = descriptor->get_midi_controller_for_port(handle, i);
                        if (DSSI_CONTROLLER_IS_SET(cc) && DSSI_IS_CC(cc))
                            param.data[j].midi_cc = DSSI_CC_NUMBER(cc);
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    param.data[j].type = PARAMETER_OUTPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;

                    if (strcmp(ldescriptor->PortNames[i], "latency") != 0 && strcmp(ldescriptor->PortNames[i], "_latency") != 0)
                    {
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needs_cout = true;
                    }
                    else
                    {
                        // latency parameter
                        min = 0;
                        max = get_sample_rate();
                        def = 0;
                        step = 1;
                        step_small = 1;
                        step_large = 1;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

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
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-in");
            }
            else
                strcpy(port_name, "control-in");

            param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        if (needs_cout)
        {
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-out");
            }
            else
                strcpy(port_name, "control-out");

            param.port_cout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }

        if (mins == 1)
        {
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":midi-in");
            }
            else
                strcpy(port_name, "midi-in");

            min.ports[0] = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        ain.count   = ains;
        aout.count  = aouts;
        min.count   = mins;
        param.count = params;

        // reload_programs(true);

        // plugin checks
        qDebug("Before: %i", m_hints);
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);
        qDebug("After:  %i", m_hints);

        if (min.count > 0 && aout.count > 0)
            m_hints |= PLUGIN_IS_SYNTH;

        if (/*carla_options.use_dssi_chunks &&*/ QString(ldescriptor->Name).endsWith(" VST", Qt::CaseSensitive))
        {
            if (descriptor->get_custom_data && descriptor->set_custom_data)
                m_hints |= PLUGIN_USES_CHUNKS;
        }

        if (aouts > 0 && (ains == aouts || ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aouts >= 2 && aouts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

        if (carla_options.global_jack_client == false)
            jack_activate(jack_client);
    }

    virtual void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        unsigned long midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* ains_buffer[ain.count];
        jack_default_audio_sample_t* aouts_buffer[aout.count];
        void* min_buffer;

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        if (min.count == 1)
            min_buffer = jack_port_get_buffer(min.ports[0], nframes);

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            short j2 = (ain.count == 1) ? 0 : 1;

            for (k=0; k<nframes; k++)
            {
                if (ains_buffer[0][k] > ains_peak_tmp[0])
                    ains_peak_tmp[0] = ains_buffer[0][k];
                if (ains_buffer[j2][k] > ains_peak_tmp[1])
                    ains_peak_tmp[1] = ains_buffer[j2][k];
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin)
        {
            jack_default_audio_sample_t* pin_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(param.port_cin, nframes);

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
                            postpone_event(PostEventParameterChange, PARAMETER_ACTIVE, 0.0);
                            break;
                        }
                        else if (status == 0x09 && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            // Dry/Wet (using '0x09', undefined)
                            set_drywet(velo_per, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, velo_per);
                        }
                        else if (status == 0x07 && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            // Volume
                            value = double(velo)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                        }
                        else if (status == 0x08 && (m_hints & PLUGIN_CAN_BALANCE) > 0)
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
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
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
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (mode == 0xC0)
                {
                    // TODO
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // TODO - Midi Input

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
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
            double bal_rangeL, bal_rangeR;
            jack_default_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                for (k=0; k<nframes; k++)
                {
                    if ((m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0)
                    {
                        if (aout.count == 1)
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                        else
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                    }

                    if (m_hints & PLUGIN_CAN_VOLUME)
                        aouts_buffer[i][k] *= x_vol;
                }

                // Balance
                if (m_hints & PLUGIN_CAN_BALANCE)
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

        if (param.port_cout)
        {
            jack_default_audio_sample_t* cout_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(param.port_cout, nframes);
            jack_midi_clear_buffer(cout_buffer);

            double value_per;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT && param.data[k].midi_cc >= 0)
                {
                    value_per = (param_buffers[k] - param.ranges[k].min)/(param.ranges[k].max - param.ranges[k].min);

                    jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                    event_buffer[0] = 0xB0 + param.data[k].midi_channel;
                    event_buffer[1] = param.data[k].midi_cc;
                    event_buffer[2] = 127*value_per;
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        ains_peak[(plugin_id*2)+0]  = ains_peak_tmp[0];
        ains_peak[(plugin_id*2)+1]  = ains_peak_tmp[1];
        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    bool init(const char* filename, const char* label, void* extra_stuff)
    {
        if (lib_open(filename))
        {
            DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)lib_symbol("dssi_descriptor");

            if (descfn)
            {
                unsigned long i = 0;
                while ((descriptor = descfn(i++)))
                {
                    ldescriptor = descriptor->LADSPA_Plugin;
                    if (strcmp(ldescriptor->Label, label) == 0)
                        break;
                }

                if (descriptor && ldescriptor)
                {
                    handle = ldescriptor->instantiate(ldescriptor, get_sample_rate());

                    if (handle)
                    {
                        m_filename = strdup(filename);
                        m_name = get_unique_name(ldescriptor->Name);

                        if (register_jack_plugin())
                        {
                            if (extra_stuff)
                            {
                                // GUI Stuff
                                const char* gui_filename = (char*)extra_stuff;

                                osc.thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
                                osc.thread->setOscData(gui_filename, ldescriptor->Label);

                                m_hints |= PLUGIN_HAS_GUI;
                            }

                            return true;
                        }
                        else
                            set_last_error("Failed to register plugin in JACK");
                    }
                    else
                        set_last_error("Plugin failed to initialize");
                }
                else
                    set_last_error("Could not find the requested plugin Label in the plugin library");
            }
            else
                set_last_error("Could not find the LASDPA Descriptor in the plugin library");
        }
        else
            set_last_error(lib_error());

        return false;
    }

private:
    LADSPA_Handle handle;
    const LADSPA_Descriptor* ldescriptor;
    const DSSI_Descriptor* descriptor;

    float* param_buffers;
    snd_seq_event_t midi_events[MAX_MIDI_EVENTS];
};

short add_plugin_dssi(const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_dssi(%s, %s, %p)", filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        DssiPlugin* plugin = new DssiPlugin;

        if (plugin->init(filename, label, extra_stuff))
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
        set_last_error("Maximum number of plugins reached");

    return id;
}

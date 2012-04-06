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

#include "ladspa/ladspa.h"
#include "ladspa_rdf.h"

bool is_port_good(int Type1, int Type2)
{
    if (LADSPA_IS_PORT_INPUT(Type1) && ! LADSPA_IS_PORT_INPUT(Type2))
        return false;
    else if (LADSPA_IS_PORT_OUTPUT(Type1) && ! LADSPA_IS_PORT_OUTPUT(Type2))
        return false;
    else if (LADSPA_IS_PORT_CONTROL(Type1) && ! LADSPA_IS_PORT_CONTROL(Type2))
        return false;
    else if (LADSPA_IS_PORT_AUDIO(Type1) && ! LADSPA_IS_PORT_AUDIO(Type2))
        return false;
    else
        return true;
}

bool is_ladspa_rdf_descriptor_valid(const LADSPA_RDF_Descriptor* rdf_descriptor, const LADSPA_Descriptor* descriptor)
{
    if (rdf_descriptor)
    {
        if (rdf_descriptor->PortCount <= descriptor->PortCount)
        {
            for (unsigned long i=0; i < rdf_descriptor->PortCount; i++)
            {
                if (is_port_good(rdf_descriptor->Ports[i].Type, descriptor->PortDescriptors[i]) == false)
                {
                    qWarning("WARNING - Plugin has RDF data, but invalid PortTypes - %i != %i", rdf_descriptor->Ports[i].Type, descriptor->PortDescriptors[i]);
                    return false;
                }
            }
            return true;
        }
        else
        {
            qWarning("WARNING - Plugin has RDF data, but invalid PortCount -> %li > %li", rdf_descriptor->PortCount, descriptor->PortCount);
            return false;
        }
    }
    else
        // No RDF Descriptor
        return false;
}

class LadspaPlugin : public CarlaPlugin
{
public:
    LadspaPlugin() : CarlaPlugin()
    {
        qDebug("LadspaPlugin::LadspaPlugin()");
        m_type = PLUGIN_LADSPA;

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        ain_rindexes  = nullptr;
        aout_rindexes = nullptr;
        param_buffers = nullptr;
    }

    virtual ~LadspaPlugin()
    {
        qDebug("LadspaPlugin::~LadspaPlugin()");

        if (handle && descriptor->deactivate && m_active_before)
            descriptor->deactivate(handle);

        if (handle && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            ladspa_rdf_free(rdf_descriptor);

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;
    }

    virtual PluginCategory category()
    {
        if (rdf_descriptor)
        {
            LADSPA_Properties Category = rdf_descriptor->Type;

            // Specific Types
            if (Category & (LADSPA_CLASS_DELAY|LADSPA_CLASS_REVERB))
                return PLUGIN_CATEGORY_DELAY;
            else if (Category & (LADSPA_CLASS_PHASER|LADSPA_CLASS_FLANGER|LADSPA_CLASS_CHORUS))
                return PLUGIN_CATEGORY_MODULATOR;
            else if (Category & (LADSPA_CLASS_AMPLIFIER))
                return PLUGIN_CATEGORY_DYNAMICS;
            else if (Category & (LADSPA_CLASS_UTILITY|LADSPA_CLASS_SPECTRAL|LADSPA_CLASS_FREQUENCY_METER))
                return PLUGIN_CATEGORY_UTILITY;

            // Pre-set LADSPA Types
            else if (LADSPA_IS_PLUGIN_DYNAMICS(Category))
                return PLUGIN_CATEGORY_DYNAMICS;
            else if (LADSPA_IS_PLUGIN_AMPLITUDE(Category))
                return PLUGIN_CATEGORY_MODULATOR;
            else if (LADSPA_IS_PLUGIN_EQ(Category))
                return PLUGIN_CATEGORY_EQ;
            else if (LADSPA_IS_PLUGIN_FILTER(Category))
                return PLUGIN_CATEGORY_FILTER;
            else if (LADSPA_IS_PLUGIN_FREQUENCY(Category))
                return PLUGIN_CATEGORY_UTILITY;
            else if (LADSPA_IS_PLUGIN_SIMULATOR(Category))
                return PLUGIN_CATEGORY_OUTRO;
            else if (LADSPA_IS_PLUGIN_TIME(Category))
                return PLUGIN_CATEGORY_DELAY;
            else if (LADSPA_IS_PLUGIN_GENERATOR(Category))
                return PLUGIN_CATEGORY_SYNTH;
        }

        // TODO - try to get category from label
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return descriptor->UniqueID;
    }

    virtual uint32_t param_scalepoint_count(uint32_t param_id)
    {
        int32_t rindex = param.data[param_id].rindex;

        bool HasPortRDF = (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount);
        if (HasPortRDF)
            return rdf_descriptor->Ports[rindex].ScalePointCount;
        else
            return 0;
    }

    virtual double get_parameter_value(uint32_t param_id)
    {
        return param_buffers[param_id];
    }

    virtual double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        int32_t param_rindex = param.data[param_id].rindex;

        bool HasPortRDF = (rdf_descriptor && param_rindex < (int32_t)rdf_descriptor->PortCount);
        if (HasPortRDF)
            return rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Value;
        else
            return 0.0;
    }

    virtual void get_label(char* buf_str)
    {
        strncpy(buf_str, descriptor->Label, STR_MAX);
    }

    virtual void get_maker(char* buf_str)
    {
        strncpy(buf_str, descriptor->Maker, STR_MAX);
    }

    virtual void get_copyright(char* buf_str)
    {
        strncpy(buf_str, descriptor->Copyright, STR_MAX);
    }

    virtual void get_real_name(char* buf_str)
    {
        if (rdf_descriptor && rdf_descriptor->Title)
            strncpy(buf_str, rdf_descriptor->Title, STR_MAX);
        else
            strncpy(buf_str, descriptor->Name, STR_MAX);
    }

    virtual void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, descriptor->PortNames[rindex], STR_MAX);
    }

    virtual void get_parameter_symbol(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;

        bool HasPortRDF = (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount);
        if (HasPortRDF)
        {
            LADSPA_RDF_Port Port = rdf_descriptor->Ports[rindex];
            if (LADSPA_PORT_HAS_LABEL(Port.Hints))
            {
                strncpy(buf_str, Port.Label, STR_MAX);
                return;
            }
        }
        *buf_str = 0;
    }

    virtual void get_parameter_label(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;

        bool HasPortRDF = (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount);
        if (HasPortRDF)
        {
            LADSPA_RDF_Port Port = rdf_descriptor->Ports[rindex];
            if (LADSPA_PORT_HAS_UNIT(Port.Hints))
            {
                switch (Port.Unit)
                {
                case LADSPA_UNIT_DB:
                    strncpy(buf_str, "dB", STR_MAX);
                    return;
                case LADSPA_UNIT_COEF:
                    strncpy(buf_str, "(coef)", STR_MAX);
                    return;
                case LADSPA_UNIT_HZ:
                    strncpy(buf_str, "Hz", STR_MAX);
                    return;
                case LADSPA_UNIT_S:
                    strncpy(buf_str, "s", STR_MAX);
                    return;
                case LADSPA_UNIT_MS:
                    strncpy(buf_str, "ms", STR_MAX);
                    return;
                case LADSPA_UNIT_MIN:
                    strncpy(buf_str, "min", STR_MAX);
                    return;
                }
            }
        }
        *buf_str = 0;
    }

    virtual void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        int32_t param_rindex = param.data[param_id].rindex;

        bool HasPortRDF = (rdf_descriptor && param_rindex < (int32_t)rdf_descriptor->PortCount);
        if (HasPortRDF)
            strncpy(buf_str, rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Label, STR_MAX);
        else
            *buf_str = 0;
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        param_buffers[param_id] = value;

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    virtual void reload()
    {
        qDebug("LadspaPlugin::reload()");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        if (_id >= 0)
            remove_from_jack();

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, params, j;
        ains = aouts = params = 0;

        const unsigned long PortCount = descriptor->PortCount;

        for (unsigned long i=0; i<PortCount; i++)
        {
            const LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[i];
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

        if (ains > 0)
        {
            ain.ports    = new jack_port_t*[ains];
            ain_rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new jack_port_t*[aouts];
            aout_rindexes = new uint32_t[aouts];
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
            const LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint PortHint  = descriptor->PortRangeHints[i];
            bool HasPortRDF = (rdf_descriptor && i < rdf_descriptor->PortCount);

            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carla_options.global_jack_client)
                {
                    strcpy(port_name, m_name);
                    strcat(port_name, ":");
                    strncat(port_name, descriptor->PortNames[i], port_name_size/2);
                }
                else
#endif
                    strncpy(port_name, descriptor->PortNames[i], port_name_size/2);

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                    ain_rindexes[j] = i;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    aout_rindexes[j] = i;
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
                if (HasPortRDF && LADSPA_PORT_HAS_DEFAULT(rdf_descriptor->Ports[j].Hints))
                    def = rdf_descriptor->Ports[j].Default;

                else if (LADSPA_IS_HINT_HAS_DEFAULT(PortHint.HintDescriptor))
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
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    param.data[j].type = PARAMETER_OUTPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;

                    if (strcmp(descriptor->PortNames[i], "latency") != 0 && strcmp(descriptor->PortNames[i], "_latency") != 0)
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

                // check for scalepoints, require at least 2 to make it useful
                if (HasPortRDF && rdf_descriptor->Ports[i].ScalePointCount > 1)
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].step_small = step_small;
                param.ranges[j].step_large = step_large;

                // Start parameters in their default values
                param_buffers[j] = def;

                descriptor->connect_port(handle, i, &param_buffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                descriptor->connect_port(handle, i, nullptr);
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

            param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
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

            param.port_cout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }

        ain.count   = ains;
        aout.count  = aouts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

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

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* ains_buffer[ain.count];
        jack_default_audio_sample_t* aouts_buffer[aout.count];

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            short j2 = (ain.count == 1) ? 0 : 1;

            for (k=0; k<nframes; k++)
            {
                if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                    ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                if (abs_d(ains_buffer[j2][k]) > ains_peak_tmp[1])
                    ains_peak_tmp[1] = abs_d(ains_buffer[j2][k]);
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

            for (i=0; i < n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                jack_midi_data_t status = pin_event.buffer[0];
                unsigned char channel   = status & 0x0F;

                // Control change
                if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    jack_midi_data_t control = pin_event.buffer[1];
                    jack_midi_data_t c_value = pin_event.buffer[2];

                    double value;

                    // Control GUI stuff (channel 0 only)
                    if (channel == 0)
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
                            if (m_active && m_active_before)
                            {
                                if (descriptor->deactivate)
                                    descriptor->deactivate(handle);

                                m_active_before = false;
                            }
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0 && param.data[k].midi_channel == channel && param.data[k].midi_cc == control)
                        {
                            value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;
                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                descriptor->connect_port(handle, ain_rindexes[i], ains_buffer[i]);

            for (i=0; i < aout.count; i++)
                descriptor->connect_port(handle, aout_rindexes[i], aouts_buffer[i]);

            if (descriptor->run)
                descriptor->run(handle, nframes);
        }
        else
        {
            if (m_active_before)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
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
                        if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                            aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
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

            double value;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT && param.data[k].midi_cc > 0)
                {
                    value = (param_buffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min) * 127;

                    jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                    event_buffer[0] = 0xB0 + param.data[k].midi_channel;
                    event_buffer[1] = param.data[k].midi_cc;
                    event_buffer[2] = value;
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

    virtual void delete_buffers()
    {
        qDebug("LadspaPlugin::delete_buffers() - start");

        if (ain.count > 0)
            delete[] ain_rindexes;

        if (aout.count > 0)
            delete[] aout_rindexes;

        if (param.count > 0)
            delete[] param_buffers;

        ain_rindexes  = nullptr;
        aout_rindexes = nullptr;
        param_buffers = nullptr;

        qDebug("LadspaPlugin::delete_buffers() - end");
    }

    bool init(const char* filename, const char* label, void* extra_stuff)
    {
        if (lib_open(filename))
        {
            LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function)lib_symbol("ladspa_descriptor");

            if (descfn)
            {
                unsigned long i = 0;
                while ((descriptor = descfn(i++)))
                {
                    if (strcmp(descriptor->Label, label) == 0)
                        break;
                }

                if (descriptor)
                {
                    handle = descriptor->instantiate(descriptor, get_sample_rate());

                    if (handle)
                    {
                        m_filename = strdup(filename);

                        const LADSPA_RDF_Descriptor* rdf_descriptor_ = (LADSPA_RDF_Descriptor*)extra_stuff;

                        if (is_ladspa_rdf_descriptor_valid(rdf_descriptor_, descriptor))
                            rdf_descriptor = ladspa_rdf_dup(rdf_descriptor_);

                        if (rdf_descriptor && rdf_descriptor->Title)
                            m_name = get_unique_name(rdf_descriptor->Title);
                        else
                            m_name = get_unique_name(descriptor->Name);

                        if (carla_jack_register_plugin(this, &jack_client))
                            return true;
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
    const LADSPA_Descriptor* descriptor;
    const LADSPA_RDF_Descriptor* rdf_descriptor;

    float* param_buffers;
    uint32_t* ain_rindexes;
    uint32_t* aout_rindexes;
};

short add_plugin_ladspa(const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_ladspa(%s, %s, %p)", filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        LadspaPlugin* plugin = new LadspaPlugin;

        if (plugin->init(filename, label, extra_stuff))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
            //osc_new_plugin(plugin);
#endif
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

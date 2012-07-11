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
#include "carla_ladspa_includes.h"
#include "carla_lv2_includes.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendLadspaPlugin Carla Backend LADSPA Plugin
 *
 * The Carla Backend LADSPA Plugin.\n
 * http://www.ladspa.org/
 * @{
 */

class LadspaPlugin : public CarlaPlugin
{
public:
    LadspaPlugin(CarlaEngine* const engine, unsigned short id) : CarlaPlugin(engine, id)
    {
        qDebug("LadspaPlugin::LadspaPlugin()");

        m_type = PLUGIN_LADSPA;

        handle = h2 = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        param_buffers = nullptr;
    }

    ~LadspaPlugin()
    {
        qDebug("LadspaPlugin::~LadspaPlugin()");

        if (descriptor)
        {
            if (descriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    descriptor->deactivate(handle);
                if (h2)
                    descriptor->deactivate(h2);
            }

            if (descriptor->cleanup)
            {
                if (handle)
                    descriptor->cleanup(handle);
                if (h2)
                    descriptor->cleanup(h2);
            }
        }

        if (rdf_descriptor)
            ladspa_rdf_free(rdf_descriptor);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        if (rdf_descriptor)
        {
            LADSPA_Properties Category = rdf_descriptor->Type;

            // Specific Types
            if (Category & (LADSPA_CLASS_DELAY|LADSPA_CLASS_REVERB))
                return PLUGIN_CATEGORY_DELAY;
            if (Category & (LADSPA_CLASS_PHASER|LADSPA_CLASS_FLANGER|LADSPA_CLASS_CHORUS))
                return PLUGIN_CATEGORY_MODULATOR;
            if (Category & (LADSPA_CLASS_AMPLIFIER))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (Category & (LADSPA_CLASS_UTILITY|LADSPA_CLASS_SPECTRAL|LADSPA_CLASS_FREQUENCY_METER))
                return PLUGIN_CATEGORY_UTILITY;

            // Pre-set LADSPA Types
            if (LADSPA_IS_PLUGIN_DYNAMICS(Category))
                return PLUGIN_CATEGORY_DYNAMICS;
            if (LADSPA_IS_PLUGIN_AMPLITUDE(Category))
                return PLUGIN_CATEGORY_MODULATOR;
            if (LADSPA_IS_PLUGIN_EQ(Category))
                return PLUGIN_CATEGORY_EQ;
            if (LADSPA_IS_PLUGIN_FILTER(Category))
                return PLUGIN_CATEGORY_FILTER;
            if (LADSPA_IS_PLUGIN_FREQUENCY(Category))
                return PLUGIN_CATEGORY_UTILITY;
            if (LADSPA_IS_PLUGIN_SIMULATOR(Category))
                return PLUGIN_CATEGORY_OTHER;
            if (LADSPA_IS_PLUGIN_TIME(Category))
                return PLUGIN_CATEGORY_DELAY;
            if (LADSPA_IS_PLUGIN_GENERATOR(Category))
                return PLUGIN_CATEGORY_SYNTH;
        }

        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        return descriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t parameterScalePointCount(uint32_t parameterId)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
            return rdf_descriptor->Ports[rindex].ScalePointCount;

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(uint32_t parameterId)
    {
        assert(parameterId < param.count);
        return param_buffers[parameterId];
    }

    double getParameterScalePointValue(uint32_t parameterId, uint32_t scalePointId)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_ScalePoint* const scalePoint = &rdf_descriptor->Ports[rindex].ScalePoints[scalePointId];

            if (scalePoint)
                return rdf_descriptor->Ports[rindex].ScalePoints[scalePointId].Value;
        }

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        if (descriptor->Label)
            strncpy(strBuf, descriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        if (rdf_descriptor && rdf_descriptor->Creator)
            strncpy(strBuf, rdf_descriptor->Creator, STR_MAX);
        else if (descriptor->Maker)
            strncpy(strBuf, descriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        if (descriptor->Copyright)
            strncpy(strBuf, descriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        if (rdf_descriptor && rdf_descriptor->Title)
            strncpy(strBuf, rdf_descriptor->Title, STR_MAX);
        else if (descriptor->Name)
            strncpy(strBuf, descriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;

        strncpy(strBuf, descriptor->PortNames[rindex], STR_MAX);
    }

    void getParameterSymbol(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const Port = &rdf_descriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_LABEL(Port->Hints) && Port->Label)
            {
                strncpy(strBuf, Port->Label, STR_MAX);
                return;
            }
        }

        *strBuf = 0;
    }

    void getParameterUnit(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_Port* const Port = &rdf_descriptor->Ports[rindex];

            if (LADSPA_PORT_HAS_UNIT(Port->Hints))
            {
                switch (Port->Unit)
                {
                case LADSPA_UNIT_DB:
                    strncpy(strBuf, "dB", STR_MAX);
                    return;
                case LADSPA_UNIT_COEF:
                    strncpy(strBuf, "(coef)", STR_MAX);
                    return;
                case LADSPA_UNIT_HZ:
                    strncpy(strBuf, "Hz", STR_MAX);
                    return;
                case LADSPA_UNIT_S:
                    strncpy(strBuf, "s", STR_MAX);
                    return;
                case LADSPA_UNIT_MS:
                    strncpy(strBuf, "ms", STR_MAX);
                    return;
                case LADSPA_UNIT_MIN:
                    strncpy(strBuf, "min", STR_MAX);
                    return;
                }
            }
        }

        *strBuf = 0;
    }

    void getParameterScalePointLabel(uint32_t parameterId, uint32_t scalePointId, char* const strBuf)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        int32_t rindex = param.data[parameterId].rindex;

        if (rdf_descriptor && rindex < (int32_t)rdf_descriptor->PortCount)
        {
            const LADSPA_RDF_ScalePoint* const scalePoint = &rdf_descriptor->Ports[rindex].ScalePoints[scalePointId];

            if (scalePoint && scalePoint->Label)
            {
                strncpy(strBuf, rdf_descriptor->Ports[rindex].ScalePoints[scalePointId].Label, STR_MAX);
                return;
            }
        }

        *strBuf = 0;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(uint32_t parameterId, double value, bool sendGui, bool sendOsc, bool sendCallback)
    {
        assert(parameterId < param.count);
        param_buffers[parameterId] = fixParameterValue(value, param.ranges[parameterId]);
        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("LadspaPlugin::reload() - start");

        // Safely disable plugin for reload
        const CarlaPluginScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t ains, aouts, params, j;
        ains = aouts = params = 0;

        const double sampleRate = x_engine->getSampleRate();
        const unsigned long PortCount = descriptor->PortCount;

        for (unsigned long i=0; i < PortCount; i++)
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

        if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK && (ains == 1 || aouts == 1) && ! h2)
            h2 = descriptor->instantiate(descriptor, sampleRate);

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

        const int portNameSize = CarlaEngine::maxPortNameSize() - 1;
        char portName[portNameSize];
        bool needsCin  = false;
        bool needsCout = false;

        for (unsigned long i=0; i<PortCount; i++)
        {
            const LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint PortHint  = descriptor->PortRangeHints[i];
            const bool HasPortRDF = (rdf_descriptor && i < rdf_descriptor->PortCount);

            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carlaOptions.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
                {
                    strcpy(portName, m_name);
                    strcat(portName, ":");
                    strncat(portName, descriptor->PortNames[i], portNameSize/2);
                }
                else
#endif
                    strncpy(portName, descriptor->PortNames[i], portNameSize);

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    ain.rindexes[j] = i;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aout.rindexes[j] = i;
                    needsCin = true;
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
                param.data[j].midiChannel = 0;
                param.data[j].midiCC = -1;

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
                if (HasPortRDF && LADSPA_PORT_HAS_DEFAULT(rdf_descriptor->Ports[i].Hints))
                    def = rdf_descriptor->Ports[i].Default;

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

                if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
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
                    needsCin = true;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    if (strcmp(descriptor->PortNames[i], "latency") == 0 || strcmp(descriptor->PortNames[i], "_latency") == 0)
                    {
                        min = 0.0;
                        max = sampleRate;
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
                        needsCout = true;
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

                // check for scalepoints, require at least 2 to make it useful
                if (HasPortRDF && rdf_descriptor->Ports[i].ScalePointCount > 1)
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = step_small;
                param.ranges[j].stepLarge = step_large;

                // Start parameters in their default values
                param_buffers[j] = def;

                descriptor->connect_port(handle, i, &param_buffers[j]);
                if (h2) descriptor->connect_port(h2, i, &param_buffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                descriptor->connect_port(handle, i, nullptr);
                if (h2) descriptor->connect_port(h2, i, nullptr);
            }
        }

        if (needsCin)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-in");
            }
            else
#endif
                strcpy(portName, "control-in");

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCout)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-out");
            }
            else
#endif
                strcpy(portName, "control-out");

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
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

        if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK || (aouts >= 2 && aouts%2 == 0))
            m_hints |= PLUGIN_CAN_BALANCE;

        x_client->activate();

        qDebug("LadspaPlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** inBuffer, float** outBuffer, uint32_t frames, uint32_t framesOffset)
    {
        uint32_t i, k;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            uint32_t count = h2 ? 2 : ain.count;

            if (count == 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs(inBuffer[0][k]);
                }
            }
            else if (count > 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs(inBuffer[0][k]);

                    if (abs(inBuffer[1][k]) > ains_peak_tmp[1])
                        ains_peak_tmp[1] = abs(inBuffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.portCin && m_active && m_activeBefore)
        {
            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            for (i=0; i < nEvents; i++)
            {
                cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case CarlaEngineEventControlChange:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->controller) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->controller) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_BALANCE(cinEvent->controller) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cinEvent->value/0.5 - 1.0;

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

                            setBalanceLeft(left, false, false);
                            setBalanceRight(right, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midiChannel != cinEvent->channel)
                            continue;
                        if (param.data[k].midiCC != cinEvent->controller)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cinEvent->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cinEvent->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeEvent(PluginPostEventParameterChange, k, value);
                        }
                    }

                    break;
                }

                case CarlaEngineEventAllSoundOff:
                    if (cinEvent->channel == cin_channel)
                    {
                        if (descriptor->deactivate)
                        {
                            descriptor->deactivate(handle);
                            if (h2) descriptor->deactivate(h2);
                        }

                        if (descriptor->activate)
                        {
                            descriptor->activate(handle);
                            if (h2) descriptor->activate(h2);
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO: ladspa special params
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (descriptor->activate)
                {
                    descriptor->activate(handle);
                    if (h2) descriptor->activate(h2);
                }
            }

            for (i=0; i < ain.count; i++)
            {
                descriptor->connect_port(handle, ain.rindexes[i], inBuffer[i]);
                if (h2 && i == 0) descriptor->connect_port(h2, ain.rindexes[i], inBuffer[1]);
            }

            for (i=0; i < aout.count; i++)
            {
                descriptor->connect_port(handle, aout.rindexes[i], outBuffer[i]);
                if (h2 && i == 0) descriptor->connect_port(h2, aout.rindexes[i], outBuffer[1]);
            }

            descriptor->run(handle, frames);
            if (h2) descriptor->run(h2, frames);
        }
        else
        {
            if (m_activeBefore)
            {
                if (descriptor->deactivate)
                {
                    descriptor->deactivate(handle);
                    if (h2) descriptor->deactivate(h2);
                }
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
            float oldBufLeft[do_balance ? frames : 0];

            uint32_t count = h2 ? 2 : aout.count;

            for (i=0; i < count; i++)
            {
                // Dry/Wet and Volume
                if (do_drywet || do_volume)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (do_drywet)
                        {
                            if (aout.count == 1 && ! h2)
                                outBuffer[i][k] = (outBuffer[i][k]*x_drywet)+(inBuffer[0][k]*(1.0-x_drywet));
                            else
                                outBuffer[i][k] = (outBuffer[i][k]*x_drywet)+(inBuffer[i][k]*(1.0-x_drywet));
                        }

                        if (do_volume)
                            outBuffer[i][k] *= x_vol;
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; i < 2 && k < frames; k++)
                {
                    if (abs(outBuffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs(outBuffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(outBuffer[i], 0.0f, sizeof(float)*frames);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.portCout && m_active)
        {
            double value;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    fixParameterValue(param_buffers[k], param.ranges[k]);

                    if (param.data[k].midiCC > 0)
                    {
                        value = (param_buffers[k] - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.portCout->writeEvent(CarlaEngineEventControlChange, framesOffset, param.data[k].midiChannel, param.data[k].midiCC, value);
                    }
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, ains_peak_tmp[0]);
        x_engine->setInputPeak(m_id, 1, ains_peak_tmp[1]);
        x_engine->setOutputPeak(m_id, 0, aouts_peak_tmp[0]);
        x_engine->setOutputPeak(m_id, 1, aouts_peak_tmp[1]);

        m_activeBefore = m_active;
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("LadspaPlugin::deleteBuffers() - start");

        if (param.count > 0)
            delete[] param_buffers;

        param_buffers = nullptr;

        qDebug("LadspaPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const LADSPA_RDF_Descriptor* const rdf_descriptor_)
    {
        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(filename))
        {
            setLastError(libError(filename));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function)libSymbol("ladspa_descriptor");

        if (! descfn)
        {
            setLastError("Could not find the LASDPA Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches label

        unsigned long i = 0;
        while ((descriptor = descfn(i++)))
        {
            if (strcmp(descriptor->Label, label) == 0)
                break;
        }

        if (! descriptor)
        {
            setLastError("Could not find the requested plugin Label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        handle = descriptor->instantiate(descriptor, x_engine->getSampleRate());

        if (! handle)
        {
            setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);

        if (is_ladspa_rdf_descriptor_valid(rdf_descriptor_, descriptor))
            rdf_descriptor = ladspa_rdf_dup(rdf_descriptor_);

        if (name)
            m_name = x_engine->getUniqueName(name);
        else if (rdf_descriptor && rdf_descriptor->Title)
            m_name = x_engine->getUniqueName(rdf_descriptor->Title);
        else
            m_name = x_engine->getUniqueName(descriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            setLastError("Failed to register plugin client");
            return false;
        }

        return true;
    }

private:
    LADSPA_Handle handle, h2;
    const LADSPA_Descriptor* descriptor;
    const LADSPA_RDF_Descriptor* rdf_descriptor;

    float* param_buffers;
};

CarlaPlugin* CarlaPlugin::newLADSPA(const initializer& init, const void* const extra)
{
    qDebug("CarlaPlugin::newLADSPA(%p, %s, %s, %s, %p)", init.engine, init.filename, init.name, init.label, extra);

    short id = init.engine->getNewPluginIndex();

    if (id < 0)
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    LadspaPlugin* const plugin = new LadspaPlugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label, (const LADSPA_RDF_Descriptor*)extra))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t ins  = plugin->audioInCount();
        uint32_t outs = plugin->audioOutCount();

        if (ins > 2 || outs > 2 || (ins != outs && ins != 0 && outs != 0))
        {
            setLastError("Carla's Rack Mode can only work with Mono or Stereo plugins, sorry!");
            qWarning("data: %i %i | %i %i %i", ins > 2, outs > 2, ins != outs, ins != 0, outs != 0);
            delete plugin;
            return nullptr;
        }
    }
#endif

    plugin->registerToOsc();

    return plugin;
}

/**@}*/

CARLA_BACKEND_END_NAMESPACE

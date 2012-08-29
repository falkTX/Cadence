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
#include "carla_dssi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendDssiPlugin Carla Backend DSSI Plugin
 *
 * The Carla Backend DSSI Plugin.\n
 * http://dssi.sourceforge.net/
 * @{
 */

class DssiPlugin : public CarlaPlugin
{
public:
    DssiPlugin(CarlaEngine* const engine, unsigned short id) : CarlaPlugin(engine, id)
    {
        qDebug("DssiPlugin::DssiPlugin()");

        m_type = PLUGIN_DSSI;

        handle = h2 = nullptr;
        descriptor  = nullptr;
        ldescriptor = nullptr;

        param_buffers = nullptr;

        memset(midiEvents, 0, sizeof(snd_seq_event_t)*MAX_MIDI_EVENTS);
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

        if (ldescriptor)
        {
            if (ldescriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    ldescriptor->deactivate(handle);
                if (h2)
                    ldescriptor->deactivate(h2);
            }

            if (ldescriptor->cleanup)
            {
                if (handle)
                    ldescriptor->cleanup(handle);
                if (h2)
                    ldescriptor->cleanup(h2);
            }
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        if (m_hints & PLUGIN_IS_SYNTH)
            return PLUGIN_CATEGORY_SYNTH;
        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        return ldescriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        Q_ASSERT(dataPtr);
        unsigned long dataSize = 0;

        if (descriptor->get_custom_data(handle, dataPtr, &dataSize))
            return dataSize;

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(uint32_t parameterId)
    {
        Q_ASSERT(parameterId < param.count);
        return param_buffers[parameterId];
    }

    void getLabel(char* const strBuf)
    {
        if (ldescriptor->Label)
            strncpy(strBuf, ldescriptor->Label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        if (ldescriptor->Maker)
            strncpy(strBuf, ldescriptor->Maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        if (ldescriptor->Copyright)
            strncpy(strBuf, ldescriptor->Copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        if (ldescriptor->Name)
            strncpy(strBuf, ldescriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;

        strncpy(strBuf, ldescriptor->PortNames[rindex], STR_MAX);
    }

    void getGuiInfo(GuiType* type, bool* resizable)
    {
        *type = (m_hints & PLUGIN_HAS_GUI) ? GUI_EXTERNAL_OSC : GUI_NONE;
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(uint32_t parameterId, double value, bool sendGui, bool sendOsc, bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count);
        param_buffers[parameterId] = fixParameterValue(value, param.ranges[parameterId]);

#ifndef BUILD_BRIDGE
        if (sendGui && osc.data.target)
            osc_send_control(&osc.data, param.data[parameterId].rindex, value);
#endif

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(CustomDataType type, const char* const key, const char* const value, bool sendGui)
    {
        Q_ASSERT(key);
        Q_ASSERT(value);

        if (! key)
            return qCritical("DssiPlugin::setCustomData(%s, \"%s\", \"%s\", %s) - key is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (! value)
            return qCritical("DssiPlugin::setCustomData(%s, \"%s\", \"%s\", %s) - value is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        descriptor->configure(handle, key, value);

#ifndef BUILD_BRIDGE
        if (sendGui && osc.data.target)
            osc_send_configure(&osc.data, key, value);
#endif

        if (strcmp(key, "reloadprograms") == 0 || strcmp(key, "load") == 0 || strncmp(key, "patches", 7) == 0)
        {
            const ScopedDisabler m(this);
            reloadPrograms(false);
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData)
    {
        Q_ASSERT(stringData);

        static QByteArray chunk;
        chunk = QByteArray::fromBase64(stringData);

        if (x_engine->isOffline())
        {
            const CarlaEngine::ScopedLocker m(x_engine);
            descriptor->set_custom_data(handle, chunk.data(), chunk.size());
        }
        else
        {
            const ScopedDisabler m(this);
            descriptor->set_custom_data(handle, chunk.data(), chunk.size());
        }
    }

    void setMidiProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        Q_ASSERT(index < (int32_t)midiprog.count);

        if (index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }
            else
            {
                const ScopedDisabler m(this, block);
                descriptor->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }

#ifndef BUILD_BRIDGE
            if (sendGui && osc.data.target)
                osc_send_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program);
#endif
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

#ifndef BUILD_BRIDGE
    void showGui(bool yesNo)
    {
        Q_ASSERT(osc.thread);

        if (! osc.thread)
        {
            qCritical("DssiPlugin::showGui(%s) - attempt to show gui, but it does not exist!", bool2str(yesNo));
            return;
        }

        if (yesNo)
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
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, mIns, params, j;
        aIns = aOuts = mIns = params = 0;

        const double sampleRate = x_engine->getSampleRate();
        const unsigned long PortCount = ldescriptor->PortCount;

        for (unsigned long i=0; i<PortCount; i++)
        {
            const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
                if (LADSPA_IS_PORT_INPUT(PortType))
                    aIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                    aOuts += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortType))
                params += 1;
        }

        if (carlaOptions.forceStereo && (aIns == 1 || aOuts == 1) && ! h2)
            h2 = ldescriptor->instantiate(ldescriptor, sampleRate);

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            mIns = 1;

        if (aIns > 0)
        {
            aIn.ports    = new CarlaEngineAudioPort*[aIns];
            aIn.rindexes = new uint32_t[aIns];
        }

        if (aOuts > 0)
        {
            aOut.ports    = new CarlaEngineAudioPort*[aOuts];
            aOut.rindexes = new uint32_t[aOuts];
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
            const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[i];
            const LADSPA_PortRangeHint PortHint  = ldescriptor->PortRangeHints[i];

            if (LADSPA_IS_PORT_AUDIO(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
                {
                    strcpy(portName, m_name);
                    strcat(portName, ":");
                    strncat(portName, ldescriptor->PortNames[i], portNameSize/2);
                }
                else
#endif
                    strncpy(portName, ldescriptor->PortNames[i], portNameSize);

                if (LADSPA_IS_PORT_INPUT(PortType))
                {
                    j = aIn.count++;
                    aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    aIn.rindexes[j] = i;
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    j = aOut.count++;
                    aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aOut.rindexes[j] = i;
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

                    // MIDI CC value
                    if (descriptor->get_midi_controller_for_port)
                    {
                        int controller = descriptor->get_midi_controller_for_port(handle, i);
                        if (DSSI_CONTROLLER_IS_SET(controller) && DSSI_IS_CC(controller))
                        {
                            int16_t cc = DSSI_CC_NUMBER(controller);
                            if (! MIDI_IS_CONTROL_BANK_SELECT(cc))
                                param.data[j].midiCC = cc;
                        }
                    }
                }
                else if (LADSPA_IS_PORT_OUTPUT(PortType))
                {
                    if (strcmp(ldescriptor->PortNames[i], "latency") == 0 || strcmp(ldescriptor->PortNames[i], "_latency") == 0)
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

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = step_small;
                param.ranges[j].stepLarge = step_large;

                // Start parameters in their default values
                param_buffers[j] = def;

                ldescriptor->connect_port(handle, i, &param_buffers[j]);
                if (h2) ldescriptor->connect_port(h2, i, &param_buffers[j]);
            }
            else
            {
                // Not Audio or Control
                qCritical("ERROR - Got a broken Port (neither Audio or Control)");
                ldescriptor->connect_port(handle, i, nullptr);
                if (h2) ldescriptor->connect_port(h2, i, nullptr);
            }
        }

        if (needsCin)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
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
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-out");
            }
            else
#endif
                strcpy(portName, "control-out");

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        if (mIns > 0)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":midi-in");
            }
            else
#endif
                strcpy(portName, "midi-in");

            midi.portMin = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (midi.portMin && aOut.count > 0)
            m_hints |= PLUGIN_IS_SYNTH;

#ifndef BUILD_BRIDGE
        if (carlaOptions.useDssiVstChunks && QString(m_filename).endsWith("dssi-vst.so", Qt::CaseInsensitive))
        {
            if (descriptor->get_custom_data && descriptor->set_custom_data)
                m_hints |= PLUGIN_USES_CHUNKS;
        }
#endif

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if ((aOuts >= 2 && aOuts%2 == 0) || h2)
            m_hints |= PLUGIN_CAN_BALANCE;

        reloadPrograms(true);

        x_client->activate();

        qDebug("DssiPlugin::reload() - end");
    }

    void reloadPrograms(bool init)
    {
        qDebug("DssiPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = midiprog.count;

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
            const DSSI_Program_Descriptor* const pdesc = descriptor->get_program(handle, i);
            Q_ASSERT(pdesc);

            midiprog.data[i].bank    = pdesc->Bank;
            midiprog.data[i].program = pdesc->Program;
            midiprog.data[i].name    = strdup(pdesc->Name);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        x_engine->osc_send_set_midi_program_count(m_id, midiprog.count);

        for (i=0; i < midiprog.count; i++)
            x_engine->osc_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_UPDATE, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool programChanged = false;

            if (midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                midiprog.current = oldCount;
                programChanged  = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                programChanged  = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                programChanged  = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                programChanged  = true;
            }

            if (programChanged)
                setMidiProgram(midiprog.current, true, true, true, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** inBuffer, float** outBuffer, uint32_t frames, uint32_t framesOffset)
    {
        uint32_t i, k;
        unsigned long midiEventCount = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (aIn.count > 0)
        {
            uint32_t count = h2 ? 2 : aIn.count;

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
            bool allNotesOffSent = false;

            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            uint32_t nextBankId = 0;
            if (midiprog.current >= 0 && midiprog.count > 0)
                nextBankId = midiprog.data[midiprog.current].bank;

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
                case CarlaEngineEventNull:
                    break;

                case CarlaEngineEventControlChange:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->controller) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, 0, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->controller) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, 0, value);
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
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, 0, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, 0, right);
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
                            postponeEvent(PluginPostEventParameterChange, k, 0, value);
                        }
                    }

                    break;
                }

                case CarlaEngineEventMidiBankChange:
                    if (cinEvent->channel == m_ctrlInChannel)
                        nextBankId = rint(cinEvent->value);
                    break;

                case CarlaEngineEventMidiProgramChange:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        uint32_t nextProgramId = rint(cinEvent->value);

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == nextBankId && midiprog.data[k].program == nextProgramId)
                            {
                                setMidiProgram(k, false, false, false, false);
                                postponeEvent(PluginPostEventMidiProgramChange, k, 0, 0.0);
                                break;
                            }
                        }
                    }
                    break;

                case CarlaEngineEventAllSoundOff:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        if (ldescriptor->deactivate)
                        {
                            ldescriptor->deactivate(handle);
                            if (h2) ldescriptor->deactivate(h2);
                        }

                        if (ldescriptor->activate)
                        {
                            ldescriptor->activate(handle);
                            if (h2) ldescriptor->activate(h2);
                        }

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineEventAllNotesOff:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        allNotesOffSent = true;
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (midi.portMin && m_ctrlInChannel >= 0 && m_ctrlInChannel < 16 && m_active && m_activeBefore)
        {
            engineMidiLock();

            for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
            {
                if (extMidiNotes[i].channel < 0)
                    break;

                snd_seq_event_t* const midiEvent = &midiEvents[midiEventCount];
                memset(midiEvent, 0, sizeof(snd_seq_event_t));

                midiEvent->type = extMidiNotes[i].velo ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
                midiEvent->data.note.channel  = m_ctrlInChannel;
                midiEvent->data.note.note     = extMidiNotes[i].note;
                midiEvent->data.note.velocity = extMidiNotes[i].velo;

                extMidiNotes[i].channel = -1;
                midiEventCount += 1;
            }

            engineMidiUnlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (System)

        if (midi.portMin && m_active && m_activeBefore)
        {
            const CarlaEngineMidiEvent* minEvent;
            uint32_t time, nEvents = midi.portMin->getEventCount();

            for (i=0; i < nEvents && midiEventCount < MAX_MIDI_EVENTS; i++)
            {
                minEvent = midi.portMin->getEvent(i);

                if (! minEvent)
                    continue;

                time = minEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                uint8_t status  = minEvent->data[0];
                uint8_t channel = status & 0x0F;

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                    status -= 0x10;

                snd_seq_event_t* const midiEvent = &midiEvents[midiEventCount];
                memset(midiEvent, 0, sizeof(snd_seq_event_t));

                midiEvent->time.tick = time;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = minEvent->data[1];

                    midiEvent->type = SND_SEQ_EVENT_NOTEOFF;
                    midiEvent->data.note.channel = channel;
                    midiEvent->data.note.note    = note;

                    if (channel == m_ctrlInChannel)
                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = minEvent->data[1];
                    uint8_t velo = minEvent->data[2];

                    midiEvent->type = SND_SEQ_EVENT_NOTEON;
                    midiEvent->data.note.channel  = channel;
                    midiEvent->data.note.note     = note;
                    midiEvent->data.note.velocity = velo;

                    if (channel == m_ctrlInChannel)
                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = minEvent->data[1];
                    uint8_t pressure = minEvent->data[2];

                    midiEvent->type = SND_SEQ_EVENT_KEYPRESS;
                    midiEvent->data.note.channel  = channel;
                    midiEvent->data.note.note     = note;
                    midiEvent->data.note.velocity = pressure;
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    uint8_t pressure = minEvent->data[1];

                    midiEvent->type = SND_SEQ_EVENT_CHANPRESS;
                    midiEvent->data.control.channel = channel;
                    midiEvent->data.control.value   = pressure;
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    uint8_t lsb = minEvent->data[1];
                    uint8_t msb = minEvent->data[2];

                    midiEvent->type = SND_SEQ_EVENT_PITCHBEND;
                    midiEvent->data.control.channel = channel;
                    midiEvent->data.control.value   = ((msb << 7) | lsb) - 8192;
                }
                else
                    continue;

                midiEventCount += 1;
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
            if (! m_activeBefore)
            {
                if (midi.portMin && m_ctrlInChannel >= 0 && m_ctrlInChannel < 16)
                {
                    memset(&midiEvents[0], 0, sizeof(snd_seq_event_t));
                    midiEvents[0].type      = SND_SEQ_EVENT_CONTROLLER;
                    midiEvents[0].data.control.channel = m_ctrlInChannel;
                    midiEvents[0].data.control.param   = MIDI_CONTROL_ALL_SOUND_OFF;

                    memset(&midiEvents[1], 0, sizeof(snd_seq_event_t));
                    midiEvents[1].type      = SND_SEQ_EVENT_CONTROLLER;
                    midiEvents[1].data.control.channel = m_ctrlInChannel;
                    midiEvents[1].data.control.param   = MIDI_CONTROL_ALL_NOTES_OFF;

                    midiEventCount = 2;
                }

                if (ldescriptor->activate)
                {
                    ldescriptor->activate(handle);
                    if (h2) ldescriptor->activate(h2);
                }
            }

            for (i=0; i < aIn.count; i++)
            {
                ldescriptor->connect_port(handle, aIn.rindexes[i], inBuffer[i]);
                if (h2 && i == 0) ldescriptor->connect_port(h2, aIn.rindexes[i], inBuffer[1]);
            }

            for (i=0; i < aOut.count; i++)
            {
                ldescriptor->connect_port(handle, aOut.rindexes[i], outBuffer[i]);
                if (h2 && i == 0) ldescriptor->connect_port(h2, aOut.rindexes[i], outBuffer[1]);
            }

            if (descriptor->run_synth)
            {
                descriptor->run_synth(handle, frames, midiEvents, midiEventCount);
                if (h2) descriptor->run_synth(handle, frames, midiEvents, midiEventCount);
            }
            else if (descriptor->run_multiple_synths)
            {
                LADSPA_Handle handlePtr[2] = { handle, h2 };
                snd_seq_event_t* midiEventsPtr[2] = { midiEvents, midiEvents };
                unsigned long midiEventCountPtr[2] = { midiEventCount, midiEventCount };
                descriptor->run_multiple_synths(h2 ? 2 : 1, handlePtr, frames, midiEventsPtr, midiEventCountPtr);
            }
            else
            {
                ldescriptor->run(handle, frames);
                if (h2) ldescriptor->run(h2, frames);
            }
        }
        else
        {
            if (m_activeBefore)
            {
                if (ldescriptor->deactivate)
                {
                    ldescriptor->deactivate(handle);
                    if (h2) ldescriptor->deactivate(h2);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_dryWet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_volume != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_balanceLeft != -1.0 || x_balanceRight != 1.0);

            double bal_rangeL, bal_rangeR;
            float oldBufLeft[do_balance ? frames : 0];

            uint32_t count = h2 ? 2 : aOut.count;

            for (i=0; i < count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (aOut.count == 1 && ! h2)
                            outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(inBuffer[0][k]*(1.0-x_dryWet));
                        else
                            outBuffer[i][k] = (outBuffer[i][k]*x_dryWet)+(inBuffer[i][k]*(1.0-x_dryWet));
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_balanceLeft+1.0)/2;
                    bal_rangeR = (x_balanceRight+1.0)/2;

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

                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_volume;
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
            for (i=0; i < aOut.count; i++)
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
    // Post-poned events

    void uiParameterChange(uint32_t index, double value)
    {
        Q_ASSERT(index < param.count);
        if (index >= param.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_control(&osc.data, param.data[index].rindex, value);
    }

    void uiMidiProgramChange(uint32_t index)
    {
        Q_ASSERT(index < midiprog.count);
        if (index >= midiprog.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program);
    }

    void uiNoteOn(uint8_t channel, uint8_t note, uint8_t velo)
    {
        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;
        osc_send_midi(&osc.data, midiData);
    }

    void uiNoteOff(uint8_t channel, uint8_t note)
    {
        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;
        osc_send_midi(&osc.data, midiData);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("DssiPlugin::deleteBuffers() - start");

        if (param.count > 0)
            delete[] param_buffers;

        param_buffers = nullptr;

        qDebug("DssiPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label, const char* const guiFilename)
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

        DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)libSymbol("dssi_descriptor");

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
            qDebug("init 003 -> %li", i);
            ldescriptor = descriptor->LADSPA_Plugin;
            if (strcmp(ldescriptor->Label, label) == 0)
                break;
        }

        if (! descriptor)
        {
            setLastError("Could not find the requested plugin Label in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // initialize plugin

        handle = ldescriptor->instantiate(ldescriptor, x_engine->getSampleRate());

        if (! handle)
        {
            setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);

        if (name)
            m_name = x_engine->getUniqueName(name);
        else
            m_name = x_engine->getUniqueName(ldescriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            setLastError("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

#ifndef BUILD_BRIDGE
        if (guiFilename)
        {
            osc.thread = new CarlaPluginThread(x_engine, this, CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI);
            osc.thread->setOscData(guiFilename, ldescriptor->Label);

            m_hints |= PLUGIN_HAS_GUI;
        }
#else
        Q_UNUSED(guiFilename);
#endif

        return true;
    }

private:
    LADSPA_Handle handle, h2;
    const LADSPA_Descriptor* ldescriptor;
    const DSSI_Descriptor* descriptor;
    snd_seq_event_t midiEvents[MAX_MIDI_EVENTS];

    float* param_buffers;
};

CarlaPlugin* CarlaPlugin::newDSSI(const initializer& init, const void* const extra)
{
    qDebug("CarlaPlugin::newDSSI(%p, \"%s\", \"%s\", \"%s\", %p)", init.engine, init.filename, init.name, init.label, extra);

    short id = init.engine->getNewPluginId();

    if (id < 0)
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    DssiPlugin* const plugin = new DssiPlugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label, (const char*)extra))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const uint32_t ins  = plugin->audioInCount();
        const uint32_t outs = plugin->audioOutCount();

        if (ins > 2 || outs > 2 || (ins != outs && ins != 0 && outs != 0))
        {
            setLastError("Carla's rack mode can only work with Mono or Stereo DSSI plugins, sorry!");
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

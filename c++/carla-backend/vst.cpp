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
#include "carla_vst.h"

#ifndef __WINE__
#include <QtGui/QDialog>
#ifdef Q_WS_X11
#include <QtGui/QX11Info>
#endif
#endif

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendVstPlugin Carla Backend VST Plugin
 *
 * The Carla Backend VST Plugin.
 * @{
 */

/*!
 * @defgroup PluginHints Plugin Hints
 * @{
 */
const unsigned int PLUGIN_CAN_PROCESS_REPLACING = 0x100; //!< VST Plugin cas use processReplacing()
const unsigned int PLUGIN_HAS_COCKOS_EXTENSIONS = 0x200; //!< VST Plugin has Cockos extensions
const unsigned int PLUGIN_USES_OLD_VSTSDK       = 0x400; //!< VST Plugin uses an old VST SDK
const unsigned int PLUGIN_WANTS_MIDI_INPUT      = 0x800; //!< VST Plugin wants MIDI input
/**@}*/

class VstPlugin : public CarlaPlugin
{
public:
    VstPlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("VstPlugin::VstPlugin()");

        m_type = PLUGIN_VST;

        effect = nullptr;
        events.numEvents = 0;
        events.reserved  = 0;

        gui.type = GUI_NONE;
        gui.visible = false;
        gui.width  = 0;
        gui.height = 0;

        isProcessing = false;

        memset(midiEvents, 0, sizeof(VstMidiEvent)*MAX_MIDI_EVENTS*2);

        for (unsigned short i=0; i < MAX_MIDI_EVENTS*2; i++)
            events.data[i] = (VstEvent*)&midiEvents[i];

        // make plugin valid
        srand(id);
        unique1 = unique2 = rand();
    }

    ~VstPlugin()
    {
        qDebug("VstPlugin::~VstPlugin()");

        // make plugin invalid
        unique2 += 1;

        if (effect)
        {
            // close UI
            if (m_hints & PLUGIN_HAS_GUI)
            {
                showGui(false);

                if (gui.type == GUI_EXTERNAL_OSC)
                {
#ifndef BUILD_BRIDGE
                    if (osc.thread)
                    {
                        // Wait a bit first, try safe quit, then force kill
                        if (osc.thread->isRunning() && ! osc.thread->wait(carlaOptions.oscUiTimeout * 100))
                        {
                            qWarning("Failed to properly stop VST OSC GUI thread");
                            osc.thread->terminate();
                        }

                        delete osc.thread;
                    }
#endif
                }
                else
                    effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
            }

            if (m_activeBefore)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            }

            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        Q_ASSERT(effect);

        intptr_t category = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        switch (category)
        {
        case kPlugCategSynth:
            return PLUGIN_CATEGORY_SYNTH;
        case kPlugCategAnalysis:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategMastering:
            return PLUGIN_CATEGORY_DYNAMICS;
        case kPlugCategRoomFx:
            return PLUGIN_CATEGORY_DELAY;
        case kPlugCategRestoration:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategGenerator:
            return PLUGIN_CATEGORY_SYNTH;
        }

        if (effect->flags & effFlagsIsSynth)
            return PLUGIN_CATEGORY_SYNTH;

        return getPluginCategoryFromName(m_name);
    }

    long uniqueId()
    {
        Q_ASSERT(effect);

        return effect->uniqueID;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        Q_ASSERT(dataPtr);
        Q_ASSERT(effect);

        if (effect)
            return effect->dispatcher(effect, effGetChunk, 0 /* bank */, 0, dataPtr, 0.0f);

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        Q_ASSERT(effect);
        Q_ASSERT(parameterId < param.count);

        if (effect)
            return effect->getParameter(effect, parameterId);

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        Q_ASSERT(effect);

        if (effect)
            effect->dispatcher(effect, effGetProductString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        Q_ASSERT(effect);

        if (effect)
            effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        Q_ASSERT(effect);

        if (effect)
            effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        Q_ASSERT(effect);

        if (effect)
            effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(effect);
        Q_ASSERT(parameterId < param.count);

        if (effect)
            effect->dispatcher(effect, effGetParamName, parameterId, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(effect);
        Q_ASSERT(parameterId < param.count);

        if (effect)
        {
            effect->dispatcher(effect, effGetParamDisplay, parameterId, 0, strBuf, 0.0f);

            if (*strBuf == 0)
                sprintf(strBuf, "%f", getParameterValue(parameterId));
        }
        else
            CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(effect);
        Q_ASSERT(parameterId < param.count);

        if (effect)
            effect->dispatcher(effect, effGetParamLabel, parameterId, 0, strBuf, 0.0f);
        else
            CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        *type = gui.type;
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count);

        effect->setParameter(effect, parameterId, fixParameterValue(value, param.ranges[parameterId]));

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setChunkData(const char* const stringData)
    {
        Q_ASSERT(m_hints & PLUGIN_USES_CHUNKS);
        Q_ASSERT(stringData);

        static QByteArray chunk;
        chunk = QByteArray::fromBase64(stringData);

        if (x_engine->isOffline())
        {
            const CarlaEngine::ScopedLocker m(x_engine);
            effect->dispatcher(effect, effSetChunk, 0 /* bank */, chunk.size(), chunk.data(), 0.0f);
        }
        else
        {
            const CarlaPlugin::ScopedDisabler m(this);
            effect->dispatcher(effect, effSetChunk, 0 /* bank */, chunk.size(), chunk.data(), 0.0f);
        }
    }

    void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        Q_ASSERT(index >= -1 && index < (int32_t)prog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)prog.count)
            return;

        if (index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
            }
            else
            {
                const ScopedDisabler m(this, block);
                effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
            }
        }

        CarlaPlugin::setProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void setGuiData(const GuiDataHandle handle)
    {
        qDebug("VstPlugin::setGuiData(%p)", handle);
        Q_ASSERT(handle);

        if (gui.type == GUI_EXTERNAL_OSC)
            return;

        int32_t value = 0;
#ifdef Q_WS_X11
        value = (int64_t)QX11Info::display();
#endif

#ifdef __WINE__
        if (effect->dispatcher(effect, effEditOpen, 0, value, handle, 0.0f) == 1)
#else
        const QDialog* const dialog = handle;
        if (effect->dispatcher(effect, effEditOpen, 0, value, (void*)dialog->winId(), 0.0f) == 1)
#endif
        {
            ERect* vstRect = nullptr;
            effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f);

            if (vstRect)
            {
                int width  = vstRect->right  - vstRect->left;
                int height = vstRect->bottom - vstRect->top;

                if (width <= 0 || height <= 0)
                {
                    qCritical("VstPlugin::setGuiData(%p) - failed to get proper window size", handle);
                    return;
                }

                gui.width  = width;
                gui.height = height;
            }
            else
                qCritical("VstPlugin::setGuiData(%p) - failed to get plugin window size", handle);
        }
        else
        {
            // failed to open UI
            m_hints &= ~PLUGIN_HAS_GUI;
            x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);

            effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
        }
    }

    void showGui(const bool yesNo)
    {
        if (gui.type == GUI_EXTERNAL_OSC)
        {
#ifndef BUILD_BRIDGE
            Q_ASSERT(osc.thread);

            if (! osc.thread)
            {
                qCritical("VstPlugin::showGui(%s) - attempt to show gui, but it does not exist!", bool2str(yesNo));
                return;
            }

            if (yesNo)
            {
                osc.thread->start();
            }
            else
            {
                if (osc.data.target)
                {
                    osc_send_hide(&osc.data);
                    osc_send_quit(&osc.data);
                    osc_clear_data(&osc.data);
                }

                if (! osc.thread->wait(500))
                    osc.thread->quit();
            }
#endif
        }
        else
        {
            if (yesNo && gui.width > 0 && gui.height > 0)
                x_engine->callback(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);
        }

        gui.visible = yesNo;
    }

    void idleGui()
    {
        //effect->dispatcher(effect, effIdle, 0, 0, nullptr, 0.0f);

        if (gui.type != GUI_EXTERNAL_OSC && gui.visible)
            effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);

        CarlaPlugin::idleGui();
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("VstPlugin::reload() - start");
        Q_ASSERT(effect);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, mIns, mOuts, params, j;
        aIns = aOuts = mIns = mOuts = params = 0;

        aIns   = effect->numInputs;
        aOuts  = effect->numOutputs;
        params = effect->numParams;

        if (vstPluginCanDo(effect, "receiveVstEvents") || vstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) > 0 || (m_hints & PLUGIN_WANTS_MIDI_INPUT))
            mIns = 1;

        if (vstPluginCanDo(effect, "sendVstEvents") || vstPluginCanDo(effect, "sendVstMidiEvent"))
            mOuts = 1;

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
        }

        const int portNameSize = CarlaEngine::maxPortNameSize() - 1;
        char portName[portNameSize];
        bool needsCtrlIn = (aOuts > 0 || params > 0);

        for (j=0; j < aIns; j++)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
                sprintf(portName, "%s:input_%02i", m_name, j+1);
            else
#endif
                sprintf(portName, "input_%02i", j+1);

            aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
            aIn.rindexes[j] = j;
        }

        for (j=0; j < aOuts; j++)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
                sprintf(portName, "%s:output_%02i", m_name, j+1);
            else
#endif
                sprintf(portName, "output_%02i", j+1);

            aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
            aOut.rindexes[j] = j;
        }

        for (j=0; j<params; j++)
        {
            param.data[j].type   = PARAMETER_INPUT;
            param.data[j].index  = j;
            param.data[j].rindex = j;
            param.data[j].hints  = 0;
            param.data[j].midiChannel = 0;
            param.data[j].midiCC = -1;

            double min, max, def, step, stepSmall, stepLarge;

            VstParameterProperties prop;
            prop.flags = 0;

            if (effect->dispatcher(effect, effGetParameterProperties, j, 0, &prop, 0))
            {
                double range[2] = { 0.0, 1.0 };

                if ((m_hints & PLUGIN_HAS_COCKOS_EXTENSIONS) > 0 && effect->dispatcher(effect, effVendorSpecific, 0xdeadbef0, j, range, 0.0) >= 0xbeef)
                {
                    min = range[0];
                    max = range[1];
                }
                else if (prop.flags & kVstParameterUsesIntegerMinMax)
                {
                    min = prop.minInteger;
                    max = prop.maxInteger;
                }
                else
                {
                    min = 0.0;
                    max = 1.0;
                }

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                if ((m_hints & PLUGIN_HAS_COCKOS_EXTENSIONS) > 0 && effect->dispatcher(effect, effVendorSpecific, kVstParameterUsesIntStep, j, nullptr, 0.0f) >= 0xbeef)
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = 10.0;
                }
                else if (prop.flags & kVstParameterIsSwitch)
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (prop.flags & kVstParameterUsesIntStep)
                {
                    step = prop.stepInteger;
                    stepSmall = prop.stepInteger;
                    stepLarge = prop.largeStepInteger;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else if (prop.flags & kVstParameterUsesFloatStep)
                {
                    step = prop.stepFloat;
                    stepSmall = prop.smallStepFloat;
                    stepLarge = prop.largeStepFloat;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    stepSmall = range/1000.0;
                    stepLarge = range/10.0;
                }

                if (prop.flags & kVstParameterCanRamp)
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;
            }
            else
            {
                min = 0.0;
                max = 1.0;
                step = 0.001;
                stepSmall = 0.0001;
                stepLarge = 0.1;
            }

            // no such thing as VST default parameters
            def = effect->getParameter(effect, j);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            param.ranges[j].min = min;
            param.ranges[j].max = max;
            param.ranges[j].def = def;
            param.ranges[j].step = step;
            param.ranges[j].stepSmall = stepSmall;
            param.ranges[j].stepLarge = stepLarge;

            param.data[j].hints |= PARAMETER_IS_ENABLED;
#ifndef BUILD_BRIDGE
            param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;
#endif

            if ((m_hints & PLUGIN_USES_OLD_VSTSDK) > 0 || effect->dispatcher(effect, effCanBeAutomated, j, 0, nullptr, 0.0f) == 1)
                param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
        }

        if (needsCtrlIn)
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

        if (mIns == 1)
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

        if (mOuts == 1)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":midi-out");
            }
            else
#endif
                strcpy(portName, "midi-out");

            midi.portMout = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        intptr_t vstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        if (vstCategory == kPlugCategSynth || vstCategory == kPlugCategGenerator)
            m_hints |= PLUGIN_IS_SYNTH;

        if (effect->flags & effFlagsProgramChunks)
            m_hints |= PLUGIN_USES_CHUNKS;

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        reloadPrograms(true);

        x_client->activate();

        qDebug("VstPlugin::reload() - end");
    }

    void reloadPrograms(const bool init)
    {
        qDebug("VstPlugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = prog.count;

        // Delete old programs
        if (prog.count > 0)
        {
            for (i=0; i < prog.count; i++)
            {
                if (prog.names[i])
                    free((void*)prog.names[i]);
            }

            delete[] prog.names;
        }

        prog.count = 0;
        prog.names = nullptr;

        // Query new programs
        prog.count = effect->numPrograms;

        if (prog.count > 0)
            prog.names = new const char* [prog.count];

        // Update names
        for (i=0; i < prog.count; i++)
        {
            char strBuf[STR_MAX] = { 0 };
            if (effect->dispatcher(effect, effGetProgramNameIndexed, i, 0, strBuf, 0.0f) != 1)
            {
                // program will be [re-]changed later
                effect->dispatcher(effect, effSetProgram, 0, i, nullptr, 0.0f);
                effect->dispatcher(effect, effGetProgramName, 0, 0, strBuf, 0.0f);
            }
            prog.names[i] = strdup(strBuf);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        x_engine->osc_send_control_set_program_count(m_id, prog.count);

        for (i=0; i < prog.count; i++)
            x_engine->osc_send_control_set_program_name(m_id, i, prog.names[i]);
#endif

        if (init)
        {
            if (prog.count > 0)
                setProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool programChanged = false;

            if (prog.count == oldCount+1)
            {
                // one program added, probably created by user
                prog.current   = oldCount;
                programChanged = true;
            }
            else if (prog.current >= (int32_t)prog.count)
            {
                // current program > count
                prog.current   = 0;
                programChanged = true;
            }
            else if (prog.current < 0 && prog.count > 0)
            {
                // programs exist now, but not before
                prog.current   = 0;
                programChanged = true;
            }
            else if (prog.current >= 0 && prog.count == 0)
            {
                // programs existed before, but not anymore
                prog.current   = -1;
                programChanged = true;
            }

            if (programChanged)
            {
                setProgram(prog.current, true, true, true, true);
            }
            else
            {
                // Program was changed during update, re-set it
                if (prog.current >= 0)
                    effect->dispatcher(effect, effSetProgram, 0, prog.current, nullptr, 0.0f);
            }
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

        double aInsPeak[2]  = { 0.0 };
        double aOutsPeak[2] = { 0.0 };

        // reset MIDI
        events.numEvents = 0;
        midiEvents[0].type = 0;

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (aIn.count > 0)
        {
            if (aIn.count == 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = abs(inBuffer[0][k]);
                }
            }
            else if (aIn.count > 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > aInsPeak[0])
                        aInsPeak[0] = abs(inBuffer[0][k]);

                    if (abs(inBuffer[1][k]) > aInsPeak[1])
                        aInsPeak[1] = abs(inBuffer[1][k]);
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

                            if (value < 0.0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0.0)
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
                    break;

                case CarlaEngineEventMidiProgramChange:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        uint32_t progId = rint(cinEvent->value);

                        if (progId < prog.count)
                        {
                            setProgram(progId, false, false, false, false);
                            postponeEvent(PluginPostEventProgramChange, progId, 0, 0.0);
                        }
                    }
                    break;

                case CarlaEngineEventAllSoundOff:
                    if (cinEvent->channel == m_ctrlInChannel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                        effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

                        effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                        effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

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
        // MIDI Input

        if (midi.portMin && m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            if (m_ctrlInChannel >= 0 && m_ctrlInChannel < 16)
            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    VstMidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->midiData[0] = m_ctrlInChannel + extMidiNotes[i].velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midiEvent->midiData[1] = extMidiNotes[i].note;
                    midiEvent->midiData[2] = extMidiNotes[i].velo;

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

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

                    VstMidiEvent* const midiEvent = &midiEvents[midiEventCount];
                    memset(midiEvent, 0, sizeof(VstMidiEvent));

                    midiEvent->type = kVstMidiType;
                    midiEvent->byteSize = sizeof(VstMidiEvent);
                    midiEvent->deltaFrames = minEvent->time;

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                    {
                        uint8_t note = minEvent->data[1];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;

                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        uint8_t note = minEvent->data[1];
                        uint8_t velo = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;
                        midiEvent->midiData[2] = velo;

                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        uint8_t note     = minEvent->data[1];
                        uint8_t pressure = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = note;
                        midiEvent->midiData[2] = pressure;
                    }
                    else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                    {
                        uint8_t pressure = minEvent->data[1];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = pressure;
                    }
                    else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        uint8_t lsb = minEvent->data[1];
                        uint8_t msb = minEvent->data[2];

                        midiEvent->midiData[0] = status;
                        midiEvent->midiData[1] = lsb;
                        midiEvent->midiData[2] = msb;
                    }
                    else
                        continue;

                    midiEventCount += 1;
                }
            } // End of MIDI Input (System)

        } // End of MIDI Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (midi.portMin && m_ctrlInChannel >= 0 && m_ctrlInChannel < 16)
                {
                    memset(&midiEvents[0], 0, sizeof(VstMidiEvent));
                    midiEvents[0].type = kVstMidiType;
                    midiEvents[0].byteSize = sizeof(VstMidiEvent);
                    midiEvents[0].midiData[0] = MIDI_STATUS_CONTROL_CHANGE + m_ctrlInChannel;
                    midiEvents[0].midiData[1] = MIDI_CONTROL_ALL_SOUND_OFF;

                    memset(&midiEvents[1], 0, sizeof(VstMidiEvent));
                    midiEvents[1].type = kVstMidiType;
                    midiEvents[1].byteSize = sizeof(VstMidiEvent);
                    midiEvents[1].midiData[0] = MIDI_STATUS_CONTROL_CHANGE + m_ctrlInChannel;
                    midiEvents[1].midiData[1] = MIDI_CONTROL_ALL_NOTES_OFF;

                    midiEventCount = 2;
                }

                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
            }

            if (midiEventCount > 0)
            {
                events.numEvents = midiEventCount;
                events.reserved  = 0;
                effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
            }

            // FIXME - make this a global option
            // don't process if not needed
            //if ((effect->flags & effFlagsNoSoundInStop) > 0 && aInsPeak[0] == 0.0 && aInsPeak[1] == 0.0 && midiEventCount == 0 && ! midi.port_mout)
            //{
            if (m_hints & PLUGIN_CAN_PROCESS_REPLACING)
            {
                isProcessing = true;
                effect->processReplacing(effect, inBuffer, outBuffer, frames);
                isProcessing = false;
            }
            else
            {
                for (i=0; i < aOut.count; i++)
                    memset(outBuffer[i], 0, sizeof(float)*frames);

#if ! VST_FORCE_DEPRECATED
                isProcessing = true;
                effect->process(effect, inBuffer, outBuffer, frames);
                isProcessing = false;
#endif
            }
            //}
        }
        else
        {
            if (m_activeBefore)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
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

            for (i=0; i < aOut.count; i++)
            {
                // Dry/Wet
                if (do_drywet)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (aOut.count == 1)
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
                    if (abs(outBuffer[i][k]) > aOutsPeak[i])
                        aOutsPeak[i] = abs(outBuffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aOut.count; i++)
                memset(outBuffer[i], 0.0f, sizeof(float)*frames);

            aOutsPeak[0] = 0.0;
            aOutsPeak[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (midi.portMout && m_active)
        {
            uint8_t data[4] = { 0 };

            for (int32_t i = midiEventCount; i < events.numEvents; i++)
            {
                data[0] = midiEvents[i].midiData[0];
                data[1] = midiEvents[i].midiData[1];
                data[2] = midiEvents[i].midiData[2];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(data[0]) && data[2] == 0)
                    data[0] -= 0x10;

                midi.portMout->writeEvent(midiEvents[i].deltaFrames, data, 3);
            }
        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, aInsPeak[0]);
        x_engine->setInputPeak(m_id, 1, aInsPeak[1]);
        x_engine->setOutputPeak(m_id, 0, aOutsPeak[0]);
        x_engine->setOutputPeak(m_id, 1, aOutsPeak[1]);

        m_activeBefore = m_active;
    }

    void bufferSizeChanged(uint32_t newBufferSize)
    {
        if (m_active)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
        }

#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, newBufferSize, nullptr, x_engine->getSampleRate());
#endif
        effect->dispatcher(effect, effSetBlockSize, 0, newBufferSize, nullptr, 0.0f);

        if (m_active)
        {
            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
        }
    }

    // -------------------------------------------------------------------
    // Post-poned events

#ifndef BUILD_BRIDGE
    void uiParameterChange(const uint32_t index, const double value)
    {
        Q_ASSERT(index < param.count);

        if (index >= param.count)
            return;

        if (gui.type == GUI_EXTERNAL_OSC && osc.data.target)
            osc_send_control(&osc.data, param.data[index].rindex, value);
    }

    void uiProgramChange(const uint32_t index)
    {
        Q_ASSERT(index < prog.count);

        if (index >= prog.count)
            return;

        if (gui.type == GUI_EXTERNAL_OSC && osc.data.target)
            osc_send_program(&osc.data, index);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);
        Q_ASSERT(velo > 0 && velo < 128);

        if (gui.type == GUI_EXTERNAL_OSC && osc.data.target)
        {
            uint8_t midiData[4] = { 0 };
            midiData[1] = MIDI_STATUS_NOTE_ON + channel;
            midiData[2] = note;
            midiData[3] = velo;
            osc_send_midi(&osc.data, midiData);
        }
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);

        if (gui.type == GUI_EXTERNAL_OSC && osc.data.target)
        {
            uint8_t midiData[4] = { 0 };
            midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
            midiData[2] = note;
            osc_send_midi(&osc.data, midiData);
        }
    }
#endif

    // -------------------------------------------------------------------

    void handleAudioMasterAutomate(const uint32_t index, const double value)
    {
        //Q_ASSERT(m_enabled);
        Q_ASSERT(index < param.count);

        if (index >= param.count /*|| ! m_enabled*/)
            return;

        if (isProcessing && ! x_engine->isOffline())
        {
            setParameterValue(index, value, false, false, false);
            postponeEvent(PluginPostEventParameterChange, index, 0, value);
        }
        else
            setParameterValue(index, value, isProcessing, true, true);
    }

    intptr_t handleAudioMasterGetCurrentProcessLevel()
    {
        if (x_engine->isOffline())
            return kVstProcessLevelOffline;
        if (isProcessing)
            return kVstProcessLevelRealtime;
        return kVstProcessLevelUser;
    }

    intptr_t handleAudioMasterGetBlockSize()
    {
        return x_engine->getBufferSize();
    }

    intptr_t handleAudioMasterGetSampleRate()
    {
        return x_engine->getSampleRate();
    }

    const VstTimeInfo_R* handleAudioMasterGetTime()
    {
        static VstTimeInfo_R vstTimeInfo;
        memset(&vstTimeInfo, 0, sizeof(VstTimeInfo_R));

        const CarlaTimeInfo* const timeInfo = x_engine->getTimeInfo();

        vstTimeInfo.flags |= kVstTransportChanged;

        if (timeInfo->playing)
            vstTimeInfo.flags |= kVstTransportPlaying;

        vstTimeInfo.samplePos  = timeInfo->frame;
        vstTimeInfo.sampleRate = x_engine->getSampleRate();

        vstTimeInfo.nanoSeconds = timeInfo->time;
        vstTimeInfo.flags |= kVstNanosValid;

        if (timeInfo->valid & CarlaEngineTimeBBT)
        {
            double ppqBar  = double(timeInfo->bbt.bar) * timeInfo->bbt.beats_per_bar - timeInfo->bbt.beats_per_bar;
            double ppqBeat = double(timeInfo->bbt.beat) - 1.0;
            double ppqTick = double(timeInfo->bbt.tick) / timeInfo->bbt.ticks_per_beat;

            // Bars
            vstTimeInfo.barStartPos = ppqBar + ppqBeat;
            vstTimeInfo.flags |= kVstBarsValid;

            // PPQ Pos
            vstTimeInfo.ppqPos = ppqBar + ppqBeat + ppqTick;
            vstTimeInfo.flags |= kVstPpqPosValid;

            // Tempo
            vstTimeInfo.tempo  = timeInfo->bbt.beats_per_minute;
            vstTimeInfo.flags |= kVstTempoValid;

            // Time Signature
            vstTimeInfo.timeSigNumerator   = timeInfo->bbt.beats_per_bar;
            vstTimeInfo.timeSigDenominator = timeInfo->bbt.beat_type;
            vstTimeInfo.flags |= kVstTimeSigValid;
        }
        else
        {
            // Tempo
            vstTimeInfo.tempo  = 120.0;
            vstTimeInfo.flags |= kVstTempoValid;

            // Time Signature
            vstTimeInfo.timeSigNumerator   = 4;
            vstTimeInfo.timeSigDenominator = 4;
            vstTimeInfo.flags |= kVstTimeSigValid;
        }

        return &vstTimeInfo;
    }

    intptr_t handleAudioMasterTempoAt()
    {
        const CarlaTimeInfo* const timeInfo = x_engine->getTimeInfo();

        if (timeInfo->valid & CarlaEngineTimeBBT)
            return timeInfo->bbt.beats_per_minute * 10000;

        return 0;
    }

    intptr_t handleAudioMasterIOChanged()
    {
        qDebug("VstPlugin::handleAudioMasterIOChanged()");
        Q_ASSERT(m_enabled);

        // TESTING

        if (! m_enabled)
            return 1;

#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            qCritical("VstPlugin::handleAudioMasterIOChanged() - plugin asked IO change, but it's not supported in rack mode");
            return 0;
        }
#endif

        engineProcessLock();
        m_enabled = false;
        engineProcessUnlock();

        if (m_active)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
        }

        reload();

        if (m_active)
        {
            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
        }

        x_engine->callback(CALLBACK_RELOAD_ALL, m_id, 0, 0, 0.0);

        return 1;
    }

    intptr_t handleAudioMasterProcessEvents(const VstEvents* const vstEvents)
    {
        Q_ASSERT(m_enabled);
        Q_ASSERT(midi.portMout);
        Q_ASSERT(isProcessing);

        if (! m_enabled)
            return 0;

        if (! midi.portMout)
            return 0;

        if (! isProcessing)
        {
            qCritical("VstPlugin:handleAudioMasterProcessEvents(%p) - received MIDI out events outside audio thread, ignoring", vstEvents);
            return 0;
        }

        for (int32_t i=0; i < vstEvents->numEvents && events.numEvents < MAX_MIDI_EVENTS*2; i++)
        {
            if (! vstEvents->events[i])
                break;

            const VstMidiEvent* const vstMidiEvent = (const VstMidiEvent*)vstEvents->events[i];

            if (vstMidiEvent->type == kVstMidiType)
                memcpy(&midiEvents[events.numEvents++], vstMidiEvent, sizeof(VstMidiEvent));
        }

        return 1;
    }

    intptr_t handleAdioMasterSizeWindow(int32_t width, int32_t height)
    {
        qDebug("VstPlugin::handleAudioMasterSizeWindow(%i, %i)", width, height);

        gui.width  = width;
        gui.height = height;

        x_engine->callback(CALLBACK_RESIZE_GUI, m_id, width, height, 0.0);

        return 1;
    }

    void handleAudioMasterUpdateDisplay()
    {
        qDebug("VstPlugin::handleAudioMasterUpdateDisplay()");

        // Update current program name
        if (prog.count > 0 && prog.current >= 0)
        {
            const int32_t index = prog.current;
            char strBuf[STR_MAX] = { 0 };

            effect->dispatcher(effect, effGetProgramName, 0, 0, strBuf, 0.0f);

            if (! prog.names[index])
            {
                prog.names[index] = strdup(strBuf);
            }
            else if (strBuf[0] != 0 && strcmp(strBuf, prog.names[index]) != 0)
            {
                free((void*)prog.names[index]);
                prog.names[index] = strdup(strBuf);
            }
        }

        // Tell backend to update
        x_engine->callback(CALLBACK_UPDATE, m_id, 0, 0, 0.0);
    }

    void handleAudioMasterWantMidi()
    {
        qDebug("VstPlugin::handleAudioMasterWantMidi()");

        m_hints |= PLUGIN_WANTS_MIDI_INPUT;
    }

    // -------------------------------------------------------------------

    static intptr_t hostCanDo(const char* const feature)
    {
        qDebug("VstPlugin::hostCanDo(\"%s\")", feature);

        if (strcmp(feature, "supplyIdle") == 0)
            return 1;
        if (strcmp(feature, "sendVstEvents") == 0)
            return 1;
        if (strcmp(feature, "sendVstMidiEvent") == 0)
            return 1;
        if (strcmp(feature, "sendVstMidiEventFlagIsRealtime") == 0)
            return -1;
        if (strcmp(feature, "sendVstTimeInfo") == 0)
            return 1;
        if (strcmp(feature, "receiveVstEvents") == 0)
            return 1;
        if (strcmp(feature, "receiveVstMidiEvent") == 0)
            return 1;
        if (strcmp(feature, "receiveVstTimeInfo") == 0)
            return -1;
        if (strcmp(feature, "reportConnectionChanges") == 0)
            return -1;
        if (strcmp(feature, "acceptIOChanges") == 0)
        {
#ifndef BUILD_BRIDGE
            if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
                return -1;
#endif
            return 1;
        }
        if (strcmp(feature, "sizeWindow") == 0)
            return 1;
        if (strcmp(feature, "offline") == 0)
            return -1;
        if (strcmp(feature, "openFileSelector") == 0)
            return -1;
        if (strcmp(feature, "closeFileSelector") == 0)
            return -1;
        if (strcmp(feature, "startStopProcess") == 0)
            return 1;
        if (strcmp(feature, "supportShell") == 0)
            return -1;
        if (strcmp(feature, "shellCategory") == 0)
            return -1;

        // unimplemented
        qWarning("VstPlugin::hostCanDo(\"%s\") - unknown feature", feature);
        return 0;
    }

    static intptr_t VSTCALLBACK hostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#ifdef DEBUG
        if (opcode != audioMasterGetTime)
            qDebug("VstPlugin::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

#if 0
        // Cockos VST extensions
        if (/*effect &&*/ ptr && (uint32_t)opcode == 0xdeadbeef && (uint32_t)index == 0xdeadf00d)
        {
            const char* const func = (char*)ptr;

            if (strcmp(func, "GetPlayPosition") == 0)
                return 0;
            if (strcmp(func, "GetPlayPosition2") == 0)
                return 0;
            if (strcmp(func, "GetCursorPosition") == 0)
                return 0;
            if (strcmp(func, "GetPlayState") == 0)
                return 0;
            if (strcmp(func, "SetEditCurPos") == 0)
                return 0;
            if (strcmp(func, "GetSetRepeat") == 0)
                return 0;
            if (strcmp(func, "GetProjectPath") == 0)
                return 0;
            if (strcmp(func, "OnPlayButton") == 0)
                return 0;
            if (strcmp(func, "OnStopButton") == 0)
                return 0;
            if (strcmp(func, "OnPauseButton") == 0)
                return 0;
            if (strcmp(func, "IsInRealTimeAudio") == 0)
                return 0;
            if (strcmp(func, "Audio_IsRunning") == 0)
                return 0;
        }
#endif

        // Check if 'resvd1' points to this plugin
        VstPlugin* self = nullptr;

#ifdef VESTIGE_HEADER
        if (effect && effect->ptr1)
        {
            self = (VstPlugin*)effect->ptr1;
#else
        if (effect && effect->resvd1)
        {
            self = (VstPlugin*)effect->resvd1;
#endif
            if (self->unique1 != self->unique2)
               self = nullptr;
        }

        intptr_t ret = 0;

        switch (opcode)
        {
        case audioMasterAutomate:
            if (self)
                self->handleAudioMasterAutomate(index, opt);
            break;

        case audioMasterVersion:
            ret = kVstVersion;
            break;

        case audioMasterCurrentId:
            // TODO
            break;

        case audioMasterIdle:
            //if (effect)
            //    effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterPinConnected:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterWantMidi:
            // Deprecated in VST SDK 2.4
            if (self)
                self->handleAudioMasterWantMidi();
            break;
#endif

        case audioMasterGetTime:
            if (self)
            {
                ret = (intptr_t)self->handleAudioMasterGetTime();
            }
            else
            {
                static VstTimeInfo_R timeInfo;
                memset(&timeInfo, 0, sizeof(VstTimeInfo_R));
                timeInfo.sampleRate = 44100.0;

                // Tempo
                timeInfo.tempo  = 120.0;
                timeInfo.flags |= kVstTempoValid;

                // Time Signature
                timeInfo.timeSigNumerator   = 4;
                timeInfo.timeSigDenominator = 4;
                timeInfo.flags |= kVstTimeSigValid;

                ret = (intptr_t)&timeInfo;
            }
            break;

        case audioMasterProcessEvents:
            if (self && ptr)
                ret = self->handleAudioMasterProcessEvents((const VstEvents*)ptr);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetTime:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            if (self)
                ret = self->handleAudioMasterTempoAt();
            if (ret == 0)
                ret = 120 * 10000;
            break;

        case audioMasterGetNumAutomatableParameters:
            // Deprecated in VST SDK 2.4
#ifdef BUILD_BRIDGE
            ret = MAX_PARAMETERS;
#else
            ret = carlaOptions.maxParameters;
#endif
            break;

        case audioMasterGetParameterQuantization:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterIOChanged:
            if (self)
                ret = self->handleAudioMasterIOChanged();
            break;

        case audioMasterNeedIdle:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterSizeWindow:
            if (self && index > 0 && value > 0)
                ret = self->handleAdioMasterSizeWindow(index, value);
            break;

        case audioMasterGetSampleRate:
            if (self)
                ret = self->handleAudioMasterGetSampleRate();
            else
                ret = 44100;
            break;

        case audioMasterGetBlockSize:
            if (self)
                ret = self->handleAudioMasterGetBlockSize();
            else
                ret = 512;
            break;

        case audioMasterGetInputLatency:
            // TODO
            break;

        case audioMasterGetOutputLatency:
            // TODO
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterGetPreviousPlug:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetNextPlug:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterWillReplaceOrAccumulate:
            // Deprecated in VST SDK 2.4
            break;
#endif

        case audioMasterGetCurrentProcessLevel:
            if (self)
                ret = self->handleAudioMasterGetCurrentProcessLevel();
            else
                ret = kVstProcessLevelUnknown;
            break;

        case audioMasterGetAutomationState:
            ret = kVstAutomationReadWrite;
            break;

        case audioMasterOfflineStart:
        case audioMasterOfflineRead:
        case audioMasterOfflineWrite:
        case audioMasterOfflineGetCurrentPass:
        case audioMasterOfflineGetCurrentMetaPass:
            // TODO
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetOutputSampleRate:
            // Deprecated in VST SDK 2.4
            break;

//#ifdef VESTIGE_HEADER
//        case audioMasterGetSpeakerArrangement:
//#else
        case audioMasterGetOutputSpeakerArrangement:
//#endif
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterGetVendorString:
            if (ptr)
                strcpy((char*)ptr, "Cadence");
            break;

        case audioMasterGetProductString:
            if (ptr)
                strcpy((char*)ptr, "Carla");
            break;

        case audioMasterGetVendorVersion:
            ret = 0x050; // 0.5.0
            break;

        case audioMasterVendorSpecific:
            // TODO - cockos extensions
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetIcon:
            // Deprecated in VST SDK 2.4
            break;
#endif

        case audioMasterCanDo:
            if (ptr)
                ret = hostCanDo((const char*)ptr);
            break;

        case audioMasterGetLanguage:
            ret = kVstLangEnglish;
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterOpenWindow:
        case audioMasterCloseWindow:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        case audioMasterGetDirectory:
            // TODO
            break;

        case audioMasterUpdateDisplay:
            if (self)
                self->handleAudioMasterUpdateDisplay();
            break;

        case audioMasterBeginEdit:
        case audioMasterEndEdit:
            // TODO
            break;

        case audioMasterOpenFileSelector:
        case audioMasterCloseFileSelector:
            // TODO
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterEditFile:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetChunkFile:
            // Deprecated in VST SDK 2.4
            // TODO
            break;

        case audioMasterGetInputSpeakerArrangement:
            // Deprecated in VST SDK 2.4
            // TODO
            break;
#endif

        default:
#ifdef DEBUG
            qDebug("VstPlugin::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif
            break;
        }

        return ret;
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label)
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

        VST_Function vstFn = (VST_Function)libSymbol("VSTPluginMain");

        if (! vstFn)
        {
            vstFn = (VST_Function)libSymbol("main");

            if (! vstFn)
            {
                setLastError("Could not find the VST main entry in the plugin library");
                return false;
            }
        }

        // ---------------------------------------------------------------
        // initialize plugin

        effect = vstFn(hostCallback);

        if ((! effect) || effect->magic != kEffectMagic)
        {
            setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(filename);

        if (name)
        {
            m_name = x_engine->getUniqueName(name);
        }
        else
        {
            char strBuf[STR_MAX] = { 0 };
            effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f);

            if (strBuf[0] != 0)
                m_name = x_engine->getUniqueName(strBuf);
            else
                m_name = x_engine->getUniqueName(label);
        }

        // ---------------------------------------------------------------
        // initialize VST stuff

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, x_engine->getBufferSize(), nullptr, x_engine->getSampleRate());
#endif
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, x_engine->getSampleRate());
        effect->dispatcher(effect, effSetBlockSize, 0, x_engine->getBufferSize(), nullptr, 0.0f);
        effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

#ifdef VESTIGE_HEADER
        effect->ptr1 = this;
#else
        effect->resvd1 = (intptr_t)this;
#endif

#if ! VST_FORCE_DEPRECATED
        // dummy pre-start to catch possible wantEvents() call on old plugins
        effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
        effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
#endif

        // special checks
        if (effect->dispatcher(effect, effCanDo, 0, 0, (void*)"hasCockosExtensions", 0.0f) == 0xbeef0000)
        {
            qDebug("Plugin has Cockos extensions!");
            m_hints |= PLUGIN_HAS_COCKOS_EXTENSIONS;
        }

        if (effect->dispatcher(effect, effGetVstVersion, 0, 0, nullptr, 0.0f) < kVstVersion)
            m_hints |= PLUGIN_USES_OLD_VSTSDK;

        if ((effect->flags & effFlagsCanReplacing) > 0 && effect->processReplacing != effect->process)
            m_hints |= PLUGIN_CAN_PROCESS_REPLACING;

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

        if (effect->flags & effFlagsHasEditor)
        {
            m_hints |= PLUGIN_HAS_GUI;

#if defined(Q_OS_LINUX) && ! defined(BUILD_BRIDGE)
            if (carlaOptions.bridge_vstx11 && carlaOptions.preferUiBridges && ! (effect->flags & effFlagsProgramChunks))
            {
                osc.thread = new CarlaPluginThread(x_engine, this, CarlaPluginThread::PLUGIN_THREAD_VST_GUI);
                osc.thread->setOscData(carlaOptions.bridge_vstx11, label);
                gui.type = GUI_EXTERNAL_OSC;
            }
            else
#endif
            {
                m_hints |= PLUGIN_USES_SINGLE_THREAD;
#if defined(Q_OS_WIN)
                gui.type = GUI_INTERNAL_HWND;
#elif defined(Q_OS_MACOS)
                gui.type = GUI_INTERNAL_COCOA;
#elif defined(Q_OS_LINUX)
                gui.type = GUI_INTERNAL_X11;
#else
                m_hints &= ~PLUGIN_HAS_GUI;
#endif
            }
        }

        return true;
    }

private:
    int unique1;

    AEffect* effect;

    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_MIDI_EVENTS*2];
    } events;
    VstMidiEvent midiEvents[MAX_MIDI_EVENTS*2];

    struct {
        GuiType type;
        bool visible;
        int width;
        int height;
    } gui;

    bool isProcessing;

    int unique2;
};

CarlaPlugin* CarlaPlugin::newVST(const initializer& init)
{
    qDebug("CarlaPlugin::newVST(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

    short id = init.engine->getNewPluginId();

    if (id < 0 || id > MAX_PLUGINS)
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    VstPlugin* const plugin = new VstPlugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label))
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

        const bool stereoInput = ins == 0 || ins == 2;
        const bool stereoOutput = outs == 0 || outs == 2;

        if (! (stereoInput && stereoOutput))
        {
            setLastError("Carla's rack mode can only work with Stereo VST plugins, sorry!");
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

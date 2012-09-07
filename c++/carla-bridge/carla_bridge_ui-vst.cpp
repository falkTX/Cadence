/*
 * Carla UI bridge code
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

#ifdef BRIDGE_VST

#include "carla_bridge_client.h"
#include "carla_vst.h"
#include "carla_midi.h"

#include <QtCore/QTimerEvent>
#include <QtGui/QDialog>

#ifdef Q_WS_X11
#include <QtGui/QX11Info>
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

// fake values
uint32_t bufferSize = 512;
double   sampleRate = 44100.0;

class CarlaVstClient : public CarlaClient, QObject
{
public:
    CarlaVstClient(CarlaToolkit* const toolkit)
        : CarlaClient(toolkit),
          QObject(nullptr)
    {
        effect = nullptr;
        widget = new QDialog(nullptr);

        idleTimer = 0;

        // make client valid
        unique1 = unique2 = rand();
    }

    ~CarlaVstClient()
    {
        // make client invalid
        unique2 += 1;
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool init(const char* binary, const char*)
    {
        // -----------------------------------------------------------------
        // open DLL

        if (! libOpen(binary))
        {
            qWarning("%s", libError());
            return false;
        }

        // -----------------------------------------------------------------
        // get DLL main entry

        VST_Function vstFn = (VST_Function)libSymbol("VSTPluginMain");

        if (! vstFn)
            vstFn = (VST_Function)libSymbol("main");

        if (! vstFn)
            return false;

        // -----------------------------------------------------------------
        // initialize plugin

        effect = vstFn(hostCallback);

        if (! (effect && effect->magic == kEffectMagic))
            return false;

        // -----------------------------------------------------------------
        // initialize VST stuff

#ifdef VESTIGE_HEADER
        effect->ptr1 = this;
#else
        effect->resvd1 = (intptr_t)this;
#endif

        int32_t value = 0;
#ifdef Q_WS_X11
        value = (int64_t)QX11Info::display();
#endif

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, bufferSize, nullptr, sampleRate);
#endif
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
        effect->dispatcher(effect, effSetBlockSize, 0, bufferSize, nullptr, 0.0f);
        effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

        if (effect->dispatcher(effect, effEditOpen, 0, value, (void*)widget->winId(), 0.0f) != 1)
            return false;

        // -----------------------------------------------------------------
        // initialize gui stuff

        ERect* vstRect = nullptr;
        effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f);

        if (vstRect)
        {
            int width  = vstRect->right - vstRect->left;
            int height = vstRect->bottom - vstRect->top;

            if (width > 0 && height > 0)
                widget->setFixedSize(width, height);
        }

        idleTimer = startTimer(50);

        return true;
    }

    void close()
    {
        if (effect)
        {
            effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        }
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const double value)
    {
        if (effect)
            effect->setParameter(effect, rindex, value);
    }

    void setProgram(const uint32_t index)
    {
        if (effect)
            effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
    }

    void setMidiProgram(const uint32_t, const uint32_t)
    {
    }

    void noteOn(const uint8_t, const uint8_t, const uint8_t)
    {
    }

    void noteOff(const uint8_t, const uint8_t)
    {
    }

    // ---------------------------------------------------------------------
    // gui

    void* getWidget() const
    {
        return widget;
    }

    bool isResizable() const
    {
        return false;
    }

    bool needsReparent() const
    {
        return true;
    }

    // ---------------------------------------------------------------------

    void handleAudioMasterAutomate(const uint32_t index, const float value)
    {
        effect->setParameter(effect, index, value);
        sendOscControl(index, value);
    }

    intptr_t handleAudioMasterGetCurrentProcessLevel()
    {
        return kVstProcessLevelUser;
    }

    intptr_t handleAudioMasterProcessEvents(const VstEvents* const vstEvents)
    {
        for (int32_t i=0; i < vstEvents->numEvents; i++)
        {
            if (! vstEvents->events[i])
                break;

            const VstMidiEvent* const vstMidiEvent = (const VstMidiEvent*)vstEvents->events[i];

            if (vstMidiEvent->type != kVstMidiType)
            {
                uint8_t status = vstMidiEvent->midiData[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && vstMidiEvent->midiData[2] == 0)
                    status -= 0x10;

                uint8_t midiBuf[4] = { 0, status, (uint8_t)vstMidiEvent->midiData[1], (uint8_t)vstMidiEvent->midiData[2] };
                sendOscMidi(midiBuf);
            }
        }

        return 1;
    }

    intptr_t handleAdioMasterSizeWindow(int32_t width, int32_t height)
    {
        Q_ASSERT(widget);

        widget->setFixedSize(width, height);

        return 1;
    }

    void handleAudioMasterUpdateDisplay()
    {
        sendOscConfigure("reloadprograms", "");
    }

    // ---------------------------------------------------------------------

    static intptr_t hostCanDo(const char* const feature)
    {
        qDebug("CarlaVstClient::hostCanDo(\"%s\")", feature);

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
            return 1;
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
        qWarning("CarlaVstClient::hostCanDo(\"%s\") - unknown feature", feature);
        return 0;
    }

    static intptr_t VSTCALLBACK hostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
    {
#if DEBUG
        qDebug("CarlaVstClient::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

        // Check if 'resvd1' points to this client
        CarlaVstClient* self = nullptr;

#ifdef VESTIGE_HEADER
        if (effect && effect->ptr1)
        {
            self = (CarlaVstClient*)effect->ptr1;
#else
        if (effect && effect->resvd1)
        {
            self = (CarlaVstClient*)effect->resvd1;
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

        case audioMasterGetTime:
            static VstTimeInfo_R timeInfo;
            memset(&timeInfo, 0, sizeof(VstTimeInfo_R));
            timeInfo.sampleRate = sampleRate;

            // Tempo
            timeInfo.tempo  = 120.0;
            timeInfo.flags |= kVstTempoValid;

            // Time Signature
            timeInfo.timeSigNumerator   = 4;
            timeInfo.timeSigDenominator = 4;
            timeInfo.flags |= kVstTimeSigValid;

            ret = (intptr_t)&timeInfo;
            break;

        case audioMasterProcessEvents:
            if (self && ptr)
                ret = self->handleAudioMasterProcessEvents((const VstEvents*)ptr);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            ret = 120 * 10000;
            break;
#endif

        case audioMasterSizeWindow:
            if (self && index > 0 && value > 0)
                ret = self->handleAdioMasterSizeWindow(index, value);
            break;

        case audioMasterGetSampleRate:
            ret = sampleRate;
            break;

        case audioMasterGetBlockSize:
            ret = bufferSize;
            break;

        case audioMasterGetCurrentProcessLevel:
            ret = kVstProcessLevelUser;
            break;

        case audioMasterGetAutomationState:
            ret = kVstAutomationReadWrite;
            break;

        case audioMasterGetVendorString:
            if (ptr)
                strcpy((char*)ptr, "Cadence");
            break;

        case audioMasterGetProductString:
            if (ptr)
                strcpy((char*)ptr, "Carla-Bridge");
            break;

        case audioMasterGetVendorVersion:
            ret = 0x050; // 0.5.0
            break;

        case audioMasterCanDo:
            if (ptr)
                ret = hostCanDo((const char*)ptr);
            break;

        case audioMasterGetLanguage:
            ret = kVstLangEnglish;
            break;

        case audioMasterUpdateDisplay:
            if (self)
                self->handleAudioMasterUpdateDisplay();
            break;

        default:
#ifdef DEBUG
            qDebug("CarlaVstClient::hostCallback(%p, %s, %i, " P_INTPTR ", %p, %f", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif
            break;
        }

        return ret;
    }

protected:
    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == idleTimer && effect)
        {
            effect->dispatcher(effect, effIdle, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
        }

        QObject::timerEvent(event);
    }

private:
    int unique1;
    AEffect* effect;
    QDialog* widget;
    int idleTimer;
    int unique2;
};

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    using namespace CarlaBridge;

    if (argc != 4)
    {
        qWarning("usage: %s <osc-url|\"null\"> <binary> <ui-title>", argv[0]);
        return 1;
    }

    const char* oscUrl  = argv[1];
    const char* binary  = argv[2];
    const char* uiTitle = argv[3];

    const bool useOsc = strcmp(oscUrl, "null");

    // try to get sampleRate value
    const char* const sampleRateStr = getenv("CARLA_SAMPLE_RATE");

    if (sampleRateStr)
        sampleRate = atof(sampleRateStr);

    // Init toolkit
    CarlaToolkit* const toolkit = CarlaToolkit::createNew(uiTitle);
    toolkit->init();

    // Init VST-UI
    CarlaVstClient client(toolkit);

    // Init OSC
    if (useOsc && ! client.oscInit(oscUrl))
    {
        toolkit->quit();
        delete toolkit;
        return -1;
    }

    // Load UI
    int ret;

    if (client.init(binary, nullptr))
    {
        toolkit->exec(&client, !useOsc);
        ret = 0;
    }
    else
    {
        qCritical("Failed to load VST UI");
        ret = 1;
    }

    // Close OSC
    if (useOsc)
    {
        client.sendOscExiting();
        client.oscClose();
    }

    // Close VST-UI
    client.close();

    // Close toolkit
    toolkit->quit();
    delete toolkit;

    return ret;
}

#endif

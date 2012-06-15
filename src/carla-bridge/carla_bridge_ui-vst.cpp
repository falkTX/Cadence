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

#include "carla_bridge.h"
#include "carla_bridge_osc.h"
#include "carla_midi.h"

#include "carla_vst_includes.h"

#include <QtGui/QDialog>

ClientData* client = nullptr;

// -------------------------------------------------------------------------

#define FAKE_SAMPLE_RATE 44100.0
#define FAKE_BUFFER_SIZE 512

class VstUiData : public ClientData
{
public:
    VstUiData(const char* ui_title) : ClientData(ui_title)
    {
        effect = nullptr;
        widget = new QDialog;
    }

    ~VstUiData()
    {
    }

    // ---------------------------------------------------------------------
    // initialization

    bool init(const char* binary, const char*)
    {
        // -----------------------------------------------------------------
        // open DLL

        if ( !lib_open(binary))
            return false;

        // -----------------------------------------------------------------
        // get DLL main entry

        VST_Function vstfn = (VST_Function)lib_symbol("VSTPluginMain");

        if (! vstfn)
        {
            vstfn = (VST_Function)lib_symbol("main");

            if (! vstfn)
                return false;
        }

        // -----------------------------------------------------------------
        // initialize plugin

        effect = vstfn(VstHostCallback);

        if (! effect || effect->magic != kEffectMagic)
            return false;

        // -----------------------------------------------------------------
        // initialize VST stuff

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, FAKE_BUFFER_SIZE, nullptr, FAKE_SAMPLE_RATE);
#endif
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, FAKE_SAMPLE_RATE);
        effect->dispatcher(effect, effSetBlockSize, 0, FAKE_BUFFER_SIZE, nullptr, 0.0f);
        effect->dispatcher(effect, effEditOpen, 0, 0, (void*)widget->winId(), 0.0f);

#ifdef VESTIGE_HEADER
        effect->ptr1 = this;
#else
        effect->resvd1 = (intptr_t)this;
#endif

        // -----------------------------------------------------------------
        // initialize gui stuff

        ERect* vstRect;

        if (effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0.0f))
        {
            int width  = vstRect->right - vstRect->left;
            int height = vstRect->bottom - vstRect->top;
            widget->setFixedSize(width, height);
            return true;
        }

        return false;
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

    void set_parameter(uint32_t index, double value)
    {
        if (effect)
            effect->setParameter(effect, index, value);
    }

    void set_program(uint32_t index)
    {
        if (effect)
            effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
    }

    void set_midi_program(uint32_t, uint32_t) {}
    void note_on(uint8_t, uint8_t) {}
    void note_off(uint8_t) {}

    // ---------------------------------------------------------------------
    // gui

    void* get_widget() const
    {
        return widget;
    }

    bool is_resizable() const
    {
        return false;
    }

    bool needs_reparent() const
    {
        return true;
    }

    static intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        switch (opcode)
        {
        case audioMasterAutomate:
            osc_send_control(index, opt);
            break;

        case audioMasterVersion:
            return kVstVersion;

        case audioMasterCurrentId:
            return 0; // TODO

        case audioMasterIdle:
            if (effect)
                effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            break;

        case audioMasterGetTime:
            static VstTimeInfo_R timeInfo;
            memset(&timeInfo, 0, sizeof(VstTimeInfo_R));
            timeInfo.sampleRate = FAKE_SAMPLE_RATE;
            return (intptr_t)&timeInfo;

        case audioMasterProcessEvents:
            if (client && ptr)
            {
                const VstEvents* const events = (VstEvents*)ptr;

                for (int32_t i=0; i < events->numEvents; i++)
                {
                    const VstMidiEvent* const midi_event = (VstMidiEvent*)events->events[i];

                    uint8_t status = midi_event->midiData[0];

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && midi_event->midiData[2] == 0)
                        status -= 0x10;

                    uint8_t midi_buf[4] = { 0, status, (uint8_t)midi_event->midiData[1], (uint8_t)midi_event->midiData[2] };
                    osc_send_midi(midi_buf);
                }
            }
            else
                qDebug("VstHostCallback:audioMasterProcessEvents - Some MIDI Out events were ignored");

            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            return 120.0 * 10000;
#endif

        case audioMasterSizeWindow:
            if (client)
                client->queque_message(BRIDGE_MESSAGE_RESIZE_GUI, index, value, 0.0f);
            return 1;

        case audioMasterGetSampleRate:
            return FAKE_SAMPLE_RATE;

        case audioMasterGetBlockSize:
            return FAKE_BUFFER_SIZE;

        case audioMasterGetVendorString:
            strcpy((char*)ptr, "falkTX");
            break;

        case audioMasterGetProductString:
            strcpy((char*)ptr, "Carla-Bridge");
            break;

        case audioMasterGetVendorVersion:
            return 0x05; // 0.5

        case audioMasterVendorSpecific:
            break;

        case audioMasterCanDo:
#if DEBUG
            qDebug("VstHostCallback:audioMasterCanDo - %s", (char*)ptr);
#endif

            if (strcmp((char*)ptr, "supplyIdle") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstEvents") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime") == 0)
                return -1;
            if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstEvents") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
                return -1;
            if (strcmp((char*)ptr, "reportConnectionChanges") == 0)
                return -1;
            if (strcmp((char*)ptr, "acceptIOChanges") == 0)
                return 1;
            if (strcmp((char*)ptr, "sizeWindow") == 0)
                return 1;
            if (strcmp((char*)ptr, "offline") == 0)
                return -1;
            if (strcmp((char*)ptr, "openFileSelector") == 0)
                return -1;
            if (strcmp((char*)ptr, "closeFileSelector") == 0)
                return -1;
            if (strcmp((char*)ptr, "startStopProcess") == 0)
                return 1;
            if (strcmp((char*)ptr, "supportShell") == 0)
                return -1;
            if (strcmp((char*)ptr, "shellCategory") == 0)
                return -1;

            // unimplemented
            qWarning("VstHostCallback:audioMasterCanDo - Got unknown feature request '%s'", (char*)ptr);
            return 0;

        case audioMasterGetLanguage:
            return kVstLangEnglish;

        case audioMasterUpdateDisplay:
            //osc_send_configure("reloadprograms", "");
            break;

        default:
#if DEBUG
            qDebug("VstHostCallback() - code: %s, index: %i, value: " P_INTPTR ", opt: %f", VstOpcode2str(opcode), index, value, opt);
#endif
            break;
        }

        return 0;
    }

private:
    AEffect* effect;
    QDialog* widget;
};

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        qCritical("%s: bad arguments", argv[0]);
        return 1;
    }

    const char* osc_url  = argv[1];
    const char* binary   = argv[2];
    const char* ui_title = argv[3];

    // Init toolkit
    toolkit_init();

    // Init VST-UI
    client = new VstUiData(ui_title);

    // Init OSC
    osc_init(osc_url);

    // Load UI
    int ret;

    if (client->init(binary, nullptr))
    {
        toolkit_loop();
        ret = 0;
    }
    else
    {
        qCritical("Failed to load VST UI");
        ret = 1;
    }

    // Close OSC
    osc_send_exiting();
    osc_close();

    // Close VST-UI
    client->close();

    // Close toolkit
    if (! ret)
        toolkit_quit();

    delete client;
    client = nullptr;

    return ret;
}

/*
 * Carla Plugin discovery code
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

#include "carla_includes.h"
#include "carla_lib_includes.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

#include "carla_ladspa_includes.h"
#include "carla_lv2_includes.h"
#include "carla_vst_includes.h"

#include "dssi/dssi.h"

#ifdef BUILD_NATIVE
#  ifdef WANT_FLUIDSYNTH
#    include <fluidsynth.h>
#  endif
#  ifdef WANT_LINUXSAMPLER
#    include "carla_linuxsampler_includes.h"
#  endif
#endif

#include "carla_backend.h"

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

// fake values to test plugins with
const uint32_t bufferSize = 512;
const double   sampleRate = 44100.0;

// Since discovery can find multi-architecture binaries, don't print ELF related errors
void print_lib_error(const char* const filename)
{
    const char* const error = lib_error(filename);
    if (error && strstr(error, "wrong ELF class") == nullptr && strstr(error, "Bad EXE format") == nullptr)
        DISCOVERY_OUT("error", error);
}

using namespace CarlaBackend;

// ------------------------------ VST Stuff ------------------------------

intptr_t VstCurrentUniqueId = 0;

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
#if DEBUG
    qDebug("VstHostCallback(%p, opcode: %s, index: %i, value: " P_INTPTR ", opt: %f", effect, VstMasterOpcode2str(opcode), index, value, opt);
#endif

    switch (opcode)
    {
    case audioMasterAutomate:
        if (effect)
            effect->setParameter(effect, index, opt);
        break;

    case audioMasterVersion:
        return kVstVersion;

    case audioMasterCurrentId:
        return VstCurrentUniqueId;

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

        return (intptr_t)&timeInfo;

    case audioMasterTempoAt:
        // Deprecated in VST SDK 2.4
        return 120 * 10000;

    case audioMasterGetSampleRate:
        return sampleRate;

    case audioMasterGetBlockSize:
        return bufferSize;

    case audioMasterGetVendorString:
        strcpy((char*)ptr, "falkTX");
        break;

    case audioMasterGetProductString:
        strcpy((char*)ptr, "Carla-Discovery");
        break;

    case audioMasterGetVendorVersion:
        return 0x05; // 0.5

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
            return -1;
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
            return 1;
        if (strcmp((char*)ptr, "shellCategory") == 0)
            return 1;

        // unimplemented
        qWarning("VstHostCallback:audioMasterCanDo - Got unknown feature request '%s'", (char*)ptr);
        return 0;

    case audioMasterGetLanguage:
        return kVstLangEnglish;

    default:
        qDebug("VstHostCallback(%p, opcode: %s, index: %i, value: " P_INTPTR ", opt: %f", effect, VstMasterOpcode2str(opcode), index, value, opt);
        break;
    }

    return 0;
    Q_UNUSED(value);
}

// ------------------------------ Plugin Checks -----------------------------

void do_ladspa_check(void* const lib_handle, const bool init)
{
    const LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function)lib_symbol(lib_handle, "ladspa_descriptor");

    if (! descfn)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    unsigned long i = 0;
    const LADSPA_Descriptor* descriptor;

    while ((descriptor = descfn(i++)))
    {
        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;

        for (unsigned long j=0; j < descriptor->PortCount; j++)
        {
            const LADSPA_PortDescriptor PortDescriptor = descriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor) && strcmp(descriptor->PortNames[j], "latency") != 0 && strcmp(descriptor->PortNames[j], "_latency") != 0)
                    parametersOuts += 1;
                parametersTotal += 1;
            }
        }

        if (init)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            const LADSPA_Handle handle = descriptor->instantiate(descriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init LADSPA plugin");
                continue;
            }

            float bufferAudio[bufferSize][audioTotal];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audioTotal);

            float bufferParams[parametersTotal];
            memset(bufferParams, 0, sizeof(float)*parametersTotal);

            float min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < descriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  PortHint = descriptor->PortRangeHints[j];

                if (LADSPA_IS_PORT_AUDIO(PortType))
                {
                    descriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(PortType))
                {
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
                        DISCOVERY_OUT("error", "Broken parameter: max - min == 0");
                        max = min + 0.1;
                    }

                    // default value
                    def = get_default_ladspa_port_value(PortHint.HintDescriptor, min, max);

                    if (def < min)
                        def = min;
                    else if (def > max)
                        def = max;

                    if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(PortType) && (strcmp(descriptor->PortNames[j], "latency") == 0 || strcmp(descriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0.0;
                        max = sampleRate;
                        def = 0.0;
                    }

                    bufferParams[iP] = def;

                    descriptor->connect_port(handle, j, &bufferParams[iP++]);
                }
            }

            if (descriptor->activate)
                descriptor->activate(handle);

            descriptor->run(handle, bufferSize);

            if (descriptor->deactivate)
                descriptor->deactivate(handle);

            if (descriptor->cleanup)
                descriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", descriptor->Name);
        DISCOVERY_OUT("label", descriptor->Label);
        DISCOVERY_OUT("maker", descriptor->Maker);
        DISCOVERY_OUT("copyright", descriptor->Copyright);
        DISCOVERY_OUT("unique_id", descriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_dssi_check(void* const lib_handle, const bool init)
{
    const DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)lib_symbol(lib_handle, "dssi_descriptor");

    if (! descfn)
    {
        DISCOVERY_OUT("error", "Not a DSSI plugin");
        return;
    }

    unsigned long i = 0;
    const DSSI_Descriptor* descriptor;

    while ((descriptor = descfn(i++)))
    {
        const LADSPA_Descriptor* ldescriptor = descriptor->LADSPA_Plugin;

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int midiIns = 0;
        int midiTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;
        int programsTotal = 0;

        for (unsigned long j=0; j < ldescriptor->PortCount; j++)
        {
            const LADSPA_PortDescriptor PortDescriptor = ldescriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor) && strcmp(ldescriptor->PortNames[j], "latency") != 0 && strcmp(ldescriptor->PortNames[j], "_latency") != 0)
                    parametersOuts += 1;
                parametersTotal += 1;
            }
        }

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            midiIns = midiTotal = 1;

        if (midiIns > 0 && audioOuts > 0)
            hints |= PLUGIN_IS_SYNTH;

        if (init)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            const LADSPA_Handle handle = ldescriptor->instantiate(ldescriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init DSSI plugin");
                continue;
            }

            // we can only get program list per-handle
            if (descriptor->get_program)
            {
                while ((descriptor->get_program(handle, programsTotal++)))
                    continue;
            }

            float bufferAudio[bufferSize][audioTotal];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audioTotal);

            float bufferParams[parametersTotal];
            memset(bufferParams, 0, sizeof(float)*parametersTotal);

            float min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < ldescriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  PortHint = ldescriptor->PortRangeHints[j];

                if (LADSPA_IS_PORT_AUDIO(PortType))
                {
                    ldescriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(PortType))
                {
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
                        DISCOVERY_OUT("error", "Broken parameter: max - min == 0");
                        max = min + 0.1;
                    }

                    // default value
                    def = get_default_ladspa_port_value(PortHint.HintDescriptor, min, max);

                    if (def < min)
                        def = min;
                    else if (def > max)
                        def = max;

                    if (LADSPA_IS_HINT_SAMPLE_RATE(PortHint.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(PortType) && (strcmp(ldescriptor->PortNames[j], "latency") == 0 || strcmp(ldescriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0.0;
                        max = sampleRate;
                        def = 0.0;
                    }

                    bufferParams[iP] = def;

                    ldescriptor->connect_port(handle, j, &bufferParams[iP++]);
                }
            }

            if (ldescriptor->activate)
                ldescriptor->activate(handle);

            if (descriptor->run_synth || descriptor->run_multiple_synths)
            {
                snd_seq_event_t midiEvents[2];
                memset(midiEvents, 0, sizeof(snd_seq_event_t)*2);

                const unsigned long midiEventCount = 2;

                midiEvents[0].type = SND_SEQ_EVENT_NOTEON;
                midiEvents[0].data.note.note     = 64;
                midiEvents[0].data.note.velocity = 100;

                midiEvents[1].type = SND_SEQ_EVENT_NOTEOFF;
                midiEvents[1].data.note.note     = 64;
                midiEvents[1].data.note.velocity = 0;
                midiEvents[1].time.tick = bufferSize/2;

                if (descriptor->run_multiple_synths)
                {
                    LADSPA_Handle handlePtr[1] = { handle };
                    snd_seq_event_t* midiEventsPtr[1] = { midiEvents };
                    unsigned long midiEventCountPtr[1] = { midiEventCount };
                    descriptor->run_multiple_synths(1, handlePtr, bufferSize, midiEventsPtr, midiEventCountPtr);
                }
                else
                    descriptor->run_synth(handle, bufferSize, midiEvents, midiEventCount);
            }
            else
                ldescriptor->run(handle, bufferSize);

            if (ldescriptor->deactivate)
                ldescriptor->deactivate(handle);

            if (ldescriptor->cleanup)
                ldescriptor->cleanup(handle);

            // end crash-free plugin test
            // -----------------------------------------------------------------------
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", ldescriptor->Name);
        DISCOVERY_OUT("label", ldescriptor->Label);
        DISCOVERY_OUT("maker", ldescriptor->Maker);
        DISCOVERY_OUT("copyright", ldescriptor->Copyright);
        DISCOVERY_OUT("unique_id", ldescriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.total", midiTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("programs.total", programsTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_lv2_check(const char* const bundle, const bool init)
{
    // Convert bundle filename to URI
    QString qBundle(QUrl::fromLocalFile(bundle).toString());
    if (! qBundle.endsWith(QDir::separator()))
        qBundle += QDir::separator();

    // Load bundle
    Lilv::Node lilvBundle(Lv2World.new_uri(qBundle.toUtf8().constData()));
    Lv2World.load_bundle(lilvBundle);

    // Load plugins in this bundle
    const Lilv::Plugins lilvPlugins = Lv2World.get_all_plugins();

    // Get all plugin URIs in this bundle
    QStringList URIs;

    LILV_FOREACH(plugins, i, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilv_plugins_get(lilvPlugins, i));
        URIs.append(QString(lilvPlugin.get_uri().as_string()));
    }

    // Get & check every plugin-instance
    for (int i=0; i < URIs.count(); i++)
    {
        const LV2_RDF_Descriptor* const rdf_descriptor = lv2_rdf_new(URIs.at(i).toUtf8().constData());

        if (init)
        {
            // test if DLL is loadable
            void* const lib_handle = lib_open(rdf_descriptor->Binary);

            if (! lib_handle)
            {
                print_lib_error(rdf_descriptor->Binary);
                continue;
            }

            lib_close(lib_handle);

            // test if we support all required ports and features
            bool supported = true;

            for (uint32_t j=0; j < rdf_descriptor->PortCount; j++)
            {
                const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[j];
                bool validPort = (LV2_IS_PORT_CONTROL(Port->Type) || LV2_IS_PORT_AUDIO(Port->Type) || LV2_IS_PORT_ATOM_SEQUENCE(Port->Type) || LV2_IS_PORT_CV(Port->Type) || LV2_IS_PORT_EVENT(Port->Type) || LV2_IS_PORT_MIDI_LL(Port->Type));

                if (! (validPort || LV2_IS_PORT_OPTIONAL(Port->Properties)))
                {
                    DISCOVERY_OUT("error", "plugin requires a non-supported port type, port-name: " << Port->Name);
                    supported = false;
                    break;
                }
            }

            for (uint32_t j=0; j < rdf_descriptor->FeatureCount && supported; j++)
            {
                const LV2_RDF_Feature* const Feature = &rdf_descriptor->Features[j];

                if (LV2_IS_FEATURE_REQUIRED(Feature->Type) && ! is_lv2_feature_supported(Feature->URI))
                {
                    DISCOVERY_OUT("error", "plugin requires a non-supported feature " << Feature->URI);
                    supported = false;
                    break;
                }
            }

            if (! supported)
                continue;
        }

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int midiIns = 0;
        int midiOuts = 0;
        int midiTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;

        for (uint32_t j=0; j < rdf_descriptor->PortCount; j++)
        {
            const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[j];

            if (LV2_IS_PORT_AUDIO(Port->Type))
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    audioIns += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LV2_IS_PORT_CONTROL(Port->Type))
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    parametersIns += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type) && ! LV2_IS_PORT_LATENCY(Port->Designation))
                    parametersOuts += 1;
                parametersTotal += 1;
            }
            else if (Port->Type & LV2_PORT_SUPPORTS_MIDI_EVENT)
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    midiIns += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type))
                    midiOuts += 1;
                midiTotal += 1;
            }
        }

        if (rdf_descriptor->Type & LV2_CLASS_INSTRUMENT)
            hints |= PLUGIN_IS_SYNTH;

        if (rdf_descriptor->UICount > 0)
            hints |= PLUGIN_HAS_GUI;

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", rdf_descriptor->Name);
        DISCOVERY_OUT("label", rdf_descriptor->URI);
        if (rdf_descriptor->Author)
            DISCOVERY_OUT("maker", rdf_descriptor->Author);
        if (rdf_descriptor->License)
            DISCOVERY_OUT("copyright", rdf_descriptor->License);
        DISCOVERY_OUT("unique_id", rdf_descriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("audio.ins", audioIns);
        DISCOVERY_OUT("audio.outs", audioOuts);
        DISCOVERY_OUT("audio.total", audioTotal);
        DISCOVERY_OUT("midi.ins", midiIns);
        DISCOVERY_OUT("midi.outs", midiOuts);
        DISCOVERY_OUT("midi.total", midiTotal);
        DISCOVERY_OUT("parameters.ins", parametersIns);
        DISCOVERY_OUT("parameters.outs", parametersOuts);
        DISCOVERY_OUT("parameters.total", parametersTotal);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_vst_check(void* const lib_handle, const bool init)
{
    VST_Function vstfn = (VST_Function)lib_symbol(lib_handle, "VSTPluginMain");

    if (! vstfn)
        vstfn = (VST_Function)lib_symbol(lib_handle, "main");

    if (! vstfn)
    {
        DISCOVERY_OUT("error", "Not a VST plugin");
        return;
    }

    AEffect* const effect = vstfn(VstHostCallback);

    if (effect && effect->magic == kEffectMagic)
    {
        const char* c_name;
        const char* c_product;
        const char* c_vendor;
        char strBuf[255] = { 0 };

        effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f);
        c_name = strdup((strBuf[0] != 0) ? strBuf : "");

        strBuf[0] = 0;
        effect->dispatcher(effect, effGetProductString, 0, 0, strBuf, 0.0f);
        c_product = strdup((strBuf[0] != 0) ? strBuf : "");

        strBuf[0] = 0;
        effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f);
        c_vendor = strdup((strBuf[0] != 0) ? strBuf : "");

        VstCurrentUniqueId = effect->uniqueID;
        intptr_t VstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        while (true)
        {
            // only init if required
            if (init || VstCategory == kPlugCategShell)
                effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

            int hints = 0;
            int audioIns = effect->numInputs;
            int audioOuts = effect->numOutputs;
            int audioTotal = audioIns + audioOuts;
            int midiIns = 0;
            int midiOuts = 0;
            int midiTotal = 0;
            int parametersIns = effect->numParams;
            int parametersTotal = parametersIns;
            int programsTotal = effect->numPrograms;

            if (effect->flags & effFlagsHasEditor)
                hints |= PLUGIN_HAS_GUI;

            if (effect->flags & effFlagsIsSynth)
                hints |= PLUGIN_IS_SYNTH;

            if (VstPluginCanDo(effect, "receiveVstEvents") || VstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) > 0)
                midiIns = 1;

            if (VstPluginCanDo(effect, "sendVstEvents") || VstPluginCanDo(effect, "sendVstMidiEvent"))
                midiOuts = 1;

            midiTotal = midiIns + midiOuts;

            // -----------------------------------------------------------------------
            // start crash-free plugin test

            if (init)
            {
                float** bufferAudioIn = new float* [audioIns];
                for (int j=0; j < audioIns; j++)
                {
                    bufferAudioIn[j] = new float [bufferSize];
                    memset(bufferAudioIn[j], 0, sizeof(float)*bufferSize);
                }

                float** bufferAudioOut = new float* [audioOuts];
                for (int j=0; j < audioOuts; j++)
                {
                    bufferAudioOut[j] = new float [bufferSize];
                    memset(bufferAudioOut[j], 0, sizeof(float)*bufferSize);
                }

                struct {
                    int32_t numEvents;
                    intptr_t reserved;
                    VstEvent* data[2];
                } events;
                VstMidiEvent midiEvents[2];
                memset(midiEvents, 0, sizeof(VstMidiEvent)*2);

                midiEvents[0].type = kVstMidiType;
                midiEvents[0].byteSize = sizeof(VstMidiEvent);
                midiEvents[0].midiData[0] = 0x90;
                midiEvents[0].midiData[1] = 64;
                midiEvents[0].midiData[2] = 100;

                midiEvents[1].type = kVstMidiType;
                midiEvents[1].byteSize = sizeof(VstMidiEvent);
                midiEvents[1].midiData[0] = 0x80;
                midiEvents[1].midiData[1] = 64;
                midiEvents[1].deltaFrames = bufferSize/2;

                events.numEvents = 2;
                events.reserved  = 0;
                events.data[0] = (VstEvent*)&midiEvents[0];
                events.data[1] = (VstEvent*)&midiEvents[1];

#if ! VST_FORCE_DEPRECATED
                effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, bufferSize, nullptr, sampleRate);
#endif
                effect->dispatcher(effect, effSetBlockSize, 0, bufferSize, nullptr, 0.0f);
                effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
                effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

                if (midiIns > 0)
                    effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);

                if (effect->flags & effFlagsCanReplacing)
                    effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#if ! VST_FORCE_DEPRECATED
                else
                    effect->process(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#endif

                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

                for (int j=0; j < audioIns; j++)
                    delete[] bufferAudioIn[j];
                for (int j=0; j < audioOuts; j++)
                    delete[] bufferAudioOut[j];
                delete[] bufferAudioIn;
                delete[] bufferAudioOut;
            }

            // end crash-free plugin test
            // -----------------------------------------------------------------------

            // only close if required
            if (init || VstCategory == kPlugCategShell)
                effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", c_name);
            DISCOVERY_OUT("label", c_product);
            DISCOVERY_OUT("maker", c_vendor);
            DISCOVERY_OUT("copyright", c_vendor);
            DISCOVERY_OUT("unique_id", VstCurrentUniqueId);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("audio.ins", audioIns);
            DISCOVERY_OUT("audio.outs", audioOuts);
            DISCOVERY_OUT("audio.total", audioTotal);
            DISCOVERY_OUT("midi.ins", midiIns);
            DISCOVERY_OUT("midi.outs", midiOuts);
            DISCOVERY_OUT("midi.total", midiTotal);
            DISCOVERY_OUT("parameters.ins", parametersIns);
            DISCOVERY_OUT("parameters.total", parametersTotal);
            DISCOVERY_OUT("programs.total", programsTotal);
            DISCOVERY_OUT("build", BINARY_NATIVE);
            DISCOVERY_OUT("end", "------------");

            if (VstCategory == kPlugCategShell)
            {
                strBuf[0] = 0;
                VstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, strBuf, 0.0f);

                if (VstCurrentUniqueId != 0)
                {
                    free((void*)c_name);
                    c_name = strdup((strBuf[0] != 0) ? strBuf : "");
                }
                else
                    break;
            }
            else
                break;
        }

        free((void*)c_name);
        free((void*)c_product);
        free((void*)c_vendor);
    }
    else
        DISCOVERY_OUT("error", "Failed to init VST plugin");
}

void do_fluidsynth_check(const char* const filename, const bool init)
{
#ifdef WANT_FLUIDSYNTH
    if (! fluid_is_soundfont(filename))
    {
        DISCOVERY_OUT("error", "Not a SF2 file");
        return;
    }

    int programs = 0;

    if (init)
    {
        fluid_settings_t* const f_settings = new_fluid_settings();
        fluid_synth_t* const f_synth = new_fluid_synth(f_settings);
        const int f_id = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id < 0)
        {
            DISCOVERY_OUT("error", "Failed to load SF2 file");
            return;
        }

        fluid_sfont_t* f_sfont;
        fluid_preset_t f_preset;

        f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);

        f_sfont->iteration_start(f_sfont);
        while (f_sfont->iteration_next(f_sfont, &f_preset))
            programs += 1;

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }

    DISCOVERY_OUT("init", "-----------");
    DISCOVERY_OUT("name", "");
    DISCOVERY_OUT("label", "");
    DISCOVERY_OUT("maker", "");
    DISCOVERY_OUT("copyright", "");
    DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
    DISCOVERY_OUT("audio.outs", 2);
    DISCOVERY_OUT("audio.total", 2);
    DISCOVERY_OUT("midi.ins", 1);
    DISCOVERY_OUT("midi.total", 1);
    DISCOVERY_OUT("programs.total", programs);
    DISCOVERY_OUT("parameters.ins", 13); // defined in Carla
    DISCOVERY_OUT("parameters.outs", 1);
    DISCOVERY_OUT("parameters.total", 14);
    DISCOVERY_OUT("build", BINARY_NATIVE);
    DISCOVERY_OUT("end", "------------");
#else
    (void)filename;
    DISCOVERY_OUT("error", "SF2 support not available");
#endif
}

void do_linuxsampler_check(const char* const filename, const char* const stype, const bool init)
{
#ifdef WANT_LINUXSAMPLER
    QFileInfo file(filename);

    if (! file.exists())
    {
        DISCOVERY_OUT("error", "Requested file does not exist");
        return;
    }

    if (! file.isFile())
    {
        DISCOVERY_OUT("error", "Requested file is not valid");
        return;
    }

    if (! file.isReadable())
    {
        DISCOVERY_OUT("error", "Requested file is not readable");
        return;
    }

    using namespace LinuxSampler;

    class LinuxSamplerScopedEngine {
    public:
        LinuxSamplerScopedEngine(const char* filename, const char* stype)
        {
            try {
                engine = EngineFactory::Create(stype);
            }
            catch (Exception& e)
            {
                DISCOVERY_OUT("error", e.what());
                return;
            }

            try {
                ins = engine->GetInstrumentManager();
            }
            catch (Exception& e)
            {
                DISCOVERY_OUT("error", e.what());
                return;
            }

            std::vector<InstrumentManager::instrument_id_t> ids;

            try {
                ids = ins->GetInstrumentFileContent(filename);
            }
            catch (Exception& e)
            {
                DISCOVERY_OUT("error", e.what());
                return;
            }

            if (ids.size() > 0)
            {
                InstrumentManager::instrument_info_t info = ins->GetInstrumentInfo(ids[0]);
                outputInfo(&info, ids.size());
            }
        }

        ~LinuxSamplerScopedEngine()
        {
            if (engine)
                EngineFactory::Destroy(engine);
        }

        static void outputInfo(InstrumentManager::instrument_info_t* const info, const int programs)
        {
            DISCOVERY_OUT("init", "-----------");

            if (info)
            {
                DISCOVERY_OUT("name", info->InstrumentName);
                DISCOVERY_OUT("label", info->Product);
                DISCOVERY_OUT("maker", info->Artists);
                DISCOVERY_OUT("copyright", info->Artists);
            }

            DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
            DISCOVERY_OUT("audio.outs", 2);
            DISCOVERY_OUT("audio.total", 2);
            DISCOVERY_OUT("midi.ins", 1);
            DISCOVERY_OUT("midi.total", 1);
            DISCOVERY_OUT("programs.total", programs);
            //DISCOVERY_OUT("parameters.ins", 13); // defined in Carla - TODO
            //DISCOVERY_OUT("parameters.outs", 1);
            //DISCOVERY_OUT("parameters.total", 14);
            DISCOVERY_OUT("build", BINARY_NATIVE);
            DISCOVERY_OUT("end", "------------");
        }

    private:
        Engine* engine;
        InstrumentManager* ins;
    };

    if (init)
        const LinuxSamplerScopedEngine engine(filename, stype);
    else
        LinuxSamplerScopedEngine::outputInfo(nullptr, 0);

#else
    (void)filename;
    DISCOVERY_OUT("error", stype << " support not available");
#endif
}

// ------------------------------ main entry point ------------------------------

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        qWarning("usage: %s <type> </path/to/plugin>", argv[0]);
        return 1;
    }

    const char* const stype    = argv[1];
    const char* const filename = argv[2];

    bool openLib;
    PluginType type;
    void* handle = nullptr;

    if (strcmp(stype, "LADSPA") == 0)
    {
        openLib = true;
        type = PLUGIN_LADSPA;
    }
    else if (strcmp(stype, "DSSI") == 0)
    {
        openLib = true;
        type = PLUGIN_DSSI;
    }
    else if (strcmp(stype, "LV2") == 0)
    {
        openLib = false;
        type = PLUGIN_LV2;
    }
    else if (strcmp(stype, "VST") == 0)
    {
        openLib = true;
        type = PLUGIN_VST;
    }
    else if (strcmp(stype, "GIG") == 0)
    {
        openLib = false;
        type = PLUGIN_GIG;
    }
    else if (strcmp(stype, "SF2") == 0)
    {
        openLib = false;
        type = PLUGIN_SF2;
    }
    else if (strcmp(stype, "SFZ") == 0)
    {
        openLib = false;
        type = PLUGIN_SFZ;
    }
    else
    {
        DISCOVERY_OUT("error", "Invalid plugin type");
        return 1;
    }

    if (openLib)
    {
        handle = lib_open(filename);

        if (! handle)
        {
            print_lib_error(filename);
            return 1;
        }
    }

    bool doInit = ! QString(filename).endsWith("dssi-vst.so", Qt::CaseInsensitive);

    if (doInit && getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECK"))
        doInit = false;

    switch (type)
    {
    case PLUGIN_LADSPA:
        do_ladspa_check(handle, doInit);
        break;
    case PLUGIN_DSSI:
        do_dssi_check(handle, doInit);
        break;
    case PLUGIN_LV2:
        do_lv2_check(filename, doInit);
        break;
    case PLUGIN_VST:
        do_vst_check(handle, doInit);
        break;
    case PLUGIN_GIG:
        do_linuxsampler_check(filename, "gig", doInit);
        break;
    case PLUGIN_SF2:
        do_fluidsynth_check(filename, doInit);
        break;
    case PLUGIN_SFZ:
        do_linuxsampler_check(filename, "sfz", doInit);
        break;
    default:
        break;
    }

    if (openLib)
        lib_close(handle);

    return 0;
}

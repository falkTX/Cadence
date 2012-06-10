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
#include "carla_vst_includes.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

#include "ladspa/ladspa.h"
#include "dssi/dssi.h"
#include "lv2_rdf.h"

#ifdef BUILD_NATIVE
#  ifdef WANT_FLUIDSYNTH
#    include <fluidsynth.h>
#  else
#    warning fluidsynth not available (no SF2 support)
#  endif
#endif

#ifdef BUILD_NATIVE
#  ifdef WANT_LINUXSAMPLER
#    include "linuxsampler/EngineFactory.h"
#  else
#    warning linuxsampler not available (no GIG and SFZ support)
#  endif
#endif

#define CARLA_BACKEND_NO_EXPORTS
#define CARLA_BACKEND_NO_NAMESPACE
#include "carla_backend.h"

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

// fake values to test plugins with
const uint32_t bufferSize = 512;
const double   sampleRate = 44100.f;

// Since discovery can find multi-architecture binaries, don't print ELF related errors
void print_lib_error(const char* filename)
{
    const char* error = lib_error(filename);
    if (error && strstr(error, "wrong ELF class") == nullptr && strstr(error, "Bad EXE format") == nullptr)
        DISCOVERY_OUT("error", error);
}

// ------------------------------ VST Stuff ------------------------------

intptr_t VstCurrentUniqueId = 0;

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
#if DEBUG
    qDebug("VstHostCallback(%p, opcode: %s, index: %i, value: " P_INTPTR ", opt: %f", effect, VstOpcode2str(opcode), index, value, opt);
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

    case audioMasterIdle:
        if (effect)
            effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
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
        qDebug("VstHostCallback(%p, opcode: %s, index: %i, value: " P_INTPTR ", opt: %f", effect, VstOpcode2str(opcode), index, value, opt);
        break;
    }

    return 0;

    (void)value;
}

// ------------------------------ Plugin Checks -----------------------------

void do_ladspa_check(void* lib_handle, bool init)
{
    LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function)lib_symbol(lib_handle, "ladspa_descriptor");

    if (! descfn)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    unsigned long i = 0;
    const LADSPA_Descriptor* descriptor;

    while ((descriptor = descfn(i++)))
    {
        LADSPA_Handle handle = nullptr;

        if (init)
        {
            handle = descriptor->instantiate(descriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init LADSPA plugin");
                continue;
            }
        }

        int hints = 0;
        int audio_ins = 0;
        int audio_outs = 0;
        int audio_total = 0;
        int parameters_ins = 0;
        int parameters_outs = 0;
        int parameters_total = 0;

        for (unsigned long j=0; j < descriptor->PortCount; j++)
        {
            const LADSPA_PortDescriptor PortDescriptor = descriptor->PortDescriptors[j];
            if (LADSPA_IS_PORT_AUDIO(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    audio_ins += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                    audio_outs += 1;
                audio_total += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    parameters_ins += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                {
                    if (strcmp(descriptor->PortNames[j], "latency") != 0 && strcmp(descriptor->PortNames[j], "_latency") != 0)
                        parameters_outs += 1;
                }
                parameters_total += 1;
            }
        }

        if (init)
        {
            // -----------------------------------------------------------------------
            // start crash-free plugin test

            float bufferAudio[bufferSize][audio_total];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audio_total);

            float bufferParams[parameters_total];
            memset(bufferParams, 0, sizeof(float)*parameters_total);

            double min, max, def;

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
        DISCOVERY_OUT("audio.ins", audio_ins);
        DISCOVERY_OUT("audio.outs", audio_outs);
        DISCOVERY_OUT("audio.total", audio_total);
        DISCOVERY_OUT("parameters.ins", parameters_ins);
        DISCOVERY_OUT("parameters.outs", parameters_outs);
        DISCOVERY_OUT("parameters.total", parameters_total);

        if (init && descriptor->cleanup)
            descriptor->cleanup(handle);

        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_dssi_check(void* lib_handle, bool init)
{
    DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)lib_symbol(lib_handle, "dssi_descriptor");

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
        LADSPA_Handle handle = nullptr;

        if (init)
        {
            handle = ldescriptor->instantiate(ldescriptor, sampleRate);

            if (! handle)
            {
                DISCOVERY_OUT("error", "Failed to init DSSI plugin");
                continue;
            }
        }

        int hints = 0;
        int audio_ins = 0;
        int audio_outs = 0;
        int audio_total = 0;
        int midi_ins = 0;
        int midi_total = 0;
        int parameters_ins = 0;
        int parameters_outs = 0;
        int parameters_total = 0;
        int programs_total = 0;

        for (unsigned long j=0; j < ldescriptor->PortCount; j++)
        {
            const LADSPA_PortDescriptor PortDescriptor = ldescriptor->PortDescriptors[j];
            if (LADSPA_IS_PORT_AUDIO(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    audio_ins += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                    audio_outs += 1;
                audio_total += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(PortDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(PortDescriptor))
                    parameters_ins += 1;
                else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
                {
                    if (strcmp(ldescriptor->PortNames[j], "latency") != 0 && strcmp(ldescriptor->PortNames[j], "_latency") != 0)
                        parameters_outs += 1;
                }
                parameters_total += 1;
            }
        }

        if (descriptor->run_synth || descriptor->run_multiple_synths)
            midi_ins = midi_total = 1;

        if (midi_ins > 0 && audio_outs > 0)
            hints |= PLUGIN_IS_SYNTH;

        if (init)
        {
            if (descriptor->get_program)
            {
                while ((descriptor->get_program(handle, programs_total++)))
                    continue;
            }

            // -----------------------------------------------------------------------
            // start crash-free plugin test

            float bufferAudio[bufferSize][audio_total];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audio_total);

            float bufferParams[parameters_total];
            memset(bufferParams, 0, sizeof(float)*parameters_total);

            double min, max, def;

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

                unsigned long midiEventCount = 2;

                midiEvents[0].type = SND_SEQ_EVENT_NOTEON;
                midiEvents[0].data.note.note     = 64;
                midiEvents[0].data.note.velocity = 100;

                midiEvents[1].type = SND_SEQ_EVENT_NOTEOFF;
                midiEvents[1].data.note.note     = 64;
                midiEvents[1].data.note.velocity = 0;
                midiEvents[1].time.tick = bufferSize/2;

                if (descriptor->run_multiple_synths)
                {
                    snd_seq_event_t* midiEventsPtr[] = { midiEvents, nullptr };
                    descriptor->run_multiple_synths(1, &handle, bufferSize, midiEventsPtr, &midiEventCount);
                }
                else
                    descriptor->run_synth(handle, bufferSize, midiEvents, midiEventCount);
            }
            else
                ldescriptor->run(handle, bufferSize);

            if (ldescriptor->deactivate)
                ldescriptor->deactivate(handle);

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
        DISCOVERY_OUT("audio.ins", audio_ins);
        DISCOVERY_OUT("audio.outs", audio_outs);
        DISCOVERY_OUT("audio.total", audio_total);
        DISCOVERY_OUT("midi.ins", midi_ins);
        DISCOVERY_OUT("midi.total", midi_total);
        DISCOVERY_OUT("parameters.ins", parameters_ins);
        DISCOVERY_OUT("parameters.outs", parameters_outs);
        DISCOVERY_OUT("parameters.total", parameters_total);
        DISCOVERY_OUT("programs.total", programs_total);

        if (init && ldescriptor->cleanup)
            ldescriptor->cleanup(handle);

        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_lv2_check(const char* bundle)
{
    // Convert bundle filename to URI
    QString qBundle(QUrl::fromLocalFile(bundle).toString());
    if (! qBundle.endsWith(QDir::separator()))
        qBundle += QDir::separator();

    // Load bundle
    Lilv::Node Bundle(Lv2World.new_uri(qBundle.toUtf8().constData()));
    Lv2World.load_bundle(Bundle);

    // Load plugins in this bundle
    const Lilv::Plugins Plugins = Lv2World.get_all_plugins();

    // Get all plugin URIs in this bundle
    QStringList URIs;

    LILV_FOREACH(plugins, i, Plugins)
    {
        Lilv::Plugin Plugin(lilv_plugins_get(Plugins, i));
        URIs.append(QString(Plugin.get_uri().as_string()));
    }

    // Get & check every plugin-instance
    for (int i=0; i < URIs.count(); i++)
    {
        const LV2_RDF_Descriptor* rdf_descriptor = lv2_rdf_new(URIs.at(i).toUtf8().constData());

        // test if DLL is loadable
        void* lib_handle = lib_open(rdf_descriptor->Binary);

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
                DISCOVERY_OUT("error", "plugin requires non-supported port, port-name: " << Port->Name);
                supported = false;
                break;
            }
        }

        if (! supported)
            continue;

        for (uint32_t j=0; j < rdf_descriptor->FeatureCount; j++)
        {
            const LV2_RDF_Feature* const Feature = &rdf_descriptor->Features[j];

            if (LV2_IS_FEATURE_REQUIRED(Feature->Type) && ! is_lv2_feature_supported(Feature->URI))
            {
                DISCOVERY_OUT("error", "plugin requires non-supported feature " << Feature->URI);
                supported = false;
                break;
            }
        }

        if (! supported)
            continue;

        int hints = 0;
        int audio_ins = 0;
        int audio_outs = 0;
        int audio_total = 0;
        int midi_ins = 0;
        int midi_outs = 0;
        int midi_total = 0;
        int parameters_ins = 0;
        int parameters_outs = 0;
        int parameters_total = 0;
        int programs_total = 0;

        for (uint32_t j=0; j < rdf_descriptor->PortCount; j++)
        {
            const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[j];

            if (LV2_IS_PORT_AUDIO(Port->Type))
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    audio_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type))
                    audio_outs += 1;
                audio_total += 1;
            }
            else if (LV2_IS_PORT_CONTROL(Port->Type))
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    parameters_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type) && ! LV2_IS_PORT_LATENCY(Port->Designation))
                    parameters_outs += 1;
                parameters_total += 1;
            }
            else if (Port->Type & LV2_PORT_SUPPORTS_MIDI_EVENT)
            {
                if (LV2_IS_PORT_INPUT(Port->Type))
                    midi_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(Port->Type))
                    midi_outs += 1;
                midi_total += 1;
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
        DISCOVERY_OUT("audio.ins", audio_ins);
        DISCOVERY_OUT("audio.outs", audio_outs);
        DISCOVERY_OUT("audio.total", audio_total);
        DISCOVERY_OUT("midi.ins", midi_ins);
        DISCOVERY_OUT("midi.outs", midi_outs);
        DISCOVERY_OUT("midi.total", midi_total);
        DISCOVERY_OUT("parameters.ins", parameters_ins);
        DISCOVERY_OUT("parameters.outs", parameters_outs);
        DISCOVERY_OUT("parameters.total", parameters_total);
        DISCOVERY_OUT("programs.total", programs_total);
        DISCOVERY_OUT("build", BINARY_NATIVE);
        DISCOVERY_OUT("end", "------------");
    }
}

void do_vst_check(void* lib_handle)
{
    VST_Function vstfn = (VST_Function)lib_symbol(lib_handle, "VSTPluginMain");

    if (! vstfn)
        vstfn = (VST_Function)lib_symbol(lib_handle, "main");

    if (! vstfn)
    {
        DISCOVERY_OUT("error", "Not a VST plugin");
        return;
    }

    AEffect* effect = vstfn(VstHostCallback);

    if (effect && effect->magic == kEffectMagic)
    {
        const char* c_name;
        const char* c_product;
        const char* c_vendor;
        char buf_str[255] = { 0 };

        effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
        c_name = strdup((buf_str[0] != 0) ? buf_str : "");

        buf_str[0] = 0;
        effect->dispatcher(effect, effGetProductString, 0, 0, buf_str, 0.0f);
        c_product = strdup((buf_str[0] != 0) ? buf_str : "");

        buf_str[0] = 0;
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
        c_vendor = strdup((buf_str[0] != 0) ? buf_str : "");

        VstCurrentUniqueId = effect->uniqueID;
        intptr_t VstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        while (true)
        {
            effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

            int hints = 0;
            int audio_ins = effect->numInputs;
            int audio_outs = effect->numOutputs;
            int audio_total = audio_ins + audio_outs;
            int midi_ins = 0;
            int midi_outs = 0;
            int midi_total = 0;
            int parameters_ins = effect->numParams;
            int parameters_total = parameters_ins;
            int programs_total = effect->numPrograms;

            if (effect->flags & effFlagsHasEditor)
                hints |= PLUGIN_HAS_GUI;

            if (effect->flags & effFlagsIsSynth)
                hints |= PLUGIN_IS_SYNTH;

            if (VstPluginCanDo(effect, "receiveVstEvents") || VstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) > 0)
                midi_ins = 1;

            if (VstPluginCanDo(effect, "sendVstEvents") || VstPluginCanDo(effect, "sendVstMidiEvent"))
                midi_outs = 1;

            midi_total = midi_ins + midi_outs;

            // -----------------------------------------------------------------------
            // start crash-free plugin test

            float** bufferAudioIn = new float* [audio_ins];
            for (int j=0; j < audio_ins; j++)
            {
                bufferAudioIn[j] = new float [bufferSize];
                memset(bufferAudioIn[j], 0, sizeof(float)*bufferSize);
            }

            float** bufferAudioOut = new float* [audio_outs];
            for (int j=0; j < audio_outs; j++)
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

            if (midi_ins > 0)
                effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);

            if (effect->flags & effFlagsCanReplacing)
                effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#if ! VST_FORCE_DEPRECATED
            else
                effect->process(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#endif

            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

            for (int j=0; j < audio_ins; j++)
                delete[] bufferAudioIn[j];
            for (int j=0; j < audio_outs; j++)
                delete[] bufferAudioOut[j];
            delete[] bufferAudioIn;
            delete[] bufferAudioOut;

            // end crash-free plugin test
            // -----------------------------------------------------------------------

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", c_name);
            DISCOVERY_OUT("label", c_product);
            DISCOVERY_OUT("maker", c_vendor);
            DISCOVERY_OUT("copyright", c_vendor);
            DISCOVERY_OUT("unique_id", VstCurrentUniqueId);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("audio.ins", audio_ins);
            DISCOVERY_OUT("audio.outs", audio_outs);
            DISCOVERY_OUT("audio.total", audio_total);
            DISCOVERY_OUT("midi.ins", midi_ins);
            DISCOVERY_OUT("midi.outs", midi_outs);
            DISCOVERY_OUT("midi.total", midi_total);
            DISCOVERY_OUT("parameters.ins", parameters_ins);
            DISCOVERY_OUT("parameters.total", parameters_total);
            DISCOVERY_OUT("programs.total", programs_total);

            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);

            DISCOVERY_OUT("build", BINARY_NATIVE);
            DISCOVERY_OUT("end", "------------");

            if (VstCategory == kPlugCategShell)
            {
                buf_str[0] = 0;
                VstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, buf_str, 0.0f);

                if (VstCurrentUniqueId != 0)
                {
                    free((void*)c_name);
                    c_name = strdup((buf_str[0] != 0) ? buf_str : "");
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

void do_fluidsynth_check(const char* filename)
{
#ifdef WANT_FLUIDSYNTH
    if (fluid_is_soundfont(filename))
    {
        fluid_settings_t* f_settings = new_fluid_settings();
        fluid_synth_t* f_synth = new_fluid_synth(f_settings);
        int f_id = fluid_synth_sfload(f_synth, filename, 0);

        if (f_id >= 0)
        {
            int programs = 0;
            fluid_sfont_t* f_sfont;
            fluid_preset_t f_preset;

            f_sfont = fluid_synth_get_sfont_by_id(f_synth, f_id);

            // initial check to know how many midi-programs we get
            f_sfont->iteration_start(f_sfont);
            while (f_sfont->iteration_next(f_sfont, &f_preset))
                programs += 1;

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", "");
            //DISCOVERY_OUT("name", f_sfont->get_name(f_sfont));
            DISCOVERY_OUT("label", "");
            DISCOVERY_OUT("maker", "");
            DISCOVERY_OUT("copyright", "");

            DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
            DISCOVERY_OUT("audio.outs", 2);
            DISCOVERY_OUT("audio.total", 2);
            DISCOVERY_OUT("midi.ins", 1);
            DISCOVERY_OUT("midi.total", 1);
            DISCOVERY_OUT("programs.total", programs);

            // defined in Carla
            DISCOVERY_OUT("parameters.ins", 13);
            DISCOVERY_OUT("parameters.outs", 1);
            DISCOVERY_OUT("parameters.total", 14);

            DISCOVERY_OUT("build", BINARY_NATIVE);
            DISCOVERY_OUT("end", "------------");
        }
        else
            DISCOVERY_OUT("error", "Failed to load SF2 file");

        delete_fluid_synth(f_synth);
        delete_fluid_settings(f_settings);
    }
    else
        DISCOVERY_OUT("error", "Not a SF2 file");
#else
    (void)filename;
    DISCOVERY_OUT("error", "SF2 support not available");
#endif
}

void do_linuxsampler_check(const char* filename, const char* stype)
{
#ifdef WANT_LINUXSAMPLER
    using namespace LinuxSampler;

    class ScopedEngine {
    public:
        ScopedEngine(const char* filename, const char* stype)
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

                DISCOVERY_OUT("init", "-----------");
                DISCOVERY_OUT("name", info.InstrumentName);
                DISCOVERY_OUT("label", info.Product);
                DISCOVERY_OUT("maker", info.Artists);
                DISCOVERY_OUT("copyright", info.Artists);

                DISCOVERY_OUT("hints", PLUGIN_IS_SYNTH);
                DISCOVERY_OUT("audio.outs", 2);
                DISCOVERY_OUT("audio.total", 2);
                DISCOVERY_OUT("midi.ins", 1);
                DISCOVERY_OUT("midi.total", 1);
                DISCOVERY_OUT("programs.total", ids.size());

                // defined in Carla - TODO
                //DISCOVERY_OUT("parameters.ins", 13);
                //DISCOVERY_OUT("parameters.outs", 1);
                //DISCOVERY_OUT("parameters.total", 14);

                DISCOVERY_OUT("build", BINARY_NATIVE);
                DISCOVERY_OUT("end", "------------");
            }
        }

        ~ScopedEngine()
        {
            if (engine)
                EngineFactory::Destroy(engine);
        }

    private:
        Engine* engine;
        InstrumentManager* ins;
    };

    QFileInfo file(filename);

    if (! file.exists())
    {
        DISCOVERY_OUT("error", "Requested file does not exist");
        return;
    }

    if (! file.isFile())
    {
        DISCOVERY_OUT("error", "Requested filename is not a file");
        return;
    }

    if (! file.isReadable())
    {
        DISCOVERY_OUT("error", "Requested file is not readable");
        return;
    }

    const ScopedEngine engine(filename, stype);

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

    const char* type_str = argv[1];
    const char* filename = argv[2];

    bool open_lib;
    PluginType type;
    void* handle = nullptr;

    if (strcmp(type_str, "LADSPA") == 0)
    {
        open_lib = true;
        type = PLUGIN_LADSPA;
    }
    else if (strcmp(type_str, "DSSI") == 0)
    {
        open_lib = true;
        type = PLUGIN_DSSI;
    }
    else if (strcmp(type_str, "LV2") == 0)
    {
        open_lib = false;
        type = PLUGIN_LV2;
    }
    else if (strcmp(type_str, "VST") == 0)
    {
        open_lib = true;
        type = PLUGIN_VST;
    }
    else if (strcmp(type_str, "GIG") == 0)
    {
        open_lib = false;
        type = PLUGIN_GIG;
    }
    else if (strcmp(type_str, "SF2") == 0)
    {
        open_lib = false;
        type = PLUGIN_SF2;
    }
    else if (strcmp(type_str, "SFZ") == 0)
    {
        open_lib = false;
        type = PLUGIN_SFZ;
    }
    else
    {
        DISCOVERY_OUT("error", "Invalid plugin type");
        return 1;
    }

    if (open_lib)
    {
        handle = lib_open(filename);

        if (! handle)
        {
            print_lib_error(filename);
            return 1;
        }
    }

    bool doInit = QString(filename).endsWith("dssi-vst.so", Qt::CaseInsensitive);

    switch (type)
    {
    case PLUGIN_LADSPA:
        do_ladspa_check(handle, doInit);
        break;
    case PLUGIN_DSSI:
        do_dssi_check(handle, doInit);
        break;
    case PLUGIN_LV2:
        do_lv2_check(filename);
        break;
    case PLUGIN_VST:
        do_vst_check(handle);
        break;
    case PLUGIN_GIG:
        do_linuxsampler_check(filename, "gig");
        break;
    case PLUGIN_SF2:
        do_fluidsynth_check(filename);
        break;
    case PLUGIN_SFZ:
        do_linuxsampler_check(filename, "sfz");
        break;
    default:
        break;
    }

    if (open_lib)
        lib_close(handle);

    return 0;
}

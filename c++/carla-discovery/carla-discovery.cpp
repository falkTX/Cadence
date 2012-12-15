/*
 * Carla Plugin discovery code
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#include <iostream>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

#include "carla_backend.hpp"
#include "carla_lib_utils.hpp"

#ifdef WANT_LADSPA
# include "carla_ladspa_utils.hpp"
#endif
#ifdef WANT_DSSI
# include "carla_ladspa_utils.hpp"
# include "dssi/dssi.h"
#endif
#ifdef WANT_LV2
# include "carla_lv2_utils.hpp"
#endif
#ifdef WANT_VST
# include "carla_vst_utils.hpp"
#endif
#ifdef WANT_FLUIDSYNTH
# include <fluidsynth.h>
#endif
#ifdef WANT_LINUXSAMPLER
# include "linuxsampler/EngineFactory.h"
#endif

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

// Fake values to test plugins with
const uint32_t bufferSize = 512;
const double   sampleRate = 44100.0;

// Since discovery can find multi-architecture binaries, don't print ELF/EXE related errors
void print_lib_error(const char* const filename)
{
    const char* const error = lib_error(filename);
    if (error && strstr(error, "wrong ELF class") == nullptr && strstr(error, "Bad EXE format") == nullptr)
        DISCOVERY_OUT("error", error);
}

using namespace CarlaBackend;

// ------------------------------ VST Stuff ------------------------------

#ifdef WANT_VST
bool vstWantsMidi = false;
intptr_t vstCurrentUniqueId = 0;

intptr_t vstHostCanDo(const char* const feature)
{
    qDebug("vstHostCanDo(\"%s\")", feature);

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
        return 1;
    if (strcmp(feature, "shellCategory") == 0)
        return 1;

    // unimplemented
    qWarning("vstHostCanDo(\"%s\") - unknown feature", feature);
    return 0;
}

intptr_t VSTCALLBACK vstHostCallback(AEffect* const effect, const int32_t opcode, const int32_t index, const intptr_t value, void* const ptr, const float opt)
{
#if DEBUG
    qDebug("vstHostCallback(%p, %s, %i, " P_INTPTR ", %p, %f)", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
#endif

    intptr_t ret = 0;

    switch (opcode)
    {
    case audioMasterAutomate:
        if (effect)
        {
            effect->setParameter(effect, index, opt);
            ret = 1;
        }
        break;

    case audioMasterVersion:
        ret = kVstVersion;
        break;

    case audioMasterCurrentId:
        ret = vstCurrentUniqueId;
        break;

#if ! VST_FORCE_DEPRECATED
    case audioMasterWantMidi:
        vstWantsMidi = true;
        ret = 1;
        break;
#endif

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

#if ! VST_FORCE_DEPRECATED
    case audioMasterTempoAt:
        ret = 120 * 10000;
        break;

    case audioMasterGetNumAutomatableParameters:
        ret = carla_minPositiveI(effect->numParams, MAX_PARAMETERS);
        break;

    case audioMasterGetParameterQuantization:
        ret = 1; // full single float precision
        break;
#endif

    case audioMasterGetSampleRate:
        ret = sampleRate;
        break;

    case audioMasterGetBlockSize:
        ret = bufferSize;
        break;

#if ! VST_FORCE_DEPRECATED
    case audioMasterWillReplaceOrAccumulate:
        ret = 1; // replace
        break;
#endif

    case audioMasterGetCurrentProcessLevel:
        ret = kVstProcessLevelUser;
        break;

    case audioMasterGetAutomationState:
        ret = kVstAutomationOff;
        break;

    case audioMasterGetVendorString:
        if (ptr)
        {
            strcpy((char*)ptr, "Cadence");
            ret = 1;
        }
        break;

    case audioMasterGetProductString:
        if (ptr)
        {
            strcpy((char*)ptr, "Carla-Discovery");
            ret = 1;
        }
        break;

    case audioMasterGetVendorVersion:
        ret = 0x050; // 0.5.0
        break;

    case audioMasterCanDo:
        if (ptr)
            ret = vstHostCanDo((const char*)ptr);
        break;

    case audioMasterGetLanguage:
        ret = kVstLangEnglish;
        break;

    default:
        qDebug("vstHostCallback(%p, %s, %i, " P_INTPTR ", %p, %f)", effect, vstMasterOpcode2str(opcode), index, value, ptr, opt);
        break;
    }

    return ret;
}
#endif

// ------------------------------ Plugin Checks -----------------------------

void do_ladspa_check(void* const libHandle, const bool init)
{
#ifdef WANT_LADSPA
    const LADSPA_Descriptor_Function descFn = (LADSPA_Descriptor_Function)lib_symbol(libHandle, "ladspa_descriptor");

    if (! descFn)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    unsigned long i = 0;
    const LADSPA_Descriptor* descriptor;

    while ((descriptor = descFn(i++)))
    {
        CARLA_ASSERT(descriptor->run);

        int hints = 0;
        int audioIns = 0;
        int audioOuts = 0;
        int audioTotal = 0;
        int parametersIns = 0;
        int parametersOuts = 0;
        int parametersTotal = 0;

        for (unsigned long j=0; j < descriptor->PortCount; j++)
        {
            const LADSPA_PortDescriptor portDescriptor = descriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && strcmp(descriptor->PortNames[j], "latency") && strcmp(descriptor->PortNames[j], "_latency") && strcmp(descriptor->PortNames[j], "_sample-rate"))
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

            LADSPA_Data bufferAudio[bufferSize][audioTotal];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audioTotal);

            LADSPA_Data bufferParams[parametersTotal];
            memset(bufferParams, 0, sizeof(float)*parametersTotal);

            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < descriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor portType  = descriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portHints = descriptor->PortRangeHints[j];

                if (LADSPA_IS_PORT_AUDIO(portType))
                {
                    descriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(portType))
                {
                    // min value
                    if (LADSPA_IS_HINT_BOUNDED_BELOW(portHints.HintDescriptor))
                        min = portHints.LowerBound;
                    else
                        min = 0.0f;

                    // max value
                    if (LADSPA_IS_HINT_BOUNDED_ABOVE(portHints.HintDescriptor))
                        max = portHints.UpperBound;
                    else
                        max = 1.0f;

                    if (min > max)
                        max = min;
                    else if (max < min)
                        min = max;

                    if (max - min == 0.0f)
                    {
                        DISCOVERY_OUT("error", "Broken parameter: max - min == 0");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portHints.HintDescriptor, min, max);

                    if (def < min)
                        def = min;
                    else if (def > max)
                        def = max;

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portHints.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(portType) && (strcmp(descriptor->PortNames[j], "latency") == 0 || strcmp(descriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
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
#else
    DISCOVERY_OUT("error", "LADSPA support not available");
    Q_UNUSED(libHandle);
    Q_UNUSED(init);
#endif
}

void do_dssi_check(void* const libHandle, const bool init)
{
#ifdef WANT_DSSI
    const DSSI_Descriptor_Function descFn = (DSSI_Descriptor_Function)lib_symbol(libHandle, "dssi_descriptor");

    if (! descFn)
    {
        DISCOVERY_OUT("error", "Not a DSSI plugin");
        return;
    }

    unsigned long i = 0;
    const DSSI_Descriptor* descriptor;

    while ((descriptor = descFn(i++)))
    {
        const LADSPA_Descriptor* const ldescriptor = descriptor->LADSPA_Plugin;
        CARLA_ASSERT(ldescriptor);
        CARLA_ASSERT(ldescriptor->run || descriptor->run_synth || descriptor->run_multiple_synths);

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
            const LADSPA_PortDescriptor portDescriptor = ldescriptor->PortDescriptors[j];

            if (LADSPA_IS_PORT_AUDIO(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    audioIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LADSPA_IS_PORT_CONTROL(portDescriptor))
            {
                if (LADSPA_IS_PORT_INPUT(portDescriptor))
                    parametersIns += 1;
                else if (LADSPA_IS_PORT_OUTPUT(portDescriptor) && strcmp(ldescriptor->PortNames[j], "latency") && strcmp(ldescriptor->PortNames[j], "_latency") && strcmp(ldescriptor->PortNames[j], "_sample-rate"))
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

            // we can only get program info per-handle
            if (descriptor->get_program)
            {
                while ((descriptor->get_program(handle, programsTotal++)))
                    continue;
            }

            LADSPA_Data bufferAudio[bufferSize][audioTotal];
            memset(bufferAudio, 0, sizeof(float)*bufferSize*audioTotal);

            LADSPA_Data bufferParams[parametersTotal];
            memset(bufferParams, 0, sizeof(float)*parametersTotal);

            LADSPA_Data min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < ldescriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor portType  = ldescriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  portHints = ldescriptor->PortRangeHints[j];

                if (LADSPA_IS_PORT_AUDIO(portType))
                {
                    ldescriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (LADSPA_IS_PORT_CONTROL(portType))
                {
                    // min value
                    if (LADSPA_IS_HINT_BOUNDED_BELOW(portHints.HintDescriptor))
                        min = portHints.LowerBound;
                    else
                        min = 0.0f;

                    // max value
                    if (LADSPA_IS_HINT_BOUNDED_ABOVE(portHints.HintDescriptor))
                        max = portHints.UpperBound;
                    else
                        max = 1.0f;

                    if (min > max)
                        max = min;
                    else if (max < min)
                        min = max;

                    if (max - min == 0.0f)
                    {
                        DISCOVERY_OUT("error", "Broken parameter: max - min == 0");
                        max = min + 0.1f;
                    }

                    // default value
                    def = get_default_ladspa_port_value(portHints.HintDescriptor, min, max);

                    if (def < min)
                        def = min;
                    else if (def > max)
                        def = max;

                    if (LADSPA_IS_HINT_SAMPLE_RATE(portHints.HintDescriptor))
                    {
                        min *= sampleRate;
                        max *= sampleRate;
                        def *= sampleRate;
                    }

                    if (LADSPA_IS_PORT_OUTPUT(portType) && (strcmp(ldescriptor->PortNames[j], "latency") == 0 || strcmp(ldescriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0.0f;
                        max = sampleRate;
                        def = 0.0f;
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

                if (descriptor->run_multiple_synths && ! descriptor->run_synth)
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
#else
    DISCOVERY_OUT("error", "DSSI support not available");
    Q_UNUSED(libHandle);
    Q_UNUSED(init);
#endif
}

void do_lv2_check(const char* const bundle, const bool init)
{
#ifdef WANT_LV2
    // Convert bundle filename to URI
    QString qBundle(QUrl::fromLocalFile(bundle).toString());
    if (! qBundle.endsWith(QDir::separator()))
        qBundle += QDir::separator();

    // Load bundle
    Lilv::Node lilvBundle(lv2World.new_uri(qBundle.toUtf8().constData()));
    lv2World.load_bundle(lilvBundle);

    // Load plugins in this bundle
    const Lilv::Plugins lilvPlugins = lv2World.get_all_plugins();

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
        CARLA_ASSERT(rdf_descriptor && rdf_descriptor->URI);

        if (init)
        {
            // test if DLL is loadable
            void* const libHandle = lib_open(rdf_descriptor->Binary);

            if (! libHandle)
            {
                print_lib_error(rdf_descriptor->Binary);
                continue;
            }

            lib_close(libHandle);

            // test if we support all required ports and features
            bool supported = true;

            for (uint32_t j=0; j < rdf_descriptor->PortCount; j++)
            {
                const LV2_RDF_Port* const port = &rdf_descriptor->Ports[j];
                bool validPort = (LV2_IS_PORT_CONTROL(port->Type) || LV2_IS_PORT_AUDIO(port->Type) || LV2_IS_PORT_ATOM_SEQUENCE(port->Type) /*|| LV2_IS_PORT_CV(port->Type)*/ || LV2_IS_PORT_EVENT(port->Type) || LV2_IS_PORT_MIDI_LL(port->Type));

                if (! (validPort || LV2_IS_PORT_OPTIONAL(port->Properties)))
                {
                    DISCOVERY_OUT("error", "plugin requires a non-supported port type, port-name: " << port->Name);
                    supported = false;
                    break;
                }
            }

            for (uint32_t j=0; j < rdf_descriptor->FeatureCount && supported; j++)
            {
                const LV2_RDF_Feature* const feature = &rdf_descriptor->Features[j];

                if (LV2_IS_FEATURE_REQUIRED(feature->Type) && ! is_lv2_feature_supported(feature->URI))
                {
                    DISCOVERY_OUT("error", "plugin requires a non-supported feature " << feature->URI);
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
            const LV2_RDF_Port* const port = &rdf_descriptor->Ports[j];

            if (LV2_IS_PORT_AUDIO(port->Type))
            {
                if (LV2_IS_PORT_INPUT(port->Type))
                    audioIns += 1;
                else if (LV2_IS_PORT_OUTPUT(port->Type))
                    audioOuts += 1;
                audioTotal += 1;
            }
            else if (LV2_IS_PORT_CONTROL(port->Type))
            {
                if (LV2_IS_PORT_DESIGNATION_LATENCY(port->Designation) || LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(port->Designation) ||
                    LV2_IS_PORT_DESIGNATION_FREEWHEELING(port->Designation) || LV2_IS_PORT_DESIGNATION_TIME(port->Designation))
                {
                    pass();
                }
                else
                {
                    if (LV2_IS_PORT_INPUT(port->Type))
                        parametersIns += 1;
                    else if (LV2_IS_PORT_OUTPUT(port->Type))
                        parametersOuts += 1;
                    parametersTotal += 1;
                }
            }
            else if (port->Type & LV2_PORT_SUPPORTS_MIDI_EVENT)
            {
                if (LV2_IS_PORT_INPUT(port->Type))
                    midiIns += 1;
                else if (LV2_IS_PORT_OUTPUT(port->Type))
                    midiOuts += 1;
                midiTotal += 1;
            }
        }

        if (rdf_descriptor->Type & LV2_CLASS_INSTRUMENT)
            hints |= PLUGIN_IS_SYNTH;

        if (rdf_descriptor->UICount > 0)
            hints |= PLUGIN_HAS_GUI;

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("label", rdf_descriptor->URI);
        if (rdf_descriptor->Name)
            DISCOVERY_OUT("name", rdf_descriptor->Name);
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
#else
    DISCOVERY_OUT("error", "LV2 support not available");
    Q_UNUSED(bundle);
    Q_UNUSED(init);
#endif
}

void do_vst_check(void* const libHandle, const bool init)
{
#ifdef WANT_VST
    VST_Function vstFn = (VST_Function)lib_symbol(libHandle, "VSTPluginMain");

    if (! vstFn)
        vstFn = (VST_Function)lib_symbol(libHandle, "main");

    if (! vstFn)
    {
        DISCOVERY_OUT("error", "Not a VST plugin");
        return;
    }

    AEffect* const effect = vstFn(vstHostCallback);

    if (! (effect && effect->magic == kEffectMagic))
    {
        DISCOVERY_OUT("error", "Failed to init VST plugin, or VST magic failed");
        return;
    }

    const char* cName;
    const char* cProduct;
    const char* cVendor;
    char strBuf[255] = { 0 };

    effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

    effect->dispatcher(effect, effGetEffectName, 0, 0, strBuf, 0.0f);
    cName = strdup((strBuf[0] != 0) ? strBuf : "");

    strBuf[0] = 0;
    effect->dispatcher(effect, effGetProductString, 0, 0, strBuf, 0.0f);
    cProduct = strdup((strBuf[0] != 0) ? strBuf : "");

    strBuf[0] = 0;
    effect->dispatcher(effect, effGetVendorString, 0, 0, strBuf, 0.0f);
    cVendor = strdup((strBuf[0] != 0) ? strBuf : "");

    vstCurrentUniqueId = effect->uniqueID;
    intptr_t vstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

    while (true)
    {
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

        if (vstPluginCanDo(effect, "receiveVstEvents") || vstPluginCanDo(effect, "receiveVstMidiEvent") || vstWantsMidi || (effect->flags & effFlagsIsSynth) > 0)
            midiIns = 1;

        if (vstPluginCanDo(effect, "sendVstEvents") || vstPluginCanDo(effect, "sendVstMidiEvent"))
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

#if ! VST_FORCE_DEPRECATED
            if ((effect->flags & effFlagsCanReplacing) > 0 && effect->processReplacing != effect->process)
                effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
            else
                effect->process(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#else
            CARLA_ASSERT(effect->flags & effFlagsCanReplacing);
            if (effect->flags & effFlagsCanReplacing)
                effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
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

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", cName);
        DISCOVERY_OUT("label", cProduct);
        DISCOVERY_OUT("maker", cVendor);
        DISCOVERY_OUT("copyright", cVendor);
        DISCOVERY_OUT("unique_id", vstCurrentUniqueId);
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

        if (vstCategory != kPlugCategShell)
            break;

        strBuf[0] = 0;
        vstWantsMidi = false;
        vstCurrentUniqueId = effect->dispatcher(effect, effShellGetNextPlugin, 0, 0, strBuf, 0.0f);

        if (vstCurrentUniqueId != 0)
        {
            free((void*)cName);
            cName = strdup((strBuf[0] != 0) ? strBuf : "");
        }
        else
            break;
    }

    effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);

    free((void*)cName);
    free((void*)cProduct);
    free((void*)cVendor);
#else
    DISCOVERY_OUT("error", "VST support not available");
    Q_UNUSED(libHandle);
    Q_UNUSED(init);
#endif
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
    DISCOVERY_OUT("error", "SF2 support not available");
    Q_UNUSED(filename);
    Q_UNUSED(init);
#endif
}

void do_linuxsampler_check(const char* const filename, const char* const stype, const bool init)
{
#ifdef WANT_LINUXSAMPLER
    const QFileInfo file(filename);

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
        LinuxSamplerScopedEngine(const char* const filename, const char* const stype)
        {
            engine = nullptr;

            try {
                engine = EngineFactory::Create(stype);
            }
            catch (const Exception& e)
            {
                DISCOVERY_OUT("error", e.what());
                return;
            }

            if (! engine)
                return;

            ins = engine->GetInstrumentManager();

            if (! ins)
            {
                DISCOVERY_OUT("error", "Failed to get LinuxSampeler instrument manager");
                return;
            }

            std::vector<InstrumentManager::instrument_id_t> ids;

            try {
                ids = ins->GetInstrumentFileContent(filename);
            }
            catch (const Exception& e)
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

        static void outputInfo(InstrumentManager::instrument_info_t* const info, const int programs, const char* const basename = nullptr)
        {
            DISCOVERY_OUT("init", "-----------");

            if (info)
            {
                DISCOVERY_OUT("name", info->InstrumentName);
                DISCOVERY_OUT("label", info->Product);
                DISCOVERY_OUT("maker", info->Artists);
                DISCOVERY_OUT("copyright", info->Artists);
            }
            else
            {
                DISCOVERY_OUT("name", basename);
                DISCOVERY_OUT("label", basename);
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
        LinuxSamplerScopedEngine::outputInfo(nullptr, 0, file.baseName().toUtf8().constData());
#else
    DISCOVERY_OUT("error", stype << " support not available");
    Q_UNUSED(filename);
    Q_UNUSED(init);
#endif
}

// ------------------------------ main entry point ------------------------------

int main(int argc, char* argv[])
{
    if (argc == 2 && strcmp(argv[1], "-formats") == 0)
    {
        printf("Available plugin formats:\n");
        printf("LADSPA: ");
#ifdef WANT_LADSPA
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("DSSI:   ");
#ifdef WANT_DSSI
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("LV2:    ");
#ifdef WANT_LV2
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("VST:    ");
#ifdef WANT_VST
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("\n");

        printf("Available sampler formats:\n");
        printf("GIG (LinuxSampler): ");
#ifdef WANT_LINUXSAMPLER
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("SF2 (FluidSynth):   ");
#ifdef WANT_FLUIDSYNTH
        printf("yes\n");
#else
        printf("no\n");
#endif
        printf("SFZ (LinuxSampler): ");
#ifdef WANT_LINUXSAMPLER
        printf("yes\n");
#else
        printf("no\n");
#endif
        return 0;
    }

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

    if (doInit && getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"))
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

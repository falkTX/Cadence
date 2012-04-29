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

#if defined (__GXX_EXPERIMENTAL_CXX0X__) && defined (__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
// nullptr is available
#else
#define nullptr (0)
#endif

#if defined(__WIN32__) || defined(__WIN64__)
#include <windows.h>
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#else
#include <dlfcn.h>
#ifndef __cdecl
#define __cdecl
#endif
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "ladspa/ladspa.h"
#include "dssi/dssi.h"

#ifdef WANT_LILV
#include "lv2/atom.h"
#include "lv2/event.h"
#include "lv2/midi.h"
#include "lilv/lilvmm.hpp"
#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"
#endif

#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"

#if VESTIGE_HEADER
#warning Using vestige header
#define kVstVersion 2400
#define effGetPlugCategory 35
#define effSetBlockSizeAndSampleRate 43
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kVstProcessPrecision32 0
#define kPlugCategUnknown 0
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#endif

#ifdef WANT_FLUIDSYNTH
#include <fluidsynth.h>
#endif

#define DISCOVERY_OUT(x, y) std::cout << "\ncarla-discovery::" << x << "::" << y << std::endl;

// fake values to test plugins with
const unsigned int bufferSize = 512;
const unsigned int sampleRate = 44100;

// ------------------------------ Carla main defs ------------------------------

// plugin hints
const unsigned int PLUGIN_HAS_GUI     = 0x01;
const unsigned int PLUGIN_IS_BRIDGE   = 0x02;
const unsigned int PLUGIN_IS_SYNTH    = 0x04;
const unsigned int PLUGIN_USES_CHUNKS = 0x08;
const unsigned int PLUGIN_CAN_DRYWET  = 0x10;
const unsigned int PLUGIN_CAN_VOLUME  = 0x20;
const unsigned int PLUGIN_CAN_BALANCE = 0x40;

enum BinaryType {
    BINARY_NONE   = 0,
    BINARY_UNIX32 = 1,
    BINARY_UNIX64 = 2,
    BINARY_WIN32  = 3,
    BINARY_WIN64  = 4
};

enum PluginType {
    PLUGIN_NONE   = 0,
    PLUGIN_LADSPA = 1,
    PLUGIN_DSSI   = 2,
    PLUGIN_LV2    = 3,
    PLUGIN_VST    = 4,
    PLUGIN_SF2    = 5
};

enum PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0,
    PLUGIN_CATEGORY_SYNTH     = 1,
    PLUGIN_CATEGORY_DELAY     = 2, // also Reverb
    PLUGIN_CATEGORY_EQ        = 3,
    PLUGIN_CATEGORY_FILTER    = 4,
    PLUGIN_CATEGORY_DYNAMICS  = 5, // Amplifier, Compressor, Gate
    PLUGIN_CATEGORY_MODULATOR = 6, // Chorus, Flanger, Phaser
    PLUGIN_CATEGORY_UTILITY   = 7, // Analyzer, Converter, Mixer
    PLUGIN_CATEGORY_OUTRO     = 8  // used to check if a plugin has a category
};

#if BUILD_UNIX32
#  define BINARY_TYPE BINARY_UNIX32
#elif  BUILD_UNIX64
#  define BINARY_TYPE BINARY_UNIX64
#elif  BUILD_WIN32
#  define BINARY_TYPE BINARY_WIN32
#elif  BUILD_WIN64
#  define BINARY_TYPE BINARY_WIN64
#else
#  error Invalid build type
#endif

// ------------------------------ library functions ------------------------------
void* lib_open(const char* filename)
{
#ifdef __WINDOWS__
    return LoadLibraryA(filename);
#else
    return dlopen(filename, RTLD_LAZY);
#endif
}

int lib_close(void* lib)
{
#ifdef __WINDOWS__
    return FreeLibrary((HMODULE)lib);
#else
    return dlclose(lib);
#endif
}

void* lib_symbol(void* lib, const char* symbol)
{
#ifdef __WINDOWS__
    return (void*)GetProcAddress((HMODULE)lib, symbol);
#else
    return dlsym(lib, symbol);
#endif
}

const char* lib_error(const char* filename)
{
#ifdef __WINDOWS__
    static char libError[2048];
    memset(libError, 0, sizeof(char)*2048);

    LPVOID winErrorString;
    DWORD  winErrorCode = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

    snprintf(libError, 2048, "%s: error code %i: %s", filename, winErrorCode, (const char*)winErrorString);
    LocalFree(winErrorString);

    return libError;
#else
    (void)filename;
    return dlerror();
#endif
}

// ------------------------------ VST Stuff ------------------------------
typedef AEffect* (*VST_Function)(audioMasterCallback);

intptr_t VstCurrentUniqueId = 0;

bool VstPluginCanDo(AEffect* effect, const char* feature)
{
    return (effect->dispatcher(effect, effCanDo, 0, 0, (void*)feature, 0.0f) == 1);
}

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
#if DEBUG
    std::cout << "VstHostCallback(" << effect << ", " << opcode << ", " << index << ", " << value << ", " << ptr << ", " << opt << ")" << std::endl;
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
        static VstTimeInfo timeInfo;
        memset(&timeInfo, 0, sizeof(VstTimeInfo));
        timeInfo.sampleRate = sampleRate;
        return (intptr_t)&timeInfo;

    case audioMasterGetSampleRate:
        return sampleRate;

    case audioMasterGetBlockSize:
        return bufferSize;

    case audioMasterGetVendorString:
        if (ptr)
            strcpy((char*)ptr, "falkTX");
        break;

    case audioMasterGetProductString:
        if (ptr)
            strcpy((char*)ptr, "Carla-Discovery");
        break;

    case audioMasterGetVendorVersion:
        return 0x05; // 0.5

    case audioMasterCanDo:
#if DEBUG
        std::cout << "VstHostCallback:audioMasterCanDo - " << (char*)ptr << std::endl;
#endif

        if (strcmp((char*)ptr, "sendVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
            return 1;
#if !VST_FORCE_DEPRECATED
        else if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
            return -1;
#endif
        else if (strcmp((char*)ptr, "reportConnectionChanges") == 0)
            return 1;
        else if (strcmp((char*)ptr, "acceptIOChanges") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sizeWindow") == 0)
            return 1;
        else if (strcmp((char*)ptr, "offline") == 0)
            return -1;
        else if (strcmp((char*)ptr, "openFileSelector") == 0)
            return -1;
        else if (strcmp((char*)ptr, "closeFileSelector") == 0)
            return -1;
        else if (strcmp((char*)ptr, "startStopProcess") == 0)
            return 1;
        else if (strcmp((char*)ptr, "shellCategory") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime") == 0)
            return -1;
        else
        {
            std::cerr << "VstHostCallback:audioMasterCanDo - Got uninplemented feature request '" << (char*)ptr << "'" << std::endl;
            return 0;
        }

    case audioMasterGetLanguage:
        return kVstLangEnglish;

    default:
        break;
    }

    return 0;

    (void)value;
}

// ------------------------------ Plugin Check ------------------------------
void do_ladspa_check(void* lib_handle)
{
    LADSPA_Descriptor_Function descfn = (LADSPA_Descriptor_Function)lib_symbol(lib_handle, "ladspa_descriptor");

    if (descfn == nullptr)
    {
        DISCOVERY_OUT("error", "Not a LADSPA plugin");
        return;
    }

    unsigned long i = 0;
    const LADSPA_Descriptor* descriptor;

    while ((descriptor = descfn(i++)))
    {
        LADSPA_Handle handle = descriptor->instantiate(descriptor, sampleRate);

        if (handle)
        {
            int hints = 0;
            PluginCategory category = PLUGIN_CATEGORY_NONE;

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

            for (unsigned long j=0; j < descriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortDescriptor = descriptor->PortDescriptors[j];
                if (PortDescriptor & LADSPA_PORT_AUDIO)
                {
                    if (PortDescriptor & LADSPA_PORT_INPUT)
                        audio_ins += 1;
                    else if (PortDescriptor & LADSPA_PORT_OUTPUT)
                        audio_outs += 1;
                    audio_total += 1;
                }
                else if (PortDescriptor & LADSPA_PORT_CONTROL)
                {
                    if (PortDescriptor & LADSPA_PORT_INPUT)
                        parameters_ins += 1;
                    else if (PortDescriptor & LADSPA_PORT_OUTPUT)
                    {
                        if (strcmp(descriptor->PortNames[j], "latency") != 0 && strcmp(descriptor->PortNames[j], "_latency") != 0)
                            parameters_outs += 1;
                    }
                    parameters_total += 1;
                }
            }

            // small crash-free plugin test
            float bufferAudio[bufferSize][audio_total];
            memset(&bufferAudio, 0, sizeof(float)*bufferSize*audio_total);

            float bufferParams[parameters_total];
            memset(&bufferParams, 0, sizeof(float)*parameters_total);

            float min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < descriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortType = descriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  PortHint = descriptor->PortRangeHints[j];

                if (PortType & LADSPA_PORT_AUDIO)
                {
                    descriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (PortType & LADSPA_PORT_CONTROL)
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

                    if (LADSPA_IS_PORT_OUTPUT(PortType) && (strcmp(descriptor->PortNames[j], "latency") == 0 || strcmp(descriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0;
                        max = sampleRate;
                        def = 0;
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

                    bufferParams[iP] = def;

                    descriptor->connect_port(handle, j, &bufferParams[iP++]);
                }
            }

            if (descriptor->activate)
                descriptor->activate(handle);

            descriptor->run(handle, bufferSize);

            if (descriptor->deactivate)
                descriptor->deactivate(handle);

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", descriptor->Name);
            DISCOVERY_OUT("label", descriptor->Label);
            DISCOVERY_OUT("maker", descriptor->Maker);
            DISCOVERY_OUT("copyright", descriptor->Copyright);
            DISCOVERY_OUT("unique_id", descriptor->UniqueID);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("category", category);
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

            if (descriptor->cleanup)
                descriptor->cleanup(handle);

            DISCOVERY_OUT("build", BINARY_TYPE);
            DISCOVERY_OUT("end", "------------");
        }
        else
            DISCOVERY_OUT("error", "Failed to init LADSPA plugin");
    }
}

void do_dssi_check(void* lib_handle)
{
    DSSI_Descriptor_Function descfn = (DSSI_Descriptor_Function)lib_symbol(lib_handle, "dssi_descriptor");

    if (descfn == nullptr)
    {
        DISCOVERY_OUT("error", "Not a DSSI plugin");
        return;
    }

    unsigned long i = 0;
    const DSSI_Descriptor* descriptor;

    while ((descriptor = descfn(i++)))
    {
        const LADSPA_Descriptor* ldescriptor = descriptor->LADSPA_Plugin;
        LADSPA_Handle handle = ldescriptor->instantiate(ldescriptor, sampleRate);

        if (handle)
        {
            int hints = 0;
            PluginCategory category = PLUGIN_CATEGORY_NONE;

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

            for (unsigned long j=0; j < ldescriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortDescriptor = ldescriptor->PortDescriptors[j];
                if (PortDescriptor & LADSPA_PORT_AUDIO)
                {
                    if (PortDescriptor & LADSPA_PORT_INPUT)
                        audio_ins += 1;
                    else if (PortDescriptor & LADSPA_PORT_OUTPUT)
                        audio_outs += 1;
                    audio_total += 1;
                }
                else if (PortDescriptor & LADSPA_PORT_CONTROL)
                {
                    if (PortDescriptor & LADSPA_PORT_INPUT)
                        parameters_ins += 1;
                    else if (PortDescriptor & LADSPA_PORT_OUTPUT)
                    {
                        if (strcmp(ldescriptor->PortNames[j], "latency") != 0 && strcmp(ldescriptor->PortNames[j], "_latency") != 0)
                            parameters_outs += 1;
                    }
                    parameters_total += 1;
                }
            }

            if (descriptor->run_synth || descriptor->run_multiple_synths)
            {
                midi_ins = 1;
                midi_total = 1;
            }

            if (midi_ins > 0 && audio_outs > 0)
                hints |= PLUGIN_IS_SYNTH;

            if (descriptor->get_program)
            {
                while ((descriptor->get_program(handle, programs_total)))
                    programs_total += 1;
            }

            // small crash-free plugin test
            float bufferAudio[bufferSize][audio_total];
            memset(&bufferAudio, 0, sizeof(float)*bufferSize*audio_total);

            float bufferParams[parameters_total];
            memset(&bufferParams, 0, sizeof(float)*parameters_total);

            float min, max, def;

            for (unsigned long j=0, iA=0, iP=0; j < ldescriptor->PortCount; j++)
            {
                const LADSPA_PortDescriptor PortType = ldescriptor->PortDescriptors[j];
                const LADSPA_PortRangeHint  PortHint = ldescriptor->PortRangeHints[j];

                if (PortType & LADSPA_PORT_AUDIO)
                {
                    ldescriptor->connect_port(handle, j, bufferAudio[iA++]);
                }
                else if (PortType & LADSPA_PORT_CONTROL)
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

                    if (LADSPA_IS_PORT_OUTPUT(PortType) && (strcmp(ldescriptor->PortNames[j], "latency") == 0 || strcmp(ldescriptor->PortNames[j], "_latency") == 0))
                    {
                        // latency parameter
                        min = 0;
                        max = sampleRate;
                        def = 0;
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

                    bufferParams[iP] = def;

                    ldescriptor->connect_port(handle, j, &bufferParams[iP++]);
                }
            }

            snd_seq_event_t midiEvents[2];
            memset(&midiEvents, 0, sizeof(snd_seq_event_t)*2);

            unsigned long midiEventCount = 2;

            midiEvents[0].type = SND_SEQ_EVENT_NOTEON;
            midiEvents[0].data.note.note     = 64;
            midiEvents[0].data.note.velocity = 100;

            midiEvents[1].type = SND_SEQ_EVENT_NOTEOFF;
            midiEvents[1].data.note.note     = 64;
            midiEvents[1].data.note.velocity = 0;
            midiEvents[1].time.tick = bufferSize/2;

            if (ldescriptor->activate)
                ldescriptor->activate(handle);

            if (descriptor->run_synth)
            {
                descriptor->run_synth(handle, bufferSize, midiEvents, midiEventCount);
            }
            else if (descriptor->run_multiple_synths)
            {
                snd_seq_event_t* midiEventsPtr[] = { midiEvents, nullptr };
                descriptor->run_multiple_synths(1, &handle, bufferSize, midiEventsPtr, &midiEventCount);
            }
            else
                ldescriptor->run(handle, bufferSize);

            if (ldescriptor->deactivate)
                ldescriptor->deactivate(handle);

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", ldescriptor->Name);
            DISCOVERY_OUT("label", ldescriptor->Label);
            DISCOVERY_OUT("maker", ldescriptor->Maker);
            DISCOVERY_OUT("copyright", ldescriptor->Copyright);
            DISCOVERY_OUT("unique_id", ldescriptor->UniqueID);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("category", category);
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

            if (ldescriptor->cleanup)
                ldescriptor->cleanup(handle);

            DISCOVERY_OUT("build", BINARY_TYPE);
            DISCOVERY_OUT("end", "------------");
        }
        else
            DISCOVERY_OUT("error", "Failed to init DSSI plugin");
    }
}

void do_lv2_check(const char* bundle)
{
#ifdef WANT_LILV
    std::string sbundle;
    sbundle += "file://";
    sbundle += bundle;
#ifdef __WINDOWS__
    sbundle += "\\";
#else
    sbundle += "/";
#endif

    Lilv::World World;
    Lilv::Node Bundle(lilv_new_uri(World.me, sbundle.c_str()));
    World.load_bundle(Bundle);

    Lilv::Node AtomBufferTypes  = Lilv::Node(lilv_new_uri(World.me, LV2_ATOM__bufferType));
    Lilv::Node EventTypeMidi    = Lilv::Node(lilv_new_uri(World.me, LV2_MIDI__MidiEvent));

    Lilv::Node PortTypeInput    = Lilv::Node(lilv_new_uri(World.me, LV2_CORE__InputPort));
    Lilv::Node PortTypeOutput   = Lilv::Node(lilv_new_uri(World.me, LV2_CORE__OutputPort));
    Lilv::Node PortTypeAudio    = Lilv::Node(lilv_new_uri(World.me, LV2_CORE__AudioPort));
    Lilv::Node PortTypeControl  = Lilv::Node(lilv_new_uri(World.me, LV2_CORE__ControlPort));
    Lilv::Node PortTypeAtom     = Lilv::Node(lilv_new_uri(World.me, LV2_ATOM__AtomPort));
    Lilv::Node PortTypeEvent    = Lilv::Node(lilv_new_uri(World.me, LV2_EVENT__EventPort));
    Lilv::Node PortTypeMidiLL   = Lilv::Node(lilv_new_uri(World.me, LV2_MIDI_LL__MidiPort));

    Lilv::Node PortPropertyLatency = Lilv::Node(lilv_new_uri(World.me, LV2_CORE__latency));

    const Lilv::Plugins Plugins = World.get_all_plugins();

    LILV_FOREACH(plugins, i, Plugins)
    {
        Lilv::Plugin p(lilv_plugins_get(Plugins, i));

        //Lilv::Nodes requiredFeatures(p.get_required_features());
        // check

        const char* filename = lilv_uri_to_path(p.get_library_uri().as_string());

        // test if DLL is loadable
        void* lib_handle = lib_open(filename);

        if (lib_handle == nullptr)
        {
            DISCOVERY_OUT("error", lib_error(filename));
            continue;
        }

        lib_close(lib_handle);

        int hints = 0;
        PluginCategory category = PLUGIN_CATEGORY_NONE;

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

        for (unsigned j=0; j < p.get_num_ports(); j++)
        {
            Lilv::Port Port = p.get_port_by_index(j);

            if (Port.is_a(PortTypeAudio))
            {
                if (Port.is_a(PortTypeInput))
                    audio_ins += 1;
                else if (Port.is_a(PortTypeOutput))
                    audio_outs += 1;
                audio_total += 1;
            }
            else if (Port.is_a(PortTypeControl))
            {
                if (Port.is_a(PortTypeInput))
                    parameters_ins += 1;
                else if (Port.is_a(PortTypeOutput))
                {
                    if (Port.has_property(PortPropertyLatency) == false)
                        parameters_outs += 1;
                }
                parameters_total += 1;
            }
            else if (Port.is_a(PortTypeAtom))
            {
                Lilv::Nodes bufferTypes(Port.get_value(AtomBufferTypes));
                if (bufferTypes.contains(EventTypeMidi))
                {
                    if (Port.is_a(PortTypeInput))
                        midi_ins += 1;
                    else if (Port.is_a(PortTypeOutput))
                        midi_outs += 1;
                    midi_total += 1;
                }
            }
            else if (Port.is_a(PortTypeEvent))
            {
                if (Port.supports_event(EventTypeMidi))
                {
                    if (Port.is_a(PortTypeInput))
                        midi_ins += 1;
                    else if (Port.is_a(PortTypeOutput))
                        midi_outs += 1;
                    midi_total += 1;
                }
            }
            else if (Port.is_a(PortTypeMidiLL))
            {
                if (Port.is_a(PortTypeInput))
                    midi_ins += 1;
                else if (Port.is_a(PortTypeOutput))
                    midi_outs += 1;
                midi_total += 1;
            }
        }

        DISCOVERY_OUT("init", "-----------");
        DISCOVERY_OUT("name", p.get_name().as_string());
        DISCOVERY_OUT("label", p.get_uri().as_string());
        if (p.get_author_name())
          DISCOVERY_OUT("maker", p.get_author_name().as_string());
        //DISCOVERY_OUT("copyright", ldescriptor->Copyright);
        //DISCOVERY_OUT("unique_id", ldescriptor->UniqueID);
        DISCOVERY_OUT("hints", hints);
        DISCOVERY_OUT("category", category);
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

        DISCOVERY_OUT("build", BINARY_TYPE);
        DISCOVERY_OUT("end", "------------");
    }
#else
    (void)bundle;
    DISCOVERY_OUT("error", "LV2 support not available");
#endif
}

void do_vst_check(void* lib_handle)
{
    VST_Function vstfn = (VST_Function)lib_symbol(lib_handle, "VSTPluginMain");

    if (vstfn == nullptr)
    {
        if (vstfn == nullptr)
        {
#ifdef TARGET_API_MAC_CARBON

            vstfn = (VST_Function)lib_symbol(lib_handle, "main_macho");
            if (vstfn == nullptr)
#endif
                vstfn = (VST_Function)lib_symbol(lib_handle, "main");
        }
    }

    if (vstfn == nullptr)
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
            PluginCategory category = PLUGIN_CATEGORY_NONE;

            switch (VstCategory)
            {
            case kPlugCategUnknown:
                category = PLUGIN_CATEGORY_NONE;
                break;
            case kPlugCategSynth:
                category = PLUGIN_CATEGORY_SYNTH;
                break;
            case kPlugCategAnalysis:
                category = PLUGIN_CATEGORY_UTILITY;
                break;
            case kPlugCategMastering:
                category = PLUGIN_CATEGORY_DYNAMICS;
                break;
            case kPlugCategRoomFx:
                category = PLUGIN_CATEGORY_DELAY;
                break;
            case kPlugCategRestoration:
                category = PLUGIN_CATEGORY_UTILITY;
                break;
            case kPlugCategGenerator:
                category = PLUGIN_CATEGORY_SYNTH;
                break;
            default:
                category = PLUGIN_CATEGORY_OUTRO;
            }

            if (effect->flags & effFlagsHasEditor)
                hints |= PLUGIN_HAS_GUI;

            if (effect->flags & effFlagsIsSynth)
            {
                hints |= PLUGIN_IS_SYNTH;

                if (category == PLUGIN_CATEGORY_NONE)
                    category = PLUGIN_CATEGORY_SYNTH;
            }

            int audio_ins = effect->numInputs;
            int audio_outs = effect->numOutputs;
            int audio_total = audio_ins + audio_outs;
            int midi_ins = 0;
            int midi_outs = 0;
            int midi_total = 0;
            int parameters_ins = effect->numParams;
            int parameters_outs = 0;
            int parameters_total = parameters_ins;
            int programs_total = effect->numPrograms;

            if (VstPluginCanDo(effect, "receiveVstEvents") || VstPluginCanDo(effect, "receiveVstMidiEvent") || effect->flags & effFlagsIsSynth)
                midi_ins = 1;

            if (VstPluginCanDo(effect, "sendVstEvents") || VstPluginCanDo(effect, "sendVstMidiEvent"))
                midi_outs = 1;

            midi_total = midi_ins + midi_outs;

            // small crash-free plugin test
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
            memset(&midiEvents, 0, sizeof(VstMidiEvent)*2);

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

#if !VST_FORCE_DEPRECATED
            effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, bufferSize, nullptr, sampleRate);
#endif
            effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, sampleRate);
            effect->dispatcher(effect, effSetBlockSize, 0, bufferSize, nullptr, 0.0f);
            effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);

            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);

            if (midi_ins == 1)
                effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);

            if (effect->flags & effFlagsCanReplacing)
                effect->processReplacing(effect, bufferAudioIn, bufferAudioOut, bufferSize);
#if !VST_FORCE_DEPRECATED
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

            DISCOVERY_OUT("init", "-----------");
            DISCOVERY_OUT("name", c_name);
            DISCOVERY_OUT("label", c_product);
            DISCOVERY_OUT("maker", c_vendor);
            DISCOVERY_OUT("copyright", c_vendor);
            DISCOVERY_OUT("unique_id", VstCurrentUniqueId);
            DISCOVERY_OUT("hints", hints);
            DISCOVERY_OUT("category", category);
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

            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);

            DISCOVERY_OUT("build", BINARY_TYPE);
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

void do_sf2_check(const char* filename)
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
            DISCOVERY_OUT("unique_id", 0);

            DISCOVERY_OUT("hints", 0);
            DISCOVERY_OUT("category", PLUGIN_CATEGORY_SYNTH);
            DISCOVERY_OUT("audio.ins", 0);
            DISCOVERY_OUT("audio.outs", 2);
            DISCOVERY_OUT("audio.total", 2);
            DISCOVERY_OUT("midi.ins", 1);
            DISCOVERY_OUT("midi.outs", 0);
            DISCOVERY_OUT("midi.total", 1);
            DISCOVERY_OUT("programs.total", programs);

            // defined in Carla
            DISCOVERY_OUT("parameters.ins", 13);
            DISCOVERY_OUT("parameters.outs", 1);
            DISCOVERY_OUT("parameters.total", 14);

            DISCOVERY_OUT("build", BINARY_TYPE);
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

// ------------------------------ main entry point ------------------------------
int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    const char* type_str = argv[1];
    const char* filename = argv[2];

    bool open_lib;
    PluginType type;
    void* handle = 0;

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
    else if (strcmp(type_str, "SF2") == 0)
    {
        open_lib = false;
        type = PLUGIN_SF2;
    }
    else
    {
        DISCOVERY_OUT("error", "Invalid plugin type");
        return 1;
    }

    if (open_lib)
    {
        handle = lib_open(filename);

        if (handle == nullptr)
        {
            const char* error = lib_error(filename);
            // Since discovery can find multi-architecture binaries, don't print ELF related errors
            if (error && strstr(error, "wrong ELF class") == nullptr && strstr(error, "Bad EXE format") == nullptr)
                DISCOVERY_OUT("error", error);
            return 1;
        }
    }

    switch (type)
    {
    case PLUGIN_LADSPA:
        do_ladspa_check(handle);
        break;
    case PLUGIN_DSSI:
        do_dssi_check(handle);
        break;
    case PLUGIN_LV2:
        do_lv2_check(filename);
        break;
    case PLUGIN_VST:
        do_vst_check(handle);
        break;
    case PLUGIN_SF2:
        do_sf2_check(filename);
        break;
    default:
        break;
    }

    if (open_lib)
        lib_close(handle);

    return 0;
}

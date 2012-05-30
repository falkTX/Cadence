/*
 * JACK Backend code for Carla
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#ifndef CARLA_VST_INCLUDES_H
#define CARLA_VST_INCLUDES_H

#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"

#if VESTIGE_HEADER
#warning Using vestige header
#define effFlagsProgramChunks (1 << 5)
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effGetProgramNameIndexed 29
#define effGetPlugCategory 35
#define effSetBlockSizeAndSampleRate 43
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#define kVstProcessLevelUser 1
#define kVstProcessLevelRealtime 2
#define kVstProcessLevelOffline 4
#define kVstProcessPrecision32 0
#define kVstTransportChanged 1
#define kVstVersion 2400
struct VstTimeInfo_R {
    double samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos;
    int32_t timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags;
};
#else
typedef VstTimeInfo VstTimeInfo_R;
#endif

typedef AEffect* (*VST_Function)(audioMasterCallback);

inline bool VstPluginCanDo(AEffect* effect, const char* feature)
{
    return (effect->dispatcher(effect, effCanDo, 0, 0, (void*)feature, 0.0f) == 1);
}

inline const char* VstOpcode2str(int32_t opcode)
{
    switch (opcode)
    {
    case audioMasterAutomate:
        return "audioMasterAutomate";
    case audioMasterVersion:
        return "audioMasterVersion";
    case audioMasterCurrentId:
        return "audioMasterCurrentId";
    case audioMasterIdle:
        return "audioMasterIdle";
#if ! VST_FORCE_DEPRECATED
    case audioMasterPinConnected:
        return "audioMasterPinConnected";
    case audioMasterWantMidi:
        return "audioMasterWantMidi";
#endif
    case audioMasterGetTime:
        return "audioMasterGetTime";
    case audioMasterProcessEvents:
        return "audioMasterProcessEvents";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetTime:
        return "audioMasterSetTime";
    case audioMasterTempoAt:
        return "audioMasterTempoAt";
    case audioMasterGetNumAutomatableParameters:
        return "audioMasterGetNumAutomatableParameters";
    case audioMasterGetParameterQuantization:
        return "audioMasterGetParameterQuantization";
#endif
    case audioMasterIOChanged:
        return "audioMasterIOChanged";
#if ! VST_FORCE_DEPRECATED
    case audioMasterNeedIdle:
        return "audioMasterNeedIdle";
#endif
    case audioMasterSizeWindow:
        return "audioMasterSizeWindow";
    case audioMasterGetSampleRate:
        return "audioMasterGetSampleRate";
    case audioMasterGetBlockSize:
        return "audioMasterGetBlockSize";
    case audioMasterGetInputLatency:
        return "audioMasterGetInputLatency";
    case audioMasterGetOutputLatency:
        return "audioMasterGetOutputLatency";
#if ! VST_FORCE_DEPRECATED
    case audioMasterGetPreviousPlug:
        return "audioMasterGetPreviousPlug";
    case audioMasterGetNextPlug:
        return "audioMasterGetNextPlug";
    case audioMasterWillReplaceOrAccumulate:
        return "audioMasterWillReplaceOrAccumulate";
#endif
    case audioMasterGetCurrentProcessLevel:
        return "audioMasterGetCurrentProcessLevel";
    case audioMasterGetAutomationState:
        return "audioMasterGetAutomationState";
    case audioMasterOfflineStart:
        return "audioMasterOfflineStart";
    case audioMasterOfflineRead:
        return "audioMasterOfflineRead";
    case audioMasterOfflineWrite:
        return "audioMasterOfflineWrite";
    case audioMasterOfflineGetCurrentPass:
        return "audioMasterOfflineGetCurrentPass";
    case audioMasterOfflineGetCurrentMetaPass:
        return "audioMasterOfflineGetCurrentMetaPass";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetOutputSampleRate:
        return "audioMasterSetOutputSampleRate";
#ifdef VESTIGE_HEADER
    case audioMasterGetSpeakerArrangement:
#else
    case audioMasterGetOutputSpeakerArrangement:
#endif
        return "audioMasterGetOutputSpeakerArrangement";
#endif
    case audioMasterGetVendorString:
        return "audioMasterGetVendorString";
    case audioMasterGetProductString:
        return "audioMasterGetProductString";
    case audioMasterGetVendorVersion:
        return "audioMasterGetVendorVersion";
    case audioMasterVendorSpecific:
        return "audioMasterVendorSpecific";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetIcon:
        return "audioMasterSetIcon";
#endif
    case audioMasterCanDo:
        return "audioMasterCanDo";
    case audioMasterGetLanguage:
        return "audioMasterGetLanguage";
#if ! VST_FORCE_DEPRECATED
    case audioMasterOpenWindow:
        return "audioMasterOpenWindow";
    case audioMasterCloseWindow:
        return "audioMasterCloseWindow";
#endif
    case audioMasterGetDirectory:
        return "audioMasterGetDirectory";
    case audioMasterUpdateDisplay:
        return "audioMasterUpdateDisplay";
    case audioMasterBeginEdit:
        return "audioMasterBeginEdit";
    case audioMasterEndEdit:
        return "audioMasterEndEdit";
    case audioMasterOpenFileSelector:
        return "audioMasterOpenFileSelector";
    case audioMasterCloseFileSelector:
        return "audioMasterCloseFileSelector";
#if ! VST_FORCE_DEPRECATED
    case audioMasterEditFile:
        return "audioMasterEditFile";
    case audioMasterGetChunkFile:
        return "audioMasterGetChunkFile";
    case audioMasterGetInputSpeakerArrangement:
        return "audioMasterGetInputSpeakerArrangement";
#endif
    default:
        return "unknown";
    }
}

#endif // CARLA_VST_INCLUDES_H

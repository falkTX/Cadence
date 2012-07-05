/*
 * Carla Backend
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

#ifdef CARLA_ENGINE_VST

#include "carla_engine.h"
#include "carla_plugin.h"

#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// -------------------------------------------------------------------------------------------------------------------
// static VST<->Engine calls

static intptr_t carla_vst_dispatcher(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    qDebug("CarlaEngineVst::dispatcher(%p, %i, %i, " P_INTPTR ", %p, %f)", effect, opcode, index, value, ptr, opt);

    CarlaEngineVst* const engine = (CarlaEngineVst*)effect->object;

    if (opcode == effClose)
    {
        delete engine;
        return 1;
    }

    return engine->handleDispatch(opcode, index, value, ptr, opt);
}

static float carla_vst_get_parameter(AEffect* effect, int32_t index)
{
    CarlaEngineVst* const engine = (CarlaEngineVst*)effect->object;
    return engine->handleGetParameter(index);
}

static void carla_vst_set_parameter(AEffect* effect, int32_t index, float value)
{
    CarlaEngineVst* const engine = (CarlaEngineVst*)effect->object;
    engine->handleSetParameter(index, value);
}

static void carla_vst_process_replacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    CarlaEngineVst* const engine = (CarlaEngineVst*)effect->object;
    engine->handleProcessReplacing(inputs, outputs,  sampleFrames);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

int CarlaEngine::maxClientNameSize()
{
    return STR_MAX/2;
}

int CarlaEngine::maxPortNameSize()
{
    return STR_MAX;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine (VST)

CarlaEngineVst::CarlaEngineVst(audioMasterCallback callback_) :
    CarlaEngine(),
    m_callback(callback_)
{
    memset(&effect, 0, sizeof(AEffect));

    // vst fields
    effect.magic    = kEffectMagic;
    effect.object   = this;
    effect.version  = kVstVersion;
    effect.uniqueID = CCONST('C', 'r', 'l', 'a');

    // plugin fields
    effect.numPrograms = 0;
    effect.numParams   = 1;
    effect.numInputs   = 2;
    effect.numOutputs  = 2;
    effect.ioRatio = 1.0f;

    // static calls
    effect.dispatcher = carla_vst_dispatcher;
    effect.process    = carla_vst_process_replacing;
    effect.getParameter = carla_vst_get_parameter;
    effect.setParameter = carla_vst_set_parameter;
    effect.processReplacing = carla_vst_process_replacing;

    // plugin flags
    //effect.flags |= effFlagsHasEditor;
    effect.flags |= effFlagsCanReplacing;
    //effect.flags |= effFlagsProgramChunks;

    // setup engine
    sampleRate = callback(audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
    bufferSize = callback(audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);

    // set engine name
    char strBuf[STR_MAX];
    callback(audioMasterGetVendorString, 0, 0, strBuf, 0.0f);

    if (strBuf[0] == 0)
        strcpy(strBuf, "CarlaVST");

    init(strBuf);

    carla_options.bridge_lv2gtk2 = strdup("/home/falktx/Personal/FOSS/GIT/Cadence/src/carla-bridge/carla-bridge-lv2-gtk2");
    //carla_options.prefer_ui_bridges = true;
}

CarlaEngineVst::~CarlaEngineVst()
{
    close();
}

// -------------------------------------------------------------------------------------------------------------------

CarlaCheckThread checkThread;

bool CarlaEngineVst::init(const char* const name_)
{
    name = strdup(name_);
    osc_init(name);

    // TEST
    CarlaPlugin::initializer init = {
        this,
        "fake-filename-here",
        nullptr,
        "http://invadarecords.com/plugins/lv2/meter"
//        "http://nedko.aranaudov.org/soft/filter/2/stereo"
    };

    short id = CarlaPlugin::newLV2(init);
    CarlaPlugins[id]->setActive(true, false, false);
    CarlaPlugins[id]->showGui(true);
    checkThread.start();

    return true;
}

bool CarlaEngineVst::close()
{
    if (name)
    {
        free((void*)name);
        name = nullptr;
    }

    checkThread.stopNow();
    osc_close();

    CarlaPlugin* plugin = CarlaPlugins[0];
    carla_proc_lock();
    plugin->setEnabled(false);
    carla_proc_unlock();
    delete plugin;

    return true;
}

bool CarlaEngineVst::isOnAudioThread()
{
    return (callback(audioMasterGetCurrentProcessLevel, 0, 0, nullptr, 0.0f) == kVstProcessLevelRealtime);
}

bool CarlaEngineVst::isOffline()
{
    return (callback(audioMasterGetCurrentProcessLevel, 0, 0, nullptr, 0.0f) == kVstProcessLevelOffline);
}

bool CarlaEngineVst::isRunning()
{
    return true;
}

CarlaEngineClient* CarlaEngineVst::addClient(CarlaPlugin* const plugin)
{
    bool active = true;

    CarlaEngineClientNativeHandle handle = {
    };

    return new CarlaEngineClient(handle, active);
    Q_UNUSED(plugin);
}

// -------------------------------------------------------------------------------------------------------------------

intptr_t CarlaEngineVst::handleDispatch(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    switch (opcode)
    {
    case effOpen:
    case effClose:
    case effSetProgram:
    case effGetProgram:
    case effSetProgramName:
    case effGetProgramName:
    case effGetParamLabel:
    case effGetParamDisplay:
        break;

    case effGetParamName:
        strncpy((char*)ptr, "Enabled", kVstMaxParamStrLen);
        break;

    case effGetVu:
        break;

    case effSetSampleRate:
        sampleRate = opt; // TODO - watch for changes
        break;

    case effSetBlockSize:
        bufferSize = value; // TODO - watch for changes
        break;

    case effMainsChanged:
    case effEditGetRect:
    case effEditOpen:
    case effEditClose:
    case effEditDraw:
    case effEditMouse:
    case effEditKey:

    case effEditIdle:
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* const plugin = CarlaPlugins[i];

            if (plugin && plugin->enabled())
                plugin->idleGui();
        }
        break;

    case effEditTop:
    case effEditSleep:
    case effIdentify:
    case effGetChunk:
    case effSetChunk:
    case effProcessEvents:
    case effCanBeAutomated:
    case effString2Parameter:
    case effGetNumProgramCategories:
    case effGetProgramNameIndexed:
    case effCopyProgram:
    case effConnectInput:
    case effConnectOutput:
    case effGetInputProperties:
    case effGetOutputProperties:
    case effGetPlugCategory:
    case effGetCurrentPosition:
    case effGetDestinationBuffer:
    case effOfflineNotify:
    case effOfflinePrepare:
    case effOfflineRun:
    case effProcessVarIo:
    case effSetSpeakerArrangement:
    case effSetBlockSizeAndSampleRate:
    case effSetBypass:
    case effGetEffectName:
    case effGetErrorText:
        break;

    case effGetVendorString:
        strncpy((char*)ptr, "falkTX", kVstMaxVendorStrLen);
        break;

    case effGetProductString:
        strncpy((char*)ptr, "Carla VST", kVstMaxProductStrLen);
        break;

    case effGetVendorVersion:
        return 0x050;

    case effVendorSpecific:
        return 0;

    case effCanDo:
    {
        const char* const feature = (char*)ptr;

        if (strcmp(feature, "sendVstEvents") == 0)
            return 1;
        if (strcmp(feature, "sendVstMidiEvent") == 0)
            return 1;
        if (strcmp(feature, "receiveVstEvents") == 0)
            return 1;
        if (strcmp(feature, "receiveVstMidiEvent") == 0)
            return 1;
        if (strcmp(feature, "receiveVstTimeInfo") == 0)
            return 1;

        return 0;
    }

    case effGetTailSize:
    case effIdle:
    case effGetIcon:
    case effSetViewPosition:
    case effGetParameterProperties:
    case effKeysRequired:
        break;

    case effGetVstVersion:
        return kVstVersion;

    case effEditKeyDown:
    case effEditKeyUp:
    case effSetEditKnobMode:
    case effGetMidiProgramName:
    case effGetCurrentMidiProgram:
    case effGetMidiProgramCategory:
    case effHasMidiProgramsChanged:
    case effGetMidiKeyName:
    case effBeginSetProgram:
    case effEndSetProgram:
    case effGetSpeakerArrangement:
    case effShellGetNextPlugin:
    case effStartProcess:
    case effStopProcess:
    case effSetTotalSampleToProcess:
    case effSetPanLaw:
    case effBeginLoadBank:
    case effBeginLoadProgram:
    case effSetProcessPrecision:
    case effGetNumMidiInputChannels:
    case effGetNumMidiOutputChannels:
        break;
    }

    return 0;
    Q_UNUSED(index);
}

float CarlaEngineVst::handleGetParameter(int32_t index)
{
    return 0.0f;
    Q_UNUSED(index);
}

void CarlaEngineVst::handleSetParameter(int32_t index, float value)
{
    return;
    Q_UNUSED(index);
    Q_UNUSED(value);
}

void CarlaEngineVst::handleProcessReplacing(float** inputs, float** outputs, int32_t frames)
{
    //const VstTimeInfo_R* const vstTimeInfo = (VstTimeInfo_R*)callback(audioMasterGetTime, 0, kVstTransportPlaying, nullptr, 0.0f);

    //timeInfo.playing = bool(vstTimeInfo->flags & kVstTransportPlaying);

    //    if (pos.unique_1 == pos.unique_2)
    //    {
    //        timeInfo.frame = pos.frame;
    //        timeInfo.time  = pos.usecs;

    //        if (pos.valid & JackPositionBBT)
    //        {
    //            timeInfo.valid = CarlaEngineTimeBBT;
    //            timeInfo.bbt.bar  = pos.bar;
    //            timeInfo.bbt.beat = pos.beat;
    //            timeInfo.bbt.tick = pos.tick;
    //            timeInfo.bbt.bar_start_tick = pos.bar_start_tick;
    //            timeInfo.bbt.beats_per_bar  = pos.beats_per_bar;
    //            timeInfo.bbt.beat_type      = pos.beat_type;
    //            timeInfo.bbt.ticks_per_beat = pos.ticks_per_beat;
    //            timeInfo.bbt.beats_per_minute = pos.beats_per_minute;
    //        }
    //        else
    //            timeInfo.valid = 0;
    //    }
    //    else
    //    {
    //timeInfo.frame = 0;
    //timeInfo.valid = 0;
    //    }

    // create temporary audio buffers
    float ains_tmp_buf1[frames];
    float ains_tmp_buf2[frames];
    float aouts_tmp_buf1[frames];
    float aouts_tmp_buf2[frames];

    float* ains_tmp[2]  = { ains_tmp_buf1, ains_tmp_buf2 };
    float* aouts_tmp[2] = { aouts_tmp_buf1, aouts_tmp_buf2 };

    // initialize audio input
    memcpy(ains_tmp_buf1, inputs[0], sizeof(float)*frames);
    memcpy(ains_tmp_buf2, inputs[1], sizeof(float)*frames);

    // initialize control input
    memset(rackControlEventsIn, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    {
        // TODO
    }

    // initialize midi input
    memset(rackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
    {
        // TODO
    }

    // initialize outputs (zero)
    memset(aouts_tmp_buf1, 0, sizeof(float)*frames);
    memset(aouts_tmp_buf2, 0, sizeof(float)*frames);
    memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

    bool processed = false;

    // process plugins
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = CarlaPlugins[i];
        if (plugin && plugin->enabled())
        {
            if (processed)
            {
                // initialize inputs (from previous outputs)
                memcpy(ains_tmp_buf1, aouts_tmp_buf1, sizeof(float)*frames);
                memcpy(ains_tmp_buf2, aouts_tmp_buf2, sizeof(float)*frames);
                memcpy(rackMidiEventsIn, rackMidiEventsOut, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

                // initialize outputs (zero)
                memset(aouts_tmp_buf1, 0, sizeof(float)*frames);
                memset(aouts_tmp_buf2, 0, sizeof(float)*frames);
                memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            }

            // process
            carla_proc_lock();

            plugin->initBuffers();

            if (carla_options.proccess_hq)
            {
                float* ains_buffer2[2];
                float* aouts_buffer2[2];

                for (int32_t j=0; j < frames; j += 8)
                {
                    ains_buffer2[0] = ains_tmp_buf1 + j;
                    ains_buffer2[1] = ains_tmp_buf2 + j;

                    aouts_buffer2[0] = aouts_tmp_buf1 + j;
                    aouts_buffer2[1] = aouts_tmp_buf2 + j;

                    plugin->process(ains_buffer2, aouts_buffer2, 8, j);
                }
            }
            else
                plugin->process(ains_tmp, aouts_tmp, frames);

            carla_proc_unlock();

            // if plugin has no audio inputs, add previous buffers
            if (plugin->audioInCount() == 0)
            {
                for (int32_t j=0; j < frames; j++)
                {
                    aouts_tmp_buf1[j] += ains_tmp_buf1[j];
                    aouts_tmp_buf2[j] += ains_tmp_buf2[j];
                }
            }

            processed = true;
        }
    }

    // if no plugins in the rack, copy inputs over outputs
    if (! processed)
    {
        memcpy(aouts_tmp_buf1, ains_tmp_buf1, sizeof(float)*frames);
        memcpy(aouts_tmp_buf2, ains_tmp_buf2, sizeof(float)*frames);
        memcpy(rackMidiEventsOut, rackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
    }

    // output audio
    memcpy(outputs[0], aouts_tmp_buf1, sizeof(float)*frames);
    memcpy(outputs[1], aouts_tmp_buf2, sizeof(float)*frames);

    // output control
    {
        // TODO
    }

    // output midi
    {
        // TODO
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(const CarlaEngineClientNativeHandle& handle_, bool active) :
    m_active(active),
    handle(handle_)
{
}

CarlaEngineClient::~CarlaEngineClient()
{
}

void CarlaEngineClient::activate()
{
    m_active = true;
}

void CarlaEngineClient::deactivate()
{
    m_active = false;
}

bool CarlaEngineClient::isActive() const
{
    return m_active;
}

bool CarlaEngineClient::isOk() const
{
    return true;
}

const CarlaEngineBasePort* CarlaEngineClient::addPort(CarlaEnginePortType type, const char* name, bool isInput)
{
    CarlaEnginePortNativeHandle portHandle = {
        nullptr
    };

    switch (type)
    {
    case CarlaEnginePortTypeAudio:
        return new CarlaEngineAudioPort(portHandle, isInput);
    case CarlaEnginePortTypeControl:
        return new CarlaEngineControlPort(portHandle, isInput);
    case CarlaEnginePortTypeMIDI:
        return new CarlaEngineMidiPort(portHandle, isInput);
    default:
        return nullptr;
    }

    Q_UNUSED(name);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(CarlaEnginePortNativeHandle& handle_, bool isInput_) :
    isInput(isInput_),
    handle(handle_)
{
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const /*engine*/)
{
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineControlPort::initBuffer(CarlaEngine* const engine)
{
    handle.buffer = isInput ? engine->rackControlEventsIn : engine->rackControlEventsOut;
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    if (! isInput)
        return 0;

    uint32_t count = 0;
    const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

    for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++)
    {
        if (events[i].type != CarlaEngineEventNull)
            count++;
        else
            break;
    }

    return count;
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

    if (index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS)
        return &events[index];
    return nullptr;
}

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

    CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

    for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++)
    {
        if (events[i].type == CarlaEngineEventNull)
        {
            events[i].type  = type;
            events[i].time  = time;
            events[i].value = value;
            events[i].channel    = channel;
            events[i].controller = controller;
            break;
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineMidiPort::initBuffer(CarlaEngine* const engine)
{
    handle.buffer = isInput ? engine->rackMidiEventsIn : engine->rackMidiEventsOut;
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    if (! isInput)
        return 0;

    uint32_t count = 0;
    const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

    for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++)
    {
        if (events[i].size > 0)
            count++;
        else
            break;
    }

    return count;
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

    if (index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS)
        return &events[index];

    return nullptr;
}

void CarlaEngineMidiPort::writeEvent(uint32_t time, uint8_t* data, uint8_t size)
{
    if (isInput || size >= 4)
        return;

    CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

    for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++)
    {
        if (events[i].size == 0)
        {
            events[i].time = time;
            events[i].size = size;
            memcpy(events[i].data, data, size);
            break;
        }
    }
}

CARLA_BACKEND_END_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// -------------------------------------------------------------------------------------------------------------------
// VST plugin entry

CARLA_EXPORT
const AEffect* VSTPluginMain(audioMasterCallback callback)
{
    qDebug("VSTPluginMain(%p)", callback);
    CarlaEngineVst* engine = new CarlaEngineVst(callback);
    return engine->getEffect();
}

#endif // CARLA_ENGINE_VST

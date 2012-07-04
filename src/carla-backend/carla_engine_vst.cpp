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

// Global VST stuff
static int32_t carla_buffer_size = 512;
static double  carla_sample_rate = 44100;

static const char* carla_client_name = nullptr;
static QThread* carla_proc_thread = nullptr;

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)

bool is_engine_running()
{
    return true; // TODO
}

const char* get_host_client_name()
{
    return carla_client_name;
}

quint32 get_buffer_size()
{
    qDebug("get_buffer_size()");
    if (carla_options.proccess_hq)
        return 8;
    return carla_buffer_size;
}

double get_sample_rate()
{
    qDebug("get_sample_rate()");
    return carla_sample_rate;
}

double get_latency()
{
    qDebug("get_latency()");
    return double(carla_buffer_size)/carla_sample_rate*1000;
}

// -------------------------------------------------------------------------------------------------------------------
// static VST<->Engine class

class CarlaEngineVst
{
public:
    CarlaEngineVst(const audioMasterCallback callback_) :
        m_callback(callback_)
    {
        qDebug("CarlaEngineVst()");
        memset(&effect, 0, sizeof(AEffect));

        // static fields
        effect.magic  = kEffectMagic;
        effect.object = this;
        effect.uniqueID = CCONST('C', 'r', 'l', 'a');
        effect.version = kVstVersion;

        // ...
        effect.numPrograms = 0;
        effect.numParams = 0;
        effect.numInputs = 2;
        effect.numOutputs = 2;
        effect.ioRatio = 1.0f;

        // static calls
        effect.dispatcher = dispatcher;
        effect.process = nullptr;
        effect.setParameter = nullptr;
        effect.getParameter = nullptr;
        effect.processReplacing = processReplacing;
        effect.processDoubleReplacing = nullptr;

        // plugin flags
        //effect.flags |= effFlagsHasEditor;
        effect.flags |= effFlagsCanReplacing;
        //effect.flags |= effFlagsProgramChunks;

        carla_sample_rate = sampleRate = callback(audioMasterGetSampleRate, 0, 0, nullptr, 0.0f);
        carla_buffer_size = bufferSize = callback(audioMasterGetBlockSize, 0, 0, nullptr, 0.0f);

        char strBuf[STR_MAX];
        callback(audioMasterGetVendorString, 0, 0, strBuf, 0.0f);

        if (strBuf[0] == 0)
            strcpy(strBuf, "CarlaVST");

        engine.init(strBuf);
    }

    ~CarlaEngineVst()
    {
        qDebug("~CarlaEngineVst()");
        engine.close();
    }

    intptr_t callback(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return m_callback(&effect, opcode, index, value, ptr, opt);
    }

    const AEffect* getEffect() const
    {
        qDebug("CarlaEngineVst::getEffect()");
        return &effect;
    }

    // ---------------------------------------------------------------

    intptr_t handleDispatch(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
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
        case effGetParamName:
        case effGetVu:
            break;

        case effSetSampleRate:
            carla_sample_rate = sampleRate = opt;
            break;

        case effSetBlockSize:
            carla_buffer_size = bufferSize = value;
            break;

        case effMainsChanged:
        case effEditGetRect:
        case effEditOpen:
        case effEditClose:
        case effEditDraw:
        case effEditMouse:
        case effEditKey:
        case effEditIdle:
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

    void handleProcessReplacing(float** inputs, float** outputs, int32_t sampleFrames)
    {
        return;
        Q_UNUSED(inputs);
        Q_UNUSED(outputs);
        Q_UNUSED(sampleFrames);
    }

    // ---------------------------------------------------------------

    static intptr_t dispatcher(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        qDebug("CarlaEngineVst::dispatcher(%p, %i, %i, " P_INTPTR ", %p, %f)", effect, opcode, index, value, ptr, opt);

        CarlaEngineVst* engine = (CarlaEngineVst*)effect->object;

        if (opcode == effClose)
        {
            delete engine;
            return 1;
        }

        return engine->handleDispatch(opcode, index, value, ptr, opt);
    }

    static void processReplacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
    {
        CarlaEngineVst* engine = (CarlaEngineVst*)effect->object;
        engine->handleProcessReplacing(inputs, outputs,  sampleFrames);
    }

private:
    AEffect effect;
    double sampleRate;
    uint32_t bufferSize;

    CarlaEngine engine;

    const audioMasterCallback m_callback;
};

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine() {}
CarlaEngine::~CarlaEngine() {}

bool CarlaEngine::init(const char* name)
{
    carla_client_name = strdup(name);

    return true;
}

bool CarlaEngine::close()
{
    if (carla_client_name)
    {
        free((void*)carla_client_name);
        carla_client_name = nullptr;
    }

    return true;
}

const CarlaTimeInfo* CarlaEngine::getTimeInfo()
{
    static CarlaTimeInfo info;
    info.playing = false;
    info.frame = 0;
    info.valid = 0;
    return &info;
}

bool CarlaEngine::isOnAudioThread()
{
    return (QThread::currentThread() == carla_proc_thread);
}

bool CarlaEngine::isOffline()
{
    return false;
}

int CarlaEngine::maxClientNameSize()
{
    return STR_MAX;
}

int CarlaEngine::maxPortNameSize()
{
    return STR_MAX;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(CarlaEngineClientNativeHandle* const clientHandle, bool isInput_) :
    isInput(isInput_),
    client(clientHandle)
{
    handle = nullptr;
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(CarlaPlugin* const plugin)
{
    handle = nullptr;
    m_active = false;
    Q_UNUSED(plugin);
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

bool CarlaEngineClient::isActive()
{
    return m_active;
}

bool CarlaEngineClient::isOk()
{
    //return bool(handle);
    return true;
}

CarlaEngineBasePort* CarlaEngineClient::addPort(const char* name, CarlaEnginePortType type, bool isInput)
{
    switch (type)
    {
    case CarlaEnginePortTypeAudio:
        return new CarlaEngineAudioPort(handle, name, isInput);
    case CarlaEnginePortTypeControl:
        return new CarlaEngineControlPort(handle, name, isInput);
    case CarlaEnginePortTypeMIDI:
        return new CarlaEngineMidiPort(handle, name, isInput);
    default:
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    Q_UNUSED(name);
}

void CarlaEngineAudioPort::initBuffer()
{
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    Q_UNUSED(name);
}

void CarlaEngineControlPort::initBuffer()
{
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    return 0;
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    return nullptr;
    Q_UNUSED(index);
}

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    Q_UNUSED(type);
    Q_UNUSED(time);
    Q_UNUSED(channel);
    Q_UNUSED(controller);
    Q_UNUSED(value);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    Q_UNUSED(name);
}

void CarlaEngineMidiPort::initBuffer()
{
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    return 0;
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    return nullptr;
    Q_UNUSED(index);
}

void CarlaEngineMidiPort::writeEvent(uint32_t time, uint8_t* data, uint8_t size)
{
    Q_UNUSED(time);
    Q_UNUSED(data);
    Q_UNUSED(size);
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

#endif // CARLA_ENGINE_RTAUDIO

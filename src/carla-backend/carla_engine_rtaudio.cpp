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

#ifdef CARLA_ENGINE_RTAUDIO

#include "carla_engine.h"
#include "carla_plugin.h"

#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// Global RtAudio stuff
static RtAudio adac(RtAudio::LINUX_PULSE);

static uint32_t carla_buffer_size = 512;
static const char* carla_client_name = nullptr;
static QThread* carla_proc_thread = nullptr;

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)

bool is_engine_running()
{
    return adac.isStreamRunning();
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
    return adac.getStreamSampleRate();
}

double get_latency()
{
    qDebug("get_latency()");
    return double(carla_buffer_size)/get_sample_rate()*1000;
}

// -------------------------------------------------------------------------------------------------------------------
// static RtAudio<->Engine calls

static int process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double /*streamTime*/, RtAudioStreamStatus /*status*/, void* /*userData*/)
{
    if (carla_proc_thread == nullptr)
        carla_proc_thread = QThread::currentThread();

    float* insPtr  = (float*)inputBuffer;
    float* outsPtr = (float*)outputBuffer;

    //float* in1  = insPtr  + 0*nframes;
    //float* in2  = insPtr  + 1*nframes;
    //float* out1 = outsPtr + 0*nframes;
    //float* out2 = outsPtr + 1*nframes;

    float ains_tmp_buf1[nframes];
    float ains_tmp_buf2[nframes];
    float aouts_tmp_buf1[nframes];
    float aouts_tmp_buf2[nframes];

    float* ains_tmp[2]  = { ains_tmp_buf1, ains_tmp_buf2 };
    float* aouts_tmp[2] = { aouts_tmp_buf1, aouts_tmp_buf2 };

    for (unsigned int i=0; i < nframes*2; i++)
    {
        if (i % 2)
            ains_tmp_buf2[i/2] = insPtr[i];
        else
            ains_tmp_buf1[i/2] = insPtr[i];
    }

    //memcpy(ains_tmp_buf1, in1, sizeof(float)*nframes);
    //memcpy(ains_tmp_buf2, in2, sizeof(float)*nframes);
    memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
    memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->enabled())
        {
            memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
            memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);

            carla_proc_lock();
            plugin->process(ains_tmp, aouts_tmp, nframes);
            carla_proc_unlock();

            memcpy(ains_tmp_buf1, aouts_tmp_buf1, sizeof(float)*nframes);
            memcpy(ains_tmp_buf2, aouts_tmp_buf2, sizeof(float)*nframes);
        }
    }

    for (unsigned int i=0; i < nframes*2; i++)
    {
        if (i % 2)
            outsPtr[i] = aouts_tmp_buf2[i/2];
        else
            outsPtr[i] = aouts_tmp_buf1[i/2];
    }

    //memcpy(out1, aouts_tmp_buf1, sizeof(float)*nframes);
    //memcpy(out2, aouts_tmp_buf2, sizeof(float)*nframes);

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine() {}
CarlaEngine::~CarlaEngine() {}

bool CarlaEngine::init(const char* name)
{
    if (adac.getDeviceCount() < 1)
    {
        set_last_error("No audio devices available");
        return false;
    }

//    for (unsigned int i=0; i < adac.getDeviceCount(); i++)
//    {
//        qWarning("DevName %i: %s", i, adac.getDeviceInfo(i).name.c_str());
//    }

    // Set the same number of channels for both input and output.
    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters iParams, oParams;
//    iParams.deviceId  = 3;
//    oParams.deviceId  = 2;
    iParams.nChannels = 2;
    oParams.nChannels = 2;
    RtAudio::StreamOptions options;
    options.flags = /*RTAUDIO_NONINTERLEAVED |*/ RTAUDIO_MINIMIZE_LATENCY /*| RTAUDIO_HOG_DEVICE*/ | RTAUDIO_SCHEDULE_REALTIME /*| RTAUDIO_ALSA_USE_DEFAULT*/;
    options.streamName = name;
    options.priority = 85;

    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, 48000, &bufferFrames, process_callback, nullptr, &options);
    }
    catch (RtError& e)
    {
        set_last_error(e.what());
        return false;
    }

    carla_buffer_size = bufferFrames;
    carla_client_name = strdup(name);

    try {
        adac.startStream();
    }
    catch (RtError& e)
    {
        set_last_error(e.what());
        return false;
    }

    return true;
}

bool CarlaEngine::close()
{
    if (carla_client_name)
    {
        free((void*)carla_client_name);
        carla_client_name = nullptr;
    }

    if (adac.isStreamRunning())
        adac.stopStream();

    if (adac.isStreamOpen())
        adac.closeStream();

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
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(CarlaPlugin* const plugin)
{
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
    return bool(handle);
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

void* CarlaEngineAudioPort::getBuffer()
{
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    Q_UNUSED(name);
}

void* CarlaEngineControlPort::getBuffer()
{
    return nullptr;
}

void CarlaEngineControlPort::initBuffer(void* buffer)
{
    //if (isInput)
    return;

    Q_UNUSED(buffer);
}

uint32_t CarlaEngineControlPort::getEventCount(void* buffer)
{
    //if (! isInput)
    return 0;

    Q_UNUSED(buffer);
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(void* buffer, uint32_t index)
{
    //if (! isInput)
    return nullptr;

    Q_UNUSED(buffer);
    Q_UNUSED(index);
}

void CarlaEngineControlPort::writeEvent(void* buffer, CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    //if (isInput)
    return;

    Q_UNUSED(buffer);
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

void* CarlaEngineMidiPort::getBuffer()
{
    return nullptr;
}

void CarlaEngineMidiPort::initBuffer(void* buffer)
{
    //if (isInput)
    return;

    Q_UNUSED(buffer);
}

uint32_t CarlaEngineMidiPort::getEventCount(void* buffer)
{
    //if (! isInput)
    return 0;

    Q_UNUSED(buffer);
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(void* buffer, uint32_t index)
{
    //if (! isInput)
    return nullptr;

    return nullptr;

    Q_UNUSED(buffer);
    Q_UNUSED(index);
}

void CarlaEngineMidiPort::writeEvent(void* buffer, uint32_t time, uint8_t* data, uint8_t size)
{
    //if (! isInput)
    return;

    Q_UNUSED(buffer);
    Q_UNUSED(time);
    Q_UNUSED(data);
    Q_UNUSED(size);
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO

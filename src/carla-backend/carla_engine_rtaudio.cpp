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

#ifdef CARLA_ENGINE_RTAUDIO

#include "carla_engine.h"
#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

// Global RtAudio stuff
static RtAudio adac(RtAudio::UNIX_JACK);

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
    if (carla_options.proccess_32x)
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

static int process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
{
    if (carla_proc_thread == nullptr)
        carla_proc_thread = QThread::currentThread();

    float* insPtr  = (float*)inputBuffer;
    float* outsPtr = (float*)outputBuffer;

    float* in1  = insPtr  + 0*nframes;
    float* in2  = insPtr  + 1*nframes;
    float* out1 = outsPtr + 0*nframes;
    float* out2 = outsPtr + 1*nframes;

    float* ains_buffer[2]  = { in1, in2 };
    float* aouts_buffer[2] = { out1, out2 };

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->enabled())
        {
            carla_proc_lock();

            plugin->process(ains_buffer, aouts_buffer, nframes);

            carla_proc_unlock();
        }
    }

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

    // Set the same number of channels for both input and output.
    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters iParams, oParams;
    iParams.nChannels = 2;
    oParams.nChannels = 2;
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME /*| RTAUDIO_ALSA_USE_DEFAULT*/;
    options.streamName = name;
    options.priority = 85;

    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, process_callback, nullptr, &options);
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

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(CarlaEngineClientNativeHandle* const clientHandle, bool isInput_) :
    isInput(isInput_),
    client(clientHandle)
{
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
    //if (handle)
    //    jack_port_unregister(client, handle);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(CarlaPlugin* const plugin)
{
    m_active = false;
    //handle   = jack_client_open(plugin->name(), JackNullOption, nullptr);

    //if (handle)
    //    jack_set_process_callback(handle, carla_jack_process_callback, plugin);
}

CarlaEngineClient::~CarlaEngineClient()
{
    //if (handle)
    //    jack_client_close(handle);
}

void CarlaEngineClient::activate()
{
    //if (handle)
    //    jack_activate(handle);
    m_active = true;
}

void CarlaEngineClient::deactivate()
{
    //if (handle)
    //    jack_deactivate(handle);
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
    if (type == CarlaEnginePortTypeAudio)
        return new CarlaEngineAudioPort(handle, name, isInput);
    if (type == CarlaEnginePortTypeControl)
        return new CarlaEngineControlPort(handle, name, isInput);
    if (type == CarlaEnginePortTypeMIDI)
        return new CarlaEngineMidiPort(handle, name, isInput);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    //handle = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineAudioPort::getBuffer(uint32_t frames)
{
    return nullptr; //jack_port_get_buffer(handle, frames);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    //handle = jack_port_register(client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineControlPort::getBuffer(uint32_t frames)
{
    //void* buffer = jack_port_get_buffer(handle, frames);

    //if (! isInput)
    //    jack_midi_clear_buffer(buffer);

    return nullptr; //buffer;
}

uint32_t CarlaEngineControlPort::getEventCount(void* buffer)
{
    if (! isInput)
        return 0;

    return 0; //jack_midi_get_event_count(buffer);
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(void* buffer, uint32_t index)
{
    //if (! isInput)
    //    return nullptr;

    //static jack_midi_event_t jackEvent;
    static CarlaEngineControlEvent carlaEvent;

    //if (jack_midi_event_get(&jackEvent, buffer, index) != 0)
    //    return nullptr;

    memset(&carlaEvent, 0, sizeof(CarlaEngineControlEvent));

    //uint8_t midiStatus  = jackEvent.buffer[0];
    //uint8_t midiChannel = midiStatus & 0x0F;

    //carlaEvent.time    = jackEvent.time;
    //carlaEvent.channel = midiChannel;

    return nullptr;
}

void CarlaEngineControlPort::writeEvent(void* buffer, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

    //jack_midi_data_t* event_buffer = jack_midi_event_reserve(buffer, 0, 3);

    //if (event_buffer)
    //{
    //    event_buffer[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
    //    event_buffer[1] = controller;
    //    event_buffer[2] = value * 127;
    //}
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
    //handle = jack_port_register(client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineMidiPort::getBuffer(uint32_t frames)
{
    //void* buffer = jack_port_get_buffer(handle, frames);

    //if (! isInput)
    //    jack_midi_clear_buffer(buffer);

    return nullptr; //buffer;
}

uint32_t CarlaEngineMidiPort::getEventCount(void* buffer)
{
    if (! isInput)
        return 0;

    return 0; //jack_midi_get_event_count(buffer);
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(void* buffer, uint32_t index)
{
    if (! isInput)
        return nullptr;

    //static jack_midi_event_t jackEvent;
    static CarlaEngineMidiEvent carlaEvent;

    //if (jack_midi_event_get(&jackEvent, buffer, index) != 0)
    //    return nullptr;

    //carlaEvent.time = jackEvent.time;
    //carlaEvent.size = jackEvent.size;
    //memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);

    return &carlaEvent;
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO

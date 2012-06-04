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

static RtAudio adac(RtAudio::LINUX_ALSA);

static uint32_t carla_buffer_size = 512;
static const char* carla_client_name = nullptr;

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)

CARLA_BACKEND_START_NAMESPACE

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
        return 32;
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

CARLA_BACKEND_END_NAMESPACE

// End of exported symbols (API)
// -------------------------------------------------------------------------------------------------------------------

static int process_callback(void* outputBuffer, void* inputBuffer, unsigned int frames, double streamTime, RtAudioStreamStatus status, void* data)
{
    // Since the number of input and output channels is equal, we can do
    // a simple buffer copy operation here.
    if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

    float* in1  = ((float**)inputBuffer)[0];
    float* in2  = ((float**)inputBuffer)[1];
    float* out1 = ((float**)outputBuffer)[0];
    float* out2 = ((float**)outputBuffer)[1];

    memcpy(out1, in1, frames);
    memcpy(out2, in2, frames);

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

CarlaEngine::CarlaEngine()
{
}

CarlaEngine::~CarlaEngine()
{
}

bool CarlaEngine::init(const char* name)
{
    if (adac.getDeviceCount() < 1)
    {
        CarlaBackend::set_last_error("No audio devices available");
        return false;
    }

    // Set the same number of channels for both input and output.
    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters iParams, oParams;
    iParams.nChannels = 2;
    oParams.nChannels = 2;
    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_ALSA_USE_DEFAULT;
    options.streamName = name;
    options.priority = 85;

    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, process_callback, nullptr, &options);
    }
    catch (RtError& e)
    {
        CarlaBackend::set_last_error(e.what());
        return false;
    }

    try {
        adac.startStream();
    }
    catch (RtError& e)
    {
        CarlaBackend::set_last_error(e.what());
        return false;
    }
}

bool CarlaEngine::close()
{
    if (adac.isStreamRunning())
        adac.stopStream();

    if (adac.isStreamOpen())
        adac.closeStream();
}

bool CarlaEngine::isOnAudioThread()
{

}

bool CarlaEngine::isOffline()
{

}

int CarlaEngine::maxClientNameSize()
{

}

#endif // CARLA_ENGINE_RTAUDIO

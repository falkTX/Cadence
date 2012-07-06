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

// get initial options from environment
RtAudio::Api getRtApiFromEnvironment()
{
#if defined(Q_OS_LINUX)
    RtAudio::Api defaultRtApi = RtAudio::LINUX_PULSE;
#elif defined(Q_OS_MACOS)
    RtAudio::Api defaultRtApi = RtAudio::MACOSX_CORE;
#elif defined(Q_OS_WIN)
    RtAudio::Api defaultRtApi = RtAudio::WINDOWS_DS;
#endif

    const char* const driver  = getenv("CARLA_BACKEND_DRIVER");

    if (! driver)
        return defaultRtApi;
#ifdef Q_OS_LINUX
    if (strcmp(driver, "LINUX_ALSA") == 0)
        return RtAudio::LINUX_ALSA;
    if (strcmp(driver, "LINUX_PULSE") == 0)
        return RtAudio::LINUX_PULSE;
    if (strcmp(driver, "LINUX_OSS") == 0)
        return RtAudio::LINUX_OSS;
#endif
    if (strcmp(driver, "UNIX_JACK") == 0)
        return RtAudio::UNIX_JACK;
#ifdef Q_OS_MACOS
    if (strcmp(driver, "MACOSX_CORE") == 0)
        return RtAudio::MACOSX_CORE;
#endif
#ifdef Q_OS_WIN
    if (strcmp(driver, "WINDOWS_ASIO") == 0)
        return RtAudio::WINDOWS_ASIO;
    if (strcmp(driver, "WINDOWS_DS") == 0)
        return RtAudio::WINDOWS_DS;
#endif

    return defaultRtApi;
}

// -------------------------------------------------------------------------------------------------------------------
// static RtAudio<->Engine calls

static int process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double /*streamTime*/, RtAudioStreamStatus /*status*/, void* /*userData*/)
{
#if 0
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
        CarlaPlugin* const plugin = CarlaPlugins[i];
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
#endif

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

// Global RtAudio stuff
static RtAudio adac(getRtApiFromEnvironment());

CarlaEngineRtAudio::CarlaEngineRtAudio()
{
}

CarlaEngineRtAudio::~CarlaEngineRtAudio()
{
}

bool CarlaEngineRtAudio::init(const char* name)
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

    //carla_buffer_size = bufferFrames;
    //carla_client_name = strdup(name);

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

bool CarlaEngineRtAudio::close()
{
    //if (carla_client_name)
    //{
    //    free((void*)carla_client_name);
    //    carla_client_name = nullptr;
    //}

    if (adac.isStreamRunning())
        adac.stopStream();

    if (adac.isStreamOpen())
        adac.closeStream();

    return true;
}

bool CarlaEngineRtAudio::isOnAudioThread()
{
    //return (QThread::currentThread() == carla_proc_thread);
    return false;
}

bool CarlaEngineRtAudio::isOffline()
{
    return false;
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO

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

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// -------------------------------------------------------------------------------------------------------------------
// static RtAudio<->Engine calls

static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
{
    CarlaEngineRtAudio* const engine = (CarlaEngineRtAudio*)userData;
    engine->handleProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine (RtAudio)

CarlaEngineRtAudio::CarlaEngineRtAudio(RtAudio::Api api)
    : CarlaEngine(),
      adac(api)
{
    qDebug("CarlaEngineRtAudio::CarlaEngineRtAudio()");

    type = CarlaEngineTypeRtAudio;

    procThread = nullptr;
}

CarlaEngineRtAudio::~CarlaEngineRtAudio()
{
    qDebug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");
}

bool CarlaEngineRtAudio::init(const char* const clientName)
{
    qDebug("CarlaEngineRtAudio::init(\"%s\")", clientName);

    procThread = nullptr;

    if (adac.getDeviceCount() < 1)
    {
        setLastError("No audio devices available");
        return false;
    }

    sampleRate = 48000;
    unsigned int rtBufferFrames = 512;

    RtAudio::StreamParameters iParams, oParams;
    //iParams.deviceId = 3;
    //oParams.deviceId = 2;
    iParams.nChannels = 2;
    oParams.nChannels = 2;
    RtAudio::StreamOptions options;
    options.flags = /*RTAUDIO_NONINTERLEAVED |*/ RTAUDIO_MINIMIZE_LATENCY /*| RTAUDIO_HOG_DEVICE*/ | RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_ALSA_USE_DEFAULT;
    options.streamName = clientName;
    options.priority = 85;

    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, sampleRate, &rtBufferFrames, carla_rtaudio_process_callback, this, &options);
    }
    catch (RtError& e)
    {
        setLastError(e.what());
        return false;
    }

    bufferSize = rtBufferFrames;
    name = strdup(clientName);

    qDebug("RtAudio bufferSize = %i", bufferSize);

    try {
        adac.startStream();
    }
    catch (RtError& e)
    {
        setLastError(e.what());
        return false;
    }

    CarlaEngine::init(name);

    return true;
}

bool CarlaEngineRtAudio::close()
{
    qDebug("CarlaEngineRtAudio::close()");
    CarlaEngine::close();

    if (name)
    {
        free((void*)name);
        name = nullptr;
    }

    if (adac.isStreamRunning())
        adac.stopStream();

    if (adac.isStreamOpen())
        adac.closeStream();

    return true;
}

bool CarlaEngineRtAudio::isOnAudioThread()
{
    return (QThread::currentThread() == procThread);
}

bool CarlaEngineRtAudio::isOffline()
{
    return false;
}

bool CarlaEngineRtAudio::isRunning()
{
    return adac.isStreamRunning();
}

CarlaEngineClient* CarlaEngineRtAudio::addClient(CarlaPlugin* const plugin)
{
    CarlaEngineClientNativeHandle handle;

    return new CarlaEngineClient(handle);
    Q_UNUSED(plugin);
}

// -------------------------------------------------------------------------------------------------------------------

void CarlaEngineRtAudio::handleProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
{
    if (procThread == nullptr)
        procThread = QThread::currentThread();

    // get buffers from RtAudio
    float* insPtr  = (float*)inputBuffer;
    float* outsPtr = (float*)outputBuffer;

    // assert buffers
    Q_ASSERT(insPtr);
    Q_ASSERT(outsPtr);

    // create temporary audio buffers
    float ains_tmp_buf1[nframes];
    float ains_tmp_buf2[nframes];
    float aouts_tmp_buf1[nframes];
    float aouts_tmp_buf2[nframes];

    float* ains_tmp[2]  = { ains_tmp_buf1, ains_tmp_buf2 };
    float* aouts_tmp[2] = { aouts_tmp_buf1, aouts_tmp_buf2 };

    // initialize audio input
    for (unsigned int i=0; i < nframes*2; i++)
    {
        if (i % 2)
            ains_tmp_buf2[i/2] = insPtr[i];
        else
            ains_tmp_buf1[i/2] = insPtr[i];
    }

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
    memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
    memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);
    memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

    bool processed = false;

    // process plugins
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = __getPlugin(i);

        if (plugin && plugin->enabled())
        {
            if (processed)
            {
                // initialize inputs (from previous outputs)
                memcpy(ains_tmp_buf1, aouts_tmp_buf1, sizeof(float)*nframes);
                memcpy(ains_tmp_buf2, aouts_tmp_buf2, sizeof(float)*nframes);
                memcpy(rackMidiEventsIn, rackMidiEventsOut, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

                // initialize outputs (zero)
                memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
                memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);
                memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            }

            // process
            plugin->engineProcessLock();

            plugin->initBuffers();

            if (carlaOptions.proccess_hp)
            {
                float* ains_buffer2[2];
                float* aouts_buffer2[2];

                for (uint32_t j=0; j < nframes; j += 8)
                {
                    ains_buffer2[0] = ains_tmp_buf1 + j;
                    ains_buffer2[1] = ains_tmp_buf2 + j;

                    aouts_buffer2[0] = aouts_tmp_buf1 + j;
                    aouts_buffer2[1] = aouts_tmp_buf2 + j;

                    plugin->process(ains_buffer2, aouts_buffer2, 8, j);
                }
            }
            else
                plugin->process(ains_tmp, aouts_tmp, nframes);

            plugin->engineProcessUnlock();

            // if plugin has no audio inputs, add previous buffers
            if (plugin->audioInCount() == 0)
            {
                for (uint32_t j=0; j < nframes; j++)
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
        memcpy(aouts_tmp_buf1, ains_tmp_buf1, sizeof(float)*nframes);
        memcpy(aouts_tmp_buf2, ains_tmp_buf2, sizeof(float)*nframes);
        memcpy(rackMidiEventsOut, rackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
    }

    // output audio
    for (unsigned int i=0; i < nframes*2; i++)
    {
        if (i % 2)
            outsPtr[i] = aouts_tmp_buf2[i/2];
        else
            outsPtr[i] = aouts_tmp_buf1[i/2];
    }

    // output control
    {
        // TODO
    }

    // output midi
    {
        // TODO
    }

    Q_UNUSED(streamTime);
    Q_UNUSED(status);
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO

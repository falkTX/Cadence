/*
 * Carla Backend
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

// -----------------------------------------

class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio(RtAudio::Api api)
        : CarlaEngine(),
          audio(api)
    {
        qDebug("CarlaEngineRtAudio::CarlaEngineRtAudio()");

        type = CarlaEngineTypeRtAudio;

        midiIn  = nullptr;
        midiOut = nullptr;

        // just to make sure
        options.forceStereo = true;
        processMode = PROCESS_MODE_CONTINUOUS_RACK;
    }

    ~CarlaEngineRtAudio()
    {
        qDebug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");
    }

    // -------------------------------------

    bool init(const char* const clientName)
    {
        qDebug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        if (audio.getDeviceCount() < 1)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        bufferSize = options.preferredBufferSize;
        sampleRate = options.preferredSampleRate;

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
            audio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, sampleRate, &bufferSize, carla_rtaudio_process_callback, this, &options);
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        try {
            audio.startStream();
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        midiIn = new MidiInAlsa(clientName, 512);
        midiIn->openVirtualPort("control-in");
        midiIn->openVirtualPort("midi-in");

        midiOut = new MidiOutAlsa(clientName);
        midiOut->openVirtualPort("control-out");
        midiOut->openVirtualPort("midi-out");

        name = getFixedClientName(clientName);
        CarlaEngine::init(name);
        return true;
    }

    bool close()
    {
        qDebug("CarlaEngineRtAudio::close()");
        CarlaEngine::close();

        if (name)
        {
            free((void*)name);
            name = nullptr;
        }

        if (audio.isStreamRunning())
            audio.stopStream();

        if (audio.isStreamOpen())
            audio.closeStream();

        if (midiIn)
        {
            midiIn->cancelCallback();
            midiIn->closePort();
            delete midiIn;
            midiIn = nullptr;
        }

        if (midiOut)
        {
            midiOut->closePort();
            delete midiOut;
            midiOut = nullptr;
        }

        return true;
    }

    bool isOffline()
    {
        return false;
    }

    bool isRunning()
    {
        return audio.isStreamRunning();
    }

    CarlaEngineClient* addClient(CarlaPlugin* const plugin)
    {
        CarlaEngineClientNativeHandle handle;
        handle.type = CarlaEngineTypeRtAudio;

        return new CarlaEngineClient(handle);
        Q_UNUSED(plugin);
    }

    // -------------------------------------

protected:
    void handleProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        if (maxPluginNumber() == 0)
            return;

        // get buffers from RtAudio
        float* insPtr  = (float*)inputBuffer;
        float* outsPtr = (float*)outputBuffer;

        // assert buffers
        CARLA_ASSERT(insPtr);
        CARLA_ASSERT(outsPtr);

        // create temporary audio buffers
        float inBuf1[nframes];
        float inBuf2[nframes];
        float outBuf1[nframes];
        float outBuf2[nframes];

        // initialize audio input
        for (unsigned int i=0; i < nframes*2; i++)
        {
            if (i % 2)
                inBuf2[i/2] = insPtr[i];
            else
                inBuf1[i/2] = insPtr[i];
        }

        // create (real) audio buffers
        float* inBuf[2]  = { inBuf1, inBuf2 };
        float* outBuf[2] = { outBuf1, outBuf2 };

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

        processRack(inBuf, outBuf, nframes);

        // output audio
        for (unsigned int i=0; i < nframes*2; i++)
        {
            if (i % 2)
                outsPtr[i] = outBuf2[i/2];
            else
                outsPtr[i] = outBuf1[i/2];
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

    // -------------------------------------

private:
    RtAudio audio;
    MidiInApi*  midiIn;
    MidiOutApi* midiOut;

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        CarlaEngineRtAudio* const _this_ = (CarlaEngineRtAudio*)userData;
        _this_->handleProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newRtAudio(RtAudio::Api api)
{
    return new CarlaEngineRtAudio(api);
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO

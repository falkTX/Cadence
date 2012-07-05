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

#ifndef CARLA_ENGINE_H
#define CARLA_ENGINE_H

#include "carla_shared.h"

#include <cstring>

class QThread;
class CarlaEngineClient;
class CarlaEngineBasePort;

#if defined(CARLA_ENGINE_JACK)
#include <jack/jack.h>
#include <jack/midiport.h>
struct CarlaEngineClientNativeHandle {
    jack_client_t* client;
};
struct CarlaEnginePortNativeHandle {
    jack_client_t* client;
    jack_port_t* port;
    void* buffer;
};
#elif defined(CARLA_ENGINE_RTAUDIO)
#include "RtAudio.h"
//#include <RtMidi.h>
struct CarlaEngineClientNativeHandle {
};
struct CarlaEnginePortNativeHandle {
    void* buffer;
};
#elif defined(CARLA_ENGINE_VST)
#include "carla_vst_includes.h"
struct CarlaEngineClientNativeHandle {
};
struct CarlaEnginePortNativeHandle {
    void* buffer;
};
#else
#error Engine type undefined!
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendEngine Carla Backend Engine
 *
 * The Carla Backend Engine
 * @{
 */

const uint32_t CarlaEngineTimeBBT = 0x1;

enum CarlaEnginePortType {
    CarlaEnginePortTypeAudio,
    CarlaEnginePortTypeControl,
    CarlaEnginePortTypeMIDI
};

enum CarlaEngineControlEventType {
    CarlaEngineEventNull = 0,
    CarlaEngineEventControlChange,
    CarlaEngineEventMidiBankChange,
    CarlaEngineEventMidiProgramChange,
    CarlaEngineEventAllSoundOff,
    CarlaEngineEventAllNotesOff
};

struct CarlaEngineControlEvent {
    CarlaEngineControlEventType type;
    uint32_t time;
    uint8_t channel;
    uint8_t controller;
    double value;
};

struct CarlaEngineMidiEvent {
    uint32_t time;
    uint8_t size;
    uint8_t data[4];
};

struct CarlaTimeInfo {
    bool playing;
    uint32_t frame;
    uint32_t time;
    uint32_t valid;
    struct {
        int32_t bar;
        int32_t beat;
        int32_t tick;
        double bar_start_tick;
        float  beats_per_bar;
        float  beat_type;
        double ticks_per_beat;
        double beats_per_minute;
    } bbt;
};

// -----------------------------------------

class CarlaEngine
{
public:
    CarlaEngine()
    {
        qDebug("CarlaEngine::CarlaEngine()");

        name = nullptr;

        sampleRate = 0.0;
        bufferSize = 0;

        memset(&timeInfo, 0, sizeof(CarlaTimeInfo));
#ifndef BUILD_BRIDGE
        memset(rackPorts, 0, sizeof(CarlaEnginePortNativeHandle)*rackPortCount);
        memset(rackControlEventsIn,  0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
        memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
        memset(rackMidiEventsIn,  0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
        memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
#endif
    }

    virtual ~CarlaEngine()
    {
        qDebug("CarlaEngine::~CarlaEngine()");

        if (name)
            free((void*)name);
    }

    // -------------------------------------

    virtual bool init(const char* const name) = 0;
    virtual bool close() = 0;

    virtual bool isOnAudioThread() = 0;
    virtual bool isOffline() = 0;
    virtual bool isRunning() = 0;

    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin) = 0;

    // -------------------------------------

    const char* getName() const
    {
        return name;
    }

    double getSampleRate() const
    {
        return sampleRate;
    }

    uint32_t getBufferSize() const
    {
#ifndef BUILD_BRIDGE
        if (carla_options.proccess_hq)
            return 8;
#endif
        return bufferSize;
    }

    const CarlaTimeInfo* getTimeInfo() const
    {
        return &timeInfo;
    }

    static int maxClientNameSize();
    static int maxPortNameSize();

    // -------------------------------------

#ifndef BUILD_BRIDGE
    // rack mode
    static const unsigned short MAX_ENGINE_CONTROL_EVENTS = 512;
    static const unsigned short MAX_ENGINE_MIDI_EVENTS    = 512;
    static const unsigned short rackPortAudioIn1   = 0;
    static const unsigned short rackPortAudioIn2   = 1;
    static const unsigned short rackPortAudioOut1  = 2;
    static const unsigned short rackPortAudioOut2  = 3;
    static const unsigned short rackPortControlIn  = 4;
    static const unsigned short rackPortControlOut = 5;
    static const unsigned short rackPortMidiIn     = 6;
    static const unsigned short rackPortMidiOut    = 7;
    static const unsigned short rackPortCount      = 8;
    CarlaEnginePortNativeHandle rackPorts[rackPortCount];
    CarlaEngineControlEvent rackControlEventsIn[MAX_ENGINE_CONTROL_EVENTS];
    CarlaEngineControlEvent rackControlEventsOut[MAX_ENGINE_CONTROL_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsIn[MAX_ENGINE_MIDI_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsOut[MAX_ENGINE_MIDI_EVENTS];
#endif

    // -------------------------------------

protected:
    const char* name;
    double sampleRate;
    uint32_t bufferSize;
    CarlaTimeInfo timeInfo;
};

#if defined(CARLA_ENGINE_JACK)
class CarlaEngineJack : public CarlaEngine
{
public:
    CarlaEngineJack();
    ~CarlaEngineJack();

    // -------------------------------------

    bool init(const char* const name);
    bool close();

    bool isOnAudioThread();
    bool isOffline();
    bool isRunning();

    CarlaEngineClient* addClient(CarlaPlugin* const plugin);

    // -------------------------------------

    void handleSampleRateCallback(double newSampleRate);
    void handleBufferSizeCallback(uint32_t newBufferSize);
    void handleFreewheelCallback(bool isFreewheel);
    void handleProcessCallback(uint32_t nframes);
    void handleShutdownCallback();

    // -------------------------------------

private:
    jack_client_t* client;
    jack_transport_state_t state;
    jack_position_t pos;
    bool freewheel;
    QThread* procThread;
};
#elif defined(CARLA_ENGINE_RTAUDIO)
class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio();
    ~CarlaEngineRtAudio();

    // -------------------------------------

    bool init(const char* const name);
    bool close();

    bool isOnAudioThread();
    bool isOffline();
    bool isRunning();

    CarlaEngineClient* addClient(CarlaPlugin* const plugin);
};
#elif defined(CARLA_ENGINE_VST)
class CarlaEngineVst : public CarlaEngine
{
public:
    CarlaEngineVst(audioMasterCallback callback);
    ~CarlaEngineVst();

    // -------------------------------------

    bool init(const char* const name);
    bool close();

    bool isOnAudioThread();
    bool isOffline();
    bool isRunning();

    CarlaEngineClient* addClient(CarlaPlugin* const plugin);

    // -------------------------------------

    intptr_t callback(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        return m_callback(&effect, opcode, index, value, ptr, opt);
    }

    const AEffect* getEffect() const
    {
        return &effect;
    }

    // -------------------------------------

    intptr_t handleDispatch(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
    float handleGetParameter(int32_t index);
    void handleSetParameter(int32_t index, float value);
    void handleProcessReplacing(float** inputs, float** outputs, int32_t frames);

private:
    AEffect effect;
    const audioMasterCallback m_callback;
    //CarlaCheckThread checkThread;
};
#endif

// -----------------------------------------

class CarlaEngineClient
{
public:
    CarlaEngineClient(const CarlaEngineClientNativeHandle& handle, bool active);
    ~CarlaEngineClient();

    void activate();
    void deactivate();

    bool isActive() const;
    bool isOk() const;

    const CarlaEngineBasePort* addPort(CarlaEnginePortType type, const char* const name, bool isInput);

private:
    bool m_active;
    const CarlaEngineClientNativeHandle handle;
};

// -----------------------------------------

class CarlaEngineBasePort
{
public:
    CarlaEngineBasePort(CarlaEnginePortNativeHandle& handle, bool isInput);
    virtual ~CarlaEngineBasePort();

    virtual void initBuffer(CarlaEngine* const engine) = 0;

protected:
    const bool isInput;
    CarlaEnginePortNativeHandle handle;
};

// -----------------------------------------

class CarlaEngineAudioPort : public CarlaEngineBasePort
{
public:
    CarlaEngineAudioPort(CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

#ifdef CARLA_ENGINE_JACK
    float* getJackAudioBuffer(uint32_t nframes);
#endif
};

// -----------------------------------------

class CarlaEngineControlPort : public CarlaEngineBasePort
{
public:
    CarlaEngineControlPort(CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineControlEvent* getEvent(uint32_t index);

    void writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value);
};

// -----------------------------------------

class CarlaEngineMidiPort : public CarlaEngineBasePort
{
public:
    CarlaEngineMidiPort(CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineMidiEvent* getEvent(uint32_t index);

    void writeEvent(uint32_t time, uint8_t* data, uint8_t size);
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_H

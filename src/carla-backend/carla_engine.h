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

#include "carla_backend.h"

#if defined(CARLA_ENGINE_JACK)
#include <jack/jack.h>
#include <jack/midiport.h>

struct CarlaEngineClientNativeHandle {
    jack_client_t* client;
};

struct CarlaEnginePortNativeHandle {
    jack_port_t* port;
    void* buffer;
};
#elif defined(CARLA_ENGINE_RTAUDIO)
#include "RtAudio.h"
//#include <RtMidi.h>

typedef void* CarlaEngineClientNativeHandle;
typedef void* CarlaEnginePortNativeHandle;
#elif defined(CARLA_ENGINE_VST)
#include "carla_vst_includes.h"
typedef void* CarlaEngineClientNativeHandle;
typedef void* CarlaEnginePortNativeHandle;
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
    CarlaEngine();
    ~CarlaEngine();

    bool init(const char* name);
    bool close();
    bool isOnAudioThread();
    bool isOffline();
    const CarlaTimeInfo* getTimeInfo();

    static int maxClientNameSize();
    static int maxPortNameSize();
};

// -----------------------------------------

class CarlaEngineBasePort
{
public:
    CarlaEngineBasePort(CarlaEngineClientNativeHandle* const client, bool isInput);
    virtual ~CarlaEngineBasePort();

    virtual void initBuffer() = 0;

protected:
    const bool isInput;
    CarlaEnginePortNativeHandle handle;
    CarlaEngineClientNativeHandle* const client;
};

// -----------------------------------------

class CarlaEngineClient
{
public:
    CarlaEngineClient(CarlaPlugin* const plugin);
    ~CarlaEngineClient();

    void activate();
    void deactivate();

    bool isActive();
    bool isOk();

    CarlaEngineBasePort* addPort(const char* name, CarlaEnginePortType type, bool isInput);

private:
    CarlaEngineClientNativeHandle handle;
    bool m_active;
};

// -----------------------------------------

class CarlaEngineAudioPort : public CarlaEngineBasePort
{
public:
    CarlaEngineAudioPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput);

    void initBuffer();

#ifdef CARLA_ENGINE_JACK
    float* getJackAudioBuffer();
#endif
};

// -----------------------------------------

class CarlaEngineControlPort : public CarlaEngineBasePort
{
public:
    CarlaEngineControlPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput);

    void initBuffer();

    uint32_t getEventCount();
    const CarlaEngineControlEvent* getEvent(uint32_t index);

    void writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value);
};

// -----------------------------------------

class CarlaEngineMidiPort : public CarlaEngineBasePort
{
public:
    CarlaEngineMidiPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput);

    void initBuffer();

    uint32_t getEventCount();
    const CarlaEngineMidiEvent* getEvent(uint32_t index);

    void writeEvent(uint32_t time, uint8_t* data, uint8_t size);
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_H

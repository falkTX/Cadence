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

#include "carla_osc.h"
#include "carla_threads.h"

#include <cassert>
#include <QtCore/QMutex>

#ifdef CARLA_ENGINE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

#ifdef CARLA_ENGINE_RTAUDIO
#include "RtAudio.h"
//#include "RtMidi.h"
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
    uint8_t  channel;
    uint16_t controller;
    double value;

    CarlaEngineControlEvent()
        : type(CarlaEngineEventNull),
          time(0),
          channel(0),
          controller(0),
          value(0.0) {}
};

struct CarlaEngineMidiEvent {
    uint32_t time;
    uint8_t size;
    uint8_t data[4];

    //    CarlaEngineMidiEvent()
    //        : time(0),
    //          size(0),
    //          data{0} {}
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

    //    CarlaTimeInfo()
    //        : playing(false),
    //          frame(0),
    //          time(0),
    //          valid(0),
    //          bbt{0, 0, 0, 0.0, 0.0f, 0.0f, 0.0, 0.0} {}
};

struct CarlaEngineClientNativeHandle {
#ifdef CARLA_ENGINE_JACK
    jack_client_t* client;
#endif
};

struct CarlaEnginePortNativeHandle {
#ifdef CARLA_ENGINE_JACK
    jack_client_t* client;
    jack_port_t* port;
#endif
};

// -----------------------------------------------------------------------

class CarlaEngineClient;
class CarlaEngineBasePort;

/*!
 * \class CarlaEngine
 *
 * \brief Carla Backend base engine class
 *
 * This is the base class for all available engine types available in Carla Backend.
 */
class CarlaEngine
{
public:
    CarlaEngine();
    virtual ~CarlaEngine();

    // -------------------------------------------------------------------
    // static values

    static const unsigned short MAX_PEAKS = 2;

    static int maxClientNameSize();
    static int maxPortNameSize();

    // -------------------------------------------------------------------
    // virtual, per-engine type calls

    virtual bool init(const char* const name) = 0;
    virtual bool close() = 0;

    virtual bool isOnAudioThread() = 0;
    virtual bool isOffline() = 0;
    virtual bool isRunning() = 0;

    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin) = 0;

    // -------------------------------------------------------------------
    // plugin management

    short getNewPluginIndex();
    CarlaPlugin* getPluginById(unsigned short id);
    CarlaPlugin* getPluginByIndex(unsigned short id);
    const char* getUniqueName(const char* name);

    void addPlugin(unsigned short id, CarlaPlugin* plugin);
    bool removePlugin(unsigned short id);

    // -------------------------------------------------------------------
    // information (base)

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
        return bufferSize;
    }

    const CarlaTimeInfo* getTimeInfo() const
    {
        return &timeInfo;
    }

    // -------------------------------------------------------------------
    // information (audio peaks)

    double getInputPeak(unsigned short pluginId, unsigned short id)
    {
        assert(pluginId < MAX_PLUGINS);
        assert(id < MAX_PEAKS);
        return m_insPeak[pluginId*MAX_PEAKS + id];
    }

    double getOutputPeak(unsigned short pluginId, unsigned short id)
    {
        assert(pluginId < MAX_PLUGINS);
        assert(id < MAX_PEAKS);
        return m_outsPeak[pluginId*MAX_PEAKS + id];
    }

    void setInputPeak(unsigned short pluginId, unsigned short id, double value)
    {
        assert(pluginId < MAX_PLUGINS);
        assert(id < MAX_PEAKS);
        m_insPeak[pluginId*MAX_PEAKS + id] = value;
    }

    void setOutputPeak(unsigned short pluginId, unsigned short id, double value)
    {
        assert(pluginId < MAX_PLUGINS);
        assert(id < MAX_PEAKS);
        m_outsPeak[pluginId*MAX_PEAKS + id] = value;
    }

    // -------------------------------------------------------------------
    // callback

    void callback(CallbackType action, unsigned short pluginId, int value1, int value2, double value3)
    {
        if (m_callback)
            m_callback(action, pluginId, value1, value2, value3);
    }

    void setCallback(CallbackFunc func)
    {
        m_callback = func;
    }

    // -------------------------------------------------------------------
    // mutex locks

    void processLock()
    {
        m_procLock.lock();
    }

    void processUnlock()
    {
        m_procLock.unlock();
    }

    void midiLock()
    {
        m_midiLock.lock();
    }

    void midiUnlock()
    {
        m_midiLock.unlock();
    }

    // -------------------------------------------------------------------
    // osc stuff

    const char* getOscServerPath() const
    {
        return m_osc.getServerPath();
    }

    bool isOscControllerRegisted() const
    {
        return m_osc.isControllerRegistered();
    }

    void osc_send_add_plugin(int plugin_id, const char* plugin_name);
    void osc_send_remove_plugin(int plugin_id);
    void osc_send_set_plugin_data(int plugin_id, int type, int category, int hints, const char* real_name, const char* label, const char* maker, const char* copyright, long unique_id);
    void osc_send_set_plugin_ports(int plugin_id, int ains, int aouts, int mins, int mouts, int cins, int couts, int ctotals);
    void osc_send_set_parameter_value(int plugin_id, int param_id, double value);
    void osc_send_set_parameter_data(int plugin_id, int param_id, int ptype, int hints, const char* name, const char* label, double current);
    void osc_send_set_parameter_ranges(int plugin_id, int param_id, double x_min, double x_max, double x_def, double x_step, double x_step_small, double x_step_large);
    void osc_send_set_parameter_midi_channel(int plugin_id, int parameter_id, int midi_channel);
    void osc_send_set_parameter_midi_cc(int plugin_id, int parameter_id, int midi_cc);
    void osc_send_set_default_value(int plugin_id, int param_id, double value);
    void osc_send_set_program(int plugin_id, int program_id);
    void osc_send_set_program_count(int plugin_id, int program_count);
    void osc_send_set_program_name(int plugin_id, int program_id, const char* program_name);
    void osc_send_set_midi_program(int plugin_id, int midi_program_id);
    void osc_send_set_midi_program_count(int plugin_id, int midi_program_count);
    void osc_send_set_midi_program_data(int plugin_id, int midi_program_id, int bank_id, int program_id, const char* midi_program_name);
    void osc_send_set_input_peak_value(int plugin_id, int port_id, double value);
    void osc_send_set_output_peak_value(int plugin_id, int port_id, double value);
    void osc_send_note_on(int plugin_id, int note, int velo);
    void osc_send_note_off(int plugin_id, int note);
    void osc_send_exit();

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // rack mode

    static const unsigned short MAX_ENGINE_CONTROL_EVENTS = 512;
    static const unsigned short MAX_ENGINE_MIDI_EVENTS    = 512;
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

    void oscInit()
    {
        m_osc.init(name);
    }

    void oscClose()
    {
        m_osc.close();
    }

private:
    CarlaOsc m_osc;
    CarlaCheckThread m_checkThread;

    QMutex m_procLock;
    QMutex m_midiLock;

    CallbackFunc m_callback;
    CarlaPlugin* m_carlaPlugins[MAX_PLUGINS];
    const char*  m_uniqueNames[MAX_PLUGINS];

    double m_insPeak[MAX_PLUGINS * MAX_PEAKS];
    double m_outsPeak[MAX_PLUGINS * MAX_PEAKS];
};

/*!
 * \class CarlaEngineScopedLocker
 *
 * \brief Carla engine scoped locker
 *
 * This is a handy class that temporarily locks an engine during a function scope.
 */
class CarlaEngineScopedLocker
{
public:
    /*!
     * Lock the engine \a engine if \a lock is true.
     * The engine is unlocked in the deconstructor of this class if \a lock is true.
     *
     * \param engine The engine to lock
     * \param lock Wherever to lock the engine or not, true by default
     */
    CarlaEngineScopedLocker(CarlaEngine* const engine, bool lock = true) :
        m_engine(engine),
        m_lock(lock)
    {
        if (m_lock)
            m_engine->processLock();
    }

    ~CarlaEngineScopedLocker()
    {
        if (m_lock)
            m_engine->processUnlock();
    }

private:
    CarlaEngine* const m_engine;
    const bool m_lock;
};

// -----------------------------------------------------------------------

class CarlaEngineClient
{
public:
    CarlaEngineClient(const CarlaEngineClientNativeHandle& handle);
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

// -----------------------------------------------------------------------

class CarlaEngineBasePort
{
public:
    CarlaEngineBasePort(const CarlaEnginePortNativeHandle& handle, bool isInput);
    virtual ~CarlaEngineBasePort();

    virtual void initBuffer(CarlaEngine* const engine) = 0;

protected:
    void* m_buffer;
    const bool isInput;
    const CarlaEnginePortNativeHandle handle;
};


class CarlaEngineAudioPort : public CarlaEngineBasePort
{
public:
    CarlaEngineAudioPort(const CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

#ifdef CARLA_ENGINE_JACK
    float* getJackAudioBuffer(uint32_t nframes);
#endif
};

class CarlaEngineControlPort : public CarlaEngineBasePort
{
public:
    CarlaEngineControlPort(const CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineControlEvent* getEvent(uint32_t index);

    void writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value);
};

class CarlaEngineMidiPort : public CarlaEngineBasePort
{
public:
    CarlaEngineMidiPort(const CarlaEnginePortNativeHandle& handle, bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineMidiEvent* getEvent(uint32_t index);

    void writeEvent(uint32_t time, uint8_t* data, uint8_t size);
};

// -----------------------------------------------------------------------

#ifdef CARLA_ENGINE_JACK
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

    // -------------------------------------

    static const unsigned short rackPortAudioIn1   = 0;
    static const unsigned short rackPortAudioIn2   = 1;
    static const unsigned short rackPortAudioOut1  = 2;
    static const unsigned short rackPortAudioOut2  = 3;
    static const unsigned short rackPortControlIn  = 4;
    static const unsigned short rackPortControlOut = 5;
    static const unsigned short rackPortMidiIn     = 6;
    static const unsigned short rackPortMidiOut    = 7;
    static const unsigned short rackPortCount      = 8;
    jack_port_t* rackJackPorts[rackPortCount];
};
#endif

// -----------------------------------------------------------------------

#ifdef CARLA_ENGINE_RTAUDIO
class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio(RtAudio::Api api);
    ~CarlaEngineRtAudio();

    // -------------------------------------

    bool init(const char* const name);
    bool close();

    bool isOnAudioThread();
    bool isOffline();
    bool isRunning();

    CarlaEngineClient* addClient(CarlaPlugin* const plugin);

    // -------------------------------------

    void handleProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status);

private:
    RtAudio adac;
};
#endif

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_H

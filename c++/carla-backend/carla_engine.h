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

#ifndef CARLA_ENGINE_H
#define CARLA_ENGINE_H

#include "carla_osc.h"
#include "carla_shared.h"
#include "carla_threads.h"

#ifdef CARLA_ENGINE_JACK
#include "carla_jackbridge.h"
#endif

#ifdef CARLA_ENGINE_RTAUDIO
#include "RtAudio.h"
#include "RtMidi.h"
#endif

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendEngine Carla Backend Engine
 *
 * The Carla Backend Engine
 * @{
 */

/*!
 * @defgroup TimeInfoValidHints TimeInfo Valid Hints
 *
 * Various hints used for CarlaTimeInfo::valid.
 * @{
 */
const uint32_t CarlaEngineTimeBBT = 0x1;
/**@}*/

enum CarlaEngineType {
    CarlaEngineTypeNull,
    CarlaEngineTypeJack,
    CarlaEngineTypeRtAudio,
    CarlaEngineTypePluginLv2,
    CarlaEngineTypePluginVst
};

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
    double   value;

    CarlaEngineControlEvent()
        : type(CarlaEngineEventNull),
          time(0),
          channel(0),
          controller(0),
          value(0.0) {}
};

struct CarlaEngineMidiEvent {
    uint32_t time;
    uint8_t  size;
    uint8_t  data[4];

    CarlaEngineMidiEvent()
        : time(0),
      #ifdef Q_COMPILER_INITIALIZER_LISTS
          size(0),
          data{0} {}
      #else
          size(0) { data[0] = data[1] = data[2] = data[3] = 0; }
      #endif
};

struct CarlaTimeInfoBBT {
    int32_t bar;
    int32_t beat;
    int32_t tick;
    double bar_start_tick;
    float  beats_per_bar;
    float  beat_type;
    double ticks_per_beat;
    double beats_per_minute;

    CarlaTimeInfoBBT()
        : bar(0),
          beat(0),
          tick(0),
          bar_start_tick(0.0),
          beats_per_bar(0.0f),
          beat_type(0.0f),
          ticks_per_beat(0.0),
          beats_per_minute(0.0) {}
};

struct CarlaTimeInfo {
    bool playing;
    uint32_t frame;
    uint32_t time;
    uint32_t valid;
    CarlaTimeInfoBBT bbt;

    CarlaTimeInfo()
        : playing(false),
          frame(0),
          time(0),
          valid(0) {}
};

struct CarlaEngineClientNativeHandle {
    CarlaEngineType type;
#ifdef CARLA_ENGINE_JACK
    jack_client_t* jackClient;
#endif

    CarlaEngineClientNativeHandle()
    {
        type = CarlaEngineTypeNull;
#ifdef CARLA_ENGINE_JACK
        jackClient = nullptr;
#endif
    }
};

struct CarlaEnginePortNativeHandle {
#ifdef CARLA_ENGINE_JACK
    jack_client_t* jackClient;
    jack_port_t* jackPort;
#endif

    CarlaEnginePortNativeHandle()
    {
#ifdef CARLA_ENGINE_JACK
        jackClient = nullptr;
        jackPort = nullptr;
#endif
    }
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
    // Static values

    static const unsigned short MAX_PEAKS = 2;

    static int maxClientNameSize();
    static int maxPortNameSize();
    static unsigned short maxPluginNumber();
    static const char* getFixedClientName(const char* const clientName);

    static unsigned int getDriverCount();
    static const char* getDriverName(unsigned int index);
    static CarlaEngine* newDriverByName(const char* driverName);

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    virtual bool init(const char* const clientName);
    virtual bool close();
    virtual bool isOffline() = 0;
    virtual bool isRunning() = 0;

    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin) = 0;

    // -------------------------------------------------------------------
    // Plugin management

    short        getNewPluginId() const;
    CarlaPlugin* getPlugin(const unsigned short id) const;
    CarlaPlugin* getPluginUnchecked(const unsigned short id) const;
    const char*  getUniquePluginName(const char* const name);

    short addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra = nullptr);
    short addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra = nullptr);
    bool  removePlugin(const unsigned short id);
    void  removeAllPlugins();
    void  idlePluginGuis();
#ifndef BUILD_BRIDGE
    void  processRack(float* inBuf[2], float* outBuf[2], uint32_t frames);
#endif

    // bridge, internal use only
    void __bridgePluginRegister(const unsigned short id, CarlaPlugin* const plugin)
    {
        m_carlaPlugins[id] = plugin;
    }

    // -------------------------------------------------------------------
    // Information (base)

    CarlaEngineType getType() const;
    const char* getName() const;
    double   getSampleRate() const;
    uint32_t getBufferSize() const;
    const CarlaTimeInfo* getTimeInfo() const;

    // -------------------------------------------------------------------
    // Information (audio peaks)

    double getInputPeak(const unsigned short pluginId, const unsigned short id) const;
    double getOutputPeak(const unsigned short pluginId, const unsigned short id) const;
    void   setInputPeak(const unsigned short pluginId, const unsigned short id, double value);
    void   setOutputPeak(const unsigned short pluginId, const unsigned short id, double value);

    // -------------------------------------------------------------------
    // Callback

    void callback(const CallbackType action, const unsigned short pluginId, const int value1, const int value2, const double value3);
    void setCallback(const CallbackFunc func, void* const ptr);

    // -------------------------------------------------------------------
    // Mutex locks

    void processLock();
    void processUnlock();
    void midiLock();
    void midiUnlock();

    // -------------------------------------------------------------------
    // OSC Stuff

    bool isOscControlRegisted() const;

#ifndef BUILD_BRIDGE
    const char* getOscServerPathTCP() const;
    const char* getOscServerPathUDP() const;
#else
    void setOscBridgeData(const CarlaOscData* const oscData);
#endif

#ifdef BUILD_BRIDGE
    void osc_send_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_program_count(const int32_t count);
    void osc_send_bridge_midi_program_count(const int32_t count);
    void osc_send_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void osc_send_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit);
    void osc_send_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC);
    void osc_send_bridge_parameter_ranges(const int32_t index, const double def, const double min, const double max, const double step, const double stepSmall, const double stepLarge);
    void osc_send_bridge_program_info(const int32_t index, const char* const name);
    void osc_send_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label);
    void osc_send_bridge_configure(const char* const key, const char* const value);
    void osc_send_bridge_set_parameter_value(const int32_t index, const double value);
    void osc_send_bridge_set_default_value(const int32_t index, const double value);
    void osc_send_bridge_set_program(const int32_t index);
    void osc_send_bridge_set_midi_program(const int32_t index);
    void osc_send_bridge_set_custom_data(const char* const stype, const char* const key, const char* const value);
    void osc_send_bridge_set_chunk_data(const char* const chunkFile);
    void osc_send_bridge_set_inpeak(const int32_t portId, const double value);
    void osc_send_bridge_set_outpeak(const int32_t portId, const double value);
#else
    void osc_send_control_add_plugin_start(const int32_t pluginId, const char* const pluginName);
    void osc_send_control_add_plugin_end(const int32_t pluginId);
    void osc_send_control_remove_plugin(const int32_t pluginId);
    void osc_send_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void osc_send_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals);
    void osc_send_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const double current);
    void osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const double min, const double max, const double def, const double step, const double stepSmall, const double stepLarge);
    void osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc);
    void osc_send_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel);
    void osc_send_control_set_parameter_value(const int32_t pluginId, const int32_t index, const double value);
    void osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const double value);
    void osc_send_control_set_program(const int32_t pluginId, const int32_t index);
    void osc_send_control_set_program_count(const int32_t pluginId, const int32_t count);
    void osc_send_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name);
    void osc_send_control_set_midi_program(const int32_t pluginId, const int32_t index);
    void osc_send_control_set_midi_program_count(const int32_t pluginId, const int32_t count);
    void osc_send_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name);
    void osc_send_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo);
    void osc_send_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note);
    void osc_send_control_set_input_peak_value(const int32_t pluginId, const int32_t portId, const double value);
    void osc_send_control_set_output_peak_value(const int32_t pluginId, const int32_t portId, const double value);
    void osc_send_control_exit();
#endif

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // Rack mode

    static const unsigned short MAX_ENGINE_CONTROL_EVENTS = 512;
    static const unsigned short MAX_ENGINE_MIDI_EVENTS    = 512;
    CarlaEngineControlEvent rackControlEventsIn[MAX_ENGINE_CONTROL_EVENTS];
    CarlaEngineControlEvent rackControlEventsOut[MAX_ENGINE_CONTROL_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsIn[MAX_ENGINE_MIDI_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsOut[MAX_ENGINE_MIDI_EVENTS];
#endif

    // -------------------------------------

    /*!
     * \class ScopedLocker
     *
     * \brief Carla engine scoped locker
     *
     * This is a handy class that temporarily locks an engine during a function scope.
     */
    class ScopedLocker
    {
    public:
        /*!
         * Lock the engine \a engine if \a lock is true.
         * The engine is unlocked in the deconstructor of this class if \a lock is true.
         *
         * \param engine The engine to lock
         * \param lock Wherever to lock the engine or not, true by default
         */
        ScopedLocker(CarlaEngine* const engine, bool lock = true)
            : m_engine(engine),
              m_lock(lock)
        {
            if (m_lock)
                m_engine->processLock();
        }

        ~ScopedLocker()
        {
            if (m_lock)
                m_engine->processUnlock();
        }

    private:
        CarlaEngine* const m_engine;
        const bool m_lock;
    };

    // -------------------------------------

protected:
    CarlaEngineType type;
    const char* name;
    uint32_t bufferSize;
    double   sampleRate;
    CarlaTimeInfo timeInfo;

    void bufferSizeChanged(const uint32_t newBufferSize);

private:
    CarlaCheckThread m_checkThread;

#ifndef BUILD_BRIDGE
    CarlaOsc m_osc;
#endif
    const CarlaOscData* m_oscData;

    QMutex m_procLock;
    QMutex m_midiLock;

    CallbackFunc m_callback;
    void*        m_callbackPtr;

    CarlaPlugin* m_carlaPlugins[MAX_PLUGINS];
    const char*  m_uniqueNames[MAX_PLUGINS];

    double m_insPeak[MAX_PLUGINS * MAX_PEAKS];
    double m_outsPeak[MAX_PLUGINS * MAX_PEAKS];

    static unsigned short m_maxPluginNumber;

#ifdef CARLA_ENGINE_JACK
    static CarlaEngine* newJack();
#endif
#ifdef CARLA_ENGINE_RTAUDIO
    static CarlaEngine* newRtAudio(RtAudio::Api api);
#endif
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

    void recomputeLatencies();

    const CarlaEngineBasePort* addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput);

private:
    bool m_active;
    const CarlaEngineClientNativeHandle handle;
};

// -----------------------------------------------------------------------

// base
class CarlaEngineBasePort
{
public:
    CarlaEngineBasePort(const CarlaEnginePortNativeHandle& handle, const bool isInput);
    virtual ~CarlaEngineBasePort();

    virtual void initBuffer(CarlaEngine* const engine) = 0;

    const CarlaEnginePortNativeHandle& getHandle() const
    {
        return handle;
    }

protected:
    void* buffer;
    const bool isInput;
    const CarlaEnginePortNativeHandle handle;
};

// audio
class CarlaEngineAudioPort : public CarlaEngineBasePort
{
public:
    CarlaEngineAudioPort(const CarlaEnginePortNativeHandle& handle, const bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getLatency() const;
    void setLatency(const uint32_t samples);

#ifdef CARLA_ENGINE_JACK
    float* getJackAudioBuffer(uint32_t nframes);
#endif

private:
    uint32_t latency;
};

// control
class CarlaEngineControlPort : public CarlaEngineBasePort
{
public:
    CarlaEngineControlPort(const CarlaEnginePortNativeHandle& handle, const bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineControlEvent* getEvent(uint32_t index);

    void writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value);
};

// midi
class CarlaEngineMidiPort : public CarlaEngineBasePort
{
public:
    CarlaEngineMidiPort(const CarlaEnginePortNativeHandle& handle, const bool isInput);

    void initBuffer(CarlaEngine* const engine);

    uint32_t getEventCount();
    const CarlaEngineMidiEvent* getEvent(uint32_t index);

    void writeEvent(uint32_t time, const uint8_t* data, uint8_t size);
};

// -----------------------------------------------------------------------

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_H

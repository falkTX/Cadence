/*
 * Carla Engine
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#ifdef CARLA_ENGINE_JACK

#include "carla_engine.hpp"
#include "carla_plugin.hpp"

#include "carla_jackbridge.h"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Engine port (JackAudio)

class CarlaEngineJackAudioPort : public CarlaEngineAudioPort
{
public:
    CarlaEngineJackAudioPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client_, jack_port_t* const port_)
        : CarlaEngineAudioPort(isInput, processMode),
          client(client_),
          port(port_)
    {
        qDebug("CarlaEngineJackAudioPort::CarlaEngineJackAudioPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

        if (processMode == PROCESS_MODE_CONTINUOUS_RACK || processMode == PROCESS_MODE_PATCHBAY)
            CARLA_ASSERT(! port);
    }

    ~CarlaEngineJackAudioPort()
    {
        qDebug("CarlaEngineJackAudioPort::~CarlaEngineJackAudioPort()");

        if (client && port)
            jackbridge_port_unregister(client, port);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        if (! port)
            return CarlaEngineAudioPort::initBuffer(engine);

        buffer = jackbridge_port_get_buffer(port, engine->getBufferSize());
    }

private:
    jack_client_t* const client;
    jack_port_t* const port;

    friend class CarlaEngineJack;
};

// -------------------------------------------------------------------------------------------------------------------
// Engine port (JackControl)

class CarlaEngineJackControlPort : public CarlaEngineControlPort
{
public:
    CarlaEngineJackControlPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client_, jack_port_t* const port_)
        : CarlaEngineControlPort(isInput, processMode),
          client(client_),
          port(port_)
    {
        qDebug("CarlaEngineJackControlPort::CarlaEngineJackControlPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

        if (processMode == PROCESS_MODE_CONTINUOUS_RACK || processMode == PROCESS_MODE_PATCHBAY)
            CARLA_ASSERT(! port);
    }

    ~CarlaEngineJackControlPort()
    {
        qDebug("CarlaEngineJackControlPort::~CarlaEngineJackControlPort()");

        if (client && port)
            jackbridge_port_unregister(client, port);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        if (! port)
            return CarlaEngineControlPort::initBuffer(engine);

        buffer = jackbridge_port_get_buffer(port, engine->getBufferSize());

        if (! isInput)
            jackbridge_midi_clear_buffer(buffer);
    }

    uint32_t getEventCount()
    {
        if (! port)
            return CarlaEngineControlPort::getEventCount();

        if (! isInput)
            return 0;

        return jackbridge_midi_get_event_count(buffer);
    }

    const CarlaEngineControlEvent* getEvent(uint32_t index)
    {
        if (! port)
            return CarlaEngineControlPort::getEvent(index);

        if (! isInput)
            return nullptr;

        static jack_midi_event_t jackEvent;
        static CarlaEngineControlEvent carlaEvent;

        if (jackbridge_midi_event_get(&jackEvent, buffer, index) != 0)
            return nullptr;

        memset(&carlaEvent, 0, sizeof(CarlaEngineControlEvent));

        uint8_t midiStatus  = jackEvent.buffer[0];
        uint8_t midiChannel = midiStatus & 0x0F;

        carlaEvent.time    = jackEvent.time;
        carlaEvent.channel = midiChannel;

        if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
        {
            uint8_t midiControl = jackEvent.buffer[1];

            if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
            {
                uint8_t midiBank = jackEvent.buffer[2];
                carlaEvent.type  = CarlaEngineMidiBankChangeEvent;
                carlaEvent.value = midiBank;
            }
            else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
            {
                carlaEvent.type = CarlaEngineAllSoundOffEvent;
            }
            else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
            {
                carlaEvent.type = CarlaEngineAllNotesOffEvent;
            }
            else
            {
                uint8_t midiValue     = jackEvent.buffer[2];
                carlaEvent.type       = CarlaEngineParameterChangeEvent;
                carlaEvent.controller = midiControl;
                carlaEvent.value      = double(midiValue)/127;
            }

            return &carlaEvent;
        }
        if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
        {
            uint8_t midiProgram = jackEvent.buffer[1];
            carlaEvent.type  = CarlaEngineMidiProgramChangeEvent;
            carlaEvent.value = midiProgram;

            return &carlaEvent;
        }

        return nullptr;
    }

    void writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint16_t controller, double value)
    {
        if (! port)
            return CarlaEngineControlPort::writeEvent(type, time, channel, controller, value);

        if (isInput)
            return;

        if (type == CarlaEngineParameterChangeEvent && MIDI_IS_CONTROL_BANK_SELECT(controller))
            type = CarlaEngineMidiBankChangeEvent;

        uint8_t data[3] = { 0 };

        switch (type)
        {
        case CarlaEngineNullEvent:
            break;
        case CarlaEngineParameterChangeEvent:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = controller;
            data[2] = value * 127;
            jackbridge_midi_event_write(buffer, time, data, 3);
            break;
        case CarlaEngineMidiBankChangeEvent:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = value;
            jackbridge_midi_event_write(buffer, time, data, 3);
            break;
        case CarlaEngineMidiProgramChangeEvent:
            data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
            data[1] = value;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        case CarlaEngineAllSoundOffEvent:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        case CarlaEngineAllNotesOffEvent:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        }
    }

private:
    jack_client_t* const client;
    jack_port_t* const port;
};

// -------------------------------------------------------------------------------------------------------------------
// Engine port (JackMIDI)

class CarlaEngineJackMidiPort : public CarlaEngineMidiPort
{
public:
    CarlaEngineJackMidiPort(const bool isInput, const ProcessMode processMode, jack_client_t* const client_, jack_port_t* const port_)
        : CarlaEngineMidiPort(isInput, processMode),
          client(client_),
          port(port_)
    {
        qDebug("CarlaEngineJackMidiPort::CarlaEngineJackMidiPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

        if (processMode == PROCESS_MODE_CONTINUOUS_RACK || processMode == PROCESS_MODE_PATCHBAY)
            CARLA_ASSERT(! port);
    }

    ~CarlaEngineJackMidiPort()
    {
        qDebug("CarlaEngineJackMidiPort::~CarlaEngineJackMidiPort()");

        if (client && port)
            jackbridge_port_unregister(client, port);
    }

    void initBuffer(CarlaEngine* const engine)
    {
        if (! port)
            return CarlaEngineMidiPort::initBuffer(engine);

        buffer = jackbridge_port_get_buffer(port, engine->getBufferSize());

        if (! isInput)
            jackbridge_midi_clear_buffer(buffer);
    }

    uint32_t getEventCount()
    {
        if (! port)
            return CarlaEngineMidiPort::getEventCount();

        if (! isInput)
            return 0;

        return jackbridge_midi_get_event_count(buffer);
    }

    const CarlaEngineMidiEvent* getEvent(uint32_t index)
    {
        if (! port)
            return CarlaEngineMidiPort::getEvent(index);

        if (! isInput)
            return nullptr;

        static jack_midi_event_t jackEvent;
        static CarlaEngineMidiEvent carlaEvent;

        if (jackbridge_midi_event_get(&jackEvent, buffer, index) == 0 && jackEvent.size <= 4)
        {
            carlaEvent.time = jackEvent.time;
            carlaEvent.size = jackEvent.size;
            memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);
            return &carlaEvent;
        }

        return nullptr;
    }

    void writeEvent(uint32_t time, const uint8_t* data, uint8_t size)
    {
        if (! port)
            return CarlaEngineMidiPort::writeEvent(time, data, size);

        if (isInput)
            return;

        jackbridge_midi_event_write(buffer, time, data, size);
    }

private:
    jack_client_t* const client;
    jack_port_t* const port;
};

// -------------------------------------------------------------------------------------------------------------------
// Jack Engine client

class CarlaEngineJackClient : public CarlaEngineClient
{
public:
    CarlaEngineJackClient(jack_client_t* const client_, const CarlaEngineType engineType, const ProcessMode processMode)
        : CarlaEngineClient(engineType, processMode),
          client(client_)
    {
    }

    ~CarlaEngineJackClient()
    {
        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (client)
                jackbridge_client_close(client);
        }
    }

    void activate()
    {
        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (client && ! isActive())
                jackbridge_activate(client);
        }

        CarlaEngineClient::activate();
    }

    void deactivate()
    {
        CarlaEngineClient::deactivate();

        if (processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            if (client && isActive())
                jackbridge_deactivate(client);
        }
    }

    bool isOk() const
    {
        if (processMode != PROCESS_MODE_CONTINUOUS_RACK)
            return bool(client);

        return CarlaEngineClient::isOk();
    }

    void setLatency(const uint32_t samples)
    {
        CarlaEngineClient::setLatency(samples);

        if (processMode != PROCESS_MODE_CONTINUOUS_RACK)
            jackbridge_recompute_total_latencies(client);
    }

    const CarlaEngineBasePort* addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput)
    {
        qDebug("CarlaEngineClient::addPort(%i, \"%s\", %s)", portType, name, bool2str(isInput));

        jack_port_t* port = nullptr;

        // Create Jack port if needed
        if (processMode != PROCESS_MODE_CONTINUOUS_RACK)
        {
            switch (portType)
            {
            case CarlaEnginePortTypeNull:
                break;
            case CarlaEnginePortTypeAudio:
                port = jackbridge_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case CarlaEnginePortTypeControl:
            case CarlaEnginePortTypeMIDI:
                port = jackbridge_port_register(client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            }
        }

        // Create Engine port
        switch (portType)
        {
        case CarlaEnginePortTypeNull:
            break;
        case CarlaEnginePortTypeAudio:
            return new CarlaEngineJackAudioPort(isInput, processMode, client, port);
        case CarlaEnginePortTypeControl:
            return new CarlaEngineJackControlPort(isInput, processMode, client, port);
        case CarlaEnginePortTypeMIDI:
            return new CarlaEngineJackMidiPort(isInput, processMode, client, port);
        }

        qCritical("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
        return nullptr;
    }

private:
    jack_client_t* const client;
};

// -------------------------------------------------------------------------------------------------------------------
// Jack Engine

class CarlaEngineJack : public CarlaEngine
{
public:
    CarlaEngineJack()
        : CarlaEngine()
#ifndef BUILD_BRIDGE
# ifdef Q_COMPILER_INITIALIZER_LISTS
        , rackJackPorts{nullptr}
# endif
#endif
    {
        qDebug("CarlaEngineJack::CarlaEngineJack()");

        globalClient = nullptr;
        state        = JackTransportStopped;
        freewheel    = false;

        memset(&pos, 0, sizeof(jack_position_t));

#ifndef BUILD_BRIDGE
# ifndef Q_COMPILER_INITIALIZER_LISTS
        for (unsigned short i=0; i < rackPortCount; i++)
            rackJackPorts[i] = nullptr;
# endif
#endif
    }

    ~CarlaEngineJack()
    {
        qDebug("CarlaEngineJack::~CarlaEngineJack()");
    }

    // -------------------------------------

    bool init(const char* const clientName)
    {
        qDebug("CarlaEngineJack::init(\"%s\")", clientName);

        freewheel = false;
        state = JackTransportStopped;

#ifndef BUILD_BRIDGE
        globalClient = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (globalClient)
        {
            sampleRate = jackbridge_get_sample_rate(globalClient);
            bufferSize = jackbridge_get_buffer_size(globalClient);

            jackbridge_set_sample_rate_callback(globalClient, carla_jack_srate_callback, this);
            jackbridge_set_buffer_size_callback(globalClient, carla_jack_bufsize_callback, this);
            jackbridge_set_freewheel_callback(globalClient, carla_jack_freewheel_callback, this);
            jackbridge_set_process_callback(globalClient, carla_jack_process_callback, this);
            jackbridge_set_latency_callback(globalClient, carla_jack_latency_callback, this);
            jackbridge_on_shutdown(globalClient, carla_jack_shutdown_callback, this);

            if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                rackJackPorts[rackPortAudioIn1]   = jackbridge_port_register(globalClient, "in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortAudioIn2]   = jackbridge_port_register(globalClient, "in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortAudioOut1]  = jackbridge_port_register(globalClient, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortAudioOut2]  = jackbridge_port_register(globalClient, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortControlIn]  = jackbridge_port_register(globalClient, "control-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortControlOut] = jackbridge_port_register(globalClient, "control-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortMidiIn]     = jackbridge_port_register(globalClient, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortMidiOut]    = jackbridge_port_register(globalClient, "midi-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            }

            if (jackbridge_activate(globalClient) == 0)
            {
                name = jackbridge_get_client_name(globalClient);
                name.toBasic();
                CarlaEngine::init(name);
                return true;
            }
            else
            {
                setLastError("Failed to activate the JACK client");
                globalClient = nullptr;
            }
        }
        else
            setLastError("Failed to create new JACK client");

        return false;
#else
        name = clientName;
        name.toBasic();

        CarlaEngine::init(name);
        return true;
#endif
    }

    bool close()
    {
        qDebug("CarlaEngineJack::close()");
        CarlaEngine::close();

#ifdef BUILD_BRIDGE
        globalClient = nullptr;
        return true;
#else
        if (jackbridge_deactivate(globalClient) == 0)
        {
            if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortAudioIn1]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortAudioIn2]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortAudioOut1]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortAudioOut2]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortControlIn]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortControlOut]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortMidiIn]);
                jackbridge_port_unregister(globalClient, rackJackPorts[rackPortMidiOut]);
            }

            if (jackbridge_client_close(globalClient) == 0)
            {
                globalClient = nullptr;
                return true;
            }
            else
                setLastError("Failed to close the JACK client");
        }
        else
            setLastError("Failed to deactivate the JACK client");

        globalClient = nullptr;
#endif
        return false;
    }

    bool isOffline() const
    {
        return freewheel;
    }

    bool isRunning() const
    {
        return bool(globalClient);
    }

    CarlaEngineType type() const
    {
        return CarlaEngineTypeJack;
    }

    CarlaEngineClient* addClient(CarlaPlugin* const plugin)
    {
        jack_client_t* client = nullptr;

#ifdef BUILD_BRIDGE
        client = globalClient = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);

        sampleRate = jackbridge_get_sample_rate(client);
        bufferSize = jackbridge_get_buffer_size(client);

        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jackbridge_set_process_callback(handle.jackClient, carla_jack_process_callback, this);
        jackbridge_set_latency_callback(handle.jackClient, carla_jack_latency_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#else
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            client = globalClient;
        }
        else if (options.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            client = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);
            jackbridge_set_process_callback(client, carla_jack_process_callback_plugin, plugin);
            jackbridge_set_latency_callback(client, carla_jack_latency_callback_plugin, plugin);
        }
#endif

        return new CarlaEngineJackClient(client, CarlaEngineTypeJack, options.processMode);
    }

    // -------------------------------------

protected:
    void handleSampleRateCallback(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

    void handleBufferSizeCallback(uint32_t newBufferSize)
    {
#ifndef BUILD_BRIDGE
        if (options.processHighPrecision)
            return;
#endif

        bufferSizeChanged(newBufferSize);
    }

    void handleFreewheelCallback(bool isFreewheel)
    {
        freewheel = isFreewheel;
    }

    void handleProcessCallback(uint32_t nframes)
    {
#ifndef BUILD_BRIDGE
        if (maxPluginNumber() == 0)
            return;
#endif
        state = jackbridge_transport_query(globalClient, &pos);

        timeInfo.playing = (state != JackTransportStopped);

        if (pos.unique_1 == pos.unique_2)
        {
            timeInfo.frame = pos.frame;
            timeInfo.time  = pos.usecs;

            if (pos.valid & JackPositionBBT)
            {
                timeInfo.valid = CarlaEngineTimeBBT;
                timeInfo.bbt.bar  = pos.bar;
                timeInfo.bbt.beat = pos.beat;
                timeInfo.bbt.tick = pos.tick;
                timeInfo.bbt.bar_start_tick = pos.bar_start_tick;
                timeInfo.bbt.beats_per_bar  = pos.beats_per_bar;
                timeInfo.bbt.beat_type      = pos.beat_type;
                timeInfo.bbt.ticks_per_beat = pos.ticks_per_beat;
                timeInfo.bbt.beats_per_minute = pos.beats_per_minute;
            }
            else
                timeInfo.valid = 0;
        }
        else
        {
            timeInfo.frame = 0;
            timeInfo.valid = 0;
        }

#ifndef BUILD_BRIDGE
        if (options.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
            {
                CarlaPlugin* const plugin = getPluginUnchecked(i);

                if (! plugin)
                    continue;

                plugin->engineProcessLock();

                if (plugin->enabled())
                {
                    plugin->initBuffers();
                    processPlugin(plugin, nframes);
                }
                else
                    processPluginNOT(plugin, nframes);

                plugin->engineProcessUnlock();
            }
        }
        else if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            // get buffers from jack
            float* audioIn1  = (float*)jackbridge_port_get_buffer(rackJackPorts[rackPortAudioIn1], nframes);
            float* audioIn2  = (float*)jackbridge_port_get_buffer(rackJackPorts[rackPortAudioIn2], nframes);
            float* audioOut1 = (float*)jackbridge_port_get_buffer(rackJackPorts[rackPortAudioOut1], nframes);
            float* audioOut2 = (float*)jackbridge_port_get_buffer(rackJackPorts[rackPortAudioOut2], nframes);
            void* controlIn  = jackbridge_port_get_buffer(rackJackPorts[rackPortControlIn], nframes);
            void* controlOut = jackbridge_port_get_buffer(rackJackPorts[rackPortControlOut], nframes);
            void* midiIn     = jackbridge_port_get_buffer(rackJackPorts[rackPortMidiIn], nframes);
            void* midiOut    = jackbridge_port_get_buffer(rackJackPorts[rackPortMidiOut], nframes);

            // assert buffers
            CARLA_ASSERT(audioIn1);
            CARLA_ASSERT(audioIn2);
            CARLA_ASSERT(audioOut1);
            CARLA_ASSERT(audioOut2);
            CARLA_ASSERT(controlIn);
            CARLA_ASSERT(controlOut);
            CARLA_ASSERT(midiIn);
            CARLA_ASSERT(midiOut);

            // create audio buffers
            float* inBuf[2]  = { audioIn1, audioIn2 };
            float* outBuf[2] = { audioOut1, audioOut2 };

            // initialize control input
            memset(rackControlEventsIn, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
            {
                jack_midi_event_t jackEvent;
                const uint32_t jackEventCount = jackbridge_midi_get_event_count(controlIn);

                uint32_t carlaEventIndex = 0;

                for (uint32_t jackEventIndex=0; jackEventIndex < jackEventCount; jackEventIndex++)
                {
                    if (jackbridge_midi_event_get(&jackEvent, controlIn, jackEventIndex) != 0)
                        continue;

                    CarlaEngineControlEvent* const carlaEvent = &rackControlEventsIn[carlaEventIndex++];

                    uint8_t midiStatus  = jackEvent.buffer[0];
                    uint8_t midiChannel = midiStatus & 0x0F;

                    carlaEvent->time    = jackEvent.time;
                    carlaEvent->channel = midiChannel;

                    if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                    {
                        uint8_t midiControl = jackEvent.buffer[1];

                        if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                        {
                            uint8_t midiBank = jackEvent.buffer[2];
                            carlaEvent->type  = CarlaEngineMidiBankChangeEvent;
                            carlaEvent->value = midiBank;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            carlaEvent->type = CarlaEngineAllSoundOffEvent;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            carlaEvent->type = CarlaEngineAllNotesOffEvent;
                        }
                        else
                        {
                            uint8_t midiValue     = jackEvent.buffer[2];
                            carlaEvent->type       = CarlaEngineParameterChangeEvent;
                            carlaEvent->controller = midiControl;
                            carlaEvent->value      = double(midiValue)/127;
                        }
                    }
                    else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                    {
                        uint8_t midiProgram = jackEvent.buffer[1];
                        carlaEvent->type  = CarlaEngineMidiProgramChangeEvent;
                        carlaEvent->value = midiProgram;
                    }
                }
            }

            // initialize midi input
            memset(rackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            {
                uint32_t i = 0, j = 0;
                jack_midi_event_t jackEvent;

                while (jackbridge_midi_event_get(&jackEvent, midiIn, j++) == 0)
                {
                    if (i == MAX_ENGINE_MIDI_EVENTS)
                        break;

                    if (jackEvent.size < 4)
                    {
                        rackMidiEventsIn[i].time = jackEvent.time;
                        rackMidiEventsIn[i].size = jackEvent.size;
                        memcpy(rackMidiEventsIn[i].data, jackEvent.buffer, jackEvent.size);
                        i += 1;
                    }
                }
            }

            // process rack
            processRack(inBuf, outBuf, nframes);

            // output control
            {
                jackbridge_midi_clear_buffer(controlOut);

                for (unsigned short i=0; i < MAX_ENGINE_CONTROL_EVENTS; i++)
                {
                    CarlaEngineControlEvent* const event = &rackControlEventsOut[i];

                    if (event->type == CarlaEngineParameterChangeEvent && MIDI_IS_CONTROL_BANK_SELECT(event->controller))
                        event->type = CarlaEngineMidiBankChangeEvent;

                    uint8_t data[4] = { 0 };

                    switch (event->type)
                    {
                    case CarlaEngineNullEvent:
                        break;
                    case CarlaEngineParameterChangeEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = event->controller;
                        data[2] = event->value * 127;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineMidiBankChangeEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_BANK_SELECT;
                        data[2] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineMidiProgramChangeEvent:
                        data[0] = MIDI_STATUS_PROGRAM_CHANGE + event->channel;
                        data[1] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineAllSoundOffEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineAllNotesOffEvent:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    }
                }
            }

            // output midi
            {
                jackbridge_midi_clear_buffer(midiOut);

                for (unsigned short i=0; i < MAX_ENGINE_MIDI_EVENTS; i++)
                {
                    if (rackMidiEventsOut[i].size == 0)
                        break;

                    jackbridge_midi_event_write(midiOut, rackMidiEventsOut[i].time, rackMidiEventsOut[i].data, rackMidiEventsOut[i].size);
                }
            }
        }
#else
        CarlaPlugin* const plugin = getPluginUnchecked(0);

        if (! plugin)
            return;

        plugin->engineProcessLock();

        if (plugin->enabled())
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
        }
        else
            processPluginNOT(plugin, nframes);

        plugin->engineProcessUnlock();
#endif
    }

    void handleLatencyCallback(jack_latency_callback_mode_t mode)
    {
#ifndef BUILD_BRIDGE
        if (options.processMode != PROCESS_MODE_SINGLE_CLIENT)
            return;
#endif

        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);

            if (plugin && plugin->enabled())
                latencyPlugin(plugin, mode);
        }
    }

    void handleShutdownCallback()
    {
        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);

            if (plugin)
                plugin->x_client = nullptr;
        }

        globalClient = nullptr;
        callback(CALLBACK_QUIT, 0, 0, 0, 0.0);
    }

    // -------------------------------------

private:
    jack_client_t* globalClient;
    jack_transport_state_t state;
    jack_position_t pos;
    bool freewheel;

    // -------------------------------------

#ifndef BUILD_BRIDGE
    enum RackPorts {
        rackPortAudioIn1   = 0,
        rackPortAudioIn2   = 1,
        rackPortAudioOut1  = 2,
        rackPortAudioOut2  = 3,
        rackPortControlIn  = 4,
        rackPortControlOut = 5,
        rackPortMidiIn     = 6,
        rackPortMidiOut    = 7,
        rackPortCount      = 8
    };

    jack_port_t* rackJackPorts[rackPortCount];
#endif

    static void processPlugin(CarlaPlugin* const p, const uint32_t nframes)
    {
        float* inBuffer[p->aIn.count];
        float* outBuffer[p->aOut.count];

#if 0
        for (uint32_t i=0; i < p->aIn.count; i++)
            inBuffer[i] = (float*)jackbridge_port_get_buffer(p->aIn.ports[i]->handle.jackPort, nframes);

        for (uint32_t i=0; i < p->aOut.count; i++)
            outBuffer[i] = (float*)jackbridge_port_get_buffer(p->aOut.ports[i]->handle.jackPort, nframes);
#endif

#ifndef BUILD_BRIDGE
        if (/*options.processHighPrecision*/ 0)
        {
            float* inBuffer2[p->aIn.count];
            float* outBuffer2[p->aOut.count];

            for (uint32_t i=0, j; i < nframes; i += 8)
            {
                for (j=0; j < p->aIn.count; j++)
                    inBuffer2[j] = inBuffer[j] + i;

                for (j=0; j < p->aOut.count; j++)
                    outBuffer2[j] = outBuffer[j] + i;

                p->process(inBuffer2, outBuffer2, 8, i);
            }
        }
        else
#endif
            p->process(inBuffer, outBuffer, nframes);
    }

    static void processPluginNOT(CarlaPlugin* const p, const uint32_t nframes)
    {
        CarlaEngineJackAudioPort* port;
        float* buffer;

        for (uint32_t i=0; i < p->aOut.count; i++)
        {
            port   = (CarlaEngineJackAudioPort*)p->aOut.ports[i];
            buffer = (float*)jackbridge_port_get_buffer(port->port, nframes);
            carla_zeroF(buffer, nframes);
        }
    }

    static void latencyPlugin(CarlaPlugin* const p, jack_latency_callback_mode_t mode)
    {
        jack_latency_range_t range;
        uint32_t pluginLatency = p->x_client->getLatency();

        if (pluginLatency == 0)
            return;

        if (mode == JackCaptureLatency)
        {
            for (uint32_t i=0; i < p->aIn.count; i++)
            {
                uint aOutI = (i >= p->aOut.count) ? p->aOut.count : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)p->aIn.ports[i])->port;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)p->aOut.ports[aOutI])->port;

                jackbridge_port_get_latency_range(portIn, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portOut, mode, &range);
            }
        }
        else
        {
            for (uint32_t i=0; i < p->aOut.count; i++)
            {
                uint aInI = (i >= p->aIn.count) ? p->aIn.count : i;
                jack_port_t* const portIn  = ((CarlaEngineJackAudioPort*)p->aIn.ports[aInI])->port;
                jack_port_t* const portOut = ((CarlaEngineJackAudioPort*)p->aOut.ports[i])->port;

                jackbridge_port_get_latency_range(portOut, mode, &range);
                range.min += pluginLatency;
                range.max += pluginLatency;
                jackbridge_port_set_latency_range(portIn, mode, &range);
            }
        }
    }

    static int carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleSampleRateCallback(newSampleRate);
        return 0;
    }

    static int carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleBufferSizeCallback(newBufferSize);
        return 0;
    }

    static void carla_jack_freewheel_callback(int starting, void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleFreewheelCallback(bool(starting));
    }

    static int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleProcessCallback(nframes);
        return 0;
    }

    static int carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (! plugin)
            return 0;

        plugin->engineProcessLock();

        if (plugin->enabled())
        {
            plugin->initBuffers();
            processPlugin(plugin, nframes);
        }
        else
            processPluginNOT(plugin, nframes);

        plugin->engineProcessUnlock();

        return 0;
    }

    static void carla_jack_latency_callback(jack_latency_callback_mode_t mode, void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleLatencyCallback(mode);
    }

    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t mode, void* arg)
    {
        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin && plugin->enabled())
            latencyPlugin(plugin, mode);
    }

    static void carla_jack_shutdown_callback(void* arg)
    {
        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleShutdownCallback();
    }
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newJack()
{
    return new CarlaEngineJack();
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_JACK

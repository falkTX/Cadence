/*
 * Carla Backend
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

#include "carla_engine.h"
#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------

class CarlaEngineJack : public CarlaEngine
{
public:
    CarlaEngineJack()
        : CarlaEngine()
#ifndef BUILD_BRIDGE
#  ifdef Q_COMPILER_INITIALIZER_LISTS
        , rackJackPorts{nullptr}
#  endif
#endif
    {
        qDebug("CarlaEngineJack::CarlaEngineJack()");

        type = CarlaEngineTypeJack;

        client = nullptr;
        state  = JackTransportStopped;
        freewheel = false;

        memset(&pos, 0, sizeof(jack_position_t));

#ifndef BUILD_BRIDGE
#  ifndef Q_COMPILER_INITIALIZER_LISTS
        for (unsigned short i=0; i < rackPortCount; i++)
            rackJackPorts[i] = nullptr;
#  endif
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
        client = jackbridge_client_open(clientName, JackNullOption, nullptr);

        if (client)
        {
            sampleRate = jackbridge_get_sample_rate(client);
            bufferSize = jackbridge_get_buffer_size(client);

            jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
            jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
            jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
            jackbridge_set_process_callback(client, carla_jack_process_callback, this);
            jackbridge_set_latency_callback(client, carla_jack_latency_callback, this);
            jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);

            if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                rackJackPorts[rackPortAudioIn1]   = jackbridge_port_register(client, "in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortAudioIn2]   = jackbridge_port_register(client, "in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortAudioOut1]  = jackbridge_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortAudioOut2]  = jackbridge_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortControlIn]  = jackbridge_port_register(client, "control-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortControlOut] = jackbridge_port_register(client, "control-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                rackJackPorts[rackPortMidiIn]     = jackbridge_port_register(client, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                rackJackPorts[rackPortMidiOut]    = jackbridge_port_register(client, "midi-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            }

            if (jackbridge_activate(client) == 0)
            {
                name = getFixedClientName(jackbridge_get_client_name(client));
                CarlaEngine::init(name);
                return true;
            }
            else
            {
                setLastError("Failed to activate the JACK client");
                client = nullptr;
            }
        }
        else
            setLastError("Failed to create new JACK client");

        return false;
#else
        name = getFixedClientName(clientName);
        CarlaEngine::init(name);
        return true;
#endif
    }

    bool close()
    {
        qDebug("CarlaEngineJack::close()");
        CarlaEngine::close();

        if (name)
        {
            free((void*)name);
            name = nullptr;
        }

#ifdef BUILD_BRIDGE
        client = nullptr;
        return true;
#else
        if (jackbridge_deactivate(client) == 0)
        {
            if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            {
                jackbridge_port_unregister(client, rackJackPorts[rackPortAudioIn1]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortAudioIn2]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortAudioOut1]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortAudioOut2]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortControlIn]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortControlOut]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortMidiIn]);
                jackbridge_port_unregister(client, rackJackPorts[rackPortMidiOut]);
            }

            if (jackbridge_client_close(client) == 0)
            {
                client = nullptr;
                return true;
            }
            else
                setLastError("Failed to close the JACK client");
        }
        else
            setLastError("Failed to deactivate the JACK client");

        client = nullptr;
#endif
        return false;
    }

    bool isOffline()
    {
        return freewheel;
    }

    bool isRunning()
    {
        return bool(client);
    }

    CarlaEngineClient* addClient(CarlaPlugin* const plugin)
    {
        CarlaEngineClientNativeHandle handle;
        handle.type = CarlaEngineTypeJack;

#ifdef BUILD_BRIDGE
        client = handle.jackClient = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);

        sampleRate = jackbridge_get_sample_rate(client);
        bufferSize = jackbridge_get_buffer_size(client);

        jackbridge_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jackbridge_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jackbridge_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jackbridge_set_process_callback(handle.jackClient, carla_jack_process_callback, this);
        jackbridge_set_latency_callback(handle.jackClient, carla_jack_latency_callback, this);
        jackbridge_on_shutdown(client, carla_jack_shutdown_callback, this);
#else
        if (carlaOptions.processMode == PROCESS_MODE_SINGLE_CLIENT)
        {
            handle.jackClient = client;
        }
        else if (carlaOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            handle.jackClient = jackbridge_client_open(plugin->name(), JackNullOption, nullptr);
            jackbridge_set_process_callback(handle.jackClient, carla_jack_process_callback_plugin, plugin);
            jackbridge_set_latency_callback(handle.jackClient, carla_jack_latency_callback_plugin, plugin);
        }
#endif

        return new CarlaEngineClient(handle);
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
        if (carlaOptions.processHighPrecision)
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
        state = jackbridge_transport_query(client, &pos);

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
        if (carlaOptions.processMode == PROCESS_MODE_SINGLE_CLIENT)
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
        else if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
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
                jackbridge_midi_event_t jackEvent;
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
                            carlaEvent->type  = CarlaEngineEventMidiBankChange;
                            carlaEvent->value = midiBank;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            carlaEvent->type = CarlaEngineEventAllSoundOff;
                        }
                        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            carlaEvent->type = CarlaEngineEventAllNotesOff;
                        }
                        else
                        {
                            uint8_t midiValue     = jackEvent.buffer[2];
                            carlaEvent->type       = CarlaEngineEventControlChange;
                            carlaEvent->controller = midiControl;
                            carlaEvent->value      = double(midiValue)/127;
                        }
                    }
                    else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                    {
                        uint8_t midiProgram = jackEvent.buffer[1];
                        carlaEvent->type  = CarlaEngineEventMidiProgramChange;
                        carlaEvent->value = midiProgram;
                    }
                }
            }

            // initialize midi input
            memset(rackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            {
                uint32_t i = 0, j = 0;
                jackbridge_midi_event_t jackEvent;

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

                    if (event->type == CarlaEngineEventControlChange && MIDI_IS_CONTROL_BANK_SELECT(event->controller))
                        event->type = CarlaEngineEventMidiBankChange;

                    uint8_t data[4] = { 0 };

                    switch (event->type)
                    {
                    case CarlaEngineEventNull:
                        break;
                    case CarlaEngineEventControlChange:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = event->controller;
                        data[2] = event->value * 127;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineEventMidiBankChange:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_BANK_SELECT;
                        data[2] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 3);
                        break;
                    case CarlaEngineEventMidiProgramChange:
                        data[0] = MIDI_STATUS_PROGRAM_CHANGE + event->channel;
                        data[1] = event->value;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineEventAllSoundOff:
                        data[0] = MIDI_STATUS_CONTROL_CHANGE + event->channel;
                        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
                        jackbridge_midi_event_write(controlOut, event->time, data, 2);
                        break;
                    case CarlaEngineEventAllNotesOff:
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

    void handleLatencyCallback()
    {
#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode != PROCESS_MODE_SINGLE_CLIENT)
            return;
#endif

        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);
            latencyPlugin(plugin);
        }
    }

    void handleShutdownCallback()
    {
        for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
        {
            CarlaPlugin* const plugin = getPluginUnchecked(i);
            plugin->x_client = nullptr;
        }

        client = nullptr;
        callback(CALLBACK_QUIT, 0, 0, 0, 0.0);
    }

    // -------------------------------------

private:
    jack_client_t* client;
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

        for (uint32_t i=0; i < p->aIn.count; i++)
            inBuffer[i] = p->aIn.ports[i]->getJackAudioBuffer(nframes);

        for (uint32_t i=0; i < p->aOut.count; i++)
            outBuffer[i] = p->aOut.ports[i]->getJackAudioBuffer(nframes);

#ifndef BUILD_BRIDGE
        if (carlaOptions.processHighPrecision)
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
        for (uint32_t i=0; i < p->aIn.count; i++)
            zeroF(p->aIn.ports[i]->getJackAudioBuffer(nframes), nframes);

        for (uint32_t i=0; i < p->aOut.count; i++)
            zeroF(p->aOut.ports[i]->getJackAudioBuffer(nframes), nframes);
    }

    static void latencyPlugin(CarlaPlugin* const p)
    {
        for (uint32_t i=0; i < p->aIn.count; i++)
        {
            const CarlaEngineAudioPort* const port = p->aIn.ports[i];
            jack_port_t* const jackPort = port->getHandle().jackPort;
            jack_latency_range_t range;

            jackbridge_port_get_latency_range(jackPort, JackPlaybackLatency, &range);

            range.min += port->getLatency();
            range.max += port->getLatency();

            jackbridge_port_set_latency_range(jackPort, JackPlaybackLatency, &range);
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
        if (mode != JackPlaybackLatency)
            return;

        CarlaEngineJack* const _this_ = (CarlaEngineJack*)arg;
        _this_->handleLatencyCallback();
    }

    static void carla_jack_latency_callback_plugin(jack_latency_callback_mode_t mode, void* arg)
    {
        if (mode != JackPlaybackLatency)
            return;

        CarlaPlugin* const plugin = (CarlaPlugin*)arg;

        if (plugin && plugin->enabled())
            latencyPlugin(plugin);
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

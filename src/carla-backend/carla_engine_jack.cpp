/*
 * Carla Backend
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

#include <iostream>
#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// -------------------------------------------------------------------------------------------------------------------
// static JACK<->Engine calls

static int carla_jack_srate_callback(jack_nframes_t newSampleRate, void* arg)
{
    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;
    engine->handleSampleRateCallback(newSampleRate);
    return 0;
}

static int carla_jack_bufsize_callback(jack_nframes_t newBufferSize, void* arg)
{
    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;
    engine->handleBufferSizeCallback(newBufferSize);
    return 0;
}

static void carla_jack_freewheel_callback(int starting, void* arg)
{
    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;
    engine->handleFreewheelCallback(bool(starting));
}

static int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
{
    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;
    engine->handleProcessCallback(nframes);
    return 0;
}

static int carla_jack_process_callback_plugin(jack_nframes_t nframes, void* arg)
{
    CarlaPlugin* const plugin = (CarlaPlugin*)arg;
    if (plugin && plugin->enabled())
    {
        plugin->engineProcessLock();
        plugin->initBuffers();
        plugin->process_jack(nframes);
        plugin->engineProcessUnlock();
    }
    return 0;
}

static void carla_jack_shutdown_callback(void* arg)
{
    CarlaEngineJack* const engine = (CarlaEngineJack*)arg;
    engine->handleShutdownCallback();
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine (JACK)

CarlaEngineJack::CarlaEngineJack() : CarlaEngine()
{
    client = nullptr;
    state  = JackTransportStopped;
    freewheel = false;
    procThread = nullptr;

    memset(&pos, 0, sizeof(jack_position_t));
}

CarlaEngineJack::~CarlaEngineJack()
{
}

// -------------------------------------------------------------------------------------------------------------------

bool CarlaEngineJack::init(const char* name_)
{
    client = jack_client_open(name_, JackNullOption, nullptr);
    state  = JackTransportStopped;
    freewheel = false;
    procThread = nullptr;

    if (client)
    {
        sampleRate = jack_get_sample_rate(client);
        bufferSize = jack_get_buffer_size(client);

        jack_set_sample_rate_callback(client, carla_jack_srate_callback, this);
        jack_set_buffer_size_callback(client, carla_jack_bufsize_callback, this);
        jack_set_freewheel_callback(client, carla_jack_freewheel_callback, this);
        jack_set_process_callback(client, carla_jack_process_callback, this);
        jack_on_shutdown(client, carla_jack_shutdown_callback, this);

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            rackJackPorts[rackPortAudioIn1].port   = jack_port_register(client, "in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            rackJackPorts[rackPortAudioIn2].port   = jack_port_register(client, "in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            rackJackPorts[rackPortAudioOut1].port  = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            rackJackPorts[rackPortAudioOut2].port  = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            rackJackPorts[rackPortControlIn].port  = jack_port_register(client, "control-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
            rackJackPorts[rackPortControlOut].port = jack_port_register(client, "control-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            rackJackPorts[rackPortMidiIn].port     = jack_port_register(client, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
            rackJackPorts[rackPortMidiOut].port    = jack_port_register(client, "midi-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }
#endif

        if (jack_activate(client) == 0)
        {
            // set client name, fixed for OSC usage
            // FIXME - put this in shared?
            char* fixedName = strdup(jack_get_client_name(client));
            for (size_t i=0; i < strlen(fixedName); i++)
            {
                if (! (std::isalpha(fixedName[i]) || std::isdigit(fixedName[i])))
                    fixedName[i] = '_';
            }

            name = strdup(fixedName);
            free((void*)fixedName);

            return true;
        }
        else
        {
            set_last_error("Failed to activate the JACK client");
            client = nullptr;
        }
    }
    else
        set_last_error("Failed to create new JACK client");

    return false;
}

bool CarlaEngineJack::close()
{
    if (name)
    {
        free((void*)name);
        name = nullptr;
    }

    if (jack_deactivate(client) == 0)
    {
#ifndef BUILD_BRIDGE
        if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            jack_port_unregister(client, rackJackPorts[rackPortAudioIn1].port);
            jack_port_unregister(client, rackJackPorts[rackPortAudioIn2].port);
            jack_port_unregister(client, rackJackPorts[rackPortAudioOut1].port);
            jack_port_unregister(client, rackJackPorts[rackPortAudioOut2].port);
            jack_port_unregister(client, rackJackPorts[rackPortControlIn].port);
            jack_port_unregister(client, rackJackPorts[rackPortControlOut].port);
            jack_port_unregister(client, rackJackPorts[rackPortMidiIn].port);
            jack_port_unregister(client, rackJackPorts[rackPortMidiOut].port);
        }
#endif

        if (jack_client_close(client) == 0)
        {
            client = nullptr;
            return true;
        }
        else
            set_last_error("Failed to close the JACK client");
    }
    else
        set_last_error("Failed to deactivate the JACK client");

    client = nullptr;
    return false;
}

bool CarlaEngineJack::isOnAudioThread()
{
    return (QThread::currentThread() == procThread);
}

bool CarlaEngineJack::isOffline()
{
    return freewheel;
}

bool CarlaEngineJack::isRunning()
{
    return bool(client);
}

#if 0
CarlaEngineClient* CarlaEngineJack::addClient(/*CarlaPlugin* const plugin*/)
{
    jack_client_t* jclient = nullptr;
    bool active = false;

#ifndef BUILD_BRIDGE
    switch (carla_options.process_mode)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
        jclient = client;
        active  = bool(jclient);
        break;

    case PROCESS_MODE_MULTIPLE_CLIENTS:
#endif
        jclient = jack_client_open(plugin->name(), JackNullOption, nullptr);

        if (jclient)
            jack_set_process_callback(jclient, carla_jack_process_callback_plugin, plugin);

#ifndef BUILD_BRIDGE
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        jclient = nullptr;
        active  = true;
        break;
    }
#endif

    CarlaEngineClientNativeHandle handle = {
        jclient
    };

    return new CarlaEngineClient(handle, active);
}
#endif

// -------------------------------------------------------------------------------------------------------------------

void CarlaEngineJack::handleSampleRateCallback(double newSampleRate)
{
    sampleRate = newSampleRate;
}

void CarlaEngineJack::handleBufferSizeCallback(uint32_t newBufferSize)
{
    bufferSize = newBufferSize;

#ifndef BUILD_BRIDGE
    if (carla_options.proccess_hq)
        return;
#endif

    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = getPluginByIndex(i);

        if (plugin && plugin->enabled())
            plugin->bufferSizeChanged(newBufferSize);
    }
}

void CarlaEngineJack::handleFreewheelCallback(bool isFreewheel)
{
    freewheel = isFreewheel;
}

void CarlaEngineJack::handleProcessCallback(uint32_t nframes)
{
    if (procThread == nullptr)
        procThread = QThread::currentThread();

    state = jack_transport_query(client, &pos);

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
    if (carla_options.process_mode == PROCESS_MODE_SINGLE_CLIENT)
    {
        for (unsigned short i=0; i < MAX_PLUGINS; i++)
        {
            CarlaPlugin* const plugin = getPluginByIndex(i);

            if (plugin && plugin->enabled())
            {
                plugin->engineProcessLock();
                plugin->initBuffers();
                plugin->process_jack(nframes);
                plugin->engineProcessUnlock();
            }
        }
    }
    else if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        // get buffers from jack
        float* audioIn1  = (float*)jack_port_get_buffer(rackJackPorts[rackPortAudioIn1].port, nframes);
        float* audioIn2  = (float*)jack_port_get_buffer(rackJackPorts[rackPortAudioIn2].port, nframes);
        float* audioOut1 = (float*)jack_port_get_buffer(rackJackPorts[rackPortAudioOut1].port, nframes);
        float* audioOut2 = (float*)jack_port_get_buffer(rackJackPorts[rackPortAudioOut2].port, nframes);
        //void* controlIn  = jack_port_get_buffer(rackJackPorts[rackPortControlIn].port, nframes);
        //void* controlOut = jack_port_get_buffer(rackJackPorts[rackPortControlOut].port, nframes);
        void* midiIn     = jack_port_get_buffer(rackJackPorts[rackPortMidiIn].port, nframes);
        void* midiOut    = jack_port_get_buffer(rackJackPorts[rackPortMidiOut].port, nframes);

        // create temporary audio buffers
        float ains_tmp_buf1[nframes];
        float ains_tmp_buf2[nframes];
        float aouts_tmp_buf1[nframes];
        float aouts_tmp_buf2[nframes];

        float* ains_tmp[2]  = { ains_tmp_buf1, ains_tmp_buf2 };
        float* aouts_tmp[2] = { aouts_tmp_buf1, aouts_tmp_buf2 };

        // initialize audio input
        memcpy(ains_tmp_buf1, audioIn1, sizeof(float)*nframes);
        memcpy(ains_tmp_buf2, audioIn2, sizeof(float)*nframes);

        // initialize control input
        memset(rackControlEventsIn, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
        {
            // TODO
        }

        // initialize midi input
        memset(rackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
        {
            uint32_t i = 0, j = 0;
            jack_midi_event_t jackEvent;

            while (jack_midi_event_get(&jackEvent, midiIn, j++) == 0)
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

        // initialize outputs (zero)
        memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
        memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);
        memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
        memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

        bool processed = false;

        // process plugins
        for (unsigned short i=0; i < MAX_PLUGINS; i++)
        {
            CarlaPlugin* const plugin = getPluginByIndex(i);

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

                if (carla_options.proccess_hq)
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
        memcpy(audioOut1, aouts_tmp_buf1, sizeof(float)*nframes);
        memcpy(audioOut2, aouts_tmp_buf2, sizeof(float)*nframes);

        // output control
        {
            // TODO
        }

        // output midi
        {
            jack_midi_clear_buffer(midiOut);

            for (unsigned short i=0; i < MAX_ENGINE_MIDI_EVENTS; i++)
            {
                if (rackMidiEventsOut[i].size == 0)
                    break;

                jack_midi_event_write(midiOut, rackMidiEventsOut[i].time, rackMidiEventsOut[i].data, rackMidiEventsOut[i].size);
            }
        }
    }
#else
    Q_UNUSED(nframes);
#endif
}

void CarlaEngineJack::handleShutdownCallback()
{
    //for (unsigned short i=0; i < MAX_PLUGINS; i++)
    //{
    // FIXME
    //CarlaPlugin* plugin = CarlaPlugins[i];
    //if (plugin && plugin->id() == plugin_id)
    //    plugin->jack_client = nullptr;
    //}
    client = nullptr;
    procThread = nullptr;
    callback(CALLBACK_QUIT, 0, 0, 0, 0.0);
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_JACK

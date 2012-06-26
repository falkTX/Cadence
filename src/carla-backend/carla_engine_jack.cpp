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

// Global JACK stuff
static jack_client_t* carla_jack_client = nullptr;
static jack_nframes_t carla_buffer_size = 512;
static jack_nframes_t carla_sample_rate = 44100;
static jack_transport_state_t carla_jack_state = JackTransportStopped;
static jack_position_t carla_jack_pos;

static bool carla_jack_is_freewheel = false;
static const char* carla_client_name = nullptr;
static QThread* carla_jack_thread = nullptr;

#ifndef BUILD_BRIDGE
// rack mode
const unsigned short rackPortAudioIn1   = 0;
const unsigned short rackPortAudioIn2   = 1;
const unsigned short rackPortAudioOut1  = 2;
const unsigned short rackPortAudioOut2  = 3;
const unsigned short rackPortControlIn  = 4;
const unsigned short rackPortControlOut = 5;
const unsigned short rackPortMidiIn     = 6;
const unsigned short rackPortMidiOut    = 7;
const unsigned short rackPortCount      = 8;
static jack_port_t* carla_jack_rack_ports[rackPortCount] = { nullptr };
static CarlaEngineMidiEvent carlaRackMidiEventsIn[MAX_MIDI_EVENTS];
static CarlaEngineMidiEvent carlaRackMidiEventsOut[MAX_MIDI_EVENTS];
#endif

// -------------------------------------------------------------------------------------------------------------------
// Exported symbols (API)

bool is_engine_running()
{
    return bool(carla_jack_client);
}

const char* get_host_client_name()
{
    return carla_client_name;
}

quint32 get_buffer_size()
{
    qDebug("get_buffer_size()");
#ifndef BUILD_BRIDGE
    if (carla_options.proccess_hq)
        return 8;
#endif
    return carla_buffer_size;
}

double get_sample_rate()
{
    qDebug("get_sample_rate()");
    return carla_sample_rate;
}

double get_latency()
{
    qDebug("get_latency()");
    return double(carla_buffer_size)/carla_sample_rate*1000;
}

// -------------------------------------------------------------------------------------------------------------------
// static JACK<->Engine calls

static int carla_jack_bufsize_callback(jack_nframes_t new_buffer_size, void*)
{
    carla_buffer_size = new_buffer_size;

#ifndef BUILD_BRIDGE
    if (carla_options.proccess_hq)
        return 0;
#endif

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->enabled())
            plugin->bufferSizeChanged(new_buffer_size);
    }

    return 0;
}

static int carla_jack_srate_callback(jack_nframes_t new_sample_rate, void*)
{
    carla_sample_rate = new_sample_rate;
    return 0;
}

static void carla_jack_freewheel_callback(int starting, void*)
{
    carla_jack_is_freewheel = bool(starting);
}

static int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
{
    if (carla_jack_thread == nullptr)
        carla_jack_thread = QThread::currentThread();

#ifndef BUILD_BRIDGE
    // request time info once (arg only null on global client)
    if (carla_jack_client && ! arg)
#endif
        carla_jack_state = jack_transport_query(carla_jack_client, &carla_jack_pos);

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_SINGLE_CLIENT)
    {
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* plugin = CarlaPlugins[i];
            if (plugin && plugin->enabled())
            {
                carla_proc_lock();
                plugin->process_jack(nframes);
                carla_proc_unlock();
            }
        }
    }
    else if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        // get buffers from jack
        float* audioIn1  = (float*)jack_port_get_buffer(carla_jack_rack_ports[rackPortAudioIn1], nframes);
        float* audioIn2  = (float*)jack_port_get_buffer(carla_jack_rack_ports[rackPortAudioIn2], nframes);
        float* audioOut1 = (float*)jack_port_get_buffer(carla_jack_rack_ports[rackPortAudioOut1], nframes);
        float* audioOut2 = (float*)jack_port_get_buffer(carla_jack_rack_ports[rackPortAudioOut2], nframes);
        //void* controlIn  = jack_port_get_buffer(carla_jack_rack_ports[rackPortControlIn], nframes);
        //void* controlOut = jack_port_get_buffer(carla_jack_rack_ports[rackPortControlOut], nframes);
        void* midiIn     = jack_port_get_buffer(carla_jack_rack_ports[rackPortMidiIn], nframes);
        void* midiOut    = jack_port_get_buffer(carla_jack_rack_ports[rackPortMidiOut], nframes);

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
        {
            // TODO
        }

        // initialize midi input
        memset(carlaRackMidiEventsIn, 0, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
        {
            uint32_t i = 0, j = 0;
            jack_midi_event_t jackEvent;

            while (jack_midi_event_get(&jackEvent, midiIn, j++) == 0)
            {
                if (i == MAX_MIDI_EVENTS)
                    break;

                if (jackEvent.size < 4)
                {
                    carlaRackMidiEventsIn[i].time = jackEvent.time;
                    carlaRackMidiEventsIn[i].size = jackEvent.size;
                    memcpy(carlaRackMidiEventsIn[i].data, jackEvent.buffer, jackEvent.size);
                    i += 1;
                }
            }
        }

        // initialize outputs (zero)
        memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
        memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);
        memset(carlaRackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);

        bool processed = false;

        // process plugins
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* plugin = CarlaPlugins[i];
            if (plugin && plugin->enabled())
            {
                if (processed)
                {
                    // initialize inputs (from previous outputs)
                    memcpy(ains_tmp_buf1, aouts_tmp_buf1, sizeof(float)*nframes);
                    memcpy(ains_tmp_buf2, aouts_tmp_buf2, sizeof(float)*nframes);
                    memcpy(carlaRackMidiEventsIn, carlaRackMidiEventsOut, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);

                    // initialize outputs (zero)
                    memset(aouts_tmp_buf1, 0, sizeof(float)*nframes);
                    memset(aouts_tmp_buf2, 0, sizeof(float)*nframes);
                    memset(carlaRackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
                }

                // process
                carla_proc_lock();

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

                carla_proc_unlock();

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
            memcpy(carlaRackMidiEventsOut, carlaRackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_MIDI_EVENTS);
        }

        // output audio
        memcpy(audioOut1, aouts_tmp_buf1, sizeof(float)*nframes);
        memcpy(audioOut2, aouts_tmp_buf2, sizeof(float)*nframes);

        // output control
        // TODO

        // output midi
        jack_midi_clear_buffer(midiOut);

        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
        {
            if (carlaRackMidiEventsOut[i].size == 0)
                break;

            jack_midi_event_write(midiOut, carlaRackMidiEventsOut[i].time, carlaRackMidiEventsOut[i].data, carlaRackMidiEventsOut[i].size);
        }
    }
    else
#endif
    {
        CarlaPlugin* plugin = (CarlaPlugin*)arg;
        if (plugin && plugin->enabled())
        {
            carla_proc_lock();
            plugin->process_jack(nframes);
            carla_proc_unlock();
        }
    }

    return 0;
}

static void carla_jack_shutdown_callback(void*)
{
    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        // FIXME
        //CarlaPlugin* plugin = CarlaPlugins[i];
        //if (plugin && plugin->id() == plugin_id)
        //    plugin->jack_client = nullptr;
    }
    carla_jack_client = nullptr;
    carla_jack_thread = nullptr;
    callback_action(CALLBACK_QUIT, 0, 0, 0, 0.0);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine

CarlaEngine::CarlaEngine() {}
CarlaEngine::~CarlaEngine() {}

bool CarlaEngine::init(const char* name)
{
    carla_jack_client = jack_client_open(name, JackNullOption, nullptr);

    if (carla_jack_client)
    {
        carla_buffer_size = jack_get_buffer_size(carla_jack_client);
        carla_sample_rate = jack_get_sample_rate(carla_jack_client);

        jack_set_buffer_size_callback(carla_jack_client, carla_jack_bufsize_callback, nullptr);
        jack_set_sample_rate_callback(carla_jack_client, carla_jack_srate_callback, nullptr);
        jack_set_freewheel_callback(carla_jack_client, carla_jack_freewheel_callback, nullptr);
        jack_set_process_callback(carla_jack_client, carla_jack_process_callback, nullptr);
        jack_on_shutdown(carla_jack_client, carla_jack_shutdown_callback, nullptr);

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            carla_jack_rack_ports[rackPortAudioIn1]   = jack_port_register(carla_jack_client, "in1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            carla_jack_rack_ports[rackPortAudioIn2]   = jack_port_register(carla_jack_client, "in2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            carla_jack_rack_ports[rackPortAudioOut1]  = jack_port_register(carla_jack_client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            carla_jack_rack_ports[rackPortAudioOut2]  = jack_port_register(carla_jack_client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            //carla_jack_rack_ports[rackPortControlIn]  = jack_port_register(carla_jack_client, "control-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
            //carla_jack_rack_ports[rackPortControlOut] = jack_port_register(carla_jack_client, "control-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
            carla_jack_rack_ports[rackPortMidiIn]     = jack_port_register(carla_jack_client, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
            carla_jack_rack_ports[rackPortMidiOut]    = jack_port_register(carla_jack_client, "midi-out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }
#endif

        if (jack_activate(carla_jack_client) == 0)
        {
            // set client name, fixed for OSC usage
            // FIXME - put this in shared?
            char* fixed_name = strdup(jack_get_client_name(carla_jack_client));
            for (size_t i=0; i < strlen(fixed_name); i++)
            {
                if (! (std::isalpha(fixed_name[i]) || std::isdigit(fixed_name[i])))
                    fixed_name[i] = '_';
            }

            carla_client_name = strdup(fixed_name);
            free((void*)fixed_name);

            return true;
        }
        else
        {
            set_last_error("Failed to activate the JACK client");
            carla_jack_client = nullptr;
        }
    }
    else
        set_last_error("Failed to create new JACK client");

    return false;
}

bool CarlaEngine::close()
{
    if (carla_client_name)
    {
        free((void*)carla_client_name);
        carla_client_name = nullptr;
    }

    if (jack_deactivate(carla_jack_client) == 0)
    {
#ifndef BUILD_BRIDGE
        if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortAudioIn1]);
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortAudioIn2]);
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortAudioOut1]);
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortAudioOut2]);
            //jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortControlIn]);
            //jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortControlOut]);
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortMidiIn]);
            jack_port_unregister(carla_jack_client, carla_jack_rack_ports[rackPortMidiOut]);
        }
#endif

        if (jack_client_close(carla_jack_client) == 0)
        {
            carla_jack_client = nullptr;
            return true;
        }
        else
            set_last_error("Failed to close the JACK client");
    }
    else
        set_last_error("Failed to deactivate the JACK client");

    carla_jack_client = nullptr;
    return false;
}

const CarlaTimeInfo* CarlaEngine::getTimeInfo()
{
    static CarlaTimeInfo info;
    info.playing = (carla_jack_state != JackTransportStopped);

    if (carla_jack_pos.unique_1 == carla_jack_pos.unique_2)
    {
        info.frame = carla_jack_pos.frame;
        info.time  = carla_jack_pos.usecs;

        if (carla_jack_pos.valid & JackPositionBBT)
        {
            info.valid = CarlaEngineTimeBBT;
            info.bbt.bar  = carla_jack_pos.bar;
            info.bbt.beat = carla_jack_pos.beat;
            info.bbt.tick = carla_jack_pos.tick;
            info.bbt.bar_start_tick = carla_jack_pos.bar_start_tick;
            info.bbt.beats_per_bar  = carla_jack_pos.beats_per_bar;
            info.bbt.beat_type      = carla_jack_pos.beat_type;
            info.bbt.ticks_per_beat = carla_jack_pos.ticks_per_beat;
            info.bbt.beats_per_minute = carla_jack_pos.beats_per_minute;
        }
        else
            info.valid = 0;
    }
    else
    {
        info.frame = 0;
        info.valid = 0;
    }

    return &info;
}

bool CarlaEngine::isOnAudioThread()
{
    return (QThread::currentThread() == carla_jack_thread);
}

bool CarlaEngine::isOffline()
{
    return carla_jack_is_freewheel;
}

int CarlaEngine::maxClientNameSize()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return STR_MAX/2;
#endif
    return jack_client_name_size();
}

int CarlaEngine::maxPortNameSize()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return STR_MAX;
#endif
    return jack_port_name_size();
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(CarlaEngineClientNativeHandle* const clientHandle, bool isInput_) :
    isInput(isInput_),
    client(clientHandle)
{
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#endif
    {
        if (client && handle)
            jack_port_unregister(client, handle);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(CarlaPlugin* const plugin)
{
#ifndef BUILD_BRIDGE
    switch (carla_options.process_mode)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
        handle   = carla_jack_client;
        m_active = bool(carla_jack_client);
        break;

    case PROCESS_MODE_MULTIPLE_CLIENTS:
#endif
        handle   = jack_client_open(plugin->name(), JackNullOption, nullptr);
        m_active = false;

        if (handle)
            jack_set_process_callback(handle, carla_jack_process_callback, plugin);
#ifndef BUILD_BRIDGE
        break;

    case PROCESS_MODE_CONTINUOUS_RACK:
        handle   = nullptr;
        m_active = bool(carla_jack_client);
        break;
    }
#endif
}

CarlaEngineClient::~CarlaEngineClient()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle)
            jack_client_close(handle);
    }
}

void CarlaEngineClient::activate()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle && ! m_active)
            jack_activate(handle);
    }

    m_active = true;
}

void CarlaEngineClient::deactivate()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle && m_active)
            jack_deactivate(handle);
    }

    m_active = false;
}

bool CarlaEngineClient::isActive()
{
    return m_active;
}

bool CarlaEngineClient::isOk()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return true;
#endif
    return bool(handle);
}

CarlaEngineBasePort* CarlaEngineClient::addPort(const char* name, CarlaEnginePortType type, bool isInput)
{
    switch (type)
    {
    case CarlaEnginePortTypeAudio:
        return new CarlaEngineAudioPort(handle, name, isInput);
    case CarlaEnginePortTypeControl:
        return new CarlaEngineControlPort(handle, name, isInput);
    case CarlaEnginePortTypeMIDI:
        return new CarlaEngineMidiPort(handle, name, isInput);
    default:
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        handle = nullptr;
    else
#endif
        handle = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineAudioPort::getBuffer()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
    else
#endif
        return jack_port_get_buffer(handle, carla_buffer_size);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        handle = nullptr;
    else
#endif
        handle = jack_port_register(client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineControlPort::getBuffer()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
#endif
    return jack_port_get_buffer(handle, carla_buffer_size);
}

void CarlaEngineControlPort::initBuffer(void* buffer)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return;
#endif
    if (isInput)
        return;

    jack_midi_clear_buffer(buffer);
}

uint32_t CarlaEngineControlPort::getEventCount(void* buffer)
{
    if (! isInput)
        return 0;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return 0; // TODO
#endif

    return jack_midi_get_event_count(buffer);
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(void* buffer, uint32_t index)
{
    if (! isInput)
        return nullptr;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr; // TODO
#endif

    static jack_midi_event_t jackEvent;
    static CarlaEngineControlEvent carlaEvent;

    if (jack_midi_event_get(&jackEvent, buffer, index) != 0)
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
            carlaEvent.type  = CarlaEngineEventMidiBankChange;
            carlaEvent.value = midiBank;
        }
        else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
        {
            carlaEvent.type = CarlaEngineEventAllSoundOff;
        }
        else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
        {
            carlaEvent.type = CarlaEngineEventAllNotesOff;
        }
        else
        {
            uint8_t midiValue     = jackEvent.buffer[2];
            carlaEvent.type       = CarlaEngineEventControlChange;
            carlaEvent.controller = midiControl;
            carlaEvent.value      = double(midiValue)/127;
        }

        return &carlaEvent;
    }
    else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
    {
        uint8_t midiProgram = jackEvent.buffer[1];
        carlaEvent.type  = CarlaEngineEventMidiProgramChange;
        carlaEvent.value = midiProgram;

        return &carlaEvent;
    }

    return nullptr;
}

void CarlaEngineControlPort::writeEvent(void* buffer, CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return; // TODO
#endif

    if (type == CarlaEngineEventControlChange && MIDI_IS_CONTROL_BANK_SELECT(controller))
        type = CarlaEngineEventMidiBankChange;

    uint8_t data[4] = { 0 };

    switch (type)
    {
    case CarlaEngineEventControlChange:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = controller;
        data[2] = value * 127;
        jack_midi_event_write(buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiBankChange:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = value;
        jack_midi_event_write(buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiProgramChange:
        data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
        data[1] = value;
        jack_midi_event_write(buffer, time, data, 2);
        break;
    case CarlaEngineEventAllSoundOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
        jack_midi_event_write(buffer, time, data, 2);
        break;
    case CarlaEngineEventAllNotesOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
        jack_midi_event_write(buffer, time, data, 2);
        break;
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(CarlaEngineClientNativeHandle* const client, const char* name, bool isInput) :
    CarlaEngineBasePort(client, isInput)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        handle = nullptr;
    else
#endif
        handle = jack_port_register(client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
}

void* CarlaEngineMidiPort::getBuffer()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
#endif
    return jack_port_get_buffer(handle, carla_buffer_size);
}

void CarlaEngineMidiPort::initBuffer(void* buffer)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return;
#endif
    if (isInput)
        return;

    jack_midi_clear_buffer(buffer);
}

uint32_t CarlaEngineMidiPort::getEventCount(void* buffer)
{
    if (! isInput)
        return 0;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
        {
            if (carlaRackMidiEventsIn[i].size > 0)
                count++;
        }
        return count;
    }
#endif
    return jack_midi_get_event_count(buffer);
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(void* buffer, uint32_t index)
{
    if (! isInput)
        return nullptr;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (index < MAX_MIDI_EVENTS)
            return &carlaRackMidiEventsIn[index];
    }
    else
#endif
    {
        static jack_midi_event_t jackEvent;
        static CarlaEngineMidiEvent carlaEvent;

        if (jack_midi_event_get(&jackEvent, buffer, index) != 0)
            return nullptr;

        if (jackEvent.size < 4)
        {
            carlaEvent.time = jackEvent.time;
            carlaEvent.size = jackEvent.size;
            memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);
            return &carlaEvent;
        }
    }

    return nullptr;
}

void CarlaEngineMidiPort::writeEvent(void* buffer, uint32_t time, uint8_t* data, uint8_t size)
{
    if (isInput)
        return;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (size >= 4)
            return;

        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
        {
            if (carlaRackMidiEventsOut[i].size == 0)
            {
                carlaRackMidiEventsOut[i].time = time;
                carlaRackMidiEventsOut[i].size = size;
                memcpy(carlaRackMidiEventsOut[i].data, data, size);
                break;
            }
        }
    }
    else
#endif
        jack_midi_event_write(buffer, time, data, size);
}

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_JACK

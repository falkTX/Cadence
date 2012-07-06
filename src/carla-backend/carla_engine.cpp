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

#include "carla_engine.h"
#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// define max possible client name
const unsigned short MaxClientNameSize = CarlaEngine::maxClientNameSize() - 5; // 5 = strlen(" (10)")

// -------------------------------------------------------------------------------------------------------------------

CarlaEngine::CarlaEngine() :
    m_checkThread(this)
{
    qDebug("CarlaEngine::CarlaEngine()");

    name = nullptr;

    sampleRate = 0.0;
    bufferSize = 0;

    memset(&timeInfo, 0, sizeof(CarlaTimeInfo));

#ifndef BUILD_BRIDGE
    memset(rackControlEventsIn,  0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    memset(rackMidiEventsIn,  0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
    memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
#  ifdef CARLA_ENGINE_JACK
    memset(rackJackPorts, 0, sizeof(CarlaEnginePortNativeHandle)*rackPortCount);
#  endif
#endif
}

CarlaEngine::~CarlaEngine()
{
    qDebug("CarlaEngine::~CarlaEngine()");

    if (name)
        free((void*)name);
}

// -------------------------------------------------------------------------------------------------------------------

const char* CarlaEngine::getName() const
{
    return name;
}

double CarlaEngine::getSampleRate() const
{
    return sampleRate;
}

uint32_t CarlaEngine::getBufferSize() const
{
    return bufferSize;
}

const CarlaTimeInfo* CarlaEngine::getTimeInfo() const
{
    return &timeInfo;
}

// -------------------------------------------------------------------------------------------------------------------

void CarlaEngine::processLock()
{
    m_procLock.lock();
}

void CarlaEngine::processUnlock()
{
    m_procLock.unlock();
}

void CarlaEngine::midiLock()
{
    m_midiLock.lock();
}

void CarlaEngine::midiUnlock()
{
    m_midiLock.unlock();
}

bool CarlaEngine::isCheckThreadRunning()
{
    return m_checkThread.isRunning();
}

void CarlaEngine::startCheckThread()
{
    m_checkThread.start(QThread::HighPriority);
}

void CarlaEngine::stopCheckThread()
{
    m_checkThread.stopNow();
}

short CarlaEngine::getNewPluginIndex()
{
#ifdef BUILD_BRIDGE
    const unsigned short max = MAX_PLUGINS;
#else
    const unsigned short max = (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK) ? 16 : MAX_PLUGINS;
#endif

    for (unsigned short i=0; i < max; i++)
    {
        if (m_carlaPlugins[i] == nullptr)
            return i;
    }

    return -1;
}

CarlaPlugin* CarlaEngine::getPluginById(unsigned short id)
{
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = m_carlaPlugins[i];

        if (plugin && plugin->id() == id)
            return plugin;
    }

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginByIndex(unsigned short index)
{
    assert(index < MAX_PLUGINS);
    return m_carlaPlugins[index];
}

const char* CarlaEngine::getUniqueName(const char* name)
{
    QString qname(name);

    if (qname.isEmpty())
        qname = "(No name)";

    qname.truncate(MaxClientNameSize-1);
    qname.replace(":", "."); // ":" is used in JACK to split client/port names

    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        // Check if unique name already exists
        if (m_uniqueNames[i] && qname == m_uniqueNames[i])
        {
            // Check if string has already been modified
            uint len = qname.size();

            // 1 digit, ex: " (2)"
            if (qname.at(len-4) == QChar(' ') && qname.at(len-3) == QChar('(') && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                int number = qname.at(len-2).toAscii()-'0';

                if (number == 9)
                    // next number is 10, 2 digits
                    qname.replace(" (9)", " (10)");
                else
                    qname[len-2] = QChar('0'+number+1);

                continue;
            }

            // 2 digits, ex: " (11)"
            if (qname.at(len-5) == QChar(' ') && qname.at(len-4) == QChar('(') && qname.at(len-3).isDigit() && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                QChar n2 = qname.at(len-2);
                QChar n3 = qname.at(len-3);

                if (n2 == QChar('9'))
                {
                    n2 = QChar('0');
                    n3 = QChar(n3.toAscii()+1);
                }
                else
                    n2 = QChar(n2.toAscii()+1);

                qname[len-2] = n2;
                qname[len-3] = n3;

                continue;
            }

            // Modify string if not
            qname += " (2)";
        }
    }

    return strdup(qname.toUtf8().constData());
}

void CarlaEngine::addPlugin(unsigned short id, CarlaPlugin* plugin)
{
    assert(id < MAX_PLUGINS);
    m_carlaPlugins[id] = plugin;
    m_uniqueNames[id]  = plugin->name();
}

bool CarlaEngine::removePlugin(unsigned short id)
{
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = m_carlaPlugins[i];

        if (plugin && plugin->id() == id)
        {
            processLock();
            plugin->setEnabled(false);
            processUnlock();

            if (/*carla_engine.isRunning() &&*/ isCheckThreadRunning())
                stopCheckThread();

            delete plugin;

            m_carlaPlugins[i] = nullptr;
            m_uniqueNames[i]  = nullptr;

            if (isRunning())
                startCheckThread();

            return true;
        }
    }

    qCritical("remove_plugin(%i) - could not find plugin", id);
    set_last_error("Could not find plugin to remove");
    return false;
}

void CarlaEngine::callback(CallbackType action, unsigned short pluginId, int value1, int value2, double value3)
{
    if (m_callback)
        m_callback(action, pluginId, value1, value2, value3);
}

void CarlaEngine::setCallback(CallbackFunc func)
{
    m_callback = func;
}

double CarlaEngine::getInputPeak(unsigned short pluginId, unsigned short id)
{
    assert(pluginId < MAX_PLUGINS);
    assert(id < MAX_PEAKS);
    return m_insPeak[pluginId*MAX_PEAKS + id];
}

double CarlaEngine::getOutputPeak(unsigned short pluginId, unsigned short id)
{
    assert(pluginId < MAX_PLUGINS);
    assert(id < MAX_PEAKS);
    return m_outsPeak[pluginId*MAX_PEAKS + id];
}

void CarlaEngine::setInputPeak(unsigned short pluginId, unsigned short id, double value)
{
    assert(pluginId < MAX_PLUGINS);
    assert(id < MAX_PEAKS);
    m_insPeak[pluginId*MAX_PEAKS + id] = value;
}

void CarlaEngine::setOutputPeak(unsigned short pluginId, unsigned short id, double value)
{
    assert(pluginId < MAX_PLUGINS);
    assert(id < MAX_PEAKS);
    m_outsPeak[pluginId*MAX_PEAKS + id] = value;
}

// -------------------------------------------------------------------------------------------------------------------

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
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(const CarlaEngineClientNativeHandle& handle_, bool active) :
    m_active(active),
    handle(handle_)
{
}

CarlaEngineClient::~CarlaEngineClient()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle.client)
            jack_client_close(handle.client);
    }
}

void CarlaEngineClient::activate()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle.client && ! m_active)
            jack_activate(handle.client);
    }

    m_active = true;
}

void CarlaEngineClient::deactivate()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (handle.client && m_active)
            jack_deactivate(handle.client);
    }

    m_active = false;
}

bool CarlaEngineClient::isActive() const
{
    return m_active;
}

bool CarlaEngineClient::isOk() const
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return true;
#endif
    return bool(handle.client);
}

const CarlaEngineBasePort* CarlaEngineClient::addPort(CarlaEnginePortType type, const char* name, bool isInput)
{
    CarlaEnginePortNativeHandle portHandle = {
        handle.client,
        nullptr,
        nullptr
    };

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#endif
    {
        switch (type)
        {
        case CarlaEnginePortTypeAudio:
            portHandle.port = jack_port_register(handle.client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
            break;
        case CarlaEnginePortTypeControl:
            portHandle.port = jack_port_register(handle.client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
            break;
        case CarlaEnginePortTypeMIDI:
            portHandle.port = jack_port_register(handle.client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
            break;
        }
    }

    switch (type)
    {
    case CarlaEnginePortTypeAudio:
        return new CarlaEngineAudioPort(portHandle, isInput);
    case CarlaEnginePortTypeControl:
        return new CarlaEngineControlPort(portHandle, isInput);
    case CarlaEnginePortTypeMIDI:
        return new CarlaEngineMidiPort(portHandle, isInput);
    default:
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(CarlaEnginePortNativeHandle& handle_, bool isInput_) :
    isInput(isInput_),
    handle(handle_)
{
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#endif
    {
        if (handle.client && handle.port)
            jack_port_unregister(handle.client, handle.port);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const /*engine*/)
{
}

float* CarlaEngineAudioPort::getJackAudioBuffer(uint32_t nframes)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
#endif
    return (float*)jack_port_get_buffer(handle.port, nframes);
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineControlPort::initBuffer(CarlaEngine* const engine)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        handle.buffer = isInput ? engine->rackControlEventsIn : engine->rackControlEventsOut;
        return;
    }
#endif

    if (handle.port)
    {
        handle.buffer = jack_port_get_buffer(handle.port, engine->getBufferSize());

        if (! isInput)
            jack_midi_clear_buffer(handle.buffer);
    }
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    if (! isInput)
        return 0;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++)
        {
            if (events[i].type != CarlaEngineEventNull)
                count++;
            else
                break;
        }

        return count;
    }
#endif

    return jack_midi_get_event_count(handle.buffer);
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

        if (index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS)
            return &events[index];
        return nullptr;
    }
#endif

    static jack_midi_event_t jackEvent;
    static CarlaEngineControlEvent carlaEvent;

    if (jack_midi_event_get(&jackEvent, handle.buffer, index) != 0)
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

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)handle.buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++)
        {
            if (events[i].type == CarlaEngineEventNull)
            {
                events[i].type  = type;
                events[i].time  = time;
                events[i].value = value;
                events[i].channel    = channel;
                events[i].controller = controller;
                break;
            }
        }

        return;
    }
#endif

    if (type == CarlaEngineEventControlChange && MIDI_IS_CONTROL_BANK_SELECT(controller))
        type = CarlaEngineEventMidiBankChange;

    uint8_t data[4] = { 0 };

    switch (type)
    {
    case CarlaEngineEventNull:
        break;
    case CarlaEngineEventControlChange:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = controller;
        data[2] = value * 127;
        jack_midi_event_write(handle.buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiBankChange:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = value;
        jack_midi_event_write(handle.buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiProgramChange:
        data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
        data[1] = value;
        jack_midi_event_write(handle.buffer, time, data, 2);
        break;
    case CarlaEngineEventAllSoundOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
        jack_midi_event_write(handle.buffer, time, data, 2);
        break;
    case CarlaEngineEventAllNotesOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
        jack_midi_event_write(handle.buffer, time, data, 2);
        break;
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineMidiPort::initBuffer(CarlaEngine* const engine)
{
#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        handle.buffer = isInput ? engine->rackMidiEventsIn : engine->rackMidiEventsOut;
        return;
    }
#endif

    if (handle.port)
    {
        handle.buffer = jack_port_get_buffer(handle.port, engine->getBufferSize());

        if (! isInput)
            jack_midi_clear_buffer(handle.buffer);
    }
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    if (! isInput)
        return 0;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++)
        {
            if (events[i].size > 0)
                count++;
            else
                break;
        }

        return count;
    }
#endif

    return jack_midi_get_event_count(handle.buffer);
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

        if (index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS)
            return &events[index];

        return nullptr;
    }
#endif

    static jack_midi_event_t jackEvent;
    static CarlaEngineMidiEvent carlaEvent;

    if (jack_midi_event_get(&jackEvent, handle.buffer, index) == 0 && jackEvent.size < 4)
    {
        carlaEvent.time = jackEvent.time;
        carlaEvent.size = jackEvent.size;
        memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);
        return &carlaEvent;
    }

    return nullptr;
}

void CarlaEngineMidiPort::writeEvent(uint32_t time, uint8_t* data, uint8_t size)
{
    if (isInput)
        return;

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (size >= 4)
            return;

        CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)handle.buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++)
        {
            if (events[i].size == 0)
            {
                events[i].time = time;
                events[i].size = size;
                memcpy(events[i].data, data, size);
                break;
            }
        }

        return;
    }
#endif

    jack_midi_event_write(handle.buffer, time, data, size);
}

CARLA_BACKEND_END_NAMESPACE

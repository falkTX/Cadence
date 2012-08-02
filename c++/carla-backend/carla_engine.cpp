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

// -------------------------------------------------------------------

CarlaEngine::CarlaEngine() :
    m_osc(this),
    m_checkThread(this),
    m_callback(nullptr),
    m_carlaPlugins{nullptr},
    m_uniqueNames{nullptr},
    m_insPeak{0.0},
    m_outsPeak{0.0}
{
    qDebug("CarlaEngine::CarlaEngine()");

    name = nullptr;
    sampleRate = 0.0;
    bufferSize = 0;
}

CarlaEngine::~CarlaEngine()
{
    qDebug("CarlaEngine::~CarlaEngine()");

    if (name)
        free((void*)name);
}

// -------------------------------------------------------------------
// static values

int CarlaEngine::maxClientNameSize()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
        return jack_client_name_size();
#endif
    return STR_MAX/2;
}

int CarlaEngine::maxPortNameSize()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
        return jack_port_name_size();
#endif
    return STR_MAX;
}

// -------------------------------------------------------------------
// plugin management

short CarlaEngine::getNewPluginId() const
{
#ifdef BUILD_BRIDGE
    const unsigned short max = MAX_PLUGINS;
#else
    const unsigned short max = (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK) ? 16 : MAX_PLUGINS;
#endif

    for (unsigned short i=0; i < max; i++)
    {
        if (! m_carlaPlugins[i])
            return i;
    }

    return -1;
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned short id) const
{
    assert(id < MAX_PLUGINS);

    if (id < MAX_PLUGINS)
        return m_carlaPlugins[id];
    return nullptr;
}

const char* CarlaEngine::getUniqueName(const char* const name)
{
    QString qname(name);

    if (qname.isEmpty())
        qname = "(No name)";

    qname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    qname.replace(":", "."); // ":" is used in JACK1 to split client/port names

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

short CarlaEngine::addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra)
{
    return addPlugin(BINARY_NATIVE, ptype, filename, name, label, extra);
}

short CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra)
{
    CarlaPlugin::initializer init = {
        this,
        filename,
        name,
        label
    };

    CarlaPlugin* plugin = nullptr;

#ifndef BUILD_BRIDGE
    if (btype != BINARY_NATIVE)
    {
#  ifdef CARLA_ENGINE_JACK
        if (carlaOptions.process_mode != CarlaBackend::PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            setLastError("Can only use bridged plugins in JACK Multi-Client mode");
            return -1;
        }
#  else
        setLastError("Can only use bridged plugins with JACK backend");
        return -1;
#  endif

        plugin = CarlaPlugin::newBridge(init, btype, ptype);
    }
    else
#endif
    {
        switch (ptype)
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_LADSPA:
            plugin = CarlaPlugin::newLADSPA(init, extra);
            break;
        case PLUGIN_DSSI:
            plugin = CarlaPlugin::newDSSI(init, extra);
            break;
        case PLUGIN_LV2:
            plugin = CarlaPlugin::newLV2(init);
            break;
        case PLUGIN_VST:
            plugin = CarlaPlugin::newVST(init);
            break;
        case PLUGIN_GIG:
            plugin = CarlaPlugin::newGIG(init);
            break;
        case PLUGIN_SF2:
            plugin = CarlaPlugin::newSF2(init);
            break;
        case PLUGIN_SFZ:
            plugin = CarlaPlugin::newSFZ(init);
            break;
        }
    }

    if (plugin == nullptr)
        return -1;

    const short id = plugin->id();

    m_carlaPlugins[id] = plugin;
    m_uniqueNames[id]  = plugin->name();

    return id;
}

bool CarlaEngine::removePlugin(const unsigned short id)
{
    CarlaPlugin* const plugin = m_carlaPlugins[id];

    if (plugin && plugin->id() == id)
    {
        processLock();
        plugin->setEnabled(false);
        processUnlock();

        if (m_checkThread.isRunning())
            m_checkThread.stopNow();

        delete plugin;

        m_carlaPlugins[id] = nullptr;
        m_uniqueNames[id]  = nullptr;

        if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            for (unsigned short i=id; i < MAX_PLUGINS; i++)
            {
                m_carlaPlugins[i] = m_carlaPlugins[i+1];
                m_uniqueNames[i]  = m_uniqueNames[i+1];

                if (m_carlaPlugins[i])
                    m_carlaPlugins[i]->setId(i+1);
            }
        }

        if (isRunning())
            m_checkThread.start(QThread::HighPriority);

        return true;
    }

    qCritical("remove_plugin(%i) - could not find plugin", id);
    setLastError("Could not find plugin to remove");
    return false;
}

void CarlaEngine::removeAllPlugins()
{
    if (m_checkThread.isRunning())
        m_checkThread.stopNow();

    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = m_carlaPlugins[i];

        if (plugin)
        {
            processLock();
            plugin->setEnabled(false);
            processUnlock();

            delete plugin;

            m_carlaPlugins[i] = nullptr;
            m_uniqueNames[i]  = nullptr;
        }
    }

    if (isRunning())
        m_checkThread.start(QThread::HighPriority);
}

void CarlaEngine::idlePluginGuis()
{
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        CarlaPlugin* const plugin = m_carlaPlugins[i];

        if (plugin && plugin->enabled())
            plugin->idleGui();
    }
}

// -------------------------------------------------------------------
// protected calls

void CarlaEngine::bufferSizeChanged(uint32_t newBufferSize)
{
    bufferSize = newBufferSize;

    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        if (m_carlaPlugins[i] && m_carlaPlugins[i]->enabled())
            m_carlaPlugins[i]->bufferSizeChanged(newBufferSize);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(const CarlaEngineClientNativeHandle& handle_) :
    handle(handle_)
{
    m_active = false;
}

CarlaEngineClient::~CarlaEngineClient()
{
    assert(! m_active);

#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#  endif
    {
        if (handle.client)
            jack_client_close(handle.client);
    }
#endif
}

void CarlaEngineClient::activate()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#  endif
    {
        if (handle.client && ! m_active)
            jack_activate(handle.client);
    }
#endif

    m_active = true;
}

void CarlaEngineClient::deactivate()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_MULTIPLE_CLIENTS)
#  endif
    {
        if (handle.client && m_active)
            jack_deactivate(handle.client);
    }
#endif

    m_active = false;
}

bool CarlaEngineClient::isActive() const
{
    return m_active;
}

bool CarlaEngineClient::isOk() const
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
        return bool(handle.client);
#endif
    return true;
}

const CarlaEngineBasePort* CarlaEngineClient::addPort(CarlaEnginePortType type, const char* name, bool isInput)
{
    CarlaEnginePortNativeHandle portHandle = {
    #ifdef CARLA_ENGINE_JACK
        handle.client,
        nullptr
    #endif
    };

#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
    {
        switch (type)
        {
        case CarlaEnginePortTypeAudio:
            portHandle.port = jack_port_register(handle.client, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
            break;
        case CarlaEnginePortTypeControl:
        case CarlaEnginePortTypeMIDI:
            portHandle.port = jack_port_register(handle.client, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
            break;
        }
    }
#else
    Q_UNUSED(name);
#endif

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

CarlaEngineBasePort::CarlaEngineBasePort(const CarlaEnginePortNativeHandle& handle_, bool isInput_) :
    isInput(isInput_),
    handle(handle_)
{
    m_buffer = nullptr;
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
    {
        if (handle.client && handle.port)
            jack_port_unregister(handle.client, handle.port);
    }
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(const CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const /*engine*/)
{
}

#ifdef CARLA_ENGINE_JACK
float* CarlaEngineAudioPort::getJackAudioBuffer(uint32_t nframes)
{
#  ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
#  endif
    assert(handle.port);
    return (float*)jack_port_get_buffer(handle.port, nframes);
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(const CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineControlPort::initBuffer(CarlaEngine* const engine)
{
    assert(engine);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        m_buffer = isInput ? engine->rackControlEventsIn : engine->rackControlEventsOut;
        return;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.port)
    {
        m_buffer = jack_port_get_buffer(handle.port, engine->getBufferSize());

        if (! isInput)
            jack_midi_clear_buffer(m_buffer);
    }
#endif
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    if (! isInput)
        return 0;

    assert(m_buffer);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)m_buffer;

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

#ifdef CARLA_ENGINE_JACK
    return jack_midi_get_event_count(m_buffer);
#else
    return 0;
#endif
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    assert(m_buffer);
    assert(index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)m_buffer;

        if (index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS)
            return &events[index];
        return nullptr;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    static jack_midi_event_t jackEvent;
    static CarlaEngineControlEvent carlaEvent;

    if (jack_midi_event_get(&jackEvent, m_buffer, index) != 0)
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
#endif

    return nullptr;
}

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

    assert(m_buffer);
    assert(type != CarlaEngineEventNull);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)m_buffer;

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

#ifdef CARLA_ENGINE_JACK
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
        jack_midi_event_write(m_buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiBankChange:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_BANK_SELECT;
        data[2] = value;
        jack_midi_event_write(m_buffer, time, data, 3);
        break;
    case CarlaEngineEventMidiProgramChange:
        data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
        data[1] = value;
        jack_midi_event_write(m_buffer, time, data, 2);
        break;
    case CarlaEngineEventAllSoundOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
        jack_midi_event_write(m_buffer, time, data, 2);
        break;
    case CarlaEngineEventAllNotesOff:
        data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
        data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
        jack_midi_event_write(m_buffer, time, data, 2);
        break;
    }
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(const CarlaEnginePortNativeHandle& handle, bool isInput) :
    CarlaEngineBasePort(handle, isInput)
{
}

void CarlaEngineMidiPort::initBuffer(CarlaEngine* const engine)
{
    assert(engine);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        m_buffer = isInput ? engine->rackMidiEventsIn : engine->rackMidiEventsOut;
        return;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.port)
    {
        m_buffer = jack_port_get_buffer(handle.port, engine->getBufferSize());

        if (! isInput)
            jack_midi_clear_buffer(m_buffer);
    }
#endif
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    if (! isInput)
        return 0;

    assert(m_buffer);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)m_buffer;

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

#ifdef CARLA_ENGINE_JACK
    return jack_midi_get_event_count(m_buffer);
#else
    return 0;
#endif
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    assert(m_buffer);
    assert(index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)m_buffer;

        if (index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS)
            return &events[index];

        return nullptr;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    static jack_midi_event_t jackEvent;
    static CarlaEngineMidiEvent carlaEvent;

    if (jack_midi_event_get(&jackEvent, m_buffer, index) == 0 && jackEvent.size <= 4)
    {
        carlaEvent.time = jackEvent.time;
        carlaEvent.size = jackEvent.size;
        memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);
        return &carlaEvent;
    }
#endif

    return nullptr;
}

void CarlaEngineMidiPort::writeEvent(uint32_t time, uint8_t* data, uint8_t size)
{
    if (isInput)
        return;

    assert(m_buffer);
    assert(data);
    assert(size > 0);

#ifndef BUILD_BRIDGE
    if (carlaOptions.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (size > 4)
            return;

        CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)m_buffer;

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

#ifdef CARLA_ENGINE_JACK
    jack_midi_event_write(m_buffer, time, data, size);
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine OSC stuff

void CarlaEngine::osc_send_add_plugin(const int32_t pluginId, const char* const pluginName)
{
    qDebug("CarlaEngine::osc_send_add_plugin(%i, \"%s\")", pluginId, pluginName);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+12];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/add_plugin");
        lo_send(oscData->target, target_path, "is", pluginId, pluginName);
    }
}

void CarlaEngine::osc_send_remove_plugin(const int32_t pluginId)
{
    qDebug("CarlaEngine::osc_send_remove_plugin(%i)", pluginId);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+15];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/remove_plugin");
        lo_send(oscData->target, target_path, "i", pluginId);
    }
}

void CarlaEngine::osc_send_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints,
                                           const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    qDebug("CarlaEngine::osc_send_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", %li)", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+17];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_plugin_data");
        lo_send(oscData->target, target_path, "iiiissssh", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts,
                                            const int32_t cIns, const int32_t cOuts, const int32_t cTotals)
{
    qDebug("CarlaEngine::osc_send_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+18];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_plugin_ports");
        lo_send(oscData->target, target_path, "iiiiiiii", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    }
}

void CarlaEngine::osc_send_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints,
                                              const char* const name, const char* const label, const double current)
{
    qDebug("CarlaEngine::osc_send_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %g)", pluginId, index, type, hints, name, label, current);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+20];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_parameter_data");
        lo_send(oscData->target, target_path, "iiiissd", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::osc_send_set_parameter_ranges(const int32_t pluginId, const int32_t index, const double min, const double max, const double def,
                                                const double step, const double stepSmall, const double stepLarge)
{
    qDebug("CarlaEngine::osc_send_set_parameter_ranges(%i, %i, %g, %g, %g, %g, %g, %g)", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+22];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_parameter_ranges");
        lo_send(oscData->target, target_path, "iidddddd", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    qDebug("CarlaEngine::osc_send_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+23];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_parameter_midi_cc");
        lo_send(oscData->target, target_path, "iii", pluginId, index, cc);
    }
}

void CarlaEngine::osc_send_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel)
{
    qDebug("CarlaEngine::osc_send_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+28];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_parameter_midi_channel");
        lo_send(oscData->target, target_path, "iii", pluginId, index, channel);
    }
}

void CarlaEngine::osc_send_set_parameter_value(const int32_t pluginId, const int32_t index, const double value)
{
#if DEBUG
    if (index < 0)
        qDebug("CarlaEngine::osc_send_set_parameter_value(%i, %s, %g)", pluginId, InternalParametersIndex2str((InternalParametersIndex)index), value);
    else
        qDebug("CarlaEngine::osc_send_set_parameter_value(%i, %i, %g)", pluginId, index, value);
#endif
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+21];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_parameter_value");
        lo_send(oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_set_default_value(const int32_t pluginId, const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_set_default_value(%i, %i, %g)", pluginId, index, value);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+19];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_default_value");
        lo_send(oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_set_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_set_program(%i, %i)", pluginId, index);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+13];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_program");
        lo_send(oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_set_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_set_program_count(%i, %i)", pluginId, count);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+19];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_program_count");
        lo_send(oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_set_program_name(const int32_t pluginId, const int32_t index, const char* const name)
{
    qDebug("CarlaEngine::osc_send_set_program_name(%i, %i, %s)", pluginId, index, name);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+18];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_program_name");
        lo_send(oscData->target, target_path, "iis", pluginId, index, name);
    }
}

void CarlaEngine::osc_send_set_midi_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_set_midi_program(%i, %i)", pluginId, index);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+18];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_midi_program");
        lo_send(oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_set_midi_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_set_midi_program_count(%i, %i)", pluginId, count);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+24];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_midi_program_count");
        lo_send(oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name)
{
    qDebug("CarlaEngine::osc_send_set_midi_program_data(%i, %i, %i, %i, %s)", pluginId, index, bank, program, name);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+23];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_midi_program_data");
        lo_send(oscData->target, target_path, "iiiis", pluginId, index, bank, program, name);
    }
}

void CarlaEngine::osc_send_set_input_peak_value(const int32_t plugin_id, const int32_t port_id, const double value)
{
    qDebug("CarlaEngine::osc_send_set_input_peak_value(%i, %i, %f)", plugin_id, port_id, value);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    //Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+22];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_input_peak_value");
        lo_send(oscData->target, target_path, "iid", plugin_id, port_id, value);
    }
}

void CarlaEngine::osc_send_set_output_peak_value(const int32_t plugin_id, const int32_t port_id, const double value)
{
    qDebug("CarlaEngine::osc_send_set_output_peak_value(%i, %i, %f)", plugin_id, port_id, value);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    //Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+23];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/set_output_peak_value");
        lo_send(oscData->target, target_path, "iid", plugin_id, port_id, value);
    }
}

void CarlaEngine::osc_send_note_on(const int32_t plugin_id, const int32_t channel, const int32_t note, const int32_t velo)
{
    qDebug("CarlaEngine::osc_send_note_on(%i, %i, %i, %i)", plugin_id, channel, note, velo);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    //Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+9];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/note_on");
        lo_send(oscData->target, target_path, "iiii", plugin_id, channel, note, velo);
    }
}

void CarlaEngine::osc_send_note_off(const int32_t plugin_id, const int32_t channel, const int32_t note)
{
    qDebug("CarlaEngine::osc_send_note_off(%i, %i, %i)", plugin_id, channel, note);
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);
    //Q_ASSERT(pluginId >= 0 && pluginId < MAX_PLUGINS);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+10];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/note_off");
        lo_send(oscData->target, target_path, "iii", plugin_id, channel, note);
    }
}

void CarlaEngine::osc_send_exit()
{
    qDebug("CarlaEngine::osc_send_exit()");
    const CarlaOscData* const oscData = m_osc.getControllerData();
    Q_ASSERT(oscData);

    if (oscData->target)
    {
        char target_path[strlen(oscData->path)+6];
        strcpy(target_path, oscData->path);
        strcat(target_path, "/exit");
        lo_send(oscData->target, target_path, "");
    }
}

CARLA_BACKEND_END_NAMESPACE

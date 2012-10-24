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

#include "carla_engine.h"
#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

unsigned short CarlaEngine::m_maxPluginNumber = 0;

CarlaEngine::CarlaEngine()
    : m_checkThread(this),
#ifndef BUILD_BRIDGE
      m_osc(this),
#endif
      m_oscData(nullptr),
      m_callback(nullptr),
#ifdef Q_COMPILER_INITIALIZER_LISTS
      m_callbackPtr(nullptr),
      m_carlaPlugins{nullptr},
      m_uniqueNames{nullptr},
      m_insPeak{0.0},
      m_outsPeak{0.0}
#else
      m_callbackPtr(nullptr)
#endif
{
    qDebug("CarlaEngine::CarlaEngine()");

    type = CarlaEngineTypeNull;
    name = nullptr;
    bufferSize = 0;
    sampleRate = 0.0;
    m_maxPluginNumber = 0;

#ifndef Q_COMPILER_INITIALIZER_LISTS
    for (unsigned short i=0; i < MAX_PLUGINS; i++)
    {
        m_carlaPlugins[i] = nullptr;
        m_uniqueNames[i]  = nullptr;
    }

    for (unsigned short i=0; i < MAX_PLUGINS * MAX_PEAKS; i++)
    {
        m_insPeak[i]  = 0.0;
        m_outsPeak[i] = 0.0;
    }
#endif
}

CarlaEngine::~CarlaEngine()
{
    qDebug("CarlaEngine::~CarlaEngine()");

    if (name)
        free((void*)name);
}

// -----------------------------------------------------------------------
// Static values

int CarlaEngine::maxClientNameSize()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.processMode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
        return jackbridge_client_name_size();
#endif
    return STR_MAX/2;
}

int CarlaEngine::maxPortNameSize()
{
#ifdef CARLA_ENGINE_JACK
#  ifndef BUILD_BRIDGE
    if (carlaOptions.processMode != PROCESS_MODE_CONTINUOUS_RACK)
#  endif
        return jackbridge_port_name_size();
#endif
    return STR_MAX;
}

unsigned short CarlaEngine::maxPluginNumber()
{
    return m_maxPluginNumber;
}

bool CarlaEngine::init(const char* const clientName)
{
    qDebug("CarlaEngine::init(\"%s\")", clientName);

#ifndef BUILD_BRIDGE
    m_osc.init(clientName);
    m_oscData = m_osc.getControlData();

    carla_setprocname(clientName);
#endif

    return true;
}

bool CarlaEngine::close()
{
    qDebug("CarlaEngine::close()");

    m_checkThread.stopNow();

#ifndef BUILD_BRIDGE
    osc_send_control_exit();
    m_osc.close();
#endif
    m_oscData = nullptr;

    m_maxPluginNumber = 0;

    return true;
}

// -----------------------------------------------------------------------
// plugin management

short CarlaEngine::getNewPluginId() const
{
    qDebug("CarlaEngine::getNewPluginId()");

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
    {
        if (! m_carlaPlugins[i])
            return i;
    }

    return -1;
}

CarlaPlugin* CarlaEngine::getPlugin(const unsigned short id) const
{
    qDebug("CarlaEngine::getPlugin(%i/%i)", id, m_maxPluginNumber);
    CARLA_ASSERT(m_maxPluginNumber != 0);
    CARLA_ASSERT(id < m_maxPluginNumber);

    if (id < m_maxPluginNumber)
        return m_carlaPlugins[id];

    return nullptr;
}

CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned short id) const
{
    CARLA_ASSERT(m_maxPluginNumber != 0);
    CARLA_ASSERT(id < m_maxPluginNumber);

    return m_carlaPlugins[id];
}

const char* CarlaEngine::getUniqueName(const char* const name)
{
    qDebug("CarlaEngine::getUniqueName(\"%s\")", name);

    QString qname(name);

    if (qname.isEmpty())
        qname = "(No name)";

    qname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    qname.replace(":", "."); // ":" is used in JACK1 to split client/port names

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
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
    qDebug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2str(btype), PluginType2str(ptype), filename, name, label, extra);
    CARLA_ASSERT(btype != BINARY_NONE);
    CARLA_ASSERT(ptype != PLUGIN_NONE);
    CARLA_ASSERT(filename);
    CARLA_ASSERT(label);

    if (m_maxPluginNumber == 0)
    {
#ifdef BUILD_BRIDGE
        m_maxPluginNumber = MAX_PLUGINS;
#else
        m_maxPluginNumber = (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK) ? 16 : MAX_PLUGINS;
#endif
    }

    CarlaPlugin::initializer init = {
        this,
        filename,
        name,
        label
    };

    CarlaPlugin* plugin = nullptr;

#ifndef BUILD_BRIDGE
    if (btype != BINARY_NATIVE || (carlaOptions.preferPluginBridges && getBinaryBidgePath(btype) && type == CarlaEngineTypeJack))
    {
#  ifdef CARLA_ENGINE_JACK
        if (carlaOptions.processMode != CarlaBackend::PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            setLastError("Can only use bridged plugins in JACK Multi-Client mode");
            return -1;
        }
#  endif

        if (type != CarlaEngineTypeJack)
        {
            setLastError("Can only use bridged plugins with JACK backend");
            return -1;
        }

        plugin = CarlaPlugin::newBridge(init, btype, ptype);
    }
    else
#endif
    {
        switch (ptype)
        {
        case PLUGIN_NONE:
            break;
        case PLUGIN_INTERNAL:
#ifndef BUILD_BRIDGE
            plugin = CarlaPlugin::newNative(init);
#endif
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
#ifdef BUILD_BRIDGE
        case PLUGIN_GIG:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
            break;
#else
        case PLUGIN_GIG:
            plugin = CarlaPlugin::newGIG(init);
            break;
        case PLUGIN_SF2:
            plugin = CarlaPlugin::newSF2(init);
            break;
        case PLUGIN_SFZ:
            plugin = CarlaPlugin::newSFZ(init);
            break;
#endif
        }
    }

    if (! plugin)
        return -1;

    const short id = plugin->id();

    m_carlaPlugins[id] = plugin;
    m_uniqueNames[id]  = plugin->name();

    if (! m_checkThread.isRunning())
        m_checkThread.startNow();

    return id;
}

bool CarlaEngine::removePlugin(const unsigned short id)
{
    qDebug("CarlaEngine::removePlugin(%i)", id);
    CARLA_ASSERT(m_maxPluginNumber != 0);
    CARLA_ASSERT(id < m_maxPluginNumber);

    CarlaPlugin* const plugin = m_carlaPlugins[id];

    if (plugin /*&& plugin->id() == id*/)
    {
        CARLA_ASSERT(plugin->id() == id);

        m_checkThread.stopNow();

        processLock();
        plugin->setEnabled(false);
        processUnlock();

        delete plugin;
        m_carlaPlugins[id] = nullptr;
        m_uniqueNames[id]  = nullptr;

#ifndef BUILD_BRIDGE
        osc_send_control_remove_plugin(id);

        if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        {
            // TODO - handle OSC server comm

            for (unsigned short i=id; i < m_maxPluginNumber-1; i++)
            {
                m_carlaPlugins[i] = m_carlaPlugins[i+1];
                m_uniqueNames[i]  = m_uniqueNames[i+1];

                if (m_carlaPlugins[i])
                    m_carlaPlugins[i]->setId(i);
            }
        }
#endif

        if (isRunning())
            m_checkThread.startNow();

        return true;
    }

    qCritical("CarlaEngine::removePlugin(%i) - could not find plugin", id);
    setLastError("Could not find plugin to remove");
    return false;
}

void CarlaEngine::removeAllPlugins()
{
    qDebug("CarlaEngine::removeAllPlugins()");

    m_checkThread.stopNow();

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
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

    m_maxPluginNumber = 0;
}

void CarlaEngine::idlePluginGuis()
{
    CARLA_ASSERT(m_maxPluginNumber != 0);

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
    {
        CarlaPlugin* const plugin = m_carlaPlugins[i];

        if (plugin && plugin->enabled())
            plugin->idleGui();
    }
}

#ifndef BUILD_BRIDGE
void CarlaEngine::processRack(float* inBuf[2], float* outBuf[2], uint32_t frames)
{
    // initialize outputs (zero)
    zeroF(outBuf[0], frames);
    zeroF(outBuf[1], frames);
    memset(rackControlEventsOut, 0, sizeof(CarlaEngineControlEvent)*MAX_ENGINE_CONTROL_EVENTS);
    memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

    bool processed = false;

    // process plugins
    for (unsigned short i=0, max=maxPluginNumber(); i < max; i++)
    {
        CarlaPlugin* const plugin = getPluginUnchecked(i);

        if (plugin && plugin->enabled())
        {
            if (processed)
            {
                // initialize inputs (from previous outputs)
                memcpy(inBuf[0], outBuf[0], sizeof(float)*frames);
                memcpy(inBuf[1], outBuf[1], sizeof(float)*frames);
                memcpy(rackMidiEventsIn, rackMidiEventsOut, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);

                // initialize outputs (zero)
                zeroF(outBuf[0], frames);
                zeroF(outBuf[1], frames);
                memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            }

            // process
            plugin->engineProcessLock();

            plugin->initBuffers();

            if (carlaOptions.processHighPrecision)
            {
                float* inBuf2[2];
                float* outBuf2[2];

                for (uint32_t j=0; j < frames; j += 8)
                {
                    inBuf2[0] = inBuf[0] + j;
                    inBuf2[1] = inBuf[1] + j;

                    outBuf2[0] = outBuf[0] + j;
                    outBuf2[1] = outBuf[1] + j;

                    plugin->process(inBuf2, outBuf2, 8, j);
                }
            }
            else
                plugin->process(inBuf, outBuf, frames);

            plugin->engineProcessUnlock();

            // if plugin has no audio inputs, add previous buffers
            if (plugin->audioInCount() == 0)
            {
                for (uint32_t j=0; j < frames; j++)
                {
                    outBuf[0][j] += inBuf[0][j];
                    outBuf[1][j] += inBuf[1][j];
                }
            }

            // if plugin has no midi output, add previous midi input
            if (plugin->midiOutCount() == 0)
            {
                memcpy(rackMidiEventsOut, rackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            }

            processed = true;
        }
    }

    // if no plugins in the rack, copy inputs over outputs
    if (! processed)
    {
        memcpy(outBuf[0], inBuf[0], sizeof(float)*frames);
        memcpy(outBuf[1], inBuf[1], sizeof(float)*frames);
        memcpy(rackMidiEventsOut, rackMidiEventsIn, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
    }
}
#endif

// -----------------------------------------------------------------------
// Information (base)

CarlaEngineType CarlaEngine::getType() const
{
    return type;
}

const char* CarlaEngine::getName() const
{
    CARLA_ASSERT(name);

    return name;
}

double CarlaEngine::getSampleRate() const
{
    //CARLA_ASSERT(sampleRate != 0.0);

    return sampleRate;
}

uint32_t CarlaEngine::getBufferSize() const
{
    //CARLA_ASSERT(bufferSize != 0);

    return bufferSize;
}

const CarlaTimeInfo* CarlaEngine::getTimeInfo() const
{
    return &timeInfo;
}

// -----------------------------------------------------------------------
// Information (audio peaks)

double CarlaEngine::getInputPeak(const unsigned short pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < m_maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    return m_insPeak[pluginId*MAX_PEAKS + id];
}

double CarlaEngine::getOutputPeak(const unsigned short pluginId, const unsigned short id) const
{
    CARLA_ASSERT(pluginId < m_maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    return m_outsPeak[pluginId*MAX_PEAKS + id];
}

void CarlaEngine::setInputPeak(const unsigned short pluginId, const unsigned short id, double value)
{
    CARLA_ASSERT(pluginId < m_maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    m_insPeak[pluginId*MAX_PEAKS + id] = value;
}

void CarlaEngine::setOutputPeak(const unsigned short pluginId, const unsigned short id, double value)
{
    CARLA_ASSERT(pluginId < m_maxPluginNumber);
    CARLA_ASSERT(id < MAX_PEAKS);

    m_outsPeak[pluginId*MAX_PEAKS + id] = value;
}

// -----------------------------------------------------------------------
// Callback

void CarlaEngine::callback(const CallbackType action, const unsigned short pluginId, const int value1, const int value2, const double value3)
{
    qDebug("CarlaEngine::callback(%s, %i, %i, %i, %f)", CallbackType2str(action), pluginId, value1, value2, value3);

    if (m_callback)
        m_callback(m_callbackPtr, action, pluginId, value1, value2, value3);
}

void CarlaEngine::setCallback(const CallbackFunc func, void* const ptr)
{
    qDebug("CarlaEngine::setCallback(%p, %p)", func, ptr);
    CARLA_ASSERT(func);

    m_callback = func;
    m_callbackPtr = ptr;
}

// -----------------------------------------------------------------------
// Mutex locks

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

// -----------------------------------------------------------------------
// OSC Stuff

bool CarlaEngine::isOscControlRegisted() const
{
#ifndef BUILD_BRIDGE
    return m_osc.isControlRegistered();
#else
    return bool(m_oscData);
#endif
}

#ifndef BUILD_BRIDGE
const char* CarlaEngine::getOscServerPathTCP() const
{
    return m_osc.getServerPathTCP();
}

const char* CarlaEngine::getOscServerPathUDP() const
{
    return m_osc.getServerPathUDP();
}
#else
void CarlaEngine::setOscBridgeData(const CarlaOscData* const oscData)
{
    m_oscData = oscData;
}
#endif

// -----------------------------------------------------------------------
// protected calls

void CarlaEngine::bufferSizeChanged(const uint32_t newBufferSize)
{
    qDebug("CarlaEngine::bufferSizeChanged(%i)", newBufferSize);

    bufferSize = newBufferSize;

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
    {
        if (m_carlaPlugins[i] && m_carlaPlugins[i]->enabled())
            m_carlaPlugins[i]->bufferSizeChanged(newBufferSize);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(const CarlaEngineClientNativeHandle& handle_)
    : handle(handle_)
{
    qDebug("CarlaEngineClient::CarlaEngineClient()");
    CARLA_ASSERT(handle.type != CarlaEngineTypeNull);

    m_active = false;
}

CarlaEngineClient::~CarlaEngineClient()
{
    qDebug("CarlaEngineClient::~CarlaEngineClient()");
    CARLA_ASSERT(! m_active);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
#ifdef CARLA_ENGINE_JACK
        if (handle.jackClient)
            jackbridge_client_close(handle.jackClient);
#endif
#ifdef CARLA_ENGINE_RTAUDIO
        if (handle.rtAudioPtr)
            delete handle.rtAudioPtr;
#endif
    }
}

void CarlaEngineClient::activate()
{
    qDebug("CarlaEngineClient::activate()");
    CARLA_ASSERT(! m_active);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (! m_active)
        {
#ifdef CARLA_ENGINE_JACK
            if (handle.jackClient)
                jackbridge_activate(handle.jackClient);
#endif
#ifdef CARLA_ENGINE_RTAUDIO
            if (handle.rtAudioPtr)
                handle.rtAudioPtr->startStream();
#endif
        }
    }

    m_active = true;
}

void CarlaEngineClient::deactivate()
{
    qDebug("CarlaEngineClient::deactivate()");
    CARLA_ASSERT(m_active);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_MULTIPLE_CLIENTS)
#endif
    {
        if (m_active)
        {
#ifdef CARLA_ENGINE_JACK
            if (handle.jackClient)
                jackbridge_deactivate(handle.jackClient);
#endif
#ifdef CARLA_ENGINE_RTAUDIO
            if (handle.rtAudioPtr)
                handle.rtAudioPtr->stopStream();
#endif
        }
    }

    m_active = false;
}

bool CarlaEngineClient::isActive() const
{
    qDebug("CarlaEngineClient::isActive()");

    return m_active;
}

bool CarlaEngineClient::isOk() const
{
    qDebug("CarlaEngineClient::isOk()");

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode != PROCESS_MODE_CONTINUOUS_RACK)
#endif
    {
#ifdef CARLA_ENGINE_JACK
        if (handle.type == CarlaEngineTypeJack)
            return bool(handle.jackClient);
#endif
#ifdef CARLA_ENGINE_RTAUDIO
        if (handle.type == CarlaEngineTypeRtAudio)
            return bool(handle.rtAudioPtr);
#endif
    }

    return true;
}

const CarlaEngineBasePort* CarlaEngineClient::addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput)
{
    qDebug("CarlaEngineClient::addPort(%i, \"%s\", %s)", portType, name, bool2str(isInput));

    CarlaEnginePortNativeHandle portHandle;
#ifdef CARLA_ENGINE_JACK
    portHandle.jackClient = handle.jackClient;
#endif

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode != PROCESS_MODE_CONTINUOUS_RACK)
#endif
    {
#ifdef CARLA_ENGINE_JACK
        if (handle.type == CarlaEngineTypeJack)
        {
            switch (portType)
            {
            case CarlaEnginePortTypeAudio:
                portHandle.jackPort = jackbridge_port_register(handle.jackClient, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            case CarlaEnginePortTypeControl:
            case CarlaEnginePortTypeMIDI:
                portHandle.jackPort = jackbridge_port_register(handle.jackClient, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
                break;
            }
        }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
        if (handle.type == CarlaEngineTypeRtAudio)
        {
            // TODO
        }
#endif
    }

    switch (portType)
    {
    case CarlaEnginePortTypeAudio:
        return new CarlaEngineAudioPort(portHandle, isInput);
    case CarlaEnginePortTypeControl:
        return new CarlaEngineControlPort(portHandle, isInput);
    case CarlaEnginePortTypeMIDI:
        return new CarlaEngineMidiPort(portHandle, isInput);
    }

    qCritical("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Base class)

CarlaEngineBasePort::CarlaEngineBasePort(const CarlaEnginePortNativeHandle& handle_, const bool isInput_)
    : isInput(isInput_),
      handle(handle_)
{
    qDebug("CarlaEngineBasePort::CarlaEngineBasePort(%s)", bool2str(isInput_));

    buffer = nullptr;
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
    qDebug("CarlaEngineBasePort::~CarlaEngineBasePort()");

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        return;
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.jackClient && handle.jackPort)
        jackbridge_port_unregister(handle.jackClient, handle.jackPort);
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(const CarlaEnginePortNativeHandle& handle, const bool isInput)
    : CarlaEngineBasePort(handle, isInput)
{
    qDebug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s)", bool2str(isInput));
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const /*engine*/)
{
}

#ifdef CARLA_ENGINE_JACK
float* CarlaEngineAudioPort::getJackAudioBuffer(uint32_t nframes)
{
#  ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
        return nullptr;
#  endif
    CARLA_ASSERT(handle.jackPort);
    return (float*)jackbridge_port_get_buffer(handle.jackPort, nframes);
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(const CarlaEnginePortNativeHandle& handle, const bool isInput)
    : CarlaEngineBasePort(handle, isInput)
{
    qDebug("CarlaEngineControlPort::CarlaEngineControlPort(%s)", bool2str(isInput));
}

void CarlaEngineControlPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        buffer = isInput ? engine->rackControlEventsIn : engine->rackControlEventsOut;
        return;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.jackPort)
    {
        buffer = jackbridge_port_get_buffer(handle.jackPort, engine->getBufferSize());

        if (! isInput)
            jackbridge_midi_clear_buffer(buffer);
    }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    if (! isInput)
        return 0;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

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
    if (handle.jackPort)
        return jackbridge_midi_get_event_count(buffer);
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif

    return 0;
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    CARLA_ASSERT(index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS);

    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

        if (index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS)
            return &events[index];
        return nullptr;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.jackPort)
    {
        static jackbridge_midi_event_t jackEvent;
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
    }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif

    return nullptr;
}

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint8_t controller, double value)
{
    if (isInput)
        return;

    CARLA_ASSERT(buffer);
    CARLA_ASSERT(type != CarlaEngineEventNull);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

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
    if (handle.jackPort)
    {
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
            jackbridge_midi_event_write(buffer, time, data, 3);
            break;
        case CarlaEngineEventMidiBankChange:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_BANK_SELECT;
            data[2] = value;
            jackbridge_midi_event_write(buffer, time, data, 3);
            break;
        case CarlaEngineEventMidiProgramChange:
            data[0] = MIDI_STATUS_PROGRAM_CHANGE + channel;
            data[1] = value;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        case CarlaEngineEventAllSoundOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_SOUND_OFF;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        case CarlaEngineEventAllNotesOff:
            data[0] = MIDI_STATUS_CONTROL_CHANGE + channel;
            data[1] = MIDI_CONTROL_ALL_NOTES_OFF;
            jackbridge_midi_event_write(buffer, time, data, 2);
            break;
        }
    }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(const CarlaEnginePortNativeHandle& handle, const bool isInput)
    : CarlaEngineBasePort(handle, isInput)
{
    qDebug("CarlaEngineMidiPort::CarlaEngineMidiPort(%s)", bool2str(isInput));
}

void CarlaEngineMidiPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        buffer = isInput ? engine->rackMidiEventsIn : engine->rackMidiEventsOut;
        return;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.jackPort)
    {
        buffer = jackbridge_port_get_buffer(handle.jackPort, engine->getBufferSize());

        if (! isInput)
            jackbridge_midi_clear_buffer(buffer);
    }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    if (! isInput)
        return 0;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

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
    if (handle.jackPort)
        return jackbridge_midi_get_event_count(buffer);
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif

    return 0;
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    CARLA_ASSERT(index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS);

    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

        if (index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS)
            return &events[index];

        return nullptr;
    }
#endif

#ifdef CARLA_ENGINE_JACK
    if (handle.jackPort)
    {
        static jackbridge_midi_event_t jackEvent;
        static CarlaEngineMidiEvent carlaEvent;

        if (jackbridge_midi_event_get(&jackEvent, buffer, index) == 0 && jackEvent.size <= 4)
        {
            carlaEvent.time = jackEvent.time;
            carlaEvent.size = jackEvent.size;
            memcpy(carlaEvent.data, jackEvent.buffer, jackEvent.size);
            return &carlaEvent;
        }
    }
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif

    return nullptr;
}

void CarlaEngineMidiPort::writeEvent(uint32_t time, const uint8_t* data, uint8_t size)
{
    if (isInput)
        return;

    CARLA_ASSERT(buffer);
    CARLA_ASSERT(data);
    CARLA_ASSERT(size > 0);

#ifndef BUILD_BRIDGE
    if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (size > 4)
            return;

        CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

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
    if (handle.jackPort)
        jackbridge_midi_event_write(buffer, time, data, size);
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    // TODO
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine OSC stuff

#ifndef BUILD_BRIDGE
void CarlaEngine::osc_send_control_add_plugin_start(const int32_t pluginId, const char* const pluginName)
{
    qDebug("CarlaEngine::osc_send_control_add_plugin_start(%i, \"%s\")", pluginId, pluginName);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(pluginName);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+18];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/add_plugin_start");
        lo_send(m_oscData->target, target_path, "is", pluginId, pluginName);
    }
}

void CarlaEngine::osc_send_control_add_plugin_end(const int32_t pluginId)
{
    qDebug("CarlaEngine::osc_send_control_add_plugin_end(%i)", pluginId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+16];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/add_plugin_end");
        lo_send(m_oscData->target, target_path, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_remove_plugin(const int32_t pluginId)
{
    qDebug("CarlaEngine::osc_send_control_remove_plugin(%i)", pluginId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+15];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/remove_plugin");
        lo_send(m_oscData->target, target_path, "i", pluginId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    qDebug("CarlaEngine::osc_send_control_set_plugin_data(%i, %i, %i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(type != PLUGIN_NONE);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+17];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_plugin_data");
        lo_send(m_oscData->target, target_path, "iiiissssh", pluginId, type, category, hints, realName, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals)
{
    qDebug("CarlaEngine::osc_send_control_set_plugin_ports(%i, %i, %i, %i, %i, %i, %i, %i)", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+18];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_plugin_ports");
        lo_send(m_oscData->target, target_path, "iiiiiiii", pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals);
    }
}

void CarlaEngine::osc_send_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const double current)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_data(%i, %i, %i, %i, \"%s\", \"%s\", %g)", pluginId, index, type, hints, name, label, current);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(type != PARAMETER_UNKNOWN);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+20];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_parameter_data");
        lo_send(m_oscData->target, target_path, "iiiissd", pluginId, index, type, hints, name, label, current);
    }
}

void CarlaEngine::osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const double min, const double max, const double def, const double step, const double stepSmall, const double stepLarge)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_ranges(%i, %i, %g, %g, %g, %g, %g, %g)", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(min < max);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+22];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_parameter_ranges");
        lo_send(m_oscData->target, target_path, "iidddddd", pluginId, index, min, max, def, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_midi_cc(%i, %i, %i)", pluginId, index, cc);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_parameter_midi_cc");
        lo_send(m_oscData->target, target_path, "iii", pluginId, index, cc);
    }
}

void CarlaEngine::osc_send_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel)
{
    qDebug("CarlaEngine::osc_send_control_set_parameter_midi_channel(%i, %i, %i)", pluginId, index, channel);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(channel >= 0 && channel < 16);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+28];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_parameter_midi_channel");
        lo_send(m_oscData->target, target_path, "iii", pluginId, index, channel);
    }
}

void CarlaEngine::osc_send_control_set_parameter_value(const int32_t pluginId, const int32_t index, const double value)
{
#if DEBUG
    if (index < -1)
        qDebug("CarlaEngine::osc_send_control_set_parameter_value(%i, %s, %g)", pluginId, InternalParametersIndex2str((InternalParametersIndex)index), value);
    else
        qDebug("CarlaEngine::osc_send_control_set_parameter_value(%i, %i, %g)", pluginId, index, value);
#endif
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+21];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_parameter_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_control_set_default_value(%i, %i, %g)", pluginId, index, value);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+19];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_default_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, index, value);
    }
}

void CarlaEngine::osc_send_control_set_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_control_set_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+13];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_program");
        lo_send(m_oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_control_set_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+19];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_program_count");
        lo_send(m_oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name)
{
    qDebug("CarlaEngine::osc_send_control_set_program_name(%i, %i, \"%s\")", pluginId, index, name);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(name);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+18];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_program_name");
        lo_send(m_oscData->target, target_path, "iis", pluginId, index, name);
    }
}

void CarlaEngine::osc_send_control_set_midi_program(const int32_t pluginId, const int32_t index)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program(%i, %i)", pluginId, index);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+18];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_midi_program");
        lo_send(m_oscData->target, target_path, "ii", pluginId, index);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_count(const int32_t pluginId, const int32_t count)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program_count(%i, %i)", pluginId, count);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(count >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+24];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_midi_program_count");
        lo_send(m_oscData->target, target_path, "ii", pluginId, count);
    }
}

void CarlaEngine::osc_send_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name)
{
    qDebug("CarlaEngine::osc_send_control_set_midi_program_data(%i, %i, %i, %i, \"%s\")", pluginId, index, bank, program, name);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(index >= 0);
    CARLA_ASSERT(bank >= 0);
    CARLA_ASSERT(program >= 0);
    CARLA_ASSERT(name);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_midi_program_data");
        lo_send(m_oscData->target, target_path, "iiiis", pluginId, index, bank, program, name);
    }
}

void CarlaEngine::osc_send_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo)
{
    qDebug("CarlaEngine::osc_send_control_note_on(%i, %i, %i, %i)", pluginId, channel, note, velo);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);
    CARLA_ASSERT(velo > 0 && velo < 128);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+9];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/note_on");
        lo_send(m_oscData->target, target_path, "iiii", pluginId, channel, note, velo);
    }
}

void CarlaEngine::osc_send_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note)
{
    qDebug("CarlaEngine::osc_send_control_note_off(%i, %i, %i)", pluginId, channel, note);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(channel >= 0 && channel < 16);
    CARLA_ASSERT(note >= 0 && note < 128);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+10];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/note_off");
        lo_send(m_oscData->target, target_path, "iii", pluginId, channel, note);
    }
}

void CarlaEngine::osc_send_control_set_input_peak_value(const int32_t pluginId, const int32_t portId, const double value)
{
    //qDebug("CarlaEngine::osc_send_control_set_input_peak_value(%i, %i, %g)", pluginId, portId, value);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+22];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_input_peak_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, portId, value);
    }
}

void CarlaEngine::osc_send_control_set_output_peak_value(const int32_t pluginId, const int32_t portId, const double value)
{
    //qDebug("CarlaEngine::osc_send_control_set_output_peak_value(%i, %i, %g)", pluginId, portId, value);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_output_peak_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, portId, value);
    }
}

void CarlaEngine::osc_send_control_exit()
{
    qDebug("CarlaEngine::osc_send_control_exit()");
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+6];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/exit");
        lo_send(m_oscData->target, target_path, "");
    }
}
#else
void CarlaEngine::osc_send_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_audio_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+20];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_audio_count");
        lo_send(m_oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+19];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_midi_count");
        lo_send(m_oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_count(%i, %i, %i)", ins, outs, total);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(total >= 0 && total >= ins + outs);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+24];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_parameter_count");
        lo_send(m_oscData->target, target_path, "iii", ins, outs, total);
    }
}

void CarlaEngine::osc_send_bridge_program_count(const int32_t count)
{
    qDebug("CarlaEngine::osc_send_bridge_program_count(%i)", count);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(count >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+22];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_program_count");
        lo_send(m_oscData->target, target_path, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_count(const int32_t count)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_program_count(%i)", count);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(count >= 0);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+27];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_midi_program_count");
        lo_send(m_oscData->target, target_path, "i", count);
    }
}

void CarlaEngine::osc_send_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId)
{
    qDebug("CarlaEngine::osc_send_bridge_plugin_info(%i, %i, \"%s\", \"%s\", \"%s\", \"%s\", " P_INT64 ")", category, hints, name, label, maker, copyright, uniqueId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(name);
    CARLA_ASSERT(label);
    CARLA_ASSERT(maker);
    CARLA_ASSERT(copyright);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+20];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_plugin_info");
        lo_send(m_oscData->target, target_path, "iissssh", category, hints, name, label, maker, copyright, uniqueId);
    }
}

void CarlaEngine::osc_send_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_info(%i, \"%s\", \"%s\")", index, name, unit);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(name);
    CARLA_ASSERT(unit);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_parameter_info");
        lo_send(m_oscData->target, target_path, "iss", index, name, unit);
    }
}

void CarlaEngine::osc_send_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_data(%i, %i, %i, %i, %i, %i)", index, type, rindex, hints, midiChannel, midiCC);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_parameter_data");
        lo_send(m_oscData->target, target_path, "iiiiii", index, type, rindex, hints, midiChannel, midiCC);
    }
}

void CarlaEngine::osc_send_bridge_parameter_ranges(const int32_t index, const double def, const double min, const double max, const double step, const double stepSmall, const double stepLarge)
{
    qDebug("CarlaEngine::osc_send_bridge_parameter_ranges(%i, %g, %g, %g, %g, %g, %g)", index, def, min, max, step, stepSmall, stepLarge);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+25];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_parameter_ranges");
        lo_send(m_oscData->target, target_path, "idddddd", index, def, min, max, step, stepSmall, stepLarge);
    }
}

void CarlaEngine::osc_send_bridge_program_info(const int32_t index, const char* const name)
{
    qDebug("CarlaEngine::osc_send_bridge_program_info(%i, \"%s\")", index, name);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+21];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_program_info");
        lo_send(m_oscData->target, target_path, "is", index, name);
    }
}

void CarlaEngine::osc_send_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label)
{
    qDebug("CarlaEngine::osc_send_bridge_midi_program_info(%i, %i, %i, \"%s\")", index, bank, program, label);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+26];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_midi_program_info");
        lo_send(m_oscData->target, target_path, "iiis", index, bank, program, label);
    }
}

void CarlaEngine::osc_send_bridge_configure(const char* const key, const char* const value)
{
    qDebug("CarlaEngine::osc_send_bridge_configure(\"%s\", \"%s\")", key, value);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(key);
    CARLA_ASSERT(value);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+18];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_configure");
        lo_send(m_oscData->target, target_path, "ss", key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_parameter_value(const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_parameter_value(%i, %g)", index, value);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+28];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_parameter_value");
        lo_send(m_oscData->target, target_path, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_default_value(const int32_t index, const double value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_default_value(%i, %g)", index, value);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+26];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_default_value");
        lo_send(m_oscData->target, target_path, "id", index, value);
    }
}

void CarlaEngine::osc_send_bridge_set_program(const int32_t index)
{
    qDebug("CarlaEngine::osc_send_bridge_set_program(%i)", index);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+20];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_program");
        lo_send(m_oscData->target, target_path, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_midi_program(const int32_t index)
{
    qDebug("CarlaEngine::osc_send_bridge_set_midi_program(%i)", index);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+25];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_midi_program");
        lo_send(m_oscData->target, target_path, "i", index);
    }
}

void CarlaEngine::osc_send_bridge_set_custom_data(const char* const stype, const char* const key, const char* const value)
{
    qDebug("CarlaEngine::osc_send_bridge_set_custom_data(\"%s\", \"%s\", \"%s\")", stype, key, value);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+24];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_custom_data");
        lo_send(m_oscData->target, target_path, "sss", stype, key, value);
    }
}

void CarlaEngine::osc_send_bridge_set_chunk_data(const char* const chunkFile)
{
    qDebug("CarlaEngine::osc_send_bridge_set_chunk_data(\"%s\")", chunkFile);
    CARLA_ASSERT(m_oscData);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_chunk_data");
        lo_send(m_oscData->target, target_path, "s", chunkFile);
    }
}

void CarlaEngine::osc_send_bridge_set_input_peak_value(const int32_t portId, const double value)
{
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+28];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_input_peak_value");
        lo_send(m_oscData->target, target_path, "id", portId, value);
    }
}

void CarlaEngine::osc_send_bridge_set_output_peak_value(const int32_t portId, const double value)
{
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+29];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_output_peak_value");
        lo_send(m_oscData->target, target_path, "id", portId, value);
    }
}
#endif

CARLA_BACKEND_END_NAMESPACE

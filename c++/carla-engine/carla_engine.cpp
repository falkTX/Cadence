/*
 * Carla Engine
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

#include "carla_engine.hpp"
#include "carla_plugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Engine port (Base)

CarlaEngineBasePort::CarlaEngineBasePort(const bool isInput_, const ProcessMode processMode_)
    : isInput(isInput_),
      processMode(processMode_)
{
    qDebug("CarlaEngineBasePort::CarlaEngineBasePort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));

    buffer = nullptr;
}

CarlaEngineBasePort::~CarlaEngineBasePort()
{
    qDebug("CarlaEngineBasePort::~CarlaEngineBasePort()");
}

// -------------------------------------------------------------------------------------------------------------------
// Engine port (Audio)

CarlaEngineAudioPort::CarlaEngineAudioPort(const bool isInput, const ProcessMode processMode)
    : CarlaEngineBasePort(isInput, processMode)
{
    qDebug("CarlaEngineAudioPort::CarlaEngineAudioPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

CarlaEngineAudioPort::~CarlaEngineAudioPort()
{
    qDebug("CarlaEngineAudioPort::~CarlaEngineAudioPort()");
}

void CarlaEngineAudioPort::initBuffer(CarlaEngine* const)
{
}

// -------------------------------------------------------------------------------------------------------------------
// Engine port (Control)

CarlaEngineControlPort::CarlaEngineControlPort(const bool isInput, const ProcessMode processMode)
    : CarlaEngineBasePort(isInput, processMode)
{
    qDebug("CarlaEngineControlPort::CarlaEngineControlPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

void CarlaEngineControlPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
        buffer = isInput ? engine->rackControlEventsIn : engine->rackControlEventsOut;
#endif
}

uint32_t CarlaEngineControlPort::getEventCount()
{
    if (! isInput)
        return 0;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++, count++)
        {
            if (events[i].type == CarlaEngineNullEvent)
                break;
        }

        return count;
    }
#endif

    return 0;
}

const CarlaEngineControlEvent* CarlaEngineControlPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CARLA_ASSERT(index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS);

        const CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

        if (index < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS)
            return &events[index];
    }
#endif

    return nullptr;
}

void CarlaEngineControlPort::writeEvent(CarlaEngineControlEventType type, uint32_t time, uint8_t channel, uint16_t controller, double value)
{
    if (isInput)
        return;

    CARLA_ASSERT(buffer);
    CARLA_ASSERT(type != CarlaEngineNullEvent);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CarlaEngineControlEvent* const events = (CarlaEngineControlEvent*)buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_CONTROL_EVENTS; i++)
        {
            if (events[i].type != CarlaEngineNullEvent)
                continue;

            events[i].type  = type;
            events[i].time  = time;
            events[i].value = value;
            events[i].channel    = channel;
            events[i].controller = controller;
            break;
        }
    }
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// Engine port (MIDI)

CarlaEngineMidiPort::CarlaEngineMidiPort(const bool isInput, const ProcessMode processMode)
    : CarlaEngineBasePort(isInput, processMode)
{
    qDebug("CarlaEngineMidiPort::CarlaEngineMidiPort(%s, %s)", bool2str(isInput), ProcessMode2Str(processMode));
}

void CarlaEngineMidiPort::initBuffer(CarlaEngine* const engine)
{
    CARLA_ASSERT(engine);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
        buffer = isInput ? engine->rackMidiEventsIn : engine->rackMidiEventsOut;
#endif
}

uint32_t CarlaEngineMidiPort::getEventCount()
{
    if (! isInput)
        return 0;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        uint32_t count = 0;
        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++, count++)
        {
            if (events[i].size == 0)
                break;
        }

        return count;
    }
#endif

    return 0;
}

const CarlaEngineMidiEvent* CarlaEngineMidiPort::getEvent(uint32_t index)
{
    if (! isInput)
        return nullptr;

    CARLA_ASSERT(buffer);

#ifndef BUILD_BRIDGE
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        CARLA_ASSERT(index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS);

        const CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

        if (index < CarlaEngine::MAX_ENGINE_MIDI_EVENTS)
            return &events[index];
    }
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
    if (processMode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (size > 4)
            return;

        CarlaEngineMidiEvent* const events = (CarlaEngineMidiEvent*)buffer;

        for (unsigned short i=0; i < CarlaEngine::MAX_ENGINE_MIDI_EVENTS; i++)
        {
            if (events[i].size != 0)
                continue;

            events[i].time = time;
            events[i].size = size;
            memcpy(events[i].data, data, size);
            break;
        }
    }
#endif
}

//#ifdef CARLA_ENGINE_JACK
//float* CarlaEngineAudioPort::getJackAudioBuffer(uint32_t nframes)
//{
//#  ifndef BUILD_BRIDGE
//    if (CarlaEngine::processMode == PROCESS_MODE_CONTINUOUS_RACK)
//        return nullptr;
//#  endif
//    CARLA_ASSERT(handle.jackPort);
//    return (float*)jackbridge_port_get_buffer(handle.jackPort, nframes);
//}
//#endif

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngineClient::CarlaEngineClient(const CarlaEngineType engineType_, const ProcessMode processMode_)
    : engineType(engineType_),
      processMode(processMode_)
{
    qDebug("CarlaEngineClient::CarlaEngineClient(%s, %s)", "" /* TODO */, ProcessMode2Str(processMode));
    CARLA_ASSERT(engineType != CarlaEngineTypeNull);

    m_active  = false;
    m_latency = 0;
}

CarlaEngineClient::~CarlaEngineClient()
{
    qDebug("CarlaEngineClient::~CarlaEngineClient()");
    CARLA_ASSERT(! m_active);
}

void CarlaEngineClient::activate()
{
    qDebug("CarlaEngineClient::activate()");
    CARLA_ASSERT(! m_active);

    m_active = true;
}

void CarlaEngineClient::deactivate()
{
    qDebug("CarlaEngineClient::deactivate()");
    CARLA_ASSERT(m_active);

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

    return true;
}

uint32_t CarlaEngineClient::getLatency() const
{
    return m_latency;
}

void CarlaEngineClient::setLatency(const uint32_t samples)
{
    m_latency = samples;
}

//const CarlaEngineBasePort* CarlaEngineClient::addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput)
//{
//    qDebug("CarlaEngineClient::addPort(%i, \"%s\", %s)", portType, name, bool2str(isInput));

//    CarlaEngineBasePort::Handle portHandle;

//#ifndef BUILD_BRIDGE
//    if (CarlaEngine::processMode != PROCESS_MODE_CONTINUOUS_RACK)
//#endif
//    {
//#ifdef CARLA_ENGINE_JACK
//        if (engineType == CarlaEngineTypeJack)
//        {
//            switch (portType)
//            {
//            case CarlaEnginePortTypeAudio:
//                portHandle = jackbridge_port_register((jack_client_t*)handle, name, JACK_DEFAULT_AUDIO_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
//                break;
//            case CarlaEnginePortTypeControl:
//            case CarlaEnginePortTypeMIDI:
//                portHandle = jackbridge_port_register((jack_client_t*)handle, name, JACK_DEFAULT_MIDI_TYPE, isInput ? JackPortIsInput : JackPortIsOutput, 0);
//                break;
//            }
//        }
//#endif
//    }

//    switch (portType)
//    {
//    case CarlaEnginePortTypeAudio:
//        return new CarlaEngineAudioPort(portHandle, isInput);
//    case CarlaEnginePortTypeControl:
//        return new CarlaEngineControlPort(portHandle, isInput);
//    case CarlaEnginePortTypeMIDI:
//        return new CarlaEngineMidiPort(portHandle, isInput);
//    }

//    qCritical("CarlaEngineClient::addPort(%i, \"%s\", %s) - invalid type", portType, name, bool2str(isInput));
//    return nullptr;
//}

//void CarlaEngineClient::removePort(CarlaEngineBasePort* const port)
//{
//#ifndef BUILD_BRIDGE
//    if (CarlaEngine::processMode == PROCESS_MODE_CONTINUOUS_RACK)
//        return;
//#endif

//#ifdef CARLA_ENGINE_JACK
//    if (engineType == CarlaEngineTypeJack)
//    {
//        if (handle && port->handle)
//            jackbridge_port_unregister((jack_client_t*)handle, (jack_port_t*)port->handle);
//    }
//#endif
//}

// -------------------------------------------------------------------------------------------------------------------
// Carla Engine Client

CarlaEngine::CarlaEngine()
    : m_thread(this),
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
}

// -----------------------------------------------------------------------
// Static values

unsigned int CarlaEngine::getDriverCount()
{
    unsigned int count = 0;
#ifdef CARLA_ENGINE_JACK
    count += 1;
#endif
#ifdef CARLA_ENGINE_RTAUDIO
    count += getRtAudioApiCount();
#endif
    return count;
}

const char* CarlaEngine::getDriverName(unsigned int index)
{
#ifdef CARLA_ENGINE_JACK
    if (index == 0)
        return "JACK";
    else
        index -= 1;
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    if (index < getRtAudioApiCount())
        return getRtAudioApiName(index);
#endif

    qWarning("CarlaEngine::getDriverName(%i) - invalid index", index);
    return nullptr;
}

CarlaEngine* CarlaEngine::newDriverByName(const char* const driverName)
{
#ifdef CARLA_ENGINE_JACK
    if (strcmp(driverName, "JACK") == 0)
        return newJack();
#else
    if (false)
        pass();
#endif

#ifdef CARLA_ENGINE_RTAUDIO
# ifdef __LINUX_ALSA__
    else if (strcmp(driverName, "ALSA") == 0)
        return newRtAudio(RTAUDIO_LINUX_ALSA);
# endif
# ifdef __LINUX_PULSE__
    else if (strcmp(driverName, "PulseAudio") == 0)
        return newRtAudio(RTAUDIO_LINUX_PULSE);
# endif
# ifdef __LINUX_OSS__
    else if (strcmp(driverName, "OSS") == 0)
        return newRtAudio(RTAUDIO_LINUX_OSS);
# endif
# ifdef __UNIX_JACK__
    else if (strcmp(driverName, "JACK (RtAudio)") == 0)
        return newRtAudio(RTAUDIO_UNIX_JACK);
# endif
# ifdef __MACOSX_CORE__
    else if (strcmp(driverName, "CoreAudio") == 0)
        return newRtAudio(RTAUDIO_MACOSX_CORE);
# endif
# ifdef __WINDOWS_ASIO__
    else if (strcmp(driverName, "ASIO") == 0)
        return newRtAudio(RTAUDIO_WINDOWS_ASIO);
# endif
# ifdef __WINDOWS_DS__
    else if (strcmp(driverName, "DirectSound") == 0)
        return newRtAudio(RTAUDIO_WINDOWS_DS);
# endif
#endif

    return nullptr;
}

// -----------------------------------------------------------------------
// Maximum values

int CarlaEngine::maxClientNameSize()
{
//#ifdef CARLA_ENGINE_JACK
//# ifndef BUILD_BRIDGE
//    if (type() == CarlaEngineTypeJack && processMode != PROCESS_MODE_CONTINUOUS_RACK)
//# endif
//        return jackbridge_client_name_size();
//#endif
    return STR_MAX/2;
}

int CarlaEngine::maxPortNameSize()
{
//#ifdef CARLA_ENGINE_JACK
//# ifndef BUILD_BRIDGE
//    if (type() == CarlaEngineTypeJack && processMode != PROCESS_MODE_CONTINUOUS_RACK)
//# endif
//        return jackbridge_port_name_size();
//#endif
    return STR_MAX;
}

unsigned short CarlaEngine::maxPluginNumber()
{
    return m_maxPluginNumber;
}

// -----------------------------------------------------------------------
// Virtual, per-engine type calls

bool CarlaEngine::init(const char* const clientName)
{
    qDebug("CarlaEngine::init(\"%s\")", clientName);

#ifndef BUILD_BRIDGE
    m_osc.init(clientName);
    m_oscData = m_osc.getControlData();

    if (strcmp(clientName, "Carla") != 0)
        carla_setprocname(clientName);
#endif

    m_thread.startNow();

    return true;
}

bool CarlaEngine::close()
{
    qDebug("CarlaEngine::close()");

    m_thread.stopNow();

#ifndef BUILD_BRIDGE
    osc_send_control_exit();
    m_osc.close();
#endif
    m_oscData = nullptr;

    m_maxPluginNumber = 0;

    name.clear();

    return true;
}

// -----------------------------------------------------------------------
// Plugin management

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

const char* CarlaEngine::getUniquePluginName(const char* const name)
{
    qDebug("CarlaEngine::getUniquePluginName(\"%s\")", name);
    CARLA_ASSERT(name);

    CarlaString sname(name);

    if (sname.isEmpty())
        sname = "(No name)";

    sname.truncate(maxClientNameSize()-5-1); // 5 = strlen(" (10)")
    sname.replace(':', '.'); // ':' is used in JACK1 to split client/port names

    for (unsigned short i=0; i < m_maxPluginNumber; i++)
    {
        // Check if unique name already exists
        if (m_uniqueNames[i] && sname == m_uniqueNames[i])
        {
            // Check if string has already been modified
            size_t len = sname.length();

            // 1 digit, ex: " (2)"
            if (sname[len-4] == ' ' && sname[len-3] == '(' && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                int number = sname[len-2]-'0';

                if (number == 9)
                {
                    // next number is 10, 2 digits
                    sname.truncate(len-4);
                    sname += " (10)";
                    //sname.replace(" (9)", " (10)");
                }
                else
                    sname[len-2] = '0'+number+1;

                continue;
            }

            // 2 digits, ex: " (11)"
            if (sname[len-5] == ' ' && sname[len-4] == '(' && sname.isDigit(len-3) && sname.isDigit(len-2) && sname[len-1] == ')')
            {
                char n2 = sname[len-2];
                char n3 = sname[len-3];

                if (n2 == '9')
                {
                    n2 = '0';
                    n3 = n3+1;
                }
                else
                    n2 = n2+1;

                sname[len-2] = n2;
                sname[len-3] = n3;

                continue;
            }

            // Modify string if not
            sname += " (2)";
        }
    }

    return strdup(sname);
}

short CarlaEngine::addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra)
{
    return addPlugin(BINARY_NATIVE, ptype, filename, name, label, extra);
}

short CarlaEngine::addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra)
{
    qDebug("CarlaEngine::addPlugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", BinaryType2Str(btype), PluginType2Str(ptype), filename, name, label, extra);
    CARLA_ASSERT(btype != BINARY_NONE);
    CARLA_ASSERT(ptype != PLUGIN_NONE);
    CARLA_ASSERT(filename);
    CARLA_ASSERT(label);

    if (m_maxPluginNumber == 0)
    {
#ifdef BUILD_BRIDGE
        m_maxPluginNumber = MAX_PLUGINS;
#else
        m_maxPluginNumber = (options.processMode == PROCESS_MODE_CONTINUOUS_RACK) ? 16 : MAX_PLUGINS;
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
    if (btype != BINARY_NATIVE || (options.preferPluginBridges && /*getBinaryBidgePath(btype) &&*/ type() == CarlaEngineTypeJack))
    {
#  ifdef CARLA_ENGINE_JACK
        if (options.processMode != CarlaBackend::PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            setLastError("Can only use bridged plugins in JACK Multi-Client mode");
            return -1;
        }
#  endif

        if (type() != CarlaEngineTypeJack)
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
#ifndef BUILD_BRIDGE
        case PLUGIN_INTERNAL:
            plugin = CarlaPlugin::newNative(init);
            break;
#endif
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
#ifndef BUILD_BRIDGE
        case PLUGIN_GIG:
            plugin = CarlaPlugin::newGIG(init);
            break;
        case PLUGIN_SF2:
            plugin = CarlaPlugin::newSF2(init);
            break;
        case PLUGIN_SFZ:
            plugin = CarlaPlugin::newSFZ(init);
            break;
#else
        default:
            break;
#endif
        }
    }

    if (! plugin)
        return -1;

    const short id = plugin->id();

    m_carlaPlugins[id] = plugin;
    m_uniqueNames[id]  = plugin->name();

    if (! m_thread.isRunning())
        m_thread.startNow();

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

        m_thread.stopNow();

        processLock();
        plugin->setEnabled(false);
        m_carlaPlugins[id] = nullptr;
        m_uniqueNames[id]  = nullptr;
        processUnlock();

        delete plugin;

#ifndef BUILD_BRIDGE
        osc_send_control_remove_plugin(id);

        if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
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
        {
            // only re-start check thread if there are still plugins left
            for (unsigned short i=0; i < m_maxPluginNumber; i++)
            {
                if (m_carlaPlugins[i])
                {
                    m_thread.startNow();
                    break;
                }
            }
        }

        return true;
    }

    qCritical("CarlaEngine::removePlugin(%i) - could not find plugin", id);
    setLastError("Could not find plugin to remove");
    return false;
}

void CarlaEngine::removeAllPlugins()
{
    qDebug("CarlaEngine::removeAllPlugins()");

    m_thread.stopNow();

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
    carla_zeroF(outBuf[0], frames);
    carla_zeroF(outBuf[1], frames);
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
                carla_zeroF(outBuf[0], frames);
                carla_zeroF(outBuf[1], frames);
                memset(rackMidiEventsOut, 0, sizeof(CarlaEngineMidiEvent)*MAX_ENGINE_MIDI_EVENTS);
            }

            // process
            plugin->engineProcessLock();

            plugin->initBuffers();

            if (options.processHighPrecision)
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

            // set peaks
            {
                double inPeak1  = 0.0;
                double inPeak2  = 0.0;
                double outPeak1 = 0.0;
                double outPeak2 = 0.0;

                for (uint32_t k=0; k < frames; k++)
                {
                    if (std::abs(inBuf[0][k]) > inPeak1)
                        inPeak1 = std::abs(inBuf[0][k]);
                    if (std::abs(inBuf[1][k]) > inPeak2)
                        inPeak2 = std::abs(inBuf[1][k]);
                    if (std::abs(outBuf[0][k]) > outPeak1)
                        outPeak1 = std::abs(outBuf[0][k]);
                    if (std::abs(outBuf[1][k]) > outPeak2)
                        outPeak2 = std::abs(outBuf[1][k]);
                }

                m_insPeak[i*MAX_PEAKS + 0] = inPeak1;
                m_insPeak[i*MAX_PEAKS + 1] = inPeak2;
                m_outsPeak[i*MAX_PEAKS + 0] = outPeak1;
                m_outsPeak[i*MAX_PEAKS + 1] = outPeak2;
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

const CarlaEngineTimeInfo* CarlaEngine::getTimeInfo() const
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
    qDebug("CarlaEngine::callback(%s, %i, %i, %i, %f)", CallbackType2Str(action), pluginId, value1, value2, value3);

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
// Error handling

const char* CarlaEngine::getLastError() const
{
    return m_lastError;
}

void CarlaEngine::setLastError(const char* const error)
{
    m_lastError = error;
}
// -----------------------------------------------------------------------
// Global options

#ifndef BUILD_BRIDGE

#define CARLA_ENGINE_SET_OPTION_RUNNING_CHECK \
    if (isRunning()) \
        return qCritical("CarlaEngine::setOption(%s, %i, \"%s\") - Cannot set this option while engine is running!", OptionsType2Str(option), value, valueStr);

void CarlaEngine::setOption(const OptionsType option, const int value, const char* const valueStr)
{
    qDebug("CarlaEngine::setOption(%s, %i, \"%s\")", OptionsType2Str(option), value, valueStr);

    switch (option)
    {
    case OPTION_PROCESS_NAME:
        carla_setprocname(valueStr);
        break;

    case OPTION_PROCESS_MODE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK

        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_PATCHBAY)
            return qCritical("CarlaEngine::setOption(%s, %i, \"%s\") - invalid value", OptionsType2Str(option), value, valueStr);

        options.processMode = static_cast<ProcessMode>(value);
        break;

    case OPTION_PROCESS_HIGH_PRECISION:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.processHighPrecision = value;
        break;

    case OPTION_MAX_PARAMETERS:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.maxParameters = (value > 0) ? value : MAX_PARAMETERS;
        break;

    case OPTION_PREFERRED_BUFFER_SIZE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.preferredBufferSize = value;
        break;

    case OPTION_PREFERRED_SAMPLE_RATE:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.preferredSampleRate = value;
        break;

    case OPTION_FORCE_STEREO:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.forceStereo = value;
        break;

    case OPTION_USE_DSSI_VST_CHUNKS:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.useDssiVstChunks = value;
        break;

    case OPTION_PREFER_PLUGIN_BRIDGES:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.preferPluginBridges = value;
        break;

    case OPTION_PREFER_UI_BRIDGES:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.preferUiBridges = value;
        break;

    case OPTION_OSC_UI_TIMEOUT:
        CARLA_ENGINE_SET_OPTION_RUNNING_CHECK
        options.oscUiTimeout = value/100;
        break;

    case OPTION_PATH_LADSPA:
        carla_setenv("LADSPA_PATH", valueStr);
        break;
    case OPTION_PATH_DSSI:
        carla_setenv("DSSI_PATH", valueStr);
        break;
    case OPTION_PATH_LV2:
        carla_setenv("LV2_PATH", valueStr);
        break;
    case OPTION_PATH_VST:
        carla_setenv("VST_PATH", valueStr);
        break;
    case OPTION_PATH_GIG:
        carla_setenv("GIG_PATH", valueStr);
        break;
    case OPTION_PATH_SF2:
        carla_setenv("SF2_PATH", valueStr);
        break;
    case OPTION_PATH_SFZ:
        carla_setenv("SFZ_PATH", valueStr);
        break;

    case OPTION_PATH_BRIDGE_POSIX32:
        options.bridge_posix32 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_POSIX64:
        options.bridge_posix64 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_WIN32:
        options.bridge_win32 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_WIN64:
        options.bridge_win64 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        options.bridge_lv2gtk2 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK3:
        options.bridge_lv2gtk3 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        options.bridge_lv2qt4 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        options.bridge_lv2x11 = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_HWND:
        options.bridge_vsthwnd = valueStr;
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        options.bridge_vstx11 = valueStr;
        break;
    }
}
#endif

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

bool CarlaEngine::idleOsc()
{
    return m_osc.idle();
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
        qDebug("CarlaEngine::osc_send_control_set_parameter_value(%i, %s, %g)", pluginId, InternalParametersIndex2Str((InternalParametersIndex)index), value);
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

void CarlaEngine::osc_send_control_set_input_peak_value(const int32_t pluginId, const int32_t portId)
{
    //qDebug("CarlaEngine::osc_send_control_set_input_peak_value(%i, %i)", pluginId, portId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+22];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_input_peak_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, portId, m_insPeak[pluginId*MAX_PEAKS + portId-1]);
    }
}

void CarlaEngine::osc_send_control_set_output_peak_value(const int32_t pluginId, const int32_t portId)
{
    //qDebug("CarlaEngine::osc_send_control_set_output_peak_value(%i, %i)", pluginId, portId);
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(pluginId >= 0 && pluginId < m_maxPluginNumber);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+23];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/set_output_peak_value");
        lo_send(m_oscData->target, target_path, "iid", pluginId, portId, m_outsPeak[pluginId*MAX_PEAKS + portId-1]);
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

void CarlaEngine::osc_send_bridge_set_inpeak(const int32_t portId)
{
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+28];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_inpeak");
        lo_send(m_oscData->target, target_path, "id", portId, m_insPeak[portId-1]);
    }
}

void CarlaEngine::osc_send_bridge_set_outpeak(const int32_t portId)
{
    CARLA_ASSERT(m_oscData);
    CARLA_ASSERT(portId == 1 || portId == 2);

    if (m_oscData && m_oscData->target)
    {
        char target_path[strlen(m_oscData->path)+29];
        strcpy(target_path, m_oscData->path);
        strcat(target_path, "/bridge_set_outpeak");
        lo_send(m_oscData->target, target_path, "id", portId, m_insPeak[portId-1]);
    }
}
#endif

CARLA_BACKEND_END_NAMESPACE

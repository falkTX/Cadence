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

#include "carla_plugin.h"
#include "plugins/carla_native.h"

CARLA_BACKEND_START_NAMESPACE

struct NativePluginMidiData {
    uint32_t count;
    uint32_t* rindexes;
    CarlaEngineMidiPort** ports;

    NativePluginMidiData()
        : count(0),
          rindexes(nullptr),
          ports(nullptr) {}
};

class NativePluginScopedInitiliazer
{
public:
    NativePluginScopedInitiliazer()
    {
    }

    ~NativePluginScopedInitiliazer()
    {
        for (size_t i=0; i < descriptors.size(); i++)
        {
            const PluginDescriptor* const desc = descriptors[i];

            if (desc->_fini)
                desc->_fini((struct _PluginDescriptor*)desc);
        }

        descriptors.clear();
    }

    void initializeIfNeeded(const PluginDescriptor* const desc)
    {
        if (descriptors.empty() || std::find(descriptors.begin(), descriptors.end(), desc) == descriptors.end())
        {
            if (desc->_init)
                desc->_init((struct _PluginDescriptor*)desc);

            descriptors.push_back(desc);
        }
    }

private:
    std::vector<const PluginDescriptor*> descriptors;
};

static NativePluginScopedInitiliazer scopedInitliazer;

class NativePlugin : public CarlaPlugin
{
public:
    NativePlugin(CarlaEngine* const engine, const unsigned short id)
        : CarlaPlugin(engine, id)
    {
        qDebug("NativePlugin::NativePlugin()");

        m_type = PLUGIN_INTERNAL;

        descriptor = nullptr;
        handle     = nullptr;
    }

    ~NativePlugin()
    {
        qDebug("NativePlugin::~NativePlugin()");

        if (descriptor)
        {
            if (descriptor->deactivate && m_activeBefore)
            {
                if (handle)
                    descriptor->deactivate(handle);
                //if (h2)
                //    descriptor->deactivate(h2);
            }

            if (descriptor->cleanup)
            {
                if (handle)
                    descriptor->cleanup(handle);
                //if (h2)
                //    descriptor->cleanup(h2);
            }
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        Q_ASSERT(descriptor);

        if (descriptor)
            return (PluginCategory)descriptor->category;

        return getPluginCategoryFromName(m_name);
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount()
    {
        return mIn.count;
    }

    uint32_t midiOutCount()
    {
        return mOut.count;
    }

    uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(parameterId < param.count);

        int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->portCount)
        {
            const PluginPort* const port = &descriptor->ports[rindex];

            if (port)
                return port->scalePointCount;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(parameterId < param.count);

        if (descriptor && handle)
            return descriptor->get_parameter_value(handle, parameterId);

        return 0.0;
    }

    double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(parameterId < param.count);
        Q_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        const int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->portCount)
        {
            const PluginPort* const port = &descriptor->ports[rindex];

            if (port && scalePointId < port->scalePointCount)
            {
                const PluginPortScalePoint* const scalePoint = &port->scalePoints[scalePointId];

                if (scalePoint)
                    return scalePoint->value;
            }
        }

        return 0.0;
    }

    void getLabel(char* const strBuf)
    {
        Q_ASSERT(descriptor);

        if (descriptor && descriptor->label)
            strncpy(strBuf, descriptor->label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        Q_ASSERT(descriptor);

        if (descriptor && descriptor->maker)
            strncpy(strBuf, descriptor->maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        Q_ASSERT(descriptor);

        if (descriptor && descriptor->copyright)
            strncpy(strBuf, descriptor->copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        Q_ASSERT(descriptor);

        if (descriptor && descriptor->name)
            strncpy(strBuf, descriptor->name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(parameterId < param.count);

        const int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->portCount)
        {
            const PluginPort* const port = &descriptor->ports[rindex];

            if (port && port->name)
            {
                strncpy(strBuf, port->name, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterName(parameterId, strBuf);
    }

    void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(parameterId < param.count);

        if (descriptor && handle)
        {
            const int32_t rindex = param.data[parameterId].rindex;

            const char* const text = descriptor->get_parameter_text(handle, rindex);

            if (text)
            {
                strncpy(strBuf, text, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterText(parameterId, strBuf);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(parameterId < param.count);

        if (descriptor && handle)
        {
            const int32_t rindex = param.data[parameterId].rindex;

            const char* const unit = descriptor->get_parameter_unit(handle, rindex);

            if (unit)
            {
                strncpy(strBuf, unit, STR_MAX);
                return;
            }
        }

        CarlaPlugin::getParameterUnit(parameterId, strBuf);
    }

    void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(parameterId < param.count);
        Q_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->portCount)
        {
            const PluginPort* const port = &descriptor->ports[rindex];

            if (port && scalePointId < port->scalePointCount)
            {
                const PluginPortScalePoint* const scalePoint = &port->scalePoints[scalePointId];

                if (scalePoint && scalePoint->label)
                {
                    strncpy(strBuf, scalePoint->label, STR_MAX);
                    return;
                }
            }
        }

        CarlaPlugin::getParameterScalePointLabel(parameterId, scalePointId, strBuf);
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(parameterId < param.count);

        if (descriptor && handle)
            descriptor->set_parameter_value(handle, parameterId, fixParameterValue(value, param.ranges[parameterId]));

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const CustomDataType type, const char* const key, const char* const value, const bool sendGui)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(type == CUSTOM_DATA_STRING);
        Q_ASSERT(key);
        Q_ASSERT(value);

        if (type != CUSTOM_DATA_STRING)
            return qCritical("NativePlugin::setCustomData(%s, \"%s\", \"%s\", %s) - type is not string", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (! key)
            return qCritical("NativePlugin::setCustomData(%s, \"%s\", \"%s\", %s) - key is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (! value)
            return qCritical("Nativelugin::setCustomData(%s, \"%s\", \"%s\", %s) - value is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (descriptor && handle)
            descriptor->set_custom_data(handle, key, value);

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(handle);
        Q_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)midiprog.count)
            return;

        if (descriptor && handle && index >= 0)
        {
            if (x_engine->isOffline())
            {
                const CarlaEngine::ScopedLocker m(x_engine, block);
                descriptor->set_midi_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }
            else
            {
                const ScopedDisabler m(this, block);
                descriptor->set_midi_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        Q_ASSERT(descriptor);

        if (descriptor && handle)
            descriptor->show_gui(handle, yesNo);
    }

    void idleGui()
    {
        // FIXME - this should not be called if there's no GUI!
        Q_ASSERT(descriptor);

        if (descriptor && descriptor->idle_gui && handle)
            descriptor->idle_gui(handle);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("NativePlugin::reload() - start");
        Q_ASSERT(descriptor);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aIns, aOuts, mIns, mOuts, params, j;
        aIns = aOuts = mIns = mOuts = params = 0;

        const double sampleRate  = x_engine->getSampleRate();
        const uint32_t portCount = descriptor->portCount;

        for (uint32_t i=0; i < portCount; i++)
        {
            const PortType portType  = descriptor->ports[i].type;
            const uint32_t portHints = descriptor->ports[i].hints;

            if (portType == PORT_TYPE_AUDIO)
            {
                if (portHints & PORT_HINT_IS_OUTPUT)
                    aOuts += 1;
                else
                    aIns += 1;
            }
            else if (portType == PORT_TYPE_MIDI)
            {
                if (portHints & PORT_HINT_IS_OUTPUT)
                    mOuts += 1;
                else
                    mIns += 1;
            }
            else if (portType == PORT_TYPE_PARAMETER)
                params += 1;
        }

        if (aIns > 0)
        {
            aIn.ports    = new CarlaEngineAudioPort*[aIns];
            aIn.rindexes = new uint32_t[aIns];
        }

        if (aOuts > 0)
        {
            aOut.ports    = new CarlaEngineAudioPort*[aOuts];
            aOut.rindexes = new uint32_t[aOuts];
        }

        if (mIns > 0)
        {
            mIn.ports    = new CarlaEngineMidiPort*[mIns];
            mIn.rindexes = new uint32_t[mIns];
        }

        if (mOuts > 0)
        {
            mOut.ports    = new CarlaEngineMidiPort*[mOuts];
            mOut.rindexes = new uint32_t[mOuts];
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
        }

        const int portNameSize = CarlaEngine::maxPortNameSize() - 2;
        char portName[portNameSize];
        bool needsCtrlIn  = false;
        bool needsCtrlOut = false;

        for (uint32_t i=0; i < portCount; i++)
        {
            const PortType portType  = descriptor->ports[i].type;
            const uint32_t portHints = descriptor->ports[i].hints;

            if (portType == PORT_TYPE_AUDIO || portType == PORT_TYPE_MIDI)
            {
                if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
                {
                    strcpy(portName, m_name);
                    strcat(portName, ":");
                    strncat(portName, descriptor->ports[i].name, portNameSize/2);
                }
                else
                    strncpy(portName, descriptor->ports[i].name, portNameSize);
            }

            if (portType == PORT_TYPE_AUDIO)
            {
                if (portHints & PORT_HINT_IS_OUTPUT)
                {
                    j = aOut.count++;
                    aOut.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aOut.rindexes[j] = i;
                    needsCtrlIn = true;
                }
                else
                {
                    j = aIn.count++;
                    aIn.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    aIn.rindexes[j] = i;
                }
            }
            else if (portType == PORT_TYPE_MIDI)
            {
                if (portHints & PORT_HINT_IS_OUTPUT)
                {
                    j = aOut.count++;
                    mOut.ports[j]    = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
                    mOut.rindexes[j] = i;
                }
                else
                {
                    j = aIn.count++;
                    mIn.ports[j]    = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    mIn.rindexes[j] = i;
                }
            }
            else if (portType == PORT_TYPE_PARAMETER)
            {
                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midiChannel = 0;
                param.data[j].midiCC = -1;

                double min, max, def, step, stepSmall, stepLarge;

                ::ParameterRanges ranges = { 0.0, 0.0, 1.0, 0.01, 0.0001, 0.1 };
                descriptor->get_parameter_ranges(handle, i, &ranges);

                // min value
                min = ranges.min;

                // max value
                min = ranges.max;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                // default value
                def = ranges.def;

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (portHints & PORT_HINT_USES_SAMPLE_RATE)
                {
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (portHints & PORT_HINT_IS_BOOLEAN)
                {
                    step = max - min;
                    stepSmall = step;
                    stepLarge = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (portHints & PORT_HINT_IS_INTEGER)
                {
                    step = 1.0;
                    stepSmall = 1.0;
                    stepLarge = 10.0;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    stepSmall = range/1000.0;
                    stepLarge = range/10.0;
                }

                if (portHints & PORT_HINT_IS_OUTPUT)
                {
                    param.data[j].type = PARAMETER_OUTPUT;
                    needsCtrlOut = true;
                }
                else
                {
                    param.data[j].type = PARAMETER_INPUT;
                    needsCtrlIn = true;
                }

                // extra parameter hints
                if (portHints & PORT_HINT_IS_ENABLED)
                    param.data[j].hints |= PARAMETER_IS_ENABLED;

                if (portHints & PORT_HINT_IS_AUTOMABLE)
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;

                if (portHints & PORT_HINT_IS_LOGARITHMIC)
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (portHints & PORT_HINT_USES_SCALEPOINTS)
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (portHints & PORT_HINT_USES_CUSTOM_TEXT)
                    param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].stepSmall = stepSmall;
                param.ranges[j].stepLarge = stepLarge;
            }
        }

        if (needsCtrlIn)
        {
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-in");
            }
            else
                strcpy(portName, "control-in");

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCtrlOut)
        {
            if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-out");
            }
            else
                strcpy(portName, "control-out");

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        aIn.count   = aIns;
        aOut.count  = aOuts;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (aOuts > 0 && (aIns == aOuts || aIns == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aOuts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aOuts >= 2 && aOuts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        m_hints |= getPluginHintsFromNative(descriptor->hints);

        reloadPrograms(true);

        x_client->activate();

        qDebug("NativePlugin::reload() - end");
    }

    // -------------------------------------------------------------------
    // Cleanup

    void removeClientPorts()
    {
        qDebug("NativePlugin::removeClientPorts() - start");

        for (uint32_t i=0; i < mIn.count; i++)
        {
            delete mIn.ports[i];
            mIn.ports[i] = nullptr;
        }

        for (uint32_t i=0; i < mOut.count; i++)
        {
            delete mOut.ports[i];
            mOut.ports[i] = nullptr;
        }

        CarlaPlugin::removeClientPorts();

        qDebug("NativePlugin::removeClientPorts() - end");
    }

    void initBuffers()
    {
        uint32_t i;

        for (i=0; i < mIn.count; i++)
        {
            if (mIn.ports[i])
                mIn.ports[i]->initBuffer(x_engine);
        }

        for (i=0; i < mOut.count; i++)
        {
            if (mOut.ports[i])
                mOut.ports[i]->initBuffer(x_engine);
        }

        CarlaPlugin::initBuffers();
    }

    void deleteBuffers()
    {
        qDebug("NativePlugin::deleteBuffers() - start");

        if (mIn.count > 0)
        {
            delete[] mIn.ports;
            delete[] mIn.rindexes;
        }

        if (mOut.count > 0)
        {
            delete[] mOut.ports;
            delete[] mOut.rindexes;
        }

        mIn.count = 0;
        mIn.ports = nullptr;
        mIn.rindexes = nullptr;

        mOut.count = 0;
        mOut.ports = nullptr;
        mOut.rindexes = nullptr;

        CarlaPlugin::deleteBuffers();

        qDebug("NativePlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    static size_t getPluginCount()
    {
        return pluginDescriptors.size();
    }

    static const PluginDescriptor* getPlugin(size_t index)
    {
        Q_ASSERT(index < pluginDescriptors.size());

        if (index < pluginDescriptors.size())
            return pluginDescriptors[index];

        return nullptr;
    }

    static void registerPlugin(const PluginDescriptor* desc)
    {
        pluginDescriptors.push_back(desc);
    }

    // -------------------------------------------------------------------

    bool init(const char* const name, const char* const label)
    {
        // ---------------------------------------------------------------
        // get descriptor that matches label

        for (size_t i=0; i < pluginDescriptors.size(); i++)
        {
            descriptor = pluginDescriptors[i];

            if (! descriptor)
                break;
            if (strcmp(descriptor->label, label) == 0)
                break;

            descriptor = nullptr;
        }

        if (! descriptor)
        {
            setLastError("Invalid internal plugin");
            return false;
        }

        scopedInitliazer.initializeIfNeeded(descriptor);

        // ---------------------------------------------------------------
        // initialize plugin

        handle = descriptor->instantiate((struct _PluginDescriptor*)descriptor, nullptr); // TODO - host

        if (! handle)
        {
            setLastError("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        if (name)
            m_name = x_engine->getUniqueName(name);
        else
            m_name = x_engine->getUniqueName(descriptor->name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            setLastError("Failed to register plugin client");
            return false;
        }

        return true;
    }

private:
    const PluginDescriptor* descriptor;
    PluginHandle handle;

    NativePluginMidiData mIn;
    NativePluginMidiData mOut;

    static std::vector<const PluginDescriptor*> pluginDescriptors;
};

std::vector<const PluginDescriptor*> NativePlugin::pluginDescriptors;

CarlaPlugin* CarlaPlugin::newNative(const initializer& init)
{
    qDebug("CarlaPlugin::newNative(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);

    short id = init.engine->getNewPluginId();

    if (id < 0 || id > CarlaEngine::maxPluginNumber())
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    NativePlugin* const plugin = new NativePlugin(init.engine, id);

    if (! plugin->init(init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();
    plugin->registerToOsc();

    return plugin;
}

size_t CarlaPlugin::getNativePluginCount()
{
    return NativePlugin::getPluginCount();
}

const PluginDescriptor* CarlaPlugin::getNativePlugin(size_t index)
{
    return NativePlugin::getPlugin(index);
}

CARLA_BACKEND_END_NAMESPACE

void carla_register_native_plugin(const PluginDescriptor* desc)
{
    CarlaBackend::NativePlugin::registerPlugin(desc);
}

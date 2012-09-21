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

        if (handle)
            pass();
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
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        Q_ASSERT(descriptor);
        Q_ASSERT(parameterId < param.count);

        //return descriptor->get_parameter_value();
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

        int32_t rindex = param.data[parameterId].rindex;

        if (descriptor && rindex < (int32_t)descriptor->portCount)
            strncpy(strBuf, descriptor->ports[rindex].name, STR_MAX);
        else
            CarlaPlugin::getParameterName(parameterId, strBuf);
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
        // get plugin

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

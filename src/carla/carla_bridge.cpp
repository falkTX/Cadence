/*
 * JACK Backend code for Carla
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

#include "carla_plugin.h"
#include "carla_threads.h"

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(PluginType type) : CarlaPlugin()
    {
        qDebug("BridgePlugin::BridgePlugin()");
        m_type = type;
    }

    virtual ~BridgePlugin()
    {
        qDebug("BridgePlugin::~BridgePlugin()");
    }

    virtual void reload()
    {
        m_hints = 0;
        m_hints |= PLUGIN_IS_BRIDGE;
    }

    bool init(const char* filename, const char* label, void* extra_stuff)
    {
        m_filename = strdup(filename);
        m_name = get_unique_name("TODO");
        Q_UNUSED(label);
        Q_UNUSED(extra_stuff);
        return true;
    }

//private:
    //CarlaPluginThread m_thread;
};

short add_plugin_bridge(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_bridge(%i, %i, %s, %s, %p)", btype, ptype, filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        BridgePlugin* plugin = new BridgePlugin(ptype);

        if (plugin->init(filename, label, extra_stuff))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

            //osc_new_plugin(plugin);
        }
        else
        {
            delete plugin;
            id = -1;
        }
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

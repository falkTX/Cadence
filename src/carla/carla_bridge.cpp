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

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(BinaryType btype, PluginType ptype) : CarlaPlugin(),
        m_binary(btype)
    {
        qDebug("BridgePlugin::BridgePlugin()");
        m_type   = ptype;
        m_hints  = PLUGIN_IS_BRIDGE;
        m_label  = nullptr;

        m_info.category  = PLUGIN_CATEGORY_NONE;
        m_info.hints     = 0;
        m_info.name      = nullptr;
        m_info.maker     = nullptr;
        m_info.unique_id = 0;
        m_info.ains  = 0;
        m_info.aouts = 0;

        m_thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_BRIDGE);
    }

    virtual ~BridgePlugin()
    {
        qDebug("BridgePlugin::~BridgePlugin()");

        if (osc.data.target)
        {
            osc_send_hide(&osc.data);
            osc_send_quit(&osc.data);
        }

        // Wait a bit first, try safe quit else force kill
        if (m_thread->isRunning())
        {
            if (m_thread->wait(2000) == false)
                m_thread->quit();

            if (m_thread->isRunning() && m_thread->wait(1000) == false)
            {
                qWarning("Failed to properly stop Bridge thread");
                m_thread->terminate();
            }
        }

        delete m_thread;

        osc_clear_data(&osc.data);

        if (m_label)
            free((void*)m_label);

        if (m_info.name)
            free((void*)m_info.name);

        if (m_info.maker)
            free((void*)m_info.maker);

        if (m_thread->isRunning())
            m_thread->quit();
    }

#if 0
    virtual PluginCategory category()
    {
        return m_info.category;
    }

    virtual long unique_id()
    {
        return m_info.unique_id;
    }

    virtual uint32_t ain_count()
    {
        return m_info.ains;
    }

    virtual uint32_t aout_count()
    {
        return m_info.aouts;
    }

    virtual uint32_t min_count()
    {
        return m_info.mins;
    }

    virtual uint32_t mout_count()
    {
        return m_info.mouts;
    }

    virtual void get_label(char* buf_str)
    {
        strncpy(buf_str, m_label, STR_MAX);
    }

    virtual void get_maker(char* buf_str)
    {
        strncpy(buf_str, m_info.maker, STR_MAX);
    }

    virtual void get_copyright(char* buf_str)
    {
        strncpy(buf_str, m_info.maker, STR_MAX);
    }

    virtual void get_real_name(char* buf_str)
    {
        strncpy(buf_str, m_info.name, STR_MAX);
    }

    virtual void reload()
    {
        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (m_info.aouts > 0 && (m_info.ains == m_info.aouts || m_info.ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (m_info.aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (m_info.aouts >= 2 && m_info.aouts % 2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        m_hints |= m_info.hints;
    }
#endif

    bool init(const char* filename, const char* label, void* extra_stuff)
    {
        if (extra_stuff == nullptr)
        {
            set_last_error("Invalid bridge info, cannot continue");
            return false;
        }

        set_last_error("Valid bridge info");

        PluginBridgeInfo* info = (PluginBridgeInfo*)extra_stuff;
        m_info.category  = info->category;
        m_info.hints     = info->hints;
        m_info.unique_id = info->unique_id;
        m_info.ains  = info->ains;
        m_info.aouts = info->aouts;

        m_info.name  = strdup(info->name);
        m_info.maker = strdup(info->maker);

        m_label = strdup(label);
        m_name  = get_unique_name(info->name);
        m_filename = strdup(filename);

        // FIXME - verify if path exists, if not return false
        m_thread->setOscData(binarytype2str(m_binary), label, plugintype2str(m_type));
        m_thread->start();

        return true;
    }

private:
    const char* m_label;
    const BinaryType m_binary;
    PluginBridgeInfo m_info;
    CarlaPluginThread* m_thread;
};

short add_plugin_bridge(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_bridge(%i, %i, %s, %s, %p)", btype, ptype, filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        BridgePlugin* plugin = new BridgePlugin(btype, ptype);

        if (plugin->init(filename, label, extra_stuff))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
            plugin->osc_global_register_new();
#endif
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

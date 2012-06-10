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

#ifdef BUILD_BRIDGE
#error Should not use linuxsampler for bridges!
#endif

#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

class LinuxSamplerPlugin : public CarlaPlugin
{
public:
    LinuxSamplerPlugin(unsigned short id, bool isGIG) : CarlaPlugin(id)
    {
        qDebug("LinuxSamplerPlugin::LinuxSamplerPlugin()");
        m_type = isGIG ? PLUGIN_GIG : PLUGIN_SFZ;

        m_label = nullptr;
    }

    ~LinuxSamplerPlugin()
    {
        qDebug("LinuxSamplerPlugin::~LinuxSamplerPlugin()");

        if (m_label)
            free((void*)m_label);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------

    bool init(const char* filename, const char* label)
    {
        m_filename = strdup(filename);
        m_label  = strdup(label);
        m_name   = get_unique_name(label);
        x_client = new CarlaEngineClient(this);

        if (x_client->isOk())
            return true;

        set_last_error("Failed to register plugin client");
        return false;
    }

private:
    const char* m_label;
};

short add_plugin_linuxsampler(const char* filename, const char* label, bool isGIG)
{
    qDebug("add_plugin_linuxsampler(%s, %s, %s)", filename, label, bool2str(isGIG));

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        LinuxSamplerPlugin* plugin = new LinuxSamplerPlugin(id, isGIG);

        if (plugin->init(filename, label))
        {
            plugin->reload();

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

            plugin->osc_register_new();
        }
        else
        {
            delete plugin;
            id = -1;
        }
    }
    else
        set_last_error("Requested file is not a valid SoundFont");

    return id;
}

short add_plugin_gig(const char* filename, const char* label)
{
    qDebug("add_plugin_gig(%s, %s)", filename, label);
    return add_plugin_linuxsampler(filename, label, true);
}

short add_plugin_sfz(const char* filename, const char* label)
{
    qDebug("add_plugin_sfz(%s, %s)", filename, label);
    return add_plugin_linuxsampler(filename, label, false);
}

CARLA_BACKEND_END_NAMESPACE

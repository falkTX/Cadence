/*
 * Carla Plugin bridge code
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

#include "carla_osc.h"
#include "carla_plugin.h"

#include <QtGui/QApplication>

// global check
bool close_now = false;

// plugin specific
short add_plugin_ladspa(const char* filename, const char* label, void* extra_stuff);
short add_plugin_dssi(const char* filename, const char* label, void* extra_stuff);
short add_plugin_vst(const char* filename, const char* label);

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        qWarning("%s :: bad arguments", argv[0]);
        return 1;
    }

    const char* osc_url  = argv[1];
    const char* stype    = argv[2];
    const char* filename = argv[3];
    const char* label    = argv[4];

    short id;
    PluginType itype;

    if (strcmp(stype, "LADSPA") == 0)
        itype = PLUGIN_LADSPA;
    else if (strcmp(stype, "DSSI") == 0)
        itype = PLUGIN_DSSI;
    else if (strcmp(stype, "VST") == 0)
        itype = PLUGIN_VST;
    else
    {
        itype = PLUGIN_NONE;
        qWarning("Invalid plugin type '%s'", stype);
        return 1;
    }

    QApplication app(argc, argv);

    osc_init(label, osc_url);
    set_last_error("no error");

    switch (itype)
    {
    case PLUGIN_LADSPA:
        id = add_plugin_ladspa(filename, label, nullptr);
        break;
    case PLUGIN_DSSI:
        id = add_plugin_dssi(filename, label, nullptr);
        break;
    case PLUGIN_VST:
        id = add_plugin_vst(filename, label);
        break;
    default:
        id = -1;
        break;
    }

    if (id >= 0)
    {
        CarlaPlugin* plugin = CarlaPlugins[0];

        if (plugin && plugin->id() >= 0)
        {
            osc_send_update();

            // FIXME
            plugin->set_active(true, false, false);

            while (close_now == false)
            {
                app.processEvents();

                if (close_now) break;

                if (plugin->ain_count() > 0)
                {
                    osc_send_bridge_ains_peak(1, ains_peak[0]);
                    osc_send_bridge_ains_peak(2, ains_peak[1]);
                }

                if (close_now) break;

                if (plugin->aout_count() > 0)
                {
                    osc_send_bridge_aouts_peak(1, aouts_peak[0]);
                    osc_send_bridge_aouts_peak(2, aouts_peak[1]);
                }

                if (close_now) break;

                carla_msleep(50);
            }

            delete plugin;
        }
    }
    else
    {
        qWarning("Plugin failed to load, error was:\n%s", get_last_error());
        return 1;
    }

    osc_close();

    return 0;
}

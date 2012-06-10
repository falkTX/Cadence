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

#ifndef CARLA_SHARED_H
#define CARLA_SHARED_H

#include "carla_backend.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

const char* bool2str(bool yesno);
const char* plugintype2str(PluginType type);
const char* binarytype2str(BinaryType type);
const char* customdatatype2str(CustomDataType type);

short get_new_plugin_id();
const char* get_unique_name(const char* name);
PluginCategory get_category_from_name(const char* name);
void* get_pointer(quintptr ptr_addr);
void set_last_error(const char* error);
void carla_proc_lock();
void carla_proc_unlock();
void carla_midi_lock();
void carla_midi_unlock();
void callback_action(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3);

// Global variables (shared)
extern const char* unique_names[MAX_PLUGINS];
extern CarlaPlugin* CarlaPlugins[MAX_PLUGINS];

extern volatile double ains_peak[MAX_PLUGINS*2];
extern volatile double aouts_peak[MAX_PLUGINS*2];

#ifndef BUILD_BRIDGE
// Global options
struct carla_options_t {
    int  max_parameters;
    bool global_jack_client;
    bool prefer_ui_bridges;
    bool proccess_hq;
    int  osc_gui_timeout;
    bool use_dssi_chunks;
    const char* bridge_unix32;
    const char* bridge_unix64;
    const char* bridge_win32;
    const char* bridge_win64;
    const char* bridge_lv2gtk2;
    const char* bridge_lv2qt4;
    const char* bridge_lv2x11;
    const char* bridge_vstx11;
};

extern carla_options_t carla_options;
#endif

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_SHARED_H

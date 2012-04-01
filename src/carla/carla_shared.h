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

#ifndef CARLA_SHARED_H
#define CARLA_SHARED_H

#include "carla_backend.h"

class CarlaPlugin;

const char* bool2str(bool yesno);
const char* plugintype2str(PluginType type);

short get_new_plugin_id();
const char* get_unique_name(const char* name);
void* get_pointer(intptr_t ptr_addr);
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

// Global options
extern carla_options_t carla_options;

#endif // CARLA_SHARED_H

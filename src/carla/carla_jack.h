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

#ifndef CARLA_JACK_H
#define CARLA_JACK_H

#include "carla_includes.h"

#include <jack/jack.h>
#include <jack/midiport.h>

class CarlaPlugin;

int carla_jack_bufsize_callback(jack_nframes_t new_buffer_size, void* arg);
int carla_jack_srate_callback(jack_nframes_t new_sample_rate, void* arg);
int carla_jack_process_callback(jack_nframes_t nframes, void* arg);
void carla_jack_shutdown_callback(void* arg);

#ifndef BUILD_BRIDGE
bool carla_jack_init(const char* client_name);
bool carla_jack_close();
#endif
bool carla_jack_register_plugin(CarlaPlugin* plugin, jack_client_t** client);

#endif // CARLA_JACK_H

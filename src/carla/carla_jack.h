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

bool carla_jack_init(const char* client_name);
bool carla_jack_close();
bool carla_jack_register_plugin(CarlaPlugin* plugin, jack_client_t** client);
bool carla_jack_transport_query(jack_position_t** pos);

#endif // CARLA_JACK_H

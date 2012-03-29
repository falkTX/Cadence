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

short add_plugin_sf2(const char* filename, const char* label)
{
    qDebug("add_plugin_sf2(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        set_last_error("Not implemented yet");
        id = -1;
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

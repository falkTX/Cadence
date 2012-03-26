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

// Global JACK client
extern jack_client_t* carla_jack_client;
extern jack_nframes_t carla_buffer_size;
extern jack_nframes_t carla_sample_rate;

int carla_jack_process_callback(jack_nframes_t nframes, void* arg)
{
    if (carla_options.global_jack_client)
    {
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* plugin = CarlaPlugins[i];
            if (plugin && plugin->id() >= 0)
            {
                carla_proc_lock();
                plugin->process(nframes);
                carla_proc_unlock();
            }
        }
    }
    else if (arg)
    {
        CarlaPlugin* plugin = (CarlaPlugin*)arg;
        if (plugin->id() >= 0)
        {
            carla_proc_lock();
            plugin->process(nframes);
            carla_proc_unlock();
        }
    }

    return 0;
}

int carla_jack_bufsize_callback(jack_nframes_t new_buffer_size, void*)
{
    carla_buffer_size = new_buffer_size;

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        CarlaPlugin* plugin = CarlaPlugins[i];
        if (plugin && plugin->id() >= 0)
            plugin->buffer_size_changed(new_buffer_size);
    }

    return 0;
}

int carla_jack_srate_callback(jack_nframes_t new_sample_rate, void*)
{
    carla_sample_rate = new_sample_rate;
    return 0;
}

void carla_jack_shutdown_callback(void*)
{
    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        //CarlaPlugin* plugin = CarlaPlugins[i];
        //if (plugin && plugin->id() == plugin_id)
        //    plugin->jack_client = nullptr;
    }
    carla_jack_client = nullptr;
    callback_action(CALLBACK_QUIT, 0, 0, 0, 0.0f);
}

//void carla_jack_register_plugin(CarlaPlugin* plugin)
//{
    //plugin->jack_client = jack_client_open(plugin->name, JackNullOption, 0);

    //if (plugin->jack_client)
    //    jack_set_process_callback(plugin->jack_client, carla_jack_process_callback, plugin);
//}

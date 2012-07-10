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

#ifndef CARLA_OSC_H
#define CARLA_OSC_H

#include "carla_osc_includes.h"
#include "carla_backend.h"

class CarlaOsc
{
public:
    CarlaOsc(CarlaBackend::CarlaEngine* const engine);
    ~CarlaOsc();

    void init(const char* const name);
    void close();

    const char* getServerPath() const
    {
        return serverPath;
    }

    const CarlaOscData* get__Data() const
    {
        return &__Data;
    }

    bool is__Registered() const
    {
        return bool(__Data.target);
    }

    // -------------------------------------

    int handleMessage(const char* const path, int argc, lo_arg** const argv, const char* const types, lo_message msg);

private:
    const char* serverPath;
    lo_server_thread serverThread;
    CarlaOscData __Data;

    CarlaBackend::CarlaEngine* const engine;

    const char* m_name;
    size_t m_name_len;

    // -------------------------------------

    int handle_register(lo_arg** const argv, lo_address source);
    int handle_unregister();

    int handle_update(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv, lo_address source);
    int handle_configure(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_control(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_midi(CarlaBackend::CarlaPlugin* plugin, lo_arg **argv);
    int handle_exiting(CarlaBackend::CarlaPlugin* plugin);

    int handle_set_active(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_drywet(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_volume(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_balance_left(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_balance_right(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_parameter(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_set_midi_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_note_on(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_note_off(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);

    int handle_lv2_atom_transfer(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_lv2_event_transfer(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);

    int handle_bridge_ains_peak(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
    int handle_bridge_aouts_peak(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv);
};

//void osc_send_lv2_atom_transfer(OSC_SEND_ARGS void*);
//void osc_send_lv2_event_transfer(OSC_SEND_ARGS const char* type, const char* key, const char* value);

#endif // CARLA_OSC_H

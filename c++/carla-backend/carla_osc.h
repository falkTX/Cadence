/*
 * Carla Backend
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#include "carla_backend.h"
#include "carla_osc_includes.h"

#define CARLA_OSC_HANDLE_ARGS1 CarlaPlugin* const plugin
#define CARLA_OSC_HANDLE_ARGS2 CARLA_OSC_HANDLE_ARGS1, const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                   \
    /* check argument count */                                                                                        \
    if (argc != argcToCompare)                                                                                        \
    {                                                                                                                 \
        qCritical("CarlaOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                     \
    }                                                                                                                 \
    if (argc > 0)                                                                                                     \
    {                                                                                                                 \
        /* check for nullness */                                                                                      \
        if (! (types && typesToCompare))                                                                              \
        {                                                                                                             \
            qCritical("CarlaOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                 \
        }                                                                                                             \
        /* check argument types */                                                                                    \
        if (strcmp(types, typesToCompare) != 0)                                                                       \
        {                                                                                                             \
            qCritical("CarlaOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                 \
        }                                                                                                             \
    }

CARLA_BACKEND_START_NAMESPACE

class CarlaOsc
{
public:
    CarlaOsc(CarlaEngine* const engine);
    ~CarlaOsc();

    void init(const char* const name);
    void close();

    // -------------------------------------------------------------------

    bool isControllerRegistered() const
    {
        return bool(m_controllerData.target);
    }

    const CarlaOscData* getControllerData() const
    {
        return &m_controllerData;
    }

    const char* getServerPath() const
    {
        return m_serverPath;
    }

private:
    CarlaEngine* const engine;

    const char* m_serverPath;
    lo_server_thread m_serverThread;
    CarlaOscData m_controllerData;

    const char* m_name;
    size_t m_name_len;

    // -------------------------------------------------------------------

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const user_data)
    {
        CarlaOsc* const _this_ = (CarlaOsc*)user_data;
        return _this_->handleMessage(path, argc, argv, types, msg);
    }

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handle_register(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source);
    int handle_unregister();

    int handle_update(CARLA_OSC_HANDLE_ARGS2, const lo_address source);
    int handle_configure(CARLA_OSC_HANDLE_ARGS2);
    int handle_control(CARLA_OSC_HANDLE_ARGS2);
    int handle_program(CARLA_OSC_HANDLE_ARGS2);
    int handle_midi(CARLA_OSC_HANDLE_ARGS2);
    int handle_exiting(CARLA_OSC_HANDLE_ARGS1);

    int handle_set_active(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_drywet(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_volume(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_balance_left(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_balance_right(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_parameter_value(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_parameter_midi_cc(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_parameter_midi_channel(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_program(CARLA_OSC_HANDLE_ARGS2);
    int handle_set_midi_program(CARLA_OSC_HANDLE_ARGS2);
    int handle_note_on(CARLA_OSC_HANDLE_ARGS2);
    int handle_note_off(CARLA_OSC_HANDLE_ARGS2);

#ifdef WANT_LV2
    int handle_lv2_atom_transfer(CARLA_OSC_HANDLE_ARGS2);
    int handle_lv2_event_transfer(CARLA_OSC_HANDLE_ARGS2);
#endif

    int handle_bridge_set_input_peak_value(CARLA_OSC_HANDLE_ARGS2);
    int handle_bridge_set_output_peak_value(CARLA_OSC_HANDLE_ARGS2);
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OSC_H

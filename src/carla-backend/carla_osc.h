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

//int osc_handle_register(lo_arg** argv, lo_address source);
//int osc_handle_unregister();

//int osc_handle_update(OSC_HANDLE_ARGS lo_arg** argv, lo_address source);
//int osc_handle_exiting(CarlaBackend::CarlaPlugin*);
//int osc_handle_lv2_atom_transfer(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_lv2_event_transfer(OSC_HANDLE_ARGS lo_arg** argv);

//int osc_handle_set_active(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_drywet(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_volume(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_balance_left(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_balance_right(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_parameter(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_program(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_set_midi_program(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_note_on(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_note_off(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_bridge_ains_peak(OSC_HANDLE_ARGS lo_arg** argv);
//int osc_handle_bridge_aouts_peak(OSC_HANDLE_ARGS lo_arg** argv);

//void osc_send_lv2_atom_transfer(OSC_SEND_ARGS void*);
void osc_send_lv2_event_transfer(OSC_SEND_ARGS const char* type, const char* key, const char* value);

//bool osc_global_registered();
//void osc_global_send_add_plugin(int plugin_id, const char* plugin_name);
//void osc_global_send_remove_plugin(int plugin_id);
//void osc_global_send_set_plugin_data(int plugin_id, int type, int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long unique_id);
//void osc_global_send_set_plugin_ports(int plugin_id, int ains, int aouts, int mins, int mouts, int cins, int couts, int ctotals);
//void osc_global_send_set_parameter_value(int plugin_id, int param_id, double value);
//void osc_global_send_set_parameter_data(int plugin_id, int param_id, int ptype, int hints, const char* name, const char* label, double current);
//void osc_global_send_set_parameter_ranges(int plugin_id, int param_id, double x_min, double x_max, double x_def, double x_step, double x_step_small, double x_step_large);
//void osc_global_send_set_parameter_midi_channel(int plugin_id, int parameter_id, int midi_channel);
//void osc_global_send_set_parameter_midi_cc(int plugin_id, int parameter_id, int midi_cc);
//void osc_global_send_set_default_value(int plugin_id, int param_id, double value);
//void osc_global_send_set_input_peak_value(int plugin_id, int port_id, double value);
//void osc_global_send_set_output_peak_value(int plugin_id, int port_id, double value);
//void osc_global_send_set_program(int plugin_id, int program_id);
//void osc_global_send_set_program_count(int plugin_id, int program_count);
//void osc_global_send_set_program_name(int plugin_id, int program_id, const char* program_name);
//void osc_global_send_set_midi_program(int plugin_id, int midi_program_id);
//void osc_global_send_set_midi_program_count(int plugin_id, int midi_program_count);
//void osc_global_send_set_midi_program_data(int plugin_id, int midi_program_id, int bank_id, int program_id, const char* midi_program_name);
//void osc_global_send_note_on(int plugin_id, int note, int velo);
//void osc_global_send_note_off(int plugin_id, int note);
//void osc_global_send_exit();

CARLA_BACKEND_START_NAMESPACE

class CarlaOsc
{
public:
    CarlaOsc(CarlaEngine* const engine);
    ~CarlaOsc();

    void init(const char* const);
    void close();

    int handleMessage(const char* path, const char* types, lo_arg** argv, int argc, void* data);

    const OscData* oscData() const;

private:
    OscData serverData;
    const char* serverPath;
    lo_server_thread serverThread;

    const CarlaEngine* const engine;
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OSC_H

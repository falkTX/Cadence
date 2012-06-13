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

int osc_handle_register(lo_arg** argv, lo_address source);
int osc_handle_unregister();

int osc_handle_update(CarlaPlugin* plugin, lo_arg** argv, lo_address source);
int osc_handle_exiting(CarlaPlugin* plugin);
int osc_handle_lv2_event_transfer(CarlaPlugin* plugin, lo_arg** argv);

int osc_handle_set_active(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_drywet(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_volume(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_balance_left(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_balance_right(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_parameter(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_set_program(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_note_on(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_note_off(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_bridge_ains_peak(CarlaPlugin* plugin, lo_arg** argv);
int osc_handle_bridge_aouts_peak(CarlaPlugin* plugin, lo_arg** argv);

void osc_send_lv2_event_transfer(OscData* osc_data, const char* type, const char* key, const char* value);

bool osc_global_registered();
void osc_global_send_add_plugin(int plugin_id, const char* plugin_name);
void osc_global_send_remove_plugin(int plugin_id);
void osc_global_send_set_plugin_data(int plugin_id, int type, int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long unique_id);
void osc_global_send_set_plugin_ports(int plugin_id, int ains, int aouts, int mins, int mouts, int cins, int couts, int ctotals);
void osc_global_send_set_parameter_value(int plugin_id, int param_id, double value);
void osc_global_send_set_parameter_data(int plugin_id, int param_id, int ptype, int hints, const char* name, const char* label, double current);
void osc_global_send_set_parameter_ranges(int plugin_id, int param_id, double x_min, double x_max, double x_def, double x_step, double x_step_small, double x_step_large);
void osc_global_send_set_parameter_midi_channel(int plugin_id, int parameter_id, int midi_channel);
void osc_global_send_set_parameter_midi_cc(int plugin_id, int parameter_id, int midi_cc);
void osc_global_send_set_default_value(int plugin_id, int param_id, double value);
void osc_global_send_set_input_peak_value(int plugin_id, int port_id, double value);
void osc_global_send_set_output_peak_value(int plugin_id, int port_id, double value);
void osc_global_send_set_program(int plugin_id, int program_id);
void osc_global_send_set_program_count(int plugin_id, int program_count);
void osc_global_send_set_program_name(int plugin_id, int program_id, const char* program_name);
void osc_global_send_set_midi_program(int plugin_id, int midi_program_id);
void osc_global_send_set_midi_program_count(int plugin_id, int midi_program_count);
void osc_global_send_set_midi_program_data(int plugin_id, int midi_program_id, int bank_id, int program_id, const char* midi_program_name);
void osc_global_send_note_on(int plugin_id, int note, int velo);
void osc_global_send_note_off(int plugin_id, int note);
void osc_global_send_exit();

#endif // CARLA_OSC_H

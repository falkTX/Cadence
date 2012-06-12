/*
 * Carla bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#ifndef CARLA_BRIDGE_OSC_H
#define CARLA_BRIDGE_OSC_H

#include "carla_osc_includes.h"

#ifdef BUILD_BRIDGE_PLUGIN
// plugin-bridge only
void osc_send_bridge_ains_peak(int index, double value);
void osc_send_bridge_aouts_peak(int index, double value);
void osc_send_bridge_audio_count(int ins, int outs, int total);
void osc_send_bridge_midi_count(int ins, int outs, int total);
void osc_send_bridge_param_count(int ins, int outs, int total);
void osc_send_bridge_program_count(int count);
void osc_send_bridge_midi_program_count(int count);
void osc_send_bridge_plugin_info(int type, int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long unique_id);
void osc_send_bridge_param_info(int index, const char* name, const char* unit);
void osc_send_bridge_param_data(int index, int type, int rindex, int hints, int midi_channel, int midi_cc);
void osc_send_bridge_param_ranges(int index, double def, double min, double max, double step, double step_small, double step_large);
void osc_send_bridge_program_info(int index, const char* name);
void osc_send_bridge_midi_program_info(int index, int bank, int program, const char* label);
void osc_send_bridge_update();
#else
// ui-bridge only
void osc_send_update();
void osc_send_exiting();
//void osc_send_lv2_atom_transfer();
void osc_send_lv2_event_transfer(const char* type, const char* key, const char* value);
#endif

#endif // CARLA_BRIDGE_OSC_H

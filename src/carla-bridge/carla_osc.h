/*
 * Carla Plugin bridge code
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

#ifndef CARLA_OSC_H
#define CARLA_OSC_H

#include "carla_includes.h"

#include <lo/lo.h>

struct OscData {
    char* path;
    lo_address source; // unused
    lo_address target;
};

void osc_init(const char* osc_name, const char* osc_url);
void osc_close();
void osc_clear_data(OscData* osc_data);

void osc_error_handler(int num, const char* msg, const char* path);
int  osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data);

// common plugin handlers
int osc_handle_control(lo_arg** argv);
int osc_handle_program(lo_arg** argv);
int osc_handle_midi_program(lo_arg** argv);
int osc_handle_show();
int osc_handle_hide();
int osc_handle_quit();

// bridge only
void osc_send_update();
void osc_send_exiting();
void osc_send_bridge_ains_peak(int index, double value);
void osc_send_bridge_aouts_peak(int index, double value);
void osc_send_bridge_audio_count(int ins, int outs, int total);
void osc_send_bridge_midi_count(int ins, int outs, int total);
void osc_send_bridge_param_count(int ins, int outs, int total);
//void osc_send_bridge_program_count();
//void osc_send_bridge_midi_program_count();
void osc_send_bridge_plugin_info(int type, int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long unique_id);
void osc_send_bridge_param_info(int index, const char* name, const char* unit);
void osc_send_bridge_param_data(int type, int index, int rindex, int hints, int midi_channel, int midi_cc);
void osc_send_bridge_param_ranges(int index, double def, double min, double max, double step, double step_small, double step_large);
//void osc_send_bridge_program_info();
//void osc_send_bridge_midi_program_info();
void osc_send_bridge_update();

// common plugin calls
void osc_send_configure(OscData* osc_data, const char* key, const char* value);
void osc_send_control(OscData* osc_data, int param_id, double value);
void osc_send_program(OscData* osc_data, int program_id);
// TODO - midi

#endif // CARLA_OSC_H

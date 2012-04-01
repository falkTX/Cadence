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

class CarlaPlugin;

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

void osc_send_configure(OscData* osc_data, const char* key, const char* value);
void osc_send_control(OscData* osc_data, int param_id, double value);
void osc_send_program(OscData* osc_data, int program_id);
void osc_send_program_as_midi(OscData* osc_data, int bank, int program);
void osc_send_midi_program(OscData* osc_data, int bank, int program);
void osc_send_show(OscData* osc_data);
void osc_send_hide(OscData* osc_data);
void osc_send_quit(OscData* osc_data);

void osc_send_update();
void osc_send_bridge_ains_peak(int index, double value);
void osc_send_bridge_aouts_peak(int index, double value);

#endif // CARLA_OSC_H

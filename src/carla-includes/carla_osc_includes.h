/*
 * JACK Backend code for Carla
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

#ifndef CARLA_OSC_INCLUDES_H
#define CARLA_OSC_INCLUDES_H

#include "carla_includes.h"

#include <lo/lo.h>

#ifdef BUILD_BRIDGE
#define OSC_SEND_ARGS
#else
#define OSC_SEND_ARGS OscData*,
#endif

struct OscData {
    char* path;
    lo_address source;
    lo_address target;
};

void osc_init(const char*);
void osc_close();
void osc_clear_data(OscData*);

void osc_error_handler(int num, const char* msg, const char* path);
int  osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data);

void osc_send_configure(OSC_SEND_ARGS const char* key, const char* value);
void osc_send_control(OSC_SEND_ARGS int control, double value);
void osc_send_program(OSC_SEND_ARGS int program);
void osc_send_midi_program(OSC_SEND_ARGS int bank, int program, bool);
void osc_send_midi(OSC_SEND_ARGS uint8_t buf[4]);

#ifdef BUILD_BRIDGE_UI
void osc_send_update();
void osc_send_exiting();
#else
void osc_send_show(OscData*);
void osc_send_hide(OscData*);
void osc_send_quit(OscData*);
#endif

#endif // CARLA_OSC_INCLUDES_H

/*
 * Carla common OSC code
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

#include <cstring>
#include <lo/lo.h>

struct OscData {
    char* path;
    lo_address source;
    lo_address target;
};

static inline
void osc_clear_data(OscData* const oscData)
{
    qDebug("osc_clear_data(%p)", oscData);

    if (oscData->path)
        free((void*)oscData->path);

    if (oscData->source)
        lo_address_free(oscData->source);

    if (oscData->target)
        lo_address_free(oscData->target);

    oscData->path = nullptr;
    oscData->source = nullptr;
    oscData->target = nullptr;
}

static inline
void osc_send_configure(const OscData* const osc_data, const char* key, const char* value)
{
    qDebug("osc_send_configure(%s, %s)", key, value);

    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+11];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/configure");
        lo_send(osc_data->target, target_path, "ss", key, value);
    }
}

static inline
void osc_send_control(const OscData* const osc_data, int index, double value)
{
    qDebug("osc_send_control(%i, %f)", index, value);
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+9];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/control");
        lo_send(osc_data->target, target_path, "if", index, value);
    }
}

static inline
void osc_send_program(const OscData* const osc_data, int program_id)
{
    qDebug("osc_send_program(%i)", program_id);
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+9];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/program");
        lo_send(osc_data->target, target_path, "i", program_id);
    }
}

static inline
void osc_send_midi_program(const OscData* const osc_data, int index)
{
    qDebug("osc_send_midi_program(%i)", index);
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+14];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/midi_program");
        lo_send(osc_data->target, target_path, "i", index);
    }
}

static inline
void osc_send_midi(const OscData* const osc_data, uint8_t buf[4])
{
    qDebug("osc_send_midi()");
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+6];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/midi");
        lo_send(osc_data->target, target_path, "m", buf);
    }
}

#ifndef BUILD_BRIDGE
static inline
void osc_send_show(const OscData* const osc_data)
{
    qDebug("osc_send_show()");
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+6];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/show");
        lo_send(osc_data->target, target_path, "");
    }
}

static inline
void osc_send_hide(const OscData* const osc_data)
{
    qDebug("osc_send_hide()");
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+6];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/hide");
        lo_send(osc_data->target, target_path, "");
    }
}

static inline
void osc_send_quit(const OscData* const osc_data)
{
    qDebug("osc_send_quit()");
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+6];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/quit");
        lo_send(osc_data->target, target_path, "");
    }
}
#endif

#endif // CARLA_OSC_INCLUDES_H

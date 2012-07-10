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

#include <cassert>
#include <cstring>
#include <lo/lo.h>

struct CarlaOscData {
    char* path;
    lo_address source;
    lo_address target;
};

static inline
void osc_clear_data(CarlaOscData* const oscData)
{
    qDebug("osc_clear_data(%p, %s)", oscData, oscData->path);

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
void osc_send_configure(const CarlaOscData* const oscData, const char* key, const char* value)
{
    qDebug("osc_send_configure(%s, %s, %s)", oscData->path, key, value);
    assert(key);
    assert(value);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+11];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/configure");
        lo_send(oscData->target, targetPath, "ss", key, value);
    }
}

static inline
void osc_send_control(const CarlaOscData* const oscData, int index, float value)
{
    qDebug("osc_send_control(%s, %i, %f)", oscData->path, index, value);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+9];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/control");
        lo_send(oscData->target, targetPath, "if", index, value);
    }
}

static inline
void osc_send_control(const CarlaOscData* const oscData, int index, double value)
{
    qDebug("osc_send_control(%s, %i, %f)", oscData->path, index, value);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+9];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/control");
        lo_send(oscData->target, targetPath, "id", index, value);
    }
}

static inline
void osc_send_program(const CarlaOscData* const oscData, int index)
{
    qDebug("osc_send_program(%s, %i)", oscData->path, index);
    assert(index >= 0);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+9];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/program");
        lo_send(oscData->target, targetPath, "i", index);
    }
}

static inline
void osc_send_program(const CarlaOscData* const oscData, int program, int bank)
{
    qDebug("osc_send_program(%s, %i, %i)", oscData->path, program, bank);
    assert(program >= 0);
    assert(bank >= 0);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+9];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/program");
        lo_send(oscData->target, targetPath, "ii", program, bank);
    }
}

static inline
void osc_send_midi_program(const CarlaOscData* const oscData, int index)
{
    qDebug("osc_send_midi_program(%s, %i)", oscData->path, index);
    assert(index >= 0);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+14];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/midi_program");
        lo_send(oscData->target, targetPath, "i", index);
    }
}

static inline
void osc_send_midi(const CarlaOscData* const oscData, uint8_t buf[4])
{
    qDebug("osc_send_midi(%s, 0x%X, %03i, %03i)", oscData->path, buf[1], buf[2], buf[3]);
    assert(buf[0] == 0);
    assert(buf[1] != 0);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+6];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/midi");
        lo_send(oscData->target, targetPath, "m", buf);
    }
}

#ifndef BUILD_BRIDGE
static inline
void osc_send_show(const CarlaOscData* const oscData)
{
    qDebug("osc_send_show(%s)", oscData->path);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+6];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/show");
        lo_send(oscData->target, targetPath, "");
    }
}

static inline
void osc_send_hide(const CarlaOscData* const oscData)
{
    qDebug("osc_send_hide(%s)", oscData->path);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+6];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/hide");
        lo_send(oscData->target, targetPath, "");
    }
}

static inline
void osc_send_quit(const CarlaOscData* const oscData)
{
    qDebug("osc_send_quit(%s)", oscData->path);

    if (oscData->target)
    {
        char targetPath[strlen(oscData->path)+6];
        strcpy(targetPath, oscData->path);
        strcat(targetPath, "/quit");
        lo_send(oscData->target, targetPath, "");
    }
}
#endif

#endif // CARLA_OSC_INCLUDES_H

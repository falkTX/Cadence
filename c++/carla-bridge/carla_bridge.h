/*
 * Carla bridge code
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

#ifndef CARLA_BRIDGE_H
#define CARLA_BRIDGE_H

#include "carla_includes.h"

#include <cstdint>

#define CARLA_BRIDGE_START_NAMESPACE namespace CarlaBridge {
#define CARLA_BRIDGE_END_NAMESPACE }

CARLA_BRIDGE_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeAPI Carla Bridge API
 *
 * The Carla Bridge API
 * @{
 */

#define MAX_BRIDGE_MESSAGES 256 //!< Maximum number of messages per client

enum MessageType {
    MESSAGE_NULL = 0,
    MESSAGE_PARAMETER,    // index, 0, value
    MESSAGE_PROGRAM,      // index, 0, 0
    MESSAGE_MIDI_PROGRAM, // bank, program, 0
    MESSAGE_NOTE_ON,      // note, velocity, 0
    MESSAGE_NOTE_OFF,     // note, 0, 0
    MESSAGE_SHOW_GUI,     // show, 0, 0
    MESSAGE_RESIZE_GUI,   // width, height, 0
    MESSAGE_SAVE_NOW,
    MESSAGE_QUIT
};

struct Message {
    MessageType type;
    int32_t value1;
    int32_t value2;
    double  value3;

    Message()
        : type(MESSAGE_NULL),
          value1(0),
          value2(0),
          value3(0.0) {}
};

/**@}*/

class CarlaClient;
class CarlaToolkit;

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_H

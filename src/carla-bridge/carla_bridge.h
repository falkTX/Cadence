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
    MESSAGE_NULL         = 0,
    MESSAGE_PARAMETER    = 1, // index, 0, value
    MESSAGE_PROGRAM      = 2, // index, 0, 0
    MESSAGE_MIDI_PROGRAM = 3, // bank, program, 0
    MESSAGE_NOTE_ON      = 4, // note, velocity, 0
    MESSAGE_NOTE_OFF     = 5, // note, 0, 0
    MESSAGE_SHOW_GUI     = 6, // show, 0, 0
    MESSAGE_RESIZE_GUI   = 7, // width, height, 0
    MESSAGE_SAVE_NOW     = 8,
    MESSAGE_QUIT         = 9
};

struct Message {
    MessageType type;
    int value1;
    int value2;
    double value3;

    Message()
        : type(MESSAGE_NULL),
          value1(0),
          value2(0),
          value3(0.0) {}
};

/**@}*/

class CarlaBridgeClient;
class CarlaBridgeToolkit;

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_H

/*
 * Carla common FluidSynth code
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_FLUIDSYNTH_INCLUDES_H
#define CARLA_FLUIDSYNTH_INCLUDES_H

#include <fluidsynth.h>

#if (FLUIDSYNTH_VERSION_MAJOR >= 1 && FLUIDSYNTH_VERSION_MINOR >= 1 && FLUIDSYNTH_VERSION_MICRO >= 4)
#  define FLUIDSYNTH_VERSION_NEW_API
#endif

#endif // CARLA_FLUIDSYNTH_INCLUDES_H

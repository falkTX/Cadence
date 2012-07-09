/*
 * Carla common includes
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

#ifndef CARLA_INCLUDES_H
#define CARLA_INCLUDES_H

#ifdef __WINE__
#define __socklen_t_defined
#define __WINE_WINSOCK2__
#define HRESULT LONG
#define Q_CORE_EXPORT
#define Q_GUI_EXPORT
#define QT_NO_STL
#endif

#if defined (__GXX_EXPERIMENTAL_CXX0X__) && defined (__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
// nullptr is available
#else
#  define nullptr (0)
#endif

#include <QtCore/Qt>

#ifdef Q_OS_WIN
#  include <winsock2.h>
#  include <windows.h>
#  define carla_sleep(t)  Sleep(t * 1000)
#  define carla_msleep(t) Sleep(t)
#  define carla_usleep(t) Sleep(t / 1000)
#  define carla_setenv(key, value) SetEnvironmentVariableA(key, value)
#else
#  include <dlfcn.h>
#  include <unistd.h>
#  define carla_sleep(t)  sleep(t)
#  define carla_msleep(t) usleep(t * 1000)
#  define carla_usleep(t) usleep(t)
#  define carla_setenv(key, value) setenv(key, value, 1)
#  ifndef __cdecl
#    define __cdecl
#  endif
#endif

// needed for qDebug/Warning/Critical sections
#if __WORDSIZE == 64
#  define P_INTPTR  "%li"
#  define P_UINTPTR "%llx"
#  define P_SIZE    "%lu"
#else
#  define P_INTPTR  "%i"
#  define P_UINTPTR "%x"
#  define P_SIZE    "%u"
#endif

// set native binary type
#if defined(Q_OS_UNIX)
#  if __LP64__
#    define BINARY_NATIVE CarlaBackend::BINARY_UNIX64
#  else
#    define BINARY_NATIVE CarlaBackend::BINARY_UNIX32
#  endif
#elif defined(Q_OS_WIN)
#  ifdef Q_OS_WIN64
#    define BINARY_NATIVE CarlaBackend::BINARY_WIN64
#  else
#    define BINARY_NATIVE CarlaBackend::BINARY_WIN32
#   endif
#else
#  warning Unknown binary type
#  define BINARY_NATIVE CarlaBackend::BINARY_NONE
#endif

// export symbols if needed
#ifdef BUILD_BRIDGE
#  define CARLA_EXPORT
#else
#  if defined(Q_OS_WIN) && ! defined(__WINE__)
#    define CARLA_EXPORT extern "C" __declspec (dllexport)
#  else
#    define CARLA_EXPORT extern "C" __attribute__ ((visibility("default")))
#  endif
#endif

static inline
const char* bool2str(bool yesno)
{
    return yesno ? "true" : "false";
}

#endif // CARLA_INCLUDES_H

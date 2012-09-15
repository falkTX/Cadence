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
#  define Q_CORE_EXPORT
#  define Q_GUI_EXPORT
#  define QT_NO_STL
#endif

#include <QtCore/Qt>

// TESTING - remove later
#ifdef QTCREATOR_TEST
#  undef Q_COMPILER_INITIALIZER_LISTS
#endif

#ifndef Q_COMPILER_LAMBDA
#  define nullptr (0)
#endif

#ifdef Q_OS_WIN
#  include <winsock2.h>
#  include <windows.h>
#  define uintptr_t size_t // FIXME
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
#if defined(Q_OS_WIN64) && ! defined(__WINE__)
#  define P_INTPTR  "%I64i"
#  define P_UINTPTR "%I64x"
#  define P_SIZE    "%I64u"
#elif __WORDSIZE == 64
#  define P_INTPTR  "%li"
#  define P_UINTPTR "%lx"
#  define P_SIZE    "%lu"
#else
#  define P_INTPTR  "%i"
#  define P_UINTPTR "%x"
#  define P_SIZE    "%u"
#endif

// set native binary type
#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
#  ifdef __LP64__
#    define BINARY_NATIVE BINARY_POSIX64
#  else
#    define BINARY_NATIVE BINARY_POSIX32
#  endif
#elif defined(Q_OS_WIN)
#  ifdef Q_OS_WIN64
#    define BINARY_NATIVE BINARY_WIN64
#  else
#    define BINARY_NATIVE BINARY_WIN32
#   endif
#else
#  warning Unknown binary type
#  define BINARY_NATIVE BINARY_OTHER
#endif

// export symbols if needed
#ifdef BUILD_BRIDGE
#  define CARLA_EXPORT extern "C"
#else
#  if defined(Q_OS_WIN) && ! defined(__WINE__)
#    define CARLA_EXPORT extern "C" __declspec (dllexport)
#  else
#    define CARLA_EXPORT extern "C" __attribute__ ((visibility("default")))
#  endif
#endif

// carla_setprocname
#ifdef Q_OS_LINUX
#  include <sys/prctl.h>
#  include <linux/prctl.h>
static inline
void carla_setprocname(const char* const name)
{
    prctl(PR_SET_NAME, name);
}
#else
static inline
void carla_setprocname(const char* const /*name*/)
{
}
#endif

static inline
const char* bool2str(bool yesno)
{
    return yesno ? "true" : "false";
}

static inline
void pass() {}

#endif // CARLA_INCLUDES_H

/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#ifndef __DISTRHO_DEFINES_H__
#define __DISTRHO_DEFINES_H__

#include "DistrhoPluginInfo.h"

#ifndef DISTRHO_PLUGIN_NAME
# error DISTRHO_PLUGIN_NAME undefined!
#endif

#ifndef DISTRHO_PLUGIN_HAS_UI
# error DISTRHO_PLUGIN_HAS_UI undefined!
#endif

#ifndef DISTRHO_PLUGIN_IS_SYNTH
# error DISTRHO_PLUGIN_IS_SYNTH undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_INPUTS
# error DISTRHO_PLUGIN_NUM_INPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_NUM_OUTPUTS
# error DISTRHO_PLUGIN_NUM_OUTPUTS undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_LATENCY
# error DISTRHO_PLUGIN_WANT_LATENCY undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_PROGRAMS
# error DISTRHO_PLUGIN_WANT_PROGRAMS undefined!
#endif

#ifndef DISTRHO_PLUGIN_WANT_STATE
# error DISTRHO_PLUGIN_WANT_STATE undefined!
#endif

#if defined(__WIN32__) || defined(__WIN64__)
# define DISTRHO_PLUGIN_EXPORT extern "C" __declspec (dllexport)
# define DISTRHO_OS_WINDOWS    1
# define DISTRHO_DLL_EXT       "dll"
#else
# define DISTRHO_PLUGIN_EXPORT extern "C" __attribute__ ((visibility("default")))
# if defined(__APPLE__)
#  define DISTRHO_OS_MAC       1
#  define DISTRHO_DLL_EXT      "dylib"
# elif defined(__HAIKU__)
#  define DISTRHO_OS_HAIKU     1
#  define DISTRHO_DLL_EXT      "so"
# elif defined(__linux__)
#  define DISTRHO_OS_LINUX     1
#  define DISTRHO_DLL_EXT      "so"
# else
#  define DISTRHO_DLL_EXT      "so"
# endif
#endif

#ifndef DISTRHO_NO_NAMESPACE
# define START_NAMESPACE_DISTRHO namespace DISTRHO {
# define END_NAMESPACE_DISTRHO }
# define USE_NAMESPACE_DISTRHO using namespace DISTRHO;
#else
# define START_NAMESPACE_DISTRHO
# define END_NAMESPACE_DISTRHO
# define USE_NAMESPACE_DISTRHO
#endif

#ifndef DISTRHO_UI_QT4
# define DISTRHO_UI_OPENGL
#endif

#define DISTRHO_UI_URI DISTRHO_PLUGIN_URI "#UI"

#endif // __DISTRHO_DEFINES_H__

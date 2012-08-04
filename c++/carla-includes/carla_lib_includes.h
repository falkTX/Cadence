/*
 * Carla common library code
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

#ifndef CARLA_LIB_INCLUDES_H
#define CARLA_LIB_INCLUDES_H

#include "carla_includes.h"

#ifdef Q_OS_WIN
#include <cstdio>
#endif

static inline
void* lib_open(const char* const filename)
{
    Q_ASSERT(filename);
#ifdef Q_OS_WIN
    return LoadLibraryA(filename);
#else
    return dlopen(filename, RTLD_NOW|RTLD_LOCAL);
#endif
}

static inline
bool lib_close(void* const lib)
{
    Q_ASSERT(lib);
#ifdef Q_OS_WIN
    return FreeLibrary((HMODULE)lib);
#else
    return (dlclose(lib) == 0);
#endif
}

static inline
void* lib_symbol(void* const lib, const char* const symbol)
{
    Q_ASSERT(lib);
    Q_ASSERT(symbol);
#ifdef Q_OS_WIN
    return (void*)GetProcAddress((HMODULE)lib, symbol);
#else
    return dlsym(lib, symbol);
#endif
}

static inline
const char* lib_error(const char* const filename)
{
    Q_ASSERT(filename);
#ifdef Q_OS_WIN
    static char libError[2048];
    memset(libError, 0, sizeof(char)*2048);

    LPVOID winErrorString;
    DWORD  winErrorCode = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

    snprintf(libError, 2048, "%s: error code %i: %s", filename, winErrorCode, (const char*)winErrorString);
    LocalFree(winErrorString);

    return libError;
#else
    return dlerror();
    Q_UNUSED(filename);
#endif
}

#endif // CARLA_LIB_INCLUDES_H

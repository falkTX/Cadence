/*
 * JackBridge library utils
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef JACKBRIDGE_LIB_UTILS_HPP_INCLUDED
#define JACKBRIDGE_LIB_UTILS_HPP_INCLUDED

#include "JackBridgeDefines.hpp"

#include <cstdio>

#ifndef JACKBRIDGE_OS_WIN
# include <dlfcn.h>
#endif

// -------------------------------------------------
// library related calls

static inline
void* lib_open(const char* const filename)
{
#ifdef JACKBRIDGE_OS_WIN
    return (void*)LoadLibraryA(filename);
#else
    return dlopen(filename, RTLD_NOW|RTLD_LOCAL);
#endif
}

static inline
bool lib_close(void* const lib)
{
    if (lib == nullptr)
        return false;

#ifdef JACKBRIDGE_OS_WIN
    return FreeLibrary((HMODULE)lib);
#else
    return (dlclose(lib) == 0);
#endif
}

static inline
void* lib_symbol(void* const lib, const char* const symbol)
{
    if (lib == nullptr && symbol == nullptr)
        return nullptr;

#ifdef JACKBRIDGE_OS_WIN
    return (void*)GetProcAddress((HMODULE)lib, symbol);
#else
    return dlsym(lib, symbol);
#endif
}

static inline
const char* lib_error(const char* const filename)
{
#ifdef JACKBRIDGE_OS_WIN
    static char libError[2048];

    LPVOID winErrorString;
    DWORD  winErrorCode = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

    std::snprintf(libError, 2048, "%s: error code %li: %s", filename, winErrorCode, (const char*)winErrorString);
    LocalFree(winErrorString);

    return libError;
#else
    return dlerror();

    // unused
    (void)filename;
#endif
}

// -------------------------------------------------

#endif // JACKBRIDGE_LIB_UTILS_HPP_INCLUDED

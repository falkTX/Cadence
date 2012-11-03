/*
 * Carla common utils
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_UTILS_H
#define CARLA_UTILS_H

#include "carla_defines.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(Q_OS_HAIKU)
# include <kernel/OS.h>
#elif defined(Q_OS_LINUX)
# include <sys/prctl.h>
# include <linux/prctl.h>
#endif

// carla_assert*
static inline
void carla_assert(const char* const assertion, const char* const file, const int line)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

static inline
void carla_assert_int(const char* const assertion, const char* const file, const int line, const int value)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

// carla_*sleep
#ifdef Q_OS_WIN
# define carla_sleep(t)  Sleep(t * 1000)
# define carla_msleep(t) Sleep(t)
# define carla_usleep(t) Sleep(t / 1000)
#else
# define carla_sleep(t)  sleep(t)
# define carla_msleep(t) usleep(t * 1000)
# define carla_usleep(t) usleep(t)
#endif

// carla_setenv
#ifdef Q_OS_WIN
# define carla_setenv(key, value) SetEnvironmentVariableA(key, value)
#else
# define carla_setenv(key, value) setenv(key, value, 1)
#endif

// carla_setprocname (not available on all platforms)
static inline
void carla_setprocname(const char* const name)
{
    CARLA_ASSERT(name);

#if defined(Q_OS_HAIKU)
    if ((thread_id this_thread = find_thread(nullptr)) != B_NAME_NOT_FOUND)
        rename_thread(this_thread, name);
#elif defined(Q_OS_LINUX)
    prctl(PR_SET_NAME, name);
#else
    qWarning("carla_setprocname(\"%s\") - unsupported on this platform");
#endif
}

// math functions
static inline
float carla_absF(const float& value)
{
    return (value < 0.0f) ? -value : value;
}

static inline
float carla_minF(const float& x, const float& y)
{
    return (x < y ? x : y);
}

static inline
float carla_maxF(const float& x, const float& y)
{
    return (x > y ? x : y);
}

static inline
void carla_zeroF(float* data, const unsigned int size)
{
    for (unsigned int i=0; i < size; i++)
        *data++ = 0.0f;
}

// other misc functions
static inline
const char* bool2str(const bool yesNo)
{
    return yesNo ? "true" : "false";
}

static inline
void pass() {}

// -------------------------------------------------
// carla_string class

class carla_string
{
public:
    // ---------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit carla_string()
    {
        buffer = ::strdup("");
    }

    explicit carla_string(char* const strBuf)
    {
        buffer = ::strdup(strBuf ? strBuf : "");
    }

    explicit carla_string(const char* const strBuf)
    {
        buffer = ::strdup(strBuf ? strBuf : "");
    }

    explicit carla_string(const int value)
    {
        const size_t strBufSize = ::abs(value/10) + 3;
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, "%d", value);

        buffer = ::strdup(strBuf);
    }

    explicit carla_string(const unsigned int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, hexadecimal ? "%u" : "0x%x", value);

        buffer = ::strdup(strBuf);
    }

    explicit carla_string(const long int value)
    {
        const size_t strBufSize = ::labs(value/10) + 3;
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, "%ld", value);

        buffer = ::strdup(strBuf);
    }

    explicit carla_string(const unsigned long int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        ::snprintf(strBuf, strBufSize, hexadecimal ? "%lu" : "0x%lx", value);

        buffer = ::strdup(strBuf);
    }

    explicit carla_string(const float value)
    {
        char strBuf[0xff];
        ::snprintf(strBuf, 0xff, "%f", value);

        buffer = ::strdup(strBuf);
    }

    explicit carla_string(const double value)
    {
        char strBuf[0xff];
        ::snprintf(strBuf, 0xff, "%g", value);

        buffer = ::strdup(strBuf);
    }

    // ---------------------------------------------
    // non-explicit constructor

    carla_string(const carla_string& str)
    {
        buffer = ::strdup(str.buffer);
    }

    // ---------------------------------------------
    // deconstructor

    ~carla_string()
    {
        ::free(buffer);
    }

    // ---------------------------------------------
    // public methods

    size_t length() const
    {
        return ::strlen(buffer);
    }

    bool isEmpty() const
    {
        return (*buffer == 0);
    }

    // ---------------------------------------------
    // public operators

    operator const char*() const
    {
        return buffer;
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf && ::strcmp(buffer, strBuf) == 0);
    }

    bool operator==(const carla_string& str) const
    {
        return operator==(str.buffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const carla_string& str) const
    {
        return !operator==(str.buffer);
    }

    carla_string& operator=(const char* const strBuf)
    {
        ::free(buffer);

        buffer = ::strdup(strBuf ? strBuf : "");

        return *this;
    }

    carla_string& operator=(const carla_string& str)
    {
        return operator=(str.buffer);
    }

    carla_string& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = ::strlen(buffer) + (strBuf ? ::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        ::strcpy(newBuf, buffer);
        ::strcat(newBuf, strBuf);
        ::free(buffer);

        buffer = ::strdup(newBuf);

        return *this;
    }

    carla_string& operator+=(const carla_string& str)
    {
        return operator+=(str.buffer);
    }

    carla_string operator+(const char* const strBuf)
    {
        const size_t newBufSize = ::strlen(buffer) + (strBuf ? ::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        ::strcpy(newBuf, buffer);
        ::strcat(newBuf, strBuf);

        return carla_string(newBuf);
    }

    carla_string operator+(const carla_string& str)
    {
        return operator+(str.buffer);
    }

    // ---------------------------------------------

private:
    char* buffer;
};

static inline
carla_string operator+(const char* const strBufBefore, const carla_string& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t newBufSize = (strBufBefore ? ::strlen(strBufBefore) : 0) + ::strlen(strBufAfter) + 1;
    char         newBuf[newBufSize];

    ::strcpy(newBuf, strBufBefore);
    ::strcat(newBuf, strBufAfter);

    return carla_string(newBuf);
}

// -------------------------------------------------

#endif // CARLA_UTILS_H

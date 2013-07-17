/*
 * JackBridge common defines
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

#ifndef JACKBRIDGE_DEFINES_HPP_INCLUDED
#define JACKBRIDGE_DEFINES_HPP_INCLUDED

// Check OS
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
# define JACKBRIDGE_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define JACKBRIDGE_OS_WIN32
#elif defined(__APPLE__)
# define JACKBRIDGE_OS_MAC
#elif defined(__HAIKU__)
# define JACKBRIDGE_OS_HAIKU
#elif defined(__linux__) || defined(__linux)
# define JACKBRIDGE_OS_LINUX
#else
# warning Unsupported platform!
#endif

#if defined(JACKBRIDGE_OS_WIN32) || defined(JACKBRIDGE_OS_WIN64)
# define JACKBRIDGE_OS_WIN
#elif ! defined(JACKBRIDGE_OS_HAIKU)
# define JACKBRIDGE_OS_UNIX
#endif

// Check for C++11 support
#if defined(HAVE_CPP11_SUPPORT)
# define JACKBRIDGE_PROPER_CPP11_SUPPORT
#elif defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)
# if  (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#  define JACKBRIDGE_PROPER_CPP11_SUPPORT
#  if  (__GNUC__ * 100 + __GNUC_MINOR__) < 407
#   define override // gcc4.7+ only
#  endif
# endif
#endif

#ifndef JACKBRIDGE_PROPER_CPP11_SUPPORT
# define override
# define noexcept
# define nullptr (0)
#endif

// Common includes
#ifdef JACKBRIDGE_OS_WIN
# include <winsock2.h>
# include <windows.h>
#else
# include <unistd.h>
# ifndef __cdecl
#  define __cdecl
# endif
#endif

// Define JACKBRIDGE_EXPORT
#if defined(JACKBRIDGE_OS_WIN) && ! defined(__WINE__)
# define JACKBRIDGE_EXPORT extern "C" __declspec (dllexport)
#else
# define JACKBRIDGE_EXPORT extern "C" __attribute__ ((visibility("default")))
#endif

#endif // JACKBRIDGE_DEFINES_HPP_INCLUDED

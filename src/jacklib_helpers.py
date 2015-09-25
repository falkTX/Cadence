#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Helper functions for extra jacklib functionality
# Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# ------------------------------------------------------------------------------------------------------------
# Try Import jacklib

try:
    import jacklib
except ImportError:
    jacklib = None

# ------------------------------------------------------------------------------------------------------------
# Get JACK error status as string

def get_jack_status_error_string(cStatus):
    status = cStatus.value

    if status == 0x0:
        return ""

    errors_map = [
        (jacklib.JackFailure, "Overall operation failed"),
        (jacklib.JackInvalidOption, "The operation contained an invalid or unsupported option"),
        (jacklib.JackNameNotUnique, "The desired client name was not unique"),
        (jacklib.JackServerStarted, "The JACK server was started as a result of this operation"),
        (jacklib.JackServerFailed, "Unable to connect to the JACK server"),
        (jacklib.JackServerError, "Communication error with the JACK server"),
        (jacklib.JackNoSuchClient, "Requested client does not exist"),
        (jacklib.JackLoadFailure, "Unable to load internal client"),
        (jacklib.JackInitFailure, "Unable to initialize client"),
        (jacklib.JackShmFailure, "Unable to access shared memory"),
        (jacklib.JackVersionError, "Client's protocol version does not match"),
        (jacklib.JackBackendError, "Backend Error"),
        (jacklib.JackClientZombie, "Client is being shutdown against its will"),
    ]

    errors_list = [
        error_line for error, error_line in errors_map
        if status & error
    ]

    return ';\n'.join(errors_list) + '.'

# ------------------------------------------------------------------------------------------------------------
# Convert C char** -> Python list

def c_char_p_p_to_list(c_char_p_p):
    i = 0
    retList = []

    if not c_char_p_p:
        return retList

    while True:
        new_char_p = c_char_p_p[i]
        if new_char_p:
            retList.append(str(new_char_p, encoding="utf-8"))
            i += 1
        else:
            break

    jacklib.free(c_char_p_p)
    return retList

# ------------------------------------------------------------------------------------------------------------
# Convert C void* -> string

def voidptr2str(void_p):
    char_p = jacklib.cast(void_p, jacklib.c_char_p)
    string = str(char_p.value, encoding="utf-8")
    return string

# ------------------------------------------------------------------------------------------------------------
# Convert C void* -> jack_default_audio_sample_t*

def translate_audio_port_buffer(void_p):
    return jacklib.cast(void_p, jacklib.POINTER(jacklib.jack_default_audio_sample_t))

# ------------------------------------------------------------------------------------------------------------
# Convert a JACK midi buffer into a python variable-size list

def translate_midi_event_buffer(void_p, size):
    if not void_p:
        return ()
    elif size == 1:
        return (void_p[0],)
    elif size == 2:
        return (void_p[0], void_p[1])
    elif size == 3:
        return (void_p[0], void_p[1], void_p[2])
    elif size == 4:
        return (void_p[0], void_p[1], void_p[2], void_p[3])
    else:
        return ()

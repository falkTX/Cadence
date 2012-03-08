#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Helper functions for extra jacklib functionality
# Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

import jacklib

def get_jack_status_error_string(c_status):
  status = c_status.value
  error_string = ""

  if (status & jacklib.JackFailure):
    error_string += "Overall operation failed;\n"
  if (status & jacklib.JackInvalidOption):
    error_string += "The operation contained an invalid or unsupported option;\n"
  if (status & jacklib.JackNameNotUnique):
    error_string += "The desired client name was not unique;\n"
  if (status & jacklib.JackServerStarted):
    error_string += "The JACK server was started as a result of this operation;\n"
  if (status & jacklib.JackServerFailed):
    error_string += "Unable to connect to the JACK server;\n"
  if (status & jacklib.JackServerError):
    error_string += "Communication error with the JACK server;\n"
  if (status & jacklib.JackNoSuchClient):
    error_string += "Requested client does not exist;\n"
  if (status & jacklib.JackLoadFailure):
    error_string += "Unable to load internal client;\n"
  if (status & jacklib.JackInitFailure):
    error_string += "Unable to initialize client;\n"
  if (status & jacklib.JackShmFailure):
    error_string += "Unable to access shared memory;\n"
  if (status & jacklib.JackVersionError):
    error_string += "Client's protocol version does not match;\n"
  if (status & jacklib.JackBackendError):
    error_string += "Backend Error;\n"
  if (status & jacklib.JackClientZombie):
    error_string += "Client is being shutdown against its will;\n"

  if (error_string):
    error_string = error_string.strip().rsplit(";", 1)[0]+"."

  return error_string

# C char** -> Python list conversion
def c_char_p_p_to_list(c_char_p_p):
  i = 0
  final_list = []

  if (not c_char_p_p):
    return final_list

  while (True):
    new_char_p = c_char_p_p[i]
    if (new_char_p):
      final_list.append(str(new_char_p, encoding="ascii"))
      i += 1
    else:
      break

  jacklib.free(c_char_p_p)
  return final_list

# C cast void* -> jack_default_audio_sample_t*
def translate_audio_port_buffer(void_p):
  return jacklib.cast(void_p, jacklib.POINTER(jacklib.jack_default_audio_sample_t))

def translate_midi_event_buffer(void_p, size):
  if (not void_p):
    return list()
  elif (size == 1):
    return (void_p[0],)
  elif (size == 2):
    return (void_p[0], void_p[1])
  elif (size == 3):
    return (void_p[0], void_p[1], void_p[2])
  elif (size == 4):
    return (void_p[0], void_p[1], void_p[2], void_p[3])
  else:
    return list()

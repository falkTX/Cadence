#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Caitlib ctypes definitions for usage in python applications
# Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

# Imports (Global)
from ctypes import *
from sys import platform

# Load Caitlib shared library
try:
    if platform == "darwin":
        caitlib = cdll.LoadLibrary("caitlib.dylib")
    elif platform in ("win32", "win64", "cygwin"):
        caitlib = cdll.LoadLibrary("caitlib.dll")
    else:
        caitlib = cdll.LoadLibrary("caitlib.so")
except:
    caitlib = None

# ---------------------------------------------------------------------------------------------------------------------
# Pre-definitions

c_uchar = c_uint8

# ---------------------------------------------------------------------------------------------------------------------
# Defines

MIDI_EVENT_TYPE_NULL             = 0x00
MIDI_EVENT_TYPE_NOTE_OFF         = 0x80
MIDI_EVENT_TYPE_NOTE_ON          = 0x90
MIDI_EVENT_TYPE_AFTER_TOUCH      = 0xA0
MIDI_EVENT_TYPE_CONTROL          = 0xB0
MIDI_EVENT_TYPE_PROGRAM          = 0xC0
MIDI_EVENT_TYPE_CHANNEL_PRESSURE = 0xD0
MIDI_EVENT_TYPE_PITCH_WHEEL      = 0xE0

# ---------------------------------------------------------------------------------------------------------------------
# Types

CaitlibHandle = c_void_p

# ---------------------------------------------------------------------------------------------------------------------
# Structs

class MidiEventControl(Structure):
    _fields_ = [
        ("controller", c_uint8),
        ("value", c_uint8)
    ]

class MidiEventNote(Structure):
    _fields_ = [
        ("note", c_uint8),
        ("velocity", c_uint8)
    ]

class MidiEventPressure(Structure):
    _fields_ = [
        ("value", c_uint8)
    ]

class MidiEventProgram(Structure):
    _fields_ = [
        ("value", c_uint8)
    ]

class MidiEventPitchWheel(Structure):
    _fields_ = [
        ("value", c_int16)
    ]

class _MidiEventPadding(Structure):
    _fields_ = [
        ("pad", ARRAY(c_uint8, 32))
    ]

class MidiEventData(Union):
    _fields_ = [
        ("control", MidiEventControl),
        ("note", MidiEventNote),
        ("pressure", MidiEventPressure),
        ("program", MidiEventProgram),
        ("pitchwheel", MidiEventPitchWheel),
        ("__padding", _MidiEventPadding),
    ]

class MidiEvent(Structure):
    _fields_ = [
        ("type", c_uint16),
        ("channel", c_uint8),
        ("data", MidiEventData),
        ("time", c_uint32)
    ]

# ---------------------------------------------------------------------------------------------------------------------
# Callbacks

# ---------------------------------------------------------------------------------------------------------------------
# Functions (Initialization)

caitlib.caitlib_init.argtypes = [c_char_p]
caitlib.caitlib_init.restype  = CaitlibHandle

caitlib.caitlib_close.argtypes = [CaitlibHandle]
caitlib.caitlib_close.restype  = None

caitlib.caitlib_create_port.argtypes = [CaitlibHandle, c_char_p]
caitlib.caitlib_create_port.restype  = c_uint32

caitlib.caitlib_destroy_port.argtypes = [CaitlibHandle, c_uint32]
caitlib.caitlib_destroy_port.restype  = None

def init(instanceName):
    return caitlib.caitlib_init(instanceName.encode("utf-8"))

def close(handle):
    caitlib.caitlib_close(handle)

def create_port(handle, portName):
    return caitlib.caitlib_create_port(handle, portName.encode("utf-8"))

def destroy_port(handle, port):
    caitlib.caitlib_destroy_port(handle, port)

# ---------------------------------------------------------------------------------------------------------------------
# Functions (Input)

caitlib.caitlib_want_events.argtypes = [CaitlibHandle, c_bool]
caitlib.caitlib_want_events.restype  = None

caitlib.caitlib_get_event.argtypes = [CaitlibHandle]
caitlib.caitlib_get_event.restype  = POINTER(MidiEvent)

def want_events(handle, yesNo):
    caitlib.caitlib_want_events(handle, yesNo)

def get_event(handle):
    return caitlib.caitlib_get_event(handle)

# ---------------------------------------------------------------------------------------------------------------------
# Functions (Output)

caitlib.caitlib_put_event.argtypes = [CaitlibHandle, c_uint32, POINTER(MidiEvent)]
caitlib.caitlib_put_event.restype  = None

caitlib.caitlib_put_control.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8, c_uint8]
caitlib.caitlib_put_control.restype  = None

caitlib.caitlib_put_note_on.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8, c_uint8]
caitlib.caitlib_put_note_on.restype  = None

caitlib.caitlib_put_note_off.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8, c_uint8]
caitlib.caitlib_put_note_off.restype  = None

caitlib.caitlib_put_aftertouch.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8, c_uint8]
caitlib.caitlib_put_aftertouch.restype  = None

caitlib.caitlib_put_channel_pressure.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8]
caitlib.caitlib_put_channel_pressure.restype  = None

caitlib.caitlib_put_program.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint8]
caitlib.caitlib_put_program.restype  = None

caitlib.caitlib_put_pitchwheel.argtypes = [CaitlibHandle, c_uint32, c_uint32, c_uint8, c_uint16]
caitlib.caitlib_put_pitchwheel.restype  = None

def put_event(handle, port, event):
    caitlib.caitlib_put_event(handle, port, event)

def put_control(handle, port, time, channel, controller, value):
    caitlib.caitlib_put_control(handle, port, time, channel, controller, value)

def put_note_on(handle, port, time, channel, note, velocity):
    caitlib.caitlib_put_note_on(handle, port, time, channel, note, velocity)

def put_note_off(handle, port, time, channel, note, velocity):
    caitlib.caitlib_put_note_off(handle, port, time, channel, note, velocity)

def put_aftertouch(handle, port, time, channel, note, pressure):
    caitlib.caitlib_put_aftertouch(handle, port, time, channel, note, pressure)

def put_channel_pressure(handle, port, time, channel, pressure):
    caitlib.caitlib_put_channel_pressure(handle, port, time, channel, pressure)

def put_program(handle, port, time, channel, program):
    caitlib.caitlib_put_program(handle, port, time, channel, program)

def put_pitchwheel(handle, port, time, channel, value):
    caitlib.caitlib_put_pitchwheel(handle, port, time, channel, value)

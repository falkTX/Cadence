#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Helper functions for extra caitlib functionality
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

import caitlib

# ------------------------------------------------------------------------------
# Instances

# Caitlib instance
class CailibInstance(object):
    def __init__(self, instanceName):
        object.__init__(self)

        try:
            self.m_lib = caitlib.init(instanceName)
        except:
            self.m_lib = None

    def isOk(self):
        return bool(self.m_lib)

    def close(self):
        caitlib.close(self.m_lib)
        self.m_lib = None

    # ---------------------------------

    def createPort(self, portName):
        return CailibPort(self.m_lib, portName)

    # ---------------------------------

    def wantEvents(self, yesNo):
        caitlib.want_events(self.m_lib, yesNo)

    def getEvent(self):
        return caitlib.get_event(self.m_lib)

# Caitlib port
class CailibPort(object):
    def __init__(self, instance, portName):
        object.__init__(self)

        self.m_lib  = instance
        self.m_port = caitlib.create_port(self.m_lib, portName)

    def isOk(self):
        return bool(self.m_port >= 0)

    def destroy(self):
        caitlib.destroy_port(self.m_lib, self.m_port)
        self.m_port = -1

    # ---------------------------------

    def addControl(self, time, channel, controller, value):
        return ControlEvent(self, time, channel, controller, value)

    def addNote(self, time, channel, pitch, velocity, duration):
        return NoteEvent(self, time, channel, pitch, velocity, duration)

    def addChannelPressure(self, time, channel, value):
        return ChannelPressureEvent(self, time, channel, value)

    def addProgram(self, time, channel, bank, program):
        return ProgramEvent(self, time, channel, bank, program)

    def addPitchWheel(self, time, channel, value):
        return PitchWheelEvent(self, time, channel, value)

# ------------------------------------------------------------------------------
# Events

# Abstract event
class AbstractEvent(object):
    def __init__(self, port, time, channel):
        object.__init__(self)

        if 0 < channel > 15:
            print("AbstractEvent(%i, %i) - invalid channel", time, channel)
            channel = 0

        if time < 0:
            print("AbstractEvent(%i, %i) - invalid time", time, channel)
            time = 0

        # Channel, 0 - 15
        self.m_channel = channel

         # Time in which the event starts, in ticks
        self.m_time = time

        # Store port instance for later
        self.m_port = port

    def channel(self):
        return self.m_channel

    def time(self):
        return self.m_time

    def getRawMidiData(self):
        return ()

# Control event
class ControlEvent(AbstractEvent):
    def __init__(self, port, time, channel, controller, value):
        AbstractEvent.__init__(self, port, time, channel)

        if 1 < controller > 127:
            print("ControlEvent(%i, %i) - invalid controller", controller, value)
            controller = 0

        if 0 < value > 127:
            print("ControlEvent(%i, %i) - invalid value", controller, value)
            value = 0

        # Controller
        self.m_controller = controller

        # Value
        self.m_value = value

        # Put event in caitlib
        caitlib.put_control(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, self.m_controller, self.m_value)

    def controller(self):
        return self.m_controller

    def value(self):
        return self.m_value

    def getRawMidiData(self):
        event = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_CONTROL + self.m_channel, self.m_controller, self.m_value)
        }
        return (event,)

# Note event
class NoteEvent(AbstractEvent):
    def __init__(self, port, time, channel, pitch, velocity, duration):
        AbstractEvent.__init__(self, port, time, channel)

        if duration < 1:
            print("NoteEvent(%i, %i, %i) - invalid duration", pitch, velocity, duration)
            duration = 1

        if 0 < pitch > 127:
            print("NoteEvent(%i, %i, %i) - invalid pitch", pitch, velocity, duration)
            pitch = 0

        if 0 < velocity > 127:
            print("NoteEvent(%i, %i, %i) - invalid velocity", pitch, velocity, duration)
            velocity = 0

        # Duration of note, in ticks
        self.m_duration = duration

        # Pitch
        self.m_pitch = pitch

        # Velocity
        self.m_velocity = velocity

        # Put event in caitlib
        caitlib.put_note_on(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, self.m_pitch, self.m_velocity)
        caitlib.put_note_off(self.m_port.m_lib, self.m_port.m_port, self.m_time+self.m_duration, self.m_channel, self.m_pitch, self.m_velocity)

    def duration(self):
        return self.m_duration

    def pitch(self):
        return self.m_pitch

    def velocity(self):
        return self.m_velocity

    def getRawMidiData(self):
        noteOn  = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_NOTE_ON  + self.m_channel, self.m_pitch, self.m_velocity)
        }
        noteOff = { self.m_time+self.m_duration:
            (caitlib.MIDI_EVENT_TYPE_NOTE_OFF + self.m_channel, self.m_pitch, self.m_velocity)
        }
        return (noteOn, noteOff)

# Channel pressure event
class ChannelPressureEvent(AbstractEvent):
    def __init__(self, port, time, channel, value):
        AbstractEvent.__init__(self, port, time, channel)

        if 0 < value > 127:
            print("ChannelPressureEvent(%i) - invalid value", value)
            value = 100

         # Value
        self.m_value = value

        # Put event in caitlib
        caitlib.put_channel_pressure(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, self.m_value)

    def value(self):
        return self.m_value

    def getRawMidiData(self):
        event = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_CHANNEL_PRESSURE + self.m_channel, self.m_value)
        }
        return (event,)

# Program event
class ProgramEvent(AbstractEvent):
    def __init__(self, port, time, channel, bank, program):
        AbstractEvent.__init__(self, port, time, channel)

        if 0 < bank > 127:
            print("ProgramEvent(%i, %i) - invalid bank", bank, program)
            bank = 0

        if 0 < program > 127:
            print("ProgramEvent(%i, %i) - invalid program", bank, program)
            program = 0

         # Bank
        self.m_bank = bank

         # Program
        self.m_program = program

        # Put event in caitlib
        caitlib.put_control(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, 0, self.m_bank)
        caitlib.put_program(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, self.m_program)

    def bank(self):
        return self.m_bank

    def program(self):
        return self.m_program

    def getRawMidiData(self):
        bank    = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_CONTROL + self.m_channel, 0x00, self.m_bank)
        }
        program = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_PROGRAM + self.m_channel, self.m_program)
        }
        return (bank, program)

# PitchWheel event
class PitchWheelEvent(AbstractEvent):
    def __init__(self, port, time, channel, value):
        AbstractEvent.__init__(self, port, time, channel)

        if -8192 < value > 8192:
            print("PitchWheelEvent(%i) - invalid value", value)
            value = 0

         # Value
        self.m_value = value

        # Put event in caitlib
        caitlib.put_pitchwheel(self.m_port.m_lib, self.m_port.m_port, self.m_time, self.m_channel, self.m_value)

    def value(self):
        return self.m_value

    def getRawMidiData(self):
        value = self.m_value + 8192;
        lsb   = value & 0x7F
        msb   = value >> 7
        event = { self.m_time:
            (caitlib.MIDI_EVENT_TYPE_PITCH_WHEEL + self.m_channel, lsb, msb)
        }
        return (event,)

# ------------------------------------------------------------------------------
# Data Sets

class Part(object):
    def __init__(self):
        object.__init__(self)

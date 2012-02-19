#!/usr/bin/env python
# -*- coding: utf-8 -*-

# JACK ctypes definitions for usage in python applications
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

# Imports (Global)
from ctypes import *
from sys import platform

# Load JACK shared library
try:
  if (platform == "win32" or platform == "win64"):
    jacklib = cdll.LoadLibrary("libjack.dll")
  else:
    jacklib = cdll.LoadLibrary("libjack.so.0")
except:
  jacklib = None

# ---------------------------------------------------------------------------------------------------------------------
# Pre-definitions

c_enum = c_int
c_uchar = c_uint8

class _jack_port(Structure):
  _fields_ = []

class _jack_client(Structure):
  _fields_ = []

class pthread_t(Structure):
  _fields_ = []

# ---------------------------------------------------------------------------------------------------------------------
# Defines

JACK_MAX_FRAMES = 4294967295 #U
JACK_LOAD_INIT_LIMIT = 1024
JACK_DEFAULT_AUDIO_TYPE = "32 bit float mono audio"
JACK_DEFAULT_MIDI_TYPE = "8 bit raw midi"

# ---------------------------------------------------------------------------------------------------------------------
# Types

jack_shmsize_t = c_int32
jack_nframes_t = c_uint32
jack_time_t = c_uint64
jack_intclient_t = c_uint64
jack_port_t = _jack_port
jack_client_t = _jack_client
jack_port_id_t = c_uint32
jack_native_thread_t = pthread_t
jack_options_t = c_enum # JackOptions
jack_status_t = c_enum # JackStatus
jack_latency_callback_mode_t = c_enum # JackLatencyCallbackMode
jack_default_audio_sample_t = c_float
jack_transport_state_t = c_enum # Transport states
jack_unique_t = c_uint64
jack_position_bits_t = c_enum # Optional struct jack_position_t fields.
jack_transport_bits_t = c_enum # Optional struct jack_transport_info_t fields
jack_midi_data_t = c_uchar

# enum JackOptions
JackNullOption    = 0x00
JackNoStartServer = 0x01
JackUseExactName  = 0x02
JackServerName    = 0x04
JackLoadName      = 0x08
JackLoadInit      = 0x10
JackSessionID     = 0x20
JackOpenOptions   = JackSessionID|JackServerName|JackNoStartServer|JackUseExactName
JackLoadOptions   = JackLoadInit|JackLoadName|JackUseExactName

# enum JackStatus
JackFailure       = 0x01
JackInvalidOption = 0x02
JackNameNotUnique = 0x04
JackServerStarted = 0x08
JackServerFailed  = 0x10
JackServerError   = 0x20
JackNoSuchClient  = 0x40
JackLoadFailure   = 0x80
JackInitFailure   = 0x100
JackShmFailure    = 0x200
JackVersionError  = 0x400
JackBackendError  = 0x800
JackClientZombie  = 0x1000

# enum JackLatencyCallbackMode
JackCaptureLatency  = 0
JackPlaybackLatency = 1

# enum JackPortFlags
JackPortIsInput    = 0x1
JackPortIsOutput   = 0x2
JackPortIsPhysical = 0x4
JackPortCanMonitor = 0x8
JackPortIsTerminal = 0x10

# Transport states
JackTransportStopped  = 0
JackTransportRolling  = 1
JackTransportLooping  = 2
JackTransportStarting = 3

# Optional struct jack_position_t fields
JackPositionBBT      = 0x10
JackPositionTimecode = 0x20
JackBBTFrameOffset   = 0x40
JackAudioVideoRatio  = 0x80
JackVideoFrameOffset = 0x100
JACK_POSITION_MASK   = JackPositionBBT|JackPositionTimecode|JackBBTFrameOffset|JackAudioVideoRatio|JackVideoFrameOffset
#EXTENDED_TIME_INFO   = 0

# Optional struct jack_transport_info_t fields
JackTransportState    = 0x1
JackTransportPosition = 0x2
JackTransportLoop     = 0x4
JackTransportSMPTE    = 0x8
JackTransportBBT      = 0x10

# ---------------------------------------------------------------------------------------------------------------------
# Structs

class jack_latency_range_t(Structure):
  _fields_ = [
    ("min", jack_nframes_t),
    ("max", jack_nframes_t)
  ]

class jack_position_t(Structure):
  _fields_ = [
    ("unique_1", jack_unique_t),
    ("usecs", jack_time_t),
    ("frame_rate", jack_nframes_t),
    ("frame", jack_nframes_t),
    ("valid", jack_position_bits_t),
    ("bar", c_int32),
    ("beat", c_int32),
    ("tick", c_int32),
    ("bar_start_tick", c_double),
    ("beats_per_bar", c_float),
    ("beat_type", c_float),
    ("ticks_per_beat", c_double),
    ("beats_per_minute", c_double),
    ("frame_time", c_double),
    ("next_time", c_double),
    ("bbt_offset", jack_nframes_t),
    ("audio_frames_per_video_frame", c_float),
    ("video_offset", jack_nframes_t),
    ("padding", ARRAY(c_int32, 7)),
    ("unique_2", jack_unique_t)
  ]

class jack_transport_info_t(Structure):
  _fields_ = [
    ("frame_rate", jack_nframes_t),
    ("usecs", jack_time_t),
    ("valid", jack_transport_bits_t),
    ("transport_state", jack_transport_state_t),
    ("frame", jack_nframes_t),
    ("loop_start", jack_nframes_t),
    ("loop_end", jack_nframes_t),
    ("smpte_offset", c_long),
    ("smpte_frame_rate", c_float),
    ("bar", c_int),
    ("beat", c_int),
    ("tick", c_int),
    ("bar_start_tick", c_double),
    ("beats_per_bar", c_float),
    ("beat_type", c_float),
    ("ticks_per_beat", c_double),
    ("beats_per_minute", c_double)
  ]

class jack_midi_event_t(Structure):
  _fields_ = [
    ("time", jack_nframes_t),
    ("size", c_size_t),
    ("buffer", POINTER(jack_midi_data_t))
  ]

# ---------------------------------------------------------------------------------------------------------------------
# Functions

def client_open(client_name, options, status):
  jacklib.jack_client_open.argtypes = [c_char_p, jack_options_t, POINTER(jack_status_t)]
  jacklib.jack_client_open.restype = POINTER(jack_client_t)
  return jacklib.jack_client_open(client_name.encode(), options, status)

def client_new(client_name):
  jacklib.jack_client_new.argtypes = [c_char_p]
  jacklib.jack_client_new.restype = POINTER(jack_client_t)
  return jacklib.jack_client_new(client_name.encode())

def client_close(client):
  jacklib.jack_client_close.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_client_close.restype = c_int
  return jacklib.jack_client_close(client)

def client_name_size():
  jacklib.jack_client_name_size.argtypes = None
  jacklib.jack_client_name_size.restype = c_int
  return jacklib.jack_client_name_size()

def get_client_name(client):
  jacklib.jack_get_client_name.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_get_client_name.restype = c_char_p
  return jacklib.jack_get_client_name(client)

def internal_client_new(client_name, load_name, load_init):
  jacklib.jack_internal_client_new.argtypes = [c_char_p, c_char_p, c_char_p]
  jacklib.jack_internal_client_new.restype = c_int
  return jacklib.jack_internal_client_new(client_name.encode(), load_name.encode(), load_init.encode())

def internal_client_close(client_name):
  jacklib.jack_internal_client_close.argtypes = [c_char_p]
  jacklib.jack_internal_client_close.restype = None
  jacklib.jack_internal_client_close(client_name.encode())

def activate(client):
  jacklib.jack_activate.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_activate.restype = c_int
  return jacklib.jack_activate(client)

def deactivate(client):
  jacklib.jack_deactivate.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_deactivate.restype = c_int
  return jacklib.jack_deactivate(client)

def client_thread_id(client):
  jacklib.jack_client_thread_id.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_client_thread_id.restype = jack_native_thread_t
  return jacklib.jack_client_thread_id(client)

def is_realtime(client):
  jacklib.jack_is_realtime.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_is_realtime.restype = c_int
  return jacklib.jack_is_realtime(client)


# Non Callback API

def thread_wait(client, status):
  jacklib.jack_thread_wait.argtypes = [POINTER(jack_client_t), c_int]
  jacklib.jack_thread_wait.restype = jack_nframes_t
  return jacklib.jack_thread_wait(client, status)

def cycle_wait(client):
  jacklib.jack_cycle_wait.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_cycle_wait.restype = jack_nframes_t
  return jacklib.jack_cycle_wait(client)

def cycle_signal(client, status):
  jacklib.jack_cycle_signal.argtypes = [POINTER(jack_client_t), c_int]
  jacklib.jack_cycle_signal.restype = None
  jacklib.jack_cycle_signal(client, status)

#def set_process_thread(client, thread_callback, arg=None):
  #global ThreadCallback
  #ThreadCallback = CFUNCTYPE(c_int, c_void_p)(thread_callback)
  #jacklib.jack_set_process_thread.restype = c_int
  #return jacklib.jack_set_process_thread(client)


# Client Callbacks


# Server Control

def set_freewheel(client, onoff):
  jacklib.jack_set_freewheel.argtypes = [POINTER(jack_client_t), c_int]
  jacklib.jack_set_freewheel.restype = c_int
  return jacklib.jack_set_freewheel(client, onoff)

def set_buffer_size(client, nframes):
  jacklib.jack_set_buffer_size.argtypes = [POINTER(jack_client_t), jack_nframes_t]
  jacklib.jack_set_buffer_size.restype = c_int
  return jacklib.jack_set_buffer_size(client, nframes)

def get_sample_rate(client):
  jacklib.jack_get_sample_rate.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_get_sample_rate.restype = jack_nframes_t
  return jacklib.jack_get_sample_rate(client)

def get_buffer_size(client):
  jacklib.jack_get_buffer_size.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_get_buffer_size.restype = jack_nframes_t
  return jacklib.jack_get_buffer_size(client)

def engine_takeover_timebase(client):
  jacklib.jack_engine_takeover_timebase.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_engine_takeover_timebase.restype = c_int
  return jacklib.jack_engine_takeover_timebase(client)

def cpu_load(client):
  jacklib.jack_cpu_load.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_cpu_load.restype = c_float
  return jacklib.jack_cpu_load(client)


# Port Functions

def port_register(client, port_name, port_type, flags, buffer_size):
  jacklib.jack_port_register.argtypes = [POINTER(jack_client_t), c_char_p, c_char_p, c_ulong, c_ulong]
  jacklib.jack_port_register.restype = POINTER(jack_port_t)
  return jacklib.jack_port_register(client, port_name.encode(), port_type.encode(), flags, buffer_size)

def port_unregister(client, port):
  jacklib.jack_port_unregister.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_port_unregister.restype = c_int
  return jacklib.jack_port_unregister(client, port)

def port_get_buffer(port, nframes):
  jacklib.jack_port_get_buffer.argtypes = [POINTER(jack_port_t), jack_nframes_t]
  jacklib.jack_port_get_buffer.restype = c_void_p
  return jacklib.jack_port_get_buffer(port, nframes)

def port_name(port):
  jacklib.jack_port_name.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_name.restype = c_char_p
  return jacklib.jack_port_name(port)

def port_short_name(port):
  jacklib.jack_port_short_name.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_short_name.restype = c_char_p
  return jacklib.jack_port_short_name(port)

def port_flags(port):
  jacklib.jack_port_flags.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_flags.restype = c_int
  return jacklib.jack_port_flags(port)

def port_type(port):
  jacklib.jack_port_type.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_type.restype = c_char_p
  return jacklib.jack_port_type(port)

def port_is_mine(client, port):
  jacklib.jack_port_is_mine.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_port_is_mine.restype = c_int
  return jacklib.jack_port_is_mine(client, port)

def port_connected(port):
  jacklib.jack_port_connected.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_connected.restype = c_int
  return jacklib.jack_port_connected(port)

def port_connected_to(port, port_name):
  jacklib.jack_port_connected_to.argtypes = [POINTER(jack_port_t), c_char_p]
  jacklib.jack_port_connected_to.restype = c_int
  return jacklib.jack_port_connected_to(port, port_name.encode())

def port_get_connections(port):
  jacklib.jack_port_get_connections.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_get_connections.restype = POINTER(c_char_p)
  return jacklib.jack_port_get_connections(port)

def port_get_all_connections(client, port):
  jacklib.jack_port_get_all_connections.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_port_get_all_connections.restype = POINTER(c_char_p)
  return jacklib.jack_port_get_all_connections(client, port)

def port_tie(src, dst):
  jacklib.jack_port_tie.argtypes = [POINTER(jack_port_t), POINTER(jack_port_t)]
  jacklib.jack_port_tie.restype = c_int
  return jacklib.jack_port_tie(src, dst)

def port_untie(port):
  jacklib.jack_port_untie.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_untie.restype = c_int
  return jacklib.jack_port_untie(port)

def port_set_name(port, port_name):
  jacklib.jack_port_set_name.argtypes = [POINTER(jack_port_t), c_char_p]
  jacklib.jack_port_set_name.restype = c_int
  return jacklib.jack_port_set_name(port, port_name.encode())

def port_set_alias(port, alias):
  jacklib.jack_port_set_alias.argtypes = [POINTER(jack_port_t), c_char_p]
  jacklib.jack_port_set_alias.restype = c_int
  return jacklib.jack_port_set_alias(port, alias.encode())

def port_unset_alias(port, alias):
  jacklib.jack_port_unset_alias.argtypes = [POINTER(jack_port_t), c_char_p]
  jacklib.jack_port_unset_alias.restype = c_int
  return jacklib.jack_port_unset_alias(port, alias.encode())

def port_get_aliases(port):
  # NOTE - this function has no 2nd argument in jacklib
  # Instead, aliases will be passed in return value, in form of (int ret, str alias1, str alias2)
  name_size = port_name_size()
  alias_type = c_char_p*2
  aliases = alias_type(" "*name_size, " "*name_size)

  jacklib.jack_port_get_aliases.argtypes = [POINTER(jack_port_t), POINTER(ARRAY(c_char_p, 2))]
  jacklib.jack_port_get_aliases.restype = c_int

  ret = jacklib.jack_port_get_aliases(port, pointer(aliases))
  return (ret, aliases[0], aliases[1])

def port_request_monitor(port, onoff):
  jacklib.jack_port_request_monitor.argtypes = [POINTER(jack_port_t), c_int]
  jacklib.jack_port_request_monitor.restype = c_int
  return jacklib.jack_port_request_monitor(port, onoff)

def port_request_monitor_by_name(client, port_name, onoff):
  jacklib.jack_port_request_monitor_by_name.argtypes = [POINTER(jack_client_t), c_char_p, c_int]
  jacklib.jack_port_request_monitor_by_name.restype = c_int
  return jacklib.jack_port_request_monitor_by_name(client, port_name.encode(), onoff)

def port_ensure_monitor(port, onoff):
  jacklib.jack_port_ensure_monitor.argtypes = [POINTER(jack_port_t), c_int]
  jacklib.jack_port_ensure_monitor.restype = c_int
  return jacklib.jack_port_ensure_monitor(port, onoff)

def port_monitoring_input(port):
  jacklib.jack_port_monitoring_input.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_monitoring_input.restype = c_int
  return jacklib.jack_port_monitoring_input(port)

def connect(client, source_port, destination_port):
  jacklib.jack_connect.argtypes = [POINTER(jack_client_t), c_char_p, c_char_p]
  jacklib.jack_connect.restype = c_int
  return jacklib.jack_connect(client, source_port.encode(), destination_port.encode())

def disconnect(client, source_port, destination_port):
  jacklib.jack_disconnect.argtypes = [POINTER(jack_client_t), c_char_p, c_char_p]
  jacklib.jack_disconnect.restype = c_int
  return jacklib.jack_disconnect(client, source_port.encode(), destination_port.encode())

def port_disconnect(client, port):
  jacklib.jack_port_disconnect.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_port_disconnect.restype = c_int
  return jacklib.jack_port_disconnect(client, port)

def port_name_size():
  jacklib.jack_port_name_size.argtypes = None
  jacklib.jack_port_name_size.restype = c_int
  return jacklib.jack_port_name_size()

def port_type_size():
  jacklib.jack_port_type_size.argtypes = None
  jacklib.jack_port_type_size.restype = c_int
  return jacklib.jack_port_type_size()

def port_type_get_buffer_size(client, port_type):
  jacklib.jack_port_type_get_buffer_size.argtypes = [POINTER(jack_client_t), c_char_p]
  jacklib.jack_port_type_get_buffer_size.restype = c_size_t
  return jacklib.jack_port_type_get_buffer_size(client, port_type.encode())


# Latency Functions

def port_set_latency(port, nframes):
  jacklib.jack_port_set_latency.argtypes = [POINTER(jack_port_t), jack_nframes_t]
  jacklib.jack_port_set_latency.restype = None
  jacklib.jack_port_set_latency(port, nframes)

def port_get_latency_range(port, mode, range_):
  jacklib.jack_port_get_latency_range.argtypes = [POINTER(jack_port_t), jack_latency_callback_mode_t, POINTER(jack_latency_range_t)]
  jacklib.jack_port_get_latency_range.restype = None
  jacklib.jack_port_get_latency_range(port, mode, range_)

def port_set_latency_range(port, mode, range_):
  jacklib.jack_port_set_latency_range.argtypes = [POINTER(jack_port_t), jack_latency_callback_mode_t, POINTER(jack_latency_range_t)]
  jacklib.jack_port_set_latency_range.restype = None
  jacklib.jack_port_set_latency_range(port, mode, range_)

def recompute_total_latencies():
  jacklib.recompute_total_latencies.argtypes = [POINTER(jack_client_t)]
  jacklib.recompute_total_latencies.restype = c_int
  return jacklib.recompute_total_latencies()

def port_get_latency(port):
  jacklib.jack_port_get_latency.argtypes = [POINTER(jack_port_t)]
  jacklib.jack_port_get_latency.restype = jack_nframes_t
  return jacklib.jack_port_get_latency(port)

def port_get_total_latency(client, port):
  jacklib.jack_port_get_total_latency.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_port_get_total_latency.restype = jack_nframes_t
  return jacklib.jack_port_get_total_latency(client, port)

def recompute_total_latency(client, port):
  jacklib.jack_recompute_total_latency.argtypes = [POINTER(jack_client_t), POINTER(jack_port_t)]
  jacklib.jack_recompute_total_latency.restype = c_int
  return jacklib.jack_recompute_total_latency(client, port)


# Port Searching

def get_ports(client, port_name_pattern, type_name_pattern, flags):
  jacklib.jack_get_ports.argtypes = [POINTER(jack_client_t), c_char_p, c_char_p, c_ulong]
  jacklib.jack_get_ports.restype = POINTER(c_char_p)
  return jacklib.jack_get_ports(client, port_name_pattern.encode(), type_name_pattern.encode(), flags)

def port_by_name(client, port_name):
  jacklib.jack_port_by_name.argtypes = [POINTER(jack_client_t), c_char_p]
  jacklib.jack_port_by_name.restype = POINTER(jack_port_t)
  return jacklib.jack_port_by_name(client, port_name.encode())

def port_by_id(client, port_id):
  jacklib.jack_port_by_id.argtypes = [POINTER(jack_client_t), jack_port_id_t]
  jacklib.jack_port_by_id.restype = POINTER(jack_port_t)
  return jacklib.jack_port_by_id(client, port_id)


# Time Functions

def frames_since_cycle_start(client):
  jacklib.jack_frames_since_cycle_start.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_frames_since_cycle_start.restype = jack_nframes_t
  return jacklib.jack_frames_since_cycle_start(client)

def frame_time(client):
  jacklib.jack_frame_time.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_frame_time.restype = jack_nframes_t
  return jacklib.jack_frame_time(client)

def last_frame_time(client):
  jacklib.jack_last_frame_time.argtypes = [POINTER(jack_client_t)]
  jacklib.jack_last_frame_time.restype = jack_nframes_t
  return jacklib.jack_last_frame_time(client)

def frames_to_time(client, nframes):
  jacklib.jack_frames_to_time.argtypes = [POINTER(jack_client_t), jack_nframes_t]
  jacklib.jack_frames_to_time.restype = jack_time_t
  return jacklib.jack_frames_to_time(client, nframes)

def time_to_frames(client, time):
  jacklib.jack_time_to_frames.argtypes = [POINTER(jack_client_t), jack_time_t]
  jacklib.jack_time_to_frames.restype = jack_nframes_t
  return jacklib.jack_time_to_frames(client, time)

def get_time():
  jacklib.jack_get_time.argtypes = None
  jacklib.jack_get_time.restype = jack_time_t
  return jacklib.jack_get_time()


# Error Output
# TODO


# Misc

def free(ptr):
  jacklib.jack_free.argtypes = [c_void_p]
  jacklib.jack_free.restype = None
  return jacklib.jack_free(ptr)

#int jack_release_timebase (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;
#int jack_set_sync_callback (jack_client_t *client,
#JackSyncCallback sync_callback,
#void *arg) JACK_OPTIONAL_WEAK_EXPORT;
#int jack_set_sync_timeout (jack_client_t *client,
#jack_time_t timeout) JACK_OPTIONAL_WEAK_EXPORT;
#int jack_set_timebase_callback (jack_client_t *client,
#int conditional,
#JackTimebaseCallback timebase_callback,
#void *arg) JACK_OPTIONAL_WEAK_EXPORT;
#int jack_transport_locate (jack_client_t *client,
#jack_nframes_t frame) JACK_OPTIONAL_WEAK_EXPORT;
#jack_transport_state_t jack_transport_query (const jack_client_t *client,
#jack_position_t *pos) JACK_OPTIONAL_WEAK_EXPORT;
#jack_nframes_t jack_get_current_transport_frame (const jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;
#int jack_transport_reposition (jack_client_t *client,
#const jack_position_t *pos) JACK_OPTIONAL_WEAK_EXPORT;
#void jack_transport_start (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;
#void jack_transport_stop (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;
#void jack_get_transport_info (jack_client_t *client,
#jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;
#void jack_set_transport_info (jack_client_t *client,
#jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

#jack_nframes_t
#jack_midi_get_event_count(void* port_buffer) JACK_OPTIONAL_WEAK_EXPORT;
#int
#jack_midi_event_get(jack_midi_event_t *event,
                    #void *port_buffer,
                    #uint32_t event_index) JACK_OPTIONAL_WEAK_EXPORT;
#void
#jack_midi_clear_buffer(void *port_buffer) JACK_OPTIONAL_WEAK_EXPORT;
#size_t
#jack_midi_max_event_size(void* port_buffer) JACK_OPTIONAL_WEAK_EXPORT;
#jack_midi_data_t*
#jack_midi_event_reserve(void *port_buffer,
                        #jack_nframes_t time,
                        #size_t data_size) JACK_OPTIONAL_WEAK_EXPORT;
#int
#jack_midi_event_write(void *port_buffer,
                      #jack_nframes_t time,
                      #const jack_midi_data_t *data,
                      #size_t data_size) JACK_OPTIONAL_WEAK_EXPORT;
#uint32_t
#jack_midi_get_lost_event_count(void *port_buffer) JACK_OPTIONAL_WEAK_EXPORT;

#typedef void (*JackLatencyCallback)(jack_latency_callback_mode_t mode, void *arg);
#typedef int (*JackProcessCallback)(jack_nframes_t nframes, void *arg);
#typedef void (*JackThreadInitCallback)(void *arg);
#typedef int (*JackGraphOrderCallback)(void *arg);
#typedef int (*JackXRunCallback)(void *arg);
#typedef int (*JackBufferSizeCallback)(jack_nframes_t nframes, void *arg);
#typedef int (*JackSampleRateCallback)(jack_nframes_t nframes, void *arg);
#typedef void (*JackPortRegistrationCallback)(jack_port_id_t port, int register, void *arg);
#typedef void (*JackClientRegistrationCallback)(const char* name, int register, void *arg);
#typedef void (*JackPortConnectCallback)(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
#typedef void (*JackFreewheelCallback)(int starting, void *arg);
#typedef void *(*JackThreadCallback)(void* arg);
#typedef void (*JackShutdownCallback)(void *arg);
#typedef void (*JackInfoShutdownCallback)(jack_status_t code, const char* reason, void *arg);
#typedef int (*JackSyncCallback)(jack_transport_state_t state,
#jack_position_t *pos,
#void *arg);
#typedef void (*JackTimebaseCallback)(jack_transport_state_t state,
#jack_nframes_t nframes,
#jack_position_t *pos,
#int new_pos,
#void *arg);













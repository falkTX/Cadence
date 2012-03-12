#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to JACK
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
from PyQt4.QtCore import QTimer
from PyQt4.QtGui import QCursor, QFontMetrics, QMenu

# Imports (Custom Stuff)
import jacksettings, logs, render
from shared import *
from jacklib_helpers import *

# Have JACK2 ?
try:
  jacklib.get_version_string()
  JACK2 = True
except:
  JACK2 = False

# Can Render ?
for iPATH in PATH:
  if (os.path.exists(os.path.join(iPATH, "jack_capture"))):
    canRender = True
    break
else:
  canRender = False

# Variables
TRANSPORT_VIEW_HMS = 0
TRANSPORT_VIEW_BBT = 1
TRANSPORT_VIEW_FRAMES = 2

buffer_sizes = (16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192)
sample_rates = (22050, 32000, 44100, 48000, 88200, 96000, 192000)

# DBus object
class DBusObject(object):
    __slots__ = [
      'loop',
      'bus',
      'a2j',
      'jack',
      'ladish_control',
      'ladish_studio',
      'ladish_room',
      'ladish_graph',
      'ladish_app_iface',
      'ladish_app_daemon',
      'patchbay'
    ]
DBus = DBusObject()

# Jack object
class JackObject(object):
    __slots__ = [
      'client'
    ]
jack = JackObject()

# Init objects
DBus.loop = None
DBus.bus = None
DBus.a2j = None
DBus.jack = None
DBus.ladish_control = None
DBus.ladish_studio = None
DBus.ladish_room = None
DBus.ladish_graph = None
DBus.ladish_app_iface = None
DBus.ladish_app_daemon = None
DBus.patchbay = None

jack.client = None

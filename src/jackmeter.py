#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Simple JACK Audio Meter
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
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QApplication, QWidget

# Imports (Custom Stuff)
from digitalpeakmeter import DigitalPeakMeter
from jacklib_helpers import *
from shared import *

global x_port1, x_port2, need_reconnect
x_port1 = 0.0
x_port2 = 0.0
need_reconnect = False
client = None

def process_callback(nframes, arg):
  global x_port1, x_port2

  p_out1 = translate_audio_port_buffer(jacklib.port_get_buffer(port_1, nframes))
  p_out2 = translate_audio_port_buffer(jacklib.port_get_buffer(port_2, nframes))

  for i in range(nframes):
    if (abs(p_out1[i]) > x_port1):
      x_port1 = abs(p_out1[i])

    if (abs(p_out2[i]) > x_port2):
      x_port2 = abs(p_out2[i])

  return 0

def port_callback(port_a, port_b, connect_yesno, arg):
  global need_reconnect
  need_reconnect = True
  return 0

def reconnect_inputs():
  play_port_1 = jacklib.port_by_name(client, "system:playback_1")
  play_port_2 = jacklib.port_by_name(client, "system:playback_2")
  list_port_1 = c_char_p_p_to_list(jacklib.port_get_all_connections(client, play_port_1))
  list_port_2 = c_char_p_p_to_list(jacklib.port_get_all_connections(client, play_port_2))
  client_name = str(jacklib.get_client_name(client), encoding="ascii")

  for port in list_port_1:
    this_port = jacklib.port_by_name(client, port)
    if not jacklib.port_is_mine(client, this_port):
      jacklib.connect(client, port, "%s:in1" % (client_name))

  for port in list_port_2:
    this_port = jacklib.port_by_name(client, port)
    if not jacklib.port_is_mine(client, this_port):
      jacklib.connect(client, port, "%s:in2" % (client_name))

  global need_reconnect
  need_reconnect = False

class MeterW(DigitalPeakMeter):
    def __init__(self, parent):
        DigitalPeakMeter.__init__(self, parent)

        client_name = str(jacklib.get_client_name(client), encoding="ascii")

        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint);
        self.setWindowTitle(client_name)
        self.setChannels(2)
        self.setOrientation(self.VERTICAL)
        self.setSmoothRelease(1)

        self.displayMeter(1, 0.0)
        self.displayMeter(2, 0.0)

        self.setRefreshRate(25)
        self.m_peakTimerId = self.startTimer(50)

    def timerEvent(self, event):
        if (event.timerId() == self.m_peakTimerId):
          global x_port1, x_port2, need_reconnect
          self.displayMeter(1, x_port1)
          self.displayMeter(2, x_port2)
          x_port1 = 0.0
          x_port2 = 0.0

          if (need_reconnect):
            reconnect_inputs()

        QWidget.timerEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':

    # App initialization
    app = QApplication(sys.argv)

    # JACK initialization
    client = jacklib.client_open("M", jacklib.JackNullOption, None)

    port_1 = jacklib.port_register(client, "in1", jacklib.JACK_DEFAULT_AUDIO_TYPE, jacklib.JackPortIsInput, 0)
    port_2 = jacklib.port_register(client, "in2", jacklib.JACK_DEFAULT_AUDIO_TYPE, jacklib.JackPortIsInput, 0)

    jacklib.set_process_callback(client, process_callback, None)
    jacklib.set_port_connect_callback(client, port_callback, None)
    jacklib.activate(client)

    reconnect_inputs()

    # Show GUI
    gui = MeterW(None)
    gui.resize(70, 600)
    gui.show()

    set_up_signals(gui)

    # App-Loop
    ret = app.exec_()

    jacklib.deactivate(client)
    jacklib.client_close(client)

    sys.exit(ret)

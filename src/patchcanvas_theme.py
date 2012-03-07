#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Patchbay Canvas Themes
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

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QFont, QPen

class Theme(object):

    # enum PortType
    THEME_PORT_SQUARE  = 0
    THEME_PORT_POLYGON = 1

    # enum List
    THEME_MODERN_DARK  = 0
    THEME_CLASSIC_DARK = 1
    THEME_MAX = 2

    def __init__(self, idx):
        super(Theme, self).__init__()

        if (idx == self.THEME_MODERN_DARK):
          # Name this theme
          self.name = "Modern Dark"

          # Canvas
          self.canvas_bg = QColor(0,0,0)

          # Boxes
          self.box_pen = QPen(QColor(76,77,78), 1, Qt.SolidLine)
          self.box_pen_sel = QPen(QColor(206,207,208), 1, Qt.DashLine)
          self.box_bg_1 = QColor(32,34,35)
          self.box_bg_2 = QColor(43,47,48)
          self.box_shadow = QColor(89,89,89,180)

          self.box_text = QPen(QColor(240,240,240), 0)
          self.box_font_name = "Deja Vu Sans"
          self.box_font_size = 8
          self.box_font_state = QFont.Bold

          # Ports
          self.port_audio_jack_pen = QPen(QColor(63,90,126), 1)
          self.port_audio_jack_pen_sel = QPen(QColor(63+30,90+30,126+30), 1)
          self.port_midi_jack_pen = QPen(QColor(159,44,42), 1)
          self.port_midi_jack_pen_sel = QPen(QColor(159+30,44+30,42+30), 1)
          self.port_midi_a2j_pen = QPen(QColor(137,76,43), 1)
          self.port_midi_a2j_pen_sel = QPen(QColor(137+30,76+30,43+30), 1)
          self.port_midi_alsa_pen = QPen(QColor(93,141,46), 1)
          self.port_midi_alsa_pen_sel = QPen(QColor(93+30,141+30,46+30), 1)

          self.port_audio_jack_bg = QColor(35,61,99)
          self.port_audio_jack_bg_sel = QColor(35+50,61+50,99+50)
          self.port_midi_jack_bg = QColor(120,15,16)
          self.port_midi_jack_bg_sel = QColor(120+50,15+50,16+50)
          self.port_midi_a2j_bg = QColor(101,47,16)
          self.port_midi_a2j_bg_sel = QColor(101+50,47+50,16+50)
          self.port_midi_alsa_bg = QColor(64,112,18)
          self.port_midi_alsa_bg_sel = QColor(64+50,112+50,18+50)

          self.port_text = QPen(QColor(250,250,250), 0)
          self.port_font_name = "Deja Vu Sans"
          self.port_font_size = 8
          self.port_font_state = QFont.Normal
          self.port_mode = self.THEME_PORT_POLYGON

          # Lines
          self.line_audio_jack = QColor(63,90,126)
          self.line_audio_jack_sel = QColor(63+90,90+90,126+90)
          self.line_audio_jack_glow = QColor(100,100,200)
          self.line_midi_jack = QColor(159,44,42)
          self.line_midi_jack_sel = QColor(159+90,44+90,42+90)
          self.line_midi_jack_glow = QColor(200,100,100)
          self.line_midi_a2j = QColor(137,76,43)
          self.line_midi_a2j_sel = QColor(137+90,76+90,43+90)
          self.line_midi_a2j_glow = QColor(166,133,133)
          self.line_midi_alsa = QColor(93,141,46)
          self.line_midi_alsa_sel = QColor(93+90,141+90,46+90)
          self.line_midi_alsa_glow = QColor(100,200,100)

          self.rubberband_pen = QPen(QColor(206,207,208), 1, Qt.SolidLine)
          self.rubberband_brush = QColor(76,77,78,100)

        elif (idx == self.THEME_CLASSIC_DARK):
          # Name this theme
          self.name = "Classic Dark"

          # Canvas
          self.canvas_bg = QColor(0,0,0)

          # Boxes
          self.box_pen = QPen(QColor(147-70,151-70,143-70), 2, Qt.SolidLine)
          self.box_pen_sel = QPen(QColor(147,151,143), 2, Qt.DashLine)
          self.box_bg_1 = QColor(30,34,36)
          self.box_bg_2 = QColor(30,34,36)
          self.box_shadow = QColor(89,89,89,180)

          self.box_text = QPen(QColor(255,255,255), 0)
          self.box_font_name = "Sans"
          self.box_font_size = 9
          self.box_font_state = QFont.Normal

          # Ports
          self.port_audio_jack_pen = QPen(QColor(35,61,99), 0)
          self.port_audio_jack_pen_sel = QPen(QColor(255,0,0), 0)
          self.port_midi_jack_pen = QPen(QColor(120,15,16), 0)
          self.port_midi_jack_pen_sel = QPen(QColor(255,0,0), 0)
          self.port_midi_a2j_pen = QPen(QColor(101,47,17), 0)
          self.port_midi_a2j_pen_sel = QPen(QColor(255,0,0), 0)
          self.port_midi_alsa_pen = QPen(QColor(63,112,19), 0)
          self.port_midi_alsa_pen_sel = QPen(QColor(255,0,0), 0)

          self.port_audio_jack_bg = QColor(35,61,99)
          self.port_audio_jack_bg_sel = QColor(255,0,0)
          self.port_midi_jack_bg = QColor(120,15,16)
          self.port_midi_jack_bg_sel = QColor(255,0,0)
          self.port_midi_a2j_bg = QColor(101,47,17)
          self.port_midi_a2j_bg_sel = QColor(255,0,0)
          self.port_midi_alsa_bg = QColor(63,112,19)
          self.port_midi_alsa_bg_sel = QColor(255,0,0)

          self.port_text = QPen(QColor(250,250,250), 0)
          self.port_font_name = "Sans"
          self.port_font_size = 8
          self.port_font_state = QFont.Normal
          self.port_mode = self.THEME_PORT_SQUARE

          # Lines
          self.line_audio_jack = QColor(53,78,116)
          self.line_audio_jack_sel = QColor(255,0,0)
          self.line_audio_jack_glow = QColor(255,0,0)
          self.line_midi_jack = QColor(139,32,32)
          self.line_midi_jack_sel = QColor(255,0,0)
          self.line_midi_jack_glow = QColor(255,0,0)
          self.line_midi_a2j = QColor(120,65,33)
          self.line_midi_a2j_sel = QColor(255,0,0)
          self.line_midi_a2j_glow = QColor(255,0,0)
          self.line_midi_alsa = QColor(81,130,36)
          self.line_midi_alsa_sel = QColor(255,0,0)
          self.line_midi_alsa_glow = QColor(255,0,0)

          self.rubberband_pen = QPen(QColor(147,151,143), 2, Qt.SolidLine)
          self.rubberband_brush = QColor(35,61,99,100)

def getDefaultTheme():
  return Theme.THEME_MODERN_DARK

def getThemeName(idx):
  if (idx == Theme.THEME_MODERN_DARK):
    return "Modern Dark"
  elif (idx == Theme.THEME_CLASSIC_DARK):
    return "Classic Dark"
  else:
    return ""

def getDefaultThemeName():
  return "Modern Dark"

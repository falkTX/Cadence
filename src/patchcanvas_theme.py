#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas Themes
# Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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
# Imports (Global)

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QFont, QFontMetrics, QPen, QPixmap

# ------------------------------------------------------------------------------------------------------------
# patchcanvas-theme.cpp

# Utilities
## hsvAdjusted() - returns color with selectively rewrote HSV parameters
def hsvAdjusted(color, hue = -1, saturation = -1, value = -1, alpha = -1):
    if hue == -1:        hue = color.hue()
    if saturation == -1: saturation = color.saturation()
    if value == -1:      value = color.value()
    if alpha == -1:      alpha = color.alpha()
    return QColor.fromHsv(hue, saturation, value, alpha)

## hsvAdjusted() variant for relative changes
def hsvAdjustedRel(color, hue = 0, saturation = 0, value = 0, alpha = 0):
    return QColor.fromHsv(
      color.hue()+hue,
      color.saturation()+saturation,
      color.value()+value,
      color.alpha()+alpha)

class Theme(object):
    # enum PortType
    THEME_PORT_SQUARE  = 0
    THEME_PORT_POLYGON = 1

    # enum List
    THEME_MODERN_DARK      = 0
    THEME_MODERN_DARK_TINY = 1
    THEME_MODERN_LIGHT     = 2
    THEME_CLASSIC_DARK     = 3
    THEME_NEOCLASSIC_DARK  = 4
    THEME_OOSTUDIO         = 5
    THEME_MAX              = 6

    # enum BackgroundType
    THEME_BG_SOLID    = 0
    THEME_BG_GRADIENT = 1

    def __init__(self, idx):
        object.__init__(self)

        self.idx = idx

        if idx == self.THEME_MODERN_DARK:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt.DashLine)
            self.box_bg_1 = QColor(32, 34, 35)
            self.box_bg_2 = QColor(43, 47, 48)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_hover = QColor(255, 255, 255, 60)
            self.box_header_pixmap  = None
            self.box_header_height  = 24
            self.box_header_spacing = 0
            self.box_rounding = 3.0

            self.box_text = QPen(QColor(240, 240, 240), 0)
            self.box_text_sel  = self.box_text
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 8
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = True

            # Ports
            self.port_text = QPen(QColor(250, 250, 250, 180), 0)
            self.port_hover = QColor(255, 255, 255, 80)
            self.port_bg_pixmap = None
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_POLYGON

            self.port_audio_jack_pen = QPen(QColor(63, 90, 126), 1)
            self.port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1)
            self.port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
            self.port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1)
            self.port_midi_a2j_pen = QPen(QColor(137, 76, 43), 1)
            self.port_midi_a2j_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)
            self.port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
            self.port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50)
            self.port_midi_a2j_bg = QColor(101, 47, 16)
            self.port_midi_a2j_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)
            self.port_midi_alsa_bg = QColor(64, 112, 18)
            self.port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_text_padding = 1
            self.port_offset   = 0
            self.port_spacing  = 3
            self.port_spacingT = 2
            self.port_rounding = 0.0

            # To not scale some line widths
            self.box_pen.setCosmetic(True)
            self.box_pen_sel.setCosmetic(True)
            self.port_audio_jack_pen.setCosmetic(True)
            self.port_audio_jack_pen_sel.setCosmetic(True)
            self.port_midi_jack_pen.setCosmetic(True)
            self.port_midi_jack_pen_sel.setCosmetic(True)
            self.port_midi_a2j_pen.setCosmetic(True)
            self.port_midi_a2j_pen_sel.setCosmetic(True)
            self.port_midi_alsa_pen.setCosmetic(True)
            self.port_midi_alsa_pen_sel.setCosmetic(True)

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 76, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_MODERN_DARK_TINY:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt.DashLine)
            self.box_bg_1 = QColor(32, 34, 35)
            self.box_bg_2 = QColor(43, 47, 48)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_hover = QColor(255, 255, 255, 60)
            self.box_header_pixmap  = None
            self.box_header_height  = 14
            self.box_header_spacing = 0
            self.box_rounding = 2.0

            self.box_text = QPen(QColor(240, 240, 240), 0)
            self.box_text_sel  = self.box_text
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 7
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = False

            # Ports
            self.port_text = QPen(QColor(250, 250, 250, 220), 0)
            self.port_hover = QColor(255, 255, 255, 80)
            self.port_bg_pixmap = None
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 6
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_POLYGON

            self.port_audio_jack_pen = QPen(QColor(63, 90, 126), 1)
            self.port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1)
            self.port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
            self.port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1)
            self.port_midi_a2j_pen = QPen(QColor(137, 76, 43), 1)
            self.port_midi_a2j_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)
            self.port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
            self.port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50)
            self.port_midi_a2j_bg = QColor(101, 47, 16)
            self.port_midi_a2j_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)
            self.port_midi_alsa_bg = QColor(64, 112, 18)
            self.port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_text_padding = 1
            self.port_offset   = 0
            self.port_spacing  = -1
            self.port_spacingT = 2
            self.port_rounding = 0.0

            # To not scale some line widths
            self.box_pen.setCosmetic(True)
            self.box_pen_sel.setCosmetic(True)
            self.port_audio_jack_pen.setCosmetic(True)
            self.port_audio_jack_pen_sel.setCosmetic(True)
            self.port_midi_jack_pen.setCosmetic(True)
            self.port_midi_jack_pen_sel.setCosmetic(True)
            self.port_midi_a2j_pen.setCosmetic(True)
            self.port_midi_a2j_pen_sel.setCosmetic(True)
            self.port_midi_alsa_pen.setCosmetic(True)
            self.port_midi_alsa_pen_sel.setCosmetic(True)

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 76, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_MODERN_LIGHT:
            # Canvas
            self.canvas_bg = QColor(248, 249, 250)

            # Boxes
            self.box_pen = QPen(QColor(0, 0, 0, 60), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(1, 2, 3), 2, Qt.DashLine)
            self.box_bg_1 = QColor(220, 220, 220)
            self.box_bg_2 = self.box_bg_1.darker(120)
            self.box_shadow = QColor(1, 1, 1, 100)
            self.box_hover = QColor(0, 0, 0, 60)
            self.box_header_pixmap  = None
            self.box_header_height  = 24
            self.box_header_spacing = 0
            self.box_rounding = 3.0

            self.box_text = QPen(QColor(1, 1, 1), 0)
            self.box_text_sel  = self.box_text
            self.box_font_name = "Ubuntu"
            self.box_font_size = 10
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = True

            # Ports
            self.port_text = QPen(QColor(255, 255, 255, 220), 1)
            self.port_hover = QColor(0, 0, 0, 255)
            self.port_bg_pixmap = None
            self.port_font_name = "Ubuntu"
            self.port_font_size = 9
            self.port_font_state = QFont.Bold
            self.port_mode = self.THEME_PORT_POLYGON

            # Port colors
            port_audio_jack_color = QColor.fromHsv(240, 214, 181)
            port_midi_jack_color = QColor.fromHsv(0, 214, 130)
            port_midi_a2j_color = QColor.fromHsv(22, 214, 102)
            port_midi_alsa_color = QColor.fromHsv(91, 214, 112)

            port_lineW = 1
            port_pen_shade = 130
            self.port_audio_jack_pen = QPen(port_audio_jack_color.darker(port_pen_shade), port_lineW)
            self.port_midi_jack_pen = QPen(port_midi_jack_color.darker(port_pen_shade), port_lineW)
            self.port_midi_a2j_pen = QPen(port_midi_a2j_color.darker(port_pen_shade), port_lineW)
            self.port_midi_alsa_pen = QPen(port_midi_alsa_color.darker(port_pen_shade), port_lineW)
            port_selW = 1.5
            self.port_audio_jack_pen_sel = QPen(port_audio_jack_color.lighter(port_pen_shade), port_selW)
            self.port_midi_jack_pen_sel = QPen(port_midi_jack_color.lighter(port_pen_shade), port_selW)
            self.port_midi_a2j_pen_sel = QPen(port_midi_a2j_color.lighter(port_pen_shade), port_selW)
            self.port_midi_alsa_pen_sel = QPen(port_midi_alsa_color.lighter(port_pen_shade), port_selW)

            port_bg_shade = 170
            self.port_audio_jack_bg = port_audio_jack_color
            self.port_midi_jack_bg = port_midi_jack_color
            self.port_midi_a2j_bg = port_midi_a2j_color
            self.port_midi_alsa_bg = port_midi_alsa_color
            self.port_audio_jack_bg_sel = hsvAdjustedRel(self.port_audio_jack_bg, saturation = -80).lighter(130)
            self.port_midi_jack_bg_sel = hsvAdjustedRel(self.port_midi_jack_bg, saturation = -80).lighter(130)
            self.port_midi_a2j_bg_sel = hsvAdjustedRel(self.port_midi_a2j_bg, saturation = -80).lighter(130)
            self.port_midi_alsa_bg_sel = hsvAdjustedRel(self.port_midi_alsa_bg, saturation = -80).lighter(130)

            self.port_audio_jack_text = QPen(hsvAdjustedRel(port_audio_jack_color, hue = -30, saturation = -70, value = 70), 1)
            self.port_midi_jack_text = QPen(hsvAdjustedRel(port_midi_jack_color, hue = 10, saturation = -70, value = 70), 1)
            self.port_midi_a2j_text = QPen(hsvAdjustedRel(port_midi_a2j_color, hue = 8, saturation = -70, value = 70), 1)
            self.port_midi_alsa_text = QPen(hsvAdjustedRel(port_midi_alsa_color, hue = -8, saturation = -70, value = 70), 1)
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_text_padding = 0.5
            self.port_offset   = 0
            self.port_spacing  = 2
            self.port_spacingT = 1
            self.port_rounding = 0.0

            # To not scale some line widths
            self.box_pen.setCosmetic(True)
            self.box_pen_sel.setCosmetic(True)
            self.port_audio_jack_pen.setCosmetic(True)
            self.port_audio_jack_pen_sel.setCosmetic(True)
            self.port_midi_jack_pen.setCosmetic(True)
            self.port_midi_jack_pen_sel.setCosmetic(True)
            self.port_midi_a2j_pen.setCosmetic(True)
            self.port_midi_a2j_pen_sel.setCosmetic(True)
            self.port_midi_alsa_pen.setCosmetic(True)
            self.port_midi_alsa_pen_sel.setCosmetic(True)

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 63, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 44, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 43, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(76, 77, 78, 130), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_CLASSIC_DARK:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(143 - 70, 143 - 70, 143 - 70), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(143, 143, 143), 1, Qt.CustomDashLine, Qt.RoundCap)
            self.box_pen_sel.setDashPattern([3, 4])
            self.box_bg_1 = QColor(30, 34, 36)
            self.box_bg_2 = QColor(30, 34, 36)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_hover = QColor(255, 255, 255, 60)
            self.box_header_pixmap  = None
            self.box_header_height  = 19
            self.box_header_spacing = 0
            self.box_rounding = 0.0

            self.box_text = QPen(QColor(255, 255, 255), 0)
            self.box_text_sel  = self.box_text
            self.box_font_name = "Sans"
            self.box_font_size = 9
            self.box_font_state = QFont.Normal

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = False

            # Ports
            self.port_text = QPen(QColor(250, 250, 250, 150), 0)
            self.port_hover = QColor(255, 255, 255, 150)
            self.port_bg_pixmap = None
            self.port_font_name = "Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_SQUARE

            self.port_audio_jack_bg = hsvAdjusted(QColor(41, 61, 99), saturation=120)
            self.port_audio_jack_bg_sel = QColor(255, 0, 0)
            self.port_midi_jack_bg = hsvAdjusted(QColor(120, 15, 16), saturation=150)
            self.port_midi_jack_bg_sel = QColor(255, 0, 0)
            self.port_midi_a2j_bg = hsvAdjusted(QColor(101, 47, 17), saturation=150)
            self.port_midi_a2j_bg_sel = QColor(255, 0, 0)
            self.port_midi_alsa_bg = hsvAdjusted(QColor(63, 112, 19), saturation=120)
            self.port_midi_alsa_bg_sel = QColor(255, 0, 0)

            port_pen = QPen(QColor(255, 255, 255, 50), 1)
            self.port_audio_jack_pen_sel = self.port_audio_jack_pen = QPen(hsvAdjustedRel(self.port_audio_jack_bg.lighter(180), saturation=-60), 1)
            self.port_midi_jack_pen_sel = self.port_midi_jack_pen = QPen(hsvAdjustedRel(self.port_midi_jack_bg.lighter(180), saturation=-60), 1)
            self.port_midi_a2j_pen_sel = self.port_midi_a2j_pen = QPen(hsvAdjustedRel(self.port_midi_a2j_bg.lighter(180), saturation=-60), 1)
            self.port_midi_alsa_pen_sel = self.port_midi_alsa_pen = QPen(hsvAdjustedRel(self.port_midi_alsa_bg.lighter(180), saturation=-60), 1)

            self.port_audio_jack_text = self.port_audio_jack_pen_sel
            self.port_audio_jack_text_sel = self.port_audio_jack_pen_sel
            self.port_midi_jack_text = self.port_midi_jack_pen_sel
            self.port_midi_jack_text_sel = self.port_midi_jack_pen_sel
            self.port_midi_a2j_text = self.port_midi_a2j_pen_sel
            self.port_midi_a2j_text_sel = self.port_midi_a2j_pen_sel
            self.port_midi_alsa_text = self.port_midi_alsa_pen_sel
            self.port_midi_alsa_text_sel = self.port_midi_alsa_pen_sel

            self.port_text_padding = 0
            self.port_offset   = 0
            self.port_spacing  = 1
            self.port_spacingT = 0
            self.port_rounding = 0.0

            # Lines
            self.line_audio_jack = QColor(53, 78, 116)
            self.line_audio_jack_sel = QColor(255, 0, 0)
            self.line_audio_jack_glow = QColor(255, 0, 0)
            self.line_midi_jack = QColor(139, 32, 32)
            self.line_midi_jack_sel = QColor(255, 0, 0)
            self.line_midi_jack_glow = QColor(255, 0, 0)
            self.line_midi_a2j = QColor(120, 65, 33)
            self.line_midi_a2j_sel = QColor(255, 0, 0)
            self.line_midi_a2j_glow = QColor(255, 0, 0)
            self.line_midi_alsa = QColor(81, 130, 36)
            self.line_midi_alsa_sel = QColor(255, 0, 0)
            self.line_midi_alsa_glow = QColor(255, 0, 0)

            self.rubberband_pen = QPen(QColor(147, 151, 143), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(35, 61, 99, 100)

        elif idx == self.THEME_NEOCLASSIC_DARK:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(143 - 70, 143 - 70, 143 - 70), 2, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(143, 143, 143), 2, Qt.CustomDashLine, Qt.RoundCap)
            self.box_pen_sel.setDashPattern([1.5, 3])
            self.box_bg_1 = QColor(30, 34, 36)
            self.box_bg_2 = QColor(30, 34, 36)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_hover = QColor(255, 255, 255, 60)
            self.box_header_pixmap  = None
            self.box_header_height  = 19
            self.box_header_spacing = 0
            self.box_rounding = 4.0

            self.box_text = QPen(QColor(255, 255, 255), 0)
            self.box_text_sel  = self.box_text
            self.box_font_name = "Sans"
            self.box_font_size = 8
            self.box_font_state = QFont.Normal

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = False

            # Ports
            self.port_text = QPen(QColor(250, 250, 250), 0)
            self.port_hover = QColor(255, 255, 255, 80)
            self.port_bg_pixmap = None
            self.port_font_name = "Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_SQUARE

            port_dash = [4, 3]
            self.port_audio_jack_pen = QPen(QColor(55, 91, 149), 1)
            self.port_midi_jack_pen = QPen(QColor(150, 25, 26), 1)
            self.port_midi_a2j_pen = QPen(QColor(141, 67, 27), 1)
            self.port_midi_alsa_pen = QPen(QColor(83, 152, 29), 1)
            self.port_audio_jack_pen_sel = QPen(QColor(55, 91, 149), 1, Qt.CustomDashLine, Qt.RoundCap)
            self.port_midi_jack_pen_sel = QPen(QColor(150, 25, 26), 1, Qt.CustomDashLine, Qt.RoundCap)
            self.port_midi_a2j_pen_sel = QPen(QColor(141, 67, 27), 1, Qt.CustomDashLine, Qt.RoundCap)
            self.port_midi_alsa_pen_sel = QPen(QColor(83, 152, 29), 1, Qt.CustomDashLine, Qt.RoundCap)
            self.port_audio_jack_pen_sel.setDashPattern(port_dash);
            self.port_midi_jack_pen_sel.setDashPattern(port_dash);
            self.port_midi_a2j_pen_sel.setDashPattern(port_dash);
            self.port_midi_alsa_pen_sel.setDashPattern(port_dash);

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_a2j_bg = QColor(101, 47, 17)
            self.port_midi_alsa_bg = QColor(63, 112, 19)
            self.port_audio_jack_bg_sel = self.port_audio_jack_bg.darker(150)
            self.port_midi_jack_bg_sel = self.port_midi_jack_bg.darker(150)
            self.port_midi_a2j_bg_sel = self.port_midi_a2j_bg.darker(150)
            self.port_midi_alsa_bg_sel = self.port_midi_alsa_bg.darker(150)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_text_padding = 1
            self.port_offset   = 0
            self.port_spacing  = 1
            self.port_spacingT = 0
            self.port_rounding = 4.0

            # Lines
            self.line_audio_jack = QColor(53, 78, 116)
            self.line_audio_jack_sel = self.line_audio_jack.lighter(200)
            self.line_audio_jack_glow = self.line_audio_jack.lighter(180)
            self.line_midi_jack = QColor(139, 32, 32)
            self.line_midi_jack_sel = self.line_midi_jack.lighter(200)
            self.line_midi_jack_glow = self.line_midi_jack.lighter(180)
            self.line_midi_a2j = QColor(120, 65, 33)
            self.line_midi_a2j_sel = self.line_midi_a2j.lighter(200)
            self.line_midi_a2j_glow = self.line_midi_a2j.lighter(180)
            self.line_midi_alsa = QColor(81, 130, 36)
            self.line_midi_alsa_sel = self.line_midi_alsa.lighter(200)
            self.line_midi_alsa_glow = self.line_midi_alsa.lighter(180)

            self.rubberband_pen = QPen(QColor.fromHsv(191, 100, 120, 170), 2, Qt.SolidLine)
            self.rubberband_brush = QColor.fromHsv(191, 100, 99, 100)

        elif idx == self.THEME_OOSTUDIO:
            # Canvas
            self.canvas_bg = QColor(11, 11, 11)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(189, 122, 214), 1, Qt.DashLine)
            self.box_bg_1 = QColor(46, 46, 46)
            self.box_bg_2 = QColor(23, 23, 23)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_hover = QColor(255, 255, 255, 60)
            self.box_header_pixmap  = QPixmap(":/bitmaps/canvas/frame_node_header.png")
            self.box_header_height  = 22
            self.box_header_spacing = 6
            self.box_rounding = 3.0

            self.box_text = QPen(QColor(144, 144, 144), 0)
            self.box_text_sel  = QPen(QColor(189, 122, 214), 0)
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 8
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_SOLID
            self.box_use_icon = False

            # Ports
            normalPortBG = QColor(46, 46, 46)
            selPortBG = QColor(23, 23, 23)

            self.port_text = QPen(QColor(155, 155, 155), 0)
            self.port_hover = QColor(255, 255, 255, 80)
            self.port_bg_pixmap = QPixmap(":/bitmaps/canvas/frame_port_bg.png")
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_SQUARE

            self.port_audio_jack_pen = QPen(selPortBG, 2)
            self.port_audio_jack_pen_sel = QPen(QColor(1, 230, 238), 1)
            self.port_midi_jack_pen = QPen(selPortBG, 2)
            self.port_midi_jack_pen_sel = QPen(QColor(252, 118, 118), 1)
            self.port_midi_a2j_pen = QPen(selPortBG, 2)
            self.port_midi_a2j_pen_sel = QPen(QColor(137, 76, 43), 1)
            self.port_midi_alsa_pen = QPen(selPortBG, 2)
            self.port_midi_alsa_pen_sel = QPen(QColor(129, 244, 118), 0)

            self.port_audio_jack_bg = normalPortBG
            self.port_audio_jack_bg_sel = selPortBG
            self.port_midi_jack_bg = normalPortBG
            self.port_midi_jack_bg_sel = selPortBG
            self.port_midi_a2j_bg = normalPortBG
            self.port_midi_a2j_bg_sel = selPortBG
            self.port_midi_alsa_bg = normalPortBG
            self.port_midi_alsa_bg_sel = selPortBG

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_audio_jack_pen_sel
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_midi_jack_pen_sel
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_midi_a2j_pen_sel
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_midi_alsa_pen_sel

            # missing, ports 2
            self.port_text_padding = 2
            self.port_offset   = -1
            self.port_spacing  = 5
            self.port_spacingT = 0
            self.port_rounding = 3.0

            # Lines
            self.line_audio_jack = QColor(64, 64, 64)
            self.line_audio_jack_sel = QColor(1, 230, 238)
            self.line_audio_jack_glow = QColor(100, 200, 100)
            self.line_midi_jack = QColor(64, 64, 64)
            self.line_midi_jack_sel = QColor(252, 118, 118)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(64, 64, 64)
            self.line_midi_a2j_sel = QColor(137+90, 76+90, 43+90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(64, 64, 64)
            self.line_midi_alsa_sel = QColor(129, 244, 118)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(1, 230, 238), 2, Qt.SolidLine)
            self.rubberband_brush = QColor(90, 90, 90, 100)

        # Font-dependant port height
        port_font = QFont(self.port_font_name, self.port_font_size, self.port_font_state)
        self.port_height = QFontMetrics(port_font).height() + 2 * self.port_text_padding


def getDefaultTheme():
    return Theme.THEME_MODERN_DARK

def getThemeName(idx):
    if idx == Theme.THEME_MODERN_DARK:
        return "Modern Dark"
    elif idx == Theme.THEME_MODERN_DARK_TINY:
        return "Modern Dark (Tiny)"
    elif idx == Theme.THEME_MODERN_LIGHT:
        return "Modern Light"
    elif idx == Theme.THEME_CLASSIC_DARK:
        return "Classic Dark"
    elif idx == Theme.THEME_NEOCLASSIC_DARK:
        return "Neoclassic Dark"
    elif idx == Theme.THEME_OOSTUDIO:
        return "OpenOctave Studio"
    else:
        return ""

def getDefaultThemeName():
    return "Modern Dark"

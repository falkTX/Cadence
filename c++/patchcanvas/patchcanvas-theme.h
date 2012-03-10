/* 
 * Patchbay Canvas engine using QGraphicsView/Scene
 * Copyright (C) 2010-2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef PATCHCANVAS_THEME_H
#define PATCHCANVAS_THEME_H

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPen>

#include "patchcanvas-api.h"

START_NAMESPACE_PATCHCANVAS

class Theme
{
public:
    enum PortType {
        THEME_PORT_SQUARE  = 0,
        THEME_PORT_POLYGON = 1
    };

    enum List {
        THEME_MODERN_DARK  = 0,
        THEME_CLASSIC_DARK = 1,
        THEME_MAX = 2
    };

    Theme(List id);

    // Canvas
    QString name;

    // Boxes
    QColor canvas_bg;
    QPen box_pen;
    QPen box_pen_sel;
    QColor box_bg_1;
    QColor box_bg_2;
    QColor box_shadow;
    QPen box_text;
    QString box_font_name;
    int box_font_size;
    QFont::Weight box_font_state;

    // Ports
    QPen port_audio_jack_pen;
    QPen port_audio_jack_pen_sel;
    QPen port_midi_jack_pen;
    QPen port_midi_jack_pen_sel;
    QPen port_midi_a2j_pen;
    QPen port_midi_a2j_pen_sel;
    QPen port_midi_alsa_pen;
    QPen port_midi_alsa_pen_sel;
    QColor port_audio_jack_bg;
    QColor port_audio_jack_bg_sel;
    QColor port_midi_jack_bg;
    QColor port_midi_jack_bg_sel;
    QColor port_midi_a2j_bg;
    QColor port_midi_a2j_bg_sel;
    QColor port_midi_alsa_bg;
    QColor port_midi_alsa_bg_sel;
    QPen port_text;
    QString port_font_name;
    int port_font_size;
    QFont::Weight port_font_state;
    PortType port_mode;

    // Lines
    QColor line_audio_jack;
    QColor line_audio_jack_sel;
    QColor line_audio_jack_glow;
    QColor line_midi_jack;
    QColor line_midi_jack_sel;
    QColor line_midi_jack_glow;
    QColor line_midi_a2j;
    QColor line_midi_a2j_sel;
    QColor line_midi_a2j_glow;
    QColor line_midi_alsa;
    QColor line_midi_alsa_sel;
    QColor line_midi_alsa_glow;
    QPen rubberband_pen;
    QColor rubberband_brush;
};

Theme::List getDefaultTheme();
QString getThemeName(Theme::List id);
QString getDefaultThemeName();

END_NAMESPACE_PATCHCANVAS

#endif // PATCHCANVAS_THEME_H

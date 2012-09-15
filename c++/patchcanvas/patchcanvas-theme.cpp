/*
 * Patchbay Canvas engine using QGraphicsView/Scene
 * Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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

#include "patchcanvas-theme.h"

START_NAMESPACE_PATCHCANVAS

Theme::Theme(List id)
{
    switch (id)
    {
    case THEME_MODERN_DARK:
        // Name this theme
        name = "Modern Dark";

        // Canvas
        canvas_bg = QColor(0, 0, 0);

        // Boxes
        box_pen = QPen(QColor(76,77,78), 1, Qt::SolidLine);
        box_pen_sel = QPen(QColor(206,207,208), 1, Qt::DashLine);
        box_bg_1 = QColor(32,34,35);
        box_bg_2 = QColor(43,47,48);
        box_shadow = QColor(89,89,89,180);

        box_text = QPen(QColor(240,240,240), 0);
        box_font_name = "Deja Vu Sans";
        box_font_size = 8;
        box_font_state = QFont::Bold;

        // Ports
        port_audio_jack_pen = QPen(QColor(63,90,126), 1);
        port_audio_jack_pen_sel = QPen(QColor(63+30,90+30,126+30), 1);
        port_midi_jack_pen = QPen(QColor(159,44,42), 1);
        port_midi_jack_pen_sel = QPen(QColor(159+30,44+30,42+30), 1);
        port_midi_a2j_pen = QPen(QColor(137,76,43), 1);
        port_midi_a2j_pen_sel = QPen(QColor(137+30,76+30,43+30), 1);
        port_midi_alsa_pen = QPen(QColor(93,141,46), 1);
        port_midi_alsa_pen_sel = QPen(QColor(93+30,141+30,46+30), 1);

        port_audio_jack_bg = QColor(35,61,99);
        port_audio_jack_bg_sel = QColor(35+50,61+50,99+50);
        port_midi_jack_bg = QColor(120,15,16);
        port_midi_jack_bg_sel = QColor(120+50,15+50,16+50);
        port_midi_a2j_bg = QColor(101,47,16);
        port_midi_a2j_bg_sel = QColor(101+50,47+50,16+50);
        port_midi_alsa_bg = QColor(64,112,18);
        port_midi_alsa_bg_sel = QColor(64+50,112+50,18+50);

        port_text = QPen(QColor(250,250,250), 0);
        port_font_name = "Deja Vu Sans";
        port_font_size = 8;
        port_font_state = QFont::Normal;
        port_mode = THEME_PORT_POLYGON;

        // Lines
        line_audio_jack = QColor(63,90,126);
        line_audio_jack_sel = QColor(63+90,90+90,126+90);
        line_audio_jack_glow = QColor(100,100,200);
        line_midi_jack = QColor(159,44,42);
        line_midi_jack_sel = QColor(159+90,44+90,42+90);
        line_midi_jack_glow = QColor(200,100,100);
        line_midi_a2j = QColor(137,76,43);
        line_midi_a2j_sel = QColor(137+90,76+90,43+90);
        line_midi_a2j_glow = QColor(166,133,133);
        line_midi_alsa = QColor(93,141,46);
        line_midi_alsa_sel = QColor(93+90,141+90,46+90);
        line_midi_alsa_glow = QColor(100,200,100);

        rubberband_pen = QPen(QColor(206,207,208), 1, Qt::SolidLine);
        rubberband_brush = QColor(76,77,78,100);
        break;

    case THEME_CLASSIC_DARK:
        // Name this theme
        name = "Classic Dark";

        // Canvas
        canvas_bg = QColor(0,0,0);

        // Boxes
        box_pen = QPen(QColor(147-70,151-70,143-70), 2, Qt::SolidLine);
        box_pen_sel = QPen(QColor(147,151,143), 2, Qt::DashLine);
        box_bg_1 = QColor(30,34,36);
        box_bg_2 = QColor(30,34,36);
        box_shadow = QColor(89,89,89,180);

        box_text = QPen(QColor(255,255,255), 0);
        box_font_name = "Sans";
        box_font_size = 9;
        box_font_state = QFont::Normal;

        // Ports
        port_audio_jack_pen = QPen(QColor(35,61,99), 0);
        port_audio_jack_pen_sel = QPen(QColor(255,0,0), 0);
        port_midi_jack_pen = QPen(QColor(120,15,16), 0);
        port_midi_jack_pen_sel = QPen(QColor(255,0,0), 0);
        port_midi_a2j_pen = QPen(QColor(101,47,17), 0);
        port_midi_a2j_pen_sel = QPen(QColor(255,0,0), 0);
        port_midi_alsa_pen = QPen(QColor(63,112,19), 0);
        port_midi_alsa_pen_sel = QPen(QColor(255,0,0), 0);

        port_audio_jack_bg = QColor(35,61,99);
        port_audio_jack_bg_sel = QColor(255,0,0);
        port_midi_jack_bg = QColor(120,15,16);
        port_midi_jack_bg_sel = QColor(255,0,0);
        port_midi_a2j_bg = QColor(101,47,17);
        port_midi_a2j_bg_sel = QColor(255,0,0);
        port_midi_alsa_bg = QColor(63,112,19);
        port_midi_alsa_bg_sel = QColor(255,0,0);

        port_text = QPen(QColor(250,250,250), 0);
        port_font_name = "Sans";
        port_font_size = 8;
        port_font_state = QFont::Normal;
        port_mode = THEME_PORT_SQUARE;

        // Lines
        line_audio_jack = QColor(53,78,116);
        line_audio_jack_sel = QColor(255,0,0);
        line_audio_jack_glow = QColor(255,0,0);
        line_midi_jack = QColor(139,32,32);
        line_midi_jack_sel = QColor(255,0,0);
        line_midi_jack_glow = QColor(255,0,0);
        line_midi_a2j = QColor(120,65,33);
        line_midi_a2j_sel = QColor(255,0,0);
        line_midi_a2j_glow = QColor(255,0,0);
        line_midi_alsa = QColor(81,130,36);
        line_midi_alsa_sel = QColor(255,0,0);
        line_midi_alsa_glow = QColor(255,0,0);

        rubberband_pen = QPen(QColor(147,151,143), 2, Qt::SolidLine);
        rubberband_brush = QColor(35,61,99,100);
        break;

    default:
        break;
    }
}

Theme::List getDefaultTheme()
{
    return Theme::THEME_MODERN_DARK;
}

QString getThemeName(Theme::List id)
{
    switch (id)
    {
    case Theme::THEME_MODERN_DARK:
        return "Modern Dark";
    case Theme::THEME_CLASSIC_DARK:
        return "Classic Dark";
    default:
        return "";
    }
}

QString getDefaultThemeName()
{
    return "Modern Dark";
}

END_NAMESPACE_PATCHCANVAS

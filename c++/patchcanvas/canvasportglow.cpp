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

#include "canvasportglow.h"

START_NAMESPACE_PATCHCANVAS

CanvasPortGlow::CanvasPortGlow(PortType port_type, QObject* parent) :
    QGraphicsDropShadowEffect(parent)
{
    setBlurRadius(12);
    setOffset(0, 0);

    if (port_type == PORT_TYPE_AUDIO_JACK)
      setColor(canvas.theme->line_audio_jack_glow);
    else if (port_type == PORT_TYPE_MIDI_JACK)
      setColor(canvas.theme->line_midi_jack_glow);
    else if (port_type == PORT_TYPE_MIDI_A2J)
      setColor(canvas.theme->line_midi_a2j_glow);
    else if (port_type == PORT_TYPE_MIDI_ALSA)
      setColor(canvas.theme->line_midi_alsa_glow);
}

END_NAMESPACE_PATCHCANVAS

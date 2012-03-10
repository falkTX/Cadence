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

#include "canvaslinemov.h"

#include <QtGui/QPainter>

#include "canvasport.h"

START_NAMESPACE_PATCHCANVAS

CanvasLineMov::CanvasLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent) :
    QGraphicsLineItem(parent, canvas.scene)
{
    m_port_mode = port_mode;
    m_port_type = port_type;

    // Port position doesn't change while moving around line
    p_lineX = scenePos().x();
    p_lineY = scenePos().y();
    p_width = ((CanvasPort*)parentItem())->getPortWidth();

    QPen pen;

    if (port_type == PORT_TYPE_AUDIO_JACK)
        pen = QPen(canvas.theme->line_audio_jack, 2);
    else if (port_type == PORT_TYPE_MIDI_JACK)
        pen = QPen(canvas.theme->line_midi_jack, 2);
    else if (port_type == PORT_TYPE_MIDI_A2J)
        pen = QPen(canvas.theme->line_midi_a2j, 2);
    else if (port_type == PORT_TYPE_MIDI_ALSA)
        pen = QPen(canvas.theme->line_midi_alsa, 2);
    else
    {
        qWarning("PatchCanvas::CanvasLineMov(%s, %s, %p) - invalid port type", port_mode2str(port_mode), port_type2str(port_type), parent);
        pen = QPen(Qt::black);
    }

    setPen(pen);
}

void CanvasLineMov::deleteFromScene()
{
    canvas.scene->removeItem(this);
    delete this;
}

void CanvasLineMov::updateLinePos(QPointF scenePos)
{
    int item_pos[2] = { 0, 0 };

    if (m_port_mode == PORT_MODE_INPUT)
    {
        item_pos[0] = 0;
        item_pos[1] = 7.5;
    }
    else if (m_port_mode == PORT_MODE_OUTPUT)
    {
        item_pos[0] = p_width+12;
        item_pos[1] = 7.5;
    }
    else
        return;

    QLineF line(item_pos[0], item_pos[1], scenePos.x()-p_lineX, scenePos.y()-p_lineY);
    setLine(line);
}

int CanvasLineMov::type() const
{
    return CanvasLineMovType;
}

void CanvasLineMov::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsLineItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS

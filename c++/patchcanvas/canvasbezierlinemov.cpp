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

#include "canvasbezierlinemov.h"

#include <QtGui/QPainter>

#include "canvasport.h"

START_NAMESPACE_PATCHCANVAS

CanvasBezierLineMov::CanvasBezierLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent) :
    QGraphicsPathItem(parent, canvas.scene)
{
    m_port_mode = port_mode;
    m_port_type = port_type;

    // Port position doesn't change while moving around line
    p_itemX = scenePos().x();
    p_itemY = scenePos().y();
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
        qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %p) - invalid port type", port_mode2str(port_mode), port_type2str(port_type), parent);
        pen = QPen(Qt::black);
    }

    QColor color(0,0,0,0);
    setBrush(color);
    setPen(pen);
}

void CanvasBezierLineMov::deleteFromScene()
{
    canvas.scene->removeItem(this);
    delete this;
}

void CanvasBezierLineMov::updateLinePos(QPointF scenePos)
{
    int old_x, old_y, mid_x, new_x, final_x, final_y;

    if (m_port_mode == PORT_MODE_INPUT)
    {
        old_x = 0;
        old_y = 7.5;
        mid_x = abs(scenePos.x()-p_itemX)/2;
        new_x = old_x-mid_x;
    }
    else if (m_port_mode == PORT_MODE_OUTPUT)
    {
        old_x = p_width+12;
        old_y = 7.5;
        mid_x = abs(scenePos.x()-(p_itemX+old_x))/2;
        new_x = old_x+mid_x;
    }
    else
        return;

    final_x = scenePos.x()-p_itemX;
    final_y = scenePos.y()-p_itemY;

    QPainterPath path(QPointF(old_x, old_y));
    path.cubicTo(new_x, old_y, new_x, final_y, final_x, final_y);
    setPath(path);
}

int CanvasBezierLineMov::type() const
{
    return CanvasBezierLineMovType;
}

void CanvasBezierLineMov::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, bool(options.antialiasing));
    QGraphicsPathItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS

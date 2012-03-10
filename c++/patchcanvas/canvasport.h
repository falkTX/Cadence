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

#ifndef CANVASPORT_H
#define CANVASPORT_H

#include "patchcanvas.h"

class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneMouseEvent;
class QPainter;

START_NAMESPACE_PATCHCANVAS

class AbstractCanvasLineMov;

class CanvasPort : public QGraphicsItem
{
public:
    CanvasPort(int port_id, QString port_name, PortMode port_mode, PortType port_type, QGraphicsItem* parent);

    int getPortId();
    PortMode getPortMode();
    PortType getPortType();
    QString getPortName();
    QString getFullPortName();
    int getPortWidth();
    int getPortHeight();

    void setPortMode(PortMode port_mode);
    void setPortType(PortType port_type);
    void setPortName(QString port_name);
    void setPortWidth(int port_width);

    virtual int type() const;

private:
    int m_port_id;
    PortMode m_port_mode;
    PortType m_port_type;
    QString m_port_name;

    int m_port_width;
    int m_port_height;
    QFont m_port_font;

    AbstractCanvasLineMov* m_line_mov;
    CanvasPort* m_hover_item;
    bool m_last_selected_state;

    bool m_mouse_down;
    bool m_cursor_moving;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORT_H

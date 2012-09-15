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

#ifndef CANVASLINEMOV_H
#define CANVASLINEMOV_H

#include <QGraphicsLineItem>

#include "abstractcanvasline.h"

class QPainter;

START_NAMESPACE_PATCHCANVAS

class CanvasLineMov :
    public AbstractCanvasLineMov,
    public QGraphicsLineItem
{
public:
    CanvasLineMov(PortMode port_mode, PortType port_type, QGraphicsItem* parent);

    virtual void deleteFromScene();

    virtual void updateLinePos(QPointF scenePos);

    virtual int type() const;

    // QGraphicsItem generic calls
    virtual void setZValue(qreal z)
    {
        QGraphicsLineItem::setZValue(z);
    }

private:
    PortMode m_port_mode;
    PortType m_port_type;
    int p_lineX;
    int p_lineY;
    int p_width;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASLINEMOV_H

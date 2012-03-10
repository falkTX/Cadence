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

#ifndef ABSTRACTCANVASLINE_H
#define ABSTRACTCANVASLINE_H

#include "patchcanvas.h"

START_NAMESPACE_PATCHCANVAS

class AbstractCanvasLine
{
public:
    AbstractCanvasLine() {}

    virtual void deleteFromScene() = 0;

    virtual bool isLocked() const = 0;
    virtual void setLocked(bool yesno) = 0;

    virtual bool isLineSelected() const = 0;
    virtual void setLineSelected(bool yesno) = 0;

    virtual void updateLinePos() = 0;

    virtual int type() const = 0;

    // QGraphicsItem generic calls
    virtual void setZValue(qreal z) = 0;
};

class AbstractCanvasLineMov
{
public:
    AbstractCanvasLineMov() {}

    virtual void deleteFromScene() = 0;

    virtual void updateLinePos(QPointF scenePos) = 0;

    virtual int type() const = 0;

    // QGraphicsItem generic calls
    virtual void setZValue(qreal z) = 0;
};

END_NAMESPACE_PATCHCANVAS

#endif // ABSTRACTCANVASLINE_H

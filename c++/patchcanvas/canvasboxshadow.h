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

#ifndef CANVASBOXSHADOW_H
#define CANVASBOXSHADOW_H

#include <QGraphicsDropShadowEffect>

#include "patchcanvas.h"

START_NAMESPACE_PATCHCANVAS

class CanvasBox;

class CanvasBoxShadow : public QGraphicsDropShadowEffect
{
public:
    CanvasBoxShadow(QObject* parent);
    void setFakeParent(CanvasBox* fakeParent);
    void setOpacity(float opacity);

protected:
    virtual void draw(QPainter* painter);

private:
    CanvasBox* m_fakeParent;
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBOXSHADOW_H

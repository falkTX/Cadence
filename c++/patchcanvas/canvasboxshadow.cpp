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

#include "canvasboxshadow.h"

#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

CanvasBoxShadow::CanvasBoxShadow(QObject* parent) :
    QGraphicsDropShadowEffect(parent)
{
    m_fakeParent = 0;

    setBlurRadius(20);
    setColor(canvas.theme->box_shadow);
    setOffset(0, 0);
}

void CanvasBoxShadow::setFakeParent(CanvasBox* fakeParent)
{
    m_fakeParent = fakeParent;
}

void CanvasBoxShadow::setOpacity(float opacity)
{
        QColor color(canvas.theme->box_shadow);
        color.setAlphaF(opacity);
        setColor(color);
}

void CanvasBoxShadow::draw(QPainter* painter)
{
    if (m_fakeParent)
        m_fakeParent->repaintLines();
    QGraphicsDropShadowEffect::draw(painter);
}

END_NAMESPACE_PATCHCANVAS

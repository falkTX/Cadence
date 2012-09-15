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

#include "canvasfadeanimation.h"

#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

CanvasFadeAnimation::CanvasFadeAnimation(QGraphicsItem* item, bool show, QObject* parent) :
    QAbstractAnimation(parent)
{
    m_show = show;
    m_duration = 0;
    m_item = item;
}

QGraphicsItem* CanvasFadeAnimation::item()
{
    return m_item;
}

void CanvasFadeAnimation::setDuration(int time)
{
    if (m_show == false && m_item->opacity() == 0.0)
        m_duration = 0;
    else
    {
        m_item->show();
        m_duration = time;
    }
}

int CanvasFadeAnimation::duration() const
{
    return m_duration;
}

void CanvasFadeAnimation::updateCurrentTime(int time)
{
    if (m_duration == 0)
      return;

    float value;

    if (m_show)
      value = float(time)/m_duration;
    else
      value = 1.0-(float(time)/m_duration);

    m_item->setOpacity(value);

    if (m_item->type() == CanvasBoxType)
        ((CanvasBox*)m_item)->setShadowOpacity(value);
}

void CanvasFadeAnimation::updateState(QAbstractAnimation::State /*newState*/, QAbstractAnimation::State /*oldState*/)
{
}

void CanvasFadeAnimation::updateDirection(QAbstractAnimation::Direction /*direction*/)
{
}

END_NAMESPACE_PATCHCANVAS

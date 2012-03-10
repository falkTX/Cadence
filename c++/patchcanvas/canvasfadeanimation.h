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

#ifndef CANVASFADEANIMATION_H
#define CANVASFADEANIMATION_H

#include <QtCore/QAbstractAnimation>

#include "patchcanvas.h"

class QGraphicsItem;

START_NAMESPACE_PATCHCANVAS

class CanvasFadeAnimation : public QAbstractAnimation
{
public:
    CanvasFadeAnimation(QGraphicsItem* item, bool show, QObject* parent=0);

    QGraphicsItem* item();
    void setDuration(int time);

    virtual int duration() const;

protected:
    virtual void updateCurrentTime(int time);
    virtual void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    virtual void updateDirection(QAbstractAnimation::Direction direction);

private:
    bool m_show;
    int m_duration;
    QGraphicsItem* m_item;
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASFADEANIMATION_H

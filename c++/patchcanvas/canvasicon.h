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

#ifndef CANVASICON_H
#define CANVASICON_H

#include <QtSvg/QGraphicsSvgItem>

#include "patchcanvas.h"

class QPainter;
class QGraphicsColorizeEffect;
class QSvgRenderer;

START_NAMESPACE_PATCHCANVAS

class CanvasIcon : public QGraphicsSvgItem
{
public:
    CanvasIcon(Icon icon, QString name, QGraphicsItem* parent);
    ~CanvasIcon();

    void setIcon(Icon icon, QString name);

    virtual int type() const;

private:
    QGraphicsColorizeEffect* m_colorFX;
    QSvgRenderer* m_renderer;
    QRectF p_size;

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASICON_H

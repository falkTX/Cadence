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

#include "canvasicon.h"

#include <QtGui/QPainter>
#include <QtGui/QGraphicsColorizeEffect>
#include <QtSvg/QSvgRenderer>

START_NAMESPACE_PATCHCANVAS

CanvasIcon::CanvasIcon(Icon icon, QString name, QGraphicsItem* parent) :
    QGraphicsSvgItem(parent)
{
    m_renderer = 0;
    p_size = QRectF(0, 0, 0, 0);

    m_colorFX = new QGraphicsColorizeEffect(this);
    m_colorFX->setColor(canvas.theme->box_text.color());

    setGraphicsEffect(m_colorFX);
    setIcon(icon, name);
}

CanvasIcon::~CanvasIcon()
{
    if (m_renderer)
        delete m_renderer;
    delete m_colorFX;
}

void CanvasIcon::setIcon(Icon icon, QString name)
{
    name = name.toLower();
    QString icon_path;

    if (icon == ICON_APPLICATION)
    {
        p_size = QRectF(3, 2, 19, 18);

        if (name.contains("audacious"))
        {
            p_size = QRectF(5, 4, 16, 16);
            icon_path = ":/scalable/pb_audacious.svg";
        }
        else if (name.contains("clementine"))
        {
            p_size = QRectF(5, 4, 16, 16);
            icon_path = ":/scalable/pb_clementine.svg";
        }
        else if (name.contains("jamin"))
        {
            p_size = QRectF(5, 3, 16, 16);
            icon_path = ":/scalable/pb_jamin.svg";
        }
        else if (name.contains("mplayer"))
        {
            p_size = QRectF(5, 4, 16, 16);
            icon_path = ":/scalable/pb_mplayer.svg";
        }
        else if (name.contains("vlc"))
        {
            p_size = QRectF(5, 3, 16, 16);
            icon_path = ":/scalable/pb_vlc.svg";
        }
        else
        {
            p_size = QRectF(5, 3, 16, 16);
            icon_path = ":/scalable/pb_generic.svg";
        }
    }
    else if (icon == ICON_HARDWARE)
    {
        p_size = QRectF(5, 2, 16, 16);
        icon_path = ":/scalable/pb_hardware.svg";

    }
    else if (icon == ICON_LADISH_ROOM)
    {
        p_size = QRectF(5, 2, 16, 16);
        icon_path = ":/scalable/pb_hardware.svg";
    }
    else
    {
        p_size = QRectF(0, 0, 0, 0);
        qCritical("PatchCanvas::CanvasIcon->setIcon(%s, %s) - unsupported Icon requested", icon2str(icon), name.toUtf8().constData());
        return;
    }

    if (m_renderer)
        delete m_renderer;

    m_renderer = new QSvgRenderer(icon_path, canvas.scene);
    setSharedRenderer(m_renderer);
    update();
}

int CanvasIcon::type() const
{
    return CanvasIconType;
}

QRectF CanvasIcon::boundingRect() const
{
    return p_size;
}

void CanvasIcon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (m_renderer)
    {
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setRenderHint(QPainter::TextAntialiasing, false);
        m_renderer->render(painter, p_size);
    }
    else
        QGraphicsSvgItem::paint(painter, option, widget);
}

END_NAMESPACE_PATCHCANVAS

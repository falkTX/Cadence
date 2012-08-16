/*
 * Pixmap Dial, a custom Qt4 widget
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

#include "pixmapdial.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>

PixmapDial::PixmapDial(QWidget* parent):
    QDial(parent)
{
    m_pixmap.load(":/bitmaps/dial_01d.png");
    m_pixmap_n_str = "01";
    m_custom_paint = CUSTOM_PAINT_NULL;

    m_hovered    = false;
    m_hover_step = HOVER_MIN;

    if (m_pixmap.width() > m_pixmap.height())
        m_orientation = HORIZONTAL;
    else
        m_orientation = VERTICAL;

    m_label = "";
    m_label_pos = QPointF(0.0, 0.0);
    m_label_width  = 0;
    m_label_height = 0;
    m_label_gradient = QLinearGradient(0, 0, 0, 1);

    if (palette().window().color().lightness() > 100)
    {
        // Light background
        QColor c = palette().dark().color();
        m_color1 = c;
        m_color2 = QColor(c.red(), c.green(), c.blue(), 0);
        m_colorT[0] = palette().buttonText().color();
        m_colorT[1] = palette().mid().color();
    }
    else
    {
        // Dark background
        m_color1 = QColor(0, 0, 0, 255);
        m_color2 = QColor(0, 0, 0, 0);
        m_colorT[0] = Qt::white;
        m_colorT[1] = Qt::darkGray;
    }

    updateSizes();
}

int PixmapDial::getSize() const
{
    return p_size;
}

void PixmapDial::setCustomPaint(CustomPaint paint)
{
    m_custom_paint = paint;
    update();
}

void PixmapDial::setEnabled(bool enabled)
{
    if (isEnabled() != enabled)
    {
        m_pixmap.load(QString(":/dial_%1%2.png").arg(m_pixmap_n_str).arg(enabled ? "" : "d"));
        updateSizes();
        update();
    }
    QDial::setEnabled(enabled);
}

void PixmapDial::setLabel(QString label)
{
    m_label = label;

    m_label_width  = QFontMetrics(font()).width(label);
    m_label_height = QFontMetrics(font()).height();

    m_label_pos.setX((p_size/2)-(m_label_width/2));
    m_label_pos.setY(p_size+m_label_height);

    m_label_gradient.setColorAt(0.0, m_color1);
    m_label_gradient.setColorAt(0.6, m_color1);
    m_label_gradient.setColorAt(1.0, m_color2);

    m_label_gradient.setStart(0, p_size/2);
    m_label_gradient.setFinalStop(0, p_size+m_label_height+5);

    m_label_gradient_rect = QRectF(p_size*1/8, p_size/2, p_size*6/8, p_size+m_label_height+5);
    update();
}

void PixmapDial::setPixmap(int pixmap_id)
{
    if (pixmap_id > 10)
        m_pixmap_n_str = QString::number(pixmap_id);
    else
        m_pixmap_n_str = QString("0%1").arg(pixmap_id);

    m_pixmap.load(QString(":/bitmaps/dial_%1%2.png").arg(m_pixmap_n_str).arg(isEnabled() ? "" : "d"));

    if (m_pixmap.width() > m_pixmap.height())
        m_orientation = HORIZONTAL;
    else
        m_orientation = VERTICAL;

    updateSizes();
    update();
}

QSize PixmapDial::minimumSizeHint() const
{
    return QSize(p_size, p_size);
}

QSize PixmapDial::sizeHint() const
{
    return QSize(p_size, p_size);
}

void PixmapDial::updateSizes()
{
    p_width  = m_pixmap.width();
    p_height = m_pixmap.height();

    if (p_width < 1)
        p_width = 1;

    if (p_height < 1)
        p_height = 1;

    if (m_orientation == HORIZONTAL)
    {
        p_size  = p_height;
        p_count = p_width/p_height;
    }
    else
    {
        p_size  = p_width;
        p_count = p_height/p_width;
    }

    setMinimumSize(p_size, p_size + m_label_height + 5);
    setMaximumSize(p_size, p_size + m_label_height + 5);
}

void PixmapDial::enterEvent(QEvent* event)
{
    m_hovered = true;
    if (m_hover_step == HOVER_MIN)
        m_hover_step = HOVER_MIN + 1;
    QDial::enterEvent(event);
}

void PixmapDial::leaveEvent(QEvent* event)
{
    m_hovered = false;
    if (m_hover_step == HOVER_MAX)
        m_hover_step = HOVER_MAX - 1;
    QDial::leaveEvent(event);
}

void PixmapDial::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    if (! m_label.isEmpty())
    {
        painter.setPen(m_color2);
        painter.setBrush(m_label_gradient);
        painter.drawRect(m_label_gradient_rect);

        painter.setPen(isEnabled() ? m_colorT[0] : m_colorT[1]);
        painter.drawText(m_label_pos, m_label);
    }

    QRectF target, source;

    if (isEnabled())
    {
        float current = value()-minimum();
        float divider = maximum()-minimum();

        if (divider == 0.0f)
            return;

        target = QRectF(0.0, 0.0, p_size, p_size);
        float value = current/divider;

        int xpos, ypos, per = int((p_count-1) * value);

        if (m_orientation == HORIZONTAL)
        {
            xpos = p_size*per;
            ypos = 0.0;
        }
        else
        {
            xpos = 0.0;
            ypos = p_size*per;
        }

        source = QRectF(xpos, ypos, p_size, p_size);
        painter.drawPixmap(target, m_pixmap, source);

        // Custom knobs (Dry/Wet and Volume)
        // TODO

        // Custom knobs (L and R)
        // TODO

        if (HOVER_MIN > m_hover_step && m_hover_step < HOVER_MAX)
        {
            m_hover_step += m_hovered ? 1 : -1;
            QTimer::singleShot(20, this, SLOT(update()));
        }
    }
    else
    {
        target = QRectF(0.0, 0.0, p_size, p_size);
        source = target;
        painter.drawPixmap(target, m_pixmap, source);
    }
}

void PixmapDial::resizeEvent(QResizeEvent* event)
{
    updateSizes();
    QDial::resizeEvent(event);
}

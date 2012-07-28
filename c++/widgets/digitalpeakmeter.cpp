/*
 * Digital Peak Meter, a custom Qt4 widget
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

#include "digitalpeakmeter.h"

#include <QtGui/QPainter>

DigitalPeakMeter::DigitalPeakMeter(QWidget* parent):
    QWidget(parent)
{
    m_channels = 0;
    m_orientation = VERTICAL;
    m_smoothMultiplier = 1;

    m_colorBackground = QColor("#111111");
    m_gradientMeter = QLinearGradient(0, 0, 1, 1);

    setChannels(0);
    setColor(GREEN);

    m_paintTimer.setInterval(60);
    connect(&m_paintTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_paintTimer.start();
}

void DigitalPeakMeter::displayMeter(int meter, float level)
{
    Q_ASSERT(meter > 0 && meter <= m_channels);

    if (meter <= 0 || meter > m_channels)
        return qCritical("DigitalPeakMeter::displayMeter(%i, %f) - invalid meter number", meter, level);

    if (level < 0.0f)
        level = -level;
    else if (level > 1.0f)
        level = 1.0f;

    m_channelsData[meter-1] = level;
}

void DigitalPeakMeter::setChannels(int channels)
{
    Q_ASSERT(channels >= 0);

    if (channels < 0)
        return qCritical("DigitalPeakMeter::setChannels(%i) - channels must be a positive integer", channels);

    m_channels = channels;
    m_channelsData.clear();
    m_lastValueData.clear();

    for (int i=0; i < channels; i++)
    {
        m_channelsData.append(0.0f);
        m_lastValueData.append(0.0f);
    }
}

void DigitalPeakMeter::setColor(Color color)
{
    if (color == GREEN)
    {
        m_colorBase  = QColor("#5DE73D");
        m_colorBaseT = QColor(15, 110, 15, 100);
    }
    else if (color == BLUE)
    {
        m_colorBase  = QColor("#52EEF8");
        m_colorBaseT = QColor(15, 15, 110, 100);
    }
    else
        return qCritical("DigitalPeakMeter::setColor(%i) - invalid color", color);

    setOrientation(m_orientation);
}

void DigitalPeakMeter::setOrientation(Orientation orientation)
{
    m_orientation = orientation;

    if (m_orientation == HORIZONTAL)
    {
        m_gradientMeter.setColorAt(0.0, m_colorBase);
        m_gradientMeter.setColorAt(0.2, m_colorBase);
        m_gradientMeter.setColorAt(0.4, m_colorBase);
        m_gradientMeter.setColorAt(0.6, m_colorBase);
        m_gradientMeter.setColorAt(0.8, Qt::yellow);
        m_gradientMeter.setColorAt(1.0, Qt::red);
    }
    else if (m_orientation == VERTICAL)
    {
        m_gradientMeter.setColorAt(0.0, Qt::red);
        m_gradientMeter.setColorAt(0.2, Qt::yellow);
        m_gradientMeter.setColorAt(0.4, m_colorBase);
        m_gradientMeter.setColorAt(0.6, m_colorBase);
        m_gradientMeter.setColorAt(0.8, m_colorBase);
        m_gradientMeter.setColorAt(1.0, m_colorBase);
    }
    else
        return qCritical("DigitalPeakMeter::setOrientation(%i) - invalid orientation", orientation);

    updateSizes();
}

void DigitalPeakMeter::setRefreshRate(int rate)
{
    Q_ASSERT(rate > 0);

    m_paintTimer.stop();
    m_paintTimer.setInterval(rate);
    m_paintTimer.start();
}

void DigitalPeakMeter::setSmoothRelease(int value)
{
    Q_ASSERT(value >= 0 && value <= 5);

    if (value < 0)
        value = 0;
    else if (value > 5)
        value = 5;

    m_smoothMultiplier = value;
}

QSize DigitalPeakMeter::minimumSizeHint() const
{
    return QSize(30, 30);
}

QSize DigitalPeakMeter::sizeHint() const
{
    return QSize(m_width, m_height);
}

void DigitalPeakMeter::updateSizes()
{
    m_width  = width();
    m_height = height();
    m_sizeMeter = 0;

    if (m_orientation == HORIZONTAL)
    {
        m_gradientMeter.setFinalStop(m_width, 0);

        if (m_channels > 0)
            m_sizeMeter = m_height/m_channels;
    }
    else if (m_orientation == VERTICAL)
    {
        m_gradientMeter.setFinalStop(0, m_height);

        if (m_channels > 0)
            m_sizeMeter = m_width/m_channels;
    }
}

void DigitalPeakMeter::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, m_width, m_height);

    int meterX = 0;
    painter.setPen(m_colorBackground);
    painter.setBrush(m_gradientMeter);

    for (int i=0; i < m_channels; i++)
    {
        float value, level = m_channelsData[i];

        if (level == m_lastValueData[i])
            continue;

        if (m_orientation == HORIZONTAL)
            value = level * m_width;
        else if (m_orientation == VERTICAL)
            value = float(m_height) - (level * m_height);
        else
            value = 0.0f;

        if (value < 0.0f)
            value = 0.0f;
        else if (m_smoothMultiplier > 0)
            value = (m_lastValueData[i] * m_smoothMultiplier + value) / (m_smoothMultiplier + 1);

        if (m_orientation == HORIZONTAL)
            painter.drawRect(0, meterX, value, m_sizeMeter);
        else if (m_orientation == VERTICAL)
            painter.drawRect(meterX, value, m_sizeMeter, m_height);

        meterX += m_sizeMeter;
        m_lastValueData[i] = value;
    }

    painter.setBrush(QColor(0, 0, 0, 0));

    if (m_orientation == HORIZONTAL)
    {
        // Variables
        int lsmall = m_width;
        int lfull  = m_height - 1;

        // Base
        painter.setPen(m_colorBaseT);
        painter.drawLine(lsmall * 0.25, 2, lsmall * 0.25, lfull-2);
        painter.drawLine(lsmall * 0.50, 2, lsmall * 0.50, lfull-2);

        // Yellow
        painter.setPen(QColor(110, 110, 15, 100));
        painter.drawLine(lsmall * 0.70, 2, lsmall * 0.70, lfull-2);
        painter.drawLine(lsmall * 0.83, 2, lsmall * 0.83, lfull-2);

        // Orange
        painter.setPen(QColor(180, 110, 15, 100));
        painter.drawLine(lsmall * 0.90, 2, lsmall * 0.90, lfull-2);

        // Red
        painter.setPen(QColor(110, 15, 15, 100));
        painter.drawLine(lsmall * 0.96, 2, lsmall * 0.96, lfull-2);

    }
    else if (m_orientation == VERTICAL)
    {
        // Variables
        int lsmall = m_height;
        int lfull  = m_width - 1;

        // Base
        painter.setPen(m_colorBaseT);
        painter.drawLine(2, lsmall - (lsmall * 0.25), lfull-2, lsmall - (lsmall * 0.25));
        painter.drawLine(2, lsmall - (lsmall * 0.50), lfull-2, lsmall - (lsmall * 0.50));

        // Yellow
        painter.setPen(QColor(110, 110, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.70), lfull-2, lsmall - (lsmall * 0.70));
        painter.drawLine(2, lsmall - (lsmall * 0.83), lfull-2, lsmall - (lsmall * 0.83));

        // Orange
        painter.setPen(QColor(180, 110, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.90), lfull-2, lsmall - (lsmall * 0.90));

        // Red
        painter.setPen(QColor(110, 15, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.96), lfull-2, lsmall - (lsmall * 0.96));
    }
}

void DigitalPeakMeter::resizeEvent(QResizeEvent* event)
{
    updateSizes();
    QWidget::resizeEvent(event);
}

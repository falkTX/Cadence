/*
 * Pixmap Dial, a custom Qt4 widget
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __PIXMAPDIAL_HPP__
#define __PIXMAPDIAL_HPP__

#include <QtGui/QPixmap>
#include <QtWidgets/QDial>

class PixmapDial : public QDial
{
public:
    enum CustomPaint {
        CUSTOM_PAINT_NULL      = 0,
        CUSTOM_PAINT_CARLA_WET = 1,
        CUSTOM_PAINT_CARLA_VOL = 2,
        CUSTOM_PAINT_CARLA_L   = 3,
        CUSTOM_PAINT_CARLA_R   = 4
    };

    PixmapDial(QWidget* parent);

    int  getSize() const;
    void setCustomPaint(CustomPaint paint);
    void setEnabled(bool enabled);
    void setLabel(QString label);
    void setPixmap(int pixmapId);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    void updateSizes();

    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    enum Orientation {
        HORIZONTAL = 0,
        VERTICAL   = 1
    };

    static const unsigned short HOVER_MIN = 0;
    static const unsigned short HOVER_MAX = 9;

    // -------------------------------------

    QPixmap fPixmap;
    QString fPixmapNum;

    CustomPaint fCustomPaint;
    Orientation fOrientation;

    bool fHovered;
    unsigned short fHoverStep;

    QString fLabel;
    QPointF fLabelPos;
    QFont   fLabelFont;
    int     fLabelWidth;
    int     fLabelHeight;

    QLinearGradient fLabelGradient;
    QRectF fLabelGradientRect;

    QColor fColor1;
    QColor fColor2;
    QColor fColorT[2];

    int fWidth, fHeight, fSize, fCount;
};

#endif // __PIXMAPDIAL_HPP__

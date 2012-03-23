#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Dial, a custom Qt4 widget
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
from PyQt4.QtCore import Qt, QPointF, QRectF, QSize
from PyQt4.QtGui import QColor, QDial, QFontMetrics, QLinearGradient, QPainter, QPixmap

# Imports (Custom Stuff)
import icons_rc

# Widget Class
class PixmapDial(QDial):

    HORIZONTAL = 0
    VERTICAL   = 1

    def __init__(self, parent):
        QDial.__init__(self, parent)

        self.m_pixmap = QPixmap(":/bitmaps/dial_01d.png")
        self.m_pixmap_n_str = "01"

        if (self.m_pixmap.width() > self.m_pixmap.height()):
          self.m_orientation = self.HORIZONTAL
        else:
          self.m_orientation = self.VERTICAL

        self.m_label = ""
        self.m_label_pos = QPointF(0.0, 0.0)
        self.m_label_width = 0
        self.m_label_height = 0
        self.m_label_gradient = QLinearGradient(0, 0, 0, 1)

        if (self.palette().window().color().lightness() > 100):
          # Light background
          self.m_color1 = QColor(100, 100, 100, 255)
          self.m_color2 = QColor(0, 0, 0, 0)
          self.m_colorT = [self.palette().text().color(), self.palette().mid().color()]
        else:
          ## Dark background
          self.m_color1 = QColor(0, 0, 0, 255)
          self.m_color2 = QColor(0, 0, 0, 0)
          self.m_colorT = [Qt.white, Qt.darkGray]

        self.updateSizes()

    def getSize(self):
        return self.p_size

    def setEnabled(self, enabled):
        if (self.isEnabled() != enabled):
          self.m_pixmap.load(":/bitmaps/dial_%s%s.png" % (self.m_pixmap_n_str, "" if enabled else "d"))
          self.updateSizes()
          self.update()
        QDial.setEnabled(self, enabled)

    def setLabel(self, label):
        self.m_label = label

        self.m_label_width  = QFontMetrics(self.font()).width(label)
        self.m_label_height = QFontMetrics(self.font()).height()

        self.m_label_pos.setX((self.p_size/2)-(self.m_label_width/2))
        self.m_label_pos.setY(self.p_size+self.m_label_height)

        self.m_label_gradient.setColorAt(0.0, self.m_color1)
        self.m_label_gradient.setColorAt(0.6, self.m_color1)
        self.m_label_gradient.setColorAt(1.0, self.m_color2)

        self.m_label_gradient.setStart(0, self.p_size/2)
        self.m_label_gradient.setFinalStop(0, self.p_size+self.m_label_height+5)

        self.m_label_gradient_rect = QRectF(self.p_size*1/8, self.p_size/2, self.p_size*6/8, self.p_size+self.m_label_height+5)
        self.update()

    def setPixmap(self, pixmap_id):
        if (pixmap_id > 10):
          self.m_pixmap_n_str = str(pixmap_id)
        else:
          self.m_pixmap_n_str = "0%i" % (pixmap_id)

        self.m_pixmap.load(":/bitmaps/dial_%s%s.png" % (self.m_pixmap_n_str, "" if self.isEnabled() else "d"))

        if (self.m_pixmap.width() > self.m_pixmap.height()):
          self.m_orientation = self.HORIZONTAL
        else:
          self.m_orientation = self.VERTICAL

        self.updateSizes()
        self.update()

    def minimumSizeHint(self):
        return QSize(self.p_size, self.p_size)

    def sizeHint(self):
        return QSize(self.p_size, self.p_size)

    def updateSizes(self):
        self.p_width = self.m_pixmap.width()
        self.p_height = self.m_pixmap.height()

        if (self.p_width < 1):
          self.p_width = 1

        if (self.p_height < 1):
          self.p_height = 1

        if (self.m_orientation == self.HORIZONTAL):
          self.p_size = self.p_height
          self.p_count = self.p_width/self.p_height
        else:
          self.p_size = self.p_width
          self.p_count = self.p_height/self.p_width

        self.setMinimumSize(self.p_size, self.p_size+self.m_label_height+5)
        self.setMaximumSize(self.p_size, self.p_size+self.m_label_height+5)

    def paintEvent(self, event):
        painter = QPainter(self)

        if (self.m_label):
          painter.setPen(self.m_color2)
          painter.setBrush(self.m_label_gradient)
          painter.drawRect(self.m_label_gradient_rect)

          painter.setPen(self.m_colorT[0] if self.isEnabled() else self.m_colorT[1])
          painter.drawText(self.m_label_pos, self.m_label)

        if (self.isEnabled()):
          current = float(self.value()-self.minimum())
          divider = float(self.maximum()-self.minimum())

          if (divider == 0.0):
            return

          target = QRectF(0.0, 0.0, self.p_size, self.p_size)
          per = int((self.p_count-1)*(current/divider))

          if (self.m_orientation == self.HORIZONTAL):
            xpos = self.p_size*per
            ypos = 0.0
          else:
            xpos = 0.0
            ypos = self.p_size*per

          source = QRectF(xpos, ypos, self.p_size, self.p_size)

        else:
          target = QRectF(0.0, 0.0, self.p_size, self.p_size)
          source = target

        painter.drawPixmap(target, self.m_pixmap, source)

    def resizeEvent(self, event):
        self.updateSizes()
        return QDial.resizeEvent(self, event)

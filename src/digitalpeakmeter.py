#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Digital Peak Meter, a custom Qt4 widget
# Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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
from PyQt4.QtCore import qCritical, Qt, QRectF, QTimer, QSize
from PyQt4.QtGui import QColor, QLinearGradient, QPainter, QWidget

# Widget Class
class DigitalPeakMeter(QWidget):
    HORIZONTAL = 1
    VERTICAL   = 2

    GREEN = 1
    BLUE  = 2

    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.m_channels = 0
        self.bg_color = QColor("#111111")

        self.base_color  = QColor("#5DE73D")
        self.base_colorT = QColor(15, 110, 15, 100)
        self.m_orientation = self.VERTICAL

        self.meter_gradient = QLinearGradient(0, 0, 1, 1)
        self.smooth_multiplier = 1

        self.setOrientation(self.VERTICAL)
        self.setChannels(2)

        self.paint_timer = QTimer()
        self.paint_timer.setInterval(60)
        self.paint_timer.timeout.connect(self.update)
        self.paint_timer.start()

    def minimumSizeHint(self):
        return QSize(30, 30)

    def sizeHint(self):
        return QSize(self.width_, self.height_)

    def setChannels(self, channels):
        self.m_channels = channels
        self.channels_data = []
        self.last_max_data = []

        if (channels > 0):
          for i in range(channels):
            self.channels_data.append(0.0)
            self.last_max_data.append(0.0) #self.height_

    def setColor(self, color):
        if (color == self.GREEN):
          self.base_color = QColor("#5DE73D")
          self.base_colorT = QColor(15, 110, 15, 100)
        elif (color == self.BLUE):
          self.base_color = QColor("#52EEF8")
          self.base_colorT = QColor(15, 15, 110, 100)
        else:
          return

        self.setOrientation(self.m_orientation)

    def setOrientation(self, orientation):
        self.m_orientation = orientation

        if (self.m_orientation == self.HORIZONTAL):
          self.meter_gradient.setColorAt(0.0, self.base_color)
          self.meter_gradient.setColorAt(0.2, self.base_color)
          self.meter_gradient.setColorAt(0.4, self.base_color)
          self.meter_gradient.setColorAt(0.6, self.base_color)
          self.meter_gradient.setColorAt(0.8, Qt.yellow)
          self.meter_gradient.setColorAt(1.0, Qt.red)

        elif (self.m_orientation == self.VERTICAL):
          self.meter_gradient.setColorAt(0.0, Qt.red)
          self.meter_gradient.setColorAt(0.2, Qt.yellow)
          self.meter_gradient.setColorAt(0.4, self.base_color)
          self.meter_gradient.setColorAt(0.6, self.base_color)
          self.meter_gradient.setColorAt(0.8, self.base_color)
          self.meter_gradient.setColorAt(1.0, self.base_color)

        self.checkSizes()

    def setRefreshRate(self, rate):
        self.paint_timer.stop()
        self.paint_timer.setInterval(rate)
        self.paint_timer.start()

    def setSmoothRelease(self, value):
        if (value < 0):
          value = 0
        elif (value > 5):
          value = 5
        self.smooth_multiplier = value

    def displayMeter(self, meter_n, level):
        if (meter_n > self.m_channels):
          qCritical("DigitalPeakMeter::displayMeter(%i, %f) - Invalid meter number", meter_n, level)
          return

        if (level < 0.0):
          level = -level
        if (level > 1.0):
          level = 1.0

        self.channels_data[meter_n-1] = level

    def checkSizes(self):
        self.width_ = self.width()
        self.height_ = self.height()
        self.meter_size = 0

        if (self.m_orientation == self.HORIZONTAL):
          self.meter_gradient.setFinalStop(self.width_, 0)
          if (self.m_channels > 0):
            self.meter_size = self.height_/self.m_channels

        elif (self.m_orientation == self.VERTICAL):
          self.meter_gradient.setFinalStop(0, self.height_)
          if (self.m_channels > 0):
            self.meter_size = self.width_/self.m_channels

    def paintEvent(self, event):
        painter = QPainter(self)

        painter.setPen(Qt.black)
        painter.setBrush(Qt.black)
        painter.drawRect(0, 0, self.width_, self.height_)

        meter_x = 0

        for i in range(self.m_channels):
          level = self.channels_data[i]

          if (level == self.last_max_data[i]):
            continue

          if (self.m_orientation == self.HORIZONTAL):
            value = self.width_*level
          elif (self.m_orientation == self.VERTICAL):
            value = self.height_-(self.height_*level)
          else:
            value = 0

          # Don't bounce the meter so much
          if (self.smooth_multiplier > 0):
            value = (self.last_max_data[i]*self.smooth_multiplier + value)/(self.smooth_multiplier+1)

          if (value < 0):
            value = 0

          painter.setPen(self.bg_color)
          painter.setBrush(self.meter_gradient)

          if (self.m_orientation == self.HORIZONTAL):
            painter.drawRect(0, meter_x, value, self.meter_size)
          elif (self.m_orientation == self.VERTICAL):
            painter.drawRect(meter_x, value, self.meter_size, self.height_)

          meter_x += self.meter_size
          self.last_max_data[i] = value

        if (self.m_orientation == self.HORIZONTAL):
          lsmall = self.width_
          lfull = self.height_-1
        elif (self.m_orientation == self.VERTICAL):
          lsmall = self.height_
          lfull = self.width_-1
        else:
          return

        painter.setBrush(QColor(0, 0, 0, 0))

        if (self.m_orientation == self.HORIZONTAL):
          # Base
          painter.setPen(self.base_colorT)
          painter.drawLine(lsmall/4, 1, lsmall/4, lfull)
          painter.drawLine(lsmall/2, 1, lsmall/2, lfull)

          # Yellow
          painter.setPen(QColor(110, 110, 15, 100))
          painter.drawLine(lsmall/1.4, 1, lsmall/1.4, lfull)
          painter.drawLine(lsmall/1.2, 1, lsmall/1.2, lfull)

          # Orange
          painter.setPen(QColor(180, 110, 15, 100))
          painter.drawLine(lsmall/1.1, 1, lsmall/1.1, lfull)

          # Red
          painter.setPen(QColor(110, 15, 15, 100))
          painter.drawLine(lsmall/1.04, 1, lsmall/1.04, lfull)

        elif (self.m_orientation == self.VERTICAL):
          # Base
          painter.setPen(self.base_colorT)
          painter.drawLine(1, lsmall-(lsmall/4), lfull, lsmall-(lsmall/4))
          painter.drawLine(1, lsmall-(lsmall/2), lfull, lsmall-(lsmall/2))

          # Yellow
          painter.setPen(QColor(110, 110, 15, 100))
          painter.drawLine(1, lsmall-(lsmall/1.4), lfull, lsmall-(lsmall/1.4))
          painter.drawLine(1, lsmall-(lsmall/1.2), lfull, lsmall-(lsmall/1.2))

          # Orange
          painter.setPen(QColor(180, 110, 15, 100))
          painter.drawLine(1, lsmall-(lsmall/1.1), lfull, lsmall-(lsmall/1.1))

          # Red
          painter.setPen(QColor(110, 15, 15, 100))
          painter.drawLine(1, lsmall-(lsmall/1.04), lfull, lsmall-(lsmall/1.04))

    def resizeEvent(self, event):
        QTimer.singleShot(0, self.checkSizes)
        return QWidget.resizeEvent(self, event)

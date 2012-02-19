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
        self.m_orientation = self.VERTICAL
        self.m_smoothMultiplier = 1

        self.m_colorBackground = QColor("#111111")
        self.m_gradientMeter = QLinearGradient(0, 0, 1, 1)

        self.setChannels(0)
        self.setColor(self.GREEN)

        self.m_paintTimer = QTimer(self)
        self.m_paintTimer.setInterval(60)
        self.m_paintTimer.timeout.connect(self.update)
        self.m_paintTimer.start()

    def minimumSizeHint(self):
        return QSize(30, 30)

    def sizeHint(self):
        return QSize(self.m_width, self.m_height)

    def setChannels(self, channels):
        self.m_channels = channels
        self.m_channels_data = []
        self.m_lastValueData = []

        for i in range(channels):
          self.m_channels_data.append(0.0)
          self.m_lastValueData.append(0.0)

    def setColor(self, color):
        if (color == self.GREEN):
          self.m_colorBase = QColor("#5DE73D")
          self.m_colorBaseT = QColor(15, 110, 15, 100)
        elif (color == self.BLUE):
          self.m_colorBase = QColor("#52EEF8")
          self.m_colorBaseT = QColor(15, 15, 110, 100)
        else:
          return

        self.setOrientation(self.m_orientation)

    def setOrientation(self, orientation):
        self.m_orientation = orientation

        if (self.m_orientation == self.HORIZONTAL):
          self.m_gradientMeter.setColorAt(0.0, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.2, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.4, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.6, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.8, Qt.yellow)
          self.m_gradientMeter.setColorAt(1.0, Qt.red)

        elif (self.m_orientation == self.VERTICAL):
          self.m_gradientMeter.setColorAt(0.0, Qt.red)
          self.m_gradientMeter.setColorAt(0.2, Qt.yellow)
          self.m_gradientMeter.setColorAt(0.4, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.6, self.m_colorBase)
          self.m_gradientMeter.setColorAt(0.8, self.m_colorBase)
          self.m_gradientMeter.setColorAt(1.0, self.m_colorBase)

        self.checkSizes()

    def setRefreshRate(self, rate):
        self.m_paintTimer.stop()
        self.m_paintTimer.setInterval(rate)
        self.m_paintTimer.start()

    def setSmoothRelease(self, value):
        if (value < 0):
          value = 0
        elif (value > 5):
          value = 5
        self.m_smoothMultiplier = value

    def displayMeter(self, meter_n, level):
        if (meter_n < 0 or meter_n > self.m_channels):
          qCritical("DigitalPeakMeter::displayMeter(%i, %f) - Invalid meter number", meter_n, level)
          return

        if (level < 0.0):
          level = -level
        if (level > 1.0):
          level = 1.0

        self.m_channels_data[meter_n-1] = level

    def checkSizes(self):
        self.m_width = self.width()
        self.m_height = self.height()
        self.m_sizeMeter = 0

        if (self.m_orientation == self.HORIZONTAL):
          self.m_gradientMeter.setFinalStop(self.m_width, 0)
          if (self.m_channels > 0):
            self.m_sizeMeter = self.m_height/self.m_channels

        elif (self.m_orientation == self.VERTICAL):
          self.m_gradientMeter.setFinalStop(0, self.m_height)
          if (self.m_channels > 0):
            self.m_sizeMeter = self.m_width/self.m_channels

    def paintEvent(self, event):
        painter = QPainter(self)

        painter.setPen(Qt.black)
        painter.setBrush(Qt.black)
        painter.drawRect(0, 0, self.m_width, self.m_height)

        meter_x = 0

        for i in range(self.m_channels):
          level = self.m_channels_data[i]

          if (level == self.m_lastValueData[i]):
            continue

          if (self.m_orientation == self.HORIZONTAL):
            value = self.m_width*level
          elif (self.m_orientation == self.VERTICAL):
            value = self.m_height-(self.m_height*level)
          else:
            value = 0

          if (value < 0):
            value = 0

          # Don't bounce the meter so much
          if (self.m_smoothMultiplier > 0):
            value = (self.m_lastValueData[i]*self.m_smoothMultiplier + value)/(self.m_smoothMultiplier+1)

          painter.setPen(self.m_colorBackground)
          painter.setBrush(self.m_gradientMeter)

          if (self.m_orientation == self.HORIZONTAL):
            painter.drawRect(0, meter_x, value, self.m_sizeMeter)
          elif (self.m_orientation == self.VERTICAL):
            painter.drawRect(meter_x, value, self.m_sizeMeter, self.m_height)

          meter_x += self.m_sizeMeter
          self.m_lastValueData[i] = value

        painter.setBrush(QColor(0, 0, 0, 0))

        if (self.m_orientation == self.HORIZONTAL):
          # Variables
          lsmall = self.m_width
          lfull  = self.m_height-1

          # Base
          painter.setPen(self.m_colorBaseT)
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
          # Variables
          lsmall = self.m_height
          lfull  = self.m_width-1

          # Base
          painter.setPen(self.m_colorBaseT)
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
        QWidget.resizeEvent(self, event)

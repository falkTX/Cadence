#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Custom Mini Canvas Preview, a custom Qt4 widget
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
from PyQt4.QtCore import Qt, QRectF, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QBrush, QColor, QCursor, QFrame, QPainter, QPen

iX = 0
iY = 1
iWidth  = 2
iHeight = 3

# Widget Class
class CanvasPreviewFrame(QFrame):
    def __init__(self, parent):
        QFrame.__init__(self, parent)

        self.m_mouseDown = False

        self.scale = 1.0
        self.scene = None
        self.real_parent = None
        self.fake_width  = 0
        self.fake_height = 0

        self.render_source = self.getRenderSource()
        self.render_target = QRectF(0, 0, 0, 0)

        self.view_pad_x = 0.0
        self.view_pad_y = 0.0
        self.view_rect = [0.0, 0.0, 10.0, 10.0]

    def init(self, scene, real_width, real_height):
        self.scene = scene
        self.fake_width  = float(real_width)/15
        self.fake_height = float(real_height)/15

        self.setMinimumSize(self.fake_width/2, self.fake_height)
        self.setMaximumSize(self.fake_width*4, self.fake_height)

        self.render_target.setWidth(real_width)
        self.render_target.setHeight(real_height)

    def getRenderSource(self):
        x_pad = (self.width()-self.fake_width)/2
        y_pad = (self.height()-self.fake_height)/2
        return QRectF(x_pad, y_pad, self.fake_width, self.fake_height)

    def setViewPosX(self, xp):
        x = xp*self.fake_width
        x_ratio = (x/self.fake_width)*self.view_rect[iWidth]/self.scale
        self.view_rect[iX] = x-x_ratio+self.render_source.x()
        self.update()

    def setViewPosY(self, yp):
        y = yp*self.fake_height
        y_ratio = (y/self.fake_height)*self.view_rect[iHeight]/self.scale
        self.view_rect[iY] = y-y_ratio+self.render_source.y()
        self.update()

    def setViewScale(self, scale):
        self.scale = scale
        QTimer.singleShot(0, self.real_parent, SLOT("slot_miniCanvasCheckAll()"))

    def setViewSize(self, width_p, height_p):
        width  = width_p*self.fake_width
        height = height_p*self.fake_height
        self.view_rect[iWidth]  = width
        self.view_rect[iHeight] = height
        self.update()

    def setRealParent(self, parent):
        self.real_parent = parent

    def handleMouseEvent(self, event_x, event_y):
        x = float(event_x) - self.render_source.x() - (self.view_rect[iWidth]/self.scale/2)
        y = float(event_y) - self.render_source.y() - (self.view_rect[iHeight]/self.scale/2)

        max_width  = self.view_rect[iWidth]/self.scale
        max_height = self.view_rect[iHeight]/self.scale
        if (max_width > self.fake_width): max_width = self.fake_width
        if (max_height > self.fake_height): max_height = self.fake_height

        if (x < 0.0): x = 0.0
        elif (x > self.fake_width-max_width): x = self.fake_width-max_width

        if (y < 0.0): y = 0.0
        elif (y > self.fake_height-max_height): y = self.fake_height-max_height

        self.view_rect[iX] = x+self.render_source.x()
        self.view_rect[iY] = y+self.render_source.y()
        self.update()

        self.emit(SIGNAL("miniCanvasMoved(double, double)"), x*self.scale/self.fake_width, y*self.scale/self.fake_height)

    def mousePressEvent(self, event):
        if (event.button() == Qt.LeftButton):
          self.m_mouseDown = True
          self.setCursor(QCursor(Qt.SizeAllCursor))
          self.handleMouseEvent(event.x(), event.y())
        event.accept()

    def mouseMoveEvent(self, event):
        if (self.m_mouseDown):
          self.handleMouseEvent(event.x(), event.y())
        event.accept()

    def mouseReleaseEvent(self, event):
        if (self.m_mouseDown):
          self.setCursor(QCursor(Qt.ArrowCursor))
        self.m_mouseDown = False
        QFrame.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)

        painter.setBrush(QBrush(Qt.darkBlue, Qt.DiagCrossPattern))
        painter.drawRect(0, 0, self.width(), self.height())

        self.scene.render(painter, self.render_source, self.render_target, Qt.KeepAspectRatio)

        max_width  = self.view_rect[iWidth]/self.scale
        max_height = self.view_rect[iHeight]/self.scale
        if (max_width > self.fake_width): max_width = self.fake_width
        if (max_height > self.fake_height): max_height = self.fake_height

        painter.setBrush(QBrush(QColor(75,75,255,30)))
        painter.setPen(QPen(Qt.blue, 2))
        painter.drawRect(self.view_rect[iX], self.view_rect[iY], max_width, max_height)

        QFrame.paintEvent(self, event)

    def resizeEvent(self, event):
        self.render_source = self.getRenderSource()
        if (self.real_parent):
          QTimer.singleShot(0, self.real_parent, SLOT("slot_miniCanvasCheckAll()"))
        QFrame.resizeEvent(self, event)

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK Sequencer
# Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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
from PyQt4.QtCore import pyqtSlot, qFatal, Qt, SIGNAL, SLOT
#from PyQt4.QtCore import QLineF, QPointF, QRectF, QSizeF
from PyQt4.QtGui import QColor
from PyQt4.QtGui import QGraphicsItem, QGraphicsScene, QGraphicsView
from PyQt4.QtGui import QFont, QFontMetrics, QPainter
from PyQt4.QtGui import QFrame, QHBoxLayout, QVBoxLayout, QPushButton, QWidget
from PyQt4.QtGui import QLineEdit

# Imports (Custom Stuff)
from caitlib_helpers import *

# ------------------------------------------------------------------------------
# Caitlyn Abstract View

#class CaitlynAbstractScene(QGraphicsScene):
    #def __init__(self, parent, view):
        #QGraphicsScene.__init__(self, parent)

        #self.m_view = view
        #if not self.m_view:
            #qFatal("CaitlynAbstractScene() - invalid view")

class CaitlynAbstractView(QWidget):
    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.m_bars        = 4
        self.m_beatsPerBar = 4
        self.m_beats       = self.m_bars*self.m_beatsPerBar

# ------------------------------------------------------------------------------
# Caitlyn Piano Roll View

#class CaitlynPianoRollView(CaitlynAbstractView):
    #def __init__(self, parent, view):
        #CaitlynAbstractView.__init__(self, parent, view)

# ------------------------------------------------------------------------------
# Caitlyn Tracker View

class CaitlynTrackerNumberButton(QPushButton):
    def __init__(self, parent, number, isBeat):
        QPushButton.__init__(self, parent)

        self.m_colorText    = QColor(Qt.white)
        self.m_colorTextBg  = QColor(Qt.blue)
        self.m_colorTextCur = QColor(Qt.red)

        self.m_font   = QFont("Monospace", 8, QFont.Normal)
        self.m_number = number
        self.m_isBeat = isBeat

        self.m_textWidth  = QFontMetrics(self.m_font).width("000")+1
        self.m_textHeight = QFontMetrics(self.m_font).height()

        self.setCheckable(True)
        self.setFlat(True)
        self.setFixedSize(self.m_textWidth, self.m_textHeight)
        self.setText(("%i" % number) if number > 99 else (" %02i" % number))
        self.setText("%03i" % number)

    def number(self):
        return self.m_number

    def paintEvent(self, event):
        painter = QPainter(self)

        if self.isChecked():
            painter.setBrush(self.m_colorTextCur)
            painter.setPen(self.m_colorTextCur)
            painter.drawRect(0, 0, self.m_textWidth, self.m_textHeight)

        elif self.m_isBeat:
            painter.setBrush(self.m_colorTextBg)
            painter.setPen(self.m_colorTextBg)
            painter.drawRect(0, 0, self.m_textWidth, self.m_textHeight)

        painter.setFont(self.m_font)
        painter.setPen(self.m_colorText)
        painter.drawText(1, self.m_textHeight - self.m_textHeight/6, self.text())

class CaitlynColorWidget(QFrame):
    def __init__(self, parent, color):
        QFrame.__init__(self, parent)

        print("CaitlynColorWidget", color)
        self.setColor(color)

        self.setFixedHeight(10)

    def color(self):
        print(self.width(), self.height())
        return self.m_color

    def setColor(self, color):
        self.m_color = color
        self.repaint()

    def paintEvent(self, event):
        print(self.width(), self.height())
        painter = QPainter(self)
        painter.setBrush(self.m_color)
        painter.setPen(self.m_color)
        painter.drawRect(0, 0, self.width(), self.height())

class CaitlynTrackerTrack(QFrame):
    def __init__(self, parent, trackName):
        QFrame.__init__(self, parent)

        self.m_wColor = CaitlynColorWidget(self, Qt.green)
        self.m_wColor.show()

        self.m_lineEdit = QLineEdit(self)
        self.m_lineEdit.setReadOnly(True)
        self.m_lineEdit.setText(trackName)

        self.m_layout = QVBoxLayout(self)
        self.m_layout.setContentsMargins(0, 0, 0, 0)
        self.m_layout.setSpacing(0)
        self.m_layout.addWidget(self.m_wColor)
        self.m_layout.addWidget(self.m_lineEdit)
        self.m_layout.addStretch() # REMOVE

class CaitlynTrackerView(CaitlynAbstractView):
    def __init__(self, parent):
        CaitlynAbstractView.__init__(self, parent)

        self.m_curBeat = 0
        self.m_curNumberButton = None

        # GUI Stuff
        self.m_frameNumbers = QFrame(self)
        self.m_frameNumbersLayout = QVBoxLayout(self.m_frameNumbers)
        self.m_frameNumbersLayout.setContentsMargins(0, 0, 0, 0)
        self.m_frameNumbersLayout.setSpacing(0)
        self.m_frameNumbersList = []

        for i in range(self.m_beats):
            button = CaitlynTrackerNumberButton(self.m_frameNumbers, i, i % self.m_beatsPerBar == 0)

            if i == 0:
                button.setChecked(True)
                self.m_curNumberButton = button

            self.connect(button, SIGNAL("clicked(bool)"), SLOT("slot_numberButtonClicked(bool)"))
            self.m_frameNumbersLayout.addWidget(button)
            self.m_frameNumbersList.append(button)

        self.m_frameNumbersLayout.addStretch()

        # TESTING
        self.m_track1 = CaitlynTrackerTrack(self, "Track 1")

        self.m_layout = QHBoxLayout(self)
        self.m_layout.addWidget(self.m_frameNumbers)
        self.m_layout.addWidget(self.m_track1)
        self.m_layout.addStretch()

    @pyqtSlot(bool)
    def slot_numberButtonClicked(self, clicked):
        if not clicked: return
        # make all other numbers unchecked
        for button in self.m_frameNumbersList:
            if button != self.sender() and button.isChecked():
                button.setChecked(False)

        self.m_curBeat = self.sender().number()
        self.m_curNumberButton = self.m_frameNumbersList[self.m_curBeat]

    def keyPressEvent(self, event):
        print("keyPressEvent")
        CaitlynAbstractView.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        print("keyReleaseEvent")

        if event.key() == Qt.Key_Up and self.m_curBeat > 0:
            self.m_curBeat -= 1
        elif event.key() == Qt.Key_Down and self.m_curBeat < self.m_beats-1:
            self.m_curBeat += 1
        else:
            CaitlynAbstractView.keyReleaseEvent(self, event)
            return

        self.m_curNumberButton.setChecked(False)
        self.m_curNumberButton = self.m_frameNumbersList[self.m_curBeat]
        self.m_curNumberButton.setChecked(True)

        CaitlynAbstractView.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        print("mousePressEvent")
        CaitlynAbstractView.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        print("mouseMoveEvent")
        CaitlynAbstractView.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        print("mouseReleaseEvent")
        CaitlynAbstractView.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self) # .viewport()

        #paddingTop  = 40
        #paddingLeft = 2
        #eventWidth  = self.m_textWidth
        #eventHeight = self.m_textHeight

        ## Draw fill behind numbers
        #painter.setBrush(self.m_colorTextBg)
        #painter.setPen(self.m_colorTextBg)

        #for i in range(self.m_beats):
            #if i % self.m_beatsPerBar == 0:
                #painter.drawRect(paddingLeft-1, paddingTop - eventHeight + i*eventHeight + eventHeight/6, eventWidth, eventHeight)

        ## Draw numbers
        #painter.setFont(self.m_font)
        #painter.setPen(self.m_colorText)

        #for i in range(self.m_beats):
            #painter.drawText(paddingLeft, paddingTop + i*eventHeight, ("%i" % i) if i > 99 else (" %02i" % i))

        CaitlynAbstractView.paintEvent(self, event)

#class CaitlynTrackerScene(CaitlynAbstractScene):
    #def __init__(self, parent, view):
        #CaitlynAbstractScene.__init__(self, parent, view)

# ------------------------------------------------------------------------------
# Test

if __name__ == '__main__':
    from PyQt4.QtGui import QApplication
    import sys
    # App initialization
    app = QApplication(sys.argv)

    # Show GUI
    view = CaitlynTrackerView(None)
    view.show()

    # App-Loop and exit
    sys.exit(app.exec_())

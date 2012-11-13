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
from PyQt4.QtCore import qFatal, QSettings
from PyQt4.QtCore import QLineF, QPointF, QRectF, QSizeF
from PyQt4.QtGui import QGraphicsItem, QGraphicsScene
from PyQt4.QtGui import QPainter
from PyQt4.QtGui import QApplication, QMainWindow

# Imports (Custom Stuff)
import ui_caitlyn
from shared_settings import *

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------
# Caitlyn Qt Graphics Scene

class CaitlynCanvasScene(QGraphicsScene):
    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_view = view
        if not self.m_view:
            qFatal("CaitlynCanvasScene() - invalid view")

# ------------------------------------------------------------------------------
# Caitlyn items

class CaitlynCanvasTimelineTopBar(QGraphicsItem):
    def __init__(self, scene, parent=None):
        QGraphicsItem.__init__(self, parent, scene)

        # horizontal length, matches min(full length of song, window)
        self.m_length = 100

    def boundingRect(self):
        return QRectF(0, 0, self.m_length*4, 4)

    def paint(self, painter, option, widget):
        painter.setRenderHint(QPainter.Antialiasing, False)

        painter.drawRect(0, 0, self.m_length*14, 16)

class CaitlynCanvasBox(QGraphicsItem):
    def __init__(self, scene, parent=None):
        QGraphicsItem.__init__(self, parent, scene)

        self.m_length = 1.0

    def length(self):
        return self.m_length

    def boundingRect(self):
        return QRectF(0, 0, self.m_length*4, 4)

    def paint(self, painter, option, widget):
        painter.setRenderHint(QPainter.Antialiasing, False)

        painter.drawRect(0, 0, self.m_length*14, 16)

# ------------------------------------------------------------------------------
# Caitlyn Main Window

class CaitlynMainW(QMainWindow, ui_caitlyn.Ui_CaitlynMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Caitlyn")
        self.loadSettings(True)

        self.scene = CaitlynCanvasScene(self, self.graphicsView)
        self.graphicsView.setScene(self.scene)
        self.graphicsView.setRenderHint(QPainter.Antialiasing, False)
        self.graphicsView.setRenderHint(QPainter.TextAntialiasing, False)

        self.item1 = CaitlynCanvasBox(self.scene)

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

        self.m_savedSettings = {
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int)
        }

    def closeEvent(self, event):
        self.scene.removeItem(self.item1)

        self.saveSettings()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Caitlyn")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    #app.setWindowIcon(QIcon(":/scalable/caitlyn.svg"))

    # Show GUI
    gui = CaitlynMainW()

    # Set-up custom signal handling
    setUpSignals(gui)

    gui.show()

    # App-Loop and exit
    sys.exit(app.exec_())

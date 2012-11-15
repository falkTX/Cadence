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
from caitlib_helpers import *
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

        # Sequencer test code
        self.m_seq   = caitlib.init("Caitlyn")
        self.m_port1 = caitlib.create_port(self.m_seq, "out1")

        m = 44

        caitlib.put_control(self.m_seq, self.m_port1, 0*m, 0, 7, 99)
        caitlib.put_control(self.m_seq, self.m_port1, 0*m, 0, 10, 63)
        caitlib.put_control(self.m_seq, self.m_port1, 0*m, 0, 0, 0)

        # 0 PrCh ch=1 p=0     -- TODO jack_midi_put_program()

        # 0 On ch=1 n=64 v=90
        # 325 Off ch=1 n=64 v=90
        # 384 On ch=1 n=62 v=90
        # 709 Off ch=1 n=62 v=90
        # 768 On ch=1 n=60 v=90
        # 1093 Off ch=1 n=60 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,     0*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1,  325*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,   384*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1,  709*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,   768*m, 0, 60, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 1093*m, 0, 60, 90)

        # 1152 On ch=1 n=62 v=90
        # 1477 Off ch=1 n=62 v=90
        # 1536 On ch=1 n=64 v=90
        # 1861 Off ch=1 n=64 v=90
        # 1920 On ch=1 n=64 v=90
        # 2245 Off ch=1 n=64 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  1152*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 1477*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  1536*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 1861*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  1920*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 2245*m, 0, 64, 90)

        # 2304 On ch=1 n=64 v=90
        # 2955 Off ch=1 n=64 v=90
        # 3072 On ch=1 n=62 v=90
        # 3397 Off ch=1 n=62 v=90
        # 3456 On ch=1 n=62 v=90
        # 3781 Off ch=1 n=62 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  2304*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 2955*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  3072*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 3397*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  3456*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 3781*m, 0, 62, 90)

        # 3840 On ch=1 n=62 v=90
        # 4491 Off ch=1 n=62 v=90
        # 4608 On ch=1 n=64 v=90
        # 4933 Off ch=1 n=64 v=90
        # 4992 On ch=1 n=67 v=90
        # 5317 Off ch=1 n=67 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  3840*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 4491*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  4608*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 4933*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  4992*m, 0, 67, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 5317*m, 0, 67, 90)

        # 5376 On ch=1 n=67 v=90
        # 6027 Off ch=1 n=67 v=90
        # 6144 On ch=1 n=64 v=90
        # 6469 Off ch=1 n=64 v=90
        # 6528 On ch=1 n=62 v=90
        # 6853 Off ch=1 n=62 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  5376*m, 0, 67, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 6027*m, 0, 67, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  6144*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 6469*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  6528*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 6853*m, 0, 62, 90)

        # 6912 On ch=1 n=60 v=90
        # 7237 Off ch=1 n=60 v=90
        # 7296 On ch=1 n=62 v=90
        # 7621 Off ch=1 n=62 v=90
        # 7680 On ch=1 n=64 v=90
        # 8005 Off ch=1 n=64 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  6912*m, 0, 60, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 7237*m, 0, 60, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  7296*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 7621*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  7680*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 8005*m, 0, 64, 90)

        # 8064 On ch=1 n=64 v=90
        # 8389 Off ch=1 n=64 v=90
        # 8448 On ch=1 n=64 v=90
        # 9099 Off ch=1 n=64 v=90
        # 9216 On ch=1 n=62 v=90
        # 9541 Off ch=1 n=62 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  8064*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 8389*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  8448*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 9099*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  9216*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 9541*m, 0, 62, 90)

        # 9600 On ch=1 n=62 v=90
        # 9925 Off ch=1 n=62 v=90
        # 9984 On ch=1 n=64 v=90
        # 10309 Off ch=1 n=64 v=90
        # 10368 On ch=1 n=62 v=90
        # 10693 Off ch=1 n=62 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,   9600*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1,  9925*m, 0, 62, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,   9984*m, 0, 64, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 10309*m, 0, 64, 90)
        caitlib.put_note_on(self.m_seq, self.m_port1,  10368*m, 0, 62, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 10693*m, 0, 62, 90)

        # 10752 On ch=1 n=60 v=90
        # 12056 Off ch=1 n=60 v=90
        caitlib.put_note_on(self.m_seq, self.m_port1,  10752*m, 0, 60, 90)
        caitlib.put_note_off(self.m_seq, self.m_port1, 12056*m, 0, 60, 90)

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

        self.m_savedSettings = {
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int)
        }

    def closeEvent(self, event):
        if self.m_seq:
            caitlib.close(self.m_seq)

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

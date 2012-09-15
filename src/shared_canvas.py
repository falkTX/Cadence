#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to PatchCanvas
# Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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
from PyQt4.QtCore import SIGNAL
from PyQt4.QtGui import QFileDialog, QImage, QPainter, QPrinter, QPrintDialog

# Imports (Custom Stuff)
import patchcanvas

# ------------------------------------------------------------------------------------------------------------

# Shared Canvas code
def canvas_arrange():
    patchcanvas.arrange()

def canvas_refresh(self_):
    patchcanvas.clear()
    self_.init_ports()

def canvas_zoom_fit(self_):
    self_.scene.zoom_fit()

def canvas_zoom_in(self_):
    self_.scene.zoom_in()

def canvas_zoom_out(self_):
    self_.scene.zoom_out()

def canvas_zoom_reset(self_):
    self_.scene.zoom_reset()

def canvas_print(self_):
    self_.scene.clearSelection()
    self_.m_export_printer = QPrinter()
    dialog = QPrintDialog(self_.m_export_printer, self_)

    if dialog.exec_():
        painter = QPainter(self_.m_export_printer)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setRenderHint(QPainter.TextAntialiasing)
        self_.scene.render(painter)

def canvas_save_image(self_):
    newPath = QFileDialog.getSaveFileName(self_, self_.tr("Save Image"), filter=self_.tr("PNG Image (*.png);;JPEG Image (*.jpg)"))

    if newPath:
        self_.scene.clearSelection()

        if newPath.endswith((".jpg", ".jpG", ".jPG", ".JPG", ".JPg", ".Jpg")):
            img_format = "JPG"
        elif newPath.endswith((".png", ".pnG", ".pNG", ".PNG", ".PNg", ".Png")):
            img_format = "PNG"
        else:
            # File-dialog may not auto-add the extension
            img_format = "PNG"
            newPath   += ".png"

        self_.m_export_image = QImage(self_.scene.sceneRect().width(), self_.scene.sceneRect().height(), QImage.Format_RGB32)
        painter = QPainter(self_.m_export_image)
        painter.setRenderHint(QPainter.Antialiasing) # TODO - set true, cleanup this
        painter.setRenderHint(QPainter.TextAntialiasing)
        self_.scene.render(painter)
        self_.m_export_image.save(newPath, img_format, 100)

# ------------------------------------------------------------------------------------------------------------

# Shared Connections
def setCanvasConnections(self_):
    self_.act_canvas_arrange.setEnabled(False) # TODO, later
    self_.connect(self_.act_canvas_arrange, SIGNAL("triggered()"), lambda: canvas_arrange())
    self_.connect(self_.act_canvas_refresh, SIGNAL("triggered()"), lambda: canvas_refresh(self_))
    self_.connect(self_.act_canvas_zoom_fit, SIGNAL("triggered()"), lambda: canvas_zoom_fit(self_))
    self_.connect(self_.act_canvas_zoom_in, SIGNAL("triggered()"), lambda: canvas_zoom_in(self_))
    self_.connect(self_.act_canvas_zoom_out, SIGNAL("triggered()"), lambda: canvas_zoom_out(self_))
    self_.connect(self_.act_canvas_zoom_100, SIGNAL("triggered()"), lambda: canvas_zoom_reset(self_))
    self_.connect(self_.act_canvas_print, SIGNAL("triggered()"), lambda: canvas_print(self_))
    self_.connect(self_.act_canvas_save_image, SIGNAL("triggered()"), lambda: canvas_save_image(self_))
    self_.connect(self_.b_canvas_zoom_fit, SIGNAL("clicked()"), lambda: canvas_zoom_fit(self_))
    self_.connect(self_.b_canvas_zoom_in, SIGNAL("clicked()"), lambda: canvas_zoom_in(self_))
    self_.connect(self_.b_canvas_zoom_out, SIGNAL("clicked()"), lambda: canvas_zoom_out(self_))
    self_.connect(self_.b_canvas_zoom_100, SIGNAL("clicked()"), lambda: canvas_zoom_reset(self_))

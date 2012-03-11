#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to PatchCanvas
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
from PyQt4.QtCore import Qt, SIGNAL
from PyQt4.QtGui import QFileDialog, QImage, QPainter, QPrinter, QPrintDialog

# Imports (Custom Stuff)
import patchcanvas

# Shared Canvas code
def canvas_arrange(self):
    patchcanvas.arrange()

def canvas_refresh(self):
    self.init_ports_prepare()
    patchcanvas.clear()
    self.init_ports()

def canvas_zoom_fit(self):
    self.scene.zoom_fit()

def canvas_zoom_in(self):
    self.scene.zoom_in()

def canvas_zoom_out(self):
    self.scene.zoom_out()

def canvas_zoom_reset(self):
    self.scene.zoom_reset()

def canvas_print(self):
    self.scene.clearSelection()
    self.m_export_printer = QPrinter()
    dialog = QPrintDialog(self.m_export_printer, self)
    if (dialog.exec_()):
      painter = QPainter(self.m_export_printer)
      painter.setRenderHint(QPainter.Antialiasing)
      painter.setRenderHint(QPainter.TextAntialiasing)
      self.scene.render(painter)

def canvas_save_image(self):
    newPath = QFileDialog.getSaveFileName(self, self.tr("Save Image"), filter=self.tr("PNG Image (*.png);;JPEG Image (*.jpg)"))
    print(newPath)

    if (newPath):
      self.scene.clearSelection()
      if (newPath.endswith((".jpg", ".jpG", ".jPG", ".JPG", ".JPg", ".Jpg"))):
        img_format = "JPG"
      elif (newPath.endswith((".png", ".pnG", ".pNG", ".PNG", ".PNg", ".Png"))):
        img_format = "PNG"
      else:
        # File-dialog may not auto-add the extension
        img_format = "PNG"
        newPath   += ".png"

      self.m_export_image = QImage(self.scene.sceneRect().width(), self.scene.sceneRect().height(), QImage.Format_RGB32)
      painter = QPainter(self.m_export_image)
      painter.setRenderHint(QPainter.Antialiasing)
      painter.setRenderHint(QPainter.TextAntialiasing)
      self.scene.render(painter)
      self.m_export_image.save(newPath, img_format, 100)

# Shared Connections
def setCanvasConnections(self):
  self.act_canvas_arrange.setEnabled(False)
  self.connect(self.act_canvas_arrange, SIGNAL("triggered()"), lambda: canvas_arrange(self))
  self.connect(self.act_canvas_refresh, SIGNAL("triggered()"), lambda: canvas_refresh(self))
  self.connect(self.act_canvas_zoom_fit, SIGNAL("triggered()"), lambda: canvas_zoom_fit(self))
  self.connect(self.act_canvas_zoom_in, SIGNAL("triggered()"), lambda: canvas_zoom_in(self))
  self.connect(self.act_canvas_zoom_out, SIGNAL("triggered()"), lambda: canvas_zoom_out(self))
  self.connect(self.act_canvas_zoom_100, SIGNAL("triggered()"), lambda: canvas_zoom_reset(self))
  self.connect(self.act_canvas_print, SIGNAL("triggered()"), lambda: canvas_print(self))
  self.connect(self.act_canvas_save_image, SIGNAL("triggered()"), lambda: canvas_save_image(self))
  self.connect(self.b_canvas_zoom_fit, SIGNAL("clicked()"), lambda: canvas_zoom_fit(self))
  self.connect(self.b_canvas_zoom_in, SIGNAL("clicked()"), lambda: canvas_zoom_in(self))
  self.connect(self.b_canvas_zoom_out, SIGNAL("clicked()"), lambda: canvas_zoom_out(self))
  self.connect(self.b_canvas_zoom_100, SIGNAL("clicked()"), lambda: canvas_zoom_reset(self))

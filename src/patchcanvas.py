#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Patchbay Canvas engine using QGraphicsView/Scene
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
from PyQt4.QtCore import pyqtSlot, qDebug, qCritical, qFatal, Qt, QObject, SIGNAL, SLOT
from PyQt4.QtCore import QPointF, QRectF, QSettings
from PyQt4.QtGui import QGraphicsScene, QGraphicsItem

# Imports (Theme)
from patchcanvas_theme import *

PATCHCANVAS_ORGANISATION_NAME = "Cadence"

# ------------------------------------------------------------------------------
# patchcanvas-api.h

# Port Mode
PORT_MODE_NULL   = 0
PORT_MODE_INPUT  = 1
PORT_MODE_OUTPUT = 2

# Port Type
PORT_TYPE_NULL       = 0
PORT_TYPE_AUDIO_JACK = 1
PORT_TYPE_MIDI_JACK  = 2
PORT_TYPE_MIDI_A2J   = 3
PORT_TYPE_MIDI_ALSA  = 4

# Callback Action
ACTION_GROUP_INFO       = 0 # group_id, N, N
ACTION_GROUP_RENAME     = 1 # group_id, N, new_name
ACTION_GROUP_SPLIT      = 2 # group_id, N, N
ACTION_GROUP_JOIN       = 3 # group_id, N, N
ACTION_PORT_INFO        = 4 # port_id, N, N
ACTION_PORT_RENAME      = 5 # port_id, N, new_name
ACTION_PORTS_CONNECT    = 6 # out_id, in_id, N
ACTION_PORTS_DISCONNECT = 7 # conn_id, N, N

# Icon
ICON_HARDWARE    = 0
ICON_APPLICATION = 1
ICON_LADISH_ROOM = 2

# Split Option
SPLIT_UNDEF = 0
SPLIT_NO    = 1
SPLIT_YES   = 2

# Canvas options
class options_t(object):
  __slots__ = [
    'theme_name',
    'bezier_lines',
    'antialiasing',
    'auto_hide_groups',
    'fancy_eyecandy'
  ]

# Canvas features
class features_t(object):
  __slots__ = [
    'group_info',
    'group_rename',
    'port_info',
    'port_rename',
    'handle_group_pos'
  ]

# ------------------------------------------------------------------------------
# patchcanvas.h

# object types
CanvasBoxType           = QGraphicsItem.UserType + 1
CanvasIconType          = QGraphicsItem.UserType + 2
CanvasPortType          = QGraphicsItem.UserType + 3
CanvasLineType          = QGraphicsItem.UserType + 4
CanvasBezierLineType    = QGraphicsItem.UserType + 5
CanvasLineMovType       = QGraphicsItem.UserType + 6
CanvasBezierLineMovType = QGraphicsItem.UserType + 7

# object lists
class group_dict_t(object):
  __slots__ = [
    'group_id',
    'group_name',
    'split',
    'icon',
    'widgets'
  ]

class port_dict_t(object):
  __slots__ = [
    'group_id',
    'port_id',
    'port_name',
    'port_mode',
    'port_type',
    'widget'
  ]

class connection_dict_t(object):
  __slots__ = [
    'connection_id',
    'port_in_id',
    'port_out_id',
    'widget'
  ]

# Main Canvas object
class Canvas(object):
  __slots__ = [
    'scene',
    'callback',
    'debug',
    'last_z_value',
    'last_group_id',
    'last_connection_id',
    'initial_pos',
    'group_list',
    'port_list',
    'connection_list',
    'postponed_groups',
    'qobject',
    'settings',
    'theme',
    'size_rect',
    'initiated'
  ]

# ------------------------------------------------------------------------------
# patchcanvas.cpp

class CanvasObject(QObject):
  def __init__(self, parent):
    QObject.__init__(self, parent)

  @pyqtSlot()
  def CanvasPostponedGroups(self):
    CanvasPostponedGroups()

  @pyqtSlot()
  def PortContextMenuDisconnect(self):
    connection_id_try = self.sender().data().toInt()
    if (connection_id_try[1]):
      CanvasCallback(ACTION_PORTS_DISCONNECT, connection_id_try[0], 0, "")

# Global objects
canvas = Canvas()
canvas.qobject   = None
canvas.settings  = None
canvas.theme     = None
canvas.initiated = False

options = options_t()
options.theme_name        = getThemeName(getDefaultTheme())
options.bezier_lines      = True
options.antialiasing      = Qt.PartiallyChecked
options.auto_hide_groups  = True
options.fancy_eyecandy    = False

features = features_t()
features.group_info       = False
features.group_rename     = True
features.port_info        = False
features.port_rename      = True
features.handle_group_pos = False

# Internal functions
def bool2str(check):
  return "True" if check else "False"

# PatchCanvas API
def set_options(new_options):
  if (canvas.initiated): return
  options.theme_name        = new_options.theme_name
  options.bezier_lines      = new_options.bezier_lines
  options.antialiasing      = new_options.antialiasing
  options.auto_hide_groups  = new_options.auto_hide_groups
  options.fancy_eyecandy    = new_options.fancy_eyecandy

def set_features(new_features):
  if (canvas.initiated): return
  features.group_info       = new_features.group_info
  features.group_rename     = new_features.group_rename
  features.port_info        = new_features.port_info
  features.port_rename      = new_features.port_rename
  features.handle_group_pos = new_features.handle_group_pos

def init(scene, callback, debug=False):
  if (debug):
    qDebug("PatchCanvas::init(%s, %s, %s)" % (scene, callback, bool2str(debug)))

  if (canvas.initiated):
    qCritical("PatchCanvas::init() - already initiated")
    return

  if (not callback):
    qFatal("PatchCanvas::init() - fatal error: callback not set")
    return

  canvas.scene = scene
  canvas.callback = callback
  canvas.debug = debug

  canvas.last_z_value = 0
  canvas.last_group_id = 0
  canvas.last_connection_id = 0
  canvas.initial_pos = QPointF(0, 0)

  canvas.group_list = []
  canvas.port_list = []
  canvas.connection_list = []
  canvas.postponed_groups = []

  if (not canvas.qobject): canvas.qobject = CanvasObject(None)
  if (not canvas.settings): canvas.settings = QSettings(PATCHCANVAS_ORGANISATION_NAME, "PatchCanvas")

  for i in range(Theme.THEME_MAX):
    this_theme_name = getThemeName(i)
    if (this_theme_name == options.theme_name):
      canvas.theme = Theme(i)
      break
  else:
    canvas.theme = Theme(getDefaultTheme())

  canvas.size_rect = QRectF()

  canvas.scene.rubberbandByTheme()
  canvas.scene.setBackgroundBrush(canvas.theme.canvas_bg)

  canvas.initiated = True

def clear():
  if (canvas.debug):
    qDebug("patchcanvas::clear()")

  group_list_ids = []
  port_list_ids  = []
  connection_list_ids = []

  for i in range(len(canvas.group_list)):
    group_list_ids.append(canvas.group_list[i].group_id)

  for i in range(len(canvas.port_list)):
    port_list_ids.append(canvas.port_list[i].port_id)

  for i in range(len(canvas.connection_list)):
    connection_list_ids.append(canvas.connection_list[i].connection_id)

  for idx in connection_list_ids:
    disconnectPorts(idx)

  for idx in port_list_ids:
    removePort(tmp_port_list[i])

  for idx in group_list_ids:
    removeGroup(idx)

  canvas.last_z_value = 0
  canvas.last_group_id = 0
  canvas.last_connection_id = 0

  canvas.group_list = []
  canvas.port_list = []
  canvas.connection_list = []
  canvas.postponed_groups = []

  canvas.initiated = False

def setInitialPos(x, y):
  if (canvas.debug):
    qDebug("patchcanvas::setInitialPos(%i, %i)" % (x, y))

  canvas.initial_pos.setX(x)
  canvas.initial_pos.setY(y)

def setCanvasSize(x, y, width, height):
  if (canvas.debug):
    qDebug("patchcanvas::setCanvasSize(%i, %i, %i, %i)" % (x, y, width, height))

  canvas.size_rect.setX(x)
  canvas.size_rect.setY(y)
  canvas.size_rect.setWidth(width)
  canvas.size_rect.setHeight(height)

# ------------------------------------------------------------------------------
# patchscene.cpp

class PatchScene(QGraphicsScene):
    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_ctrl_down = False
        self.m_mouse_down_init  = False
        self.m_mouse_rubberband = False

        self.m_rubberband = self.addRect(QRectF(0, 0, 0, 0))
        self.m_rubberband.setZValue(-1)
        self.m_rubberband.hide()
        self.m_rubberband_selection  = False
        self.m_rubberband_orig_point = QPointF(0, 0)

        self.m_view = view
        if (not self.m_view):
          qFatal("PatchCanvas::PatchScene() - Invalid view")

    def fixScaleFactor(self):
        scale = self.m_view.transform().m11()
        if (scale > 3.0):
          self.m_view.resetTransform()
          self.m_view.scale(3.0, 3.0)
        elif (scale < 0.2):
          self.m_view.resetTransform()
          self.m_view.scale(0.2, 0.2)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def rubberbandByTheme(self):
        self.m_rubberband.setPen(canvas.theme.rubberband_pen)
        self.m_rubberband.setBrush(canvas.theme.rubberband_brush)

    def zoom_fit(self):
      min_x = min_y = max_x = max_y = None
      items_list = self.items()

      if (len(items_list) > 0):
        for i in range(len(items_list)):
          if (items_list[i].isVisible() and items_list[i].type() == CanvasBoxType):
            pos  = items_list[i].scenePos()
            rect = items_list[i].boundingRect()

            if (min_x == None):
              min_x = pos.x()
            elif (pos.x() < min_x):
              min_x = pos.x()

            if (min_y == None):
              min_y = pos.y()
            elif (pos.y() < min_y):
              min_y = pos.y()

            if (max_x == None):
              max_x = pos.x()+rect.width()
            elif (pos.x()+rect.width() > max_x):
              max_x = pos.x()+rect.width()

            if (max_y == None):
              max_y = pos.y()+rect.height()
            elif (pos.y()+rect.height() > max_y):
              max_y = pos.y()+rect.height()

        self.m_view.fitInView(min_x, min_y, abs(max_x-min_x), abs(max_y-min_y), Qt.KeepAspectRatio)
        self.fixScaleFactor()

    def zoom_in(self):
        if (self.m_view.transform().m11() < 3.0):
          self.m_view.scale(1.2, 1.2)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def zoom_out(self):
        if (self.m_view.transform().m11() > 0.2):
          self.m_view.scale(0.8, 0.8)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def zoom_reset(self):
        self.m_view.resetTransform()
        self.emit(SIGNAL("scaleChanged(double)"), 1.0)

    def keyPressEvent(self, event):
        if (not self.m_view):
          event.reject()

        if (event.key() == Qt.Key_Control):
          self.m_ctrl_down = True

        if (event.key() == Qt.Key_Home):
          self.zoom_fit()
          event.accept()

        elif (self.m_ctrl_down):
          if (event.key() == Qt.Key_Plus):
            self.zoom_in()
            event.accept()
          elif (event.key() == Qt.Key_Minus):
            self.zoom_out()
            event.accept()
          elif (event.key() == Qt.Key_1):
            self.zoom_reset()
            event.accept()
          else:
            QGraphicsScene.keyPressEvent(self, event)

        else:
          QGraphicsScene.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if (event.key() == Qt.Key_Control):
          self.m_ctrl_down = False
        QGraphicsScene.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        if (event.button() == Qt.LeftButton):
          self.m_mouse_down_init = True
        else:
          self.m_mouse_down_init = False
        self.m_mouse_rubberband = False
        QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if (self.m_mouse_down_init):
          self.m_mouse_rubberband = (len(self.selectedItems()) == 0)
          self.m_mouse_down_init  = False

        if (self.m_mouse_rubberband):
          if (self.m_rubberband_selection == False):
            self.m_rubberband.show()
            self.m_rubberband_selection  = True
            self.m_rubberband_orig_point = event.scenePos()

          pos = event.scenePos()

          if (pos.x() > self.m_rubberband_orig_point.x()):
            x = self.m_rubberband_orig_point.x()
          else:
            x = pos.x()

          if (pos.y() > self.m_rubberband_orig_point.y()):
            y = self.m_rubberband_orig_point.y()
          else:
            y = pos.y()

          self.m_rubberband.setRect(x, y, abs(pos.x()-self.m_rubberband_orig_point.x()), abs(pos.y()-self.m_rubberband_orig_point.y()))

          event.accept()

        else:
          QGraphicsScene.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if (self.m_rubberband_selection):
          items_list = self.items()
          if (len(items_list) > 0):
            for item in items_list:
              if (item.isVisible() and item.type() == CanvasBoxType):
                item_rect         = item.sceneBoundingRect()
                item_top_left     = QPointF(item_rect.x(), item_rect.y())
                item_bottom_right = QPointF(item_rect.x()+item_rect.width(), item_rect.y()+item_rect.height())

                if (self.m_rubberband.contains(item_top_left) and self.m_rubberband.contains(item_bottom_right)):
                  item.setSelected(True)

            self.m_rubberband.hide()
            self.m_rubberband.setRect(0, 0, 0, 0)
            self.m_rubberband_selection = False

        else:
          items_list = self.selectedItems()
          for item in items_list:
            if (item.isVisible() and item.type() == CanvasBoxType):
              item.checkItemPos()
              self.emit(SIGNAL("sceneGroupMoved(int, int, QPointF)"), item.getGroupId(), item.getSplittedMode(), item.scenePos())

        self.m_mouse_down_init  = False
        self.m_mouse_rubberband = False
        QGraphicsScene.mouseReleaseEvent(self, event)

    def wheelEvent(self, event):
        if (not self.m_view):
          event.reject()

        if (self.m_ctrl_down):
          factor = 1.41 ** (event.delta()/240.0)
          self.m_view.scale(factor, factor)

          self.fixScaleFactor()
          event.accept()

        else:
          QGraphicsScene.wheelEvent(self, event)

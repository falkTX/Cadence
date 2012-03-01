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
from PyQt4.QtCore import QPointF, QRectF, QSettings, QTimer
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

def split2str(split):
  if (split == SPLIT_UNDEF):
    return "SPLIT_UNDEF"
  elif (split == SPLIT_NO):
    return "SPLIT_NO"
  elif (split == SPLIT_YES):
    return "SPLIT_YES"
  else:
    return "SPLIT_???"

def icon2str(icon):
  if (icon == ICON_HARDWARE):
    return "ICON_HARDWARE"
  elif (ICON_APPLICATION):
    return "ICON_APPLICATION"
  elif (ICON_LADISH_ROOM):
    return "ICON_LADISH_ROOM"
  else:
    return "ICON_???"

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
    removePort(idx)

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

def addGroup(group_id, group_name, split=SPLIT_UNDEF, icon=ICON_APPLICATION):
  if (canvas.debug):
    qDebug("patchcanvas::addGroup(%i, %s, %s, %s)" % (group_id, group_name, split2str(split), icon2str(icon)))

  #if (split == SPLIT_UNDEF and features.handle_group_pos):
    #split = canvas.settings.value("CanvasPositions/%s_SPLIT" % (group_name), split, type=int)

  #group_box = CanvasBox(group_id, group_name, icon)

  #group_dict = group_dict_t()
  #group_dict.group_id   = group_id
  #group_dict.group_name = group_name
  #group_dict.split = bool(split == SPLIT_YES)
  #group_dict.icon  = icon
  #group_dict.widgets = (group_box, None)

  #if (split == SPLIT_YES):
    #group_box.setSplit(True, PORT_MODE_OUTPUT)

    #if (features.handle_group_pos):
      #group_box.setPos(canvas.settings.value(QString("CanvasPositions/%1_o").arg(group_name), CanvasGetNewGroupPos()).toPointF())
    #else:
      #group_box.setPos(CanvasGetNewGroupPos())

    #group_sbox = CanvasBox(group_id, group_name, icon)
    #group_sbox.setSplit(True, PORT_MODE_INPUT)

    #group_dict.widgets[1] = group_sbox

    #if (features.handle_group_pos):
      #group_sbox.setPos(canvas.settings.value(QString("CanvasPositions/%1_i").arg(group_name), CanvasGetNewGroupPos(True)).toPointF())
    #else:
      #group_sbox.setPos(CanvasGetNewGroupPos(True))

    #if (not options.auto_hide_groups and options.fancy_eyecandy):
      #ItemFX(group_sbox, True)

    #canvas.last_z_value += 1
    #group_sbox.setZValue(canvas.last_z_value)

  #else:
    #group_box.setSplit(False)

    #if (features.handle_group_pos):
      #group_box.setPos(canvas.settings.value(QString("CanvasPositions/%1").arg(group_name), CanvasGetNewGroupPos()).toPointF())
    #else:
      ## Special ladish fake-split groups
      #horizontal = (icon == ICON_HARDWARE or icon == ICON_LADISH_ROOM)
      #group_box.setPos(CanvasGetNewGroupPos(horizontal))

  #if (not options.auto_hide_groups and options.fancy_eyecandy):
    #ItemFX(group_box, True)

  #canvas.last_z_value += 1
  #group_box.setZValue(canvas.last_z_value)

  #canvas.group_list.append(group_dict)

  #QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def removeGroup(group_id):
  if (canvas.debug):
    qDebug("patchcanvas::removeGroup(%i)" % (group_id))

  #if (CanvasGetGroupPortCount(group_id) > 0):
    #if (canvas.debug):
      #qDebug("patchcanvas::removeGroup() - This group still has ports, postpone it's removal")
    #canvas.postponed_groups.append(group_id)
    #QTimer.singleShot(100, canvas.qobject, SLOT("CanvasPostponedGroups()"))
    #return

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #item = canvas.group_list[i].widgets[0]
      #group_name = canvas.group_list[i].group_name

      #if (canvas.group_list[i].split):
        #s_item = canvas.group_list[i].widgets[1]

        #if (features.handle_group_pos):
          #canvas.settings.setValue(QString("CanvasPositions/%1_o").arg(group_name), item.pos())
          #canvas.settings.setValue(QString("CanvasPositions/%1_i").arg(group_name), s_item.pos())
          #canvas.settings.setValue(QString("CanvasPositions/%1_s").arg(group_name), SPLIT_YES)

        #if (options.fancy_eyecandy and s_item.isVisible()):
          #ItemFX(s_item, False)
        #else:
          #s_item.removeIconFromScene()
          #canvas.scene.removeItem(s_item)

      #else:
        #if (features.handle_group_pos):
          #canvas.settings.setValue(QString("CanvasPositions/%1").arg(group_name), item.pos())
          #canvas.settings.setValue(QString("CanvasPositions/%1_s").arg(group_name), SPLIT_NO)

      #if (options.fancy_eyecandy and item.isVisible()):
        #ItemFX(item, False)
      #else:
        #item.removeIconFromScene()
        #canvas.scene.removeItem(item)

      #canvas.group_list.pop(i)

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::removeGroup() - Unable to find group to remove")

def renameGroup(group_id, new_group_name):
  if (canvas.debug):
    qDebug("patchcanvas::renameGroup(%i, %s)" % (group_id, new_group_name))

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #canvas.group_list[i].widgets[0].setGroupName(new_group_name)
      #canvas.group_list[i].group_name = new_group_name

      #if (canvas.group_list[i].split):
        #canvas.group_list[i].widgets[1].setGroupName(new_group_name)

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::renameGroup() - Unable to find group to rename")

def splitGroup(group_id):
  if (canvas.debug):
    qDebug("patchcanvas::splitGroup(%i)" % (group_id))

  #item = None
  #group_name = ""
  #group_icon = ICON_APPLICATION
  #ports_data = []
  #conns_data = []

  ## Step 1 - Store all Item data
  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #if (canvas.group_list[i].split):
        #qCritical("patchcanvas::splitGroup() - group is already splitted")
        #return

      #item = canvas.group_list[i].widgets[0]
      #group_name = canvas.group_list[i].group_name
      #group_icon = canvas.group_list[i].icon
      #break

  #if (not item):
    #qCritical("PatchCanvas::splitGroup() - Unable to find group to split")
    #return

  #port_list_ids = list(item.getPortList())

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id in port_list_ids):
      #port_dict = port_dict_t()
      #port_dict.group_id  = canvas.port_list[i].group_id
      #port_dict.port_id   = canvas.port_list[i].port_id
      #port_dict.port_name = canvas.port_list[i].port_name
      #port_dict.port_mode = canvas.port_list[i].port_mode
      #port_dict.port_type = canvas.port_list[i].port_type
      #port_dict.widget    = None
      #ports_data.append(port_dict)

  #for i in range(len(canvas.connection_list)):
    #if (canvas.connection_list[i].port_out_id in port_list_ids or canvas.connection_list[i].port_in_id in port_list_ids):
      #conns_data.append(canvas.connection_list[i])

  ## Step 2 - Remove Item and Children
  #for i in range(len(conns_data)):
    #disconnectPorts(conns_data[i].connection_id)

  #for i in range(len(port_list_ids)):
    #removePort(port_list_ids[i])

  #removeGroup(group_id)

  ## Step 3 - Re-create Item, now splitted
  #addGroup(group_id, group_name, SPLIT_YES, group_icon)

  #for i in range(len(ports_data)):
    #addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type)

  #for i in range(len(conns_data)):
    #connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id)

  #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))

def joinGroup(group_id):
  if (canvas.debug):
    qDebug("patchcanvas::joinGroup(%i)" % (group_id))

  #item = None
  #s_item = None
  #group_name = ""
  #group_icon = ICON_APPLICATION
  #ports_data = []
  #conns_data = []

  ## Step 1 - Store all Item data
  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #if (not canvas.group_list[i].split):
        #qCritical("patchcanvas::joinGroup() - group is not splitted")
        #return

      #item   = canvas.group_list[i].widgets[0]
      #s_item = canvas.group_list[i].widgets[1]
      #group_name = canvas.group_list[i].group_name
      #group_icon = canvas.group_list[i].icon
      #break
  #else:
    #qCritical("patchcanvas::joinGroup() - Unable to find groups to join")
    #return

  #port_list_ids = list(item.getPortList())
  #port_list_idss = s_item.getPortList()
  #for i in range(len(port_list_idss)):
    #if (port_list_idss[i] not in port_list_ids):
      #port_list_ids.append(port_list_idss[i])

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id in port_list_ids):
      #port_dict = port_dict_t()
      #port_dict.group_id  = canvas.port_list[i].group_id
      #port_dict.port_id   = canvas.port_list[i].port_id
      #port_dict.port_name = canvas.port_list[i].port_name
      #port_dict.port_mode = canvas.port_list[i].port_mode
      #port_dict.port_type = canvas.port_list[i].port_type
      #port_dict.widget    = None
      #ports_data.append(port_dict)

  #for i in range(len(canvas.connection_list)):
    #if (canvas.connection_list[i].port_out_id in port_list_ids or canvas.connection_list[i].port_in_id in port_list_ids):
      #conns_data.append(canvas.connection_list[i])

  ## Step 2 - Remove Item and Children
  #for i in range(len(conns_data)):
    #disconnectPorts(conns_data[i].connection_id)

  #for i in range(len(port_list_ids)):
    #removePort(port_list_ids[i])

  #removeGroup(group_id)

  ## Step 3 - Re-create Item, now together
  #addGroup(group_id, group_name, SPLIT_NO, group_icon)

  #for i in range(len(ports_data)):
    #addPort(group_id, ports_data[i].port_id, ports_data[i].port_name, ports_data[i].port_mode, ports_data[i].port_type)

  #for i in range(len(conns_data)):
    #connectPorts(conns_data[i].connection_id, conns_data[i].port_out_id, conns_data[i].port_in_id)

  #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))

def getGroupPos(group_id, port_mode=PORT_MODE_OUTPUT):
  if (canvas.debug):
    qDebug("patchcanvas::getGroupPos(%i, %i)" % (group_id, port_mode))

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #if (canvas.group_list[i].split):
        #if (port_mode == PORT_MODE_OUTPUT):
          #return canvas.group_list[i].widgets[0].pos()
        #elif (port_mode == PORT_MODE_INPUT):
          #return canvas.group_list[i].widgets[1].pos()
        #else:
          #return QPointF(0,0)
      #else:
        #return canvas.group_list[i].widgets[0].pos()

  #qCritical("patchcanvas::getGroupPos() - Unable to find group")

def setGroupPos(group_id, group_pos_x, group_pos_y):
  setGroupPos(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y)

def setGroupPos(group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i):
  if (canvas.debug):
    qDebug("patchcanvas::setGroupPos(%i, %i, %i, %i, %i)" % (group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #canvas.group_list[i].widgets[0].setPos(group_pos_x_o, group_pos_y_o)

      #if (canvas.group_list[i].split):
        #canvas.group_list[i].widgets[1].setPos(group_pos_x_i, group_pos_y_i)

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::setGroupPos() - Unable to find group to reposition")

def setGroupIcon(group_id, icon):
  if (canvas.debug):
    qDebug("patchcanvas::setGroupIcon(%i, %s)" % (group_id, icon2str(icon)))

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #canvas.group_list[i].icon = icon
      #canvas.group_list[i].widgets[0].setIcon(icon)

      #if (canvas.group_list[i].split):
        #canvas.group_list[i].widgets[1].setIcon(icon)

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::setGroupIcon() - Unable to find group to change icon")

def addPort(group_id, port_id, port_name, port_mode, port_type):
  if (canvas.debug):
    qDebug("patchcanvas::addPort(%i, %i, %s, %i, %i)" % (group_id, port_id, port_name, port_mode, port_type))

  #box_widget = None
  #port_widget = None

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #n_widget = 0
      #if (canvas.group_list[i].split and canvas.group_list[i].widgets[0].getSplittedMode() != port_mode):
        #n_widget = 1
      #box_widget = canvas.group_list[i].widgets[n_widget]
      #port_widget = box_widget.addPortFromGroup(port_id, port_mode, port_type, port_name)
      #break

  #if (not box_widget or not port_widget):
    #qCritical("patchcanvas::addPort() - Unable to find parent group")
    #return

  #if (options.fancy_eyecandy):
    #ItemFX(port_widget, True)

  #port_dict = port_dict_t()
  #port_dict.group_id  = group_id
  #port_dict.port_id   = port_id
  #port_dict.port_name = port_name
  #port_dict.port_mode = port_mode
  #port_dict.port_type = port_type
  #port_dict.widget    = port_widget
  #canvas.port_list.append(port_dict)

  #box_widget.updatePositions()

  #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))

def removePort(port_id):
  if (canvas.debug):
    qDebug("patchcanvas::removePort(%i)" % (port_id))

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_id):
      #item = canvas.port_list[i].widget
      #item.parentItem().removePortFromGroup(port_id)
      #if (options.fancy_eyecandy):
        #ItemFX(item, False, True)
      #else:
        #canvas.scene.removeItem(item)
      #canvas.port_list.pop(i)

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::removePort() - Unable to find port to remove")

def renamePort(port_id, new_port_name):
  if (canvas.debug):
    qDebug("patchcanvas::renamePort(%i, %s)" % (port_id, new_port_name))

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_id):
      #canvas.port_list[i].port_name = new_port_name
      #canvas.port_list[i].widget.setPortName(new_port_name)
      #canvas.port_list[i].widget.parentItem().updatePositions()

      #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))
      #return

  #qCritical("patchcanvas::renamePort() - Unable to find port to rename")

def connectPorts(connection_id, port_out_id, port_in_id):
  if (canvas.debug):
    qDebug("patchcanvas::connectPorts(%i, %i, %i)" % (connection_id, port_out_id, port_in_id))

  #port_out = None
  #port_in  = None
  #port_out_parent = None
  #port_in_parent  = None

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_out_id):
      #port_out = canvas.port_list[i].widget
      #port_out_parent = port_out.parentItem()
    #elif (canvas.port_list[i].port_id == port_in_id):
      #port_in = canvas.port_list[i].widget
      #port_in_parent = port_in.parentItem()

  #if (not port_out or not port_in):
    #qCritical("patchcanvas::connectPorts() - Unable to find ports to connect")
    #return

  #connection_dict = connection_dict_t()
  #connection_dict.connection_id = connection_id
  #connection_dict.port_out_id = port_out_id
  #connection_dict.port_in_id  = port_in_id

  #if (options.bezier_lines):
    #connection_dict.widget = CanvasBezierLine(port_out, port_in, None)
  #else:
    #connection_dict.widget = CanvasLine(port_out, port_in, None)

  #port_out_parent.addLineFromGroup(connection_dict.widget, connection_id)
  #port_in_parent.addLineFromGroup(connection_dict.widget, connection_id)

  #canvas.last_z_value += 1
  #port_out_parent.setZValue(canvas.last_z_value)
  #port_in_parent.setZValue(canvas.last_z_value)

  #canvas.last_z_value += 1
  #connection_dict.widget.setZValue(canvas.last_z_value)

  #canvas.connection_list.append(connection_dict)

  #if (options.fancy_eyecandy):
    #ItemFX(connection_dict.widget, True)

  #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))

def disconnectPorts(connection_id):
  if (canvas.debug):
    qDebug("patchcanvas::disconnectPorts(%i)" % (connection_id))

  #port_1_id = port_2_id = 0
  #line  = None
  #item1 = None
  #item2 = None

  #for i in range(len(canvas.connection_list)):
    #if (canvas.connection_list[i].connection_id == connection_id):
      #port_1_id = canvas.connection_list[i].port_out_id
      #port_2_id = canvas.connection_list[i].port_in_id
      #line = canvas.connection_list[i].widget
      #canvas.connection_list.pop(i)
      #break

  #if (not line):
    #qCritical("patchcanvas::disconnectPorts - Unable to find connection ports")
    #return

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_1_id):
      #item1 = canvas.port_list[i].widget
      #break
  #else:
    #qCritical("patchcanvas::disconnectPorts - Unable to find output port")
    #return

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_2_id):
      #item2 = canvas.port_list[i].widget
      #break
  #else:
    #qCritical("patchcanvas::disconnectPorts - Unable to find input port")
    #return

  #item1.parentItem().removeLineFromGroup(connection_id)
  #item2.parentItem().removeLineFromGroup(connection_id)

  #if (options.fancy_eyecandy):
    #ItemFX(line, False, True)
  #else:
    #canvas.scene.removeItem(line)

  #QTimer.singleShot(0, canvas.scene, SIGNAL("update()"))

def Arrange():
  if (canvas.debug):
    qDebug("PatchCanvas::Arrange()")

# Extra Internal functions

def CanvasGetGroupName(group_id):
  if (canvas.debug):
    qDebug("PatchCanvas::CanvasGetGroupName(%i)", group_id)

  #for i in range(len(canvas.group_list)):
    #if (canvas.group_list[i].group_id == group_id):
      #return canvas.group_list[i].group_name

  #qCritical("PatchCanvas::CanvasGetGroupName() - unable to find group")
  return ""

def CanvasGetGroupPortCount(group_id):
  if (canvas.debug):
    qDebug("patchcanvas::CanvasGetGroupPortCount(%i)" % (group_id))

  port_count = 0
  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].group_id == group_id):
      #port_count += 1

  return port_count

def CanvasGetNewGroupPos(horizontal=False):
  if (canvas.debug):
    qDebug("patchcanvas::CanvasGetNewGroupPos(%s)" % (bool2str(horizontal)))

  new_pos = QPointF(canvas.initial_pos.x(), canvas.initial_pos.y())
  #items = canvas.scene.items()

  #break_loop = False
  #while (not break_loop):
    #break_for = False
    #for i in range(len(items)):
      #if (items[i].type() == CanvasBoxType):
        #if (items[i].sceneBoundingRect().contains(new_pos)):
          #if (horizontal):
            #new_pos += QPointF(items[i].boundingRect().width()+15, 0)
          #else:
            #new_pos += QPointF(0, items[i].boundingRect().height()+15)
          #break_for = True
          #break

      #if (i >= len(items)-1 and not break_for):
        #break_loop = True

  return new_pos

def CanvasGetPortName(port_id):
  if (canvas.debug):
    qDebug("PatchCanvas::CanvasGetPortName(%i)" % (port_id))

  #for i in range(len(canvas.port_list)):
    #if (canvas.port_list[i].port_id == port_id):
      #group_id = canvas.port_list[i].group_id
      #for j in range(len(canvas.group_list)):
        #if (canvas.group_list[j].group_id == group_id):
          #return canvas.group_list[j].group_name + ":" + canvas.port_list[i].port_name
      #break

  #qCritical("PatchCanvas::CanvasGetPortName() - unable to find port")
  return ""

def CanvasGetPortConnectionList(port_id):
  if (canvas.debug):
    qDebug("patchcanvas::CanvasGetPortConnectionList(%i)" % (port_id))

  port_con_list = []

  #for i in range(len(canvas.connection_list)):
    #if (canvas.connection_list[i].port_out_id == port_id or canvas.connection_list[i].port_in_id == port_id):
      #port_con_list.append(canvas.connection_list[i].connection_id)

  return port_con_list

def CanvasGetConnectedPort(connection_id, port_id):
  if (canvas.debug):
    qDebug("PatchCanvas::CanvasGetConnectedPort(%i, %i)" % (connection_id, port_id))

  #for i in range(len(canvas.connection_list)):
    #if (canvas.connection_list[i].connection_id == connection_id):
      #if (canvas.connection_list[i].port_out_id == port_id):
        #return canvas.connection_list[i].port_in_id
      #else:
        #return canvas.connection_list[i].port_out_id

  #qCritical("PatchCanvas::CanvasGetConnectedPort() - unable to find connection")
  return 0

def CanvasPostponedGroups():
  if (canvas.debug):
    qDebug("patchcanvas::CanvasPostponedGroups()")

  #for i in range(len(canvas.postponed_groups)):
    #group_id = canvas.postponed_groups[i]

    #for j in range(len(canvas.group_list)):
      #if (canvas.group_list[j].group_id == group_id):
        #item = canvas.group_list[j].widgets[0]
        #s_item = None

        #if (canvas.group_list[j].split):
          #s_item = canvas.group_list[j].widgets[1]

        #if (item.getPortCount() == 0 and (not s_item or s_item.getPortCount() == 0)):
          #removeGroup(group_id)
          #canvas.postponed_groups.pop(i)

        #break

  #if (len(canvas.postponed_groups) > 0):
    #QTimer.singleShot(100, canvas.qobject, SLOT("CanvasPostponedGroups()"));

def CanvasCallback(action, value1, value2, value_str):
  if (canvas.debug):
    qDebug("PatchCanvas::CanvasCallback(%i, %i, %i, %s)" % (action, value1, value2, QStringStr(value_str)))

  canvas.callback(action, value1, value2, value_str);

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

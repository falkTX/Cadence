#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code
# Copyright (C) 2010-2012 Filipe Coelho <falktx@gmail.com>
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
import os, sys
from unicodedata import normalize
from PyQt4.QtCore import qWarning, SIGNAL, SLOT
from PyQt4.QtGui import QIcon, QMessageBox, QFileDialog

# Set Platform
if (sys.platform == "darwin"):
  from PyQt4.QtGui import qt_mac_set_menubar_icons
  qt_mac_set_menubar_icons(False)
  LINUX   = False
  WINDOWS = False
elif ("linux" in sys.platform):
  LINUX   = True
  WINDOWS = False
elif ("win" in sys.platform):
  LINUX   = False
  WINDOWS = True
else:
  LINUX   = False
  WINDOWS = False

if (not WINDOWS):
  from signal import signal, SIGINT, SIGTERM, SIGUSR1, SIGUSR2

# Set Version
VERSION = "0.5.0"

# Set Debug mode
DEBUG = True

# Global variables
global x_gui
x_gui = None

# Small integrity tests
HOME = os.getenv("HOME")
if (HOME == None):
  qWarning("HOME variable not set")
  HOME = "/tmp"
elif (not os.path.exists(HOME)):
  qWarning("HOME variable set but not valid")
  HOME = "/tmp"

PATH_env = os.getenv("PATH")
if (PATH_env == None):
  qWarning("PATH variable not set")
  PATH = ("/bin", "/sbin", "/usr/local/bin", "/usr/local/sbin", "/usr/bin", "/usr/sbin", "/usr/games")
else:
  PATH = PATH_env.split(os.pathsep)
del PATH_env

MIDI_CC_LIST = (
  #"0x00 Bank Select",
  "0x01 Modulation",
  "0x02 Breath",
  "0x03 (Undefined)",
  "0x04 Foot",
  "0x05 Portamento",
  #"0x06 (Data Entry MSB)",
  "0x07 Volume",
  "0x08 Balance",
  "0x09 (Undefined)",
  "0x0A Pan",
  "0x0B Expression",
  "0x0C FX Control 1",
  "0x0D FX Control 2",
  "0x0E (Undefined)",
  "0x0F (Undefined)",
  "0x10 General Purpose 1",
  "0x11 General Purpose 2",
  "0x12 General Purpose 3",
  "0x13 General Purpose 4",
  "0x14 (Undefined)",
  "0x15 (Undefined)",
  "0x16 (Undefined)",
  "0x17 (Undefined)",
  "0x18 (Undefined)",
  "0x19 (Undefined)",
  "0x1A (Undefined)",
  "0x1B (Undefined)",
  "0x1C (Undefined)",
  "0x1D (Undefined)",
  "0x1E (Undefined)",
  "0x1F (Undefined)",
  #"0x20 *Bank Select",
  #"0x21 *Modulation",
  #"0x22 *Breath",
  #"0x23 *(Undefined)",
  #"0x24 *Foot",
  #"0x25 *Portamento",
  #"0x26 *(Data Entry MSB)",
  #"0x27 *Volume",
  #"0x28 *Balance",
  #"0x29 *(Undefined)",
  #"0x2A *Pan",
  #"0x2B *Expression",
  #"0x2C *FX *Control 1",
  #"0x2D *FX *Control 2",
  #"0x2E *(Undefined)",
  #"0x2F *(Undefined)",
  #"0x30 *General Purpose 1",
  #"0x31 *General Purpose 2",
  #"0x32 *General Purpose 3",
  #"0x33 *General Purpose 4",
  #"0x34 *(Undefined)",
  #"0x35 *(Undefined)",
  #"0x36 *(Undefined)",
  #"0x37 *(Undefined)",
  #"0x38 *(Undefined)",
  #"0x39 *(Undefined)",
  #"0x3A *(Undefined)",
  #"0x3B *(Undefined)",
  #"0x3C *(Undefined)",
  #"0x3D *(Undefined)",
  #"0x3E *(Undefined)",
  #"0x3F *(Undefined)",
  #"0x40 Damper On/Off", # <63 off, >64 on
  #"0x41 Portamento On/Off", # <63 off, >64 on
  #"0x42 Sostenuto On/Off", # <63 off, >64 on
  #"0x43 Soft Pedal On/Off", # <63 off, >64 on
  #"0x44 Legato Footswitch", # <63 Normal, >64 Legato
  #"0x45 Hold 2", # <63 off, >64 on
  "0x46 Control 1 [Variation]",
  "0x47 Control 2 [Timbre]",
  "0x48 Control 3 [Release]",
  "0x49 Control 4 [Attack]",
  "0x4A Control 5 [Brightness]",
  "0x4B Control 6 [Decay]",
  "0x4C Control 7 [Vib Rate]",
  "0x4D Control 8 [Vib Depth]",
  "0x4E Control 9 [Vib Delay]",
  "0x4F Control 10 [Undefined]",
  "0x50 General Purpose 5",
  "0x51 General Purpose 6",
  "0x52 General Purpose 8",
  "0x53 General Purpose 9",
  "0x54 Portamento Control",
  "0x5B FX 1 Depth [Reverb]",
  "0x5C FX 2 Depth [Tremolo]",
  "0x5D FX 3 Depth [Chorus]",
  "0x5E FX 4 Depth [Detune]",
  "0x5F FX 5 Depth [Phaser]"
  )

# Check if an object is a number (float support)
def isNumber(value):
  try:
    float(value)
    return True
  except:
    return False

# Convert a value to a list
def toList(value):
    if value is None:
      return []
    elif not isinstance(value, list):
      return [value]
    else:
      return value

# Convert a ctypes char_p to a string
def toString(value):
    if (value):
      return value.decode("utf-8", errors="ignore")
    else:
      return ""

# Remove/convert non-ascii chars from a string
def unicode2ascii(string):
  return normalize('NFKD', string).encode("ascii", "ignore").decode("utf-8")

# Get Icon from user theme, using our own as backup (Oxygen)
def getIcon(icon, size=16):
  return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# QLineEdit and QPushButtom combo
def getAndSetPath(self_, currentPath, lineEdit):
    newPath = QFileDialog.getExistingDirectory(self_, self_.tr("Set Path"), currentPath, QFileDialog.ShowDirsOnly)
    if (newPath):
      lineEdit.setText(newPath)
    return newPath

# Custom MessageBox
def CustomMessageBox(self_, icon, title, text, extraText="", buttons=QMessageBox.Yes|QMessageBox.No, defButton=QMessageBox.No):
    msgBox = QMessageBox(self_)
    msgBox.setIcon(icon)
    msgBox.setWindowTitle(title)
    msgBox.setText(text)
    msgBox.setInformativeText(extraText)
    msgBox.setStandardButtons(buttons)
    msgBox.setDefaultButton(defButton)
    return msgBox.exec_()

# signal handler for unix systems
def set_up_signals(_gui):
    if (WINDOWS):
      return

    global x_gui
    x_gui = _gui
    signal(SIGINT,  signal_handler)
    signal(SIGTERM, signal_handler)
    signal(SIGUSR1, signal_handler)
    signal(SIGUSR2, signal_handler)

    x_gui.connect(x_gui, SIGNAL("SIGUSR2()"), lambda gui=x_gui: showWindow(gui))
    x_gui.connect(x_gui, SIGNAL("SIGTERM()"), SLOT("close()"))

def signal_handler(sig, frame):
  global x_gui
  if (sig in (SIGINT, SIGTERM)):
    x_gui.emit(SIGNAL("SIGTERM()"))
  elif (sig == SIGUSR1):
    x_gui.emit(SIGNAL("SIGUSR1()"))
  elif (sig == SIGUSR2):
    x_gui.emit(SIGNAL("SIGUSR2()"))

def showWindow(self_):
  if (self_.isMaximized()):
    self_.showMaximized()
  else:
    self_.showNormal()

# Shared Icons
def setIcons(self_, modes):
  if ("canvas" in modes):
    self_.act_canvas_arrange.setIcon(getIcon("view-sort-ascending"))
    self_.act_canvas_refresh.setIcon(getIcon("view-refresh"))
    self_.act_canvas_zoom_fit.setIcon(getIcon("zoom-fit-best"))
    self_.act_canvas_zoom_in.setIcon(getIcon("zoom-in"))
    self_.act_canvas_zoom_out.setIcon(getIcon("zoom-out"))
    self_.act_canvas_zoom_100.setIcon(getIcon("zoom-original"))
    self_.act_canvas_print.setIcon(getIcon("document-print"))
    self_.b_canvas_zoom_fit.setIcon(getIcon("zoom-fit-best"))
    self_.b_canvas_zoom_in.setIcon(getIcon("zoom-in"))
    self_.b_canvas_zoom_out.setIcon(getIcon("zoom-out"))
    self_.b_canvas_zoom_100.setIcon(getIcon("zoom-original"))

  if ("jack" in modes):
    self_.act_jack_clear_xruns.setIcon(getIcon("edit-clear"))
    self_.act_jack_configure.setIcon(getIcon("configure"))
    self_.act_jack_render.setIcon(getIcon("media-record"))
    self_.b_jack_clear_xruns.setIcon(getIcon("edit-clear"))
    self_.b_jack_configure.setIcon(getIcon("configure"))
    self_.b_jack_render.setIcon(getIcon("media-record"))

  if ("transport" in modes):
    self_.act_transport_play.setIcon(getIcon("media-playback-start"))
    self_.act_transport_stop.setIcon(getIcon("media-playback-stop"))
    self_.act_transport_backwards.setIcon(getIcon("media-seek-backward"))
    self_.act_transport_forwards.setIcon(getIcon("media-seek-forward"))
    self_.b_transport_play.setIcon(getIcon("media-playback-start"))
    self_.b_transport_stop.setIcon(getIcon("media-playback-stop"))
    self_.b_transport_backwards.setIcon(getIcon("media-seek-backward"))
    self_.b_transport_forwards.setIcon(getIcon("media-seek-forward"))

  if ("misc" in modes):
    self_.act_quit.setIcon(getIcon("application-exit"))
    self_.act_configure.setIcon(getIcon("configure"))

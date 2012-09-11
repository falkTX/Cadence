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
from PyQt4.QtGui import QFileDialog, QIcon, QMessageBox
from codecs import open as codecopen

# Set Platform
if sys.platform == "darwin":
    from PyQt4.QtGui import qt_mac_set_menubar_icons
    qt_mac_set_menubar_icons(False)
    HAIKU   = False
    LINUX   = False
    MACOS   = True
    WINDOWS = False
elif "haiku" in sys.platform:
    HAIKU   = True
    LINUX   = False
    MACOS   = False
    WINDOWS = False
elif "linux" in sys.platform:
    HAIKU   = False
    LINUX   = True
    MACOS   = False
    WINDOWS = False
elif sys.platform in ("win32", "win64", "cygwin"):
    HAIKU   = False
    LINUX   = False
    MACOS   = False
    WINDOWS = True
else:
    HAIKU   = False
    LINUX   = False
    MACOS   = False
    WINDOWS = False

try:
    from signal import signal, SIGINT, SIGTERM, SIGUSR1, SIGUSR2
    haveSignal = True
except:
    haveSignal = False

# Set Version
VERSION = "0.5.0"

# Set Debug mode
DEBUG = bool("-d" in sys.argv or "-debug" in sys.argv or "--debug" in sys.argv)

# Global variables
global x_gui
x_gui = None

# Small integrity tests
HOME = os.getenv("HOME")
if HOME is None:
    if WINDOWS:
        HOME = os.getenv("TMP")
    else:
        qWarning("HOME variable not set")
        HOME = "/tmp"
elif not os.path.exists(HOME):
    qWarning("HOME variable set but not valid")
    if WINDOWS:
        HOME = os.getenv("TMP")
    else:
        HOME = "/tmp"

PATH_env = os.getenv("PATH")
if PATH_env is None:
    qWarning("PATH variable not set")
    if MACOS:
        PATH = ("/opt/local/bin", "/usr/local/bin", "/usr/bin", "/bin")
    elif WINDOWS:
        WINDIR = os.getenv("WINDIR")
        PATH   = (os.path.join(WINDIR, "system32"), WINDIR)
        del WINDIR
    else:
        PATH = ("/usr/local/bin", "/usr/bin", "/bin")
else:
    PATH = PATH_env.split(os.pathsep)
del PATH_env

# ------------------------------------------------------------------------------------------------------------

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

# ------------------------------------------------------------------------------------------------------------

# Remove/convert non-ascii chars from a string
def asciiString(string):
    return normalize('NFKD', string).encode("ascii", "ignore").decode("utf-8")

# Convert a ctypes c_char_p to a python string
def cString(value):
    if not value:
        return ""
    return value.decode("utf-8", errors="ignore")

# Check if a value is a number (float support)
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

def uopen(filename, mode="r"):
    return codecopen(filename, encoding="utf-8", mode=mode)

# QLineEdit and QPushButton combo
def getAndSetPath(self_, currentPath, lineEdit):
    newPath = QFileDialog.getExistingDirectory(self_, self_.tr("Set Path"), currentPath, QFileDialog.ShowDirsOnly)
    if newPath:
        lineEdit.setText(newPath)
    return newPath

# Get Icon from user theme, using our own as backup (Oxygen)
def getIcon(icon, size=16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

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

# ------------------------------------------------------------------------------------------------------------

# signal handler for unix systems
def setUpSignals(self_):
    if not haveSignal:
        return

    global x_gui
    x_gui = self_
    signal(SIGINT,  signalHandler)
    signal(SIGTERM, signalHandler)
    signal(SIGUSR1, signalHandler)
    signal(SIGUSR2, signalHandler)

    x_gui.connect(x_gui, SIGNAL("SIGUSR2()"), lambda: showWindowHandler())
    x_gui.connect(x_gui, SIGNAL("SIGTERM()"), SLOT("close()"))

def signalHandler(sig, frame):
    global x_gui
    if sig in (SIGINT, SIGTERM):
        x_gui.emit(SIGNAL("SIGTERM()"))
    elif sig == SIGUSR1:
        x_gui.emit(SIGNAL("SIGUSR1()"))
    elif sig == SIGUSR2:
        x_gui.emit(SIGNAL("SIGUSR2()"))

def showWindowHandler():
    global x_gui
    if x_gui.isMaximized():
        x_gui.showMaximized()
    else:
        x_gui.showNormal()

# ------------------------------------------------------------------------------------------------------------

# Shared Icons
def setIcons(self_, modes):
    if "canvas" in modes:
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

    if "jack" in modes:
        self_.act_jack_clear_xruns.setIcon(getIcon("edit-clear"))
        self_.act_jack_configure.setIcon(getIcon("configure"))
        self_.act_jack_render.setIcon(getIcon("media-record"))
        self_.b_jack_clear_xruns.setIcon(getIcon("edit-clear"))
        self_.b_jack_configure.setIcon(getIcon("configure"))
        self_.b_jack_render.setIcon(getIcon("media-record"))

    if "transport" in modes:
        self_.act_transport_play.setIcon(getIcon("media-playback-start"))
        self_.act_transport_stop.setIcon(getIcon("media-playback-stop"))
        self_.act_transport_backwards.setIcon(getIcon("media-seek-backward"))
        self_.act_transport_forwards.setIcon(getIcon("media-seek-forward"))
        self_.b_transport_play.setIcon(getIcon("media-playback-start"))
        self_.b_transport_stop.setIcon(getIcon("media-playback-stop"))
        self_.b_transport_backwards.setIcon(getIcon("media-seek-backward"))
        self_.b_transport_forwards.setIcon(getIcon("media-seek-forward"))

    if "misc" in modes:
        self_.act_quit.setIcon(getIcon("application-exit"))
        self_.act_configure.setIcon(getIcon("configure"))

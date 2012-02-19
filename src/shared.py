#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Common/Shared code
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
import os, sys
from PyQt4.QtCore import pyqtSlot, qWarning, SIGNAL, SLOT
from PyQt4.QtGui import QIcon, QMessageBox, QFileDialog

# Set Platform
if ("linux" in sys.platform):
  LINUX   = True
  WINDOWS = False
elif ("win" in sys.platform):
  LINUX   = False
  WINDOWS = True
else:
  LINUX   = False
  WINDOWS = False

if (WINDOWS == False):
  from signal import signal, SIGINT, SIGTERM, SIGUSR1, SIGUSR2

# Small integrity tests
HOME = os.getenv("HOME")
if (HOME == None):
  qWarning("HOME variable not set")
  HOME = "/tmp"
elif (os.path.exists(HOME) == False):
  qWarning("HOME variable set but not valid")
  HOME = "/tmp"

PATH_env = os.getenv("PATH")
if (PATH_env == None):
  qWarning("PATH variable not set")
  PATH = ("/bin", "/sbin", "/usr/local/bin", "/usr/local/sbin", "/usr/bin", "/usr/sbin", "/usr/games")
else:
  PATH = PATH_env.split(os.pathsep)
  del PATH_env

# Get Icon from user theme, using our own as backup (Oxygen)
def getIcon(icon, size=16):
  return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# QLineEdit and QPushButtom combo
def getAndSetPath(self, currentPath, lineEdit):
    newPath = QFileDialog.getExistingDirectory(self, self.tr("Set Path"), currentPath, QFileDialog.ShowDirsOnly)
    if (newPath):
      lineEdit.setText(newPath)
    return newPath

# Custom MessageBox
def CustomMessageBox(self, icon, title, text, extra_text="", buttons=QMessageBox.Yes|QMessageBox.No, defButton=QMessageBox.No):
    msgBox = QMessageBox(self)
    msgBox.setIcon(icon)
    msgBox.setWindowTitle(title)
    msgBox.setText(text)
    msgBox.setInformativeText(extra_text)
    msgBox.setStandardButtons(buttons)
    msgBox.setDefaultButton(defButton)
    return msgBox.exec_()

# signal handler for unix systems
def set_up_signals(_gui):
  if (WINDOWS == False):
    from signal import signal, SIGINT, SIGTERM, SIGUSR1, SIGUSR2
    global x_gui
    x_gui = _gui
    signal(SIGINT,  signal_handler)
    signal(SIGTERM, signal_handler)
    signal(SIGUSR1, signal_handler)
    signal(SIGUSR2, signal_handler)

    x_gui.connect(x_gui, SIGNAL("SIGUSR2()"), lambda gui=x_gui: showWindow(gui))
    x_gui.connect(x_gui, SIGNAL("SIGTERM()"), SLOT("close()"))

def signal_handler(sig=0, frame=0):
  global x_gui
  if (sig in (SIGINT, SIGTERM)):
    x_gui.emit(SIGNAL("SIGTERM()"))
  elif (sig == SIGUSR1):
    x_gui.emit(SIGNAL("SIGUSR1()"))
  elif (sig == SIGUSR2):
    x_gui.emit(SIGNAL("SIGUSR2()"))

def showWindow(self):
  if (self.isMaximized()):
    self.showMaximized()
  else:
    self.showNormal()

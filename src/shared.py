#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Common/Shared code
# Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>

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
import os
from PyQt4.QtCore import qDebug, qWarning
from PyQt4.QtGui import QIcon

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

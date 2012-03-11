#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to Settings dialog
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
from PyQt4.QtCore import pyqtSlot, SIGNAL, SLOT
from PyQt4.QtGui import QDialog, QDialogButtonBox, QIcon, QPixmap

# Imports (Custom Stuff)
import ui_settings_app
from patchcanvas_theme import *

# Define values here so we don't have to import fully patchcanvas here
CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# Settings Dialog
class SettingsW(QDialog, ui_settings_app.Ui_SettingsW):
    def __init__(self, parent, appName, hasGL):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.ms_AutoHideGroups = bool(appName == "catarina")

        self.settings = self.parent().settings
        self.loadSettings()

        if not hasGL:
          self.cb_canvas_use_opengl.setChecked(False)
          self.cb_canvas_use_opengl.setEnabled(False)

        self.label_icon.setPixmap(QPixmap(":/48x48/%s" % (appName)))
        self.lw_page.item(0, 0).setIcon(QIcon(":/48x48/%s" % (appName)))

        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))
        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))

    def hideRow(self, row):
        self.lw_page.hideRow(row)

    def setCurrentRow(self, row):
        self.lw_page.setCurrentCell(row, 0)

    def loadSettings(self):
        self.cb_canvas_hide_groups.setChecked(self.settings.value("Canvas/AutoHideGroups", self.ms_AutoHideGroups, type=bool))
        self.cb_canvas_bezier_lines.setChecked(self.settings.value("Canvas/UseBezierLines", True, type=bool))
        self.cb_canvas_eyecandy.setCheckState(self.settings.value("Canvas/EyeCandy", CANVAS_EYECANDY_SMALL, type=int))
        self.cb_canvas_use_opengl.setChecked(self.settings.value("Canvas/UseOpenGL", False, type=bool))
        self.cb_canvas_render_aa.setCheckState(self.settings.value("Canvas/Antialiasing", CANVAS_ANTIALIASING_SMALL, type=int))
        self.cb_canvas_render_text_aa.setChecked(self.settings.value("Canvas/TextAntialiasing", True, type=bool))
        self.cb_canvas_render_hq_aa.setChecked(self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

        theme_name = self.settings.value("Canvas/Theme", getDefaultThemeName(), type=str)

        for i in range(Theme.THEME_MAX):
          this_theme_name = getThemeName(i)
          self.cb_canvas_theme.addItem(this_theme_name)
          if (this_theme_name == theme_name):
            self.cb_canvas_theme.setCurrentIndex(i)

    @pyqtSlot()
    def slot_saveSettings(self):
        self.settings.setValue("Canvas/Theme", self.cb_canvas_theme.currentText())
        self.settings.setValue("Canvas/AutoHideGroups", self.cb_canvas_hide_groups.isChecked())
        self.settings.setValue("Canvas/UseBezierLines", self.cb_canvas_bezier_lines.isChecked())
        self.settings.setValue("Canvas/UseOpenGL", self.cb_canvas_use_opengl.isChecked())
        self.settings.setValue("Canvas/TextAntialiasing", self.cb_canvas_render_text_aa.isChecked())
        self.settings.setValue("Canvas/HighQualityAntialiasing", self.cb_canvas_render_hq_aa.isChecked())

        # 0, 1, 2 match their enum variants
        self.settings.setValue("Canvas/EyeCandy", self.cb_canvas_eyecandy.checkState())
        self.settings.setValue("Canvas/Antialiasing", self.cb_canvas_render_aa.checkState())

    @pyqtSlot()
    def slot_resetSettings(self):
        self.cb_canvas_theme.setCurrentIndex(0)
        self.cb_canvas_hide_groups.setChecked(self.ms_AutoHideGroups)
        self.cb_canvas_bezier_lines.setChecked(True)
        self.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
        self.cb_canvas_use_opengl.setChecked(False)
        self.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
        self.cb_canvas_render_text_aa.setChecked(True)
        self.cb_canvas_render_hq_aa.setChecked(False)

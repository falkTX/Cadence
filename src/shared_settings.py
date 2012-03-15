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
from shared import *
from patchcanvas_theme import *

# Define values here so we don't have to import full patchcanvas here
CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# ladish defines
LADISH_CONF_KEY_DAEMON_NOTIFY           = "/org/ladish/daemon/notify"
LADISH_CONF_KEY_DAEMON_SHELL            = "/org/ladish/daemon/shell"
LADISH_CONF_KEY_DAEMON_TERMINAL         = "/org/ladish/daemon/terminal"
LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART = "/org/ladish/daemon/studio_autostart"
LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY    = "/org/ladish/daemon/js_save_delay"

LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT           = True
LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT            = "sh"
LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT         = "x-terminal-emulator"
LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT = True
LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT    = 0

# Internal defines
global SETTINGS_DEFAULT_PROJECT_FOLDER
SETTINGS_DEFAULT_PROJECT_FOLDER = "/tmp"

def setDefaultProjectFolder(folder):
  global SETTINGS_DEFAULT_PROJECT_FOLDER
  SETTINGS_DEFAULT_PROJECT_FOLDER = folder

# Settings Dialog
class SettingsW(QDialog, ui_settings_app.Ui_SettingsW):
    def __init__(self, parent, appName, hasGL):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        # Load app-specific settings
        self.ms_AutoHideGroups = True
        self.ms_UseSystemTray  = True
        self.ms_CloseToTray    = False

        if (appName == "catarina"):
          self.ms_AutoHideGroups = False
          self.lw_page.hideRow(0)
          self.lw_page.hideRow(2)
          self.lw_page.hideRow(3)
          self.lw_page.setCurrentCell(1, 0)

        elif (appName == "catia"):
          self.ms_UseSystemTray = False
          self.group_main_paths.setVisible(False)
          self.lw_page.hideRow(2)
          self.lw_page.hideRow(3)
          self.lw_page.setCurrentCell(0, 0)

        elif (appName == "claudia"):
          self.cb_jack_port_alias.setVisible(False)
          self.label_jack_port_alias.setVisible(False)
          self.lw_page.setCurrentCell(0, 0)

        self.settings = self.parent().settings
        self.loadSettings()

        if not hasGL:
          self.cb_canvas_use_opengl.setChecked(False)
          self.cb_canvas_use_opengl.setEnabled(False)

        self.label_icon.setPixmap(QPixmap(":/48x48/%s" % (appName)))
        self.lw_page.item(0, 0).setIcon(QIcon(":/48x48/%s" % (appName)))
        self.lw_page.item(3, 0).setIcon(QIcon.fromTheme("application-x-executable", QIcon(":/48x48/exec.png")))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        self.connect(self.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetPath()"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

    def loadSettings(self):
        # ------------------------
        # Page 0

        self.le_main_def_folder.setText(self.settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER, type=str))
        self.cb_tray_enable.setChecked(self.settings.value("Main/UseSystemTray", self.ms_UseSystemTray, type=bool))
        self.cb_tray_close_to.setChecked(self.settings.value("Main/CloseToTray", self.ms_CloseToTray, type=bool))
        self.sb_gui_refresh.setValue(self.settings.value("Main/RefreshInterval", 120, type=int))
        self.cb_jack_port_alias.setCurrentIndex(self.settings.value("Main/JackPortAlias", 2, type=int))

        # ------------------------
        # Page 1

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

        # ------------------------
        # Page 2

        self.cb_ladish_notify.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool))
        self.le_ladish_shell.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
        self.le_ladish_terminal.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
        self.cb_ladish_studio_autostart.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool))
        self.sb_ladish_jsdelay.setValue(self.settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int))

        # ------------------------
        # Page 3

        database = self.settings.value("Apps/Database", "LADISH", type=str)
        if (database == "LADISH"):
          self.rb_database_ladish.setChecked(True)
        elif (database == "Klaudia"):
          self.rb_database_kxstudio.setChecked(True)

    @pyqtSlot()
    def slot_getAndSetPath(self):
        getAndSetPath(self, self.le_main_def_folder.text(), self.le_main_def_folder)

    @pyqtSlot()
    def slot_saveSettings(self):
        # TODO - check if page is visible
        # ------------------------
        # Page 0

        self.settings.setValue("Main/UseSystemTray", self.cb_tray_enable.isChecked())
        self.settings.setValue("Main/CloseToTray", self.cb_tray_close_to.isChecked())
        self.settings.setValue("Main/RefreshInterval", self.sb_gui_refresh.value())

        if (self.group_main_paths.isVisible()):
          self.settings.setValue("Main/DefaultProjectFolder", self.le_main_def_folder.text())

        if (self.cb_jack_port_alias.isVisible()):
          self.settings.setValue("Main/JackPortAlias", self.cb_jack_port_alias.currentIndex())

        # ------------------------
        # Page 1

        self.settings.setValue("Canvas/Theme", self.cb_canvas_theme.currentText())
        self.settings.setValue("Canvas/AutoHideGroups", self.cb_canvas_hide_groups.isChecked())
        self.settings.setValue("Canvas/UseBezierLines", self.cb_canvas_bezier_lines.isChecked())
        self.settings.setValue("Canvas/UseOpenGL", self.cb_canvas_use_opengl.isChecked())
        self.settings.setValue("Canvas/TextAntialiasing", self.cb_canvas_render_text_aa.isChecked())
        self.settings.setValue("Canvas/HighQualityAntialiasing", self.cb_canvas_render_hq_aa.isChecked())

        # 0, 1, 2 match their enum variants
        self.settings.setValue("Canvas/EyeCandy", self.cb_canvas_eyecandy.checkState())
        self.settings.setValue("Canvas/Antialiasing", self.cb_canvas_render_aa.checkState())

        # ------------------------
        # Page 2

        self.settings.setValue(LADISH_CONF_KEY_DAEMON_NOTIFY, self.cb_ladish_notify.isChecked())
        self.settings.setValue(LADISH_CONF_KEY_DAEMON_SHELL, self.le_ladish_shell.text())
        self.settings.setValue(LADISH_CONF_KEY_DAEMON_TERMINAL, self.le_ladish_terminal.text())
        self.settings.setValue(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, self.cb_ladish_studio_autostart.isChecked())
        self.settings.setValue(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, self.sb_ladish_jsdelay.value())

        # ------------------------
        # Page 3

        self.settings.setValue("Apps/Database", "LADISH" if self.rb_database_ladish.isChecked() else "Klaudia")

    @pyqtSlot()
    def slot_resetSettings(self):
        self.le_main_def_folder.setText(SETTINGS_DEFAULT_PROJECT_FOLDER)
        self.cb_tray_enable.setChecked(self.ms_UseSystemTray)
        self.cb_tray_close_to.setChecked(self.ms_CloseToTray)
        self.sb_gui_refresh.setValue(120)
        self.cb_jack_port_alias.setCurrentIndex(1)
        self.cb_ladish_notify.setChecked(LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT)
        self.cb_ladish_studio_autostart.setChecked(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT)
        self.le_ladish_shell.setText(LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT)
        self.le_ladish_terminal.setText(LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT)
        self.cb_canvas_theme.setCurrentIndex(0)
        self.cb_canvas_hide_groups.setChecked(self.ms_AutoHideGroups)
        self.cb_canvas_bezier_lines.setChecked(True)
        self.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
        self.cb_canvas_use_opengl.setChecked(False)
        self.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
        self.cb_canvas_render_text_aa.setChecked(True)
        self.cb_canvas_render_hq_aa.setChecked(False)
        self.rb_database_ladish.setChecked(True)

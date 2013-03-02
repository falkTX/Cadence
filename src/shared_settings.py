#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to Settings dialog
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

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDialog, QDialogButtonBox

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_settings_app
from shared import *
from patchcanvas_theme import *

# ------------------------------------------------------------------------------------------------------------
# Global variables

# Tab indexes
TAB_INDEX_MAIN         = 0
TAB_INDEX_CANVAS       = 1
TAB_INDEX_LADISH       = 2
TAB_INDEX_NONE         = 3

# PatchCanvas defines
CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# LADISH defines
LADISH_CONF_KEY_DAEMON_NOTIFY           = "/org/ladish/daemon/notify"
LADISH_CONF_KEY_DAEMON_SHELL            = "/org/ladish/daemon/shell"
LADISH_CONF_KEY_DAEMON_TERMINAL         = "/org/ladish/daemon/terminal"
LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART = "/org/ladish/daemon/studio_autostart"
LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY    = "/org/ladish/daemon/js_save_delay"

# LADISH defaults
LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT           = True
LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT            = "sh"
LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT         = "x-terminal-emulator"
LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT = True
LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT    = 0

# Internal defines and defaults
global SETTINGS_DEFAULT_PROJECT_FOLDER
SETTINGS_DEFAULT_PROJECT_FOLDER   = HOME

# ------------------------------------------------------------------------------------------------------------
# Change internal defines and defaults

def setDefaultProjectFolder(folder):
    global SETTINGS_DEFAULT_PROJECT_FOLDER
    SETTINGS_DEFAULT_PROJECT_FOLDER = folder

# ------------------------------------------------------------------------------------------------------------
# Settings Dialog

class SettingsW(QDialog, ui_settings_app.Ui_SettingsW):
    def __init__(self, parent, appName, hasOpenGL=False):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Set default settings

        self.m_refreshInterval = 120
        self.m_autoHideGroups  = True
        self.m_useSystemTray   = True
        self.m_closeToTray     = False

        # -------------------------------------------------------------
        # Set app-specific settings

        if appName == "catarina":
            self.m_autoHideGroups = False
            self.lw_page.hideRow(TAB_INDEX_MAIN)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.lw_page.setCurrentCell(TAB_INDEX_CANVAS, 0)

        elif appName == "catia":
            self.m_useSystemTray = False
            self.group_main_paths.setEnabled(False)
            self.group_main_paths.setVisible(False)
            self.group_tray.setEnabled(False)
            self.group_tray.setVisible(False)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        elif appName == "claudia":
            self.cb_jack_port_alias.setEnabled(False)
            self.cb_jack_port_alias.setVisible(False)
            self.label_jack_port_alias.setEnabled(False)
            self.label_jack_port_alias.setVisible(False)
            self.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        else:
            self.lw_page.hideRow(TAB_INDEX_MAIN)
            self.lw_page.hideRow(TAB_INDEX_CANVAS)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.stackedWidget.setCurrentIndex(TAB_INDEX_NONE)
            return

        # -------------------------------------------------------------
        # Load settings

        self.settings = parent.settings
        self.loadSettings()

        # -------------------------------------------------------------
        # Set-up GUI

        if not hasOpenGL:
            self.cb_canvas_use_opengl.setChecked(False)
            self.cb_canvas_use_opengl.setEnabled(False)

        self.lw_page.item(0, 0).setIcon(getIcon(appName, 48))
        self.label_icon_main.setPixmap(getIcon(appName, 48).pixmap(48, 48))

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

        self.connect(self.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetProjectPath()"))

    def loadSettings(self):
        if not self.lw_page.isRowHidden(TAB_INDEX_MAIN):
            self.le_main_def_folder.setText(self.settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER, type=str))
            self.cb_tray_enable.setChecked(self.settings.value("Main/UseSystemTray", self.m_useSystemTray, type=bool))
            self.cb_tray_close_to.setChecked(self.settings.value("Main/CloseToTray", self.m_closeToTray, type=bool))
            self.sb_gui_refresh.setValue(self.settings.value("Main/RefreshInterval", self.m_refreshInterval, type=int))
            self.cb_jack_port_alias.setCurrentIndex(self.settings.value("Main/JackPortAlias", 2, type=int))

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CANVAS):
            self.cb_canvas_hide_groups.setChecked(self.settings.value("Canvas/AutoHideGroups", self.m_autoHideGroups, type=bool))
            self.cb_canvas_bezier_lines.setChecked(self.settings.value("Canvas/UseBezierLines", True, type=bool))
            self.cb_canvas_eyecandy.setCheckState(self.settings.value("Canvas/EyeCandy", CANVAS_EYECANDY_SMALL, type=int))
            self.cb_canvas_use_opengl.setChecked(self.settings.value("Canvas/UseOpenGL", False, type=bool))
            self.cb_canvas_render_aa.setCheckState(self.settings.value("Canvas/Antialiasing", CANVAS_ANTIALIASING_SMALL, type=int))
            self.cb_canvas_render_hq_aa.setChecked(self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

            themeName = self.settings.value("Canvas/Theme", getDefaultThemeName(), type=str)

            for i in range(Theme.THEME_MAX):
                thisThemeName = getThemeName(i)
                self.cb_canvas_theme.addItem(thisThemeName)
                if thisThemeName == themeName:
                    self.cb_canvas_theme.setCurrentIndex(i)

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_LADISH):
            self.cb_ladish_notify.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool))
            self.le_ladish_shell.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
            self.le_ladish_terminal.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
            self.cb_ladish_studio_autostart.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool))
            self.sb_ladish_jsdelay.setValue(self.settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int))

    @pyqtSlot()
    def slot_saveSettings(self):
        if not self.lw_page.isRowHidden(TAB_INDEX_MAIN):
            self.settings.setValue("Main/RefreshInterval", self.sb_gui_refresh.value())

            if self.group_tray.isEnabled():
                self.settings.setValue("Main/UseSystemTray", self.cb_tray_enable.isChecked())
                self.settings.setValue("Main/CloseToTray", self.cb_tray_close_to.isChecked())

            if self.group_main_paths.isEnabled():
                self.settings.setValue("Main/DefaultProjectFolder", self.le_main_def_folder.text())

            if self.cb_jack_port_alias.isEnabled():
                self.settings.setValue("Main/JackPortAlias", self.cb_jack_port_alias.currentIndex())

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CANVAS):
            self.settings.setValue("Canvas/Theme", self.cb_canvas_theme.currentText())
            self.settings.setValue("Canvas/AutoHideGroups", self.cb_canvas_hide_groups.isChecked())
            self.settings.setValue("Canvas/UseBezierLines", self.cb_canvas_bezier_lines.isChecked())
            self.settings.setValue("Canvas/UseOpenGL", self.cb_canvas_use_opengl.isChecked())
            self.settings.setValue("Canvas/HighQualityAntialiasing", self.cb_canvas_render_hq_aa.isChecked())

            # 0, 1, 2 match their enum variants
            self.settings.setValue("Canvas/EyeCandy", self.cb_canvas_eyecandy.checkState())
            self.settings.setValue("Canvas/Antialiasing", self.cb_canvas_render_aa.checkState())

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_LADISH):
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_NOTIFY, self.cb_ladish_notify.isChecked())
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_SHELL, self.le_ladish_shell.text())
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_TERMINAL, self.le_ladish_terminal.text())
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, self.cb_ladish_studio_autostart.isChecked())
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, self.sb_ladish_jsdelay.value())

    @pyqtSlot()
    def slot_resetSettings(self):
        if self.lw_page.currentRow() == TAB_INDEX_MAIN:
            self.le_main_def_folder.setText(SETTINGS_DEFAULT_PROJECT_FOLDER)
            self.cb_tray_enable.setChecked(self.m_useSystemTray)
            self.cb_tray_close_to.setChecked(self.m_closeToTray)
            self.sb_gui_refresh.setValue(self.m_refreshInterval)
            self.cb_jack_port_alias.setCurrentIndex(2)

        elif self.lw_page.currentRow() == TAB_INDEX_CANVAS:
            self.cb_canvas_theme.setCurrentIndex(0)
            self.cb_canvas_hide_groups.setChecked(self.m_autoHideGroups)
            self.cb_canvas_bezier_lines.setChecked(True)
            self.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
            self.cb_canvas_use_opengl.setChecked(False)
            self.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
            self.cb_canvas_render_hq_aa.setChecked(False)

        elif self.lw_page.currentRow() == TAB_INDEX_LADISH:
            self.cb_ladish_notify.setChecked(LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT)
            self.cb_ladish_studio_autostart.setChecked(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT)
            self.le_ladish_shell.setText(LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT)
            self.le_ladish_terminal.setText(LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT)

    @pyqtSlot()
    def slot_getAndSetProjectPath(self):
        getAndSetPath(self, self.le_main_def_folder.text(), self.le_main_def_folder)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

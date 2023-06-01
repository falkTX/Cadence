#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to the Settings dialog
# Copyright (C) 2010-2018 Filipe Coelho <falktx@falktx.com>
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

if True:
    from PyQt5.QtCore import pyqtSlot, QSettings
    from PyQt5.QtWidgets import QDialog, QDialogButtonBox
else:
    from PyQt4.QtCore import pyqtSlot, QSettings
    from PyQt4.QtGui import QDialog, QDialogButtonBox

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_settings_app
from shared import *
from patchcanvas_theme import *

# ------------------------------------------------------------------------------------------------------------
# Global variables

# Tab indexes
TAB_INDEX_MAIN   = 0
TAB_INDEX_CANVAS = 1
TAB_INDEX_LADISH = 2
TAB_INDEX_NONE   = 3

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
LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT         = "xterm"
LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT = True
LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT    = 0

# Internal defaults
global SETTINGS_DEFAULT_PROJECT_FOLDER
SETTINGS_DEFAULT_PROJECT_FOLDER = HOME

# ------------------------------------------------------------------------------------------------------------
# Change internal defaults

def setDefaultProjectFolder(folder):
    global SETTINGS_DEFAULT_PROJECT_FOLDER
    SETTINGS_DEFAULT_PROJECT_FOLDER = folder

# ------------------------------------------------------------------------------------------------------------
# Settings Dialog

class SettingsW(QDialog):
    def __init__(self, parent, appName, hasOpenGL=False):
        QDialog.__init__(self, parent)
        self.ui = ui_settings_app.Ui_SettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set default settings

        self.fRefreshInterval = 120
        self.fAutoHideGroups  = True
        self.fUseSystemTray   = True
        self.fCloseToTray     = False

        # -------------------------------------------------------------
        # Set app-specific settings

        if appName == "catarina":
            self.fAutoHideGroups = False
            self.ui.lw_page.hideRow(TAB_INDEX_MAIN)
            self.ui.lw_page.hideRow(TAB_INDEX_LADISH)
            self.ui.lw_page.setCurrentCell(TAB_INDEX_CANVAS, 0)

        elif appName == "catia":
            self.fUseSystemTray = False
            self.ui.group_main_paths.setEnabled(False)
            self.ui.group_main_paths.setVisible(False)
            self.ui.group_tray.setEnabled(False)
            self.ui.group_tray.setVisible(False)
            self.ui.lw_page.hideRow(TAB_INDEX_LADISH)
            self.ui.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        elif appName == "claudia":
            self.ui.cb_jack_port_alias.setEnabled(False)
            self.ui.cb_jack_port_alias.setVisible(False)
            self.ui.label_jack_port_alias.setEnabled(False)
            self.ui.label_jack_port_alias.setVisible(False)
            self.ui.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        else:
            self.ui.lw_page.hideRow(TAB_INDEX_MAIN)
            self.ui.lw_page.hideRow(TAB_INDEX_CANVAS)
            self.ui.lw_page.hideRow(TAB_INDEX_LADISH)
            self.ui.stackedWidget.setCurrentIndex(TAB_INDEX_NONE)
            return

        # -------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------
        # Set-up GUI

        if not hasOpenGL:
            self.ui.cb_canvas_use_opengl.setChecked(False)
            self.ui.cb_canvas_use_opengl.setEnabled(False)

        self.ui.lw_page.item(0, 0).setIcon(getIcon(appName, 48))
        self.ui.label_icon_main.setPixmap(getIcon(appName, 48).pixmap(48, 48))

        # -------------------------------------------------------------
        # Set-up connections

        self.accepted.connect(self.slot_saveSettings)
        self.ui.buttonBox.button(QDialogButtonBox.Reset).clicked.connect(self.slot_resetSettings)
        self.ui.b_main_def_folder_open.clicked.connect(self.slot_getAndSetProjectPath)

    def loadSettings(self):
        settings = QSettings()

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_MAIN):
            self.ui.le_main_def_folder.setText(settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER, type=str))
            self.ui.cb_tray_enable.setChecked(settings.value("Main/UseSystemTray", self.fUseSystemTray, type=bool))
            self.ui.cb_tray_close_to.setChecked(settings.value("Main/CloseToTray", self.fCloseToTray, type=bool))
            self.ui.sb_gui_refresh.setValue(settings.value("Main/RefreshInterval", self.fRefreshInterval, type=int))
            self.ui.cb_jack_port_alias.setCurrentIndex(settings.value("Main/JackPortAlias", 2, type=int))

        # ---------------------------------------

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_CANVAS):
            self.ui.cb_canvas_hide_groups.setChecked(settings.value("Canvas/AutoHideGroups", self.fAutoHideGroups, type=bool))
            self.ui.cb_canvas_bezier_lines.setChecked(settings.value("Canvas/UseBezierLines", True, type=bool))
            self.ui.cb_canvas_eyecandy.setCheckState(settings.value("Canvas/EyeCandy", CANVAS_EYECANDY_SMALL, type=int))
            self.ui.cb_canvas_use_opengl.setChecked(settings.value("Canvas/UseOpenGL", False, type=bool))
            self.ui.cb_canvas_render_aa.setCheckState(settings.value("Canvas/Antialiasing", CANVAS_ANTIALIASING_SMALL, type=int))
            self.ui.cb_canvas_render_hq_aa.setChecked(settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

            themeName = settings.value("Canvas/Theme", getDefaultThemeName(), type=str)

            for i in range(Theme.THEME_MAX):
                thisThemeName = getThemeName(i)
                self.ui.cb_canvas_theme.addItem(thisThemeName)
                if thisThemeName == themeName:
                    self.ui.cb_canvas_theme.setCurrentIndex(i)

        # ---------------------------------------

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_LADISH):
            self.ui.cb_ladish_notify.setChecked(settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool))
            self.ui.le_ladish_shell.setText(settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
            self.ui.le_ladish_terminal.setText(settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
            self.ui.cb_ladish_studio_autostart.setChecked(settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool))
            self.ui.sb_ladish_jsdelay.setValue(settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int))

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings()

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_MAIN):
            settings.setValue("Main/RefreshInterval", self.ui.sb_gui_refresh.value())

            if self.ui.group_tray.isEnabled():
                settings.setValue("Main/UseSystemTray", self.ui.cb_tray_enable.isChecked())
                settings.setValue("Main/CloseToTray", self.ui.cb_tray_close_to.isChecked())

            if self.ui.group_main_paths.isEnabled():
                settings.setValue("Main/DefaultProjectFolder", self.ui.le_main_def_folder.text())

            if self.ui.cb_jack_port_alias.isEnabled():
                settings.setValue("Main/JackPortAlias", self.ui.cb_jack_port_alias.currentIndex())

        # ---------------------------------------

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_CANVAS):
            settings.setValue("Canvas/Theme", self.ui.cb_canvas_theme.currentText())
            settings.setValue("Canvas/AutoHideGroups", self.ui.cb_canvas_hide_groups.isChecked())
            settings.setValue("Canvas/UseBezierLines", self.ui.cb_canvas_bezier_lines.isChecked())
            settings.setValue("Canvas/UseOpenGL", self.ui.cb_canvas_use_opengl.isChecked())
            settings.setValue("Canvas/HighQualityAntialiasing", self.ui.cb_canvas_render_hq_aa.isChecked())

            # 0, 1, 2 match their enum variants
            settings.setValue("Canvas/EyeCandy", self.ui.cb_canvas_eyecandy.checkState())
            settings.setValue("Canvas/Antialiasing", self.ui.cb_canvas_render_aa.checkState())

        # ---------------------------------------

        if not self.ui.lw_page.isRowHidden(TAB_INDEX_LADISH):
            settings.setValue(LADISH_CONF_KEY_DAEMON_NOTIFY, self.ui.cb_ladish_notify.isChecked())
            settings.setValue(LADISH_CONF_KEY_DAEMON_SHELL, self.ui.le_ladish_shell.text())
            settings.setValue(LADISH_CONF_KEY_DAEMON_TERMINAL, self.ui.le_ladish_terminal.text())
            settings.setValue(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, self.ui.cb_ladish_studio_autostart.isChecked())
            settings.setValue(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, self.ui.sb_ladish_jsdelay.value())

    @pyqtSlot()
    def slot_resetSettings(self):
        if self.ui.lw_page.currentRow() == TAB_INDEX_MAIN:
            self.ui.le_main_def_folder.setText(SETTINGS_DEFAULT_PROJECT_FOLDER)
            self.ui.cb_tray_enable.setChecked(self.fUseSystemTray)
            self.ui.cb_tray_close_to.setChecked(self.fCloseToTray)
            self.ui.sb_gui_refresh.setValue(self.fRefreshInterval)
            self.ui.cb_jack_port_alias.setCurrentIndex(2)

        elif self.ui.lw_page.currentRow() == TAB_INDEX_CANVAS:
            self.ui.cb_canvas_theme.setCurrentIndex(0)
            self.ui.cb_canvas_hide_groups.setChecked(self.fAutoHideGroups)
            self.ui.cb_canvas_bezier_lines.setChecked(True)
            self.ui.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_use_opengl.setChecked(False)
            self.ui.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_render_hq_aa.setChecked(False)

        elif self.ui.lw_page.currentRow() == TAB_INDEX_LADISH:
            self.ui.cb_ladish_notify.setChecked(LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT)
            self.ui.cb_ladish_studio_autostart.setChecked(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT)
            self.ui.le_ladish_shell.setText(LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT)
            self.ui.le_ladish_terminal.setText(LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT)

    @pyqtSlot()
    def slot_getAndSetProjectPath(self):
        getAndSetPath(self, self.ui.le_main_def_folder.text(), self.ui.le_main_def_folder)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

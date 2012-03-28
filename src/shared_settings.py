#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to Settings dialog
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
global SETTINGS_DEFAULT_PLUGINS_PATHS

SETTINGS_DEFAULT_PROJECT_FOLDER = "/tmp"
SETTINGS_DEFAULT_PLUGINS_PATHS  = [None, None, None, None, None]

def setDefaultProjectFolder(folder):
  global SETTINGS_DEFAULT_PROJECT_FOLDER
  SETTINGS_DEFAULT_PROJECT_FOLDER = folder

def setDefaultPluginsPaths(ladspas, dssis, lv2s, vsts, sf2s):
  global SETTINGS_DEFAULT_PLUGINS_PATHS
  SETTINGS_DEFAULT_PLUGINS_PATHS[0] = ladspas
  SETTINGS_DEFAULT_PLUGINS_PATHS[1] = dssis
  SETTINGS_DEFAULT_PLUGINS_PATHS[2] = lv2s
  SETTINGS_DEFAULT_PLUGINS_PATHS[3] = vsts
  SETTINGS_DEFAULT_PLUGINS_PATHS[4] = sf2s

# Settings Dialog
class SettingsW(QDialog, ui_settings_app.Ui_SettingsW):
    def __init__(self, parent, appName, hasOpenGL=False):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        # Load app-specific settings
        self.ms_RefreshInterval = 120
        self.ms_AutoHideGroups  = True
        self.ms_UseSystemTray   = True
        self.ms_CloseToTray     = False

        if (appName == "catarina"):
          self.ms_AutoHideGroups = False
          self.lw_page.hideRow(0)
          self.lw_page.hideRow(2)
          self.lw_page.hideRow(3)
          self.lw_page.hideRow(4)
          self.lw_page.hideRow(5)
          self.lw_page.setCurrentCell(1, 0)

        elif (appName == "catia"):
          self.ms_UseSystemTray = False
          self.group_main_paths.setEnabled(False)
          self.group_main_paths.setVisible(False)
          self.lw_page.hideRow(2)
          self.lw_page.hideRow(3)
          self.lw_page.hideRow(4)
          self.lw_page.hideRow(5)
          self.lw_page.setCurrentCell(0, 0)

        elif (appName == "claudia"):
          self.cb_jack_port_alias.setEnabled(False)
          self.cb_jack_port_alias.setVisible(False)
          self.label_jack_port_alias.setEnabled(False)
          self.label_jack_port_alias.setVisible(False)
          self.lw_page.hideRow(3) # TODO
          self.lw_page.hideRow(4)
          self.lw_page.hideRow(5)
          self.lw_page.setCurrentCell(0, 0)

        elif (appName == "carla"):
          self.ms_RefreshInterval = 60
          self.cb_jack_port_alias.setEnabled(False)
          self.cb_jack_port_alias.setVisible(False)
          self.label_jack_port_alias.setEnabled(False)
          self.label_jack_port_alias.setVisible(False)
          self.group_tray.setEnabled(False)
          self.group_tray.setVisible(False)
          self.lw_page.hideRow(1)
          self.lw_page.hideRow(2)
          self.lw_page.hideRow(3)
          self.lw_page.setCurrentCell(0, 0)

        self.settings = self.parent().settings
        self.loadSettings()

        if (not hasOpenGL):
          self.cb_canvas_use_opengl.setChecked(False)
          self.cb_canvas_use_opengl.setEnabled(False)

        self.label_icon.setPixmap(QPixmap(":/48x48/%s" % (appName)))
        self.lw_page.item(0, 0).setIcon(QIcon(":/48x48/%s" % (appName)))
        self.lw_page.item(3, 0).setIcon(QIcon.fromTheme("application-x-executable", QIcon(":/48x48/exec.png")))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        self.connect(self.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetPath_project()"))
        self.connect(self.b_paths_add, SIGNAL("clicked()"), SLOT("slot_addPath()"))
        self.connect(self.b_paths_remove, SIGNAL("clicked()"), SLOT("slot_removePath()"))
        self.connect(self.b_paths_change, SIGNAL("clicked()"), SLOT("slot_changePath()"))
        self.connect(self.tw_paths, SIGNAL("currentChanged(int)"), SLOT("slot_pathTabChanged(int)"))
        self.connect(self.lw_ladspa, SIGNAL("currentRowChanged(int)"), SLOT("slot_pathRowChanged(int)"))
        self.connect(self.lw_dssi, SIGNAL("currentRowChanged(int)"), SLOT("slot_pathRowChanged(int)"))
        self.connect(self.lw_lv2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pathRowChanged(int)"))
        self.connect(self.lw_vst, SIGNAL("currentRowChanged(int)"), SLOT("slot_pathRowChanged(int)"))
        self.connect(self.lw_sf2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pathRowChanged(int)"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

        self.lw_ladspa.setCurrentRow(0)
        self.lw_dssi.setCurrentRow(0)
        self.lw_lv2.setCurrentRow(0)
        self.lw_vst.setCurrentRow(0)
        self.lw_sf2.setCurrentRow(0)
        self.slot_pathTabChanged(self.tw_paths.currentIndex())

    def loadSettings(self):
        # ------------------------
        # Page 0

        self.le_main_def_folder.setText(self.settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER, type=str))
        self.cb_tray_enable.setChecked(self.settings.value("Main/UseSystemTray", self.ms_UseSystemTray, type=bool))
        self.cb_tray_close_to.setChecked(self.settings.value("Main/CloseToTray", self.ms_CloseToTray, type=bool))
        self.sb_gui_refresh.setValue(self.settings.value("Main/RefreshInterval", self.ms_RefreshInterval, type=int))
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

        # ------------------------
        # Page 4

        self.ch_engine_global_client.setChecked(self.settings.value("Engine/GlobalClient", False, type=bool))
        self.ch_engine_dssi_chunks.setChecked(self.settings.value("Engine/DSSIChunks", False, type=bool))
        self.ch_engine_prefer_bridges.setChecked(self.settings.value("Engine/PreferBridges", True, type=bool))

        # ------------------------
        # Page 5

        ladspas = toList(self.settings.value("Paths/LADSPA", SETTINGS_DEFAULT_PLUGINS_PATHS[0]))
        dssis = toList(self.settings.value("Paths/DSSI", SETTINGS_DEFAULT_PLUGINS_PATHS[1]))
        lv2s = toList(self.settings.value("Paths/LV2", SETTINGS_DEFAULT_PLUGINS_PATHS[2]))
        vsts = toList(self.settings.value("Paths/VST", SETTINGS_DEFAULT_PLUGINS_PATHS[3]))
        sf2s = toList(self.settings.value("Paths/SF2", SETTINGS_DEFAULT_PLUGINS_PATHS[4]))

        ladspas.sort()
        dssis.sort()
        lv2s.sort()
        vsts.sort()
        sf2s.sort()

        for ladspa in ladspas:
          self.lw_ladspa.addItem(ladspa)

        for dssi in dssis:
          self.lw_dssi.addItem(dssi)

        for lv2 in lv2s:
          self.lw_lv2.addItem(lv2)

        for vst in vsts:
          self.lw_vst.addItem(vst)

        for sf2 in sf2s:
          self.lw_sf2.addItem(sf2)

    @pyqtSlot()
    def slot_getAndSetPath_project(self):
        getAndSetPath(self, self.le_main_def_folder.text(), self.le_main_def_folder)

    @pyqtSlot()
    def slot_addPath(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)
        if (newPath):
          if (self.tw_paths.currentIndex() == 0):
            self.lw_ladspa.addItem(newPath)
          elif (self.tw_paths.currentIndex() == 1):
            self.lw_dssi.addItem(newPath)
          elif (self.tw_paths.currentIndex() == 2):
            self.lw_lv2.addItem(newPath)
          elif (self.tw_paths.currentIndex() == 3):
            self.lw_vst.addItem(newPath)
          elif (self.tw_paths.currentIndex() == 4):
            self.lw_sf2.addItem(newPath)

    @pyqtSlot()
    def slot_removePath(self):
        if (self.tw_paths.currentIndex() == 0):
          self.lw_ladspa.takeItem(self.lw_ladspa.currentRow())
        elif (self.tw_paths.currentIndex() == 1):
          self.lw_dssi.takeItem(self.lw_dssi.currentRow())
        elif (self.tw_paths.currentIndex() == 2):
          self.lw_lv2.takeItem(self.lw_lv2.currentRow())
        elif (self.tw_paths.currentIndex() == 3):
          self.lw_vst.takeItem(self.lw_vst.currentRow())
        elif (self.tw_paths.currentIndex() == 4):
          self.lw_sf2.takeItem(self.lw_sf2.currentRow())

    @pyqtSlot()
    def slot_changePath(self):
        if (self.tw_paths.currentIndex() == 0):
          currentPath = self.lw_ladspa.currentItem().text()
        elif (self.tw_paths.currentIndex() == 1):
          currentPath = self.lw_dssi.currentItem().text()
        elif (self.tw_paths.currentIndex() == 2):
          currentPath = self.lw_lv2.currentItem().text()
        elif (self.tw_paths.currentIndex() == 3):
          currentPath = self.lw_vst.currentItem().text()
        elif (self.tw_paths.currentIndex() == 4):
          currentPath = self.lw_sf2.currentItem().text()
        else:
          currentPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), currentPath, QFileDialog.ShowDirsOnly)
        if (newPath):
          if (self.tw_paths.currentIndex() == 0):
            self.lw_ladspa.currentItem().setText(newPath)
          elif (self.tw_paths.currentIndex() == 1):
            self.lw_dssi.currentItem().setText(newPath)
          elif (self.tw_paths.currentIndex() == 2):
            self.lw_lv2.currentItem().setText(newPath)
          elif (self.tw_paths.currentIndex() == 3):
            self.lw_vst.currentItem().setText(newPath)
          elif (self.tw_paths.currentIndex() == 4):
            self.lw_sf2.currentItem().setText(newPath)

    @pyqtSlot(int)
    def slot_pathTabChanged(self, index):
        if (index == 0):
          row = self.lw_ladspa.currentRow()
        elif (index == 1):
          row = self.lw_dssi.currentRow()
        elif (index == 2):
          row = self.lw_lv2.currentRow()
        elif (index == 3):
          row = self.lw_vst.currentRow()
        elif (index == 4):
          row = self.lw_sf2.currentRow()
        else:
          row = -1

        check = bool(row >= 0)
        self.b_paths_remove.setEnabled(check)
        self.b_paths_change.setEnabled(check)

    @pyqtSlot(int)
    def slot_pathRowChanged(self, row):
        check = bool(row >= 0)
        self.b_paths_remove.setEnabled(check)
        self.b_paths_change.setEnabled(check)

    @pyqtSlot()
    def slot_saveSettings(self):
        # TODO - check if page is visible
        # ------------------------
        # Page 0

        self.settings.setValue("Main/RefreshInterval", self.sb_gui_refresh.value())

        if (self.group_tray.isEnabled()):
          self.settings.setValue("Main/UseSystemTray", self.cb_tray_enable.isChecked())
          self.settings.setValue("Main/CloseToTray", self.cb_tray_close_to.isChecked())

        if (self.group_main_paths.isEnabled()):
          self.settings.setValue("Main/DefaultProjectFolder", self.le_main_def_folder.text())

        if (self.cb_jack_port_alias.isEnabled()):
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

        # ------------------------
        # Page 4

        self.settings.setValue("Engine/GlobalClient", self.ch_engine_global_client.isChecked())
        self.settings.setValue("Engine/DSSIChunks", self.ch_engine_dssi_chunks.isChecked())
        self.settings.setValue("Engine/PreferBridges", self.ch_engine_prefer_bridges.isChecked())

        # ------------------------
        # Page 5

        ladspas = []
        dssis = []
        lv2s = []
        vsts = []
        sf2s = []

        for i in range(self.lw_ladspa.count()):
          ladspas.append(self.lw_ladspa.item(i).text())

        for i in range(self.lw_dssi.count()):
          dssis.append(self.lw_dssi.item(i).text())

        for i in range(self.lw_lv2.count()):
          lv2s.append(self.lw_lv2.item(i).text())

        for i in range(self.lw_vst.count()):
          vsts.append(self.lw_vst.item(i).text())

        for i in range(self.lw_sf2.count()):
          sf2s.append(self.lw_sf2.item(i).text())

        self.settings.setValue("Paths/LADSPA", ladspas)
        self.settings.setValue("Paths/DSSI", dssis)
        self.settings.setValue("Paths/LV2", lv2s)
        self.settings.setValue("Paths/VST", vsts)
        self.settings.setValue("Paths/SF2", sf2s)

    @pyqtSlot()
    def slot_resetSettings(self):
        self.le_main_def_folder.setText(SETTINGS_DEFAULT_PROJECT_FOLDER)
        self.cb_tray_enable.setChecked(self.ms_UseSystemTray)
        self.cb_tray_close_to.setChecked(self.ms_CloseToTray)
        self.sb_gui_refresh.setValue(self.ms_RefreshInterval)
        self.cb_jack_port_alias.setCurrentIndex(2)
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
        self.ch_engine_global_client.setChecked(False)
        self.ch_engine_dssi_chunks.setChecked(False)
        self.ch_engine_prefer_bridges.setChecked(True)

        ladspas, dssis, lv2s, vsts, sf2s = SETTINGS_DEFAULT_PLUGINS_PATHS

        ladspas.sort()
        dssis.sort()
        lv2s.sort()
        vsts.sort()
        sf2s.sort()

        self.lw_ladspa.clear()
        self.lw_dssi.clear()
        self.lw_lv2.clear()
        self.lw_vst.clear()
        self.lw_sf2.clear()

        for ladspa in ladspas:
          self.lw_ladspa.addItem(ladspa)

        for dssi in dssis:
          self.lw_dssi.addItem(dssi)

        for lv2 in lv2s:
          self.lw_lv2.addItem(lv2)

        for vst in vsts:
          self.lw_vst.addItem(vst)

        for sf2 in sf2s:
          self.lw_sf2.addItem(sf2)

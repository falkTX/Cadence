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

# Special rule for AVLinux
AVLINUX_PY2BUILD = False

if AVLINUX_PY2BUILD:
    from sip import setapi
    setapi("QString", 2)
    setapi("QVariant", 1)

# Imports (Global)
from PyQt4.QtCore import pyqtSlot, SIGNAL, SLOT
from PyQt4.QtGui import QDialog, QDialogButtonBox, QIcon, QPixmap

# Imports (Custom Stuff)
import ui_settings_app
from shared import *
from patchcanvas_theme import *

TAB_INDEX_MAIN   = 0
TAB_INDEX_CANVAS = 1
TAB_INDEX_LADISH = 2
TAB_INDEX_CARLA_ENGINE = 3
TAB_INDEX_CARLA_PATHS  = 4

# Define values here so we don't have to import full patchcanvas
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
SETTINGS_DEFAULT_PROJECT_FOLDER = HOME
SETTINGS_DEFAULT_PLUGINS_PATHS  = [[], [], [], [], [], [], []]

def setDefaultProjectFolder(folder):
    global SETTINGS_DEFAULT_PROJECT_FOLDER
    SETTINGS_DEFAULT_PROJECT_FOLDER = folder

def setDefaultPluginsPaths(ladspas, dssis, lv2s, vsts, gigs, sf2s, sfzs):
    global SETTINGS_DEFAULT_PLUGINS_PATHS
    SETTINGS_DEFAULT_PLUGINS_PATHS  = [ladspas, dssis, lv2s, vsts, gigs, sf2s, sfzs]

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

        if appName == "catarina":
            self.ms_AutoHideGroups = False
            self.lw_page.hideRow(0)
            self.lw_page.hideRow(2)
            self.lw_page.hideRow(3)
            self.lw_page.hideRow(4)
            self.lw_page.setCurrentCell(1, 0)

        elif appName == "catia":
            self.ms_UseSystemTray = False
            self.group_main_paths.setEnabled(False)
            self.group_main_paths.setVisible(False)
            self.group_tray.setEnabled(False)
            self.group_tray.setVisible(False)
            self.lw_page.hideRow(2)
            self.lw_page.hideRow(3)
            self.lw_page.hideRow(4)
            self.lw_page.setCurrentCell(0, 0)

        elif appName == "claudia":
            self.cb_jack_port_alias.setEnabled(False)
            self.cb_jack_port_alias.setVisible(False)
            self.label_jack_port_alias.setEnabled(False)
            self.label_jack_port_alias.setVisible(False)
            self.lw_page.hideRow(3)
            self.lw_page.hideRow(4)
            self.lw_page.setCurrentCell(0, 0)

        elif appName == "carla":
            self.ms_RefreshInterval = 60
            self.cb_jack_port_alias.setEnabled(False)
            self.cb_jack_port_alias.setVisible(False)
            self.label_jack_port_alias.setEnabled(False)
            self.label_jack_port_alias.setVisible(False)
            self.group_tray.setEnabled(False)
            self.group_tray.setVisible(False)
            self.lw_page.hideRow(1)
            self.lw_page.hideRow(2)
            self.lw_page.setCurrentCell(0, 0)

        self.settings = self.parent().settings
        self.loadSettings()

        if not hasOpenGL:
            self.cb_canvas_use_opengl.setChecked(False)
            self.cb_canvas_use_opengl.setEnabled(False)

        self.label_icon.setPixmap(QPixmap(":/48x48/%s" % appName))
        self.lw_page.item(0, 0).setIcon(QIcon(":/48x48/%s" % appName))
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
        self.lw_gig.setCurrentRow(0)
        self.lw_sf2.setCurrentRow(0)
        self.lw_sfz.setCurrentRow(0)
        self.slot_pathTabChanged(self.tw_paths.currentIndex())

    def loadSettings(self):
        if not self.lw_page.isRowHidden(TAB_INDEX_MAIN):
            if AVLINUX_PY2BUILD:
                self.le_main_def_folder.setText(self.settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER).toString())
                self.cb_tray_enable.setChecked(self.settings.value("Main/UseSystemTray", self.ms_UseSystemTray).toBool())
                self.cb_tray_close_to.setChecked(self.settings.value("Main/CloseToTray", self.ms_CloseToTray).toBool())
                self.sb_gui_refresh.setValue(self.settings.value("Main/RefreshInterval", self.ms_RefreshInterval).toInt()[0])
                self.cb_jack_port_alias.setCurrentIndex(self.settings.value("Main/JackPortAlias", 2).toInt()[0])
            else:
                self.le_main_def_folder.setText(self.settings.value("Main/DefaultProjectFolder", SETTINGS_DEFAULT_PROJECT_FOLDER, type=str))
                self.cb_tray_enable.setChecked(self.settings.value("Main/UseSystemTray", self.ms_UseSystemTray, type=bool))
                self.cb_tray_close_to.setChecked(self.settings.value("Main/CloseToTray", self.ms_CloseToTray, type=bool))
                self.sb_gui_refresh.setValue(self.settings.value("Main/RefreshInterval", self.ms_RefreshInterval, type=int))
                self.cb_jack_port_alias.setCurrentIndex(self.settings.value("Main/JackPortAlias", 2, type=int))

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CANVAS):
            self.cb_canvas_hide_groups.setChecked(self.settings.value("Canvas/AutoHideGroups", self.ms_AutoHideGroups, type=bool))
            self.cb_canvas_bezier_lines.setChecked(self.settings.value("Canvas/UseBezierLines", True, type=bool))
            self.cb_canvas_eyecandy.setCheckState(self.settings.value("Canvas/EyeCandy", CANVAS_EYECANDY_SMALL, type=int))
            self.cb_canvas_use_opengl.setChecked(self.settings.value("Canvas/UseOpenGL", False, type=bool))
            self.cb_canvas_render_aa.setCheckState(self.settings.value("Canvas/Antialiasing", CANVAS_ANTIALIASING_SMALL, type=int))
            self.cb_canvas_render_hq_aa.setChecked(self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

            theme_name = self.settings.value("Canvas/Theme", getDefaultThemeName(), type=str)

            for i in range(Theme.THEME_MAX):
                this_theme_name = getThemeName(i)
                self.cb_canvas_theme.addItem(this_theme_name)
                if this_theme_name == theme_name:
                    self.cb_canvas_theme.setCurrentIndex(i)

        # ---------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_LADISH):
            self.cb_ladish_notify.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool))
            self.le_ladish_shell.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
            self.le_ladish_terminal.setText(self.settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
            self.cb_ladish_studio_autostart.setChecked(self.settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool))
            self.sb_ladish_jsdelay.setValue(self.settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int))

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_ENGINE):
            if AVLINUX_PY2BUILD:
                self.ch_engine_global_client.setChecked(self.settings.value("Engine/GlobalJackClient", False).toBool())
                self.ch_engine_dssi_chunks.setChecked(self.settings.value("Engine/UseDSSIChunks", False).toBool())
                self.ch_engine_prefer_bridges.setChecked(self.settings.value("Engine/PreferUIBridges", True).toBool())
            else:
                self.ch_engine_global_client.setChecked(self.settings.value("Engine/GlobalJackClient", False, type=bool))
                self.ch_engine_dssi_chunks.setChecked(self.settings.value("Engine/UseDSSIChunks", False, type=bool))
                self.ch_engine_prefer_bridges.setChecked(self.settings.value("Engine/PreferUIBridges", True, type=bool))

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_PATHS):
            if AVLINUX_PY2BUILD:
                ladspas = QVariantStringList(self.settings.value("Paths/LADSPA", SETTINGS_DEFAULT_PLUGINS_PATHS[0]).toList())
                dssis = QVariantStringList(self.settings.value("Paths/DSSI", SETTINGS_DEFAULT_PLUGINS_PATHS[1]).toList())
                lv2s = QVariantStringList(self.settings.value("Paths/LV2", SETTINGS_DEFAULT_PLUGINS_PATHS[2]).toList())
                vsts = QVariantStringList(self.settings.value("Paths/VST", SETTINGS_DEFAULT_PLUGINS_PATHS[3]).toList())
                gigs = QVariantStringList(self.settings.value("Paths/GIG", SETTINGS_DEFAULT_PLUGINS_PATHS[4]).toList())
                sf2s = QVariantStringList(self.settings.value("Paths/SF2", SETTINGS_DEFAULT_PLUGINS_PATHS[5]).toList())
                sfzs = QVariantStringList(self.settings.value("Paths/SFZ", SETTINGS_DEFAULT_PLUGINS_PATHS[6]).toList())
            else:
                ladspas = toList(self.settings.value("Paths/LADSPA", SETTINGS_DEFAULT_PLUGINS_PATHS[0]))
                dssis = toList(self.settings.value("Paths/DSSI", SETTINGS_DEFAULT_PLUGINS_PATHS[1]))
                lv2s = toList(self.settings.value("Paths/LV2", SETTINGS_DEFAULT_PLUGINS_PATHS[2]))
                vsts = toList(self.settings.value("Paths/VST", SETTINGS_DEFAULT_PLUGINS_PATHS[3]))
                gigs = toList(self.settings.value("Paths/GIG", SETTINGS_DEFAULT_PLUGINS_PATHS[4]))
                sf2s = toList(self.settings.value("Paths/SF2", SETTINGS_DEFAULT_PLUGINS_PATHS[5]))
                sfzs = toList(self.settings.value("Paths/SFZ", SETTINGS_DEFAULT_PLUGINS_PATHS[6]))

            ladspas.sort()
            dssis.sort()
            lv2s.sort()
            vsts.sort()
            gigs.sort()
            sf2s.sort()
            sfzs.sort()

            for ladspa in ladspas:
                self.lw_ladspa.addItem(ladspa)

            for dssi in dssis:
                self.lw_dssi.addItem(dssi)

            for lv2 in lv2s:
                self.lw_lv2.addItem(lv2)

            for vst in vsts:
                self.lw_vst.addItem(vst)

            for gig in gigs:
                self.lw_gig.addItem(gig)

            for sf2 in sf2s:
                self.lw_sf2.addItem(sf2)

            for sfz in sfzs:
                self.lw_sfz.addItem(sfz)

    @pyqtSlot()
    def slot_getAndSetPath_project(self):
        getAndSetPath(self, self.le_main_def_folder.text(), self.le_main_def_folder)

    @pyqtSlot()
    def slot_addPath(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)
        if newPath:
            if self.tw_paths.currentIndex() == 0:
                self.lw_ladspa.addItem(newPath)
            elif self.tw_paths.currentIndex() == 1:
                self.lw_dssi.addItem(newPath)
            elif self.tw_paths.currentIndex() == 2:
                self.lw_lv2.addItem(newPath)
            elif self.tw_paths.currentIndex() == 3:
                self.lw_vst.addItem(newPath)
            elif self.tw_paths.currentIndex() == 4:
                self.lw_gig.addItem(newPath)
            elif self.tw_paths.currentIndex() == 5:
                self.lw_sf2.addItem(newPath)
            elif self.tw_paths.currentIndex() == 6:
                self.lw_sfz.addItem(newPath)

    @pyqtSlot()
    def slot_removePath(self):
        if self.tw_paths.currentIndex() == 0:
            self.lw_ladspa.takeItem(self.lw_ladspa.currentRow())
        elif self.tw_paths.currentIndex() == 1:
            self.lw_dssi.takeItem(self.lw_dssi.currentRow())
        elif self.tw_paths.currentIndex() == 2:
            self.lw_lv2.takeItem(self.lw_lv2.currentRow())
        elif self.tw_paths.currentIndex() == 3:
            self.lw_vst.takeItem(self.lw_vst.currentRow())
        elif self.tw_paths.currentIndex() == 4:
            self.lw_gig.takeItem(self.lw_gig.currentRow())
        elif self.tw_paths.currentIndex() == 5:
            self.lw_sf2.takeItem(self.lw_sf2.currentRow())
        elif self.tw_paths.currentIndex() == 6:
            self.lw_sfz.takeItem(self.lw_sfz.currentRow())

    @pyqtSlot()
    def slot_changePath(self):
        if self.tw_paths.currentIndex() == 0:
            currentPath = self.lw_ladspa.currentItem().text()
        elif self.tw_paths.currentIndex() == 1:
            currentPath = self.lw_dssi.currentItem().text()
        elif self.tw_paths.currentIndex() == 2:
            currentPath = self.lw_lv2.currentItem().text()
        elif self.tw_paths.currentIndex() == 3:
            currentPath = self.lw_vst.currentItem().text()
        elif self.tw_paths.currentIndex() == 4:
            currentPath = self.lw_gig.currentItem().text()
        elif self.tw_paths.currentIndex() == 5:
            currentPath = self.lw_sf2.currentItem().text()
        elif self.tw_paths.currentIndex() == 6:
            currentPath = self.lw_sfz.currentItem().text()
        else:
            currentPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), currentPath, QFileDialog.ShowDirsOnly)
        if newPath:
            if self.tw_paths.currentIndex() == 0:
                self.lw_ladspa.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 1:
                self.lw_dssi.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 2:
                self.lw_lv2.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 3:
                self.lw_vst.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 4:
                self.lw_gig.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 5:
                self.lw_sf2.currentItem().setText(newPath)
            elif self.tw_paths.currentIndex() == 6:
                self.lw_sfz.currentItem().setText(newPath)

    @pyqtSlot(int)
    def slot_pathTabChanged(self, index):
        if index == 0:
            row = self.lw_ladspa.currentRow()
        elif index == 1:
            row = self.lw_dssi.currentRow()
        elif index == 2:
            row = self.lw_lv2.currentRow()
        elif index == 3:
            row = self.lw_vst.currentRow()
        elif index == 4:
            row = self.lw_gig.currentRow()
        elif index == 5:
            row = self.lw_sf2.currentRow()
        elif index == 6:
            row = self.lw_sfz.currentRow()
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

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_ENGINE):
            self.settings.setValue("Engine/GlobalJackClient", self.ch_engine_global_client.isChecked())
            self.settings.setValue("Engine/UseDSSIChunks", self.ch_engine_dssi_chunks.isChecked())
            self.settings.setValue("Engine/PreferUIBridges", self.ch_engine_prefer_bridges.isChecked())

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_PATHS):
            ladspas = []
            dssis = []
            lv2s = []
            vsts = []
            gigs = []
            sf2s = []
            sfzs = []

            for i in range(self.lw_ladspa.count()):
                ladspas.append(self.lw_ladspa.item(i).text())

            for i in range(self.lw_dssi.count()):
                dssis.append(self.lw_dssi.item(i).text())

            for i in range(self.lw_lv2.count()):
                lv2s.append(self.lw_lv2.item(i).text())

            for i in range(self.lw_vst.count()):
                vsts.append(self.lw_vst.item(i).text())

            for i in range(self.lw_gig.count()):
                gigs.append(self.lw_gig.item(i).text())

            for i in range(self.lw_sf2.count()):
                sf2s.append(self.lw_sf2.item(i).text())

            for i in range(self.lw_sfz.count()):
                sfzs.append(self.lw_sfz.item(i).text())

            self.settings.setValue("Paths/LADSPA", ladspas)
            self.settings.setValue("Paths/DSSI", dssis)
            self.settings.setValue("Paths/LV2", lv2s)
            self.settings.setValue("Paths/VST", vsts)
            self.settings.setValue("Paths/GIG", gigs)
            self.settings.setValue("Paths/SF2", sf2s)
            self.settings.setValue("Paths/SFZ", sfzs)

    @pyqtSlot()
    def slot_resetSettings(self):
        if self.lw_page.currentRow() == TAB_INDEX_MAIN:
            self.le_main_def_folder.setText(SETTINGS_DEFAULT_PROJECT_FOLDER)
            self.cb_tray_enable.setChecked(self.ms_UseSystemTray)
            self.cb_tray_close_to.setChecked(self.ms_CloseToTray)
            self.sb_gui_refresh.setValue(self.ms_RefreshInterval)
            self.cb_jack_port_alias.setCurrentIndex(2)

        elif self.lw_page.currentRow() == TAB_INDEX_CANVAS:
            self.cb_canvas_theme.setCurrentIndex(0)
            self.cb_canvas_hide_groups.setChecked(self.ms_AutoHideGroups)
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

        elif self.lw_page.currentRow() == TAB_INDEX_CARLA_ENGINE:
            self.ch_engine_global_client.setChecked(False)
            self.ch_engine_dssi_chunks.setChecked(False)
            self.ch_engine_prefer_bridges.setChecked(True)

        elif self.lw_page.currentRow() == TAB_INDEX_CARLA_PATHS:
            ladspas, dssis, lv2s, vsts, gigs, sf2s, sfzs = SETTINGS_DEFAULT_PLUGINS_PATHS

            if self.tw_paths.currentIndex() == 0:
                self.lw_ladspa.clear()
                ladspas.sort()

                for ladspa in ladspas:
                    self.lw_ladspa.addItem(ladspa)

            elif self.tw_paths.currentIndex() == 1:
                self.lw_dssi.clear()
                dssis.sort()

                for dssi in dssis:
                    self.lw_dssi.addItem(dssi)

            elif self.tw_paths.currentIndex() == 2:
                self.lw_lv2.clear()
                lv2s.sort()

                for lv2 in lv2s:
                    self.lw_lv2.addItem(lv2)

            elif self.tw_paths.currentIndex() == 3:
                self.lw_vst.clear()
                vsts.sort()

                for vst in vsts:
                    self.lw_vst.addItem(vst)

            elif self.tw_paths.currentIndex() == 4:
                self.lw_gig.clear()
                gigs.sort()

                for gig in gigs:
                    self.lw_gig.addItem(gig)

            elif self.tw_paths.currentIndex() == 5:
                self.lw_sf2.clear()
                sf2s.sort()

                for sf2 in sf2s:
                    self.lw_sf2.addItem(sf2)

            elif self.tw_paths.currentIndex() == 6:
                self.lw_sfz.clear()
                sfzs.sort()

                for sfz in sfzs:
                    self.lw_sfz.addItem(sfz)

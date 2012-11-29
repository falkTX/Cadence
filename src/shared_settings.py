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
TAB_INDEX_CARLA_ENGINE = 3
TAB_INDEX_CARLA_PATHS  = 4
TAB_INDEX_NONE         = 5

# Carla defines
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2

# Single and Multiple client mode is only for JACK,
# but we still want to match QComboBox index to defines,
# so add +2 pos padding if driverName != "JACK".
PROCESS_MODE_NON_JACK_PADDING = 2

# Carla defaults
CARLA_DEFAULT_PROCESS_HIGH_PRECISION = False
CARLA_DEFAULT_MAX_PARAMETERS         = 200
CARLA_DEFAULT_FORCE_STEREO           = False
CARLA_DEFAULT_USE_DSSI_VST_CHUNKS    = False
CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES  = False
CARLA_DEFAULT_PREFER_UI_BRIDGES      = True
CARLA_DEFAULT_OSC_UI_TIMEOUT         = 4000
CARLA_DEFAULT_DISABLE_CHECKS         = False

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
global SETTINGS_DEFAULT_PLUGINS_PATHS
global SETTINGS_AVAILABLE_ENGINE_DRIVERS
SETTINGS_DEFAULT_PROJECT_FOLDER   = HOME
SETTINGS_DEFAULT_PLUGINS_PATHS    = [[], [], [], [], [], [], []]
SETTINGS_AVAILABLE_ENGINE_DRIVERS = ["JACK"]

# ------------------------------------------------------------------------------------------------------------
# Change internal defines and defaults

def setDefaultProjectFolder(folder):
    global SETTINGS_DEFAULT_PROJECT_FOLDER
    SETTINGS_DEFAULT_PROJECT_FOLDER = folder

def setDefaultPluginsPaths(ladspa, dssi, lv2, vst, gig, sf2, sfz):
    global SETTINGS_DEFAULT_PLUGINS_PATHS
    SETTINGS_DEFAULT_PLUGINS_PATHS = [ladspa, dssi, lv2, vst, gig, sf2, sfz]

def setAvailableEngineDrivers(drivers):
    global SETTINGS_AVAILABLE_ENGINE_DRIVERS
    SETTINGS_AVAILABLE_ENGINE_DRIVERS = drivers

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
            self.lw_page.hideRow(TAB_INDEX_CARLA_ENGINE)
            self.lw_page.hideRow(TAB_INDEX_CARLA_PATHS)
            self.lw_page.setCurrentCell(TAB_INDEX_CANVAS, 0)

        elif appName == "catia":
            self.m_useSystemTray = False
            self.group_main_paths.setEnabled(False)
            self.group_main_paths.setVisible(False)
            self.group_tray.setEnabled(False)
            self.group_tray.setVisible(False)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.lw_page.hideRow(TAB_INDEX_CARLA_ENGINE)
            self.lw_page.hideRow(TAB_INDEX_CARLA_PATHS)
            self.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        elif appName == "claudia":
            self.cb_jack_port_alias.setEnabled(False)
            self.cb_jack_port_alias.setVisible(False)
            self.label_jack_port_alias.setEnabled(False)
            self.label_jack_port_alias.setVisible(False)
            self.lw_page.hideRow(TAB_INDEX_CARLA_ENGINE)
            self.lw_page.hideRow(TAB_INDEX_CARLA_PATHS)
            self.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

        elif appName == "carla":
            self.m_refreshInterval = 60
            self.cb_jack_port_alias.setEnabled(False)
            self.cb_jack_port_alias.setVisible(False)
            self.label_jack_port_alias.setEnabled(False)
            self.label_jack_port_alias.setVisible(False)
            self.group_tray.setEnabled(False)
            self.group_tray.setVisible(False)
            self.lw_page.hideRow(TAB_INDEX_CANVAS)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.lw_page.setCurrentCell(TAB_INDEX_MAIN, 0)

            global SETTINGS_AVAILABLE_ENGINE_DRIVERS
            for driver in SETTINGS_AVAILABLE_ENGINE_DRIVERS:
                self.cb_engine_audio_driver.addItem(driver)

        else:
            self.lw_page.hideRow(TAB_INDEX_MAIN)
            self.lw_page.hideRow(TAB_INDEX_CANVAS)
            self.lw_page.hideRow(TAB_INDEX_LADISH)
            self.lw_page.hideRow(TAB_INDEX_CARLA_ENGINE)
            self.lw_page.hideRow(TAB_INDEX_CARLA_PATHS)
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
        self.lw_page.item(3, 0).setIcon(getIcon("jack", 48))
        self.label_icon_main.setPixmap(getIcon(appName, 48).pixmap(48, 48))
        self.label_icon_engine.setPixmap(getIcon("jack", 48).pixmap(48, 48))

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

        self.connect(self.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetProjectPath()"))
        self.connect(self.cb_engine_audio_driver, SIGNAL("currentIndexChanged(int)"), SLOT("slot_engineAudioDriverChanged()"))
        self.connect(self.b_paths_add, SIGNAL("clicked()"), SLOT("slot_addPluginPath()"))
        self.connect(self.b_paths_remove, SIGNAL("clicked()"), SLOT("slot_removePluginPath()"))
        self.connect(self.b_paths_change, SIGNAL("clicked()"), SLOT("slot_changePluginPath()"))
        self.connect(self.tw_paths, SIGNAL("currentChanged(int)"), SLOT("slot_pluginPathTabChanged(int)"))
        self.connect(self.lw_ladspa, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.lw_dssi, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.lw_lv2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.lw_vst, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.lw_sf2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))

        # -------------------------------------------------------------
        # Post-connect setup

        self.lw_ladspa.setCurrentRow(0)
        self.lw_dssi.setCurrentRow(0)
        self.lw_lv2.setCurrentRow(0)
        self.lw_vst.setCurrentRow(0)
        self.lw_gig.setCurrentRow(0)
        self.lw_sf2.setCurrentRow(0)
        self.lw_sfz.setCurrentRow(0)
        self.slot_pluginPathTabChanged(self.tw_paths.currentIndex())

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

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_ENGINE):
            audioDriver = self.settings.value("Engine/AudioDriver", "JACK", type=str)
            for i in range(self.cb_engine_audio_driver.count()):
                if self.cb_engine_audio_driver.itemText(i) == audioDriver:
                    self.cb_engine_audio_driver.setCurrentIndex(i)
                    break
            else:
                self.cb_engine_audio_driver.setCurrentIndex(-1)

            if audioDriver == "JACK":
                processModeIndex = self.settings.value("Engine/ProcessMode", PROCESS_MODE_MULTIPLE_CLIENTS, type=int)
                self.cb_engine_process_mode_jack.setCurrentIndex(processModeIndex)
                self.sw_engine_process_mode.setCurrentIndex(0)
            else:
                processModeIndex  = self.settings.value("Engine/ProcessMode", PROCESS_MODE_CONTINUOUS_RACK, type=int)
                processModeIndex -= PROCESS_MODE_NON_JACK_PADDING
                self.cb_engine_process_mode_other.setCurrentIndex(processModeIndex)
                self.sw_engine_process_mode.setCurrentIndex(1)

            self.sb_engine_max_params.setValue(self.settings.value("Engine/MaxParameters", CARLA_DEFAULT_MAX_PARAMETERS, type=int))
            self.ch_engine_prefer_ui_bridges.setChecked(self.settings.value("Engine/PreferUiBridges", CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool))
            self.sb_engine_oscgui_timeout.setValue(self.settings.value("Engine/OscUiTimeout", CARLA_DEFAULT_OSC_UI_TIMEOUT, type=int))
            self.ch_engine_disable_checks.setChecked(self.settings.value("Engine/DisableChecks", CARLA_DEFAULT_DISABLE_CHECKS, type=bool))
            self.ch_engine_dssi_chunks.setChecked(self.settings.value("Engine/UseDssiVstChunks", CARLA_DEFAULT_USE_DSSI_VST_CHUNKS, type=bool))
            self.ch_engine_prefer_plugin_bridges.setChecked(self.settings.value("Engine/PreferPluginBridges", CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool))
            self.ch_engine_force_stereo.setChecked(self.settings.value("Engine/ForceStereo", CARLA_DEFAULT_FORCE_STEREO, type=bool))
            self.ch_engine_process_hp.setChecked(self.settings.value("Engine/ProcessHighPrecision", CARLA_DEFAULT_PROCESS_HIGH_PRECISION, type=bool))

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_PATHS):
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
            audioDriver = self.cb_engine_audio_driver.currentText()
            self.settings.setValue("Engine/AudioDriver", audioDriver)

            if audioDriver == "JACK":
                self.settings.setValue("Engine/ProcessMode", self.cb_engine_process_mode_jack.currentIndex())
            else:
                self.settings.setValue("Engine/ProcessMode", self.cb_engine_process_mode_other.currentIndex()+PROCESS_MODE_NON_JACK_PADDING)

            self.settings.setValue("Engine/MaxParameters", self.sb_engine_max_params.value())
            self.settings.setValue("Engine/PreferUiBridges", self.ch_engine_prefer_ui_bridges.isChecked())
            self.settings.setValue("Engine/OscUiTimeout", self.sb_engine_oscgui_timeout.value())
            self.settings.setValue("Engine/DisableChecks", self.ch_engine_disable_checks.isChecked())
            self.settings.setValue("Engine/UseDssiVstChunks", self.ch_engine_dssi_chunks.isChecked())
            self.settings.setValue("Engine/PreferPluginBridges", self.ch_engine_prefer_plugin_bridges.isChecked())
            self.settings.setValue("Engine/ForceStereo", self.ch_engine_force_stereo.isChecked())
            self.settings.setValue("Engine/ProcessHighPrecision", self.ch_engine_process_hp.isChecked())

        # --------------------------------------------

        if not self.lw_page.isRowHidden(TAB_INDEX_CARLA_PATHS):
            # FIXME - find a cleaner way to do this, *.items() ?
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

        elif self.lw_page.currentRow() == TAB_INDEX_CARLA_ENGINE:
            self.cb_engine_audio_driver.setCurrentIndex(0)
            self.sb_engine_max_params.setValue(CARLA_DEFAULT_MAX_PARAMETERS)
            self.ch_engine_prefer_ui_bridges.setChecked(CARLA_DEFAULT_PREFER_UI_BRIDGES)
            self.sb_engine_oscgui_timeout.setValue(CARLA_DEFAULT_OSC_UI_TIMEOUT)
            self.ch_engine_disable_checks.setChecked(CARLA_DEFAULT_DISABLE_CHECKS)
            self.ch_engine_dssi_chunks.setChecked(CARLA_DEFAULT_USE_DSSI_VST_CHUNKS)
            self.ch_engine_prefer_plugin_bridges.setChecked(CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES)
            self.ch_engine_force_stereo.setChecked(CARLA_DEFAULT_FORCE_STEREO)
            self.ch_engine_process_hp.setChecked(CARLA_DEFAULT_PROCESS_HIGH_PRECISION)

            if self.cb_engine_audio_driver.currentText() == "JACK":
                self.cb_engine_process_mode_jack.setCurrentIndex(PROCESS_MODE_MULTIPLE_CLIENTS)
                self.sw_engine_process_mode.setCurrentIndex(0)
            else:
                self.cb_engine_process_mode_other.setCurrentIndex(PROCESS_MODE_CONTINUOUS_RACK-PROCESS_MODE_NON_JACK_PADDING)
                self.sw_engine_process_mode.setCurrentIndex(1)

        elif self.lw_page.currentRow() == TAB_INDEX_CARLA_PATHS:
            ladspas, dssis, lv2s, vsts, gigs, sf2s, sfzs = SETTINGS_DEFAULT_PLUGINS_PATHS

            if self.tw_paths.currentIndex() == 0:
                ladspas.sort()
                self.lw_ladspa.clear()

                for ladspa in ladspas:
                    self.lw_ladspa.addItem(ladspa)

            elif self.tw_paths.currentIndex() == 1:
                dssis.sort()
                self.lw_dssi.clear()

                for dssi in dssis:
                    self.lw_dssi.addItem(dssi)

            elif self.tw_paths.currentIndex() == 2:
                lv2s.sort()
                self.lw_lv2.clear()

                for lv2 in lv2s:
                    self.lw_lv2.addItem(lv2)

            elif self.tw_paths.currentIndex() == 3:
                vsts.sort()
                self.lw_vst.clear()

                for vst in vsts:
                    self.lw_vst.addItem(vst)

            elif self.tw_paths.currentIndex() == 4:
                gigs.sort()
                self.lw_gig.clear()

                for gig in gigs:
                    self.lw_gig.addItem(gig)

            elif self.tw_paths.currentIndex() == 5:
                sf2s.sort()
                self.lw_sf2.clear()

                for sf2 in sf2s:
                    self.lw_sf2.addItem(sf2)

            elif self.tw_paths.currentIndex() == 6:
                sfzs.sort()
                self.lw_sfz.clear()

                for sfz in sfzs:
                    self.lw_sfz.addItem(sfz)

    @pyqtSlot()
    def slot_getAndSetProjectPath(self):
        getAndSetPath(self, self.le_main_def_folder.text(), self.le_main_def_folder)

    @pyqtSlot()
    def slot_engineAudioDriverChanged(self):
        if self.cb_engine_audio_driver.currentText() == "JACK":
            self.sw_engine_process_mode.setCurrentIndex(0)
        else:
            self.sw_engine_process_mode.setCurrentIndex(1)

    @pyqtSlot()
    def slot_addPluginPath(self):
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
    def slot_removePluginPath(self):
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
    def slot_changePluginPath(self):
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
    def slot_pluginPathTabChanged(self, index):
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
    def slot_pluginPathRowChanged(self, row):
        check = bool(row >= 0)
        self.b_paths_remove.setEnabled(check)
        self.b_paths_change.setEnabled(check)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

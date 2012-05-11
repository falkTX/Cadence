#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

# TODO - options:
# - max parameters
# - osc gui timeout

# Imports (Global)
import json, os, sys
from sip import unwrapinstance
from PyQt4.QtCore import pyqtSlot, Qt, QSettings, QTimer, QThread
from PyQt4.QtGui import QApplication, QColor, QCursor, QDialog, QFontMetrics, QInputDialog, QFrame, QMainWindow, QMenu, QPainter, QTableWidgetItem, QVBoxLayout, QWidget
from PyQt4.QtXml import QDomDocument

# Imports (Custom Stuff)
import ui_carla, ui_carla_about, ui_carla_database, ui_carla_edit, ui_carla_parameter, ui_carla_plugin, ui_carla_refresh
from carla_backend import *
from shared_settings import *

ICON_STATE_NULL = 0
ICON_STATE_WAIT = 1
ICON_STATE_OFF  = 2
ICON_STATE_ON   = 3

PALETTE_COLOR_NONE   = 0
PALETTE_COLOR_WHITE  = 1
PALETTE_COLOR_RED    = 2
PALETTE_COLOR_GREEN  = 3
PALETTE_COLOR_BLUE   = 4
PALETTE_COLOR_YELLOW = 5
PALETTE_COLOR_ORANGE = 6
PALETTE_COLOR_BROWN  = 7
PALETTE_COLOR_PINK   = 8

# Save support
save_state_dict = {
    'Type': "",
    'Name': "",
    'Label': "",
    'Binary': "",
    'UniqueID': 0,
    'Active': False,
    'DryWet': 1.0,
    'Volume': 1.0,
    'Balance-Left': -1.0,
    'Balance-Right': 1.0,
    'Parameters': [],
    'CurrentProgramIndex': -1,
    'CurrentProgramName': "",
    'CurrentMidiBank': -1,
    'CurrentMidiProgram': -1,
    'CustomData': [],
    'Chunk': None
}

save_state_parameter = {
    'index': 0,
    'name': "",
    'symbol': "",
    'value': 0.0,
    'midi_channel': 1,
    'midi_cc': -1
}

save_state_custom_data = {
    'type': CUSTOM_DATA_INVALID,
    'key': "",
    'value': ""
}

# set defaults
DEFAULT_PROJECT_FOLDER = HOME
setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)
setDefaultPluginsPaths(LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, SF2_PATH)

def CustomDataType2String(dtype):
    if dtype == CUSTOM_DATA_STRING:
        return "string"
    elif dtype == CUSTOM_DATA_PATH:
        return "path"
    elif dtype == CUSTOM_DATA_CHUNK:
        return "chunk"
    elif dtype == CUSTOM_DATA_BINARY:
        return "binary"
    else:
        return "null"

def CustomDataString2Type(stype):
    if stype == "string":
        return CUSTOM_DATA_STRING
    elif stype == "path":
        return CUSTOM_DATA_PATH
    elif stype == "chunk":
        return CUSTOM_DATA_CHUNK
    elif stype == "binary":
        return CUSTOM_DATA_BINARY
    else:
        return CUSTOM_DATA_INVALID

def getStateDictFromXML(xml_node):
    x_save_state_dict = deepcopy(save_state_dict)

    node = xml_node.firstChild()
    while not node.isNull():
        if (node.toElement().tagName() == "Info"):
            xml_info = node.toElement().firstChild()

            while not xml_info.isNull():
                tag = xml_info.toElement().tagName()
                text = xml_info.toElement().text().strip()

                if tag == "Type":
                    x_save_state_dict['Type'] = text
                elif tag == "Name":
                    x_save_state_dict['Name'] = text
                elif tag in ("Label", "URI"):
                    x_save_state_dict['Label'] = text
                elif tag == "Binary":
                    x_save_state_dict['Binary'] = text
                elif tag == "UniqueID":
                    if text.isdigit():
                        x_save_state_dict['UniqueID'] = int(text)

                xml_info = xml_info.nextSibling()

        elif (node.toElement().tagName() == "Data"):
            xml_data = node.toElement().firstChild()

            while not xml_data.isNull():
                tag = xml_data.toElement().tagName()
                text = xml_data.toElement().text().strip()

                if tag == "Active":
                    x_save_state_dict['Active'] = bool(text == "Yes")
                elif tag == "DryWet":
                    if isNumber(text):
                        x_save_state_dict['DryWet'] = float(text)
                elif (tag == "Vol"):
                    if isNumber(text):
                        x_save_state_dict['Volume'] = float(text)
                elif (tag == "Balance-Left"):
                    if isNumber(text):
                        x_save_state_dict['Balance-Left'] = float(text)
                elif (tag == "Balance-Right"):
                    if isNumber(text):
                        x_save_state_dict['Balance-Right'] = float(text)

                elif tag == "CurrentProgramIndex":
                    if text.isdigit():
                        x_save_state_dict['CurrentProgramIndex'] = int(text)
                elif tag == "CurrentProgramName":
                    x_save_state_dict['CurrentProgramName'] = text

                elif tag == "CurrentMidiBank":
                    if text.isdigit():
                        x_save_state_dict['CurrentMidiBank'] = int(text)
                elif tag == "CurrentMidiProgram":
                    if text.isdigit():
                        x_save_state_dict['CurrentMidiProgram'] = int(text)

                elif tag == "Parameter":
                    x_save_state_parameter = deepcopy(save_state_parameter)

                    xml_subdata = xml_data.toElement().firstChild()

                    while not xml_subdata.isNull():
                        ptag = xml_subdata.toElement().tagName()
                        ptext = xml_subdata.toElement().text().strip()

                        if ptag == "index":
                            if ptext.isdigit():
                                x_save_state_parameter['index'] = int(ptext)
                        elif ptag == "name":
                            x_save_state_parameter['name'] = ptext
                        elif ptag == "symbol":
                            x_save_state_parameter['symbol'] = ptext
                        elif ptag == "value":
                            if isNumber(ptext):
                                x_save_state_parameter['value'] = float(ptext)
                        elif ptag == "midi_channel":
                            if ptext.isdigit():
                                x_save_state_parameter['midi_channel'] = int(ptext)
                        elif ptag == "midi_cc":
                            if ptext.isdigit():
                                x_save_state_parameter['midi_cc'] = int(ptext)

                        xml_subdata = xml_subdata.nextSibling()

                    x_save_state_dict['Parameters'].append(x_save_state_parameter)

                elif (tag == "CustomData"):
                    x_save_state_custom_data = deepcopy(save_state_custom_data)

                    xml_subdata = xml_data.toElement().firstChild()

                    while not xml_subdata.isNull():
                        ctag = xml_subdata.toElement().tagName()
                        ctext = xml_subdata.toElement().text().strip()

                        if ctag == "type":
                            x_save_state_custom_data['type'] = CustomDataString2Type(ctext)
                        elif ctag == "key":
                            x_save_state_custom_data['key'] = ctext.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","\\").replace("&quot;","\"")
                        elif ctag == "value":
                            x_save_state_custom_data['value'] = ctext.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","\\").replace("&quot;","\"")

                        xml_subdata = xml_subdata.nextSibling()

                    x_save_state_dict['CustomData'].append(x_save_state_custom_data)

                elif tag == "Chunk":
                    x_save_state_dict['Chunk'] = text

                xml_data = xml_data.nextSibling()

        node = node.nextSibling()

    return x_save_state_dict

# Separate Thread for Plugin Search
class SearchPluginsThread(QThread):
    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.settings_db = self.parent().settings_db

        self.check_ladspa = True
        self.check_dssi = True
        self.check_lv2 = True
        self.check_vst = True
        self.check_sf2 = True

        self.check_native = None
        self.check_bins = []

    def skipPlugin(self):
        # TODO - windows and mac support
        apps = ""
        apps += " carla-discovery"
        apps += " carla-discovery-unix32"
        apps += " carla-discovery-unix64"
        apps += " carla-discovery-win32.exe"
        apps += " carla-discovery-win64.exe"

        if LINUX:
            os.system("killall -KILL %s" % apps)

    def pluginLook(self, percent, plugin):
        self.emit(SIGNAL("PluginLook(int, QString)"), percent, plugin)

    def setSearchBins(self, bins):
        self.check_bins = bins

    def setSearchNative(self, native):
        self.check_native = native

    def setSearchTypes(self, ladspa, dssi, lv2, vst, sf2):
        self.check_ladspa = ladspa
        self.check_dssi = dssi
        self.check_lv2 = lv2
        self.check_vst = vst
        self.check_sf2 = sf2

    def setLastLoadedBinary(self, binary):
        self.settings_db.setValue("Plugins/LastLoadedBinary", binary)

    def run(self):
        # TODO - split across several fuctions
        global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, SF2_PATH
        os.environ['LADSPA_PATH'] = splitter.join(LADSPA_PATH)
        os.environ['DSSI_PATH'] = splitter.join(DSSI_PATH)
        os.environ['LV2_PATH'] = splitter.join(LV2_PATH)
        os.environ['VST_PATH'] = splitter.join(VST_PATH)
        os.environ['SF2_PATH'] = splitter.join(SF2_PATH)

        blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))
        bins = []
        bins_w = []

        m_count = type_count = 0
        if self.check_ladspa: m_count += 1
        if self.check_dssi:   m_count += 1
        if self.check_lv2:    m_count += 1
        if self.check_vst:    m_count += 1

        check_native = check_wine = False

        if LINUX:
            OS = "LINUX"
        elif MACOS:
            OS = "MACOS"
        elif WINDOWS:
            OS = "WINDOWS"
        else:
            OS = "UNKNOWN"

        if LINUX or MACOS:
            if carla_discovery_unix32 in self.check_bins or carla_discovery_unix64 in self.check_bins:
                type_count += m_count
                check_native = True
                if carla_discovery_unix32 in self.check_bins:
                    bins.append(carla_discovery_unix32)
                if carla_discovery_unix64 in self.check_bins:
                    bins.append(carla_discovery_unix64)

            if carla_discovery_win32 in self.check_bins or carla_discovery_win64 in self.check_bins:
                type_count += m_count
                check_wine = True
                if carla_discovery_win32 in self.check_bins:
                    bins_w.append(carla_discovery_win32)
                if carla_discovery_win64 in self.check_bins:
                    bins_w.append(carla_discovery_win64)

        elif WINDOWS:
            if carla_discovery_win32 in self.check_bins or carla_discovery_win64 in self.check_bins:
                type_count += m_count
                check_native = True
                if carla_discovery_win32 in self.check_bins:
                    bins.append(carla_discovery_win32)
                if carla_discovery_win64 in self.check_bins:
                    bins.append(carla_discovery_win64)

        if self.check_sf2: type_count += 1

        if type_count == 0:
            return

        ladspa_plugins = []
        dssi_plugins = []
        lv2_plugins = []
        vst_plugins = []
        soundfonts = []

        ladspa_rdf_info = []

        last_value = 0
        percent_value = 100 / type_count

        # ----- LADSPA
        if (self.check_ladspa):
            if (check_native):
                ladspa_binaries = []

                for iPATH in LADSPA_PATH:
                    binaries = findBinaries(iPATH, OS)
                    for binary in binaries:
                        if (binary not in ladspa_binaries):
                            ladspa_binaries.append(binary)

                ladspa_binaries.sort()

                for i in range(len(ladspa_binaries)):
                    ladspa = ladspa_binaries[i]
                    if (getShortFileName(ladspa) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (ladspa))
                        continue
                    else:
                        percent = ( float(i) / len(ladspa_binaries) ) * percent_value
                        self.pluginLook((last_value + percent) * 0.9, ladspa)
                        self.setLastLoadedBinary(ladspa)
                        for bin_ in bins:
                            plugins = checkPluginLADSPA(ladspa, bin_)
                            if (plugins != None):
                                ladspa_plugins.append(plugins)

                last_value += percent_value

            if (check_wine):
                ladspa_binaries_w = []

                for iPATH in LADSPA_PATH:
                    binaries = findBinaries(iPATH, "WINDOWS")
                    for binary in binaries:
                        if (binary not in ladspa_binaries_w):
                            ladspa_binaries_w.append(binary)

                ladspa_binaries_w.sort()

                # Check binaries, wine
                for i in range(len(ladspa_binaries_w)):
                    ladspa_w = ladspa_binaries_w[i]
                    if (getShortFileName(ladspa_w) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (ladspa_w))
                        continue
                    else:
                        percent = ( float(i) / len(ladspa_binaries_w) ) * percent_value
                        self.pluginLook((last_value + percent) * 0.9, ladspa_w)
                        self.setLastLoadedBinary(ladspa_w)
                        for bin_w in bins_w:
                            plugins_w = checkPluginLADSPA(ladspa_w, bin_w, True)
                            if (plugins_w != None):
                                ladspa_plugins.append(plugins_w)

                last_value += percent_value

            if (haveLRDF):
                m_value = 0
                if (check_native): m_value += 0.1
                if (check_wine):   m_value += 0.1

                if (m_value > 0):
                    start_value = last_value - (percent_value * m_value)

                    self.pluginLook(start_value, "LADSPA RDFs...")
                    ladspa_rdf_info = ladspa_rdf.recheck_all_plugins(self, start_value, percent_value, m_value)

        # ----- DSSI
        if (self.check_dssi):
            if (check_native):
                dssi_binaries = []

                for iPATH in DSSI_PATH:
                    binaries = findBinaries(iPATH, OS)
                    for binary in binaries:
                        if (binary not in dssi_binaries):
                            dssi_binaries.append(binary)

                dssi_binaries.sort()

                for i in range(len(dssi_binaries)):
                    dssi = dssi_binaries[i]
                    if (getShortFileName(dssi) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (dssi))
                        continue
                    else:
                        percent = ( float(i) / len(dssi_binaries) ) * percent_value
                        self.pluginLook(last_value + percent, dssi)
                        self.setLastLoadedBinary(dssi)
                        for bin_ in bins:
                            plugins = checkPluginDSSI(dssi, bin_)
                            if (plugins != None):
                                dssi_plugins.append(plugins)

                last_value += percent_value

            if (check_wine):
                dssi_binaries_w = []

                for iPATH in DSSI_PATH:
                    binaries = findBinaries(iPATH, "WINDOWS")
                    for binary in binaries:
                        if (binary not in dssi_binaries_w):
                            dssi_binaries_w.append(binary)

                dssi_binaries_w.sort()

                # Check binaries, wine
                for i in range(len(dssi_binaries_w)):
                    dssi_w = dssi_binaries_w[i]
                    if (getShortFileName(dssi_w) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (dssi_w))
                        continue
                    else:
                        percent = ( float(i) / len(dssi_binaries_w) ) * percent_value
                        self.pluginLook(last_value + percent, dssi_w)
                        self.setLastLoadedBinary(dssi_w)
                        for bin_w in bins_w:
                            plugins_w = checkPluginDSSI(dssi_w, bin_w, True)
                            if (plugins_w != None):
                                dssi_plugins.append(plugins_w)

                last_value += percent_value

        # ----- LV2
        if (self.check_lv2):
            self.pluginLook(last_value, "LV2 bundles...")
            lv2_bundles = []

            for iPATH in LV2_PATH:
                bundles = findLV2Bundles(iPATH)
                for bundle in bundles:
                    if (bundle not in lv2_bundles):
                        lv2_bundles.append(bundle)

            lv2_bundles.sort()

            if (check_native):
                for i in range(len(lv2_bundles)):
                    lv2 = lv2_bundles[i]
                    if (getShortFileName(lv2) in blacklist):
                        print("bundle %s is blacklisted, skip it" % (lv2))
                        continue
                    else:
                        percent = ( float(i) / len(lv2_bundles) ) * percent_value
                        self.pluginLook(last_value + percent, lv2)
                        self.setLastLoadedBinary(lv2)
                        for bin_ in bins:
                            plugins = checkPluginLV2(lv2, bin_)
                            if (plugins != None):
                                lv2_plugins.append(plugins)

                last_value += percent_value

            if (check_wine):
                # Check binaries, wine
                for i in range(len(lv2_bundles)):
                    lv2_w = lv2_bundles[i]
                    if (getShortFileName(lv2_w) in blacklist):
                        print("bundle %s is blacklisted, skip it" % (lv2_w))
                        continue
                    else:
                        percent = ( float(i) / len(lv2_bundles) ) * percent_value
                        self.pluginLook(last_value + percent, lv2_w)
                        self.setLastLoadedBinary(lv2_w)
                        for bin_w in bins_w:
                            plugins = checkPluginLV2(lv2_w, bin_w)
                            if (plugins != None):
                                lv2_plugins.append(plugins)

                last_value += percent_value

        # ----- VST
        if (self.check_vst):
            if (check_native):
                vst_binaries = []

                for iPATH in VST_PATH:
                    binaries = findBinaries(iPATH, OS)
                    for binary in binaries:
                        if (binary not in vst_binaries):
                            vst_binaries.append(binary)

                vst_binaries.sort()

                for i in range(len(vst_binaries)):
                    vst = vst_binaries[i]
                    if (getShortFileName(vst) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (vst))
                        continue
                    else:
                        percent = ( float(i) / len(vst_binaries) ) * percent_value
                        self.pluginLook(last_value + percent, vst)
                        self.setLastLoadedBinary(vst)
                        for bin_ in bins:
                            plugins = checkPluginVST(vst, bin_)
                            if (plugins != None):
                                vst_plugins.append(plugins)

                last_value += percent_value

            if (check_wine):
                vst_binaries_w = []

                for iPATH in VST_PATH:
                    binaries = findBinaries(iPATH, "WINDOWS")
                    for binary in binaries:
                        if (binary not in vst_binaries_w):
                            vst_binaries_w.append(binary)

                vst_binaries_w.sort()

                # Check binaries, wine
                for i in range(len(vst_binaries_w)):
                    vst_w = vst_binaries_w[i]
                    if (getShortFileName(vst_w) in blacklist):
                        print("plugin %s is blacklisted, skip it" % (vst_w))
                        continue
                    else:
                        percent = ( float(i) / len(vst_binaries_w) ) * percent_value
                        self.pluginLook(last_value + percent, vst_w)
                        self.setLastLoadedBinary(vst_w)
                        for bin_w in bins_w:
                            plugins_w = checkPluginVST(vst_w, bin_w, True)
                            if (plugins_w != None):
                                vst_plugins.append(plugins_w)

                last_value += percent_value

        # ----- SF2
        if (self.check_sf2):
            sf2_files = []
            for iPATH in SF2_PATH:
                files = findSoundFonts(iPATH)
                for file_ in files:
                    if (file_ not in sf2_files):
                        sf2_files.append(file_)

            for i in range(len(sf2_files)):
                sf2 = sf2_files[i]
                if (getShortFileName(sf2) in blacklist):
                    print("soundfont %s is blacklisted, skip it" % (sf2))
                    continue
                else:
                    percent = (( float(i) / len(sf2_files) ) * percent_value)
                    self.pluginLook(last_value + percent, sf2)
                    self.setLastLoadedBinary(sf2)
                    soundfont = checkPluginSF2(sf2, self.check_native)
                    if (soundfont):
                        soundfonts.append(soundfont)

        self.setLastLoadedBinary("")

        # Save plugins to database
        self.pluginLook(100, "Database...")

        if (self.check_ladspa):
            self.settings_db.setValue("Plugins/LADSPA", ladspa_plugins)

        if (self.check_dssi):
            self.settings_db.setValue("Plugins/DSSI", dssi_plugins)

        if (self.check_lv2):
            self.settings_db.setValue("Plugins/LV2", lv2_plugins)

        if (self.check_vst):
            self.settings_db.setValue("Plugins/VST", vst_plugins)

        if (self.check_sf2):
            self.settings_db.setValue("Plugins/SF2", soundfonts)

        self.settings_db.sync()

        if (haveLRDF):
            SettingsDir = os.path.join(HOME, ".config", "Cadence")

            if (self.check_ladspa):
                f_ladspa = open(os.path.join(SettingsDir, "ladspa_rdf.db"), 'w')
                if (f_ladspa):
                    json.dump(ladspa_rdf_info, f_ladspa)
                    f_ladspa.close()

# Plugin Refresh Dialog
class PluginRefreshW(QDialog, ui_carla_refresh.Ui_PluginRefreshW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.b_skip.setVisible(False)

        if (LINUX):
            self.ch_unix32.setText("Linux 32bit")
            self.ch_unix64.setText("Linux 64bit")
        elif (MACOS):
            self.ch_unix32.setText("MacOS 32bit")
            self.ch_unix64.setText("MacOS 64bit")

        self.settings = self.parent().settings
        self.settings_db = self.parent().settings_db
        self.loadSettings()

        self.pThread = SearchPluginsThread(self)

        if (carla_discovery_unix32 and not WINDOWS):
            self.ico_unix32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_unix32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_unix32.setChecked(False)
            self.ch_unix32.setEnabled(False)

        if (carla_discovery_unix64 and not WINDOWS):
            self.ico_unix64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_unix64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_unix64.setChecked(False)
            self.ch_unix64.setEnabled(False)

        if (carla_discovery_win32):
            self.ico_win32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_win32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_win32.setChecked(False)
            self.ch_win32.setEnabled(False)

        if (carla_discovery_win64):
            self.ico_win64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_win64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_win64.setChecked(False)
            self.ch_win64.setEnabled(False)

        if haveLRDF:
            self.ico_rdflib.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_rdflib.setPixmap(getIcon("dialog-error").pixmap(16, 16))

        hasNative = False
        hasNonNative = False

        if (LINUX or MACOS):
            if (is64bit):
                hasNative = bool(carla_discovery_unix64)
                hasNonNative = bool(carla_discovery_unix32 or carla_discovery_win32 or carla_discovery_win64)
                self.pThread.setSearchNative(carla_discovery_unix64)
            else:
                hasNative = bool(carla_discovery_unix32)
                hasNonNative = bool(carla_discovery_unix64 or carla_discovery_win32 or carla_discovery_win64)
                self.pThread.setSearchNative(carla_discovery_unix32)
        elif (WINDOWS):
            if (is64bit):
                hasNative = bool(carla_discovery_win64)
                hasNonNative = bool(carla_discovery_win32)
                self.pThread.setSearchNative(carla_discovery_win64)
            else:
                hasNative = bool(carla_discovery_win32)
                hasNonNative = bool(carla_discovery_win64)
                self.pThread.setSearchNative(carla_discovery_win32)
        else:
            hasNative = False

        if not hasNative:
            self.ch_sf2.setChecked(False)
            self.ch_sf2.setEnabled(False)

            if not hasNonNative:
                self.ch_ladspa.setChecked(False)
                self.ch_ladspa.setEnabled(False)
                self.ch_dssi.setChecked(False)
                self.ch_dssi.setEnabled(False)
                self.ch_vst.setChecked(False)
                self.ch_vst.setEnabled(False)

                if not haveLRDF:
                    self.b_refresh.setEnabled(False)

        self.connect(self.b_refresh, SIGNAL("clicked()"), SLOT("slot_refresh_plugins()"))
        self.connect(self.b_skip, SIGNAL("clicked()"), SLOT("slot_skip()"))
        self.connect(self.pThread, SIGNAL("PluginLook(int, QString)"), SLOT("slot_handlePluginLook(int, QString)"))
        self.connect(self.pThread, SIGNAL("finished()"), SLOT("slot_handlePluginThreadFinished()"))

    @pyqtSlot()
    def slot_refresh_plugins(self):
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(100)
        self.progressBar.setValue(0)
        self.b_refresh.setEnabled(False)
        self.b_skip.setVisible(True)
        self.b_close.setVisible(False)

        bins = []
        if (self.ch_unix32.isChecked()):
            bins.append(carla_discovery_unix32)
        if (self.ch_unix64.isChecked()):
            bins.append(carla_discovery_unix64)
        if (self.ch_win32.isChecked()):
            bins.append(carla_discovery_win32)
        if (self.ch_win64.isChecked()):
            bins.append(carla_discovery_win64)

        self.pThread.setSearchBins(bins)
        self.pThread.setSearchTypes(self.ch_ladspa.isChecked(), self.ch_dssi.isChecked(), self.ch_lv2.isChecked(), self.ch_vst.isChecked(), self.ch_sf2.isChecked())
        self.pThread.start()

    @pyqtSlot()
    def slot_skip(self):
        self.pThread.skipPlugin()

    @pyqtSlot(int, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.progressBar.setFormat("%s" % (plugin))
        self.progressBar.setValue(percent)

    @pyqtSlot()
    def slot_handlePluginThreadFinished(self):
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(1)
        self.progressBar.setValue(1)
        self.progressBar.setFormat(self.tr("Done"))
        self.b_refresh.setEnabled(True)
        self.b_skip.setVisible(False)
        self.b_close.setVisible(True)

    def saveSettings(self):
        self.settings.setValue("PluginDatabase/SearchLADSPA", self.ch_ladspa.isChecked())
        self.settings.setValue("PluginDatabase/SearchDSSI", self.ch_dssi.isChecked())
        self.settings.setValue("PluginDatabase/SearchLV2", self.ch_lv2.isChecked())
        self.settings.setValue("PluginDatabase/SearchVST", self.ch_vst.isChecked())
        self.settings.setValue("PluginDatabase/SearchSF2", self.ch_sf2.isChecked())
        self.settings.setValue("PluginDatabase/SearchUnix32", self.ch_unix32.isChecked())
        self.settings.setValue("PluginDatabase/SearchUnix64", self.ch_unix64.isChecked())
        self.settings.setValue("PluginDatabase/SearchWin32", self.ch_win32.isChecked())
        self.settings.setValue("PluginDatabase/SearchWin64", self.ch_win64.isChecked())
        self.settings_db.setValue("Plugins/LastLoadedBinary", "")

    def loadSettings(self):
        self.ch_ladspa.setChecked(self.settings.value("PluginDatabase/SearchLADSPA", True, type=bool))
        self.ch_dssi.setChecked(self.settings.value("PluginDatabase/SearchDSSI", True, type=bool))
        self.ch_lv2.setChecked(self.settings.value("PluginDatabase/SearchLV2", True, type=bool))
        self.ch_vst.setChecked(self.settings.value("PluginDatabase/SearchVST", True, type=bool))
        self.ch_sf2.setChecked(self.settings.value("PluginDatabase/SearchSF2", True, type=bool))
        self.ch_unix32.setChecked(self.settings.value("PluginDatabase/SearchUnix32", True, type=bool))
        self.ch_unix64.setChecked(self.settings.value("PluginDatabase/SearchUnix64", True, type=bool))
        self.ch_win32.setChecked(self.settings.value("PluginDatabase/SearchWin32", True, type=bool))
        self.ch_win64.setChecked(self.settings.value("PluginDatabase/SearchWin64", True, type=bool))

    def closeEvent(self, event):
        if (self.pThread.isRunning()):
            self.pThread.terminate()
            self.pThread.wait()
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Plugin Database Dialog
class PluginDatabaseW(QDialog, ui_carla_database.Ui_PluginDatabaseW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.warning_old_shown = False
        self.b_add.setEnabled(False)

        if (BINARY_NATIVE in (BINARY_UNIX32, BINARY_WIN32)):
            self.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ch_bridged.setText(self.tr("Bridged (32bit)"))

        self.settings = self.parent().settings
        self.settings_db = self.parent().settings_db
        self.loadSettings()

        if (not (LINUX or MACOS)):
            self.ch_bridged_wine.setChecked(False)
            self.ch_bridged_wine.setEnabled(False)

        # Blacklist plugins
        if not self.settings_db.contains("Plugins/Blacklisted"):
            blacklist = [] # FIXME
            # Broken or useless plugins
            #blacklist.append("dssi-vst.so")
            blacklist.append("liteon_biquad-vst.so")
            blacklist.append("liteon_biquad-vst_64bit.so")
            blacklist.append("fx_blur-vst.so")
            blacklist.append("fx_blur-vst_64bit.so")
            blacklist.append("fx_tempodelay-vst.so")
            blacklist.append("Scrubby_64bit.so")
            blacklist.append("Skidder_64bit.so")
            blacklist.append("libwormhole2_64bit.so")
            blacklist.append("vexvst.so")
            #blacklist.append("deckadance.dll")
            self.settings_db.setValue("Plugins/Blacklisted", blacklist)

        self.connect(self.b_add, SIGNAL("clicked()"), SLOT("slot_add_plugin()"))
        self.connect(self.b_refresh, SIGNAL("clicked()"), SLOT("slot_refresh_plugins()"))
        self.connect(self.tb_filters, SIGNAL("clicked()"), SLOT("slot_maybe_show_filters()"))
        self.connect(self.tableWidget, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkPlugin(int)"))
        self.connect(self.tableWidget, SIGNAL("cellDoubleClicked(int, int)"), SLOT("slot_add_plugin()"))

        self.connect(self.lineEdit, SIGNAL("textChanged(QString)"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_effects, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_instruments, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_midi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_other, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_sf2, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_ladspa, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_dssi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_lv2, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_vst, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_native, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_bridged, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_bridged_wine, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_gui, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_stereo, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))

        self.ret_plugin = None

    def showFilters(self, yesno):
        if (yesno):
            arrow = Qt.UpArrow
        else:
            arrow = Qt.DownArrow

        self.tb_filters.setArrowType(arrow)
        self.frame.setVisible(yesno)

    def reAddPlugins(self):
        row_count = self.tableWidget.rowCount()
        for x in range(row_count):
            self.tableWidget.removeRow(0)

        self.last_table_index = 0
        self.tableWidget.setSortingEnabled(False)

        ladspa_plugins = toList(self.settings_db.value("Plugins/LADSPA", []))
        dssi_plugins = toList(self.settings_db.value("Plugins/DSSI", []))
        lv2_plugins = toList(self.settings_db.value("Plugins/LV2", []))
        vst_plugins = toList(self.settings_db.value("Plugins/VST", []))
        soundfonts = toList(self.settings_db.value("Plugins/SF2", []))

        ladspa_count = 0
        dssi_count = 0
        lv2_count = 0
        vst_count = 0
        sf2_count = 0

        for plugins in ladspa_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "LADSPA")
                ladspa_count += 1

        for plugins in dssi_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "DSSI")
                dssi_count += 1

        for plugins in lv2_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "LV2")
                lv2_count += 1

        for plugins in vst_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "VST")
                vst_count += 1

        for soundfonts_i in soundfonts:
            for soundfont in soundfonts_i:
                self.addPluginToTable(soundfont, "SF2")
                sf2_count += 1

        self.slot_checkFilters()
        self.tableWidget.setSortingEnabled(True)
        self.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        self.label.setText(self.tr("Have %i LADSPA, %i DSSI, %i LV2, %i VST and %i SoundFonts" % (ladspa_count, dssi_count, lv2_count, vst_count, sf2_count)))

    def addPluginToTable(self, plugin, ptype):
        index = self.last_table_index

        if ("build" not in plugin.keys()):
            if (self.warning_old_shown == False):
                QMessageBox.warning(self, self.tr("Warning"), self.tr("You're using a Carla-Database from an old version of Carla, please update the plugins"))
                self.warning_old_shown = True
            return

        if (plugin['build'] == BINARY_NATIVE):
            bridge_text = self.tr("No")
        else:
            type_text = self.tr("Unknown")
            if (LINUX or MACOS):
                if (plugin['build'] == BINARY_UNIX32):
                    type_text = "32bit"
                elif (plugin['build'] == BINARY_UNIX64):
                    type_text = "64bit"
                elif (plugin['build'] == BINARY_WIN32):
                    type_text = "Windows 32bit"
                elif (plugin['build'] == BINARY_WIN64):
                    type_text = "Windows 64bit"
            elif (WINDOWS):
                if (plugin['build'] == BINARY_WIN32):
                    type_text = "32bit"
                elif (plugin['build'] == BINARY_WIN64):
                    type_text = "64bit"
            bridge_text = self.tr("Yes (%s)" % (type_text))

        self.tableWidget.insertRow(index)
        self.tableWidget.setItem(index, 0, QTableWidgetItem(plugin['name']))
        self.tableWidget.setItem(index, 1, QTableWidgetItem(plugin['label']))
        self.tableWidget.setItem(index, 2, QTableWidgetItem(plugin['maker']))
        self.tableWidget.setItem(index, 3, QTableWidgetItem(str(plugin['unique_id'])))
        self.tableWidget.setItem(index, 4, QTableWidgetItem(str(plugin['audio.ins'])))
        self.tableWidget.setItem(index, 5, QTableWidgetItem(str(plugin['audio.outs'])))
        self.tableWidget.setItem(index, 6, QTableWidgetItem(str(plugin['parameters.ins'])))
        self.tableWidget.setItem(index, 7, QTableWidgetItem(str(plugin['parameters.outs'])))
        self.tableWidget.setItem(index, 8, QTableWidgetItem(str(plugin['programs.total'])))
        self.tableWidget.setItem(index, 9, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_HAS_GUI) else self.tr("No")))
        self.tableWidget.setItem(index, 10, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_IS_SYNTH) else self.tr("No")))
        self.tableWidget.setItem(index, 11, QTableWidgetItem(bridge_text))
        self.tableWidget.setItem(index, 12, QTableWidgetItem(ptype))
        self.tableWidget.setItem(index, 13, QTableWidgetItem(plugin['binary']))
        self.tableWidget.item(self.last_table_index, 0).plugin_data = plugin
        self.last_table_index += 1

    @pyqtSlot()
    def slot_add_plugin(self):
        if (self.tableWidget.currentRow() >= 0):
            self.ret_plugin = self.tableWidget.item(self.tableWidget.currentRow(), 0).plugin_data
            self.accept()
        else:
            self.reject()

    @pyqtSlot()
    def slot_refresh_plugins(self):
        lastLoadedPlugin = self.settings_db.value("Plugins/LastLoadedBinary", "", type=str)
        if (lastLoadedPlugin):
            lastLoadedPlugin = getShortFileName(lastLoadedPlugin)
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There was an error while checking the plugin %s.\n"
                                                                         "Do you want to blacklist it?" % (lastLoadedPlugin)),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if (ask == QMessageBox.Yes):
                blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))
                blacklist.append(lastLoadedPlugin)
                self.settings_db.setValue("Plugins/Blacklisted", blacklist)

        self.label.setText(self.tr("Looking for plugins..."))
        PluginRefreshW(self).exec_()

        self.reAddPlugins()
        self.parent().loadRDFs()

    @pyqtSlot()
    def slot_maybe_show_filters(self):
        self.showFilters(not self.frame.isVisible())

    @pyqtSlot(int)
    def slot_checkPlugin(self, row):
        self.b_add.setEnabled(row >= 0)

    @pyqtSlot()
    def slot_checkFilters(self):
        text = self.lineEdit.text().lower()
        hide_effects = not self.ch_effects.isChecked()
        hide_instruments = not self.ch_instruments.isChecked()
        hide_midi = not self.ch_midi.isChecked()
        hide_other = not self.ch_other.isChecked()
        hide_ladspa = not self.ch_ladspa.isChecked()
        hide_dssi = not self.ch_dssi.isChecked()
        hide_lv2 = not self.ch_lv2.isChecked()
        hide_vst = not self.ch_vst.isChecked()
        hide_sf2 = not self.ch_sf2.isChecked()
        hide_native = not self.ch_native.isChecked()
        hide_bridged = not self.ch_bridged.isChecked()
        hide_bridged_wine = not self.ch_bridged_wine.isChecked()
        hide_non_gui = self.ch_gui.isChecked()
        hide_non_stereo = self.ch_stereo.isChecked()

        if (LINUX or MACOS):
            native_bins = [BINARY_UNIX32, BINARY_UNIX64]
            wine_bins = [BINARY_WIN32, BINARY_WIN64]
        elif (WINDOWS):
            native_bins = [BINARY_WIN32, BINARY_WIN64]
            wine_bins = []
        else:
            native_bins = []
            wine_bins = []

        row_count = self.tableWidget.rowCount()
        for i in range(row_count):
            self.tableWidget.showRow(i)

            plugin = self.tableWidget.item(i, 0).plugin_data
            ains   = plugin['audio.ins']
            aouts  = plugin['audio.outs']
            mins   = plugin['midi.ins']
            mouts  = plugin['midi.outs']
            ptype  = self.tableWidget.item(i, 12).text()
            is_synth  = bool(plugin['hints'] & PLUGIN_IS_SYNTH)
            is_effect = bool(ains > 0 < aouts and not is_synth)
            is_midi   = bool(ains == 0 and aouts == 0 and mins > 0 < mouts)
            is_sf2    = bool(ptype == "SF2")
            is_other  = bool(not (is_effect or is_synth or is_midi or is_sf2))
            is_native = bool(plugin['build'] == BINARY_NATIVE)
            is_stereo = bool(ains == 2 and aouts == 2) or (is_synth and aouts == 2)
            has_gui   = bool(plugin['hints'] & PLUGIN_HAS_GUI)

            is_bridged = bool(not is_native and plugin['build'] in native_bins)
            is_bridged_wine = bool(not is_native and plugin['build'] in wine_bins)

            if (hide_effects and is_effect):
                self.tableWidget.hideRow(i)
            elif (hide_instruments and is_synth):
                self.tableWidget.hideRow(i)
            elif (hide_midi and is_midi):
                self.tableWidget.hideRow(i)
            elif (hide_other and is_other):
                self.tableWidget.hideRow(i)
            elif (hide_sf2 and is_sf2):
                self.tableWidget.hideRow(i)
            elif (hide_ladspa and ptype == "LADSPA"):
                self.tableWidget.hideRow(i)
            elif (hide_dssi and ptype == "DSSI"):
                self.tableWidget.hideRow(i)
            elif (hide_lv2 and ptype == "LV2"):
                self.tableWidget.hideRow(i)
            elif (hide_vst and ptype == "VST"):
                self.tableWidget.hideRow(i)
            elif (hide_native and is_native):
                self.tableWidget.hideRow(i)
            elif (hide_bridged and is_bridged):
                self.tableWidget.hideRow(i)
            elif (hide_bridged_wine and is_bridged_wine):
                self.tableWidget.hideRow(i)
            elif (hide_non_gui and not has_gui):
                self.tableWidget.hideRow(i)
            elif (hide_non_stereo and not is_stereo):
                self.tableWidget.hideRow(i)
            elif (text and not (
                text in self.tableWidget.item(i, 0).text().lower() or
                text in self.tableWidget.item(i, 1).text().lower() or
                text in self.tableWidget.item(i, 2).text().lower() or
                text in self.tableWidget.item(i, 3).text().lower() or
                text in self.tableWidget.item(i, 13).text().lower())):
                self.tableWidget.hideRow(i)

    def saveSettings(self):
        self.settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        self.settings.setValue("PluginDatabase/TableGeometry", self.tableWidget.horizontalHeader().saveState())
        self.settings.setValue("PluginDatabase/ShowFilters", (self.tb_filters.arrowType() == Qt.UpArrow))
        self.settings.setValue("PluginDatabase/ShowEffects", self.ch_effects.isChecked())
        self.settings.setValue("PluginDatabase/ShowInstruments", self.ch_instruments.isChecked())
        self.settings.setValue("PluginDatabase/ShowMIDI", self.ch_midi.isChecked())
        self.settings.setValue("PluginDatabase/ShowOther", self.ch_other.isChecked())
        self.settings.setValue("PluginDatabase/ShowLADSPA", self.ch_ladspa.isChecked())
        self.settings.setValue("PluginDatabase/ShowDSSI", self.ch_dssi.isChecked())
        self.settings.setValue("PluginDatabase/ShowLV2", self.ch_lv2.isChecked())
        self.settings.setValue("PluginDatabase/ShowVST", self.ch_vst.isChecked())
        self.settings.setValue("PluginDatabase/ShowSF2", self.ch_sf2.isChecked())
        self.settings.setValue("PluginDatabase/ShowNative", self.ch_native.isChecked())
        self.settings.setValue("PluginDatabase/ShowBridged", self.ch_bridged.isChecked())
        self.settings.setValue("PluginDatabase/ShowBridgedWine", self.ch_bridged_wine.isChecked())
        self.settings.setValue("PluginDatabase/ShowHasGUI", self.ch_gui.isChecked())
        self.settings.setValue("PluginDatabase/ShowStereoOnly", self.ch_stereo.isChecked())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("PluginDatabase/Geometry", ""))
        self.tableWidget.horizontalHeader().restoreState(self.settings.value("PluginDatabase/TableGeometry", ""))
        self.showFilters(self.settings.value("PluginDatabase/ShowFilters", False, type=bool))
        self.ch_effects.setChecked(self.settings.value("PluginDatabase/ShowEffects", True, type=bool))
        self.ch_instruments.setChecked(self.settings.value("PluginDatabase/ShowInstruments", True, type=bool))
        self.ch_midi.setChecked(self.settings.value("PluginDatabase/ShowMIDI", True, type=bool))
        self.ch_other.setChecked(self.settings.value("PluginDatabase/ShowOther", True, type=bool))
        self.ch_ladspa.setChecked(self.settings.value("PluginDatabase/ShowLADSPA", True, type=bool))
        self.ch_dssi.setChecked(self.settings.value("PluginDatabase/ShowDSSI", True, type=bool))
        self.ch_lv2.setChecked(self.settings.value("PluginDatabase/ShowLV2", True, type=bool))
        self.ch_vst.setChecked(self.settings.value("PluginDatabase/ShowVST", True, type=bool))
        self.ch_sf2.setChecked(self.settings.value("PluginDatabase/ShowSF2", True, type=bool))
        self.ch_native.setChecked(self.settings.value("PluginDatabase/ShowNative", True, type=bool))
        self.ch_bridged.setChecked(self.settings.value("PluginDatabase/ShowBridged", True, type=bool))
        self.ch_bridged_wine.setChecked(self.settings.value("PluginDatabase/ShowBridgedWine", True, type=bool))
        self.ch_gui.setChecked(self.settings.value("PluginDatabase/ShowHasGUI", False, type=bool))
        self.ch_stereo.setChecked(self.settings.value("PluginDatabase/ShowStereoOnly", False, type=bool))
        self.reAddPlugins()

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# About Carla Dialog
class AboutW(QDialog, ui_carla_about.Ui_AboutW):
    def __init__(self, parent=None):
        super(AboutW, self).__init__(parent)
        self.setupUi(self)

        self.l_about.setText(self.tr(""
                                     "<br>Version %s"
                                     "<br>Carla is a Multi-Plugin Host for JACK.<br>"
                                     "<br>Copyright (C) 2011 falkTX<br>"
                                     "<br><i>VST is a trademark of Steinberg Media Technologies GmbH.</i>"
                                     "" % VERSION))

        host_osc_url = c_string(CarlaHost.get_host_osc_url())
        self.le_osc_url.setText(host_osc_url)

        self.l_osc_cmds.setText(""
                                " /set_active         <i-value>\n"
                                " /set_drywet         <f-value>\n"
                                " /set_vol            <f-value>\n"
                                " /set_balance_left   <f-value>\n"
                                " /set_balance_right  <f-value>\n"
                                " /set_parameter      <i-index> <f-value>\n"
                                " /set_program        <i-index>\n"
                                " /set_midi_program   <i-index>\n"
                                " /note_on            <i-note> <i-velo>\n"
                                " /note_off           <i-note> <i-velo>\n"
        )

        self.l_example.setText("/Carla/2/set_parameter_value 2 0.5")
        self.l_example_help.setText("<i>(as in this example, \"2\" is the plugin number)</i>")

        self.l_ladspa.setText(self.tr("Everything! (Including LRDF)"))
        self.l_dssi.setText(self.tr("Everything! (Including CustomData/Chunks)"))
        self.l_lv2.setText(self.tr("About 95&#37; complete (only missing minor features).<br/>"
                                   "Implemented Feature/Extensions:"
                                   "<ul>"
                                   "<li>http://lv2plug.in/ns/ext/atom</li>"
                                   "<li>http://lv2plug.in/ns/ext/data-access</li>"
                                   "<li>http://lv2plug.in/ns/ext/event</li>"
                                   "<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                   "<li>http://lv2plug.in/ns/ext/log</li>"
                                   "<li>http://lv2plug.in/ns/ext/midi</li>"
                                   #"<li>http://lv2plug.in/ns/ext/patch</li>"
                                   "<li>http://lv2plug.in/ns/ext/port-props</li>"
                                   #"<li>http://lv2plug.in/ns/ext/presets</li>"
                                   "<li>http://lv2plug.in/ns/ext/state</li>"
                                   #"<li>http://lv2plug.in/ns/ext/time</li>"
                                   "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                   "<li>http://lv2plug.in/ns/ext/urid</li>"
                                   "<li>http://lv2plug.in/ns/ext/worker</li>"
                                   "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                   "<li>http://lv2plug.in/ns/extensions/units</li>"
                                   #"<li>http://home.gna.org/lv2dynparam/v1</li>"
                                   "<li>http://home.gna.org/lv2dynparam/rtmempool/v1</li>"
                                   "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                   #"<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                   "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                   "<li>http://nedko.arnaudov.name/lv2/external_ui/</li>"
                                   "</ul>"))
        self.l_vst.setText(self.tr("<p>About 85&#37; complete (missing vst bank/presets and some minor stuff)</p>"))

# Single Plugin Parameter
class PluginParameter(QWidget, ui_carla_parameter.Ui_PluginParameter):
    def __init__(self, parent, pinfo, plugin_id):
        QWidget.__init__(self, parent)
        self.setupUi(self)

        self.ptype = pinfo['type']
        self.parameter_id = pinfo['index']
        self.hints = pinfo['hints']

        self.midi_cc = -1
        self.midi_channel = 1
        self.plugin_id = plugin_id

        self.add_MIDI_CCs_to_ComboBox()

        self.label.setText(pinfo['name'])

        if (self.ptype == PARAMETER_INPUT):
            self.widget.set_minimum(pinfo['minimum'])
            self.widget.set_maximum(pinfo['maximum'])
            self.widget.set_default(pinfo['default'])
            self.widget.set_value(pinfo['current'], False)
            self.widget.set_label(pinfo['unit'])
            self.widget.set_step(pinfo['step'])
            self.widget.set_step_small(pinfo['step_small'])
            self.widget.set_step_large(pinfo['step_large'])
            self.widget.set_scalepoints(pinfo['scalepoints'], (pinfo['hints'] & PARAMETER_USES_SCALEPOINTS))

            if (not self.hints & PARAMETER_IS_ENABLED):
                self.widget.set_read_only(True)
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

            elif (not self.hints & PARAMETER_IS_AUTOMABLE):
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        elif (self.ptype == PARAMETER_OUTPUT):
            self.widget.set_minimum(pinfo['minimum'])
            self.widget.set_maximum(pinfo['maximum'])
            self.widget.set_value(pinfo['current'], False)
            self.widget.set_label(pinfo['unit'])
            self.widget.set_read_only(True)

            if (not self.hints & PARAMETER_IS_AUTOMABLE):
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        else:
            self.widget.setVisible(False)
            self.combo.setVisible(False)
            self.sb_channel.setVisible(False)

        self.set_parameter_midi_channel(pinfo['midi_channel'])
        self.set_parameter_midi_cc(pinfo['midi_cc'])

        self.connect(self.widget, SIGNAL("valueChanged(double)"), SLOT("slot_valueChanged(double)"))
        self.connect(self.sb_channel, SIGNAL("valueChanged(int)"), SLOT("slot_midiChannelChanged(int)"))
        self.connect(self.combo, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiCcChanged(int)"))

        #if force_parameters_style:
        #self.widget.force_plastique_style()

        if (self.hints & PARAMETER_USES_CUSTOM_TEXT):
            self.widget.set_text_call(self.textCallFunction)

        self.widget.updateAll()

    def set_default_value(self, value):
        self.widget.set_default(value)

    def set_parameter_value(self, value, send=True):
        self.widget.set_value(value, send)

    def set_parameter_midi_channel(self, channel):
        self.midi_channel = channel
        self.sb_channel.setValue(channel - 1)

    def set_parameter_midi_cc(self, cc_index):
        self.midi_cc = cc_index
        self.set_MIDI_CC_in_ComboBox(cc_index)

    def add_MIDI_CCs_to_ComboBox(self):
        for MIDI_CC in MIDI_CC_LIST:
            self.combo.addItem(MIDI_CC)

    def set_MIDI_CC_in_ComboBox(self, midi_cc):
        for i in range(len(MIDI_CC_LIST)):
            midi_cc_text = MIDI_CC_LIST[i].split(" ")[0]
            if (int(midi_cc_text, 16) == midi_cc):
                cc_index = i
                break
        else:
            cc_index = -1

        cc_index += 1
        self.combo.setCurrentIndex(cc_index)

    def textCallFunction(self):
        return c_string(CarlaHost.get_parameter_text(self.plugin_id, self.parameter_id))

    @pyqtSlot(float)
    def slot_valueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, double)"), self.parameter_id, value)

    @pyqtSlot(int)
    def slot_midiChannelChanged(self, channel):
        if (self.midi_channel != channel):
            self.emit(SIGNAL("midiChannelChanged(int, int)"), self.parameter_id, channel)
        self.midi_channel = channel

    @pyqtSlot(int)
    def slot_midiCcChanged(self, cc_index):
        if (cc_index <= 0):
            midi_cc = -1
        else:
            midi_cc_text = MIDI_CC_LIST[cc_index - 1].split(" ")[0]
            midi_cc = int(midi_cc_text, 16)

        if (self.midi_cc != midi_cc):
            self.emit(SIGNAL("midiCcChanged(int, int)"), self.parameter_id, midi_cc)
        self.midi_cc = midi_cc

# Plugin GUI
class PluginGUI(QDialog):
    def __init__(self, parent, plugin_name, resizable):
        QDialog.__init__(self, parent)

        self.myLayout = QVBoxLayout(self)
        self.myLayout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.myLayout)

        self.resizable = resizable
        self.setNewSize(50, 50)

        self.setWindowTitle("%s (GUI)" % (plugin_name))

    def setNewSize(self, width, height):
        if (width < 30):
            width = 30
        if (height < 30):
            height = 30

        if (self.resizable):
            self.resize(width, height)
        else:
            self.setFixedSize(width, height)

    def hideEvent(self, event):
        # FIXME
        event.accept()
        self.close()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Plugin Editor (Built-in)
class PluginEdit(QDialog, ui_carla_edit.Ui_PluginEdit):
    def __init__(self, parent, plugin_id):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.pinfo = None
        self.ptype = PLUGIN_NONE
        self.plugin_id = plugin_id

        self.parameter_count = 0
        self.parameter_list = [] # type, id, widget
        self.parameter_list_to_update = [] # ids

        self.state_filename = None
        self.cur_program_index = -1
        self.cur_midi_program_index = -1

        self.tab_icon_off = QIcon(":/bitmaps/led_off.png")
        self.tab_icon_on = QIcon(":/bitmaps/led_yellow.png")
        self.tab_icon_count = 0
        self.tab_icon_timers = []

        self.connect(self.b_save_state, SIGNAL("clicked()"), SLOT("slot_saveState()"))
        self.connect(self.b_load_state, SIGNAL("clicked()"), SLOT("slot_loadState()"))

        self.connect(self.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))
        self.connect(self.keyboard, SIGNAL("notesOn()"), SLOT("slot_notesOn()"))
        self.connect(self.keyboard, SIGNAL("notesOff()"), SLOT("slot_notesOff()"))

        self.connect(self.cb_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_programIndexChanged(int)"))
        self.connect(self.cb_midi_programs, SIGNAL("currentIndexChanged(int)"),
            SLOT("slot_midiProgramIndexChanged(int)"))

        self.keyboard.setMode(self.keyboard.HORIZONTAL)
        self.keyboard.setOctaves(6)
        self.scrollArea.ensureVisible(self.keyboard.width() * 1 / 5, 0)
        self.scrollArea.setVisible(False)

        # TODO - not implemented yet
        self.b_reload_program.setEnabled(False)
        self.b_reload_midi_program.setEnabled(False)

        self.do_reload_all()

    def set_parameter_to_update(self, parameter_id):
        if (parameter_id not in self.parameter_list_to_update):
            self.parameter_list_to_update.append(parameter_id)

    def set_parameter_midi_channel(self, parameter_id, channel):
        for ptype, pid, pwidget in self.parameter_list:
            if (pid == parameter_id):
                pwidget.set_parameter_midi_channel(channel)
                break

    def set_parameter_midi_cc(self, parameter_id, midi_cc):
        for ptype, pid, pwidget in self.parameter_list:
            if (pid == parameter_id):
                pwidget.set_parameter_midi_cc(midi_cc)
                break

    def set_program(self, program_id):
        self.cur_program_index = program_id
        self.cb_programs.setCurrentIndex(program_id)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def set_midi_program(self, midi_program_id):
        self.cur_midi_program_index = midi_program_id
        self.cb_midi_programs.setCurrentIndex(midi_program_id)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def do_update(self):
        # Update current program text
        if (self.cb_programs.count() > 0):
            pindex = self.cb_programs.currentIndex()
            pname = c_string(CarlaHost.get_program_name(self.plugin_id, pindex))
            self.cb_programs.setItemText(pindex, pname)

        # Update current midi program text
        if (self.cb_midi_programs.count() > 0):
            mpindex = self.cb_midi_programs.currentIndex()
            mpname = "%s %s" % (self.cb_midi_programs.currentText().split(" ", 1)[0],
                                c_string(CarlaHost.get_midi_program_name(self.plugin_id, mpindex)))
            self.cb_midi_programs.setItemText(mpindex, mpname)

        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        QTimer.singleShot(0, self, SLOT("slot_checkOutputControlParameters()"))

    def do_reload_all(self):
        self.pinfo = CarlaHost.get_plugin_info(self.plugin_id)
        if (self.pinfo['valid']):
            self.pinfo["binary"] = c_string(self.pinfo["binary"])
            self.pinfo["name"] = c_string(self.pinfo["name"])
            self.pinfo["label"] = c_string(self.pinfo["label"])
            self.pinfo["maker"] = c_string(self.pinfo["maker"])
            self.pinfo["copyright"] = c_string(self.pinfo["copyright"])
        else:
            self.pinfo["type"] = PLUGIN_NONE
            self.pinfo["category"] = PLUGIN_CATEGORY_NONE
            self.pinfo["hints"] = 0x0
            self.pinfo["binary"] = ""
            self.pinfo["name"] = "(Unknown)"
            self.pinfo["label"] = ""
            self.pinfo["maker"] = ""
            self.pinfo["copyright"] = ""
            self.pinfo["unique_id"] = 0

        self.do_reload_info()
        self.do_reload_parameters()
        self.do_reload_programs()

    def do_reload_info(self):
        if (self.ptype == PLUGIN_NONE and self.pinfo['type'] in (PLUGIN_DSSI, PLUGIN_SF2)):
            self.tab_programs.setCurrentIndex(1)

        self.ptype = self.pinfo['type']

        real_plugin_name = c_string(CarlaHost.get_real_plugin_name(self.plugin_id))

        self.le_name.setText(real_plugin_name)
        self.le_name.setToolTip(real_plugin_name)
        self.le_label.setText(self.pinfo['label'])
        self.le_label.setToolTip(self.pinfo['label'])
        self.le_maker.setText(self.pinfo['maker'])
        self.le_maker.setToolTip(self.pinfo['maker'])
        self.le_copyright.setText(self.pinfo['copyright'])
        self.le_copyright.setToolTip(self.pinfo['copyright'])
        self.le_unique_id.setText(str(self.pinfo['unique_id']))
        self.le_unique_id.setToolTip(str(self.pinfo['unique_id']))

        self.label_plugin.setText("\n%s\n" % (self.pinfo['name']))
        self.setWindowTitle(self.pinfo['name'])

        if (self.ptype == PLUGIN_LADSPA):
            self.le_type.setText("LADSPA")
        elif (self.ptype == PLUGIN_DSSI):
            self.le_type.setText("DSSI")
        elif (self.ptype == PLUGIN_LV2):
            self.le_type.setText("LV2")
        elif (self.ptype == PLUGIN_VST):
            self.le_type.setText("VST")
        elif (self.ptype == PLUGIN_SF2):
            self.le_type.setText("SoundFont")
        else:
            self.le_type.setText(self.tr("Unknown"))

        audio_count = CarlaHost.get_audio_port_count_info(self.plugin_id)
        if (not audio_count['valid']):
            audio_count['ins'] = 0
            audio_count['outs'] = 0
            audio_count['total'] = 0

        midi_count = CarlaHost.get_midi_port_count_info(self.plugin_id)
        if (not midi_count['valid']):
            midi_count['ins'] = 0
            midi_count['outs'] = 0
            midi_count['total'] = 0

        param_count = CarlaHost.get_parameter_count_info(self.plugin_id)
        if (not param_count['valid']):
            param_count['ins'] = 0
            param_count['outs'] = 0
            param_count['total'] = 0

        self.le_ains.setText(str(audio_count['ins']))
        self.le_aouts.setText(str(audio_count['outs']))
        self.le_params.setText(str(param_count['ins']))
        self.le_couts.setText(str(param_count['outs']))
        self.le_is_synth.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_IS_SYNTH) else (self.tr("No")))
        self.le_has_gui.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_HAS_GUI) else (self.tr("No")))

        self.scrollArea.setVisible(
            self.pinfo['hints'] & PLUGIN_IS_SYNTH or (midi_count['ins'] > 0 < midi_count['outs']))
        self.parent().recheck_hints(self.pinfo['hints'])

    def do_reload_parameters(self):
        parameters_count = CarlaHost.get_parameter_count(self.plugin_id)

        self.parameter_list = []
        self.parameter_list_to_update = []

        self.tab_icon_count = 0
        self.tab_icon_timers = []

        for i in range(self.tabWidget.count()):
            if (i == 0): continue
            self.tabWidget.widget(1).deleteLater()
            self.tabWidget.removeTab(1)

        if (parameters_count <= 0):
            pass

        elif (parameters_count <= MAX_PARAMETERS):
            p_in = []
            p_in_tmp = []
            p_in_index = 0
            p_in_width = 0

            p_out = []
            p_out_tmp = []
            p_out_index = 0
            p_out_width = 0

            for i in range(parameters_count):
                param_info = CarlaHost.get_parameter_info(self.plugin_id, i)
                param_data = CarlaHost.get_parameter_data(self.plugin_id, i)
                param_ranges = CarlaHost.get_parameter_ranges(self.plugin_id, i)

                if not param_info['valid']:
                    continue

                parameter = {
                    'type': param_data['type'],
                    'hints': param_data['hints'],
                    'name': c_string(param_info['name']),
                    'unit': c_string(param_info['unit']),
                    'scalepoints': [],

                    'index': param_data['index'],
                    'default': param_ranges['def'],
                    'minimum': param_ranges['min'],
                    'maximum': param_ranges['max'],
                    'step': param_ranges['step'],
                    'step_small': param_ranges['step_small'],
                    'step_large': param_ranges['step_large'],
                    'midi_channel': param_data['midi_channel'],
                    'midi_cc': param_data['midi_cc'],

                    'current': CarlaHost.get_current_parameter_value(self.plugin_id, i)
                }

                for j in range(param_info['scalepoint_count']):
                    scalepoint = CarlaHost.get_scalepoint_info(self.plugin_id, i, j)
                    parameter['scalepoints'].append({
                          'value': scalepoint['value'],
                          'label': c_string(scalepoint['label'])
                        })

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if (parameter['type'] == PARAMETER_INPUT):
                    p_in_tmp.append(parameter)
                    p_in_width_tmp = QFontMetrics(self.font()).width(parameter['name'])

                    if (p_in_width_tmp > p_in_width):
                        p_in_width = p_in_width_tmp

                    if (len(p_in_tmp) == 10):
                        p_in.append((p_in_tmp, p_in_width))
                        p_in_tmp = []
                        p_in_index = 0
                        p_in_width = 0
                    else:
                        p_in_index += 1

                elif (parameter['type'] == PARAMETER_OUTPUT):
                    p_out_tmp.append(parameter)
                    p_out_width_tmp = QFontMetrics(self.font()).width(parameter['name'])

                    if (p_out_width_tmp > p_out_width):
                        p_out_width = p_out_width_tmp

                    if (len(p_out_tmp) == 10):
                        p_out.append((p_out_tmp, p_out_width))
                        p_out_tmp = []
                        p_out_index = 0
                        p_out_width = 0
                    else:
                        p_out_index += 1

            else:
                # Final page width values
                if (0 < len(p_in_tmp) < 10):
                    p_in.append((p_in_tmp, p_in_width))

                if (0 < len(p_out_tmp) < 10):
                    p_out.append((p_out_tmp, p_out_width))

            # -----------------------------------------------------------------
            # Create parameter widgets

            if (len(p_in) > 0):
                self.createParameterWidgets(p_in, self.tr("Parameters"), PARAMETER_INPUT)

            if (len(p_out) > 0):
                self.createParameterWidgets(p_out, self.tr("Outputs"), PARAMETER_OUTPUT)

        else: # > MAX_PARAMETERS
            fake_name = "This plugin has too many parameters to display here!"

            p_fake = []
            p_fake_tmp = []
            p_fake_width = QFontMetrics(self.font()).width(fake_name)

            parameter = {
                'type': PARAMETER_UNKNOWN,
                'hints': 0,
                'name': fake_name,
                'unit': "",
                'scalepoints': [],

                'index': 0,
                'default': 0,
                'minimum': 0,
                'maximum': 0,
                'step': 0,
                'step_small': 0,
                'step_large': 0,
                'midi_channel': 0,
                'midi_cc': -1,

                'current': 0.0
            }

            p_fake_tmp.append(parameter)
            p_fake.append((p_fake_tmp, p_fake_width))

            self.createParameterWidgets(p_fake, self.tr("Information"), PARAMETER_UNKNOWN)

    def do_reload_programs(self):
        # Programs
        self.cb_programs.blockSignals(True)
        self.cb_programs.clear()

        program_count = CarlaHost.get_program_count(self.plugin_id)

        if (program_count > 0):
            self.cb_programs.setEnabled(True)

            for i in range(program_count):
                pname = c_string(CarlaHost.get_program_name(self.plugin_id, i))
                self.cb_programs.addItem(pname)

            self.cur_program_index = CarlaHost.get_current_program_index(self.plugin_id)
            self.cb_programs.setCurrentIndex(self.cur_program_index)

        else:
            self.cb_programs.setEnabled(False)

        self.cb_programs.blockSignals(False)

        # MIDI Programs
        self.cb_midi_programs.blockSignals(True)
        self.cb_midi_programs.clear()

        midi_program_count = CarlaHost.get_midi_program_count(self.plugin_id)

        if (midi_program_count > 0):
            self.cb_midi_programs.setEnabled(True)

            for i in range(midi_program_count):
                midip = CarlaHost.get_midi_program_info(self.plugin_id, i)

                bank = int(midip['bank'])
                prog = int(midip['program'])
                label = c_string(midip['label'])

                self.cb_midi_programs.addItem("%03i:%03i - %s" % (bank, prog, label))

            self.cur_midi_program_index = CarlaHost.get_current_midi_program_index(self.plugin_id)
            self.cb_midi_programs.setCurrentIndex(self.cur_midi_program_index)

        else:
            self.cb_midi_programs.setEnabled(False)

        self.cb_midi_programs.blockSignals(False)

    def saveState_InternalFormat(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PRESET>\n"
                   "<CARLA-PRESET VERSION='%s'>\n") % (VERSION)

        content += self.parent().getSaveXMLContent()

        content += "</CARLA-PRESET>\n"

        try:
            open(self.state_filename, "w").write(content)
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save state file"))

    def saveState_Lv2Format(self):
        pass

    def saveState_VstFormat(self):
        pass

    def loadState_InternalFormat(self):
        try:
            state_read = open(self.state_filename, "r").read()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load state file"))
            return

        xml = QDomDocument()
        xml.setContent(state_read)

        xml_node = xml.documentElement()

        if (xml_node.tagName() != "CARLA-PRESET"):
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla state file"))
            return

        x_save_state_dict = getStateDictFromXML(xml_node)

        self.parent().loadStateDict(x_save_state_dict)

    def createParameterWidgets(self, p_list_full, tab_name, ptype):
        for i in range(len(p_list_full)):
            p_list = p_list_full[i][0]
            width = p_list_full[i][1]

            if (len(p_list) > 0):
                container = QWidget(self.tabWidget)
                layout = QVBoxLayout(container)
                container.setLayout(layout)

                for j in range(len(p_list)):
                    pwidget = PluginParameter(container, p_list[j], self.plugin_id)
                    pwidget.label.setMinimumWidth(width)
                    pwidget.label.setMaximumWidth(width)
                    pwidget.tab_index = self.tabWidget.count()
                    layout.addWidget(pwidget)

                    self.parameter_list.append((ptype, p_list[j]['index'], pwidget))

                    if (ptype == PARAMETER_INPUT):
                        self.connect(pwidget, SIGNAL("valueChanged(int, double)"),
                            SLOT("slot_parameterValueChanged(int, double)"))

                    self.connect(pwidget, SIGNAL("midiChannelChanged(int, int)"),
                        SLOT("slot_parameterMidiChannelChanged(int, int)"))
                    self.connect(pwidget, SIGNAL("midiCcChanged(int, int)"),
                        SLOT("slot_parameterMidiCcChanged(int, int)"))

                # FIXME
                layout.addStretch()

                self.tabWidget.addTab(container, "%s (%i)" % (tab_name, i + 1))

                if (ptype == PARAMETER_INPUT):
                    self.tabWidget.setTabIcon(pwidget.tab_index, self.tab_icon_off)

                self.tab_icon_timers.append(ICON_STATE_NULL)

    def animateTab(self, index):
        if (self.tab_icon_timers[index - 1] == ICON_STATE_NULL):
            self.tabWidget.setTabIcon(index, self.tab_icon_on)
        self.tab_icon_timers[index - 1] = ICON_STATE_ON

    def check_gui_stuff(self):
        # Check Tab icons
        for i in range(len(self.tab_icon_timers)):
            if (self.tab_icon_timers[i] == ICON_STATE_ON):
                self.tab_icon_timers[i] = ICON_STATE_WAIT
            elif (self.tab_icon_timers[i] == ICON_STATE_WAIT):
                self.tab_icon_timers[i] = ICON_STATE_OFF
            elif (self.tab_icon_timers[i] == ICON_STATE_OFF):
                self.tabWidget.setTabIcon(i + 1, self.tab_icon_off)
                self.tab_icon_timers[i] = ICON_STATE_NULL

        # Check parameters needing update
        for parameter_id in self.parameter_list_to_update:
            value = CarlaHost.get_current_parameter_value(self.plugin_id, parameter_id)

            for ptype, pid, pwidget in self.parameter_list:
                if (pid == parameter_id):
                    pwidget.set_parameter_value(value, False)

                    if (ptype == PARAMETER_INPUT):
                        self.animateTab(pwidget.tab_index)

                    break

        # Clear all parameters
        self.parameter_list_to_update = []

        # Update output parameters
        QTimer.singleShot(0, self, SLOT("slot_checkOutputControlParameters()"))

    @pyqtSlot()
    def slot_saveState(self):
        # TODO - LV2 and VST native formats
        if (self.state_filename):
            ask_try = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if (ask_try == QMessageBox.Ok):
                self.saveState_InternalFormat()
            else:
                self.state_filename = None
                self.slot_saveState()

        else:
            file_filter = self.tr("Carla State File (*.carxs)")
            filename_try = QFileDialog.getSaveFileName(self, self.tr("Save Carla State File"), filter=file_filter)

            if (filename_try):
                self.state_filename = filename_try
                self.saveState_InternalFormat()

    @pyqtSlot()
    def slot_loadState(self):
        # TODO - LV2 and VST native formats
        file_filter = self.tr("Carla State File (*.carxs)")
        filename_try = QFileDialog.getOpenFileName(self, self.tr("Open Carla State File"), filter=file_filter)

        if (filename_try):
            self.state_filename = filename_try
            self.loadState_InternalFormat()

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameter_id, value):
        CarlaHost.set_parameter_value(self.plugin_id, parameter_id, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameter_id, channel):
        CarlaHost.set_parameter_midi_channel(self.plugin_id, parameter_id, channel - 1)

    @pyqtSlot(int, int)
    def slot_parameterMidiCcChanged(self, parameter_id, cc_index):
        CarlaHost.set_parameter_midi_cc(self.plugin_id, parameter_id, cc_index)

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        if (self.cur_program_index != index):
            CarlaHost.set_program(self.plugin_id, index)
            QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        self.cur_program_index = index

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        if (self.cur_midi_program_index != index):
            CarlaHost.set_midi_program(self.plugin_id, index)
            QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        self.cur_midi_program_index = index

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        CarlaHost.send_midi_note(self.plugin_id, True, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        CarlaHost.send_midi_note(self.plugin_id, False, note, 0)

    @pyqtSlot()
    def slot_notesOn(self):
        self.parent().led_midi.setChecked(True)

    @pyqtSlot()
    def slot_notesOff(self):
        self.parent().led_midi.setChecked(False)

    @pyqtSlot()
    def slot_checkInputControlParameters(self):
        for ptype, pid, pwidget in self.parameter_list:
            if (ptype == PARAMETER_INPUT):
                if (self.pinfo["type"] != PLUGIN_SF2):
                    pwidget.set_default_value(CarlaHost.get_default_parameter_value(self.plugin_id, pid))
                pwidget.set_parameter_value(CarlaHost.get_current_parameter_value(self.plugin_id, pid), False)

    @pyqtSlot()
    def slot_checkOutputControlParameters(self):
        for ptype, pid, pwidget in self.parameter_list:
            if (ptype == PARAMETER_OUTPUT):
                pwidget.set_parameter_value(CarlaHost.get_current_parameter_value(self.plugin_id, pid), False)

# (New) Plugin Widget
class PluginWidget(QFrame, ui_carla_plugin.Ui_PluginWidget):
    def __init__(self, parent, plugin_id):
        QFrame.__init__(self, parent)
        self.setupUi(self)

        self.plugin_id = plugin_id

        self.params_total = 0
        self.parameter_activity_timer = None

        self.last_led_ain_state = False
        self.last_led_aout_state = False

        # Fake effect
        self.color_1 = QColor(0, 0, 0, 220)
        self.color_2 = QColor(0, 0, 0, 170)
        self.color_3 = QColor(7, 7, 7, 250)
        self.color_4 = QColor(14, 14, 14, 255)

        self.led_enable.setColor(self.led_enable.BIG_RED)
        self.led_enable.setChecked(False)

        self.led_control.setColor(self.led_control.YELLOW)
        self.led_control.setEnabled(False)

        self.led_midi.setColor(self.led_midi.RED)
        self.led_midi.setEnabled(False)

        self.led_audio_in.setColor(self.led_audio_in.GREEN)
        self.led_audio_in.setEnabled(False)

        self.led_audio_out.setColor(self.led_audio_out.BLUE)
        self.led_audio_out.setEnabled(False)

        self.dial_drywet.setPixmap(1)
        self.dial_vol.setPixmap(2)
        self.dial_b_left.setPixmap(1)
        self.dial_b_right.setPixmap(1)

        self.dial_drywet.setLabel("Wet")
        self.dial_vol.setLabel("Vol")
        self.dial_b_left.setLabel("L")
        self.dial_b_right.setLabel("R")

        self.peak_in.setColor(self.peak_in.GREEN)
        self.peak_in.setOrientation(self.peak_in.HORIZONTAL)

        self.peak_out.setColor(self.peak_in.BLUE)
        self.peak_out.setOrientation(self.peak_out.HORIZONTAL)

        audio_count = CarlaHost.get_audio_port_count_info(self.plugin_id)
        if (not audio_count['valid']):
            audio_count['ins'] = 0
            audio_count['outs'] = 0
            audio_count['total'] = 0

        self.peaks_in = int(audio_count['ins'])
        self.peaks_out = int(audio_count['outs'])

        if (self.peaks_in > 2):
            self.peaks_in = 2

        if (self.peaks_out > 2):
            self.peaks_out = 2

        self.peak_in.setChannels(self.peaks_in)
        self.peak_out.setChannels(self.peaks_out)

        self.pinfo = CarlaHost.get_plugin_info(self.plugin_id)
        if (self.pinfo['valid']):
            self.pinfo["binary"] = c_string(self.pinfo["binary"])
            self.pinfo["name"] = c_string(self.pinfo["name"])
            self.pinfo["label"] = c_string(self.pinfo["label"])
            self.pinfo["maker"] = c_string(self.pinfo["maker"])
            self.pinfo["copyright"] = c_string(self.pinfo["copyright"])
        else:
            self.pinfo["type"] = PLUGIN_NONE
            self.pinfo["category"] = PLUGIN_CATEGORY_NONE
            self.pinfo["hints"] = 0
            self.pinfo["binary"] = ""
            self.pinfo["name"] = "(Unknown)"
            self.pinfo["label"] = ""
            self.pinfo["maker"] = ""
            self.pinfo["copyright"] = ""
            self.pinfo["unique_id"] = 0

        # Set widget page
        if (self.pinfo['type'] == PLUGIN_NONE or audio_count['total'] == 0):
            self.stackedWidget.setCurrentIndex(1)

        self.label_name.setText(self.pinfo['name'])

        # Enable/disable features
        self.recheck_hints(self.pinfo['hints'])

        # Colorify
        if (self.pinfo['category'] == PLUGIN_CATEGORY_SYNTH):
            self.setWidgetColor(PALETTE_COLOR_WHITE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DELAY):
            self.setWidgetColor(PALETTE_COLOR_ORANGE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_EQ):
            self.setWidgetColor(PALETTE_COLOR_GREEN)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_FILTER):
            self.setWidgetColor(PALETTE_COLOR_BLUE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DYNAMICS):
            self.setWidgetColor(PALETTE_COLOR_PINK)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_MODULATOR):
            self.setWidgetColor(PALETTE_COLOR_RED)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_UTILITY):
            self.setWidgetColor(PALETTE_COLOR_YELLOW)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_OTHER):
            self.setWidgetColor(PALETTE_COLOR_BROWN)
        else:
            self.setWidgetColor(PALETTE_COLOR_NONE)

        if (self.pinfo['hints'] & PLUGIN_IS_SYNTH):
            self.led_audio_in.setVisible(False)
        else:
            self.led_midi.setVisible(False)

        self.edit_dialog = PluginEdit(self, self.plugin_id)
        self.edit_dialog.hide()
        self.edit_dialog_geometry = None

        if (self.pinfo['hints'] & PLUGIN_HAS_GUI):
            gui_info = CarlaHost.get_gui_info(self.plugin_id)
            self.gui_dialog_type = gui_info['type']

            if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_X11)):
                self.gui_dialog = None
                self.gui_dialog = PluginGUI(self, self.pinfo['name'], gui_info['resizable'])
                self.gui_dialog.hide()
                self.gui_dialog_geometry = None
                self.connect(self.gui_dialog, SIGNAL("finished(int)"), SLOT("slot_guiClosed()"))

                # TODO - display
                CarlaHost.set_gui_data(self.plugin_id, 0, unwrapinstance(self.gui_dialog))

            elif (self.gui_dialog_type in (GUI_EXTERNAL_OSC, GUI_EXTERNAL_LV2)):
                self.gui_dialog = None

            else:
                self.gui_dialog = None
                self.gui_dialog_type = GUI_NONE
                self.b_gui.setEnabled(False)

        else:
            self.gui_dialog = None
            self.gui_dialog_type = GUI_NONE

        self.connect(self.led_enable, SIGNAL("clicked(bool)"), SLOT("slot_setActive(bool)"))
        self.connect(self.dial_drywet, SIGNAL("sliderMoved(int)"), SLOT("slot_setDryWet(int)"))
        self.connect(self.dial_vol, SIGNAL("sliderMoved(int)"), SLOT("slot_setVolume(int)"))
        self.connect(self.dial_b_left, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceLeft(int)"))
        self.connect(self.dial_b_right, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceRight(int)"))
        self.connect(self.b_gui, SIGNAL("clicked(bool)"), SLOT("slot_guiClicked(bool)"))
        self.connect(self.b_edit, SIGNAL("clicked(bool)"), SLOT("slot_editClicked(bool)"))
        self.connect(self.b_remove, SIGNAL("clicked()"), SLOT("slot_removeClicked()"))

        self.connect(self.dial_drywet, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_vol, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_b_left, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_b_right, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))

        self.connect(self.edit_dialog, SIGNAL("finished(int)"), SLOT("slot_editClosed()"))

        self.check_gui_stuff()

    def set_active(self, active, gui_send=False, callback_send=True):
        if (gui_send): self.led_enable.setChecked(active)
        if (callback_send): CarlaHost.set_active(self.plugin_id, active)

    def set_drywet(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_drywet.setValue(value)
        if (callback_send): CarlaHost.set_drywet(self.plugin_id, float(value) / 1000)

        message = self.tr("Output dry/wet (%s%%)" % (value / 10))
        self.dial_drywet.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_volume(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_vol.setValue(value)
        if (callback_send): CarlaHost.set_volume(self.plugin_id, float(value) / 1000)

        message = self.tr("Output volume (%s%%)" % (value / 10))
        self.dial_vol.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_balance_left(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_b_left.setValue(value)
        if (callback_send): CarlaHost.set_balance_left(self.plugin_id, float(value) / 1000)

        if (value == 0):
            message = self.tr("Left Panning (Center)")
        elif (value < 0):
            message = self.tr("Left Panning (%s%% Left)" % (-value / 10))
        else:
            message = self.tr("Left Panning (%s%% Right)" % (value / 10))

        self.dial_b_left.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_balance_right(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_b_right.setValue(value)
        if (callback_send): CarlaHost.set_balance_right(self.plugin_id, float(value) / 1000)

        if (value == 0):
            message = self.tr("Right Panning (Center)")
        elif (value < 0):
            message = self.tr("Right Panning (%s%% Left" % (-value / 10))
        else:
            message = self.tr("Right Panning (%s%% Right)" % (value / 10))

        self.dial_b_right.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def setWidgetColor(self, color):
        if (color == PALETTE_COLOR_WHITE):
            r = 110
            g = 110
            b = 110
            texture = 7
        elif (color == PALETTE_COLOR_RED):
            r = 110
            g = 15
            b = 15
            texture = 3
        elif (color == PALETTE_COLOR_GREEN):
            r = 15
            g = 110
            b = 15
            texture = 6
        elif (color == PALETTE_COLOR_BLUE):
            r = 15
            g = 15
            b = 110
            texture = 4
        elif (color == PALETTE_COLOR_YELLOW):
            r = 110
            g = 110
            b = 15
            texture = 2
        elif (color == PALETTE_COLOR_ORANGE):
            r = 180
            g = 110
            b = 15
            texture = 5
        elif (color == PALETTE_COLOR_BROWN):
            r = 110
            g = 35
            b = 15
            texture = 1
        elif (color == PALETTE_COLOR_PINK):
            r = 110
            g = 15
            b = 110
            texture = 8
        else:
            r = 35
            g = 35
            b = 35
            texture = 9

        self.setStyleSheet("""
        QFrame#PluginWidget {
                  background-image: url(:/bitmaps/textures/metal_%i-512px.jpg);
                  background-repeat: repeat-x;
                  background-position: top left;
                }
        QLabel#label_name {
          color: white;
        }
        QFrame#frame_name {
          background-image: url(:/bitmaps/glass.png);
          background-color: rgb(%i, %i, %i);
          border: 2px outset;
          border-color: rgb(%i, %i, %i);
        }
        QFrame#frame_controls {
          background-image: url(:/bitmaps/glass2.png);
          background-color: rgb(30, 30, 30);
          border: 2px outset;
          border-color: rgb(30, 30, 30);
        }
        QFrame#frame_peaks {
          background-color: rgba(30, 30, 30, 200);
          border: 2px outset;
          border-color: rgba(30, 30, 30, 225);
        }
      """ % (texture, r, g, b, r, g, b))

    def recheck_hints(self, hints):
        self.pinfo['hints'] = hints
        self.dial_drywet.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_DRYWET)
        self.dial_vol.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_VOLUME)
        self.dial_b_left.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.dial_b_right.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.b_gui.setEnabled(self.pinfo['hints'] & PLUGIN_HAS_GUI)

    def getSaveXMLContent(self):
        CarlaHost.prepare_for_save(self.plugin_id)

        if (self.pinfo['type'] == PLUGIN_LADSPA):
            type_str = "LADSPA"
        elif (self.pinfo['type'] == PLUGIN_DSSI):
            type_str = "DSSI"
        elif (self.pinfo['type'] == PLUGIN_LV2):
            type_str = "LV2"
        elif (self.pinfo['type'] == PLUGIN_VST):
            type_str = "VST"
        elif (self.pinfo['type'] == PLUGIN_SF2):
            type_str = "SoundFont"
        else:
            type_str = "Unknown"

        x_save_state_dict = deepcopy(save_state_dict)

        # ----------------------------
        # Basic info

        x_save_state_dict['Type'] = type_str
        x_save_state_dict['Name'] = self.pinfo['name']
        x_save_state_dict['Label'] = self.pinfo['label']
        x_save_state_dict['Binary'] = self.pinfo['binary']
        x_save_state_dict['UniqueID'] = self.pinfo['unique_id']

        # ----------------------------
        # Internals

        x_save_state_dict['Active'] = self.led_enable.isChecked()
        x_save_state_dict['DryWet'] = float(self.dial_drywet.value()) / 1000
        x_save_state_dict['Volume'] = float(self.dial_vol.value()) / 1000
        x_save_state_dict['Balance-Left'] = float(self.dial_b_left.value()) / 1000
        x_save_state_dict['Balance-Right'] = float(self.dial_b_right.value()) / 1000

        # ----------------------------
        # Current Program

        if (self.edit_dialog.cb_programs.currentIndex() >= 0):
            x_save_state_dict['CurrentProgramIndex'] = self.edit_dialog.cb_programs.currentIndex()
            x_save_state_dict['CurrentProgramName'] = self.edit_dialog.cb_programs.currentText()

        # ----------------------------
        # Current MIDI Program

        if (self.edit_dialog.cb_midi_programs.currentIndex() >= 0):
            midi_program_info = CarlaHost.get_midi_program_info(self.plugin_id, self.edit_dialog.cb_midi_programs.currentIndex())
            x_save_state_dict['CurrentMidiBank'] = midi_program_info['bank']
            x_save_state_dict['CurrentMidiProgram'] = midi_program_info['program']

        # ----------------------------
        # Parameters

        parameter_count = CarlaHost.get_parameter_count(self.plugin_id)

        for i in range(parameter_count):
            parameter_info = CarlaHost.get_parameter_info(self.plugin_id, i)
            parameter_data = CarlaHost.get_parameter_data(self.plugin_id, i)

            if (not parameter_info['valid'] or parameter_data['type'] != PARAMETER_INPUT):
                continue

            x_save_state_parameter = deepcopy(save_state_parameter)

            x_save_state_parameter['index'] = parameter_data['index']
            x_save_state_parameter['name'] = c_string(parameter_info['name'])
            x_save_state_parameter['symbol'] = c_string(parameter_info['symbol'])
            x_save_state_parameter['value'] = CarlaHost.get_current_parameter_value(self.plugin_id, parameter_data['index'])
            x_save_state_parameter['midi_channel'] = parameter_data['midi_channel'] + 1
            x_save_state_parameter['midi_cc'] = parameter_data['midi_cc']

            if (parameter_data['hints'] & PARAMETER_USES_SAMPLERATE):
                x_save_state_parameter['value'] /= CarlaHost.get_sample_rate()

            x_save_state_dict['Parameters'].append(x_save_state_parameter)

        # ----------------------------
        # Custom Data

        custom_data_count = CarlaHost.get_custom_data_count(self.plugin_id)

        for i in range(custom_data_count):
            custom_data = CarlaHost.get_custom_data(self.plugin_id, i)

            if (custom_data['type'] == CUSTOM_DATA_INVALID):
                continue

            x_save_state_custom_data = deepcopy(save_state_custom_data)

            x_save_state_custom_data['type']  = CustomDataType2String(custom_data['type'])
            x_save_state_custom_data['key']   = c_string(custom_data['key']).replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("\\","&apos;").replace("\"","&quot;")
            x_save_state_custom_data['value'] = c_string(custom_data['value']).replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("\\","&apos;").replace("\"","&quot;")

            x_save_state_dict['CustomData'].append(x_save_state_custom_data)

        # ----------------------------
        # Chunk

        if (self.pinfo['hints'] & PLUGIN_USES_CHUNKS):
            x_save_state_dict['Chunk'] = c_string(CarlaHost.get_chunk_data(self.plugin_id))

        # ----------------------------
        # Generate XML for this plugin

        # TODO - convert to xml safe strings where needed

        content = ""

        content += "  <Info>\n"
        content += "   <Type>%s</Type>\n" % (x_save_state_dict['Type'])
        content += "   <Name>%s</Name>\n" % (x_save_state_dict['Name'])
        if (self.pinfo['type'] == PLUGIN_LV2):
            content += "   <URI>%s</URI>\n" % (x_save_state_dict['Label'])
        else:
            content += "   <Label>%s</Label>\n" % (x_save_state_dict['Label'])
            content += "   <Binary>%s</Binary>\n" % (x_save_state_dict['Binary'])
            if (x_save_state_dict['UniqueID'] != 0):
                content += "   <UniqueID>%li</UniqueID>\n" % x_save_state_dict['UniqueID']
        content += "  </Info>\n"

        content += "\n"
        content += "  <Data>\n"
        content += "   <Active>%s</Active>\n" % ("Yes" if x_save_state_dict['Active'] else "No")
        content += "   <DryWet>%f</DryWet>\n" % (x_save_state_dict['DryWet'])
        content += "   <Volume>%f</Volume>\n" % (x_save_state_dict['Volume'])
        content += "   <Balance-Left>%f</Balance-Left>\n" % (x_save_state_dict['Balance-Left'])
        content += "   <Balance-Right>%f</Balance-Right>\n" % (x_save_state_dict['Balance-Right'])

        for parameter in x_save_state_dict['Parameters']:
            content += "\n"
            content += "   <Parameter>\n"
            content += "    <index>%i</index>\n" % (parameter['index'])
            content += "    <name>%s</name>\n" % (parameter['name'])
            if (parameter['symbol']):
                content += "    <symbol>%s</symbol>\n" % (parameter['symbol'])
            content += "    <value>%f</value>\n" % (parameter['value'])
            if (parameter['midi_cc'] > 0):
                content += "    <midi_channel>%i</midi_channel>\n" % (parameter['midi_channel'])
                content += "    <midi_cc>%i</midi_cc>\n" % (parameter['midi_cc'])
            content += "   </Parameter>\n"

        if (x_save_state_dict['CurrentProgramIndex'] >= 0):
            content += "\n"
            content += "   <CurrentProgramIndex>%i</CurrentProgramIndex>\n" % (x_save_state_dict['CurrentProgramIndex'])
            content += "   <CurrentProgramName>%s</CurrentProgramName>\n" % (x_save_state_dict['CurrentProgramName'])

        if (x_save_state_dict['CurrentMidiBank'] >= 0 and x_save_state_dict['CurrentMidiProgram'] >= 0):
            content += "\n"
            content += "   <CurrentMidiBank>%i</CurrentMidiBank>\n" % (x_save_state_dict['CurrentMidiBank'])
            content += "   <CurrentMidiProgram>%i</CurrentMidiProgram>\n" % (x_save_state_dict['CurrentMidiProgram'])

        for custom_data in x_save_state_dict['CustomData']:
            content += "\n"
            content += "   <CustomData>\n"
            content += "    <type>%s</type>\n" % (custom_data['type'])
            content += "    <key>%s</key>\n" % (custom_data['key'])
            if (custom_data['type'] in ("string", "binary")):
                content += "    <value>\n"
                content += "%s\n" % (custom_data['value'])
                content += "    </value>\n"
            else:
                content += "    <value>%s</value>\n" % (custom_data['value'])
            content += "   </CustomData>\n"

        if (x_save_state_dict['Chunk']):
            content += "\n"
            content += "   <Chunk>\n"
            content += "%s\n" % (x_save_state_dict['Chunk'])
            content += "   </Chunk>\n"

        content += "  </Data>\n"

        return content

    def loadStateDict(self, content):
        # ---------------------------------------------------------------------
        # Part 1 - set custom data
        for custom_data in content['CustomData']:
            CarlaHost.set_custom_data(self.plugin_id, custom_data['type'], custom_data['key'], custom_data['value'])

        # ---------------------------------------------------------------------
        # Part 2 - set program
        program_id = -1
        program_count = CarlaHost.get_program_count(self.plugin_id)

        if (content['CurrentProgramName']):
            test_pname = c_string(CarlaHost.get_program_name(self.plugin_id, content['CurrentProgramIndex']))

            # Program index and name matches
            if (content['CurrentProgramName'] == test_pname):
                program_id = content['CurrentProgramIndex']

            # index < count
            elif (content['CurrentProgramIndex'] < program_count):
                program_id = content['CurrentProgramIndex']

            # index not valid, try to find by name
            else:
                for i in range(program_count):
                    test_pname = c_string(CarlaHost.get_program_name(self.plugin_id, i))

                    if (content['CurrentProgramName'] == test_pname):
                        program_id = i
                        break

            # set program now, if valid
            if (program_id >= 0):
                CarlaHost.set_program(self.plugin_id, program_id)
                self.edit_dialog.set_program(program_id)

        # ---------------------------------------------------------------------
        # Part 3 - set midi program
        if (content['CurrentMidiBank'] >= 0 and content['CurrentMidiProgram'] >= 0):
            midi_program_count = CarlaHost.get_midi_program_count(self.plugin_id)

            for i in range(midi_program_count):
                program_info = CarlaHost.get_midi_program_info(self.plugin_id, i)
                if (program_info['bank'] == content['CurrentMidiBank'] and program_info['program'] == content[
                                                                                                      'CurrentMidiProgram']):
                    CarlaHost.set_midi_program(self.plugin_id, i)
                    self.edit_dialog.set_midi_program(i)
                    break

        # ---------------------------------------------------------------------
        # Part 4a - get plugin parameter symbols
        param_symbols = [] # (index, symbol)

        for parameter in content['Parameters']:
            if (parameter['symbol']):
                param_info = CarlaHost.get_parameter_info(self.plugin_id, parameter['index'])

                if (param_info['valid'] and param_info['symbol']):
                    param_symbols.append((parameter['index'], c_string(param_info['symbol'])))

        # ---------------------------------------------------------------------
        # Part 4b - set parameter values (carefully)
        for parameter in content['Parameters']:
            index = -1

            if (content['Type'] == "LADSPA"):
                # Try to set by symbol, otherwise use index
                if (parameter['symbol']):
                    for param_symbol in param_symbols:
                        if (param_symbol[1] == parameter['symbol']):
                            index = param_symbol[0]
                            break
                    else:
                        index = parameter['index']
                else:
                    index = parameter['index']

            elif (content['Type'] == "LV2"):
                # Symbol only
                if (parameter['symbol']):
                    for param_symbol in param_symbols:
                        if (param_symbol[1] == parameter['symbol']):
                            index = param_symbol[0]
                            break
                    else:
                        print("Failed to find LV2 parameter symbol for %i -> %s" % (
                            parameter['index'], parameter['symbol']))
                else:
                    print("LV2 Plugin parameter #%i, '%s', has no symbol" % (parameter['index'], parameter['name']))

            else:
                # Index only
                index = parameter['index']

            # Now set parameter
            if (index >= 0):
                param_data = CarlaHost.get_parameter_data(self.plugin_id, parameter['index'])
                if (param_data['hints'] & PARAMETER_USES_SAMPLERATE):
                    parameter['value'] *= CarlaHost.get_sample_rate()

                CarlaHost.set_parameter_value(self.plugin_id, index, parameter['value'])
                CarlaHost.set_parameter_midi_channel(self.plugin_id, index, parameter['midi_channel'] - 1)
                CarlaHost.set_parameter_midi_cc(self.plugin_id, index, parameter['midi_cc'])
            else:
                print("Could not set parameter data for %i -> %s" % (parameter['index'], parameter['name']))

        # ---------------------------------------------------------------------
        # Part 5 - set chunk data
        if (content['Chunk']):
            CarlaHost.set_chunk_data(self.plugin_id, content['Chunk'])

        # ---------------------------------------------------------------------
        # Part 6 - set internal stuff
        self.set_drywet(content['DryWet'] * 1000, True, True)
        self.set_volume(content['Volume'] * 1000, True, True)
        self.set_balance_left(content['Balance-Left'] * 1000, True, True)
        self.set_balance_right(content['Balance-Right'] * 1000, True, True)
        self.edit_dialog.do_reload_all()

        self.set_active(content['Active'], True, True)

    def check_gui_stuff(self):
        # Input peaks
        if (self.peaks_in > 0):
            if (self.peaks_in > 1):
                peak1 = CarlaHost.get_input_peak_value(self.plugin_id, 1)
                peak2 = CarlaHost.get_input_peak_value(self.plugin_id, 2)
                led_ain_state = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_in.displayMeter(1, peak1)
                self.peak_in.displayMeter(2, peak2)

            else:
                peak = CarlaHost.get_input_peak_value(self.plugin_id, 1)
                led_ain_state = bool(peak != 0.0)

                self.peak_in.displayMeter(1, peak)

            if (led_ain_state != self.last_led_ain_state):
                self.led_audio_in.setChecked(led_ain_state)

            self.last_led_ain_state = led_ain_state

        # Output peaks
        if (self.peaks_out > 0):
            if (self.peaks_out > 1):
                peak1 = CarlaHost.get_output_peak_value(self.plugin_id, 1)
                peak2 = CarlaHost.get_output_peak_value(self.plugin_id, 2)
                led_aout_state = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_out.displayMeter(1, peak1)
                self.peak_out.displayMeter(2, peak2)

            else:
                peak = CarlaHost.get_output_peak_value(self.plugin_id, 1)
                led_aout_state = bool(peak != 0.0)

                self.peak_out.displayMeter(1, peak)

            if (led_aout_state != self.last_led_aout_state):
                self.led_audio_out.setChecked(led_aout_state)

            self.last_led_aout_state = led_aout_state

    def check_gui_stuff2(self):
        # Parameter Activity LED
        if (self.parameter_activity_timer == ICON_STATE_ON):
            self.led_control.setChecked(True)
            self.parameter_activity_timer = ICON_STATE_WAIT
        elif (self.parameter_activity_timer == ICON_STATE_WAIT):
            self.parameter_activity_timer = ICON_STATE_OFF
        elif (self.parameter_activity_timer == ICON_STATE_OFF):
            self.led_control.setChecked(False)
            self.parameter_activity_timer = None

        # Update edit dialog
        self.edit_dialog.check_gui_stuff()

    @pyqtSlot(bool)
    def slot_setActive(self, yesno):
        self.set_active(yesno, False, True)

    @pyqtSlot(int)
    def slot_setDryWet(self, value):
        self.set_drywet(value, False, True)

    @pyqtSlot(int)
    def slot_setVolume(self, value):
        self.set_volume(value, False, True)

    @pyqtSlot(int)
    def slot_setBalanceLeft(self, value):
        self.set_balance_left(value, False, True)

    @pyqtSlot(int)
    def slot_setBalanceRight(self, value):
        self.set_balance_right(value, False, True)

    @pyqtSlot(bool)
    def slot_guiClicked(self, show):
        if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_X11)):
            if (show):
                if (self.gui_dialog_geometry):
                    self.gui_dialog.restoreGeometry(self.gui_dialog_geometry)
            else:
                self.gui_dialog_geometry = self.gui_dialog.saveGeometry()
            self.gui_dialog.setVisible(show)
        CarlaHost.show_gui(self.plugin_id, show)

    @pyqtSlot()
    def slot_guiClosed(self):
        self.b_gui.setChecked(False)

    @pyqtSlot(bool)
    def slot_editClicked(self, show):
        if (show):
            if (self.edit_dialog_geometry):
                self.edit_dialog.restoreGeometry(self.edit_dialog_geometry)
        else:
            self.edit_dialog_geometry = self.edit_dialog.saveGeometry()
        self.edit_dialog.setVisible(show)

    @pyqtSlot()
    def slot_editClosed(self):
        self.b_edit.setChecked(False)

    @pyqtSlot()
    def slot_removeClicked(self):
        gui.remove_plugin(self.plugin_id, True)

    @pyqtSlot()
    def slot_showCustomDialMenu(self):
        dial_name = self.sender().objectName()
        if (dial_name == "dial_drywet"):
            minimum = 0
            maximum = 100
            default = 100
            label = "Dry/Wet"
        elif (dial_name == "dial_vol"):
            minimum = 0
            maximum = 127
            default = 100
            label = "Volume"
        elif (dial_name == "dial_b_left"):
            minimum = -100
            maximum = 100
            default = -100
            label = "Balance-Left"
        elif (dial_name == "dial_b_right"):
            minimum = -100
            maximum = 100
            default = 100
            label = "Balance-Right"
        else:
            minimum = 0
            maximum = 100
            default = 100
            label = "Unknown"

        current = self.sender().value() / 10

        menu = QMenu(self)
        act_x_reset = menu.addAction(self.tr("Reset (%i%%)" % (default)))
        menu.addSeparator()
        act_x_min = menu.addAction(self.tr("Set to Minimum (%i%%)" % (minimum)))
        act_x_cen = menu.addAction(self.tr("Set to Center"))
        act_x_max = menu.addAction(self.tr("Set to Maximum (%i%%)" % (maximum)))
        menu.addSeparator()
        act_x_set = menu.addAction(self.tr("Set value..."))

        if (label not in ("Balance-Left", "Balance-Right")):
            menu.removeAction(act_x_cen)

        act_x_sel = menu.exec_(QCursor.pos())

        if (act_x_sel == act_x_set):
            value_try = QInputDialog.getInteger(self, self.tr("Set value"), label, current, minimum, maximum, 1)
            if (value_try[1]):
                value = value_try[0] * 10
            else:
                value = None

        elif (act_x_sel == act_x_min):
            value = minimum * 10
        elif (act_x_sel == act_x_max):
            value = maximum * 10
        elif (act_x_sel == act_x_reset):
            value = default * 10
        elif (act_x_sel == act_x_cen):
            value = 0
        else:
            value = None

        if (value != None):
            if (label == "Dry/Wet"):
                self.set_drywet(value, True, True)
            elif (label == "Volume"):
                self.set_volume(value, True, True)
            elif (label == "Balance-Left"):
                self.set_balance_left(value, True, True)
            elif (label == "Balance-Right"):
                self.set_balance_right(value, True, True)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setPen(self.color_1)
        painter.drawLine(0, 3, self.width(), 3)
        painter.drawLine(0, self.height() - 4, self.width(), self.height() - 4)
        painter.setPen(self.color_2)
        painter.drawLine(0, 2, self.width(), 2)
        painter.drawLine(0, self.height() - 3, self.width(), self.height() - 3)
        painter.setPen(self.color_3)
        painter.drawLine(0, 1, self.width(), 1)
        painter.drawLine(0, self.height() - 2, self.width(), self.height() - 2)
        painter.setPen(self.color_4)
        painter.drawLine(0, 0, self.width(), 0)
        painter.drawLine(0, self.height() - 1, self.width(), self.height() - 1)
        QFrame.paintEvent(self, event)

# Main Window
class CarlaMainW(QMainWindow, ui_carla.Ui_CarlaMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Load Settings

        self.settings = QSettings("Cadence", "Carla")
        self.settings_db = QSettings("Cadence", "Carla-Database")
        self.loadSettings(True)

        self.loadRDFs()

        self.setStyleSheet("""
          QWidget#centralwidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                stop:0 rgb( 7,  7,  7),
                stop:1 rgb(28, 28, 28)
            );
          }
        """)

        # -------------------------------------------------------------
        # Internal stuff

        self.m_bridge_info = None
        self.m_project_filename = None

        self.m_plugin_list = []
        for x in range(MAX_PLUGINS):
            self.m_plugin_list.append(None)

        CarlaHost.set_callback_function(self.callback_function)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.act_plugin_remove_all.setEnabled(False)
        self.resize(self.width(), 0)

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.act_file_new, SIGNAL("triggered()"), SLOT("slot_file_new()"))
        self.connect(self.act_file_open, SIGNAL("triggered()"), SLOT("slot_file_open()"))
        self.connect(self.act_file_save, SIGNAL("triggered()"), SLOT("slot_file_save()"))
        self.connect(self.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_file_save_as()"))

        self.connect(self.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_plugin_add()"))
        self.connect(self.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_remove_all()"))

        self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))
        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        self.connect(self, SIGNAL("DebugCallback(int, int, int, double)"),
            SLOT("slot_handleDebugCallback(int, int, int, double)"))
        self.connect(self, SIGNAL("ParameterCallback(int, int, double)"),
            SLOT("slot_handleParameterCallback(int, int, double)"))
        self.connect(self, SIGNAL("ProgramCallback(int, int)"), SLOT("slot_handleProgramCallback(int, int)"))
        self.connect(self, SIGNAL("MidiProgramCallback(int, int)"), SLOT("slot_handleMidiProgramCallback(int, int)"))
        self.connect(self, SIGNAL("NoteOnCallback(int, int, int)"), SLOT("slot_handleNoteOnCallback(int, int)"))
        self.connect(self, SIGNAL("NoteOffCallback(int, int)"), SLOT("slot_handleNoteOffCallback(int, int)"))
        self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), SLOT("slot_handleShowGuiCallback(int, int)"))
        self.connect(self, SIGNAL("ResizeGuiCallback(int, int, int)"),
            SLOT("slot_handleResizeGuiCallback(int, int, int)"))
        self.connect(self, SIGNAL("UpdateCallback(int)"), SLOT("slot_handleUpdateCallback(int)"))
        self.connect(self, SIGNAL("ReloadInfoCallback(int)"), SLOT("slot_handleReloadInfoCallback(int)"))
        self.connect(self, SIGNAL("ReloadParametersCallback(int)"), SLOT("slot_handleReloadParametersCallback(int)"))
        self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), SLOT("slot_handleReloadProgramsCallback(int)"))
        self.connect(self, SIGNAL("ReloadAllCallback(int)"), SLOT("slot_handleReloadAllCallback(int)"))
        self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        self.TIMER_GUI_STUFF = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])   # Peaks
        self.TIMER_GUI_STUFF2 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 2) # LEDs and edit dialog

    def callback_function(self, action, plugin_id, value1, value2, value3):
        if (plugin_id < 0 or plugin_id >= MAX_PLUGINS):
            return

        if (action == CALLBACK_DEBUG):
            self.emit(SIGNAL("DebugCallback(int, int, int, double)"), plugin_id, value1, value2, value3)
        elif (action == CALLBACK_PARAMETER_CHANGED):
            self.emit(SIGNAL("ParameterCallback(int, int, double)"), plugin_id, value1, value3)
        elif (action == CALLBACK_PROGRAM_CHANGED):
            self.emit(SIGNAL("ProgramCallback(int, int)"), plugin_id, value1)
        elif (action == CALLBACK_MIDI_PROGRAM_CHANGED):
            self.emit(SIGNAL("MidiProgramCallback(int, int)"), plugin_id, value1)
        elif (action == CALLBACK_NOTE_ON):
            self.emit(SIGNAL("NoteOnCallback(int, int, int)"), plugin_id, value1, value2)
        elif (action == CALLBACK_NOTE_OFF):
            self.emit(SIGNAL("NoteOffCallback(int, int)"), plugin_id, value1)
        elif (action == CALLBACK_SHOW_GUI):
            self.emit(SIGNAL("ShowGuiCallback(int, int)"), plugin_id, value1)
        elif (action == CALLBACK_RESIZE_GUI):
            self.emit(SIGNAL("ResizeGuiCallback(int, int, int)"), plugin_id, value1, value2)
        elif (action == CALLBACK_UPDATE):
            self.emit(SIGNAL("UpdateCallback(int)"), plugin_id)
        elif (action == CALLBACK_RELOAD_INFO):
            self.emit(SIGNAL("ReloadInfoCallback(int)"), plugin_id)
        elif (action == CALLBACK_RELOAD_PARAMETERS):
            self.emit(SIGNAL("ReloadParametersCallback(int)"), plugin_id)
        elif (action == CALLBACK_RELOAD_PROGRAMS):
            self.emit(SIGNAL("ReloadProgramsCallback(int)"), plugin_id)
        elif (action == CALLBACK_RELOAD_ALL):
            self.emit(SIGNAL("ReloadAllCallback(int)"), plugin_id)
        elif (action == CALLBACK_QUIT):
            self.emit(SIGNAL("QuitCallback()"))

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_file_save()"))

    @pyqtSlot(int, int, int, float)
    def slot_handleDebugCallback(self, plugin_id, value1, value2, value3):
        print("DEBUG :: %i, %i, %i, %f)" % (plugin_id, value1, value2, value3))

    @pyqtSlot(int, int, float)
    def slot_handleParameterCallback(self, plugin_id, parameter_id, value):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.parameter_activity_timer = ICON_STATE_ON

            if (parameter_id == PARAMETER_ACTIVE):
                pwidget.set_active((value > 0.0), True, False)
            elif (parameter_id == PARAMETER_DRYWET):
                pwidget.set_drywet(value * 1000, True, False)
            elif (parameter_id == PARAMETER_VOLUME):
                pwidget.set_volume(value * 1000, True, False)
            elif (parameter_id == PARAMETER_BALANCE_LEFT):
                pwidget.set_balance_left(value * 1000, True, False)
            elif (parameter_id == PARAMETER_BALANCE_RIGHT):
                pwidget.set_balance_right(value * 1000, True, False)
            elif (parameter_id >= 0):
                pwidget.edit_dialog.set_parameter_to_update(parameter_id)

    @pyqtSlot(int, int)
    def slot_handleProgramCallback(self, plugin_id, program_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.set_program(program_id)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramCallback(self, plugin_id, midi_program_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.set_midi_program(midi_program_id)

    @pyqtSlot(int, int)
    def slot_handleNoteOnCallback(self, plugin_id, note):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.keyboard.noteOn(note, False)

    @pyqtSlot(int, int)
    def slot_handleNoteOffCallback(self, plugin_id, note):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.keyboard.noteOff(note, False)

    @pyqtSlot(int, int)
    def slot_handleShowGuiCallback(self, plugin_id, show):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            if (show == 0):
                pwidget.b_gui.setChecked(False)
                pwidget.b_gui.setEnabled(True)
            elif (show == 1):
                pwidget.b_gui.setChecked(True)
                pwidget.b_gui.setEnabled(True)
            elif (show == -1):
                pwidget.b_gui.setChecked(False)
                pwidget.b_gui.setEnabled(False)

    @pyqtSlot(int, int, int)
    def slot_handleResizeGuiCallback(self, plugin_id, width, height):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            gui_dialog = pwidget.gui_dialog
            if (gui_dialog):
                gui_dialog.setNewSize(width, height)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.do_update()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.do_reload_info()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.do_reload_parameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.do_reload_programs()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if (pwidget):
            pwidget.edit_dialog.do_reload_all()

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("JACK has been stopped or crashed.\nPlease start JACK and restart Carla"),
            "You may want to save your session now...", QMessageBox.Ok, QMessageBox.Ok)

    def add_plugin(self, btype, ptype, filename, label, extra_stuff, activate):
        new_plugin_id = CarlaHost.add_plugin(btype, ptype, filename, label, extra_stuff)

        if (new_plugin_id < 0):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                c_string(CarlaHost.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
        else:
            pwidget = PluginWidget(self, new_plugin_id)
            self.w_plugins.layout().addWidget(pwidget)
            self.m_plugin_list[new_plugin_id] = pwidget
            self.act_plugin_remove_all.setEnabled(True)

            pwidget.peak_in.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])
            pwidget.peak_out.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])

            if (activate):
                pwidget.set_active(True, True, True)

        return new_plugin_id

    def remove_plugin(self, plugin_id, showError):
        pwidget = self.m_plugin_list[plugin_id]
        pwidget.edit_dialog.close()

        if (pwidget.gui_dialog):
            pwidget.gui_dialog.close()

        if (CarlaHost.remove_plugin(plugin_id)):
            pwidget.close()
            pwidget.deleteLater()
            self.w_plugins.layout().removeWidget(pwidget)
            self.m_plugin_list[plugin_id] = None

        else:
            if (showError):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to remove plugin"),
                    c_string(CarlaHost.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        for i in range(MAX_PLUGINS):
            if (self.m_plugin_list[i] != None):
                self.act_plugin_remove_all.setEnabled(True)
                break
        else:
            self.act_plugin_remove_all.setEnabled(False)

    def get_extra_stuff(self, plugin):
        build = plugin['build']
        ptype = plugin['type']

        if (build != BINARY_NATIVE):
            # Store object so we can return a pointer
            if (self.m_bridge_info == None):
                self.m_bridge_info = PluginBridgeInfo()
            self.m_bridge_info.category  = plugin['category']
            self.m_bridge_info.hints     = plugin['hints']
            self.m_bridge_info.name      = plugin['name'].encode("utf-8")
            self.m_bridge_info.maker     = plugin['maker'].encode("utf-8")
            self.m_bridge_info.unique_id = plugin['unique_id']
            self.m_bridge_info.ains  = plugin['audio.ins']
            self.m_bridge_info.aouts = plugin['audio.outs']
            self.m_bridge_info.mins  = plugin['midi.ins']
            self.m_bridge_info.mouts = plugin['midi.outs']
            return pointer(self.m_bridge_info)

        elif (ptype == PLUGIN_LADSPA):
            unique_id = plugin['unique_id']
            for rdf_item in self.ladspa_rdf_list:
                if (rdf_item.UniqueID == unique_id):
                    return pointer(rdf_item)
            else:
                return c_nullptr

        elif (ptype == PLUGIN_DSSI):
            if (plugin['hints'] & PLUGIN_HAS_GUI):
                gui = findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
                if (gui):
                    return gui.encode("utf-8")
            return c_nullptr

        else:
            return c_nullptr

    def save_project(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PROJECT>\n"
                   "<CARLA-PROJECT VERSION='%s'>\n") % (VERSION)

        first_plugin = True

        for pwidget in self.m_plugin_list:
            if (pwidget):
                if (not first_plugin):
                    content += "\n"

                real_plugin_name = c_string(CarlaHost.get_real_plugin_name(pwidget.plugin_id))
                if (real_plugin_name):
                    content += " <!-- %s -->\n" % real_plugin_name

                content += " <Plugin>\n"
                content += pwidget.getSaveXMLContent()
                content += " </Plugin>\n"

                first_plugin = False

        content += "</CARLA-PROJECT>\n"

        try:
            open(self.m_project_filename, "w").write(content)
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save project file"))

    def load_project(self):
        try:
            project_read = open(self.m_project_filename, "r").read()
        except:
            project_read = None

        if not project_read:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load project file"))
            return

        xml = QDomDocument()
        xml.setContent(project_read)

        xml_node = xml.documentElement()
        if (xml_node.tagName() != "CARLA-PROJECT"):
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla project file"))
            return

        x_ladspa_plugins = None
        x_dssi_plugins = None
        x_lv2_plugins = None
        x_vst_plugins = None
        x_sf2_plugins = None

        x_failed_plugins = []
        x_save_state_dicts = []

        node = xml_node.firstChild()
        while not node.isNull():
            if (node.toElement().tagName() == "Plugin"):
                x_save_state_dict = getStateDictFromXML(node)
                x_save_state_dicts.append(x_save_state_dict)
            node = node.nextSibling()

        for x_save_state_dict in x_save_state_dicts:
            ptype = x_save_state_dict['Type']
            label = x_save_state_dict['Label']
            binary = x_save_state_dict['Binary']
            binaryS = getShortFileName(binary)
            unique_id = x_save_state_dict['UniqueID']

            if (ptype == "LADSPA"):
                if (not x_ladspa_plugins): x_ladspa_plugins = toList(self.settings_db.value("Plugins/LADSPA", []))
                x_plugins = x_ladspa_plugins

            elif (ptype == "DSSI"):
                if (not x_dssi_plugins): x_dssi_plugins = toList(self.settings_db.value("Plugins/DSSI", []))
                x_plugins = x_dssi_plugins

            elif (ptype == "LV2"):
                if (not x_lv2_plugins): x_lv2_plugins = toList(self.settings_db.value("Plugins/LV2", []))
                x_plugins = x_lv2_plugins

            elif (ptype == "VST"):
                if (not x_vst_plugins): x_vst_plugins = toList(self.settings_db.value("Plugins/VST", []))
                x_plugins = x_vst_plugins

            elif (ptype == "SoundFont"):
                if (not x_sf2_plugins): x_sf2_plugins = toList(self.settings_db.value("Plugins/SF2", []))
                x_plugins = x_sf2_plugins

            else:
                x_failed_plugins.append(x_save_state_dict['Name'])
                continue

            # Try UniqueID -> Label -> Binary (full) -> Binary (short)
            plugin_ulB = None
            plugin_ulb = None
            plugin_ul = None
            plugin_uB = None
            plugin_ub = None
            plugin_lB = None
            plugin_lb = None
            plugin_u = None
            plugin_l = None
            plugin_B = None

            for _plugins in x_plugins:
                for x_plugin in _plugins:
                    if (
                        unique_id == x_plugin['unique_id'] and label == x_plugin['label'] and binary == x_plugin[
                                                                                                        'binary']):
                        plugin_ulB = x_plugin
                        break
                    elif (
                        unique_id == x_plugin['unique_id'] and label == x_plugin[
                                                                        'label'] and binaryS == getShortFileName(
                            x_plugin['binary'])):
                        plugin_ulb = x_plugin
                    elif (unique_id == x_plugin['unique_id'] and label == x_plugin['label']):
                        plugin_ul = x_plugin
                    elif (unique_id == x_plugin['unique_id'] and binary == x_plugin['binary']):
                        plugin_uB = x_plugin
                    elif (unique_id == x_plugin['unique_id'] and binaryS == getShortFileName(x_plugin['binary'])):
                        plugin_ub = x_plugin
                    elif (label == x_plugin['label'] and binary == x_plugin['binary']):
                        plugin_lB = x_plugin
                    elif (label == x_plugin['label'] and binaryS == getShortFileName(x_plugin['binary'])):
                        plugin_lb = x_plugin
                    elif (unique_id == x_plugin['unique_id']):
                        plugin_u = x_plugin
                    elif (label == x_plugin['label']):
                        plugin_l = x_plugin
                    elif (binary == x_plugin['binary']):
                        plugin_B = x_plugin

            # LADSPA uses UniqueID or binary+label
            if (ptype == "LADSPA"):
                plugin_l = None
                plugin_B = None

            # DSSI uses binary+label (UniqueID ignored)
            elif (ptype == "DSSI"):
                plugin_ul = None
                plugin_uB = None
                plugin_ub = None
                plugin_u = None
                plugin_l = None
                plugin_B = None

            # LV2 uses URIs (label in this case)
            elif (ptype == "LV2"):
                plugin_uB = None
                plugin_ub = None
                plugin_u = None
                plugin_B = None

            # VST uses UniqueID
            elif (ptype == "VST"):
                plugin_lB = None
                plugin_lb = None
                plugin_l = None
                plugin_B = None

            # SoundFonts use binaries
            elif (ptype == "SF2"):
                plugin_ul = None
                plugin_u = None
                plugin_l = None

            if (plugin_ulB):
                plugin = plugin_ulB
            elif (plugin_ulb):
                plugin = plugin_ulb
            elif (plugin_ul):
                plugin = plugin_ul
            elif (plugin_uB):
                plugin = plugin_uB
            elif (plugin_ub):
                plugin = plugin_ub
            elif (plugin_lB):
                plugin = plugin_lB
            elif (plugin_lb):
                plugin = plugin_lb
            elif (plugin_u):
                plugin = plugin_u
            elif (plugin_l):
                plugin = plugin_l
            elif (plugin_B):
                plugin = plugin_B
            else:
                plugin = None

            if (plugin):
                btype = plugin['build']
                ptype = plugin['type']
                filename = plugin['binary']
                label = plugin['label']
                extra_stuff = self.get_extra_stuff(plugin)
                new_plugin_id = self.add_plugin(btype, ptype, filename, label, extra_stuff, False)

                if (new_plugin_id >= 0):
                    pwidget = self.m_plugin_list[new_plugin_id]
                    pwidget.loadStateDict(x_save_state_dict)

                else:
                    x_failed_plugins.append(x_save_state_dict['Name'])

            else:
                x_failed_plugins.append(x_save_state_dict['Name'])

        if (len(x_failed_plugins) > 0):
            text = self.tr("The following plugins were not found or failed to initialize:\n")
            for plugin in x_failed_plugins:
                text += plugin
                text += "\n"

            self.statusBar().showMessage("State file loaded with errors")
            QMessageBox.critical(self, self.tr("Error"), text)

        else:
            self.statusBar().showMessage("State file loaded sucessfully!")

    def loadRDFs(self):
        # Save RDF info for later
        if haveLRDF:
            SettingsDir = os.path.join(HOME, ".config", "Cadence")

            fr_ladspa_file = os.path.join(SettingsDir, "ladspa_rdf.db")
            if (os.path.exists(fr_ladspa_file)):
                fr_ladspa = open(fr_ladspa_file, 'r')
                if (fr_ladspa):
                    try:
                        self.ladspa_rdf_list = ladspa_rdf.get_c_ladspa_rdfs(json.load(fr_ladspa))
                    except:
                        self.ladspa_rdf_list = []
                    fr_ladspa.close()
                    return

        self.ladspa_rdf_list = []

    @pyqtSlot()
    def slot_file_new(self):
        self.slot_remove_all()
        self.m_project_filename = None
        self.setWindowTitle("Carla")

    @pyqtSlot()
    def slot_file_open(self):
        file_filter = self.tr("Carla Project File (*.carxp)")
        filename = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"),
            self.m_savedSettings["Main/DefaultProjectFolder"], filter=file_filter)

        if (filename):
            self.m_project_filename = filename
            self.slot_remove_all()
            self.load_project()
            self.setWindowTitle("Carla - %s" % (getShortFileName(self.m_project_filename)))

    @pyqtSlot()
    def slot_file_save(self, saveAs=False):
        if (self.m_project_filename == None or saveAs):
            file_filter = self.tr("Carla Project File (*.carxp)")
            filename = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"),
                self.m_savedSettings["Main/DefaultProjectFolder"], filter=file_filter)

            if (filename):
                self.m_project_filename = filename
                self.save_project()
                self.setWindowTitle("Carla - %s" % (getShortFileName(self.m_project_filename)))

        else:
            self.save_project()

    @pyqtSlot()
    def slot_file_save_as(self):
        self.slot_file_save(True)

    @pyqtSlot()
    def slot_plugin_add(self):
        dialog = PluginDatabaseW(self)
        if (dialog.exec_()):
            btype = dialog.ret_plugin['build']
            ptype = dialog.ret_plugin['type']
            filename = dialog.ret_plugin['binary']
            label = dialog.ret_plugin['label']
            extra_stuff = self.get_extra_stuff(dialog.ret_plugin)
            self.add_plugin(btype, ptype, filename, label, extra_stuff, True)

    @pyqtSlot()
    def slot_remove_all(self):
        for i in range(MAX_PLUGINS):
            if (self.m_plugin_list[i]):
                self.remove_plugin(i, False)

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = SettingsW(self, "carla")
        if (dialog.exec_()):
            self.loadSettings(False)

            for pwidget in self.m_plugin_list:
                if (pwidget):
                    pwidget.peak_in.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])
                    pwidget.peak_out.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])

    @pyqtSlot()
    def slot_aboutCarla(self):
        AboutW(self).exec_()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowToolbar", self.toolBar.isVisible())

    def loadSettings(self, geometry):
        if (geometry):
            self.restoreGeometry(self.settings.value("Geometry", ""))

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.toolBar.setVisible(show_toolbar)

        self.m_savedSettings = {
            "Main/DefaultProjectFolder": self.settings.value("Main/DefaultProjectFolder", DEFAULT_PROJECT_FOLDER,
                type=str),
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int)
        }

        global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, SF2_PATH
        LADSPA_PATH = toList(self.settings.value("Paths/LADSPA", LADSPA_PATH))
        DSSI_PATH = toList(self.settings.value("Paths/DSSI", DSSI_PATH))
        LV2_PATH = toList(self.settings.value("Paths/LV2", LV2_PATH))
        VST_PATH = toList(self.settings.value("Paths/VST", VST_PATH))
        SF2_PATH = toList(self.settings.value("Paths/SF2", SF2_PATH))

        CarlaHost.set_option(OPTION_PATH_LADSPA, 0, splitter.join(LADSPA_PATH))
        CarlaHost.set_option(OPTION_PATH_DSSI, 0, splitter.join(DSSI_PATH))
        CarlaHost.set_option(OPTION_PATH_LV2, 0, splitter.join(LV2_PATH))
        CarlaHost.set_option(OPTION_PATH_VST, 0, splitter.join(VST_PATH))
        CarlaHost.set_option(OPTION_PATH_SF2, 0, splitter.join(SF2_PATH))

    def timerEvent(self, event):
        if (event.timerId() == self.TIMER_GUI_STUFF):
            for pwidget in self.m_plugin_list:
                if (pwidget): pwidget.check_gui_stuff()
            CarlaHost.idle_guis()
        elif (event.timerId() == self.TIMER_GUI_STUFF2):
            for pwidget in self.m_plugin_list:
                if (pwidget): pwidget.check_gui_stuff2()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        self.slot_remove_all()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("falkTX")
    app.setWindowIcon(QIcon(":/scalable/carla.svg"))

    lib_prefix = None
    project_filename = None

    for i in range(len(app.arguments())):
        if (i == 0): continue
        argument = app.arguments()[i]

        if argument.startswith("--with-libprefix="):
            lib_prefix = argument.replace("--with-libprefix=", "")

        elif (os.path.exists(argument)):
            project_filename = argument

    #style = app.style().metaObject().className()
    #force_parameters_style = (style in ("Bespin::Style",))

    CarlaHost = Host(lib_prefix)

    # Create GUI and read settings
    gui = CarlaMainW()

    # Init backend
    CarlaHost.set_option(OPTION_GLOBAL_JACK_CLIENT, gui.settings.value("Engine/GlobalClient", False, type=bool), "")
    CarlaHost.set_option(OPTION_USE_DSSI_CHUNKS, gui.settings.value("Engine/DSSIChunks", False, type=bool), "")
    CarlaHost.set_option(OPTION_PREFER_UI_BRIDGES, gui.settings.value("Engine/PreferBridges", True, type=bool), "")

    if (carla_bridge_unix32):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_UNIX32, 0, carla_bridge_unix32)

    if (carla_bridge_unix64):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_UNIX64, 0, carla_bridge_unix64)

    if (carla_bridge_win32):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_WIN32, 0, carla_bridge_win32)

    if (carla_bridge_win64):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_WIN64, 0, carla_bridge_win64)

    if (carla_bridge_lv2_gtk2):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_LV2_GTK2, 0, carla_bridge_lv2_gtk2)

    if (carla_bridge_lv2_qt4):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_LV2_QT4, 0, carla_bridge_lv2_qt4)

    if (carla_bridge_lv2_x11):
        CarlaHost.set_option(OPTION_PATH_BRIDGE_LV2_X11, 0, carla_bridge_lv2_x11)

    if (not CarlaHost.carla_init("Carla")):
        CustomMessageBox(None, QMessageBox.Critical, "Error", "Could not connect to JACK",
            c_string(CarlaHost.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
        sys.exit(1)

    # Set-up custom signal handling
    set_up_signals(gui)

    # Show GUI
    gui.show()

    if (project_filename):
        gui.m_project_filename = project_filename
        gui.load_project()
        gui.setWindowTitle("Carla - %s" % (getShortFileName(project_filename)))

    # App-Loop
    ret = app.exec_()

    # Close Host
    if (CarlaHost.carla_is_engine_running()):
        if (not CarlaHost.carla_close()):
            print(c_string(CarlaHost.get_last_error()))

    # Exit properly
    sys.exit(ret)

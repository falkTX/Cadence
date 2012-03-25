#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com> FIXME
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
import json, os, sys
#from PyQt4.QtCore import Qt, QSettings, QThread, QTimer, QVariant, SIGNAL, SLOT
#from PyQt4.QtGui import QApplication, QColor, QCursor, QFileDialog, QFontMetrics, QInputDialog, QMenu, QPainter, QPixmap, QVBoxLayout
from time import sleep
#from sip import unwrapinstance
from PyQt4.QtCore import pyqtSlot, Qt, QSettings, QThread
from PyQt4.QtGui import QApplication, QDialog, QFrame, QMainWindow, QTableWidgetItem, QWidget
from PyQt4.QtXml import QDomDocument

# Imports (Custom Stuff)
import ui_carla, ui_carla_about, ui_carla_database, ui_carla_edit, ui_carla_parameter, ui_carla_plugin, ui_carla_refresh
from carla_backend import *
from shared import *

ICON_STATE_WAIT = 0
ICON_STATE_OFF  = 1
ICON_STATE_ON   = 2

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
  'Vol': 1.0,
  'Balance-Left': -1.0,
  'Balance-Right': 1.0,
  'Parameters': [],
  'ProgramIndex': -1,
  'ProgramName': "",
  'MidiBank': -1,
  'MidiProgram': -1,
  'CustomData': [],
  'Chunk': None
}

save_state_parameter = {
  'index': 0,
  'rindex': 0,
  'name': "",
  'symbol': "",
  'value': 0.0,
  'midi_channel': 1,
  'midi_cc': -1
}

save_state_custom_data = {
  'type': "",
  'key': "",
  'value': ""
}

def is_number(value):
  string = str(value)
  if (string != ""):
    if (string[0] == "-"):
      string = string.replace("-","",1)
    sstring = string.split(".")
    if (len(sstring) == 1 and sstring[0].isdigit()):
      return True
    elif (len(sstring) == 2 and sstring[0].isdigit() and sstring[1].isdigit()):
      return True
    else:
      return False
  else:
    return False

def getStateSaveDict(xml_node):
    x_save_state_dict = deepcopy(save_state_dict)

    node = xml_node.firstChild()
    while not node.isNull():
      if (node.toElement().tagName() == "Info"):
        xml_info = node.toElement().firstChild()

        while not xml_info.isNull():
          tag  = QStringStr(xml_info.toElement().tagName())
          text = QStringStr(xml_info.toElement().text())
          if (tag == "Type"):
            x_save_state_dict['Type'] = text
          elif (tag == "Name"):
            x_save_state_dict['Name'] = text
          elif (tag == "Label"):
            x_save_state_dict['Label'] = text
          elif (tag == "Binary"):
            x_save_state_dict['Binary'] = text
          elif (tag == "UniqueID"):
            if (text.isdigit()):
              x_save_state_dict['UniqueID'] = long(text)
          xml_info = xml_info.nextSibling()

      elif (node.toElement().tagName() == "Data"):
        xml_data = node.toElement().firstChild()

        while not xml_data.isNull():
          tag  = QStringStr(xml_data.toElement().tagName())
          text = QStringStr(xml_data.toElement().text())

          if (tag == "Active"):
            x_save_state_dict['Active'] = bool(text == "Yes")
          elif (tag == "DryWet"):
            if (is_number(text)):
              x_save_state_dict['DryWet'] = float(text)
          elif (tag == "Vol"):
            if (is_number(text)):
              x_save_state_dict['Vol'] = float(text)
          elif (tag == "Balance-Left"):
            if (is_number(text)):
              x_save_state_dict['Balance-Left'] = float(text)
          elif (tag == "Balance-Right"):
            if (is_number(text)):
              x_save_state_dict['Balance-Right'] = float(text)
          elif (tag == "ProgramIndex"):
            if (is_number(text)):
              x_save_state_dict['ProgramIndex'] = int(text)
          elif (tag == "ProgramName"):
            x_save_state_dict['ProgramName'] = text
          elif (tag == "MidiBank"):
            if (is_number(text)):
              x_save_state_dict['MidiBank'] = int(text)
          elif (tag == "MidiProgram"):
            if (is_number(text)):
              x_save_state_dict['MidiProgram'] = int(text)
          elif (tag == "Chunk"):
            if (text):
              x_save_state_dict['Chunk'] = text.strip()

          elif (tag == "Parameter"):
            x_save_state_parameter = deepcopy(save_state_parameter)

            xml_subdata = xml_data.toElement().firstChild()
            while not xml_subdata.isNull():
              ptag  = QStringStr(xml_subdata.toElement().tagName())
              ptext = QStringStr(xml_subdata.toElement().text())

              if (ptag == "index"):
                if (is_number(ptext)):
                  x_save_state_parameter['index'] = int(ptext)
              elif (ptag == "rindex"):
                if (is_number(ptext)):
                  x_save_state_parameter['rindex'] = int(ptext)
              elif (ptag == "name"):
                x_save_state_parameter['name'] = ptext
              elif (ptag == "symbol"):
                x_save_state_parameter['symbol'] = ptext
              elif (ptag == "value"):
                if (is_number(ptext)):
                  x_save_state_parameter['value'] = float(ptext)
              elif (ptag == "midi_channel"):
                if (is_number(ptext)):
                  x_save_state_parameter['midi_channel'] = int(ptext)
              elif (ptag == "midi_cc"):
                if (is_number(ptext)):
                  x_save_state_parameter['midi_cc'] = int(ptext)

              xml_subdata = xml_subdata.nextSibling()

            x_save_state_dict['Parameters'].append(x_save_state_parameter)

          elif (tag == "CustomData"):
            x_save_state_custom_data = deepcopy(save_state_custom_data)

            xml_subdata = xml_data.toElement().firstChild()
            while not xml_subdata.isNull():
              ctag  = QStringStr(xml_subdata.toElement().tagName())
              ctext = QStringStr(xml_subdata.toElement().text())

              if (ctag == "type"):
                if (is_number(ctext)):
                  x_save_state_custom_data['type'] = int(ctext)
              elif (ctag == "key"):
                x_save_state_custom_data['key'] = ctext
              elif (ctag == "value"):
                if (ctext):
                  x_value = ctext.strip()
                  x_save_state_custom_data['value'] = x_value.replace("&lt;","<").replace("&gt;",">").replace("&quot;","\"")

              xml_subdata = xml_subdata.nextSibling()

            x_save_state_dict['CustomData'].append(x_save_state_custom_data)

          xml_data = xml_data.nextSibling()

      node = node.nextSibling()

    return x_save_state_dict

def strPyPluginInfo(qt_pinfo):
    pinfo = deepcopy(PyPluginInfo)
    pinfo['type']      = qt_pinfo[QString('type')]
    pinfo['category']  = qt_pinfo[QString('category')]
    pinfo['hints']     = qt_pinfo[QString('hints')]
    pinfo['binary']    = QStringStr(qt_pinfo[QString('binary')])
    pinfo['name']      = QStringStr(qt_pinfo[QString('name')])
    pinfo['label']     = QStringStr(qt_pinfo[QString('label')])
    pinfo['maker']     = QStringStr(qt_pinfo[QString('maker')])
    pinfo['copyright'] = QStringStr(qt_pinfo[QString('copyright')])
    pinfo['id']        = QStringStr(qt_pinfo[QString('id')])
    pinfo['audio.ins']   = qt_pinfo[QString('audio.ins')]
    pinfo['audio.outs']  = qt_pinfo[QString('audio.outs')]
    pinfo['audio.total'] = qt_pinfo[QString('audio.total')]
    pinfo['midi.ins']    = qt_pinfo[QString('midi.ins')]
    pinfo['midi.outs']   = qt_pinfo[QString('midi.outs')]
    pinfo['midi.total']  = qt_pinfo[QString('midi.total')]
    pinfo['parameters.ins']   = qt_pinfo[QString('parameters.ins')]
    pinfo['parameters.outs']  = qt_pinfo[QString('parameters.outs')]
    pinfo['parameters.total'] = qt_pinfo[QString('parameters.total')]
    pinfo['programs.total']   = qt_pinfo[QString('programs.total')]
    return pinfo

# Separate Thread for Plugin Search
class SearchPluginsThread(QThread):
    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.settings_db = self.parent().settings_db

        self.check_ladspa = True
        self.check_dssi   = True
        self.check_lv2    = True
        self.check_vst    = True
        self.check_sf2    = True

        self.check_native = None
        self.check_bins   = []

    def skipPlugin(self):
        # TODO - windows support
        os.system("killall -KILL carla-discovery carla-discovery-unix32 carla-discovery-unix64 carla-discovery-win32.exe carla-discovery-win64.exe")

    def pluginLook(self, percent, plugin):
        self.emit(SIGNAL("PluginLook(int, QString)"), percent, plugin)

    def setSearchBins(self, bins):
        self.check_bins = bins

    def setSearchNative(self, native):
        self.check_native = native

    def setSearchTypes(self, ladspa, dssi, lv2, vst, sf2):
        self.check_ladspa = ladspa
        self.check_dssi   = dssi
        self.check_lv2    = lv2
        self.check_vst    = vst
        self.check_sf2    = sf2

    def setLastLoadedBinary(self, binary):
        self.settings_db.setValue("Plugins/LastLoadedBinary", binary)

    def run(self):
        blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))
        bins   = []
        bins_w = []

        m_count = type_count = 0
        if (self.check_ladspa): m_count += 1
        if (self.check_dssi):   m_count += 1
        if (self.check_vst):    m_count += 1

        check_native = check_wine = False

        if (LINUX):
          OS = "LINUX"
        elif (MACOS):
          OS = "MACOS"
        elif (WINDOWS):
          OS = "WINDOWS"
        else:
          OS = "UNKNOWN"

        if (LINUX or MACOS):
          if (carla_discovery_unix32 in self.check_bins or carla_discovery_unix64 in self.check_bins):
            type_count  += m_count
            check_native = True
            if (carla_discovery_unix32 in self.check_bins):
              bins.append(carla_discovery_unix32)
            if (carla_discovery_unix64 in self.check_bins):
              bins.append(carla_discovery_unix64)

          if (carla_discovery_win32 in self.check_bins or carla_discovery_win64 in self.check_bins):
            type_count += m_count
            check_wine  = True
            if (carla_discovery_win32 in self.check_bins):
              bins_w.append(carla_discovery_win32)
            if (carla_discovery_win64 in self.check_bins):
              bins_w.append(carla_discovery_win64)

        elif (WINDOWS):
          if (carla_discovery_win32 in self.check_bins or carla_discovery_win64 in self.check_bins):
            type_count  += m_count
            check_native = True
            if (carla_discovery_win32 in self.check_bins):
              bins.append(carla_discovery_win32)
            if (carla_discovery_win64 in self.check_bins):
              bins.append(carla_discovery_win64)

        if (self.check_lv2): type_count += 1
        if (self.check_sf2): type_count += 1

        if (type_count == 0):
          return

        ladspa_plugins  = []
        dssi_plugins    = []
        lv2_plugins     = []
        vst_plugins     = []
        soundfonts      = []

        ladspa_rdf_info = []
        lv2_rdf_info    = []

        last_value = 0
        percent_value = 100/type_count

        # ----- LADSPA
        if (self.check_ladspa):

          if (check_native):
            ladspa_binaries = []

            for PATH in LADSPA_PATH:
              binaries = findBinaries(PATH, OS)
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
                self.pluginLook((last_value + percent)*0.9, ladspa)
                self.setLastLoadedBinary(ladspa)
                for bin_ in bins:
                  plugins = checkPluginLADSPA(ladspa, bin_)
                  if (plugins != None):
                    ladspa_plugins.append(plugins)

            last_value += percent_value

          if (check_wine):
            ladspa_binaries_w = []

            for PATH in LADSPA_PATH:
              binaries = findBinaries(PATH, "WINDOWS")
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
                self.pluginLook((last_value + percent)*0.9, ladspa_w)
                self.setLastLoadedBinary(ladspa_w)
                for bin_w in bins_w:
                  plugins_w = checkPluginLADSPA(ladspa_w, bin_w, True)
                  if (plugins_w != None):
                    ladspa_plugins.append(plugins_w)

            last_value += percent_value

          if (haveRDF):
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

            for PATH in DSSI_PATH:
              binaries = findBinaries(PATH, OS)
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

            for PATH in DSSI_PATH:
              binaries = findBinaries(PATH, "WINDOWS")
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

        ## ----- LV2
        #if (self.check_lv2 and haveRDF):
          #self.disccover_skip_kill = ""
          #self.pluginLook(self.last_value, "LV2 bundles...")
          #lv2_rdf_info = lv2_rdf.recheck_all_plugins(self)
          #for info in lv2_rdf_info:
            #plugins = checkPluginLV2(info)
            #if (plugins != None):
              #lv2_plugins.append(plugins)

          #self.last_value += self.percent_value

        # ----- VST
        if (self.check_vst):

          if (check_native):
            vst_binaries = []

            for PATH in VST_PATH:
              binaries = findBinaries(PATH, OS)
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

            for PATH in VST_PATH:
              binaries = findBinaries(PATH, "WINDOWS")
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
          for PATH in SF2_PATH:
            files = findSoundFonts(PATH)
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

        if (haveRDF):
          SettingsDir = os.path.join(HOME, ".config", "Cadence")

          if (self.check_ladspa):
            f_ladspa = open(os.path.join(SettingsDir, "ladspa_rdf.db"), 'w')
            if (f_ladspa):
              json.dump(ladspa_rdf_info, f_ladspa)
              f_ladspa.close()

          #if (self.check_lv2):
            #f_lv2 = open(os.path.join(SettingsDir, "lv2_rdf.db"), 'w')
            #if (f_lv2):
              #json.dump(lv2_rdf_info, f_lv2)
              #f_lv2.close()

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

        if (haveRDF):
          self.ico_rdflib.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
          self.ico_rdflib.setPixmap(getIcon("dialog-error").pixmap(16, 16))
          self.ch_lv2.setChecked(False)
          self.ch_lv2.setEnabled(False)

        if (LINUX or MACOS):
          if (is64bit):
            hasNative    = bool(carla_discovery_unix64)
            hasNonNative = bool(carla_discovery_unix32 or carla_discovery_win32 or carla_discovery_win64)
            self.pThread.setSearchNative(carla_discovery_unix64)
          else:
            hasNative    = bool(carla_discovery_unix32)
            hasNonNative = bool(carla_discovery_unix64 or carla_discovery_win32 or carla_discovery_win64)
            self.pThread.setSearchNative(carla_discovery_unix32)
        elif (WINDOWS):
          if (is64bit):
            hasNative    = bool(carla_discovery_win64)
            hasNonNative = bool(carla_discovery_win32)
            self.pThread.setSearchNative(carla_discovery_win64)
          else:
            hasNative    = bool(carla_discovery_win32)
            hasNonNative = bool(carla_discovery_win64)
            self.pThread.setSearchNative(carla_discovery_win32)
        else:
          hasNative = False

        if (not hasNative):
          self.ch_sf2.setChecked(False)
          self.ch_sf2.setEnabled(False)

          if (not hasNonNative):
            self.ch_ladspa.setChecked(False)
            self.ch_ladspa.setEnabled(False)
            self.ch_dssi.setChecked(False)
            self.ch_dssi.setEnabled(False)
            self.ch_vst.setChecked(False)
            self.ch_vst.setEnabled(False)

            if (not haveRDF):
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

# Plugin Database Dialog
class PluginDatabaseW(QDialog, ui_carla_database.Ui_PluginDatabaseW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.b_add.setEnabled(False)

        if (BINARY_NATIVE in (BINARY_UNIX32, BINARY_WIN32)):
          self.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
          self.ch_bridged.setText(self.tr("Bridged (32bit)"))

        self.settings = self.parent().settings
        self.settings_db = self.parent().settings_db
        self.loadSettings()

        if (bool(LINUX or MACOS) == False):
          self.ch_bridged_wine.setChecked(False)
          self.ch_bridged_wine.setEnabled(False)

        # Blacklist plugins
        if not self.settings_db.contains("Plugins/Blacklisted"):
          blacklist = []
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
        for i in range(row_count):
          self.tableWidget.removeRow(0)

        self.last_table_index = 0
        self.tableWidget.setSortingEnabled(False)

        ladspa_plugins = toList(self.settings_db.value("Plugins/LADSPA", []))
        dssi_plugins   = toList(self.settings_db.value("Plugins/DSSI", []))
        lv2_plugins    = toList(self.settings_db.value("Plugins/LV2", []))
        vst_plugins    = toList(self.settings_db.value("Plugins/VST", []))
        soundfonts     = toList(self.settings_db.value("Plugins/SF2", []))

        ladspa_count = 0
        dssi_count   = 0
        lv2_count    = 0
        vst_count    = 0
        sf2_count    = 0

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
            self.addPluginToTable(plugin, "SF2")
            sf2_count += 1

        self.slot_checkFilters()
        self.tableWidget.setSortingEnabled(True)
        self.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        self.label.setText(self.tr("Have %i LADSPA, %i DSSI, %i LV2, %i VST and %i SoundFonts" % (ladspa_count, dssi_count, lv2_count, vst_count, sf2_count)))

    def addPluginToTable(self, plugin, ptype):
        index = self.last_table_index

        if (plugin['build'] == BINARY_NATIVE):
          bridge_text = self.tr("No")
        else:
          type_text = self.tr("Unknown")
          if (LINUX or MAC):
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
          ask = QMessageBox.question(self, self.tr("Warning"), self.tr(""
                  "There was an error while checking the plugin %s.\n"
                  "Do you want to blacklist it?" % (lastLoadedPlugin)), QMessageBox.Yes|QMessageBox.No, QMessageBox.Yes)

          if (ask == QMessageBox.Yes):
            blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))
            blacklist.append(lastLoadedPlugin)
            self.settings_db.setValue("Plugins/Blacklisted", blacklist)

        self.label.setText(self.tr("Looking for plugins..."))
        PluginRefreshW(self).exec_()

        self.reAddPlugins()
        #self.parent().loadRDFs()

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
        hide_native  = not self.ch_native.isChecked()
        hide_bridged = not self.ch_bridged.isChecked()
        hide_bridged_wine = not self.ch_bridged_wine.isChecked()
        hide_non_gui = self.ch_gui.isChecked()
        hide_non_stereo = self.ch_stereo.isChecked()

        if (LINUX or MACOS):
          native_bins = [BINARY_UNIX32, BINARY_UNIX64]
          wine_bins   = [BINARY_WIN32, BINARY_WIN64]
        elif (WINDOWS):
          native_bins = [BINARY_WIN32, BINARY_WIN64]
          wine_bins   = []
        else:
          native_bins = []
          wine_bins   = []

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
          is_effect = bool(ains > 0 and aouts > 0 and not is_synth)
          is_midi   = bool(ains == 0 and aouts == 0 and mins > 0 and mouts > 0)
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
                    text in self.tableWidget.item(i, 0).text().toLower() or
                    text in self.tableWidget.item(i, 1).text().toLower() or
                    text in self.tableWidget.item(i, 2).text().toLower() or
                    text in self.tableWidget.item(i, 3).text().toLower() or
                    text in self.tableWidget.item(i, 14).text().toLower())
               ):
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
            "" % (VERSION)))

        host_osc_url = NativeHost.get_host_osc_url()
        if (not host_osc_url): host_osc_url = ""

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
        self.l_lv2.setText(self.tr("About 95&#37; complete (missing state files and other minor features).<br/>"
                                   "Implemented Feature/Extensions:"
                                   "<ul>"
                                   #"<li>http://lv2plug.in/ns/ext/cv-port</li>"
                                   #"<li>http://lv2plug.in/ns/ext/data-access</li>"
                                   #"<li>http://lv2plug.in/ns/ext/event</li>"
                                   #"<li>http://lv2plug.in/ns/ext/host-info</li>"
                                   #"<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                   #"<li>http://lv2plug.in/ns/ext/midi</li>"
                                   #"<li>http://lv2plug.in/ns/ext/port-props</li>"
                                   #"<li>http://lv2plug.in/ns/ext/presets</li>"
                                   #"<li>http://lv2plug.in/ns/ext/state (not files)</li>"
                                   #"<li>http://lv2plug.in/ns/ext/time</li>"
                                   #"<li>http://lv2plug.in/ns/ext/ui-resize</li>"
                                   #"<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                   #"<li>http://lv2plug.in/ns/ext/urid</li>"
                                   #"<li>http://lv2plug.in/ns/extensions/units</li>"
                                   #"<li>http://lv2plug.in/ns/extensions/ui</li>"
                                   #"<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                   #"<li>http://home.gna.org/lv2dynparam/rtmempool/v1</li>"
                                   #"<li>http://nedko.arnaudov.name/lv2/external_ui/</li>"
                                   "</ul>"
                                   "<i>(Note that Gtk2 UIs with instance-access will not work, such as IR.lv2)</i>"))
        self.l_vst.setText(self.tr("<p>About 75&#37; complete (missing MIDI-Output and some minor stuff)</p>"))

# Single Plugin Parameter
class PluginParameter(QWidget, ui_carla_parameter.Ui_PluginParameter):
    def __init__(self, parent=None, pinfo=None, plugin_id=-1):
        super(PluginParameter, self).__init__(parent)
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
          self.widget.set_label(pinfo['label'])
          self.widget.set_step(pinfo['step'])
          self.widget.set_step_small(pinfo['step_small'])
          self.widget.set_step_large(pinfo['step_large'])
          self.widget.set_scalepoints(pinfo['scalepoints'], (pinfo['hints'] & PARAMETER_USES_SCALEPOINTS))

          if (not self.hints & PARAMETER_IS_AUTOMABLE):
            self.combo.setEnabled(False)
            self.sb_channel.setEnabled(False)

          if (not self.hints & PARAMETER_IS_ENABLED):
            self.widget.set_read_only(True)
            self.combo.setEnabled(False)
            self.sb_channel.setEnabled(False)

        elif (self.ptype == PARAMETER_OUTPUT):
          self.widget.set_minimum(pinfo['minimum'])
          self.widget.set_maximum(pinfo['maximum'])
          self.widget.set_value(pinfo['current'], False)
          self.widget.set_label(pinfo['label'])
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

        self.connect(self.widget, SIGNAL("valueChanged(float)"), self.handleValueChanged)
        self.connect(self.sb_channel, SIGNAL("valueChanged(int)"), self.handleMidiChannelChanged)
        self.connect(self.combo, SIGNAL("currentIndexChanged(int)"), self.handleMidiCcChanged)

        if force_parameters_style:
          self.widget.force_plastique_style()

        self.widget.updateAll()

    def set_default_value(self, value):
        self.widget.set_default(value)

    def set_parameter_value(self, value, send=True):
        self.widget.set_value(value, send)

    def set_parameter_midi_channel(self, channel):
        self.midi_channel = channel
        self.sb_channel.setValue(channel-1)

    def set_parameter_midi_cc(self, cc_index):
        self.midi_cc = cc_index
        self.set_MIDI_CC_in_ComboBox(cc_index)

    def handleValueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, float)"), self.parameter_id, value)

    def handleMidiChannelChanged(self, channel):
        if (self.midi_channel != channel):
          self.emit(SIGNAL("midiChannelChanged(int, int)"), self.parameter_id, channel)
        self.midi_channel = channel

    def handleMidiCcChanged(self, cc_index):
        if (cc_index <= 0):
          midi_cc = -1
        else:
          midi_cc_text = QStringStr(MIDI_CC_LIST[cc_index-1]).split(" ")[0]
          midi_cc = int(midi_cc_text, 16)

        if (self.midi_cc != midi_cc):
          self.emit(SIGNAL("midiCcChanged(int, int)"), self.parameter_id, midi_cc)
        self.midi_cc = midi_cc

    def add_MIDI_CCs_to_ComboBox(self):
        for MIDI_CC in MIDI_CC_LIST:
          self.combo.addItem(MIDI_CC)

    def set_MIDI_CC_in_ComboBox(self, midi_cc):
        for i in range(len(MIDI_CC_LIST)):
          midi_cc_text = QStringStr(MIDI_CC_LIST[i]).split(" ")[0]
          if (int(midi_cc_text, 16) == midi_cc):
            cc_index = i
            break
        else:
          cc_index = -1

        cc_index += 1
        self.combo.setCurrentIndex(cc_index)

# Plugin GUI
class PluginGUI(QDialog):
    def __init__(self, parent, plugin_name, gui_data):
        super(PluginGUI, self).__init__(parent)

        self.myLayout = QVBoxLayout(self)
        self.myLayout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.myLayout)

        self.resizable = gui_data['resizable']
        self.set_new_size(gui_data['width'], gui_data['height'])

        if (not plugin_name):
          plugin_name = "Plugin"

        self.setWindowTitle(plugin_name+" (GUI)")

    def set_new_size(self, width, height):
        if (width < 30):
          width = 30
        if (height < 30):
          height = 30

        if (self.resizable):
          self.resize(width, height)
        else:
          self.setFixedSize(width, height)

    def hideEvent(self, event):
        event.accept()
        self.close()

# Plugin Editor (Built-in)
class PluginEdit(QDialog, ui_carla_edit.Ui_PluginEdit):
    def __init__(self, parent, plugin_id):
        super(PluginEdit, self).__init__(parent)
        self.setupUi(self)

        self.pinfo = None
        self.ptype = PLUGIN_NONE
        self.plugin_id = plugin_id

        self.parameter_count = 0
        self.parameter_list = []
        self.parameter_list_to_update = []

        self.state_filename = None
        self.cur_program_index = -1
        self.cur_midi_program_index = -1

        self.tab_icon_off = QIcon(":/bitmaps/led_off.png")
        self.tab_icon_on  = QIcon(":/bitmaps/led_yellow.png")
        self.tab_icon_count = 0
        self.tab_icon_timers = []

        self.connect(self.b_save_state, SIGNAL("clicked()"), self.save_state)
        self.connect(self.b_load_state, SIGNAL("clicked()"), self.load_state)

        self.connect(self.keyboard, SIGNAL("noteOn(int)"), self.handleNoteOn)
        self.connect(self.keyboard, SIGNAL("noteOff(int)"), self.handleNoteOff)

        self.connect(self.keyboard, SIGNAL("notesOn()"), self.handleNotesOn)
        self.connect(self.keyboard, SIGNAL("notesOff()"), self.handleNotesOff)

        self.connect(self.cb_programs, SIGNAL("currentIndexChanged(int)"),  self.handleProgramIndexChanged)
        self.connect(self.cb_midi_programs, SIGNAL("currentIndexChanged(int)"),  self.handleMidiProgramIndexChanged)

        self.keyboard.setMode(self.keyboard.HORIZONTAL)
        self.keyboard.setOctaves(6)
        self.scrollArea.ensureVisible(self.keyboard.width()*1/5, 0)
        self.scrollArea.setVisible(False)

        # TODO - not implemented yet
        self.b_reload_program.setEnabled(False)
        self.b_reload_midi_program.setEnabled(False)

        self.do_reload_all()

        self.TIMER_GUI_STUFF = self.startTimer(100)

    def do_update(self):
        self.checkInputControlParameters()
        self.checkOutputControlParameters()

        # Update current program text
        if (self.cb_programs.count() > 0):
          pindex = self.cb_programs.currentIndex()
          pname  = NativeHost.get_program_name(self.plugin_id, pindex)
          if (not pname): pname = ""
          self.cb_programs.setItemText(pindex, pname)

        # Update current midi program text
        if (self.cb_midi_programs.count() > 0):
          mpindex = self.cb_midi_programs.currentIndex()
          mpname  = NativeHost.get_midi_program_name(self.plugin_id, pindex)
          if (not mpname): mpname = ""
          self.cb_midi_programs.setItemText(pindex, mpname)
          # FIXME - leave 001:001 alone

    def do_reload_all(self):
        self.pinfo = NativeHost.get_plugin_info(self.plugin_id)
        if (not self.pinfo['valid']):
          self.pinfo["type"]      = PLUGIN_NONE
          self.pinfo["category"]  = PLUGIN_CATEGORY_NONE
          self.pinfo["hints"]     = 0
          self.pinfo["binary"]    = ""
          self.pinfo["name"]      = "(Unknown)"
          self.pinfo["label"]     = ""
          self.pinfo["maker"]     = ""
          self.pinfo["copyright"] = ""
          self.pinfo["unique_id"] = 0

        # Save from null values
        if not self.pinfo['name']:  self.pinfo['name'] = ""
        if not self.pinfo['label']: self.pinfo['label'] = ""
        if not self.pinfo['maker']: self.pinfo['maker'] = ""
        if not self.pinfo['copyright']: self.pinfo['copyright'] = ""

        self.do_reload_info()
        self.do_reload_parameters()
        self.do_reload_programs()

    def do_reload_info(self):
        if (self.ptype == PLUGIN_NONE and self.pinfo['type'] in (PLUGIN_DSSI, PLUGIN_SF2)):
          self.tab_programs.setCurrentIndex(1)

        self.ptype = self.pinfo['type']

        real_plugin_name = NativeHost.get_real_plugin_name(self.plugin_id)
        if not real_plugin_name: real_plugin_name = ""

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
        elif (self.ptype == PLUGIN_WINVST):
          self.le_type.setText("Windows VST")
        elif (self.ptype == PLUGIN_SF2):
          self.le_type.setText("SoundFont")
        else:
          self.le_type.setText(self.tr("Unknown"))

        audio_count = NativeHost.get_audio_port_count_info(self.plugin_id)
        if (not audio_count['valid']):
          audio_count['ins']   = 0
          audio_count['outs']  = 0
          audio_count['total'] = 0

        midi_count = NativeHost.get_midi_port_count_info(self.plugin_id)
        if (not midi_count['valid']):
          midi_count['ins']   = 0
          midi_count['outs']  = 0
          midi_count['total'] = 0

        param_count = NativeHost.get_parameter_count_info(self.plugin_id)
        if (not param_count['valid']):
          param_count['ins']   = 0
          param_count['outs']  = 0
          param_count['total'] = 0

        self.le_ains.setText(str(audio_count['ins']))
        self.le_aouts.setText(str(audio_count['outs']))
        self.le_params.setText(str(param_count['ins']))
        self.le_couts.setText(str(param_count['outs']))
        self.le_is_synth.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_IS_SYNTH) else (self.tr("No")))
        self.le_has_gui.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_HAS_GUI) else (self.tr("No")))

        self.scrollArea.setVisible(self.pinfo['hints'] & PLUGIN_IS_SYNTH or (midi_count['ins'] > 0 and midi_count['outs'] > 0))
        self.parent().recheck_hints(self.pinfo['hints'])

    def do_reload_parameters(self):
        parameters_count = NativeHost.get_parameter_count(self.plugin_id)

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
            param_info   = NativeHost.get_parameter_info(self.plugin_id, i)
            param_data   = NativeHost.get_parameter_data(self.plugin_id, i)
            param_ranges = NativeHost.get_parameter_ranges(self.plugin_id, i)

            if not param_info['valid']:
              continue

            # Save from null values
            if not param_info['name']:   param_info['name']   = ""
            if not param_info['symbol']: param_info['symbol'] = ""
            if not param_info['label']:  param_info['label']  = ""

            parameter = {
              'type':  param_data['type'],
              'hints': param_data['hints'],
              'name':  param_info['name'],
              'label': param_info['label'],
              'scalepoints': [],

              'index':   param_data['index'],
              'default': param_ranges['def'],
              'minimum': param_ranges['min'],
              'maximum': param_ranges['max'],
              'step':       param_ranges['step'],
              'step_small': param_ranges['step_small'],
              'step_large': param_ranges['step_large'],
              'midi_channel': param_data['midi_channel'],
              'midi_cc': param_data['midi_cc'],

              'current': NativeHost.get_current_parameter_value(self.plugin_id, i)
            }

            for j in range(param_info['scalepoint_count']):
              scalepoint = NativeHost.get_scalepoint_info(self.plugin_id, i, j)
              parameter['scalepoints'].append(scalepoint)

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
            if (len(p_in_tmp) > 0 and len(p_in_tmp) < 10):
              p_in.append((p_in_tmp, p_in_width))

            if (len(p_out_tmp) > 0 and len(p_out_tmp) < 10):
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
            'type':  PARAMETER_UNKNOWN,
            'hints': 0,
            'name':  fake_name,
            'label': "",
            'scalepoints': [],

            'index':   0,
            'default': 0,
            'minimum': 0,
            'maximum': 0,
            'step':       0,
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
        old_current   = self.cur_program_index
        program_count = NativeHost.get_program_count(self.plugin_id)

        if (self.cb_programs.count() > 0):
          self.cur_program_index = -1
          self.set_program(-1)
          self.cb_programs.clear()

        if (program_count > 0):
          self.cb_programs.setEnabled(True)
          self.cur_program_index = 0

          for i in range(program_count):
            pname = NativeHost.get_program_name(self.plugin_id, i)
            if (not pname): pname = ""
            self.cb_programs.addItem(pname)

          if (old_current < 0):
            old_current = 0

          self.cur_program_index = old_current
          self.set_program(old_current)

        else:
          self.cb_programs.setEnabled(False)

        # MIDI Programs
        old_midi_current   = self.cur_midi_program_index
        midi_program_count = NativeHost.get_midi_program_count(self.plugin_id)

        if (self.cb_midi_programs.count() > 0):
          self.cur_midi_program_index = -1
          self.set_midi_program(-1)
          self.cb_midi_programs.clear()

        if (midi_program_count > 0):
          self.cb_midi_programs.setEnabled(True)
          self.cur_midi_program_index = 0

          for i in range(midi_program_count):
            midip = NativeHost.get_midi_program_info(self.plugin_id, i)
            if (not midip['label']): midip['label'] = ""

            bank = midip['bank']
            prog = midip['program']

            if (bank < 10):
              bank_str = "00%i" % (bank)
            elif (bank < 100):
              bank_str = "0%i" % (bank)
            else:
              bank_str = "%i" % (bank)

            if (prog < 10):
              prog_str = "00%i" % (prog)
            elif (prog < 100):
              prog_str = "0%i" % (prog)
            else:
              prog_str = "%i" % (prog)

            self.cb_midi_programs.addItem("%s:%s - %s" % (bank_str, prog_str, midip['label']))

          if (old_midi_current < 0):
            old_midi_current = 0

          self.cur_midi_program_index = old_midi_current
          self.set_midi_program(old_midi_current)

        else:
          self.cb_midi_programs.setEnabled(False)

    def set_parameter_value(self, parameter_id, value):
        if (parameter_id not in self.parameter_list_to_update):
          self.parameter_list_to_update.append(parameter_id)

    def set_parameter_midi_channel(self, parameter_id, channel):
        for parameter in self.parameter_list:
          ptype, pid, pwidget = parameter
          if (parameter_id == pid):
            pwidget.set_parameter_midi_channel(channel)
            break

    def set_parameter_midi_cc(self, parameter_id, midi_cc):
        for parameter in self.parameter_list:
          ptype, pid, pwidget = parameter
          if (parameter_id == pid):
            pwidget.set_parameter_midi_cc(midi_cc)
            break

    def set_program(self, program_id):
        self.cur_program_index = program_id
        self.cb_programs.setCurrentIndex(program_id)
        QTimer.singleShot(0, self.checkInputControlParameters)

    def set_midi_program(self, midi_program_id):
        self.cur_midi_program_index = midi_program_id
        self.cb_midi_programs.setCurrentIndex(midi_program_id)
        QTimer.singleShot(0, self.checkInputControlParameters)

    def handleParameterValueChanged(self, parameter_id, value):
        NativeHost.set_parameter_value(self.plugin_id, parameter_id, value)

    def handleParameterMidiChannelChanged(self, parameter_id, channel):
        NativeHost.set_parameter_midi_channel(self.plugin_id, parameter_id, channel-1)

    def handleParameterMidiCcChanged(self, parameter_id, cc_index):
        NativeHost.set_parameter_midi_cc(self.plugin_id, parameter_id, cc_index)

    def handleProgramIndexChanged(self, index):
        if (self.cur_program_index != index):
          NativeHost.set_program(self.plugin_id, index)
          QTimer.singleShot(0, self.checkInputControlParameters)
        self.cur_program_index = index

    def handleMidiProgramIndexChanged(self, index):
        if (self.cur_midi_program_index != index):
          NativeHost.set_midi_program(self.plugin_id, index)
          QTimer.singleShot(0, self.checkInputControlParameters)
        self.cur_midi_program_index = index

    def handleNoteOn(self, note):
        NativeHost.send_midi_note(self.plugin_id, True, note, 100)

    def handleNoteOff(self, note):
        NativeHost.send_midi_note(self.plugin_id, False, note, 100)

    def handleNotesOn(self):
        self.parent().led_midi.setChecked(True)

    def handleNotesOff(self):
        self.parent().led_midi.setChecked(False)

    def save_state(self):
        # TODO - LV2 and VST native formats
        if (self.state_filename == None):
          file_filter = self.tr("Carla State File (*.carxs)")
          filename_try = QFileDialog.getSaveFileName(self, self.tr("Save Carla State File"), filter=file_filter)

          if (not filename_try.isEmpty()):
            self.state_filename = QStringStr(filename_try)
            self.save_state_InternalFormat()
          else:
            self.state_filename = None

        else:
          ask_try = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

          if (ask_try == QMessageBox.Ok):
            self.save_state_InternalFormat()
          else:
            self.state_filename = None
            self.saveState()

    def load_state(self):
        # TODO - LV2 and VST native formats
        file_filter = self.tr("Carla State File (*.carxs)")
        filename_try = QFileDialog.getOpenFileName(self, self.tr("Open Carla State File"), filter=file_filter)

        if (not filename_try.isEmpty()):
          self.state_filename = QStringStr(filename_try)
          self.load_state_InternalFormat()

    def save_state_InternalFormat(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PRESET>\n"
                   "<CARLA-PRESET VERSION='%s'>\n") % (VERSION)

        content += self.parent().getSaveXMLContent()

        content += "</CARLA-PRESET>\n"

        if (open(self.state_filename, "w").write(content)):
          QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save state file"))

    def load_state_InternalFormat(self):
        state_read = open(self.state_filename, "r").read()

        if not state_read:
          QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load state file"))
          return

        xml = QDomDocument()
        xml.setContent(state_read)

        xml_node = xml.documentElement()
        if (xml_node.tagName() != "CARLA-PRESET"):
          QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla state file"))
          return

        x_save_state_dict = getStateSaveDict(xml_node)

        # TODO - verify plugin

        self.parent().load_save_state_dict(x_save_state_dict)

    def createParameterWidgets(self, p_list_full, tab_name, ptype):
        for i in range(len(p_list_full)):
          p_list = p_list_full[i][0]
          width  = p_list_full[i][1]

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
                self.connect(pwidget, SIGNAL("valueChanged(int, float)"),  self.handleParameterValueChanged)

              self.connect(pwidget, SIGNAL("midiChannelChanged(int, int)"),  self.handleParameterMidiChannelChanged)
              self.connect(pwidget, SIGNAL("midiCcChanged(int, int)"),  self.handleParameterMidiCcChanged)

            layout.addStretch()

            self.tabWidget.addTab(container, "%s (%i)" % (tab_name, i+1))

            if (ptype == PARAMETER_INPUT):
              self.tabWidget.setTabIcon(pwidget.tab_index, self.tab_icon_off)
              self.tab_icon_timers.append(ICON_STATE_OFF)
            else:
              self.tab_icon_timers.append(None)

    def animateTab(self, index):
        if (self.tab_icon_timers[index-1] == None):
          self.tabWidget.setTabIcon(index, self.tab_icon_on)
        self.tab_icon_timers[index-1] = ICON_STATE_ON

    def checkInputControlParameters(self):
        for parameter in self.parameter_list:
          parameter_type = parameter[0]
          if (parameter_type == PARAMETER_INPUT):
            parameter_id     = parameter[1]
            parameter_widget = parameter[2]
            parameter_widget.set_default_value(NativeHost.get_default_parameter_value(self.plugin_id, parameter_id))
            parameter_widget.set_parameter_value(NativeHost.get_current_parameter_value(self.plugin_id, parameter_id), False)

    def checkOutputControlParameters(self):
        for parameter in self.parameter_list:
          parameter_type = parameter[0]
          if (parameter_type == PARAMETER_OUTPUT):
            parameter_id     = parameter[1]
            parameter_widget = parameter[2]
            parameter_widget.set_parameter_value(NativeHost.get_current_parameter_value(self.plugin_id, parameter_id), False)

    def checkTabIcons(self):
        for i in range(len(self.tab_icon_timers)):
          if (self.tab_icon_timers[i] == ICON_STATE_ON):
            self.tab_icon_timers[i] = ICON_STATE_WAIT
          elif (self.tab_icon_timers[i] == ICON_STATE_WAIT):
            self.tab_icon_timers[i] = ICON_STATE_OFF
          elif (self.tab_icon_timers[i] == ICON_STATE_OFF):
            self.tabWidget.setTabIcon(i+1, self.tab_icon_off)
            self.tab_icon_timers[i] = None

    def checkUpdatedParameters(self):
        for parameter_id in self.parameter_list_to_update:
          value = NativeHost.get_current_parameter_value(self.plugin_id, parameter_id)

          for parameter in self.parameter_list:
            x_parameter_type   = parameter[0]
            x_parameter_id     = parameter[1]
            x_parameter_widget = parameter[2]

            if (x_parameter_id == parameter_id):
              x_parameter_widget.set_parameter_value(value, False)

              if (x_parameter_type == PARAMETER_INPUT):
                self.animateTab(x_parameter_widget.tab_index)

              break

        for i in self.parameter_list_to_update:
          self.parameter_list_to_update.pop(0)

    def timerEvent(self, event):
        if (event.timerId() == self.TIMER_GUI_STUFF):
          self.checkOutputControlParameters()
          self.checkTabIcons()
          self.checkUpdatedParameters()
        return QDialog.timerEvent(self, event)

# (New) Plugin Widget
class PluginWidget(QFrame, ui_carla_plugin.Ui_PluginWidget):
    def __init__(self, parent, plugin_id):
        super(PluginWidget, self).__init__(parent)
        self.setupUi(self)

        self.plugin_id = plugin_id

        self.params_total = 0
        self.parameter_activity_timer = None

        # Fake effect
        self.color_1 = QColor( 0,  0,  0, 220)
        self.color_2 = QColor( 0,  0,  0, 170)
        self.color_3 = QColor( 7,  7,  7, 250)
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

        audio_count = NativeHost.get_audio_port_count_info(self.plugin_id)
        if (not audio_count['valid']):
          audio_count['ins']   = 0
          audio_count['outs']  = 0
          audio_count['total'] = 0

        self.peaks_in  = audio_count['ins']
        self.peaks_out = audio_count['outs']

        if (self.peaks_in > 2):
          self.peaks_in = 2

        if (self.peaks_out > 2):
          self.peaks_out = 2

        self.peak_in.setChannels(self.peaks_in)
        self.peak_out.setChannels(self.peaks_out)

        self.pinfo = NativeHost.get_plugin_info(self.plugin_id)
        if (not self.pinfo['valid']):
          self.pinfo["type"]      = PLUGIN_NONE
          self.pinfo["category"]  = PLUGIN_CATEGORY_NONE
          self.pinfo["hints"]     = 0
          self.pinfo["binary"]    = ""
          self.pinfo["name"]      = "(Unknown)"
          self.pinfo["label"]     = ""
          self.pinfo["maker"]     = ""
          self.pinfo["copyright"] = ""
          self.pinfo["unique_id"] = 0

        # Save from null values
        if not self.pinfo['name']:  self.pinfo['name'] = ""
        if not self.pinfo['label']: self.pinfo['label'] = ""
        if not self.pinfo['maker']: self.pinfo['maker'] = ""
        if not self.pinfo['copyright']: self.pinfo['copyright'] = ""

        # Set widget page
        if (self.pinfo['type'] == PLUGIN_NONE or audio_count['total'] == 0):
          self.stackedWidget.setCurrentIndex(1)

        self.label_name.setText(self.pinfo['name'])

        self.dial_drywet.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_DRYWET)
        self.dial_vol.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_VOL)
        self.dial_b_left.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.dial_b_right.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.b_gui.setEnabled(self.pinfo['hints'] & PLUGIN_HAS_GUI)

        # Colorify
        if (self.pinfo['category'] == PLUGIN_CATEGORY_SYNTH):
          self.set_plugin_widget_color(PALETTE_COLOR_WHITE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DELAY):
          self.set_plugin_widget_color(PALETTE_COLOR_ORANGE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_EQ):
          self.set_plugin_widget_color(PALETTE_COLOR_GREEN)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_FILTER):
          self.set_plugin_widget_color(PALETTE_COLOR_BLUE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DYNAMICS):
          self.set_plugin_widget_color(PALETTE_COLOR_PINK)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_MODULATOR):
          self.set_plugin_widget_color(PALETTE_COLOR_RED)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_UTILITY):
          self.set_plugin_widget_color(PALETTE_COLOR_YELLOW)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_OUTRO):
          self.set_plugin_widget_color(PALETTE_COLOR_BROWN)
        else:
          self.set_plugin_widget_color(PALETTE_COLOR_NONE)

        if (self.pinfo['hints'] & PLUGIN_IS_SYNTH):
          self.led_audio_in.setVisible(False)
        else:
          self.led_midi.setVisible(False)

        self.edit_dialog = PluginEdit(self, self.plugin_id)
        self.edit_dialog.hide()
        self.edit_dialog_geometry = QVariant(self.edit_dialog.saveGeometry())

        if (self.pinfo['hints'] & PLUGIN_HAS_GUI):
          gui_data = NativeHost.get_gui_data(self.plugin_id)
          self.gui_dialog_type = gui_data['type']

          if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_X11)):
            self.gui_dialog = PluginGUI(self, self.pinfo['name'], gui_data)
            self.gui_dialog.hide()
            self.gui_dialog_geometry = QVariant(self.gui_dialog.saveGeometry())
            self.connect(self.gui_dialog, SIGNAL("finished(int)"), self.gui_dialog_closed)

            NativeHost.set_gui_data(self.plugin_id, Display, unwrapinstance(self.gui_dialog))

          elif (self.gui_dialog_type in (GUI_EXTERNAL_OSC, GUI_EXTERNAL_LV2)):
            self.gui_dialog = None

          else:
            self.gui_dialog = None
            self.b_gui.setEnabled(False)

        else:
          self.gui_dialog = None
          self.gui_dialog_type = GUI_NONE

        self.connect(self.led_enable, SIGNAL("clicked(bool)"), self.set_active)
        self.connect(self.dial_drywet, SIGNAL("sliderMoved(int)"), self.set_drywet)
        self.connect(self.dial_vol, SIGNAL("sliderMoved(int)"), self.set_vol)
        self.connect(self.dial_b_left, SIGNAL("sliderMoved(int)"), self.set_balance_left)
        self.connect(self.dial_b_right, SIGNAL("sliderMoved(int)"), self.set_balance_right)
        self.connect(self.b_gui, SIGNAL("clicked(bool)"), self.handleShowGUI)
        self.connect(self.b_edit, SIGNAL("clicked(bool)"), self.handleEdit)
        self.connect(self.b_remove, SIGNAL("clicked()"), self.handleRemove)

        self.connect(self.dial_drywet, SIGNAL("customContextMenuRequested(QPoint)"), self.showCustomDialMenu)
        self.connect(self.dial_vol, SIGNAL("customContextMenuRequested(QPoint)"), self.showCustomDialMenu)
        self.connect(self.dial_b_left, SIGNAL("customContextMenuRequested(QPoint)"), self.showCustomDialMenu)
        self.connect(self.dial_b_right, SIGNAL("customContextMenuRequested(QPoint)"), self.showCustomDialMenu)

        self.connect(self.edit_dialog, SIGNAL("finished(int)"), self.edit_dialog_closed)

        #self.check_gui_stuff()
        self.TIMER_GUI_STUFF = self.startTimer(50)

    def recheck_hints(self, hints):
        self.pinfo['hints'] = hints
        self.dial_drywet.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_DRYWET)
        self.dial_vol.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_VOL)
        self.dial_b_left.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.dial_b_right.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.b_gui.setEnabled(self.pinfo['hints'] & PLUGIN_HAS_GUI)

    def set_active(self, active, gui_send=False, callback_send=True):
        if (gui_send): self.led_enable.setChecked(active)
        if (callback_send): NativeHost.set_active(self.plugin_id, active)

    def set_drywet(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_drywet.setValue(value)
        if (callback_send): NativeHost.set_drywet(self.plugin_id, float(value)/1000)

        message = self.tr("Output dry/wet (%1%)").arg(value/10)
        self.dial_drywet.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_vol(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_vol.setValue(value)
        if (callback_send): NativeHost.set_vol(self.plugin_id, float(value)/1000)

        message = self.tr("Output volume (%1%)").arg(value/10)
        self.dial_vol.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_balance_left(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_b_left.setValue(value)
        if (callback_send): NativeHost.set_balance_left(self.plugin_id, float(value)/1000)

        if (value == 0):
          message = self.tr("Left Panning (Center)")
        elif (value < 0):
          message = self.tr("Left Panning (%1% Left)").arg(-value/10)
        else:
          message = self.tr("Left Panning (%1% Right)").arg(value/10)

        self.dial_b_left.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def set_balance_right(self, value, gui_send=False, callback_send=True):
        if (gui_send): self.dial_b_right.setValue(value)
        if (callback_send): NativeHost.set_balance_right(self.plugin_id, float(value)/1000)

        if (value == 0):
          message = self.tr("Right Panning (Center)")
        elif (value < 0):
          message = self.tr("Right Panning (%1%) Left").arg(-value/10)
        else:
          message = self.tr("Right Panning (%1% Right)").arg(value/10)

        self.dial_b_right.setStatusTip(message)
        gui.statusBar().showMessage(message)

    def gui_dialog_closed(self):
        self.b_gui.setChecked(False)

    def edit_dialog_closed(self):
        self.b_edit.setChecked(False)

    def check_gui_stuff(self):
        # Input peaks
        if (self.peaks_in > 0):
          if (self.peaks_in > 1):
            peak1 = NativeHost.get_input_peak_value(self.plugin_id, 1)
            peak2 = NativeHost.get_input_peak_value(self.plugin_id, 2)
            self.peak_in.displayMeter(1, peak1)
            self.peak_in.displayMeter(2, peak2)
            self.led_audio_in.setChecked((peak1 != 0.0 or peak2 != 0.0))

          else:
            peak = NativeHost.get_input_peak_value(self.plugin_id, 1)
            self.peak_in.displayMeter(1, peak)
            self.led_audio_in.setChecked((peak != 0.0))

        # Output peaks
        if (self.peaks_out > 0):
          if (self.peaks_out > 1):
            peak1 = NativeHost.get_output_peak_value(self.plugin_id, 1)
            peak2 = NativeHost.get_output_peak_value(self.plugin_id, 2)
            self.peak_out.displayMeter(1, peak1)
            self.peak_out.displayMeter(2, peak2)
            self.led_audio_out.setChecked((peak1 != 0.0 or peak2 != 0.0))

          else:
            peak = NativeHost.get_output_peak_value(self.plugin_id, 1)
            self.peak_out.displayMeter(1, peak)
            self.led_audio_out.setChecked((peak != 0.0))

        # Parameter Activity LED
        if (self.parameter_activity_timer == ICON_STATE_ON):
          self.led_control.setChecked(True)
          self.parameter_activity_timer = ICON_STATE_WAIT
        elif (self.parameter_activity_timer == ICON_STATE_WAIT):
          self.parameter_activity_timer = ICON_STATE_OFF
        elif (self.parameter_activity_timer == ICON_STATE_OFF):
          self.led_control.setChecked(False)
          self.parameter_activity_timer = None

    def set_plugin_widget_color(self, color):
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
        w = 15
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
        texture = 4

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
          background-color: rgba(%i, %i, %i);
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
          border: 2px inset;
          border-color: rgba(30, 30, 30, 255);
        }
      """ % (texture, r, g, b, r, g, b))

    def handleShowGUI(self, show):
        if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_X11)):
          if (show):
            self.gui_dialog.restoreGeometry(self.gui_dialog_geometry.toByteArray())
          else:
            self.gui_dialog_geometry = QVariant(self.gui_dialog.saveGeometry())
          self.gui_dialog.setVisible(show)
        NativeHost.show_gui(self.plugin_id, show)

    def handleEdit(self, show):
        if (show):
          self.edit_dialog.restoreGeometry(self.edit_dialog_geometry.toByteArray())
        else:
          self.edit_dialog_geometry = QVariant(self.edit_dialog.saveGeometry())
        self.edit_dialog.setVisible(show)

    def handleRemove(self):
        gui.func_remove_plugin(self.plugin_id, True)

    def showCustomDialMenu(self, pos):
        dial_name = QStringStr(self.sender().objectName())
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

        current = self.sender().value()/10

        menu = QMenu(self)
        act_x_reset = menu.addAction(self.tr("Reset (%1%)").arg(default))
        menu.addSeparator()
        act_x_min = menu.addAction(self.tr("Set to Minimum (%1%)").arg(minimum))
        act_x_cen = menu.addAction(self.tr("Set to Center"))
        act_x_max = menu.addAction(self.tr("Set to Maximum (%1%)").arg(maximum))
        menu.addSeparator()
        act_x_set = menu.addAction(self.tr("Set value..."))

        if (label not in ("Balance-Left", "Balance-Right")):
          menu.removeAction(act_x_cen)

        act_x_sel = menu.exec_(QCursor.pos())

        if (act_x_sel == act_x_set):
          value_try = QInputDialog.getInteger(self, self.tr("Set value"), label, current, minimum, maximum, 1)
          if (value_try[1]):
            value = value_try[0]*10
          else:
            value = None

        elif (act_x_sel == act_x_min):
          value = minimum*10
        elif (act_x_sel == act_x_max):
          value = maximum*10
        elif (act_x_sel == act_x_reset):
          value = default*10
        elif (act_x_sel == act_x_cen):
          value = 0
        else:
          value = None

        if (value != None):
          if (label == "Dry/Wet"):
            self.set_drywet(value, True, True)
          elif (label == "Volume"):
            self.set_vol(value, True, True)
          elif (label == "Balance-Left"):
            self.set_balance_left(value, True, True)
          elif (label == "Balance-Right"):
            self.set_balance_right(value, True, True)

    def getSaveXMLContent(self):
        NativeHost.prepare_for_save(self.plugin_id)

        if (self.pinfo['type'] == PLUGIN_LADSPA):
          type_str = "LADSPA"
        elif (self.pinfo['type'] == PLUGIN_DSSI):
          type_str = "DSSI"
        elif (self.pinfo['type'] == PLUGIN_LV2):
          type_str = "LV2"
        elif (self.pinfo['type'] == PLUGIN_VST):
          type_str = "VST"
        elif (self.pinfo['type'] == PLUGIN_WINVST):
          type_str = "Windows VST"
        elif (self.pinfo['type'] == PLUGIN_SF2):
          type_str = "SoundFont"
        else:
          type_str = "Unknown"

        real_plugin_name = NativeHost.get_real_plugin_name(self.plugin_id)
        if not real_plugin_name: real_plugin_name = ""

        x_save_state_dict = deepcopy(save_state_dict)

        # ----------------------------
        # Basic info

        x_save_state_dict['Type']   = type_str
        x_save_state_dict['Name']   = real_plugin_name
        x_save_state_dict['Label']  = self.pinfo['label']
        x_save_state_dict['Binary'] = self.pinfo['binary']
        x_save_state_dict['UniqueID'] = self.pinfo['unique_id']

        # ----------------------------
        # Internals

        x_save_state_dict['Active'] = self.led_enable.isChecked()
        x_save_state_dict['DryWet'] = float(self.dial_drywet.value())/1000
        x_save_state_dict['Vol']    = float(self.dial_vol.value())/1000
        x_save_state_dict['Balance-Left']  = float(self.dial_b_left.value())/1000
        x_save_state_dict['Balance-Right'] = float(self.dial_b_right.value())/1000

        # ----------------------------
        # Programs

        if (self.edit_dialog.cb_programs.currentIndex() >= 0):
          x_save_state_dict['ProgramIndex'] = self.edit_dialog.cb_programs.currentIndex()
          x_save_state_dict['ProgramName']  = QStringStr(self.edit_dialog.cb_programs.currentText())

        # ----------------------------
        # MIDI Programs

        if (self.edit_dialog.cb_midi_programs.currentIndex() >= 0):
          midi_program_info = NativeHost.get_midi_program_info(self.plugin_id, self.edit_dialog.cb_midi_programs.currentIndex())
          x_save_state_dict['MidiBank']    = midi_program_info['bank']
          x_save_state_dict['MidiProgram'] = midi_program_info['program']

        # ----------------------------
        # Parameters

        parameter_count = NativeHost.get_parameter_count(self.plugin_id)

        for i in range(parameter_count):
          parameter_info = NativeHost.get_parameter_info(self.plugin_id, i)
          parameter_data = NativeHost.get_parameter_data(self.plugin_id, i)

          if (not parameter_info['valid'] or parameter_data['type'] != PARAMETER_INPUT):
            continue

          # Save from null values
          if not parameter_info['name']:   parameter_info['name']   = ""
          if not parameter_info['symbol']: parameter_info['symbol'] = ""
          if not parameter_info['label']:  parameter_info['label']  = ""

          x_save_state_parameter = deepcopy(save_state_parameter)

          x_save_state_parameter['index']  = parameter_data['index']
          x_save_state_parameter['rindex'] = parameter_data['rindex']
          x_save_state_parameter['name']   = parameter_info['name']
          x_save_state_parameter['symbol'] = parameter_info['symbol']
          x_save_state_parameter['value']  = NativeHost.get_current_parameter_value(self.plugin_id, parameter_data['index'])
          x_save_state_parameter['midi_channel'] = parameter_data['midi_channel']+1
          x_save_state_parameter['midi_cc'] = parameter_data['midi_cc']

          if (parameter_data['hints'] & PARAMETER_USES_SAMPLERATE):
            x_save_state_parameter['value'] /= NativeHost.get_sample_rate()

          x_save_state_dict['Parameters'].append(x_save_state_parameter)

        # ----------------------------
        # Custom Data

        custom_data_count = NativeHost.get_custom_data_count(self.plugin_id)

        for i in range(custom_data_count):
          custom_data = NativeHost.get_custom_data(self.plugin_id, i)

          if (custom_data['type'] == CUSTOM_DATA_INVALID):
            continue

          # Save from null values
          if not custom_data['key']:   custom_data['key'] = ""
          if not custom_data['value']: custom_data['value'] = ""

          x_save_state_custom_data = deepcopy(save_state_custom_data)

          x_save_state_custom_data['type']  = custom_data['type']
          x_save_state_custom_data['key']   = custom_data['key']
          x_save_state_custom_data['value'] = custom_data['value']

          x_save_state_dict['CustomData'].append(x_save_state_custom_data)

        # ----------------------------
        # Chunk

        if (self.pinfo['hints'] & PLUGIN_USES_CHUNKS):
          chunk_data = NativeHost.get_chunk_data(self.plugin_id)
          if chunk_data:
            x_save_state_dict['Chunk'] = chunk_data

        # ----------------------------
        # Generate XML for this plugin

        content  = ""

        content += "  <Info>\n"
        content += "   <Type>%s</Type>\n" % (x_save_state_dict['Type'])
        content += "   <Name>%s</Name>\n" % (x_save_state_dict['Name'])
        content += "   <Label>%s</Label>\n" % (x_save_state_dict['Label'])
        content += "   <Binary>%s</Binary>\n" % (x_save_state_dict['Binary'])
        content += "   <UniqueID>%li</UniqueID>\n" % (x_save_state_dict['UniqueID'])
        content += "  </Info>\n"

        content += "\n"
        content += "  <Data>\n"
        content += "   <Active>%s</Active>\n" % ("Yes" if x_save_state_dict['Active'] else "No")
        content += "   <DryWet>%f</DryWet>\n" % (x_save_state_dict['DryWet'])
        content += "   <Vol>%f</Vol>\n" % (x_save_state_dict['Vol'])
        content += "   <Balance-Left>%f</Balance-Left>\n" % (x_save_state_dict['Balance-Left'])
        content += "   <Balance-Right>%f</Balance-Right>\n" % (x_save_state_dict['Balance-Right'])

        for parameter in x_save_state_dict['Parameters']:
          content += "\n"
          content += "   <Parameter>\n"
          content += "    <index>%i</index>\n" % (parameter['index'])
          content += "    <rindex>%i</rindex>\n" % (parameter['rindex'])
          content += "    <name>%s</name>\n" % (parameter['name'])
          content += "    <symbol>%s</symbol>\n" % (parameter['symbol'])
          content += "    <value>%f</value>\n" % (parameter['value'])
          content += "    <midi_channel>%i</midi_channel>\n" % (parameter['midi_channel'])
          content += "    <midi_cc>%i</midi_cc>\n" % (parameter['midi_cc'])
          content += "   </Parameter>\n"

        if (x_save_state_dict['ProgramIndex'] >= 0):
          content += "\n"
          content += "   <ProgramIndex>%i</ProgramIndex>\n" % (x_save_state_dict['ProgramIndex'])
          content += "   <ProgramName>%s</ProgramName>\n" % (x_save_state_dict['ProgramName'])

        if (x_save_state_dict['MidiBank'] >= 0 and x_save_state_dict['MidiProgram'] >= 0):
          content += "\n"
          content += "   <MidiBank>%i</MidiBank>\n" % (x_save_state_dict['MidiBank'])
          content += "   <MidiProgram>%i</MidiProgram>\n" % (x_save_state_dict['MidiProgram'])

        for custom_data in x_save_state_dict['CustomData']:
          if (not custom_data['value'].endswith("\n")):
            custom_data['value'] += "\n"
          content += "\n"
          content += "   <CustomData>\n"
          content += "    <type>%i</type>\n" % (custom_data['type'])
          content += "    <key>%s</key>\n" % (custom_data['key'])
          content += "    <value>\n"
          content += "%s" % (Qt.escape(custom_data['value']))
          content += "    </value>\n"
          content += "   </CustomData>\n"

        if (x_save_state_dict['Chunk']):
          if (not x_save_state_dict['Chunk'].endswith("\n")):
            x_save_state_dict['Chunk'] += "\n"
          content += "\n"
          content += "   <Chunk>\n"
          content += "%s" % (x_save_state_dict['Chunk'])
          content += "   </Chunk>\n"

        content += "  </Data>\n"

        return content

    def load_save_state_dict(self, content):

        # Part 1 - set custom data
        for custom_data in content['CustomData']:
          NativeHost.set_custom_data(self.plugin_id, custom_data['type'], custom_data['key'], custom_data['value'])

        # Part 2 - set program (carefully)
        program_id = -1
        program_count = NativeHost.get_program_count(self.plugin_id)

        if (content['ProgramName']):
          test_pname = NativeHost.get_program_name(self.plugin_id, content['ProgramIndex'])

          if (content['ProgramName'] == test_pname):
            program_id = content['ProgramIndex']
          else:
            for i in range(program_count):
              new_test_pname = NativeHost.get_program_name(self.plugin_id, i)
              if (content['ProgramName'] == new_test_pname):
                program_id = i
                break
            else:
              if (content['ProgramIndex'] < program_count):
                program_id = content['ProgramIndex']
        else:
          if (content['ProgramIndex'] < program_count):
            program_id = content['ProgramIndex']

        if (program_id >= 0):
          NativeHost.set_program(self.plugin_id, program_id)
          self.edit_dialog.set_program(program_id)

        # Part 3 - set midi program
        if (content['MidiBank'] >= 0 and content['MidiProgram'] >= 0):
          midi_program_count = NativeHost.get_midi_program_count(self.plugin_id)

          for i in range(midi_program_count):
            program_info = NativeHost.get_midi_program_info(self.plugin_id, i)
            if (program_info['bank'] == content['MidiBank'] and program_info['program'] == content['MidiProgram']):
              NativeHost.set_midi_program(self.plugin_id, i)
              self.edit_dialog.set_midi_program(i)
              break

        # Part 4a - store symbol values, for ladspa and lv2
        param_symbols = [] # (index, symbol)

        for parameter in content['Parameters']:
          if (parameter['symbol']):
            param_info = NativeHost.get_parameter_info(self.plugin_id, parameter['index'])

            if (param_info['valid'] and param_info['symbol']):
              param_symbols.append((parameter['index'], param_info['symbol']))

        # Part 4b - set parameter values (carefully)
        for parameter in content['Parameters']:
          index = -1

          if (content['Type'] == "LADSPA"):
            # Try to set by symbol, otherwise use index
            if (parameter['symbol'] != None and parameter['symbol'] != ""):
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
            if (parameter['symbol'] != None and parameter['symbol'] != ""):
              for param_symbol in param_symbols:
                if (param_symbol[1] == parameter['symbol']):
                  index = param_symbol[0]
                  break
              else:
                print("Failed to find LV2 parameter symbol for", parameter['index'], "->", parameter['name'])
            else:
              print("LV2 Plugin parameter", parameter['index'], "has no symbol ->", parameter['name'])

          else:
            # Index only
            index = parameter['index']

          if (index >= 0):
            param_data = NativeHost.get_parameter_data(self.plugin_id, parameter['index'])
            if (param_data['hints'] & PARAMETER_USES_SAMPLERATE):
              parameter['value'] *= NativeHost.get_sample_rate()

            NativeHost.set_parameter_value(self.plugin_id, index, parameter['value'])
            NativeHost.set_parameter_midi_channel(self.plugin_id, index, parameter['midi_channel']-1)
            NativeHost.set_parameter_midi_cc(self.plugin_id, index, parameter['midi_cc'])
          else:
            print("Could not set parameter data for", parameter['index'], "->", parameter['name'])

        # Part 5 - set chunk data
        if (content['Chunk']):
          NativeHost.set_chunk_data(self.plugin_id, content['Chunk'])

        # Part 6 - set internal stuff
        self.set_drywet(content['DryWet']*1000, True, True)
        self.set_vol(content['Vol']*1000, True, True)
        self.set_balance_left(content['Balance-Left']*1000, True, True)
        self.set_balance_right(content['Balance-Right']*1000, True, True)
        self.edit_dialog.do_reload_all()

        self.set_active(content['Active'], True, True)

        # Done!
        gui.statusBar().showMessage("State File Loaded Sucessfully!")

    def timerEvent(self, event):
        if (event.timerId() == self.TIMER_GUI_STUFF):
          self.check_gui_stuff()
        return QFrame.timerEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setPen(self.color_1)
        painter.drawLine(0, 3, self.width(), 3)
        painter.drawLine(0, self.height()-4, self.width(), self.height()-4)
        painter.setPen(self.color_2)
        painter.drawLine(0, 2, self.width(), 2)
        painter.drawLine(0, self.height()-3, self.width(), self.height()-3)
        painter.setPen(self.color_3)
        painter.drawLine(0, 1, self.width(), 1)
        painter.drawLine(0, self.height()-2, self.width(), self.height()-2)
        painter.setPen(self.color_4)
        painter.drawLine(0, 0, self.width(), 0)
        painter.drawLine(0, self.height()-1, self.width(), self.height()-1)
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
        self.loadSettings()

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

        self.m_plugin_list = []
        for x in range(MAX_PLUGINS):
          self.m_plugin_list.append(None)

        self.m_project_filename = None

        NativeHost.set_callback_function(self.callback_function)

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

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        #self.connect(self, SIGNAL("SIGUSR1()"), self.handleSIGUSR1)
        #self.connect(self, SIGNAL("DebugCallback(int, int, int, double)"), self.handleDebugCallback)
        #self.connect(self, SIGNAL("ParameterCallback(int, int, double)"), self.handleParameterCallback)
        #self.connect(self, SIGNAL("ProgramCallback(int, int)"), self.handleProgramCallback)
        #self.connect(self, SIGNAL("MidiProgramCallback(int, int)"), self.handleMidiProgramCallback)
        #self.connect(self, SIGNAL("NoteOnCallback(int, int, int)"), self.handleNoteOnCallback)
        #self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), self.handleNoteOffCallback)
        #self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), self.handleShowGuiCallback)
        #self.connect(self, SIGNAL("ResizeGuiCallback(int, int, int)"), self.handleResizeGuiCallback)
        #self.connect(self, SIGNAL("UpdateCallback(int)"), self.handleUpdateCallback)
        #self.connect(self, SIGNAL("ReloadInfoCallback(int)"), self.handleReloadInfoCallback)
        #self.connect(self, SIGNAL("ReloadParametersCallback(int)"), self.handleReloadParametersCallback)
        #self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), self.handleReloadProgramsCallback)
        #self.connect(self, SIGNAL("ReloadAllCallback(int)"), self.handleReloadAllCallback)
        #self.connect(self, SIGNAL("QuitCallback()"), self.handleQuitCallback)

    def callback_function(self, action, plugin_id, value1, value2, value3):
        if (plugin_id < 0 or plugin_id >= MAX_PLUGINS):
          return

        #if (action == CALLBACK_DEBUG):
          #self.emit(SIGNAL("DebugCallback(int, int, int, double)"), plugin_id, value1, value2, value3)
        #elif (action == CALLBACK_PARAMETER_CHANGED):
          #self.emit(SIGNAL("ParameterCallback(int, int, double)"), plugin_id, value1, value3)
        #elif (action == CALLBACK_PROGRAM_CHANGED):
          #self.emit(SIGNAL("ProgramCallback(int, int)"), plugin_id, value1)
        #elif (action == CALLBACK_MIDI_PROGRAM_CHANGED):
          #self.emit(SIGNAL("MidiProgramCallback(int, int)"), plugin_id, value1)
        #elif (action == CALLBACK_NOTE_ON):
          #self.emit(SIGNAL("NoteOnCallback(int, int, int)"), plugin_id, value1, value2)
        #elif (action == CALLBACK_NOTE_OFF):
          #self.emit(SIGNAL("NoteOffCallback(int, int, int)"), plugin_id, value1, value2)
        #elif (action == CALLBACK_SHOW_GUI):
          #self.emit(SIGNAL("ShowGuiCallback(int, int)"), plugin_id, value1)
        #elif (action == CALLBACK_RESIZE_GUI):
          #self.emit(SIGNAL("ResizeGuiCallback(int, int, int)"), plugin_id, value1, value2)
        #elif (action == CALLBACK_UPDATE):
          #self.emit(SIGNAL("UpdateCallback(int)"), plugin_id)
        #elif (action == CALLBACK_RELOAD_INFO):
          #self.emit(SIGNAL("ReloadInfoCallback(int)"), plugin_id)
        #elif (action == CALLBACK_RELOAD_PARAMETERS):
          #self.emit(SIGNAL("ReloadParametersCallback(int)"), plugin_id)
        #elif (action == CALLBACK_RELOAD_PROGRAMS):
          #self.emit(SIGNAL("ReloadProgramsCallback(int)"), plugin_id)
        #elif (action == CALLBACK_RELOAD_ALL):
          #self.emit(SIGNAL("ReloadAllCallback(int)"), plugin_id)
        #elif (action == CALLBACK_QUIT):
          #self.emit(SIGNAL("QuitCallback()"))

    def handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_file_save()"))

    #def handleDebugCallback(self, plugin_id, value1, value2, value3):
        #print "DEBUG ::", plugin_id, value1, value2, value3

    #def handleParameterCallback(self, plugin_id, parameter_id, value):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.parameter_activity_timer = ICON_STATE_ON

          #if (parameter_id == PARAMETER_ACTIVE):
            #pwidget.set_active((value > 0.0), True, False)
          #elif (parameter_id == PARAMETER_DRYWET):
            #pwidget.set_drywet(value*1000, True, False)
          #elif (parameter_id == PARAMETER_VOLUME):
            #pwidget.set_vol(value*1000, True, False)
          #elif (parameter_id == PARAMETER_BALANCE_LEFT):
            #pwidget.set_balance_left(value*1000, True, False)
          #elif (parameter_id == PARAMETER_BALANCE_RIGHT):
            #pwidget.set_balance_right(value*1000, True, False)
          #elif (parameter_id >= 0):
            #pwidget.edit_dialog.set_parameter_value(parameter_id, value)

    #def handleProgramCallback(self, plugin_id, program_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_program(program_id)

    #def handleMidiProgramCallback(self, plugin_id, midi_program_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_midi_program(midi_program_id)

    #def handleNoteOnCallback(self, plugin_id, note, velo):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.keyboard.noteOn(note, False)

    #def handleNoteOffCallback(self, plugin_id, note, velo):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.keyboard.noteOff(note, False)

    #def handleShowGuiCallback(self, plugin_id, show):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #if (show == 0):
            #pwidget.b_gui.setChecked(False)
            #pwidget.b_gui.setEnabled(True)
          #elif (show == 1):
            #pwidget.b_gui.setChecked(True)
            #pwidget.b_gui.setEnabled(True)
          #elif (show == -1):
            #pwidget.b_gui.setChecked(False)
            #pwidget.b_gui.setEnabled(False)

    #def handleResizeGuiCallback(self, plugin_id, width, height):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #gui_dialog = pwidget.gui_dialog
          #if (gui_dialog):
            #gui_dialog.set_new_size(width, height)

    #def handleUpdateCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.do_update()

    #def handleReloadInfoCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.do_reload_info()

    #def handleReloadParametersCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.do_reload_parameters()

    #def handleReloadProgramsCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.do_reload_programs()

    #def handleReloadAllCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.do_reload_all()

    #def handleQuitCallback(self):
        #CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"), self.tr("JACK has been stopped or crashed.\nPlease start JACK and restart Carla"),
                                              #"You may want to save your session now...", QMessageBox.Ok, QMessageBox.Ok)

    def add_plugin(self, btype, ptype, filename, label, extra_stuff, activate):
        new_plugin_id = NativeHost.add_plugin(btype, ptype, filename, label, extra_stuff)

        if (new_plugin_id < 0):
          CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                                              NativeHost.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
        #else:
          #pwidget = PluginWidget(self, new_plugin_id)
          #self.w_plugins.layout().addWidget(pwidget)
          #self.plugin_list[new_plugin_id] = pwidget
          #self.act_plugin_remove_all.setEnabled(True)

          #if (activate):
            #pwidget.set_active(True, True)

        return new_plugin_id

    def remove_plugin(self, plugin_id, showError):
        pwidget = self.m_plugin_list[plugin_id]
        #pwidget.edit_dialog.close()

        #if (pwidget.gui_dialog):
          #pwidget.gui_dialog.close()

        #if (NativeHost.remove_plugin(plugin_id)):
          #pwidget.close()
          #pwidget.deleteLater()
          #self.w_plugins.layout().removeWidget(pwidget)
          #self.plugin_list[plugin_id] = None

        #else:
          #if (showError):
            #CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to remove plugin"),
                                                  #NativeHost.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        for i in range(MAX_PLUGINS):
          if (self.m_plugin_list[i] != None):
            self.act_plugin_remove_all.setEnabled(True)
            break
        else:
          self.act_plugin_remove_all.setEnabled(False)

    def get_extra_stuff(self, plugin):
        ptype = plugin['type']

        if (ptype == PLUGIN_LADSPA):
          unique_id = plugin['unique_id']
          for rdf_item in self.ladspa_rdf_list:
            if (rdf_item.UniqueID == unique_id):
              return pointer(rdf_item)
          else:
            return c_nullptr

        elif (ptype == PLUGIN_DSSI):
          if (plugin['hints'] & PLUGIN_HAS_GUI):
            return findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
          else:
            return c_nullptr

        #elif (ptype == PLUGIN_LV2):
          #p_uri = plugin['label']
          #for rdf_item in self.lv2_rdf_list:
            #if (rdf_item.URI == p_uri):
              #return pointer(rdf_item)
          #else:
            #return c_nullptr

        #elif (ptype == PLUGIN_WINVST):
          ## Store object so we can return a pointer
          #if (self.winvst_info == None):
            #self.winvst_info = WinVstBaseInfo()
          #self.winvst_info.category  = plugin['category']
          #self.winvst_info.hints     = plugin['hints']
          #self.winvst_info.name      = plugin['name']
          #self.winvst_info.maker     = plugin['maker']
          #self.winvst_info.unique_id = long(plugin['id'])
          #self.winvst_info.ains      = plugin['audio.ins']
          #self.winvst_info.aouts     = plugin['audio.outs']
          #return pointer(self.winvst_info)

        else:
          return c_nullptr

    def save_project(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PROJECT>\n"
                   "<CARLA-PROJECT VERSION='%s'>\n") % (VERSION)

        first_plugin = True

        for pwidget in self.m_plugin_list:
          if (pwidget != None):
            if (first_plugin == False):
              content += "\n"

            content += " <Plugin>\n"
            #content += pwidget.getSaveXMLContent()
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

        #failed_plugins = []
        #x_save_state_dicts = []

        #node = xml_node.firstChild()
        #while not node.isNull():
          #if (node.toElement().tagName() == "Plugin"):
            #x_save_state_dict = getStateSaveDict(node)
            #x_save_state_dicts.append(x_save_state_dict)
          #node = node.nextSibling()

        #for x_save_state_dict in x_save_state_dicts:
          #ptype = x_save_state_dict['Type']
          #label = x_save_state_dict['Label']
          #binary = x_save_state_dict['Binary']
          #unique_id = str(x_save_state_dict['UniqueID'])
          #plugin = None

          #if (ptype == "LADSPA"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/LADSPA").toList())
          #elif (ptype == "DSSI"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/DSSI").toList())
          #elif (ptype == "LV2"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/LV2").toList())
          #elif (ptype == "VST"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/VST").toList())
          #elif (ptype == "Windows VST"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/WINVST").toList())
          #elif (ptype == "SoundFont"):
            #x_plugins = QVariantPyObjectList(self.settings_db.value("Plugins/SF2").toList())
          #else:
            #failed_plugins.append(x_save_state_dict['Name'])
            #continue

          ## Try UniqueID -> Label -> Binary
          #plugin_ulb = None
          #plugin_ul = None
          #plugin_ub = None
          #plugin_lb = None
          #plugin_u = None
          #plugin_l = None
          #plugin_b = None

          #for plugins in x_plugins:
            #if (ptype == "SoundFont"):
              #plugins = (plugins,)

            #for plugin_ in plugins:
              #plugin = strPyPluginInfo(plugin_)

              #if (unique_id == plugin['id'] and label == plugin['label'] and binary == plugin['binary']):
                #plugin_ulb = plugin
                #break
              #elif (unique_id == plugin['id'] and label == plugin['label']):
                #plugin_ul = plugin
              #elif (unique_id == plugin['id'] and binary == plugin['binary']):
                #plugin_ub = plugin
              #elif (label == plugin['label'] and binary == plugin['binary']):
                #plugin_lb = plugin
              #elif (unique_id == plugin['id']):
                #plugin_u = plugin
              #elif (label == plugin['label']):
                #plugin_l = plugin
              #elif (binary == plugin['binary']):
                #plugin_b = plugin

          ## LV2 only uses URIs (label in this case)
          #if (ptype != "LV2"):
            #plugin_ub = None
            #plugin_u = None
            #plugin_b = None

          ## SoundFonts only uses Binaries
          #if (ptype != "LV2"):
            #plugin_ul = None
            #plugin_u = None
            #plugin_l = None

          #if (plugin_ulb):
            #plugin = plugin_ulb
          #elif (plugin_ul):
            #plugin = plugin_ul
          #elif (plugin_ub):
            #plugin = plugin_ub
          #elif (plugin_lb):
            #plugin = plugin_lb
          #elif (plugin_u):
            #plugin = plugin_u
          #elif (plugin_l):
            #plugin = plugin_l
          #elif (plugin_b):
            #plugin = plugin_b
          #else:
            #plugin = None

          #if (plugin != None):
            #extra_stuff   = self.get_extra_stuff(plugin)
            #new_plugin_id = self.func_add_plugin(plugin['type'], binary, label, extra_stuff, False)

            #if (new_plugin_id >= 0):
              #pwidget = self.plugin_list[new_plugin_id]
              #pwidget.load_save_state_dict(x_save_state_dict)

            #else:
              #failed_plugins.append(x_save_state_dict['Name'])

          #else:
            #failed_plugins.append(x_save_state_dict['Name'])

        #if (len(failed_plugins) > 0):
          #print "----------- FAILED TO LOAD!! ->", failed_plugins
          ## TODO - display error

    def loadRDFs(self):
        # Save RDF info for later
        if (haveRDF):
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

          #fr_lv2_file = os.path.join(SettingsDir, "lv2_rdf.db")
          #if (os.path.exists(fr_lv2_file)):
            #fr_lv2 = open(fr_lv2_file, 'r')
            #if (fr_lv2):
              #try:
                #self.lv2_rdf_list = lv2_rdf.get_c_lv2_rdfs(json.load(fr_lv2))
              #except:
                #self.lv2_rdf_list = []
              #fr_lv2.close()

        else:
          self.ladspa_rdf_list = []
          #self.lv2_rdf_list = []

    @pyqtSlot()
    def slot_file_new(self):
        self.slot_remove_all()
        self.m_project_filename = None
        self.setWindowTitle("Carla")

    @pyqtSlot()
    def slot_file_open(self):
        file_filter = self.tr("Carla Project File (*.carxp)")
        filename    = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), filter=file_filter)

        if (filename):
          self.m_project_filename = filename
          self.slot_remove_all()
          self.load_project()
          self.setWindowTitle("Carla - %s" % (getShortFileName(self.m_project_filename)))

    @pyqtSlot()
    def slot_file_save(self, saveAs=False):
       if (self.m_project_filename == None or saveAs):
          file_filter = self.tr("Carla Project File (*.carxp)")
          filename    = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), filter=file_filter)

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
          btype    = dialog.ret_plugin['build']
          ptype    = dialog.ret_plugin['type']
          filename = dialog.ret_plugin['binary']
          label    = dialog.ret_plugin['label']
          extra_stuff = self.get_extra_stuff(dialog.ret_plugin)
          self.add_plugin(btype, ptype, filename, label, extra_stuff, True)

    @pyqtSlot()
    def slot_remove_all(self):
        for i in range(MAX_PLUGINS):
          if (self.m_plugin_list[i] != None):
            self.remove_plugin(i, False)

    @pyqtSlot()
    def slot_aboutCarla(self):
        AboutW(self).exec_()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))

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

    #style = app.style().metaObject().className()
    #force_parameters_style = (style in ("Bespin::Style",))

    NativeHost = Host()
    #NativeHost.set_option(OPTION_GLOBAL_JACK_CLIENT, 0, "")
    #NativeHost.set_option(OPTION_BRIDGE_PATH_LV2_GTK2, 0, carla_bridge_lv2_gtk2)
    #NativeHost.set_option(OPTION_BRIDGE_PATH_LV2_QT4, 0, carla_bridge_lv2_qt4)
    #NativeHost.set_option(OPTION_BRIDGE_PATH_LV2_X11, 0, carla_bridge_lv2_x11)
    #NativeHost.set_option(OPTION_BRIDGE_PATH_VST_QT4, 0, carla_bridge_vst_qt4)
    #NativeHost.set_option(OPTION_BRIDGE_PATH_WINVST, 0, carla_bridge_winvst)

    if (not NativeHost.carla_init("Carla")):
      CustomMessageBox(None, QMessageBox.Critical, "Error", "Could not connect to JACK",
                            NativeHost.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
      sys.exit(1)

    ## X11 Display
    #Display = 0
    #if (LINUX):
      #Display_env = os.getenv("DISPLAY")
      #if (Display_env != None):
        #try:
          #Display = int(float(Display_env.replace(":","")))
        #except:
          #Display = 0

    # Show GUI
    gui = CarlaMainW()
    gui.show()

    # Set-up custom signal handling
    set_up_signals(gui)

    for i in range(len(app.arguments())):
      if (i == 0): continue
      try_path = app.arguments()[i]
      if (os.path.exists(try_path)):
        gui.m_project_filename = try_path
        gui.load_project()
        gui.setWindowTitle("Carla - %s" % (getShortFileName(try_path)))

    # App-Loop
    ret = app.exec_()

    # Close Host
    if (NativeHost.carla_is_engine_running()):
      if (not NativeHost.carla_close()):
        print(NativeHost.get_last_error())

    # Exit properly
    sys.exit(ret)

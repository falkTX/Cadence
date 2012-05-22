#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK Settings Dialog
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
from PyQt4.QtCore import pyqtSlot, Qt, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QDialog, QDialogButtonBox, QMessageBox
from sys import platform

# Imports (Custom Stuff)
import ui_settings_jack

# Imports (DBus)
try:
  import dbus
except:
  dbus = None

# -------------------------------------------------------------

# global object
global jackctl
jackctl = None

# enum jack_timer_type_t
JACK_TIMER_SYSTEM_CLOCK  = 0
JACK_TIMER_CYCLE_COUNTER = 1
JACK_TIMER_HPET          = 2

# Set Platform
if ("linux" in platform):
  LINUX   = True
else:
  LINUX   = False

# Init DBus
def initBus(bus):
  global jackctl
  try:
    jackctl = dbus.Interface(bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller"), "org.jackaudio.Configure")
    return 0
  except:
    jackctl = None
    return 1

# -------------------------------------------------------------
# Helper functions

def bool2str(yesno):
  return "True" if yesno else "False"

def getBufferSize():
  return getDriverParameter("period", -1)

def getSampleRate():
  return getDriverParameter("rate", -1)

def isRealtime():
  return getEngineParameter("realtime", False)

def setBufferSize(bsize):
  return setDriverParameter("period", dbus.UInt32(bsize))

def setSampleRate(srate):
  return setDriverParameter("rate", dbus.UInt32(srate))

# -------------------------------------------------------------
# Helper functions (engine)

def engineHasFeature(feature):
  feature_list = jackctl.ReadContainer(["engine"])[1]
  return bool(dbus.String(feature) in feature_list)

def getEngineParameter(parameter, error):
    if (not engineHasFeature(parameter)):
      return error
    else:
      return jackctl.GetParameterValue(["engine",parameter])[2]

def setEngineParameter(parameter, value, optional=True):
    if (not engineHasFeature(parameter)):
      return False
    elif (optional):
      if (value != jackctl.GetParameterValue(["engine",parameter])[2]):
        return bool(jackctl.SetParameterValue(["engine",parameter],value))
      else:
        return False
    else:
      return bool(jackctl.SetParameterValue(["engine",parameter],value))

# -------------------------------------------------------------
# Helper functions (driver)

def driverHasFeature(feature):
    feature_list = jackctl.ReadContainer(["driver"])[1]
    return bool(dbus.String(feature) in feature_list)

def getDriverParameter(parameter, error):
    if (not driverHasFeature(parameter)):
      return error
    else:
      return jackctl.GetParameterValue(["driver",parameter])[2]

def setDriverParameter(parameter, value, optional=True):
    if (not driverHasFeature(parameter)):
      return False
    elif (optional):
      if (value != jackctl.GetParameterValue(["driver",parameter])[2]):
        return bool(jackctl.SetParameterValue(["driver",parameter],value))
      else:
        return False
    else:
      return bool(jackctl.SetParameterValue(["driver",parameter],value))

# -------------------------------------------------------------
# JACK Settings Dialog

class JackSettingsW(QDialog, ui_settings_jack.Ui_JackSettingsW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Check if we've got valid control interface

        if not jackctl:
          QTimer.singleShot(0, self, SLOT("slot_closeWithError()"))
          return

        # -------------------------------------------------------------
        # Align driver text and hide non available ones

        driver_list = jackctl.ReadContainer(["drivers"])[1]
        for i in range(self.obj_server_driver.rowCount()):
          self.obj_server_driver.item(0, i).setTextAlignment(Qt.AlignCenter)
          if (dbus.String(self.obj_server_driver.item(0, i).text().lower()) not in driver_list):
            self.obj_server_driver.hideRow(i)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveJackSettings()"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetJackSettings()"))

        self.connect(self.obj_driver_duplex, SIGNAL("clicked(bool)"), SLOT("slot_checkDuplexSelection(bool)"))
        self.connect(self.obj_server_driver, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkDriverSelection(int)"))

        self.connect(self.obj_driver_capture, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkALSASelection()"))
        self.connect(self.obj_driver_playback, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkALSASelection()"))

        # -------------------------------------------------------------
        # Load initial settings

        self.m_driver = ""
        self.m_server_clock_source_broken = False

        self.checkEngine()
        self.loadServerSettings()
        self.loadDriverSettings(True) # reset because we'll change it below

        # -------------------------------------------------------------
        # Load selected JACK driver

        self.m_driver = str(jackctl.GetParameterValue(["engine","driver"])[2])
        for i in range(self.obj_server_driver.rowCount()):
          if (self.obj_server_driver.item(i, 0).text().lower() == self.m_driver):
            self.obj_server_driver.setCurrentCell(i, 0)
            break

        # Special ALSA check
        self.slot_checkALSASelection()

    # -------------------------------------------------------------
    # Engine calls

    def checkEngine(self):
        self.obj_server_name.setEnabled(engineHasFeature("name"))
        self.obj_server_realtime.setEnabled(engineHasFeature("realtime"))
        self.obj_server_realtime_priority.setEnabled(engineHasFeature("realtime-priority"))
        self.obj_server_temporary.setEnabled(engineHasFeature("temporary"))
        self.obj_server_verbose.setEnabled(engineHasFeature("verbose"))
        self.obj_server_client_timeout.setEnabled(engineHasFeature("client-timeout"))
        self.obj_server_clock_source.setEnabled(engineHasFeature("clock-source"))
        self.obj_server_port_max.setEnabled(engineHasFeature("port-max"))
        self.obj_server_replace_registry.setEnabled(engineHasFeature("replace-registry"))
        self.obj_server_sync.setEnabled(engineHasFeature("sync"))
        self.obj_server_self_connect_mode.setEnabled(engineHasFeature("self-connect-mode"))

        # Disable clock-source if not on Linux
        if (not LINUX):
          self.obj_server_clock_source.setEnabled(False)

    # -------------------------------------------------------------
    # Server calls

    def saveServerSettings(self):
        if (self.obj_server_name.isEnabled()):
          value = dbus.String(self.obj_server_name.text())
          setEngineParameter("name", value, True)

        if (self.obj_server_realtime.isEnabled()):
          value = dbus.Boolean(self.obj_server_realtime.isChecked())
          setEngineParameter("realtime", value, True)

        if (self.obj_server_realtime_priority.isEnabled()):
          value = dbus.Int32(self.obj_server_realtime_priority.value())
          setEngineParameter("realtime-priority", value, True)

        if (self.obj_server_temporary.isEnabled()):
          value = dbus.Boolean(self.obj_server_temporary.isChecked())
          setEngineParameter("temporary", value, True)

        if (self.obj_server_verbose.isEnabled()):
          value = dbus.Boolean(self.obj_server_verbose.isChecked())
          setEngineParameter("verbose", value, True)

        if (self.obj_server_client_timeout.isEnabled()):
          value = dbus.Int32(int(self.obj_server_client_timeout.currentText()))
          setEngineParameter("client-timeout", value, True)

        if (self.obj_server_clock_source.isEnabled()):
          value = None
          if (self.obj_server_clock_source_system.isChecked()):
            if (self.m_server_clock_source_broken):
              value = dbus.UInt32(JACK_TIMER_SYSTEM_CLOCK)
            else:
              value = dbus.Byte("s".encode("utf-8"))
          elif (self.obj_server_clock_source_cycle.isChecked()):
            if (self.m_server_clock_source_broken):
              value = dbus.UInt32(JACK_TIMER_CYCLE_COUNTER)
            else:
              value = dbus.Byte("c".encode("utf-8"))
          elif (self.obj_server_clock_source_hpet.isChecked()):
            if (self.m_server_clock_source_broken):
              value = dbus.UInt32(JACK_TIMER_HPET)
            else:
              value = dbus.Byte("h".encode("utf-8"))
          else:
            value = None
            print("JackSettings::saveServerSettings() - Cannot save clock-source value")

          if (value != None):
            setEngineParameter("clock-source", value, True)

        if (self.obj_server_port_max.isEnabled()):
          value = dbus.UInt32(int(self.obj_server_port_max.currentText()))
          setEngineParameter("port-max", value, True)

        if (self.obj_server_replace_registry.isEnabled()):
          value = dbus.Boolean(self.obj_server_replace_registry.isChecked())
          setEngineParameter("replace-registry", value, True)

        if (self.obj_server_sync.isEnabled()):
          value = dbus.Boolean(self.obj_server_sync.isChecked())
          setEngineParameter("sync", value, True)

        if (self.obj_server_self_connect_mode.isEnabled()):
          if (self.obj_server_self_connect_mode_0.isChecked()):
            value = dbus.Byte(" ".encode("utf-8"))
          elif (self.obj_server_self_connect_mode_1.isChecked()):
            value = dbus.Byte("E".encode("utf-8"))
          elif (self.obj_server_self_connect_mode_2.isChecked()):
            value = dbus.Byte("e".encode("utf-8"))
          elif (self.obj_server_self_connect_mode_3.isChecked()):
            value = dbus.Byte("A".encode("utf-8"))
          elif (self.obj_server_self_connect_mode_4.isChecked()):
            value = dbus.Byte("a".encode("utf-8"))
          else:
            value = None
            print("JackSettings::saveServerSettings() - Cannot save self-connect-mode value")

          if (value != None):
            setEngineParameter("self-connect-mode", value, True)

    def loadServerSettings(self, reset=False, forceReset=False):
        settings = jackctl.ReadContainer(["engine"])

        for i in range(len(settings[1])):
          attribute = str(settings[1][i])
          if (reset):
            value = jackctl.GetParameterValue(["engine",attribute])[1]
            if (forceReset and attribute != "driver"):
              jackctl.ResetParameterValue(["engine",attribute])
          else:
            value = jackctl.GetParameterValue(["engine",attribute])[2]

          if (attribute == "name"):
            self.obj_server_name.setText(str(value))
          elif (attribute == "realtime"):
            self.obj_server_realtime.setChecked(bool(value))
          elif (attribute == "realtime-priority"):
            self.obj_server_realtime_priority.setValue(int(value))
          elif (attribute == "temporary"):
            self.obj_server_temporary.setChecked(bool(value))
          elif (attribute == "verbose"):
            self.obj_server_verbose.setChecked(bool(value))
          elif (attribute == "client-timeout"):
            self.setComboBoxValue(self.obj_server_client_timeout, str(value))
          elif (attribute == "clock-source"):
            value = str(value)
            if (value == "c"):
              self.obj_server_clock_source_cycle.setChecked(True)
            elif (value == "h"):
              self.obj_server_clock_source_hpet.setChecked(True)
            elif (value == "s"):
              self.obj_server_clock_source_system.setChecked(True)
            else:
              self.m_server_clock_source_broken = True
              if (value == str(JACK_TIMER_SYSTEM_CLOCK)):
                self.obj_server_clock_source_system.setChecked(True)
              elif (value == str(JACK_TIMER_CYCLE_COUNTER)):
                self.obj_server_clock_source_cycle.setChecked(True)
              elif (value == str(JACK_TIMER_HPET)):
                self.obj_server_clock_source_hpet.setChecked(True)
              else:
                self.obj_server_clock_source.setEnabled(False)
                print("JackSettings::saveServerSettings() - Invalid clock-source value '%s'" % (value))
          elif (attribute == "port-max"):
            self.setComboBoxValue(self.obj_server_port_max, str(value))
          elif (attribute == "replace-registry"):
            self.obj_server_replace_registry.setChecked(bool(value))
          elif (attribute == "sync"):
            self.obj_server_sync.setChecked(bool(value))
          elif (attribute == "self-connect-mode"):
            value = str(value)
            if (value == " "):
              self.obj_server_self_connect_mode_0.setChecked(True)
            elif (value == "E"):
              self.obj_server_self_connect_mode_1.setChecked(True)
            elif (value == "e"):
              self.obj_server_self_connect_mode_2.setChecked(True)
            elif (value == "A"):
              self.obj_server_self_connect_mode_3.setChecked(True)
            elif (value == "a"):
              self.obj_server_self_connect_mode_4.setChecked(True)
            else:
              self.obj_server_self_connect_mode.setEnabled(False)
              print("JackSettings::loadServerSettings() - Invalid self-connect-mode value '%s'" % (value))
          elif (attribute in ("driver", "slave-drivers")):
            pass
          else:
            print("JackSettings::loadServerSettings() - Unimplemented server attribute '%s', value: '%s'" % (attribute, str(value)))

    # -------------------------------------------------------------
    # Driver calls

    def saveDriverSettings(self):
        if (self.obj_driver_device.isEnabled()):
          value = dbus.String(self.obj_driver_device.currentText().split(" ")[0])
          if (value != jackctl.GetParameterValue(["driver", "device"])[2]):
            jackctl.SetParameterValue(["driver", "device"], value)

        if (self.obj_driver_capture.isEnabled()):
          if (self.m_driver == "alsa"):
            value = dbus.String(self.obj_driver_capture.currentText().split(" ")[0])
          elif (self.m_driver == "dummy"):
            value = dbus.UInt32(int(self.obj_driver_capture.currentText()))
          elif (self.m_driver == "firewire"):
            value = dbus.Boolean(self.obj_driver_capture.currentIndex() == 1)
          else:
            value = None
            print("JackSettings::saveDriverSettings() - Cannot save capture value")

          if (value != None):
            setDriverParameter("capture", value, True)

        if (self.obj_driver_playback.isEnabled()):
          if (self.m_driver == "alsa"):
            value = dbus.String(self.obj_driver_playback.currentText().split(" ")[0])
          elif (self.m_driver == "dummy"):
            value = dbus.UInt32(int(self.obj_driver_playback.currentText()))
          elif (self.m_driver == "firewire"):
            value = dbus.Boolean(self.obj_driver_playback.currentIndex() == 1)
          else:
            value = None
            print("JackSettings::saveDriverSettings() - Cannot save playback value")

          if (value != None):
            setDriverParameter("playback", value, True)

        if (self.obj_driver_rate.isEnabled()):
          value = dbus.UInt32(int(self.obj_driver_rate.currentText()))
          setDriverParameter("rate", value, True)

        if (self.obj_driver_period.isEnabled()):
          value = dbus.UInt32(int(self.obj_driver_period.currentText()))
          setDriverParameter("period", value, True)

        if (self.obj_driver_nperiods.isEnabled()):
          value = dbus.UInt32(self.obj_driver_nperiods.value())
          setDriverParameter("nperiods", value, True)

        if (self.obj_driver_hwmon.isEnabled()):
          value = dbus.Boolean(self.obj_driver_hwmon.isChecked())
          setDriverParameter("hwmon", value, True)

        if (self.obj_driver_hwmeter.isEnabled()):
          value = dbus.Boolean(self.obj_driver_hwmeter.isChecked())
          setDriverParameter("hwmeter", value, True)

        if (self.obj_driver_duplex.isEnabled()):
          value = dbus.Boolean(self.obj_driver_duplex.isChecked())
          setDriverParameter("duplex", value, True)

        if (self.obj_driver_softmode.isEnabled()):
          value = dbus.Boolean(self.obj_driver_softmode.isChecked())
          setDriverParameter("softmode", value, True)

        if (self.obj_driver_monitor.isEnabled()):
          value = dbus.Boolean(self.obj_driver_monitor.isChecked())
          setDriverParameter("monitor", value, True)

        if (self.obj_driver_dither.isEnabled()):
          if (self.obj_driver_dither.currentIndex() == 0):
            value = dbus.Byte("n".encode("utf-8"))
          elif (self.obj_driver_dither.currentIndex() == 1):
            value = dbus.Byte("r".encode("utf-8"))
          elif (self.obj_driver_dither.currentIndex() == 2):
            value = dbus.Byte("s".encode("utf-8"))
          elif (self.obj_driver_dither.currentIndex() == 3):
            value = dbus.Byte("t".encode("utf-8"))
          else:
            value = None
            print("JackSettings::saveDriverSettings() - Cannot save dither value")

          if (value != None):
            setDriverParameter("dither", value, True)

        if (self.obj_driver_inchannels.isEnabled()):
          value = dbus.UInt32(self.obj_driver_inchannels.value())
          setDriverParameter("inchannels", value, True)

        if (self.obj_driver_outchannels.isEnabled()):
          value = dbus.UInt32(self.obj_driver_outchannels.value())
          setDriverParameter("outchannels", value, True)

        if (self.obj_driver_shorts.isEnabled()):
          value = dbus.Boolean(self.obj_driver_shorts.isChecked())
          setDriverParameter("shorts", value, True)

        if (self.obj_driver_input_latency.isEnabled()):
          value = dbus.UInt32(self.obj_driver_input_latency.value())
          setDriverParameter("input-latency", value, True)

        if (self.obj_driver_output_latency.isEnabled()):
          value = dbus.UInt32(self.obj_driver_output_latency.value())
          setDriverParameter("output-latency", value, True)

        if (self.obj_driver_midi_driver.isEnabled()):
          if (self.obj_driver_midi_driver.currentIndex() == 0):
            value = dbus.String("none")
          elif (self.obj_driver_midi_driver.currentIndex() == 1):
            value = dbus.String("seq")
          elif (self.obj_driver_midi_driver.currentIndex() == 2):
            value = dbus.String("raw")
          else:
            value = None
            print("JackSettings::saveDriverSettings() - Cannot save midi-driver value")

          if (value != None):
            if (driverHasFeature("midi")):
              setDriverParameter("midi", value, True)
            else:
              setDriverParameter("midi-driver", value, True)

        if (self.obj_driver_wait.isEnabled()):
          value = dbus.UInt32(self.obj_driver_wait.value())
          setDriverParameter("wait", value, True)

        if (self.obj_driver_verbose.isEnabled()):
          value = dbus.UInt32(self.obj_driver_verbose.value())
          setDriverParameter("verbose", value, True)

        if (self.obj_driver_snoop.isEnabled()):
          value = dbus.Boolean(self.obj_driver_snoop.isChecked())
          setDriverParameter("snoop", value, True)

        if (self.obj_driver_channels.isEnabled()):
          value = dbus.Int32(self.obj_driver_channels.value())
          setDriverParameter("channels", value, True)

    def loadDriverSettings(self, reset=False, forceReset=False):
        settings = jackctl.ReadContainer(["driver"])

        for i in range(len(settings[1])):
          attribute = str(settings[1][i])
          if (reset):
            value = jackctl.GetParameterValue(["driver",attribute])[1]
            if (forceReset):
              jackctl.ResetParameterValue(["driver",attribute])
          else:
            value = jackctl.GetParameterValue(["driver",attribute])[2]

          if (attribute == "device"):
            self.setComboBoxValue(self.obj_driver_device, str(value), True)
          elif (attribute == "capture"):
            if (self.m_driver == "firewire"):
              self.obj_driver_capture.setCurrentIndex(1 if bool(value) else 0)
            else:
              self.setComboBoxValue(self.obj_driver_capture, str(value), True)
          elif (attribute == "playback"):
            if (self.m_driver == "firewire"):
              self.obj_driver_playback.setCurrentIndex(1 if bool(value) else 0)
            else:
              self.setComboBoxValue(self.obj_driver_playback, str(value), True)
          elif (attribute == "rate"):
            self.setComboBoxValue(self.obj_driver_rate, str(value))
          elif (attribute == "period"):
            self.setComboBoxValue(self.obj_driver_period, str(value))
          elif (attribute == "nperiods"):
            self.obj_driver_nperiods.setValue(int(value))
          elif (attribute == "hwmon"):
            self.obj_driver_hwmon.setChecked(bool(value))
          elif (attribute == "hwmeter"):
            self.obj_driver_hwmeter.setChecked(bool(value))
          elif (attribute == "duplex"):
            self.obj_driver_duplex.setChecked(bool(value))
          elif (attribute == "softmode"):
            self.obj_driver_softmode.setChecked(bool(value))
          elif (attribute == "monitor"):
            self.obj_driver_monitor.setChecked(bool(value))
          elif (attribute == "dither"):
            value = str(value)
            if (value == "n"):
              self.obj_driver_dither.setCurrentIndex(0)
            elif (value == "r"):
              self.obj_driver_dither.setCurrentIndex(1)
            elif (value == "s"):
              self.obj_driver_dither.setCurrentIndex(2)
            elif (value == "t"):
              self.obj_driver_dither.setCurrentIndex(3)
            else:
              self.obj_driver_dither.setEnabled(False)
              print("JackSettings::loadDriverSettings() - Invalid dither value '%s'" % (value))
          elif (attribute == "inchannels"):
            self.obj_driver_inchannels.setValue(int(value))
          elif (attribute == "outchannels"):
            self.obj_driver_outchannels.setValue(int(value))
          elif (attribute == "shorts"):
            self.obj_driver_shorts.setChecked(bool(value))
          elif (attribute == "input-latency"):
            self.obj_driver_input_latency.setValue(int(value))
          elif (attribute == "output-latency"):
            self.obj_driver_output_latency.setValue(int(value))
          elif (attribute in ("midi", "midi-driver")):
            value = str(value)
            if (value == "none"):
              self.obj_driver_midi_driver.setCurrentIndex(0)
            elif (value == "seq"):
              self.obj_driver_midi_driver.setCurrentIndex(1)
            elif (value == "raw"):
              self.obj_driver_midi_driver.setCurrentIndex(2)
            else:
              self.obj_driver_midi_driver.setEnabled(False)
              print("JackSettings::loadDriverSettings() - Invalid midi-driver value '%s'" % (value))
          elif (attribute == "wait"):
            self.obj_driver_wait.setValue(int(value))
          elif (attribute == "verbose"):
            self.obj_driver_verbose.setValue(int(value))
          elif (attribute == "snoop"):
            self.obj_driver_snoop.setChecked(bool(value))
          elif (attribute == "channels"):
            self.obj_driver_channels.setValue(int(value))
          else:
            print("JackSettings::loadDriverSettings() - Unimplemented driver attribute '%s', value: '%s'" % (attribute, str(value)))

    # -------------------------------------------------------------
    # Helper functions

    def setComboBoxValue(self, box, text, split=False):
        for i in range(box.count()):
          if (box.itemText(i) == text or (box.itemText(i).split(" ")[0] == text and split)):
            box.setCurrentIndex(i)
            break
        else:
          if (text):
            box.addItem(text)
            box.setCurrentIndex(box.count()-1)

    # -------------------------------------------------------------
    # Qt SLOT calls

    @pyqtSlot()
    def slot_checkALSASelection(self):
        if (self.m_driver == "alsa"):
          check = (self.obj_driver_duplex.isChecked() and self.obj_driver_capture.currentIndex() > 0 and self.obj_driver_playback.currentIndex() > 0)
          self.obj_driver_device.setEnabled(not check)

    @pyqtSlot(bool)
    def slot_checkDuplexSelection(self, active):
        if (driverHasFeature("duplex")):
          self.obj_driver_capture.setEnabled(active)
          self.obj_driver_capture_label.setEnabled(active)
          #self.obj_driver_playback.setEnabled(active)
          #self.obj_driver_playback_label.setEnabled(active)
          self.obj_driver_inchannels.setEnabled(active)
          self.obj_driver_inchannels_label.setEnabled(active)
          self.obj_driver_input_latency.setEnabled(active)
          self.obj_driver_input_latency_label.setEnabled(active)

        self.slot_checkALSASelection()

    @pyqtSlot(int)
    def slot_checkDriverSelection(self, row):
        # Save previous settings
        self.saveDriverSettings()

        # Set new Jack driver
        self.m_driver = dbus.String(self.obj_server_driver.item(row, 0).text().lower())
        jackctl.SetParameterValue(["engine","driver"],self.m_driver)

        # Add device list
        self.obj_driver_device.clear()
        if (driverHasFeature("device")):
          dev_list = jackctl.GetParameterConstraint(["driver","device"])[3]
          for i in range(len(dev_list)):
            self.obj_driver_device.addItem(dev_list[i][0]+" [%s]" % (str(dev_list[i][1])))

        # Custom 'playback' and 'capture' values
        self.obj_driver_capture.clear()
        self.obj_driver_playback.clear()
        if (self.m_driver == "alsa"):
          self.obj_driver_capture.addItem("none")
          self.obj_driver_playback.addItem("none")
          dev_list = jackctl.GetParameterConstraint(["driver","device"])[3]
          for i in range(len(dev_list)):
            self.obj_driver_capture.addItem(dev_list[i][0]+" ["+dev_list[i][1]+"]")
            self.obj_driver_playback.addItem(dev_list[i][0]+" ["+dev_list[i][1]+"]")
        elif (self.m_driver == "dummy"):
          for i in range(16):
            self.obj_driver_capture.addItem("%i" % ((i*2)+2))
            self.obj_driver_playback.addItem("%i" % ((i*2)+2))
        elif (self.m_driver == "firewire"):
          self.obj_driver_capture.addItem("no")
          self.obj_driver_capture.addItem("yes")
          self.obj_driver_playback.addItem("no")
          self.obj_driver_playback.addItem("yes")
        elif (driverHasFeature("playback") or driverHasFeature("capture")):
          print("JackSettings::slot_checkDriverSelection() - Custom playback/capture for driver '%s' not implemented yet" % (self.m_driver))

        # Load Driver Settings
        self.loadDriverSettings()

        # Enable widgets according to driver
        self.obj_driver_capture.setEnabled(driverHasFeature("capture"))
        self.obj_driver_capture_label.setEnabled(driverHasFeature("capture"))
        self.obj_driver_playback.setEnabled(driverHasFeature("playback"))
        self.obj_driver_playback_label.setEnabled(driverHasFeature("playback"))
        self.obj_driver_device.setEnabled(driverHasFeature("device"))
        self.obj_driver_device_label.setEnabled(driverHasFeature("device"))
        self.obj_driver_rate.setEnabled(driverHasFeature("rate"))
        self.obj_driver_rate_label.setEnabled(driverHasFeature("rate"))
        self.obj_driver_period.setEnabled(driverHasFeature("period"))
        self.obj_driver_period_label.setEnabled(driverHasFeature("period"))
        self.obj_driver_nperiods.setEnabled(driverHasFeature("nperiods"))
        self.obj_driver_nperiods_label.setEnabled(driverHasFeature("nperiods"))
        self.obj_driver_hwmon.setEnabled(driverHasFeature("hwmon"))
        self.obj_driver_hwmeter.setEnabled(driverHasFeature("hwmeter"))
        self.obj_driver_duplex.setEnabled(driverHasFeature("duplex"))
        self.obj_driver_softmode.setEnabled(driverHasFeature("softmode"))
        self.obj_driver_monitor.setEnabled(driverHasFeature("monitor"))
        self.obj_driver_dither.setEnabled(driverHasFeature("dither"))
        self.obj_driver_dither_label.setEnabled(driverHasFeature("dither"))
        self.obj_driver_inchannels.setEnabled(driverHasFeature("inchannels"))
        self.obj_driver_inchannels_label.setEnabled(driverHasFeature("inchannels"))
        self.obj_driver_outchannels.setEnabled(driverHasFeature("outchannels"))
        self.obj_driver_outchannels_label.setEnabled(driverHasFeature("outchannels"))
        self.obj_driver_shorts.setEnabled(driverHasFeature("shorts"))
        self.obj_driver_input_latency.setEnabled(driverHasFeature("input-latency"))
        self.obj_driver_input_latency_label.setEnabled(driverHasFeature("input-latency"))
        self.obj_driver_output_latency.setEnabled(driverHasFeature("output-latency"))
        self.obj_driver_output_latency_label.setEnabled(driverHasFeature("output-latency"))
        self.obj_driver_midi_driver.setEnabled(driverHasFeature("midi") or driverHasFeature("midi-driver"))
        self.obj_driver_midi_driver_label.setEnabled(driverHasFeature("midi") or driverHasFeature("midi-driver"))
        self.obj_driver_wait.setEnabled(driverHasFeature("wait"))
        self.obj_driver_wait_label.setEnabled(driverHasFeature("wait"))
        self.obj_driver_verbose.setEnabled(driverHasFeature("verbose"))
        self.obj_driver_verbose_label.setEnabled(driverHasFeature("verbose"))
        self.obj_driver_snoop.setEnabled(driverHasFeature("snoop"))
        self.obj_driver_channels.setEnabled(driverHasFeature("channels"))
        self.obj_driver_channels_label.setEnabled(driverHasFeature("channels"))

        # Misc stuff
        if (self.obj_server_driver.item(0, row).text() == "ALSA"):
          self.toolbox_driver_misc.setCurrentIndex(1)
          self.obj_driver_capture_label.setText(self.tr("Input Device:"))
          self.obj_driver_playback_label.setText(self.tr("Output Device:"))

        elif (self.obj_server_driver.item(0, row).text() == "Dummy"):
          self.toolbox_driver_misc.setCurrentIndex(2)
          self.obj_driver_capture_label.setText(self.tr("Input Ports:"))
          self.obj_driver_playback_label.setText(self.tr("Output Ports:"))

        elif (self.obj_server_driver.item(0, row).text() == "FireWire"):
          self.toolbox_driver_misc.setCurrentIndex(3)
          self.obj_driver_capture_label.setText(self.tr("Capture Ports:"))
          self.obj_driver_playback_label.setText(self.tr("Playback Ports:"))

        elif (self.obj_server_driver.item(0, row).text() == "Loopback"):
          self.toolbox_driver_misc.setCurrentIndex(4)

        else:
          self.toolbox_driver_misc.setCurrentIndex(0)

        self.slot_checkDuplexSelection(self.obj_driver_duplex.isChecked())

    @pyqtSlot()
    def slot_saveJackSettings(self):
        self.saveServerSettings()
        self.saveDriverSettings()

    @pyqtSlot()
    def slot_resetJackSettings(self):
        if (self.tabWidget.currentIndex() == 0):
          self.loadServerSettings(True, True)
        elif (self.tabWidget.currentIndex() == 1):
          self.loadDriverSettings(True, True)

    @pyqtSlot()
    def slot_closeWithError(self):
        QMessageBox.critical(self, self.tr("Error"), self.tr("jackdbus is not available!\nIt's not possible to configure JACK at this point."))
        self.close()

# -------------------------------------------------------------
# Allow to use this as a standalone app
if __name__ == '__main__':

    # Additional imports
    import icons_rc
    from sys import argv as sys_argv, exit as sys_exit
    from PyQt4.QtGui import QApplication, QIcon

    # App initialization
    app = QApplication(sys_argv)

    # Connect to DBus
    if (dbus):
      if (initBus(dbus.SessionBus())):
        QMessageBox.critical(None, app.translate("JackSettingsW", "Error"), app.translate("JackSettingsW", "jackdbus is not available!\nIs not possible to configure JACK at this point."))
        sys_exit(1)
    else:
      QMessageBox.critical(None, app.translate("JackSettingsW", "Error"), app.translate("JackSettingsW", "DBus is not available, cannot continue."))
      sys_exit(1)

    # Show GUI
    gui = JackSettingsW(None)
    gui.setWindowIcon(QIcon(":/scalable/jack.svg"))
    gui.show()

    # App-Loop
    sys_exit(app.exec_())

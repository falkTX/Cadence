#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK Settings Dialog
# Copyright (C) 2010-2013 Filipe Coelho <falktx@falktx.com>
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

from PyQt4.QtCore import pyqtSlot, Qt, QSettings, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QDialog, QDialogButtonBox, QFontMetrics, QMessageBox
from sys import platform, version_info

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_settings_jack

# ------------------------------------------------------------------------------------------------------------
# Try Import DBus

try:
    import dbus
except:
    dbus = None

# ------------------------------------------------------------------------------------------------------------
# Global object

global gJackctl, gResetNeeded
gJackctl     = None
gResetNeeded = False

# ------------------------------------------------------------------------------------------------------------
# enum jack_timer_type_t

JACK_TIMER_SYSTEM_CLOCK  = 0
JACK_TIMER_CYCLE_COUNTER = 1
JACK_TIMER_HPET          = 2

# ------------------------------------------------------------------------------------------------------------
# Set Platform

if "linux" in platform:
    LINUX = True

    import glob
else:
    LINUX = False

# ------------------------------------------------------------------------------------------------------------
# Init DBus

def initBus(bus):
    global gJackctl

    if not bus:
        gJackctl = None
        return 1

    try:
        gJackctl = dbus.Interface(bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller"), "org.jackaudio.Configure")
        return 0
    except:
        gJackctl = None
        return 1

def needsInit():
    global gJackctl
    return bool(gJackctl is None)

def setResetNeeded(yesNo):
    global gResetNeeded
    gResetNeeded = yesNo

# ------------------------------------------------------------------------------------------------------------
# Helper functions

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

# ------------------------------------------------------------------------------------------------------------
# Helper functions (engine)

def engineHasFeature(feature):
    if gJackctl is None:
        return False
    try:
        featureList = gJackctl.ReadContainer(["engine"])[1]
    except:
        featureList = ()
    return bool(dbus.String(feature) in featureList)

def getEngineParameter(parameter, fallback):
    if gJackctl is None or not engineHasFeature(parameter):
        return fallback
    else:
        try:
            return gJackctl.GetParameterValue(["engine", parameter])[2]
        except:
            return fallback

def setEngineParameter(parameter, value, optional=True):
    if not engineHasFeature(parameter):
        return False
    elif optional:
        paramValueTry = gJackctl.GetParameterValue(["engine", parameter])
        if paramValueTry is None:
            return False
        paramValue = paramValueTry[2]
        if value != paramValue:
            return bool(gJackctl.SetParameterValue(["engine", parameter], value))
        else:
            return False
    else:
        return bool(gJackctl.SetParameterValue(["engine", parameter], value))

# ------------------------------------------------------------------------------------------------------------
# Helper functions (driver)

def driverHasFeature(feature):
    if gJackctl is None:
        return False
    try:
        featureList = gJackctl.ReadContainer(["driver"])[1]
    except:
        featureList = ()
    return bool(dbus.String(feature) in featureList)

def getDriverParameter(parameter, fallback):
    if gJackctl is None or not driverHasFeature(parameter):
        return fallback
    else:
        try:
            return gJackctl.GetParameterValue(["driver", parameter])[2]
        except:
            return fallback

def setDriverParameter(parameter, value, optional=True):
    if not driverHasFeature(parameter):
        return False
    elif optional:
        if value != gJackctl.GetParameterValue(["driver", parameter])[2]:
            return bool(gJackctl.SetParameterValue(["driver", parameter], value))
        else:
            return False
    else:
        return bool(gJackctl.SetParameterValue(["driver", parameter], value))

# ------------------------------------------------------------------------------------------------------------
# JACK Settings Dialog

class JackSettingsW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_settings_jack.Ui_JackSettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Check if we've got valid control interface

        global gJackctl

        if gJackctl is None:
            QTimer.singleShot(0, self, SLOT("slot_closeWithError()"))
            return

        # -------------------------------------------------------------
        # Align driver text and hide non available ones

        driverList = gJackctl.ReadContainer(["drivers"])[1]
        maxWidth   = 75

        for i in range(self.ui.obj_server_driver.rowCount()):
            self.ui.obj_server_driver.item(0, i).setTextAlignment(Qt.AlignCenter)

            itexText  = self.ui.obj_server_driver.item(0, i).text()
            itemWidth = QFontMetrics(self.ui.obj_server_driver.font()).width(itexText)+25

            if itemWidth > maxWidth:
                maxWidth = itemWidth

            if dbus.String(itexText.lower()) not in driverList:
                self.ui.obj_server_driver.hideRow(i)

        self.ui.obj_server_driver.setMinimumWidth(maxWidth)
        self.ui.obj_server_driver.setMaximumWidth(maxWidth)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveJackSettings()"))
        self.connect(self.ui.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetJackSettings()"))

        self.connect(self.ui.obj_driver_duplex, SIGNAL("clicked(bool)"), SLOT("slot_checkDuplexSelection(bool)"))
        self.connect(self.ui.obj_server_driver, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkDriverSelection(int)"))

        self.connect(self.ui.obj_driver_capture, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkALSASelection()"))
        self.connect(self.ui.obj_driver_playback, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkALSASelection()"))

        # -------------------------------------------------------------
        # Load initial settings

        self.fDriverName = ""
        self.fBrokenServerClockSource = False

        self.checkEngine()
        self.loadServerSettings()
        self.loadDriverSettings(True) # reset because we'll change it below

        # -------------------------------------------------------------
        # Load selected JACK driver

        self.fDriverName = str(gJackctl.GetParameterValue(["engine", "driver"])[2])
        for i in range(self.ui.obj_server_driver.rowCount()):
            if self.ui.obj_server_driver.item(i, 0).text().lower() == self.fDriverName:
                self.ui.obj_server_driver.setCurrentCell(i, 0)
                break

        # Special ALSA check
        self.slot_checkALSASelection()

        # -------------------------------------------------------------
        # Load last GUI settings

        self.loadSettings()

    # -----------------------------------------------------------------
    # Engine calls

    def checkEngine(self):
        self.ui.obj_server_realtime.setEnabled(engineHasFeature("realtime"))
        self.ui.obj_server_realtime_priority.setEnabled(engineHasFeature("realtime-priority"))
        self.ui.obj_server_temporary.setEnabled(engineHasFeature("temporary"))
        self.ui.obj_server_verbose.setEnabled(engineHasFeature("verbose"))
        self.ui.obj_server_alias.setEnabled(engineHasFeature("alias"))
        self.ui.obj_server_client_timeout.setEnabled(engineHasFeature("client-timeout"))
        self.ui.obj_server_clock_source.setEnabled(engineHasFeature("clock-source"))
        self.ui.obj_server_port_max.setEnabled(engineHasFeature("port-max"))
        self.ui.obj_server_replace_registry.setEnabled(engineHasFeature("replace-registry"))
        self.ui.obj_server_sync.setEnabled(engineHasFeature("sync"))
        self.ui.obj_server_self_connect_mode.setEnabled(engineHasFeature("self-connect-mode"))

        # Disable clock-source if not on Linux
        if not LINUX:
            self.ui.obj_server_clock_source.setEnabled(False)

    # -----------------------------------------------------------------
    # Server calls

    def saveServerSettings(self):
        # always reset server name
        if engineHasFeature("name"):
            setEngineParameter("name", "default", True)

        if self.ui.obj_server_realtime.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_realtime.isChecked())
            setEngineParameter("realtime", value, True)

        if self.ui.obj_server_realtime_priority.isEnabled():
            value = dbus.Int32(self.ui.obj_server_realtime_priority.value())
            setEngineParameter("realtime-priority", value, True)

        if self.ui.obj_server_temporary.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_temporary.isChecked())
            setEngineParameter("temporary", value, True)

        if self.ui.obj_server_verbose.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_verbose.isChecked())
            setEngineParameter("verbose", value, True)

        if self.ui.obj_server_alias.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_alias.isChecked())
            setEngineParameter("alias", value, True)

        if self.ui.obj_server_client_timeout.isEnabled():
            value = dbus.Int32(int(self.ui.obj_server_client_timeout.currentText()))
            setEngineParameter("client-timeout", value, True)

        if self.ui.obj_server_clock_source.isEnabled():
            if self.ui.obj_server_clock_source_system.isChecked():
                if self.fBrokenServerClockSource:
                    value = dbus.UInt32(JACK_TIMER_SYSTEM_CLOCK)
                else:
                    value = dbus.Byte("s".encode("utf-8"))
            elif self.ui.obj_server_clock_source_cycle.isChecked():
                if self.fBrokenServerClockSource:
                    value = dbus.UInt32(JACK_TIMER_CYCLE_COUNTER)
                else:
                    value = dbus.Byte("c".encode("utf-8"))
            elif self.ui.obj_server_clock_source_hpet.isChecked():
                if self.fBrokenServerClockSource:
                    value = dbus.UInt32(JACK_TIMER_HPET)
                else:
                    value = dbus.Byte("h".encode("utf-8"))
            else:
                value = None
                print("JackSettingsW::saveServerSettings() - Cannot save clock-source value")

            if value != None:
                setEngineParameter("clock-source", value, True)

        if self.ui.obj_server_port_max.isEnabled():
            value = dbus.UInt32(int(self.ui.obj_server_port_max.currentText()))
            setEngineParameter("port-max", value, True)

        if self.ui.obj_server_replace_registry.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_replace_registry.isChecked())
            setEngineParameter("replace-registry", value, True)

        if self.ui.obj_server_sync.isEnabled():
            value = dbus.Boolean(self.ui.obj_server_sync.isChecked())
            setEngineParameter("sync", value, True)

        if self.ui.obj_server_self_connect_mode.isEnabled():
            if self.ui.obj_server_self_connect_mode_0.isChecked():
                value = dbus.Byte(" ".encode("utf-8"))
            elif self.ui.obj_server_self_connect_mode_1.isChecked():
                value = dbus.Byte("E".encode("utf-8"))
            elif self.ui.obj_server_self_connect_mode_2.isChecked():
                value = dbus.Byte("e".encode("utf-8"))
            elif self.ui.obj_server_self_connect_mode_3.isChecked():
                value = dbus.Byte("A".encode("utf-8"))
            elif self.ui.obj_server_self_connect_mode_4.isChecked():
                value = dbus.Byte("a".encode("utf-8"))
            else:
                value = None
                print("JackSettingsW::saveServerSettings() - Cannot save self-connect-mode value")

            if value != None:
                setEngineParameter("self-connect-mode", value, True)

    def loadServerSettings(self, reset=False, forceReset=False):
        global gJackctl

        settings = gJackctl.ReadContainer(["engine"])

        for i in range(len(settings[1])):
            attribute = str(settings[1][i])
            if reset:
                valueTry = gJackctl.GetParameterValue(["engine", attribute])

                if valueTry is None:
                    continue
                else:
                    value = valueTry[1]

                if forceReset and attribute != "driver":
                    gJackctl.ResetParameterValue(["engine", attribute])
            else:
                valueTry = gJackctl.GetParameterValue(["engine", attribute])

                if valueTry is None:
                    continue
                else:
                    value = valueTry[2]

            if attribute == "name":
                pass # Don't allow to change this
            elif attribute == "realtime":
                self.ui.obj_server_realtime.setChecked(bool(value))
            elif attribute == "realtime-priority":
                self.ui.obj_server_realtime_priority.setValue(int(value))
            elif attribute == "temporary":
                self.ui.obj_server_temporary.setChecked(bool(value))
            elif attribute == "verbose":
                self.ui.obj_server_verbose.setChecked(bool(value))
            elif attribute == "alias":
                self.ui.obj_server_alias.setChecked(bool(value))
            elif attribute == "client-timeout":
                self.setComboBoxValue(self.ui.obj_server_client_timeout, str(value))
            elif attribute == "clock-source":
                value = str(value)
                if value == "c":
                    self.ui.obj_server_clock_source_cycle.setChecked(True)
                elif value == "h":
                    self.ui.obj_server_clock_source_hpet.setChecked(True)
                elif value == "s":
                    self.ui.obj_server_clock_source_system.setChecked(True)
                else:
                    self.fBrokenServerClockSource = True
                    if value == str(JACK_TIMER_SYSTEM_CLOCK):
                        self.ui.obj_server_clock_source_system.setChecked(True)
                    elif value == str(JACK_TIMER_CYCLE_COUNTER):
                        self.ui.obj_server_clock_source_cycle.setChecked(True)
                    elif value == str(JACK_TIMER_HPET):
                        self.ui.obj_server_clock_source_hpet.setChecked(True)
                    else:
                        self.ui.obj_server_clock_source.setEnabled(False)
                        print("JackSettingsW::saveServerSettings() - Invalid clock-source value '%s'" % value)
            elif attribute == "port-max":
                self.setComboBoxValue(self.ui.obj_server_port_max, str(value))
            elif attribute == "replace-registry":
                self.ui.obj_server_replace_registry.setChecked(bool(value))
            elif attribute == "sync":
                self.ui.obj_server_sync.setChecked(bool(value))
            elif attribute == "self-connect-mode":
                value = str(value)
                if value == " ":
                    self.ui.obj_server_self_connect_mode_0.setChecked(True)
                elif value == "E":
                    self.ui.obj_server_self_connect_mode_1.setChecked(True)
                elif value == "e":
                    self.ui.obj_server_self_connect_mode_2.setChecked(True)
                elif value == "A":
                    self.ui.obj_server_self_connect_mode_3.setChecked(True)
                elif value == "a":
                    self.ui.obj_server_self_connect_mode_4.setChecked(True)
                else:
                    self.ui.obj_server_self_connect_mode.setEnabled(False)
                    print("JackSettingsW::loadServerSettings() - Invalid self-connect-mode value '%s'" % value)
            elif attribute in ("driver", "slave-drivers"):
                pass
            else:
                print("JackSettingsW::loadServerSettings() - Unimplemented server attribute '%s', value: '%s'" % (attribute, str(value)))

    # -----------------------------------------------------------------
    # Driver calls

    # resetIfNeeded: fix alsa parameter re-order bug in JACK 1.9.8 (reset/remove non-used values)

    def saveDriverSettings(self, resetIfNeeded):
        global gJackctl, gResetNeeded

        if resetIfNeeded and not gResetNeeded:
            resetIfNeeded = False

        if self.ui.obj_driver_device.isEnabled():
            value = dbus.String(self.ui.obj_driver_device.currentText().split(" [")[0])
            if value != gJackctl.GetParameterValue(["driver", "device"])[2]:
                gJackctl.SetParameterValue(["driver", "device"], value)

        elif resetIfNeeded:
            gJackctl.ResetParameterValue(["driver", "device"])

        if self.ui.obj_driver_capture.isEnabled():
            if self.fDriverName == "alsa":
                value = dbus.String(self.ui.obj_driver_capture.currentText().split(" ")[0])
            elif self.fDriverName == "dummy":
                value = dbus.UInt32(int(self.ui.obj_driver_capture.currentText()))
            elif self.fDriverName == "firewire":
                value = dbus.Boolean(self.ui.obj_driver_capture.currentIndex() == 1)
            else:
                value = None
                print("JackSettingsW::saveDriverSettings() - Cannot save capture value")

            if value != None:
                setDriverParameter("capture", value, True)

        elif resetIfNeeded:
            gJackctl.ResetParameterValue(["driver", "capture"])

        if self.ui.obj_driver_playback.isEnabled():
            if self.fDriverName == "alsa":
                value = dbus.String(self.ui.obj_driver_playback.currentText().split(" ")[0])
            elif self.fDriverName == "dummy":
                value = dbus.UInt32(int(self.ui.obj_driver_playback.currentText()))
            elif self.fDriverName == "firewire":
                value = dbus.Boolean(self.ui.obj_driver_playback.currentIndex() == 1)
            else:
                value = None
                print("JackSettingsW::saveDriverSettings() - Cannot save playback value")

            if value != None:
                setDriverParameter("playback", value, True)

        elif resetIfNeeded:
            gJackctl.ResetParameterValue(["driver", "playback"])

        if self.ui.obj_driver_rate.isEnabled():
            value = dbus.UInt32(int(self.ui.obj_driver_rate.currentText()))
            setDriverParameter("rate", value, True)

        if self.ui.obj_driver_period.isEnabled():
            value = dbus.UInt32(int(self.ui.obj_driver_period.currentText()))
            setDriverParameter("period", value, True)

        if self.ui.obj_driver_nperiods.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_nperiods.value())
            setDriverParameter("nperiods", value, True)

        if self.ui.obj_driver_hwmon.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_hwmon.isChecked())
            setDriverParameter("hwmon", value, True)

        if self.ui.obj_driver_hwmeter.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_hwmeter.isChecked())
            setDriverParameter("hwmeter", value, True)

        if self.ui.obj_driver_duplex.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_duplex.isChecked())
            setDriverParameter("duplex", value, True)

        if self.ui.obj_driver_hw_alias.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_hw_alias.isChecked())
            setDriverParameter("hw-alias", value, True)

        if self.ui.obj_driver_softmode.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_softmode.isChecked())
            setDriverParameter("softmode", value, True)

        if self.ui.obj_driver_monitor.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_monitor.isChecked())
            setDriverParameter("monitor", value, True)

        if self.ui.obj_driver_dither.isEnabled():
            if self.ui.obj_driver_dither.currentIndex() == 0:
                value = dbus.Byte("n".encode("utf-8"))
            elif self.ui.obj_driver_dither.currentIndex() == 1:
                value = dbus.Byte("r".encode("utf-8"))
            elif self.ui.obj_driver_dither.currentIndex() == 2:
                value = dbus.Byte("s".encode("utf-8"))
            elif self.ui.obj_driver_dither.currentIndex() == 3:
                value = dbus.Byte("t".encode("utf-8"))
            else:
                value = None
                print("JackSettingsW::saveDriverSettings() - Cannot save dither value")

            if value != None:
                setDriverParameter("dither", value, True)

        if self.ui.obj_driver_inchannels.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_inchannels.value())
            setDriverParameter("inchannels", value, True)

        if self.ui.obj_driver_outchannels.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_outchannels.value())
            setDriverParameter("outchannels", value, True)

        if self.ui.obj_driver_shorts.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_shorts.isChecked())
            setDriverParameter("shorts", value, True)

        if self.ui.obj_driver_input_latency.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_input_latency.value())
            setDriverParameter("input-latency", value, True)

        if self.ui.obj_driver_output_latency.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_output_latency.value())
            setDriverParameter("output-latency", value, True)

        if self.ui.obj_driver_midi_driver.isEnabled():
            if self.ui.obj_driver_midi_driver.currentIndex() == 0:
                value = dbus.String("none")
            elif self.ui.obj_driver_midi_driver.currentIndex() == 1:
                value = dbus.String("seq")
            elif self.ui.obj_driver_midi_driver.currentIndex() == 2:
                value = dbus.String("raw")
            else:
                value = None
                print("JackSettingsW::saveDriverSettings() - Cannot save midi-driver value")

            if value != None:
                if driverHasFeature("midi"):
                    setDriverParameter("midi", value, True)
                else:
                    setDriverParameter("midi-driver", value, True)

        if self.ui.obj_driver_wait.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_wait.value())
            setDriverParameter("wait", value, True)

        if self.ui.obj_driver_verbose.isEnabled():
            value = dbus.UInt32(self.ui.obj_driver_verbose.value())
            setDriverParameter("verbose", value, True)

        if self.ui.obj_driver_snoop.isEnabled():
            value = dbus.Boolean(self.ui.obj_driver_snoop.isChecked())
            setDriverParameter("snoop", value, True)

        if self.ui.obj_driver_channels.isEnabled():
            value = dbus.Int32(self.ui.obj_driver_channels.value())
            setDriverParameter("channels", value, True)

    def loadDriverSettings(self, reset=False, forceReset=False):
        global gJackctl

        settings = gJackctl.ReadContainer(["driver"])

        for i in range(len(settings[1])):
            attribute = str(settings[1][i])
            if reset:
                value = gJackctl.GetParameterValue(["driver", attribute])[1]
                if forceReset:
                    gJackctl.ResetParameterValue(["driver", attribute])
            else:
                value = gJackctl.GetParameterValue(["driver", attribute])[2]

            if attribute == "device":
                self.setComboBoxValue(self.ui.obj_driver_device, str(value), True)
            elif attribute == "capture":
                if self.fDriverName == "firewire":
                    self.ui.obj_driver_capture.setCurrentIndex(1 if bool(value) else 0)
                else:
                    self.setComboBoxValue(self.ui.obj_driver_capture, str(value), True)
            elif attribute == "playback":
                if self.fDriverName == "firewire":
                    self.ui.obj_driver_playback.setCurrentIndex(1 if bool(value) else 0)
                else:
                    self.setComboBoxValue(self.ui.obj_driver_playback, str(value), True)
            elif attribute == "rate":
                self.setComboBoxValue(self.ui.obj_driver_rate, str(value))
            elif attribute == "period":
                self.setComboBoxValue(self.ui.obj_driver_period, str(value))
            elif attribute == "nperiods":
                self.ui.obj_driver_nperiods.setValue(int(value))
            elif attribute == "hwmon":
                self.ui.obj_driver_hwmon.setChecked(bool(value))
            elif attribute == "hwmeter":
                self.ui.obj_driver_hwmeter.setChecked(bool(value))
            elif attribute == "duplex":
                self.ui.obj_driver_duplex.setChecked(bool(value))
            elif attribute == "hw-alias":
                self.ui.obj_driver_hw_alias.setChecked(bool(value))
            elif attribute == "softmode":
                self.ui.obj_driver_softmode.setChecked(bool(value))
            elif attribute == "monitor":
                self.ui.obj_driver_monitor.setChecked(bool(value))
            elif attribute == "dither":
                value = str(value)
                if value == "n":
                    self.ui.obj_driver_dither.setCurrentIndex(0)
                elif value == "r":
                    self.ui.obj_driver_dither.setCurrentIndex(1)
                elif value == "s":
                    self.ui.obj_driver_dither.setCurrentIndex(2)
                elif value == "t":
                    self.ui.obj_driver_dither.setCurrentIndex(3)
                else:
                    self.ui.obj_driver_dither.setEnabled(False)
                    print("JackSettingsW::loadDriverSettings() - Invalid dither value '%s'" % value)
            elif attribute == "inchannels":
                self.ui.obj_driver_inchannels.setValue(int(value))
            elif attribute == "outchannels":
                self.ui.obj_driver_outchannels.setValue(int(value))
            elif attribute == "shorts":
                self.ui.obj_driver_shorts.setChecked(bool(value))
            elif attribute == "input-latency":
                self.ui.obj_driver_input_latency.setValue(int(value))
            elif attribute == "output-latency":
                self.ui.obj_driver_output_latency.setValue(int(value))
            elif attribute in ("midi", "midi-driver"):
                value = str(value)
                if value == "none":
                    self.ui.obj_driver_midi_driver.setCurrentIndex(0)
                elif value == "seq":
                    self.ui.obj_driver_midi_driver.setCurrentIndex(1)
                elif value == "raw":
                    self.ui.obj_driver_midi_driver.setCurrentIndex(2)
                else:
                    self.ui.obj_driver_midi_driver.setEnabled(False)
                    print("JackSettingsW::loadDriverSettings() - Invalid midi-driver value '%s'" % value)
            elif attribute == "wait":
                self.ui.obj_driver_wait.setValue(int(value))
            elif attribute == "verbose":
                self.ui.obj_driver_verbose.setValue(int(value))
            elif attribute == "snoop":
                self.ui.obj_driver_snoop.setChecked(bool(value))
            elif attribute == "channels":
                self.ui.obj_driver_channels.setValue(int(value))
            else:
                print("JackSettingsW::loadDriverSettings() - Unimplemented driver attribute '%s', value: '%s'" % (attribute, str(value)))

    # -----------------------------------------------------------------
    # Helper functions

    def getAlsaDeviceList(self, mode = 'DUPLEX'):

        class AlsaDevice:
            @property
            def key(self):
                return '%s,%s' % (self.card, self.device)

            def has(self, mode):
                if mode in self.mode: return True
                if mode == 'DUPLEX':
                    if ('CAPTURE' in self.mode) and ('PLAYBACK' in self.mode):
                        return True
                return False

            def __repr__(self):
                return 'AlsaDevice<id="%s,%s" mode=%s>' % (self.card, self.device, self.mode)

        alsaDeviceList = {}

        for filename in glob.glob('/proc/asound/card*/*/info'):

            info = open(filename)

            device = AlsaDevice()

            for line in info:
                key, value = line.split(':', 2)
                setattr(device, key, value.strip())

            # Skip loopback device
            if device.name == 'Loopback': continue

            device_old = alsaDeviceList.get(device.key, None)
            if device_old:
                if device.stream not in device_old.mode:
                    device_old.mode.append(device.stream)
            else:
                device.mode = [ device.stream ]
                alsaDeviceList[device.key] = device

        return [('hw:%s,%s [%s]' % (device.card, device.device, device.name)) for device in alsaDeviceList.values() if device.has(mode)]


    def setComboBoxValue(self, box, text, split=False):
        for i in range(box.count()):
            if box.itemText(i) == text or (box.itemText(i).split(" ")[0] == text and split):
                box.setCurrentIndex(i)
                break
        else:
            if text:
                box.addItem(text)
                box.setCurrentIndex(box.count() - 1)

    # -----------------------------------------------------------------
    # Qt SLOT calls

    @pyqtSlot()
    def slot_checkALSASelection(self):
        if self.fDriverName == "alsa":
            check = bool(self.ui.obj_driver_duplex.isChecked() and (self.ui.obj_driver_capture.currentIndex() > 0 or self.ui.obj_driver_playback.currentIndex() > 0))
            self.ui.obj_driver_device.setEnabled(not check)

    @pyqtSlot(bool)
    def slot_checkDuplexSelection(self, active):
        if driverHasFeature("duplex"):
            self.ui.obj_driver_capture.setEnabled(active)
            self.ui.obj_driver_capture_label.setEnabled(active)
            self.ui.obj_driver_playback.setEnabled(active)
            self.ui.obj_driver_playback_label.setEnabled(active)
            #self.ui.obj_driver_inchannels.setEnabled(active)
            #self.ui.obj_driver_inchannels_label.setEnabled(active)
            #self.ui.obj_driver_input_latency.setEnabled(active)
            #self.ui.obj_driver_input_latency_label.setEnabled(active)

        self.slot_checkALSASelection()

    @pyqtSlot(int)
    def slot_checkDriverSelection(self, row):
        global gJackctl

        # Save previous settings
        self.saveDriverSettings(False)

        # Set new Jack driver
        self.fDriverName = dbus.String(self.ui.obj_server_driver.item(row, 0).text().lower())
        gJackctl.SetParameterValue(["engine", "driver"], self.fDriverName)

        # Add device list
        self.ui.obj_driver_device.clear()
        if driverHasFeature("device"):
            if LINUX and self.fDriverName == "alsa":
                dev_list = self.getAlsaDeviceList()
                for dev in dev_list:
                    self.ui.obj_driver_device.addItem(dev)
            else:
                dev_list = gJackctl.GetParameterConstraint(["driver", "device"])[3]
                for i in range(len(dev_list)):
                    self.ui.obj_driver_device.addItem(dev_list[i][0] + " [%s]" % str(dev_list[i][1]))

        # Custom 'playback' and 'capture' values
        self.ui.obj_driver_capture.clear()
        self.ui.obj_driver_playback.clear()

        if self.fDriverName == "alsa":
            self.ui.obj_driver_capture.addItem("none")
            self.ui.obj_driver_playback.addItem("none")

            if LINUX:
                dev_list = self.getAlsaDeviceList('CAPTURE')
                for dev in dev_list:
                    self.ui.obj_driver_capture.addItem(dev)

                dev_list = self.getAlsaDeviceList('PLAYBACK')
                for dev in dev_list:
                    self.ui.obj_driver_playback.addItem(dev)
            else:
                dev_list = gJackctl.GetParameterConstraint(["driver", "device"])[3]
                for i in range(len(dev_list)):
                    self.ui.obj_driver_capture.addItem(dev_list[i][0] + " [" + dev_list[i][1] + "]")
                    self.ui.obj_driver_playback.addItem(dev_list[i][0] + " [" + dev_list[i][1] + "]")

        elif self.fDriverName == "dummy":
            for i in range(16):
                self.ui.obj_driver_capture.addItem("%i" % int((i * 2) + 2))
                self.ui.obj_driver_playback.addItem("%i" % int((i * 2) + 2))

        elif self.fDriverName == "firewire":
            self.ui.obj_driver_capture.addItem("no")
            self.ui.obj_driver_capture.addItem("yes")
            self.ui.obj_driver_playback.addItem("no")
            self.ui.obj_driver_playback.addItem("yes")

        elif driverHasFeature("playback") or driverHasFeature("capture"):
            print("JackSettingsW::slot_checkDriverSelection() - Custom playback/capture for driver '%s' not implemented yet" % self.fDriverName)

        # Load Driver Settings
        self.loadDriverSettings()

        # Enable widgets according to driver
        self.ui.obj_driver_capture.setEnabled(driverHasFeature("capture"))
        self.ui.obj_driver_capture_label.setEnabled(driverHasFeature("capture"))
        self.ui.obj_driver_playback.setEnabled(driverHasFeature("playback"))
        self.ui.obj_driver_playback_label.setEnabled(driverHasFeature("playback"))
        self.ui.obj_driver_device.setEnabled(driverHasFeature("device"))
        self.ui.obj_driver_device_label.setEnabled(driverHasFeature("device"))
        self.ui.obj_driver_rate.setEnabled(driverHasFeature("rate"))
        self.ui.obj_driver_rate_label.setEnabled(driverHasFeature("rate"))
        self.ui.obj_driver_period.setEnabled(driverHasFeature("period"))
        self.ui.obj_driver_period_label.setEnabled(driverHasFeature("period"))
        self.ui.obj_driver_nperiods.setEnabled(driverHasFeature("nperiods"))
        self.ui.obj_driver_nperiods_label.setEnabled(driverHasFeature("nperiods"))
        self.ui.obj_driver_hwmon.setEnabled(driverHasFeature("hwmon"))
        self.ui.obj_driver_hwmeter.setEnabled(driverHasFeature("hwmeter"))
        self.ui.obj_driver_duplex.setEnabled(driverHasFeature("duplex"))
        self.ui.obj_driver_hw_alias.setEnabled(driverHasFeature("hw-alias"))
        self.ui.obj_driver_softmode.setEnabled(driverHasFeature("softmode"))
        self.ui.obj_driver_monitor.setEnabled(driverHasFeature("monitor"))
        self.ui.obj_driver_dither.setEnabled(driverHasFeature("dither"))
        self.ui.obj_driver_dither_label.setEnabled(driverHasFeature("dither"))
        self.ui.obj_driver_inchannels.setEnabled(driverHasFeature("inchannels"))
        self.ui.obj_driver_inchannels_label.setEnabled(driverHasFeature("inchannels"))
        self.ui.obj_driver_outchannels.setEnabled(driverHasFeature("outchannels"))
        self.ui.obj_driver_outchannels_label.setEnabled(driverHasFeature("outchannels"))
        self.ui.obj_driver_shorts.setEnabled(driverHasFeature("shorts"))
        self.ui.obj_driver_input_latency.setEnabled(driverHasFeature("input-latency"))
        self.ui.obj_driver_input_latency_label.setEnabled(driverHasFeature("input-latency"))
        self.ui.obj_driver_output_latency.setEnabled(driverHasFeature("output-latency"))
        self.ui.obj_driver_output_latency_label.setEnabled(driverHasFeature("output-latency"))
        self.ui.obj_driver_midi_driver.setEnabled(driverHasFeature("midi") or driverHasFeature("midi-driver"))
        self.ui.obj_driver_midi_driver_label.setEnabled(driverHasFeature("midi") or driverHasFeature("midi-driver"))
        self.ui.obj_driver_wait.setEnabled(driverHasFeature("wait"))
        self.ui.obj_driver_wait_label.setEnabled(driverHasFeature("wait"))
        self.ui.obj_driver_verbose.setEnabled(driverHasFeature("verbose"))
        self.ui.obj_driver_verbose_label.setEnabled(driverHasFeature("verbose"))
        self.ui.obj_driver_snoop.setEnabled(driverHasFeature("snoop"))
        self.ui.obj_driver_channels.setEnabled(driverHasFeature("channels"))
        self.ui.obj_driver_channels_label.setEnabled(driverHasFeature("channels"))

        # Misc stuff
        if self.ui.obj_server_driver.item(0, row).text() == "ALSA":
            self.ui.toolbox_driver_misc.setCurrentIndex(1)
            self.ui.obj_driver_capture_label.setText(self.tr("Input Device:"))
            self.ui.obj_driver_playback_label.setText(self.tr("Output Device:"))

        elif self.ui.obj_server_driver.item(0, row).text() == "Dummy":
            self.ui.toolbox_driver_misc.setCurrentIndex(2)
            self.ui.obj_driver_capture_label.setText(self.tr("Input Ports:"))
            self.ui.obj_driver_playback_label.setText(self.tr("Output Ports:"))

        elif self.ui.obj_server_driver.item(0, row).text() == "FireWire":
            self.ui.toolbox_driver_misc.setCurrentIndex(3)
            self.ui.obj_driver_capture_label.setText(self.tr("Capture Ports:"))
            self.ui.obj_driver_playback_label.setText(self.tr("Playback Ports:"))

        elif self.ui.obj_server_driver.item(0, row).text() == "Loopback":
            self.ui.toolbox_driver_misc.setCurrentIndex(4)

        else:
            self.ui.toolbox_driver_misc.setCurrentIndex(0)

        self.slot_checkDuplexSelection(self.ui.obj_driver_duplex.isChecked())

    @pyqtSlot()
    def slot_saveJackSettings(self):
        self.saveServerSettings()
        self.saveDriverSettings(True)

    @pyqtSlot()
    def slot_resetJackSettings(self):
        if self.ui.tabWidget.currentIndex() == 0:
            self.loadServerSettings(True, True)
        elif self.ui.tabWidget.currentIndex() == 1:
            self.loadDriverSettings(True, True)

    @pyqtSlot()
    def slot_closeWithError(self):
        QMessageBox.critical(self, self.tr("Error"), self.tr("jackdbus is not available!\nIt's not possible to configure JACK at this point."))
        self.close()

    def saveSettings(self):
        settings = QSettings("Cadence", "JackSettings")
        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("CurrentTab", self.ui.tabWidget.currentIndex())

    def loadSettings(self):
        settings = QSettings("Cadence", "JackSettings")
        self.restoreGeometry(settings.value("Geometry", ""))
        self.ui.tabWidget.setCurrentIndex(settings.value("CurrentTab", 0, type=int))

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Allow to use this as a standalone app

if __name__ == '__main__':
    # Additional imports
    import resources_rc
    from sys import argv as sys_argv, exit as sys_exit
    from PyQt4.QtGui import QApplication, QIcon

    # App initialization
    app = QApplication(sys_argv)

    # Connect to DBus
    if dbus:
        if initBus(dbus.SessionBus()):
            QMessageBox.critical(None, app.translate("JackSettingsW", "Error"), app.translate("JackSettingsW",
                "jackdbus is not available!\n"
                "Is not possible to configure JACK at this point."))
            sys_exit(1)
    else:
        QMessageBox.critical(None, app.translate("JackSettingsW", "Error"),
            app.translate("JackSettingsW", "DBus is not available, cannot continue."))
        sys_exit(1)

    # Show GUI
    gui = JackSettingsW(None)
    gui.setWindowIcon(QIcon(":/scalable/jack.svg"))
    gui.show()

    # App-Loop
    sys_exit(app.exec_())

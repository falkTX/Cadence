#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ...
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
from subprocess import getoutput
from PyQt4.QtCore import QSettings
from PyQt4.QtGui import QApplication, QMainWindow

# Imports (Custom Stuff)
import ui_cadence
import systray
from shared_jack import *
from shared_settings import *

try:
    import dbus
    from dbus.mainloop.qt import DBusQtMainLoop
    haveDBus = True
except:
    haveDBus = False

# ---------------------------------------------------------------------

def get_architecture():
    # FIXME - more checks
    if sys.int_info[1] == 4:
        return "64-bit"
    return "32-bit"

def get_linux_distro():
    distro = ""

    if os.path.exists("/etc/lsb-release"):
        distro = getoutput(". /etc/lsb-release && echo $DISTRIB_DESCRIPTION")
    elif os.path.exists("/etc/arch-release"):
        distro = "ArchLinux"
    else:
        distro = os.uname()[0]

    return distro

def get_linux_kernel():
    return os.uname()[2]

def get_windows_information():
    major = sys.getwindowsversion()[0]
    minor = sys.getwindowsversion()[1]
    servp = sys.getwindowsversion()[4]

    os      = "Windows"
    version = servp

    if major == 4 and minor == 0:
        os      = "Windows 95"
        version = "RTM"
    elif major == 4 and minor == 10:
        os      = "Windows 98"
        version = "Second Edition"
    elif major == 5 and minor == 0:
        os      = "Windows 2000"
    elif major == 5 and minor == 1:
        os      = "Windows XP"
    elif major == 5 and minor == 2:
        os      = "Windows Server 2003"
    elif major == 6 and minor == 0:
        os      = "Windows Vista"
    elif major == 6 and minor == 1:
        os      = "Windows 7"
    elif major == 6 and minor == 2:
        os      = "Windows 8"

    return (os, version)

# ---------------------------------------------------------------------

# Main Window
class CadenceMainW(QMainWindow, ui_cadence.Ui_CadenceMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Cadence")
        self.loadSettings(True)

        # TODO
        self.b_jack_restart.setEnabled(False)

        # -------------------------------------------------------------
        # System Information

        if WINDOWS:
            info = get_windows_information()
            self.label_info_os.setText(info[0])
            self.label_info_version.setText(info[1])
        elif MACOS:
            self.label_info_os.setText("Mac OS")
            self.label_info_version.setText("Unknown")
        elif LINUX:
            self.label_info_os.setText(get_linux_distro())
            self.label_info_version.setText(get_linux_kernel())
        else:
            self.label_info_os.setText("Unknown")
            self.label_info_version.setText("Unknown")

        self.label_info_arch.setText(get_architecture())

        # -------------------------------------------------------------
        # Set-up icons

        self.act_quit.setIcon(getIcon("application-exit"))
        #self.act_jack_start.setIcon(getIcon("media-playback-start"))
        #self.act_jack_stop.setIcon(getIcon("media-playback-stop"))
        #self.act_jack_clear_xruns.setIcon(getIcon("edit-clear"))
        #self.act_jack_configure.setIcon(getIcon("configure"))
        #self.act_a2j_start.setIcon(getIcon("media-playback-start"))
        #self.act_a2j_stop.setIcon(getIcon("media-playback-stop"))
        #self.act_pulse_start.setIcon(getIcon("media-playback-start"))
        #self.act_pulse_stop.setIcon(getIcon("media-playback-stop"))

        # -------------------------------------------------------------
        # Set-up systray

        self.systray = systray.GlobalSysTray(self, "Cadence", "cadence")
        self.systray.setToolTip("Cadence")
        self.systray.show()

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self.b_jack_start, SIGNAL("clicked()"), SLOT("slot_JackServerStart()"))
        self.connect(self.b_jack_stop, SIGNAL("clicked()"), SLOT("slot_JackServerStop()"))
        self.connect(self.b_jack_restart, SIGNAL("clicked()"), SLOT("slot_JackServerForceRestart()"))
        self.connect(self.b_jack_configure, SIGNAL("clicked()"), SLOT("slot_JackServerConfigure()"))

        self.connect(self.act_tools_catarina, SIGNAL("triggered()"), lambda tool="catarina": self.func_start_tool(tool))
        self.connect(self.act_tools_catia, SIGNAL("triggered()"), lambda tool="catia": self.func_start_tool(tool))
        self.connect(self.act_tools_claudia, SIGNAL("triggered()"), lambda tool="claudia": self.func_start_tool(tool))
        self.connect(self.act_tools_carla, SIGNAL("triggered()"), lambda tool="carla": self.func_start_tool(tool))
        self.connect(self.act_tools_logs, SIGNAL("triggered()"), lambda tool="cadence_logs": self.func_start_tool(tool))
        self.connect(self.act_tools_meter, SIGNAL("triggered()"), lambda tool="cadence_jackmeter": self.func_start_tool(tool))
        self.connect(self.act_tools_render, SIGNAL("triggered()"), lambda tool="cadence_render": self.func_start_tool(tool))
        self.connect(self.act_tools_xycontroller, SIGNAL("triggered()"), lambda tool="cadence_xycontroller": self.func_start_tool(tool))
        self.connect(self.pic_catia, SIGNAL("clicked()"), lambda tool="catia": self.func_start_tool(tool))
        self.connect(self.pic_claudia, SIGNAL("clicked()"), lambda tool="claudia": self.func_start_tool(tool))
        self.connect(self.pic_carla, SIGNAL("clicked()"), lambda tool="carla": self.func_start_tool(tool))
        self.connect(self.pic_logs, SIGNAL("clicked()"), lambda tool="cadence_logs": self.func_start_tool(tool))
        self.connect(self.pic_render, SIGNAL("clicked()"), lambda tool="cadence_render": self.func_start_tool(tool))
        self.connect(self.pic_xycontroller, SIGNAL("clicked()"), lambda tool="cadence_xycontroller": self.func_start_tool(tool))

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.m_timer120  = None
        self.m_timer1000 = self.startTimer(1000)

        self.DBusReconnect()

        if haveDBus:
            DBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword='dest', path_keyword='path',
                member_keyword='member', interface_keyword='interface', sender_keyword='sender', )

    def DBusReconnect(self):
        if haveDBus:
            try:
                DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
                jacksettings.initBus(DBus.bus)
            except:
                DBus.jack = None

            try:
                DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            except:
                DBus.a2j = None

        if DBus.jack:
            if DBus.jack.IsStarted():
                self.jackStarted()
            else:
                self.jackStopped()
                self.label_jack_realtime.setText("Yes" if jacksettings.isRealtime() else "No")
        else:
            self.jackStopped()
            self.label_jack_status.setText("Unavailable")
            self.label_jack_realtime.setText("Unknown")
            self.groupBox_jack.setEnabled(False)
            self.groupBox_jack.setTitle("-- jackdbus is not available --")
            self.b_jack_start.setEnabled(False)
            self.b_jack_stop.setEnabled(False)
            self.b_jack_restart.setEnabled(False)
            self.b_jack_configure.setEnabled(False)

    def DBusSignalReceiver(self, *args, **kwds):
        if kwds['interface'] == "org.freedesktop.DBus" and kwds['path'] == "/org/freedesktop/DBus" and kwds['member'] == "NameOwnerChanged":
            appInterface, appId, newId = args

            if not newId:
                # Something crashed
                if appInterface == "org.gna.home.a2jmidid":
                    QTimer.singleShot(0, self, SLOT("slot_handleCrash_a2j()"))
                elif appInterface == "org.jackaudio.service":
                    QTimer.singleShot(0, self, SLOT("slot_handleCrash_jack()"))

        elif kwds['interface'] == "org.jackaudio.JackControl":
            if DEBUG: print("org.jackaudio.JackControl", kwds['member'])
            if kwds['member'] == "ServerStarted":
                self.jackStarted()
            elif kwds['member'] == "ServerStopped":
                self.jackStopped()
        #elif kwds['interface'] == "org.gna.home.a2jmidid.control":
            #if kwds['member'] == "bridge_started":
                #self.a2jStarted()
            #elif kwds['member'] == "bridge_stopped":
                #self.a2jStopped()

    def jackStarted(self):
        self.m_last_dsp_load = DBus.jack.GetLoad()
        self.m_last_xruns    = DBus.jack.GetXruns()
        self.m_last_buffer_size = DBus.jack.GetBufferSize()

        self.b_jack_start.setEnabled(False)
        self.b_jack_stop.setEnabled(True)

        self.label_jack_status.setText("Started")
        #self.label_jack_status_ico.setPixmap("")

        if (DBus.jack.IsRealtime()):
            self.label_jack_realtime.setText("Yes")
            #self.label_jack_realtime_ico.setPixmap("")
        else:
            self.label_jack_realtime.setText("No")
            #self.label_jack_realtime_ico.setPixmap("")

        self.label_jack_dsp.setText("%.2f%%" % self.m_last_dsp_load)
        self.label_jack_xruns.setText(str(self.m_last_xruns))
        self.label_jack_bfsize.setText("%i samples" % self.m_last_buffer_size)
        self.label_jack_srate.setText("%i Hz" % DBus.jack.GetSampleRate())
        self.label_jack_latency.setText("%.1f ms" % DBus.jack.GetLatency())

        self.m_timer120 = self.startTimer(120)

    def jackStopped(self):
        if self.m_timer120:
            self.killTimer(self.m_timer120)
            self.m_timer120 = None

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.b_jack_start.setEnabled(True)
        self.b_jack_stop.setEnabled(False)

        self.label_jack_status.setText("Stopped")
        self.label_jack_dsp.setText("---")
        self.label_jack_xruns.setText("---")
        self.label_jack_bfsize.setText("---")
        self.label_jack_srate.setText("---")
        self.label_jack_latency.setText("---")

    def func_start_tool(self, tool):
        os.system("%s &" % tool)

    @pyqtSlot()
    def slot_JackServerStart(self):
        try:
            DBus.jack.StartServer()
        except:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to start JACK, please check the logs for more information."))

    @pyqtSlot()
    def slot_JackServerStop(self):
        try:
            DBus.jack.StopServer()
        except:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to stop JACK, please check the logs for more information."))

    @pyqtSlot()
    def slot_JackServerForceRestart(self):
        pass

    @pyqtSlot()
    def slot_JackServerConfigure(self):
        jacksettings.JackSettingsW(self).exec_()

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if DBus.jack:
            DBus.jack.ResetXruns()

    @pyqtSlot()
    def slot_handleCrash_a2j(self):
        pass

    @pyqtSlot()
    def slot_handleCrash_jack(self):
        self.DBusReconnect()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

        self.m_savedSettings = {
            "Main/UseSystemTray": self.settings.value("Main/UseSystemTray", True, type=bool),
            "Main/CloseToTray": self.settings.value("Main/CloseToTray", True, type=bool)
        }

    def timerEvent(self, event):
        if event.timerId() == self.m_timer120:
            if DBus.jack and self.m_last_dsp_load != None:
                next_dsp_load = DBus.jack.GetLoad()
                next_xruns    = DBus.jack.GetXruns()

                if self.m_last_dsp_load != next_dsp_load:
                    self.m_last_dsp_load = next_dsp_load
                    self.label_jack_dsp.setText("%.2f%%" % self.m_last_dsp_load)

                if self.m_last_xruns != next_xruns:
                    self.m_last_xruns = next_xruns
                    self.label_jack_xruns.setText(str(self.m_last_xruns))

        elif event.timerId() == self.m_timer1000:
            if DBus.jack and self.m_last_buffer_size != None:
                next_buffer_size = DBus.jack.GetBufferSize()

                if self.m_last_buffer_size != next_buffer_size:
                    self.m_last_buffer_size = next_buffer_size
                    self.label_jack_bfsize.setText("%i samples" % self.m_last_buffer_size)
                    self.label_jack_latency.setText("%.1f ms" % DBus.jack.GetLatency())

            else:
                self.update()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        self.systray.close()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Catia")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/catia.svg"))

    if haveDBus:
        DBus.loop = DBusQtMainLoop(set_as_default=True)
        DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)

    # Show GUI
    gui = CadenceMainW()

    # Set-up custom signal handling
    set_up_signals(gui)

    gui.show()

    # Exit properly
    sys.exit(app.exec_())

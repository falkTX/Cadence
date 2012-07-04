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

havePulseAudio = os.path.exists("/usr/bin/pulseaudio")

# ---------------------------------------------------------------------

DEFAULT_LADSPA_PATH = [
    os.path.join(HOME, ".ladspa"),
    os.path.join("/", "usr", "lib", "ladspa"),
    os.path.join("/", "usr", "local", "lib", "ladspa")
]

DEFAULT_DSSI_PATH = [
    os.path.join(HOME, ".dssi"),
    os.path.join("/", "usr", "lib", "dssi"),
    os.path.join("/", "usr", "local", "lib", "dssi")
]

DEFAULT_LV2_PATH = [
    os.path.join(HOME, ".lv2"),
    os.path.join("/", "usr", "lib", "lv2"),
    os.path.join("/", "usr", "local", "lib", "lv2")
]

DEFAULT_VST_PATH = [
    os.path.join(HOME, ".vst"),
    os.path.join("/", "usr", "lib", "vst"),
    os.path.join("/", "usr", "local", "lib", "vst")
]

DESKTOP_X_IMAGE = [
    "eog.desktop",
    "kde4/digikam.desktop",
    "kde4/gwenview.desktop"
]

DESKTOP_X_MUSIC = [
    "audacious.desktop",
    "clementine.desktop",
    "smplayer.desktop",
    "vlc.desktop",
    "kde4/amarok.desktop"
]

DESKTOP_X_VIDEO = [
    "smplayer.desktop",
    "vlc.desktop"
]

DESKTOP_X_TEXT = [
    "gedit.desktop",
    "kde4/kate.desktop",
    "kde4/kwrite.desktop"
]

DESKTOP_X_BROWSER = [
    "chrome.desktop",
    "firefox.desktop",
    "kde4/konqbrowser.desktop"
]

XDG_APPLICATIONS_PATH = [
    "/usr/share/applications",
    "/usr/local/share/applications"
]

WINEASIO_PREFIX = "HKEY_CURRENT_USER\Software\Wine\WineASIO"

# Global Settings
GlobalSettings = QSettings("Cadence", "GlobalSettings")

# ---------------------------------------------------------------------

def get_architecture():
    if LINUX:
        return os.uname()[4]
    elif WINDOWS:
        if sys.platform == "win32":
            return "32-bit"
        if sys.platform == "win64":
            return "64-bit"
    return "Unknown"

def get_linux_information():
    if os.path.exists("/etc/lsb-release"):
        distro = getoutput(". /etc/lsb-release && echo $DISTRIB_DESCRIPTION")
    elif os.path.exists("/etc/arch-release"):
        distro = "ArchLinux"
    else:
        distro = os.uname()[0]

    kernel = os.uname()[2]

    return (distro, kernel)

def get_mac_information():
    # TODO
    return ("Mac OS", "Unknown")

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

def isDesktopFileInstalled(desktop):
    for X_PATH in XDG_APPLICATIONS_PATH:
        if os.path.exists(os.path.join(X_PATH, desktop)):
            return True
    else:
        return False

def getXdgProperty(fileRead, key):
    fileReadSplit = fileRead.split(key, 1)

    if len(fileReadSplit) > 1:
        value = fileReadSplit[1].split(";\n", 1)[0].strip().replace("=", "", 1)
        return value

    return None

def searchAndSetComboBoxValue(comboBox, value):
    for i in range(comboBox.count()):
        if comboBox.itemText(i).replace("/","-") == value:
            comboBox.setCurrentIndex(i)
            comboBox.setEnabled(True)
            return True
    return False

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
        # Set-up GUI (System Information)

        if LINUX:
            info = get_linux_information()
        elif MACOS:
            info = get_mac_information()
        elif WINDOWS:
            info = get_windows_information()
        else:
            info = ("Unknown", "Unknown")

        self.label_info_os.setText(info[0])
        self.label_info_version.setText(info[1])
        self.label_info_arch.setText(get_architecture())

        # -------------------------------------------------------------
        # Set-up GUI (Tweaks)

        self.settings_changed_types = []
        self.frame_tweaks_settings.setVisible(False)

        for i in range(self.tw_tweaks.rowCount()):
            self.tw_tweaks.item(0, i).setTextAlignment(Qt.AlignCenter)

        self.tw_tweaks.setCurrentCell(0, 0)

        # -------------------------------------------------------------
        # Set-up GUI (Tweaks, Audio Plugins PATH)

        self.b_tweak_plugins_change.setEnabled(False)
        self.b_tweak_plugins_remove.setEnabled(False)

        for iPath in DEFAULT_LADSPA_PATH:
            self.list_LADSPA.addItem(iPath)

        for iPath in DEFAULT_DSSI_PATH:
            self.list_DSSI.addItem(iPath)

        for iPath in DEFAULT_LV2_PATH:
            self.list_LV2.addItem(iPath)

        for iPath in DEFAULT_VST_PATH:
            self.list_VST.addItem(iPath)

        EXTRA_LADSPA_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_LADSPA_PATH", "", type=str)
        EXTRA_DSSI_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_DSSI_PATH", "", type=str)
        EXTRA_LV2_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_LV2_PATH", "", type=str)
        EXTRA_VST_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_VST_PATH", "", type=str)

        for iPath in EXTRA_LADSPA_DIRS.split(":"):
            if os.path.exists(iPath):
                self.list_LADSPA.addItem(iPath)

        for iPath in EXTRA_DSSI_DIRS.split(":"):
            if os.path.exists(iPath):
                self.list_DSSI.addItem(iPath)

        for iPath in EXTRA_LV2_DIRS.split(":"):
            if os.path.exists(iPath):
                self.list_LV2.addItem(iPath)

        for iPath in EXTRA_VST_DIRS.split(":"):
            if os.path.exists(iPath):
                self.list_VST.addItem(iPath)

        self.list_LADSPA.sortItems(Qt.AscendingOrder)
        self.list_DSSI.sortItems(Qt.AscendingOrder)
        self.list_LV2.sortItems(Qt.AscendingOrder)
        self.list_VST.sortItems(Qt.AscendingOrder)

        self.list_LADSPA.setCurrentRow(0)
        self.list_DSSI.setCurrentRow(0)
        self.list_LV2.setCurrentRow(0)
        self.list_VST.setCurrentRow(0)

        # -------------------------------------------------------------
        # Set-up GUI (Tweaks, Default Applications)

        for desktop in DESKTOP_X_IMAGE:
            if isDesktopFileInstalled(desktop):
                self.cb_app_image.addItem(desktop)

        for desktop in DESKTOP_X_MUSIC:
            if isDesktopFileInstalled(desktop):
                self.cb_app_music.addItem(desktop)

        for desktop in DESKTOP_X_VIDEO:
            if isDesktopFileInstalled(desktop):
                self.cb_app_video.addItem(desktop)

        for desktop in DESKTOP_X_TEXT:
            if isDesktopFileInstalled(desktop):
                self.cb_app_text.addItem(desktop)

        for desktop in DESKTOP_X_BROWSER:
            if isDesktopFileInstalled(desktop):
                self.cb_app_browser.addItem(desktop)

        if self.cb_app_image.count() == 0:
            self.ch_app_image.setEnabled(False)

        if self.cb_app_music.count() == 0:
            self.ch_app_music.setEnabled(False)

        if self.cb_app_video.count() == 0:
            self.ch_app_video.setEnabled(False)

        if self.cb_app_text.count() == 0:
            self.ch_app_text.setEnabled(False)

        if self.cb_app_browser.count() == 0:
            self.ch_app_browser.setEnabled(False)

        mimeappsPath = os.path.join(HOME, ".local", "share", "applications", "mimeapps.list")

        if os.path.exists(mimeappsPath):
            fd = open(mimeappsPath, "r")
            mimeappsRead = fd.read()
            fd.close()

            x_image   = getXdgProperty(mimeappsRead, "image/x-bitmap")
            x_music   = getXdgProperty(mimeappsRead, "audio/x-wav")
            x_video   = getXdgProperty(mimeappsRead, "video/x-ogg")
            x_text    = getXdgProperty(mimeappsRead, "application/x-zerosize")
            x_browser = getXdgProperty(mimeappsRead, "text/html")

            if x_image and searchAndSetComboBoxValue(self.cb_app_image, x_image):
                self.ch_app_image.setChecked(True)

            if x_music and searchAndSetComboBoxValue(self.cb_app_music, x_music):
                self.ch_app_music.setChecked(True)

            if x_video and searchAndSetComboBoxValue(self.cb_app_video, x_video):
                self.ch_app_video.setChecked(True)

            if x_text and searchAndSetComboBoxValue(self.cb_app_text, x_text):
                self.ch_app_text.setChecked(True)

            if x_browser and searchAndSetComboBoxValue(self.cb_app_browser, x_browser):
                self.ch_app_browser.setChecked(True)

        else: # ~/.local/share/applications/mimeapps.list doesn't exist
            if not os.path.exists(os.path.join(HOME, ".local")):
                os.mkdir(os.path.join(HOME, ".local"))
            elif not os.path.exists(os.path.join(HOME, ".local", "share")):
                os.mkdir(os.path.join(HOME, ".local", "share"))
            elif not os.path.exists(os.path.join(HOME, ".local", "share", "applications")):
                os.mkdir(os.path.join(HOME, ".local", "share", "applications"))

        # -------------------------------------------------------------
        # Set-up GUI (Tweaks, WineASIO)

        # ...

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

        self.connect(self.b_tweaks_apply_now, SIGNAL("clicked()"), SLOT("slot_tweaksApply()"))

        self.connect(self.b_tweak_plugins_add, SIGNAL("clicked()"), SLOT("slot_tweakPluginAdd()"))
        self.connect(self.b_tweak_plugins_change, SIGNAL("clicked()"), SLOT("slot_tweakPluginChange()"))
        self.connect(self.b_tweak_plugins_remove, SIGNAL("clicked()"), SLOT("slot_tweakPluginRemove()"))
        self.connect(self.b_tweak_plugins_reset, SIGNAL("clicked()"), SLOT("slot_tweakPluginReset()"))
        self.connect(self.tb_tweak_plugins, SIGNAL("currentChanged(int)"), SLOT("slot_tweakPluginTypeChanged(int)"))
        self.connect(self.list_LADSPA, SIGNAL("currentRowChanged(int)"), SLOT("slot_tweakPluginsLadspaRowChanged(int)"))
        self.connect(self.list_DSSI, SIGNAL("currentRowChanged(int)"), SLOT("slot_tweakPluginsDssiRowChanged(int)"))
        self.connect(self.list_LV2, SIGNAL("currentRowChanged(int)"), SLOT("slot_tweakPluginsLv2RowChanged(int)"))
        self.connect(self.list_VST, SIGNAL("currentRowChanged(int)"), SLOT("slot_tweakPluginsVstRowChanged(int)"))

        #self.connect(self.ch_image, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.ch_music, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.ch_video, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.ch_text, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.ch_office, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.ch_browser, SIGNAL("clicked()"), self.func_settings_changed_apps)
        #self.connect(self.cb_app_image, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_image)
        #self.connect(self.cb_app_music, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_music)
        #self.connect(self.cb_app_video, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_video)
        #self.connect(self.cb_app_text, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_text)
        #self.connect(self.cb_app_office, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_office)
        #self.connect(self.cb_app_browser, SIGNAL("currentIndexChanged(int)"), self.func_app_changed_browser)

        # -------------------------------------------------------------

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

    def func_settings_changed(self, stype):
        if stype not in self.settings_changed_types:
            self.settings_changed_types.append(stype)
        self.frame_tweaks_settings.setVisible(True)

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
        jacksettingsW = jacksettings.JackSettingsW(self)
        jacksettingsW.exec_()
        del jacksettingsW

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

    @pyqtSlot()
    def slot_tweaksApply(self):
        if "plugins" in self.settings_changed_types:
            EXTRA_LADSPA_DIRS = []
            EXTRA_DSSI_DIRS = []
            EXTRA_LV2_DIRS = []
            EXTRA_VST_DIRS = []

            for i in range(self.list_LADSPA.count()):
                iPath = self.list_LADSPA.item(i).text()
                if iPath not in DEFAULT_LADSPA_PATH and iPath not in EXTRA_LADSPA_DIRS:
                    EXTRA_LADSPA_DIRS.append(iPath)

            for i in range(self.list_DSSI.count()):
                iPath = self.list_DSSI.item(i).text()
                if iPath not in DEFAULT_DSSI_PATH and iPath not in EXTRA_DSSI_DIRS:
                    EXTRA_DSSI_DIRS.append(iPath)

            for i in range(self.list_LV2.count()):
                iPath = self.list_LV2.item(i).text()
                if iPath not in DEFAULT_LV2_PATH and iPath not in EXTRA_LV2_DIRS:
                    EXTRA_LV2_DIRS.append(iPath)

            for i in range(self.list_VST.count()):
                iPath = self.list_VST.item(i).text()
                if iPath not in DEFAULT_VST_PATH and iPath not in EXTRA_VST_DIRS:
                    EXTRA_VST_DIRS.append(iPath)

            GlobalSettings.setValue("AudioPlugins/EXTRA_LADSPA_PATH", ":".join(EXTRA_LADSPA_DIRS))
            GlobalSettings.setValue("AudioPlugins/EXTRA_DSSI_PATH", ":".join(EXTRA_DSSI_DIRS))
            GlobalSettings.setValue("AudioPlugins/EXTRA_LV2_PATH", ":".join(EXTRA_LV2_DIRS))
            GlobalSettings.setValue("AudioPlugins/EXTRA_VST_PATH", ":".join(EXTRA_VST_DIRS))

        self.settings_changed_types = []
        self.frame_tweaks_settings.setVisible(False)

    @pyqtSlot()
    def slot_tweaksSettingsChanged_apps(self):
        self.func_settings_changed("apps")

    @pyqtSlot()
    def slot_tweaksSettingsChanged_wineasio(self):
        self.func_settings_changed("wineasio")

    @pyqtSlot()
    def slot_tweakPluginAdd(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        if self.tb_tweak_plugins.currentIndex() == 0:
            self.list_LADSPA.addItem(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 1:
            self.list_DSSI.addItem(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 2:
            self.list_LV2.addItem(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 3:
            self.list_VST.addItem(newPath)

        self.func_settings_changed("plugins")

    @pyqtSlot()
    def slot_tweakPluginChange(self):
        if self.tb_tweak_plugins.currentIndex() == 0:
            curPath = self.list_LADSPA.item(self.list_LADSPA.currentRow()).text()
        elif self.tb_tweak_plugins.currentIndex() == 1:
            curPath = self.list_DSSI.item(self.list_DSSI.currentRow()).text()
        elif self.tb_tweak_plugins.currentIndex() == 2:
            curPath = self.list_LV2.item(self.list_LV2.currentRow()).text()
        elif self.tb_tweak_plugins.currentIndex() == 3:
            curPath = self.list_VST.item(self.list_VST.currentRow()).text()
        else:
            curPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Change Path"), curPath, QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        if self.tb_tweak_plugins.currentIndex() == 0:
          self.list_LADSPA.item(self.list_LADSPA.currentRow()).setText(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 1:
          self.list_DSSI.item(self.list_DSSI.currentRow()).setText(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 2:
          self.list_LV2.item(self.list_LV2.currentRow()).setText(newPath)
        elif self.tb_tweak_plugins.currentIndex() == 3:
          self.list_VST.item(self.list_VST.currentRow()).setText(newPath)

        self.func_settings_changed("plugins")

    @pyqtSlot()
    def slot_tweakPluginRemove(self):
        if self.tb_tweak_plugins.currentIndex() == 0:
          self.list_LADSPA.takeItem(self.list_LADSPA.currentRow())
        elif self.tb_tweak_plugins.currentIndex() == 1:
          self.list_DSSI.takeItem(self.list_DSSI.currentRow())
        elif self.tb_tweak_plugins.currentIndex() == 2:
          self.list_LV2.takeItem(self.list_LV2.currentRow())
        elif self.tb_tweak_plugins.currentIndex() == 3:
          self.list_VST.takeItem(self.list_VST.currentRow())

        self.func_settings_changed("plugins")

    @pyqtSlot()
    def slot_tweakPluginReset(self):
        if self.tb_tweak_plugins.currentIndex() == 0:
            self.list_LADSPA.clear()

            for iPath in DEFAULT_LADSPA_PATH:
                self.list_LADSPA.addItem(iPath)

        elif self.tb_tweak_plugins.currentIndex() == 1:
            self.list_DSSI.clear()

            for iPath in DEFAULT_DSSI_PATH:
                self.list_DSSI.addItem(iPath)

        elif self.tb_tweak_plugins.currentIndex() == 2:
            self.list_LV2.clear()

            for iPath in DEFAULT_LV2_PATH:
                self.list_LV2.addItem(iPath)

        elif self.tb_tweak_plugins.currentIndex() == 3:
            self.list_VST.clear()

            for iPath in DEFAULT_VST_PATH:
                self.list_VST.addItem(iPath)

        self.func_settings_changed("plugins")

    @pyqtSlot(int)
    def slot_tweakPluginTypeChanged(self, index):
        if index == 0:
            self.list_LADSPA.setCurrentRow(-1)
            self.list_LADSPA.setCurrentRow(0)
        elif index == 1:
            self.list_DSSI.setCurrentRow(-1)
            self.list_DSSI.setCurrentRow(0)
        elif index == 2:
            self.list_LV2.setCurrentRow(-1)
            self.list_LV2.setCurrentRow(0)
        elif index == 3:
            self.list_VST.setCurrentRow(-1)
            self.list_VST.setCurrentRow(0)

    @pyqtSlot(int)
    def slot_tweakPluginsLadspaRowChanged(self, index):
        nonRemovable = (index >= 0 and self.list_LADSPA.item(index).text() not in DEFAULT_LADSPA_PATH)
        self.b_tweak_plugins_change.setEnabled(nonRemovable)
        self.b_tweak_plugins_remove.setEnabled(nonRemovable)

    @pyqtSlot(int)
    def slot_tweakPluginsDssiRowChanged(self, index):
        nonRemovable = (index >= 0 and self.list_DSSI.item(index).text() not in DEFAULT_DSSI_PATH)
        self.b_tweak_plugins_change.setEnabled(nonRemovable)
        self.b_tweak_plugins_remove.setEnabled(nonRemovable)

    @pyqtSlot(int)
    def slot_tweakPluginsLv2RowChanged(self, index):
        nonRemovable = (index >= 0 and self.list_LV2.item(index).text() not in DEFAULT_LV2_PATH)
        self.b_tweak_plugins_change.setEnabled(nonRemovable)
        self.b_tweak_plugins_remove.setEnabled(nonRemovable)

    @pyqtSlot(int)
    def slot_tweakPluginsVstRowChanged(self, index):
        nonRemovable = (index >= 0 and self.list_VST.item(index).text() not in DEFAULT_VST_PATH)
        self.b_tweak_plugins_change.setEnabled(nonRemovable)
        self.b_tweak_plugins_remove.setEnabled(nonRemovable)

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

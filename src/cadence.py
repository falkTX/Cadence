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

# FIXME - py3 later
try:
    from commands import getoutput
except:
    from subprocess import getoutput

# Imports (Global)
from platform import architecture
from PyQt4.QtGui import QApplication, QLabel, QMainWindow, QSizePolicy

# Imports (Custom Stuff)
import ui_cadence
import systray
from shared_cadence import *
from shared_jack import *
from shared_settings import *

try:
    import dbus
    from dbus.mainloop.qt import DBusQtMainLoop
    haveDBus = True
except:
    haveDBus = False

havePulseAudio = os.path.exists("/usr/bin/pulseaudio")
haveWine       = os.path.exists("/usr/bin/regedit")

if haveWine:
    WINEPREFIX = os.getenv("WINEPREFIX")
    if not WINEPREFIX:
        WINEPREFIX = os.path.join(HOME, ".wine")

# ---------------------------------------------------------------------

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

# ---------------------------------------------------------------------

def get_architecture():
    return architecture()[0]

def get_haiku_information():
    # TODO
    return ("Haiku OS", "Unknown")

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
    return False

def getDesktopFileContents(desktop):
    for X_PATH in XDG_APPLICATIONS_PATH:
        if os.path.exists(os.path.join(X_PATH, desktop)):
            fd = open(os.path.join(X_PATH, desktop), "r")
            contents = fd.read()
            fd.close()
            return contents
    return None

def getXdgProperty(fileRead, key):
    fileReadSplit = fileRead.split(key, 1)

    if len(fileReadSplit) > 1:
        fileReadLine         = fileReadSplit[1].split("\n",1)[0]
        fileReadLineStripped = fileReadLine.rsplit(";",1)[0].strip()
        value = fileReadLineStripped.replace("=","",1)
        return value

    return None

def getWineAsioKeyValue(key, default):
  wineFile = os.path.join(WINEPREFIX, "user.reg")

  if not os.path.exists(wineFile):
      return default

  wineDumpF = open(wineFile, "r")
  wineDump  = wineDumpF.read()
  wineDumpF.close()

  wineDumpSplit = wineDump.split("[Software\\\\Wine\\\\WineASIO]")

  if len(wineDumpSplit) <= 1:
      return default

  wineDumpSmall = wineDumpSplit[1].split("[")[0]
  keyDumpSplit  = wineDumpSmall.split('"%s"' % key)

  if len(keyDumpSplit) <= 1:
      return default

  keyDumpSmall = keyDumpSplit[1].split(":")[1].split("\n")[0]
  return keyDumpSmall

def searchAndSetComboBoxValue(comboBox, value):
    for i in range(comboBox.count()):
        if comboBox.itemText(i).replace("/","-") == value:
            comboBox.setCurrentIndex(i)
            comboBox.setEnabled(True)
            return True
    return False

def smartHex(value, length):
  hexStr = hex(value).replace("0x","")

  if len(hexStr) < length:
      zeroCount = length - len(hexStr)
      hexStr = "%s%s" % ("0"*zeroCount, hexStr)

  return hexStr

# ---------------------------------------------------------------------

cadenceSystemChecks = []

class CadenceSystemCheck(object):
    ICON_ERROR = 0
    ICON_WARN  = 1
    ICON_OK    = 2

    def __init__(self):
        object.__init__(self)

        self.name   = self.tr("check")
        self.icon   = self.ICON_OK
        self.result = self.tr("yes")

        self.moreInfo = self.tr("nothing to report")

    def tr(self, text):
        return app.translate("CadenceSystemCheck", text)

class CadenceSystemCheck_audioGroup(CadenceSystemCheck):
    def __init__(self):
        CadenceSystemCheck.__init__(self)

        self.name = self.tr("User in audio group")

        user   = getoutput("whoami").strip()
        groups = getoutput("groups").strip().split(" ")

        if "audio" in groups:
            self.icon     = self.ICON_OK
            self.result   = self.tr("Yes")
            self.moreInfo = None

        else:
            fd = open("/etc/group", "r")
            groupRead = fd.read().strip().split("\n")
            fd.close()

            onAudioGroup = False
            for lineRead in groupRead:
                if lineRead.startswith("audio:"):
                    groups = lineRead.split(":")[-1].split(",")
                    if user in groups:
                        onAudioGroup = True
                    break

            if onAudioGroup:
                self.icon     = self.ICON_WARN
                self.result   = self.tr("Yes, but needs relogin")
                self.moreInfo = None
            else:
                self.icon     = self.ICON_ERROR
                self.result   = self.tr("No")
                self.moreInfo = None

class CadenceSystemCheck_kernel(CadenceSystemCheck):
    def __init__(self):
        CadenceSystemCheck.__init__(self)

        self.name = self.tr("Current kernel")

        uname3 = os.uname()[2]

        versionInt   = []
        versionStr   = uname3.split("-",1)[0]
        versionSplit = versionStr.split(".")

        for split in versionSplit:
            if split.isdigit():
                versionInt.append(int(split))
            else:
                versionInt = [0, 0, 0]
                break

        self.result = versionStr + " "

        if "-" not in uname3:
            self.icon     = self.ICON_WARN
            self.result  += self.tr("Vanilla")
            self.moreInfo = None

        else:
            if uname3.endswith("-pae"):
                kernelType   = uname3.split("-")[-2].lower()
                self.result += kernelType.title() + " (PAE)"
            else:
                kernelType   = uname3.split("-")[-1].lower()
                self.result += kernelType.title()

            if kernelType in ("rt", "realtime") or (kernelType == "lowlatency" and versionInt >= [2, 6, 39]):
                self.icon     = self.ICON_OK
                self.moreInfo = None
            elif versionInt >= [2, 6, 39]:
                self.icon     = self.ICON_WARN
                self.moreInfo = None
            else:
                self.icon     = self.ICON_ERROR
                self.moreInfo = None

def initSystemChecks():
    if LINUX:
        cadenceSystemChecks.append(CadenceSystemCheck_kernel())
        cadenceSystemChecks.append(CadenceSystemCheck_audioGroup())

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

        self.pix_apply   = QIcon(getIcon("dialog-ok-apply", 16)).pixmap(16, 16)
        self.pix_cancel  = QIcon(getIcon("dialog-cancel", 16)).pixmap(16, 16)
        self.pix_error   = QIcon(getIcon("dialog-error", 16)).pixmap(16, 16)
        self.pix_warning = QIcon(getIcon("dialog-warning", 16)).pixmap(16, 16)

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

        if HAIKU:
            info = get_haiku_information()
        elif LINUX:
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
        # Set-up GUI (System Checks)

        #self.label_check_helper1.setVisible(False)
        #self.label_check_helper2.setVisible(False)
        #self.label_check_helper3.setVisible(False)

        index = 2
        checksLayout = self.groupBox_checks.layout()

        for check in cadenceSystemChecks:
            widgetName   = QLabel("%s:" % check.name)
            widgetIcon   = QLabel("")
            widgetResult = QLabel(check.result)

            if check.moreInfo:
                widgetName.setToolTip(check.moreInfo)
                widgetIcon.setToolTip(check.moreInfo)
                widgetResult.setToolTip(check.moreInfo)

            #widgetName.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
            #widgetIcon.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
            #widgetIcon.setMinimumSize(16, 16)
            #widgetIcon.setMaximumSize(16, 16)
            #widgetResult.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)

            if check.icon == check.ICON_ERROR:
                widgetIcon.setPixmap(self.pix_error)
            elif check.icon == check.ICON_WARN:
                widgetIcon.setPixmap(self.pix_warning)
            elif check.icon == check.ICON_OK:
                widgetIcon.setPixmap(self.pix_apply)
            else:
                widgetIcon.setPixmap(self.pix_cancel)

            checksLayout.addWidget(widgetName, index, 0, Qt.AlignRight)
            checksLayout.addWidget(widgetIcon, index, 1, Qt.AlignHCenter)
            checksLayout.addWidget(widgetResult, index, 2, Qt.AlignLeft)

            index += 1

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

            x_image   = getXdgProperty(mimeappsRead, "image/bmp")
            x_music   = getXdgProperty(mimeappsRead, "audio/wav")
            x_video   = getXdgProperty(mimeappsRead, "video/webm")
            x_text    = getXdgProperty(mimeappsRead, "text/plain")
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

        if haveWine:
            ins  = int(getWineAsioKeyValue("Number of inputs", "00000010"), 16)
            outs = int(getWineAsioKeyValue("Number of outputs", "00000010"), 16)
            hw   = bool(int(getWineAsioKeyValue("Connect to hardware", "00000001"), 10))

            autostart    = bool(int(getWineAsioKeyValue("Autostart server", "00000000"), 10))
            fixed_bsize  = bool(int(getWineAsioKeyValue("Fixed buffersize", "00000001"), 10))
            prefer_bsize = int(getWineAsioKeyValue("Preferred buffersize", "00000400"), 16)

            for bsize in buffer_sizes:
                self.cb_wineasio_bsizes.addItem(str(bsize))
                if bsize == prefer_bsize:
                    self.cb_wineasio_bsizes.setCurrentIndex(self.cb_wineasio_bsizes.count()-1)

            self.sb_wineasio_ins.setValue(ins)
            self.sb_wineasio_outs.setValue(outs)
            self.cb_wineasio_hw.setChecked(hw)

            self.cb_wineasio_autostart.setChecked(autostart)
            self.cb_wineasio_fixed_bsize.setChecked(fixed_bsize)

        else:
            # No Wine
            self.tw_tweaks.hideRow(2)

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
        self.connect(self.act_tools_logs, SIGNAL("triggered()"), lambda tool="cadence-logs": self.func_start_tool(tool))
        self.connect(self.act_tools_meter, SIGNAL("triggered()"), lambda tool="cadence-jackmeter": self.func_start_tool(tool))
        self.connect(self.act_tools_render, SIGNAL("triggered()"), lambda tool="cadence-render": self.func_start_tool(tool))
        self.connect(self.act_tools_xycontroller, SIGNAL("triggered()"), lambda tool="cadence-xycontroller": self.func_start_tool(tool))
        self.connect(self.pic_catia, SIGNAL("clicked()"), lambda tool="catia": self.func_start_tool(tool))
        self.connect(self.pic_claudia, SIGNAL("clicked()"), lambda tool="claudia": self.func_start_tool(tool))
        self.connect(self.pic_carla, SIGNAL("clicked()"), lambda tool="carla": self.func_start_tool(tool))
        self.connect(self.pic_logs, SIGNAL("clicked()"), lambda tool="cadence-logs": self.func_start_tool(tool))
        self.connect(self.pic_render, SIGNAL("clicked()"), lambda tool="cadence-render": self.func_start_tool(tool))
        self.connect(self.pic_xycontroller, SIGNAL("clicked()"), lambda tool="cadence-xycontroller": self.func_start_tool(tool))

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

        self.connect(self.ch_app_image, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_apps()"))
        self.connect(self.cb_app_image, SIGNAL("highlighted(int)"), SLOT("slot_tweakAppImageHighlighted(int)"))
        self.connect(self.cb_app_image, SIGNAL("currentIndexChanged(int)"), SLOT("slot_tweakAppImageChanged(int)"))
        self.connect(self.ch_app_music, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_apps()"))
        self.connect(self.cb_app_music, SIGNAL("highlighted(int)"), SLOT("slot_tweakAppMusicHighlighted(int)"))
        self.connect(self.cb_app_music, SIGNAL("currentIndexChanged(int)"), SLOT("slot_tweakAppMusicChanged(int)"))
        self.connect(self.ch_app_video, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_apps()"))
        self.connect(self.cb_app_video, SIGNAL("highlighted(int)"), SLOT("slot_tweakAppVideoHighlighted(int)"))
        self.connect(self.cb_app_video, SIGNAL("currentIndexChanged(int)"), SLOT("slot_tweakAppVideoChanged(int)"))
        self.connect(self.ch_app_text, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_apps()"))
        self.connect(self.cb_app_text, SIGNAL("highlighted(int)"), SLOT("slot_tweakAppTextHighlighted(int)"))
        self.connect(self.cb_app_text, SIGNAL("currentIndexChanged(int)"), SLOT("slot_tweakAppTextChanged(int)"))
        self.connect(self.ch_app_browser, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_apps()"))
        self.connect(self.cb_app_browser, SIGNAL("highlighted(int)"), SLOT("slot_tweakAppBrowserHighlighted(int)"))
        self.connect(self.cb_app_browser, SIGNAL("currentIndexChanged(int)"),SLOT("slot_tweakAppBrowserChanged(int)"))

        self.connect(self.sb_wineasio_ins, SIGNAL("valueChanged(int)"), SLOT("slot_tweaksSettingsChanged_wineasio()"))
        self.connect(self.sb_wineasio_outs, SIGNAL("valueChanged(int)"), SLOT("slot_tweaksSettingsChanged_wineasio()"))
        self.connect(self.cb_wineasio_hw, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_wineasio()"))
        self.connect(self.cb_wineasio_autostart, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_wineasio()"))
        self.connect(self.cb_wineasio_fixed_bsize, SIGNAL("clicked()"), SLOT("slot_tweaksSettingsChanged_wineasio()"))
        self.connect(self.cb_wineasio_bsizes, SIGNAL("currentIndexChanged(int)"), SLOT("slot_tweaksSettingsChanged_wineasio()"))

        # -------------------------------------------------------------

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.m_timer250  = None
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
            self.label_jack_status_ico.setPixmap(self.pix_error)
            self.label_jack_realtime.setText("Unknown")
            self.label_jack_realtime_ico.setPixmap(self.pix_error)
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
        self.label_jack_status_ico.setPixmap(self.pix_apply)

        if DBus.jack.IsRealtime():
            self.label_jack_realtime.setText("Yes")
            self.label_jack_realtime_ico.setPixmap(self.pix_apply)
        else:
            self.label_jack_realtime.setText("No")
            self.label_jack_realtime_ico.setPixmap(self.pix_cancel)

        self.label_jack_dsp.setText("%.2f%%" % self.m_last_dsp_load)
        self.label_jack_xruns.setText(str(self.m_last_xruns))
        self.label_jack_bfsize.setText("%i samples" % self.m_last_buffer_size)
        self.label_jack_srate.setText("%i Hz" % DBus.jack.GetSampleRate())
        self.label_jack_latency.setText("%.1f ms" % DBus.jack.GetLatency())

        self.m_timer250 = self.startTimer(250)

    def jackStopped(self):
        if self.m_timer250:
            self.killTimer(self.m_timer250)
            self.m_timer250 = None

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.b_jack_start.setEnabled(True)
        self.b_jack_stop.setEnabled(False)

        self.label_jack_status.setText("Stopped")
        self.label_jack_status_ico.setPixmap(self.pix_cancel)

        self.label_jack_dsp.setText("---")
        self.label_jack_xruns.setText("---")
        self.label_jack_bfsize.setText("---")
        self.label_jack_srate.setText("---")
        self.label_jack_latency.setText("---")

    def setAppDetails(self, desktop):
        appContents = getDesktopFileContents(desktop)
        name    = getXdgProperty(appContents, "Name")
        icon    = getXdgProperty(appContents, "Icon")
        comment = getXdgProperty(appContents, "Comment")

        if not name:
            name = self.cb_app_image.currentText().replace(".desktop","").title()
        if not icon:
            icon = ""
        if not comment:
           comment = ""

        self.ico_app.setPixmap(getIcon(icon, 48).pixmap(48, 48))
        self.label_app_name.setText(name)
        self.label_app_comment.setText(comment)

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

        if "apps" in self.settings_changed_types:
            mimeFileContent = ""

            # Fix common mime errors
            mimeFileContent += "application/x-designer=designer-qt4.desktop;\n"
            mimeFileContent += "application/x-ms-dos-executable=wine.desktop;\n"
            mimeFileContent += "audio/x-minipsf=audacious.desktop;\n"
            mimeFileContent += "audio/x-psf=audacious.desktop;\n"

            if self.ch_app_image.isChecked():
                imageApp = self.cb_app_image.currentText().replace("/","-")
                mimeFileContent += "image/bmp=%s;\n" % imageApp
                mimeFileContent += "image/gif=%s;\n" % imageApp
                mimeFileContent += "image/jp2=%s;\n" % imageApp
                mimeFileContent += "image/jpeg=%s;\n" % imageApp
                mimeFileContent += "image/png=%s;\n" % imageApp
                mimeFileContent += "image/svg+xml=%s;\n" % imageApp
                mimeFileContent += "image/svg+xml-compressed=%s;\n" % imageApp
                mimeFileContent += "image/tiff=%s;\n" % imageApp
                mimeFileContent += "image/x-canon-cr2=%s;\n" % imageApp
                mimeFileContent += "image/x-canon-crw=%s;\n" % imageApp
                mimeFileContent += "image/x-eps=%s;\n" % imageApp
                mimeFileContent += "image/x-kodak-dcr=%s;\n" % imageApp
                mimeFileContent += "image/x-kodak-k25=%s;\n" % imageApp
                mimeFileContent += "image/x-kodak-kdc=%s;\n" % imageApp
                mimeFileContent += "image/x-nikon-nef=%s;\n" % imageApp
                mimeFileContent += "image/x-olympus-orf=%s;\n" % imageApp
                mimeFileContent += "image/x-panasonic-raw=%s;\n" % imageApp
                mimeFileContent += "image/x-pcx=%s;\n" % imageApp
                mimeFileContent += "image/x-pentax-pef=%s;\n" % imageApp
                mimeFileContent += "image/x-portable-anymap=%s;\n" % imageApp
                mimeFileContent += "image/x-portable-bitmap=%s;\n" % imageApp
                mimeFileContent += "image/x-portable-graymap=%s;\n" % imageApp
                mimeFileContent += "image/x-portable-pixmap=%s;\n" % imageApp
                mimeFileContent += "image/x-sony-arw=%s;\n" % imageApp
                mimeFileContent += "image/x-sony-sr2=%s;\n" % imageApp
                mimeFileContent += "image/x-sony-srf=%s;\n" % imageApp
                mimeFileContent += "image/x-tga=%s;\n" % imageApp
                mimeFileContent += "image/x-xbitmap=%s;\n" % imageApp
                mimeFileContent += "image/x-xpixmap=%s;\n" % imageApp

            if self.ch_app_music.isChecked():
                musicApp = self.cb_app_music.currentText().replace("/","-")
                mimeFileContent += "application/vnd.apple.mpegurl=%s;\n" % musicApp
                mimeFileContent += "application/xspf+xml=%s;\n" % musicApp
                mimeFileContent += "application/x-smaf=%s;\n" % musicApp
                mimeFileContent += "audio/AMR=%s;\n" % musicApp
                mimeFileContent += "audio/AMR-WB=%s;\n" % musicApp
                mimeFileContent += "audio/aac=%s;\n" % musicApp
                mimeFileContent += "audio/ac3=%s;\n" % musicApp
                mimeFileContent += "audio/basic=%s;\n" % musicApp
                mimeFileContent += "audio/flac=%s;\n" % musicApp
                mimeFileContent += "audio/m3u=%s;\n" % musicApp
                mimeFileContent += "audio/mp2=%s;\n" % musicApp
                mimeFileContent += "audio/mp4=%s;\n" % musicApp
                mimeFileContent += "audio/mpeg=%s;\n" % musicApp
                mimeFileContent += "audio/ogg=%s;\n" % musicApp
                mimeFileContent += "audio/vnd.rn-realaudio=%s;\n" % musicApp
                mimeFileContent += "audio/vorbis=%s;\n" % musicApp
                mimeFileContent += "audio/webm=%s;\n" % musicApp
                mimeFileContent += "audio/wav=%s;\n" % musicApp
                mimeFileContent += "audio/x-adpcm=%s;\n" % musicApp
                mimeFileContent += "audio/x-aifc=%s;\n" % musicApp
                mimeFileContent += "audio/x-aiff=%s;\n" % musicApp
                mimeFileContent += "audio/x-aiffc=%s;\n" % musicApp
                mimeFileContent += "audio/x-ape=%s;\n" % musicApp
                mimeFileContent += "audio/x-cda=%s;\n" % musicApp
                mimeFileContent += "audio/x-flac=%s;\n" % musicApp
                mimeFileContent += "audio/x-flac+ogg=%s;\n" % musicApp
                mimeFileContent += "audio/x-gsm=%s;\n" % musicApp
                mimeFileContent += "audio/x-m4b=%s;\n" % musicApp
                mimeFileContent += "audio/x-matroska=%s;\n" % musicApp
                mimeFileContent += "audio/x-mp2=%s;\n" % musicApp
                mimeFileContent += "audio/x-mpegurl=%s;\n" % musicApp
                mimeFileContent += "audio/x-ms-asx=%s;\n" % musicApp
                mimeFileContent += "audio/x-ms-wma=%s;\n" % musicApp
                mimeFileContent += "audio/x-musepack=%s;\n" % musicApp
                mimeFileContent += "audio/x-ogg=%s;\n" % musicApp
                mimeFileContent += "audio/x-oggflac=%s;\n" % musicApp
                mimeFileContent += "audio/x-pn-realaudio-plugin=%s;\n" % musicApp
                mimeFileContent += "audio/x-riff=%s;\n" % musicApp
                mimeFileContent += "audio/x-scpls=%s;\n" % musicApp
                mimeFileContent += "audio/x-speex=%s;\n" % musicApp
                mimeFileContent += "audio/x-speex+ogg=%s;\n" % musicApp
                mimeFileContent += "audio/x-tta=%s;\n" % musicApp
                mimeFileContent += "audio/x-vorbis+ogg=%s;\n" % musicApp
                mimeFileContent += "audio/x-wav=%s;\n" % musicApp
                mimeFileContent += "audio/x-wavpack=%s;\n" % musicApp

            if self.ch_app_video.isChecked():
                videoApp = self.cb_app_video.currentText().replace("/","-")
                mimeFileContent +="application/mxf=%s;\n" % videoApp
                mimeFileContent +="application/ogg=%s;\n" % videoApp
                mimeFileContent +="application/ram=%s;\n" % videoApp
                mimeFileContent +="application/vnd.ms-asf=%s;\n" % videoApp
                mimeFileContent +="application/vnd.ms-wpl=%s;\n" % videoApp
                mimeFileContent +="application/vnd.rn-realmedia=%s;\n" % videoApp
                mimeFileContent +="application/x-ms-wmp=%s;\n" % videoApp
                mimeFileContent +="application/x-ms-wms=%s;\n" % videoApp
                mimeFileContent +="application/x-netshow-channel=%s;\n" % videoApp
                mimeFileContent +="application/x-ogg=%s;\n" % videoApp
                mimeFileContent +="application/x-quicktime-media-link=%s;\n" % videoApp
                mimeFileContent +="video/3gpp=%s;\n" % videoApp
                mimeFileContent +="video/3gpp2=%s;\n" % videoApp
                mimeFileContent +="video/divx=%s;\n" % videoApp
                mimeFileContent +="video/dv=%s;\n" % videoApp
                mimeFileContent +="video/flv=%s;\n" % videoApp
                mimeFileContent +="video/mp2t=%s;\n" % videoApp
                mimeFileContent +="video/mp4=%s;\n" % videoApp
                mimeFileContent +="video/mpeg=%s;\n" % videoApp
                mimeFileContent +="video/ogg=%s;\n" % videoApp
                mimeFileContent +="video/quicktime=%s;\n" % videoApp
                mimeFileContent +="video/vivo=%s;\n" % videoApp
                mimeFileContent +="video/vnd.rn-realvideo=%s;\n" % videoApp
                mimeFileContent +="video/webm=%s;\n" % videoApp
                mimeFileContent +="video/x-anim=%s;\n" % videoApp
                mimeFileContent +="video/x-flic=%s;\n" % videoApp
                mimeFileContent +="video/x-flv=%s;\n" % videoApp
                mimeFileContent +="video/x-m4v=%s;\n" % videoApp
                mimeFileContent +="video/x-matroska=%s;\n" % videoApp
                mimeFileContent +="video/x-ms-asf=%s;\n" % videoApp
                mimeFileContent +="video/x-ms-wm=%s;\n" % videoApp
                mimeFileContent +="video/x-ms-wmp=%s;\n" % videoApp
                mimeFileContent +="video/x-ms-wmv=%s;\n" % videoApp
                mimeFileContent +="video/x-ms-wvx=%s;\n" % videoApp
                mimeFileContent +="video/x-msvideo=%s;\n" % videoApp
                mimeFileContent +="video/x-nsv=%s;\n" % videoApp
                mimeFileContent +="video/x-ogg=%s;\n" % videoApp
                mimeFileContent +="video/x-ogm=%s;\n" % videoApp
                mimeFileContent +="video/x-ogm+ogg=%s;\n" % videoApp
                mimeFileContent +="video/x-theora=%s;\n" % videoApp
                mimeFileContent +="video/x-theora+ogg=%s;\n" % videoApp
                mimeFileContent +="video/x-wmv=%s;\n" % videoApp

            if self.ch_app_text.isChecked():
                # TODO - more mimetypes
                textApp = self.cb_app_text.currentText().replace("/","-")
                mimeFileContent +="application/rdf+xml=%s;\n" % textApp
                mimeFileContent +="application/xml=%s;\n" % textApp
                mimeFileContent +="application/xml-dtd=%s;\n" % textApp
                mimeFileContent +="application/xml-external-parsed-entity=%s;\n" % textApp
                mimeFileContent +="application/xsd=%s;\n" % textApp
                mimeFileContent +="application/xslt+xml=%s;\n" % textApp
                mimeFileContent +="application/x-trash=%s;\n" % textApp
                mimeFileContent +="application/x-wine-extension-inf=%s;\n" % textApp
                mimeFileContent +="application/x-wine-extension-ini=%s;\n" % textApp
                mimeFileContent +="application/x-zerosize=%s;\n" % textApp
                mimeFileContent +="text/css=%s;\n" % textApp
                mimeFileContent +="text/plain=%s;\n" % textApp
                mimeFileContent +="text/x-authors=%s;\n" % textApp
                mimeFileContent +="text/x-c++-hdr=%s;\n" % textApp
                mimeFileContent +="text/x-c++-src=%s;\n" % textApp
                mimeFileContent +="text/x-changelog=%s;\n" % textApp
                mimeFileContent +="text/x-chdr=%s;\n" % textApp
                mimeFileContent +="text/x-cmake=%s;\n" % textApp
                mimeFileContent +="text/x-copying=%s;\n" % textApp
                mimeFileContent +="text/x-credits=%s;\n" % textApp
                mimeFileContent +="text/x-csharp=%s;\n" % textApp
                mimeFileContent +="text/x-csrc=%s;\n" % textApp
                mimeFileContent +="text/x-install=%s;\n" % textApp
                mimeFileContent +="text/x-log=%s;\n" % textApp
                mimeFileContent +="text/x-lua=%s;\n" % textApp
                mimeFileContent +="text/x-makefile=%s;\n" % textApp
                mimeFileContent +="text/x-ms-regedit=%s;\n" % textApp
                mimeFileContent +="text/x-nfo=%s;\n" % textApp
                mimeFileContent +="text/x-objchdr=%s;\n" % textApp
                mimeFileContent +="text/x-objcsrc=%s;\n" % textApp
                mimeFileContent +="text/x-pascal=%s;\n" % textApp
                mimeFileContent +="text/x-patch=%s;\n" % textApp
                mimeFileContent +="text/x-python=%s;\n" % textApp
                mimeFileContent +="text/x-readme=%s;\n" % textApp
                mimeFileContent +="text/x-vhdl=%s;\n" % textApp

            if self.ch_app_browser.isChecked():
                # TODO - needs something else for default browser
                browserApp = self.cb_app_browser.currentText().replace("/","-")
                mimeFileContent +="application/atom+xml=%s;\n" % browserApp
                mimeFileContent +="application/rss+xml=%s;\n" % browserApp
                mimeFileContent +="application/vnd.mozilla.xul+xml=%s;\n" % browserApp
                mimeFileContent +="application/x-mozilla-bookmarks=%s;\n" % browserApp
                mimeFileContent +="application/x-mswinurl=%s;\n" % browserApp
                mimeFileContent +="application/x-xbel=%s;\n" % browserApp
                mimeFileContent +="application/xhtml+xml=%s;\n" % browserApp
                mimeFileContent +="text/html=%s;\n" % browserApp
                mimeFileContent +="text/opml+xml=%s;\n" % browserApp

            realMimeFileContent  ="[Default Applications]\n"
            realMimeFileContent += mimeFileContent
            realMimeFileContent +="\n"
            realMimeFileContent +="[Added Associations]\n"
            realMimeFileContent += mimeFileContent
            realMimeFileContent +="\n"

            local_xdg_defaults = os.path.join(HOME, ".local", "share", "applications", "defaults.list")
            local_xdg_mimeapps = os.path.join(HOME, ".local", "share", "applications", "mimeapps.list")

            writeFile = open(local_xdg_defaults, "w")
            writeFile.write(realMimeFileContent)
            writeFile.close()

            writeFile = open(local_xdg_mimeapps, "w")
            writeFile.write(realMimeFileContent)
            writeFile.close()

        if "wineasio" in self.settings_changed_types:
            REGFILE  = 'REGEDIT4\n'
            REGFILE += '\n'
            REGFILE += '[HKEY_CURRENT_USER\Software\Wine\WineASIO]\n'
            REGFILE += '"Autostart server"=dword:0000000%i\n' % int(1 if self.cb_wineasio_autostart.isChecked() else 0)
            REGFILE += '"Connect to hardware"=dword:0000000%i\n' % int(1 if self.cb_wineasio_hw.isChecked() else 0)
            REGFILE += '"Fixed buffersize"=dword:0000000%i\n' % int(1 if self.cb_wineasio_fixed_bsize.isChecked() else 0)
            REGFILE += '"Number of inputs"=dword:000000%s\n' % smartHex(self.sb_wineasio_ins.value(), 2)
            REGFILE += '"Number of outputs"=dword:000000%s\n' % smartHex(self.sb_wineasio_outs.value(), 2)
            REGFILE += '"Preferred buffersize"=dword:0000%s\n' % smartHex(int(self.cb_wineasio_bsizes.currentText()), 4)

            writeFile = open("/tmp/cadence-wineasio.reg", "w")
            writeFile.write(REGFILE)
            writeFile.close()

            os.system("regedit /tmp/cadence-wineasio.reg")

        self.settings_changed_types = []
        self.frame_tweaks_settings.setVisible(False)

    @pyqtSlot()
    def slot_tweaksSettingsChanged_apps(self):
        self.func_settings_changed("apps")

    @pyqtSlot()
    def slot_tweaksSettingsChanged_wineasio(self):
        self.func_settings_changed("wineasio")

    @pyqtSlot(int)
    def slot_tweakAppImageHighlighted(self, index):
        self.setAppDetails(self.cb_app_image.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppImageChanged(self):
        self.setAppDetails(self.cb_app_image.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppMusicHighlighted(self, index):
        self.setAppDetails(self.cb_app_music.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppMusicChanged(self):
        self.setAppDetails(self.cb_app_music.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppVideoHighlighted(self, index):
        self.setAppDetails(self.cb_app_video.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppVideoChanged(self):
        self.setAppDetails(self.cb_app_video.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppTextHighlighted(self, index):
        self.setAppDetails(self.cb_app_text.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppTextChanged(self):
        self.setAppDetails(self.cb_app_text.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppBrowserHighlighted(self, index):
        self.setAppDetails(self.cb_app_browser.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppBrowserChanged(self):
        self.setAppDetails(self.cb_app_browser.currentText())
        self.func_settings_changed("apps")

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
        # Force row change
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

        GlobalSettings.setValue("JACK/AutoStart", self.cb_jack_autostart.isChecked())
        #GlobalSettings.setValue("A2J-MIDI/AutoStart", self.cb_a2jmidi_autostart.isChecked())
        #GlobalSettings.setValue("Pulse2JACK/AutoStart", (havePulseAudio and self.cb_pulse_autostart.isChecked()))

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

        self.m_savedSettings = {
            "Main/UseSystemTray": self.settings.value("Main/UseSystemTray", True, type=bool),
            "Main/CloseToTray": self.settings.value("Main/CloseToTray", True, type=bool)
        }

        self.cb_jack_autostart.setChecked(GlobalSettings.value("JACK/AutoStart", True, type=bool))
        #self.cb_a2j_autostart.setChecked(GlobalSettings.value("A2J/AutoStart", True).toBool())
        #self.cb_pulse_autostart.setChecked(GlobalSettings.value("Pulse2JACK/AutoStart", havePulseAudio).toBool())

    def timerEvent(self, event):
        if event.timerId() == self.m_timer250:
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
    app.setApplicationName("Cadence")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/cadence.svg"))

    if haveDBus:
        DBus.loop = DBusQtMainLoop(set_as_default=True)
        DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)

    initSystemChecks()

    # Show GUI
    gui = CadenceMainW()

    # Set-up custom signal handling
    setUpSignals(gui)

    if "--gnome-settings" in app.arguments():
        gui.setMenuBar(None)
        gui.tabWidget.removeTab(0)
        gui.tabWidget.removeTab(0)
        gui.tabWidget.tabBar().hide()
        gui.label_tweaks.setText(gui.tr("Cadence Tweaks"))
        gui.menubar.clear()
        gui.systray.hide()

    gui.show()

    # Exit properly
    sys.exit(gui.systray.exec_(app))

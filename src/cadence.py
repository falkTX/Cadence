#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Cadence, JACK utilities
# Copyright (C) 2010-2018 Filipe Coelho <falktx@falktx.com>
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

from platform import architecture

if True:
    from PyQt5.QtCore import QFileSystemWatcher, QThread, QSemaphore
    from PyQt5.QtWidgets import QApplication, QDialogButtonBox, QLabel, QMainWindow, QSizePolicy
else:
    from PyQt4.QtCore import QFileSystemWatcher, QThread, QSemaphore
    from PyQt4.QtGui import QApplication, QDialogButtonBox, QLabel, QMainWindow, QSizePolicy

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import systray
import ui_cadence
import ui_cadence_tb_jack
import ui_cadence_tb_alsa
import ui_cadence_tb_a2j
import ui_cadence_tb_pa
import ui_cadence_rwait
from shared_cadence import *
from shared_canvasjack import *
from shared_settings import *

# ------------------------------------------------------------------------------------------------------------
# Import getoutput

from subprocess import getoutput

# ------------------------------------------------------------------------------------------------------------
# Try Import DBus

try:
    import dbus
    from dbus.mainloop.pyqt5 import DBusQtMainLoop
    haveDBus = True
except:
    kxstudioWorkaround = "/opt/kxstudio/python3/dist-packages/dbus/mainloop"
    if os.path.exists(kxstudioWorkaround):
        try:
            sys.path.insert(1, kxstudioWorkaround)
            from pyqt5 import DBusQtMainLoop
            haveDBus = True
        except:
            haveDBus = False
    else:
        haveDBus = False

# ------------------------------------------------------------------------------------------------------------
# Check for PulseAudio and Wine

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
    "kde4/gwenview.desktop",
    "org.kde.digikam.desktop",
    "org.kde.gwenview.desktop",
]

DESKTOP_X_MUSIC = [
    "audacious.desktop",
    "clementine.desktop",
    "smplayer.desktop",
    "vlc.desktop",
    "kde4/amarok.desktop",
    "org.kde.amarok.desktop",
]

DESKTOP_X_VIDEO = [
    "smplayer.desktop",
    "vlc.desktop",
]

DESKTOP_X_TEXT = [
    "gedit.desktop",
    "kde4/kate.desktop",
    "kde4/kwrite.desktop",
    "org.kde.kate.desktop",
    "org.kde.kwrite.desktop",
]

DESKTOP_X_BROWSER = [
    "chrome.desktop",
    "firefox.desktop",
    "iceweasel.desktop",
    "kde4/konqbrowser.desktop",
    "org.kde.konqbrowser.desktop",
]

XDG_APPLICATIONS_PATH = [
    "/usr/share/applications",
    "/usr/local/share/applications"
]

WINEASIO_PREFIX = "HKEY_CURRENT_USER\Software\Wine\WineASIO"

# ---------------------------------------------------------------------

global jackClientIdALSA, jackClientIdPulseCapture, jackClientIdPulsePlayback
jackClientIdALSA          = -1
jackClientIdPulseCapture  = -1
jackClientIdPulsePlayback = -1

# jackdbus indexes
iGraphVersion    = 0
iJackClientId    = 1
iJackClientName  = 2
iJackPortId      = 3
iJackPortName    = 4
iJackPortNewName = 5
iJackPortFlags   = 5
iJackPortType    = 6

asoundrc_aloop = (""
"# ------------------------------------------------------\n"
"# Custom asoundrc file for use with snd-aloop and JACK\n"
"#\n"
"# use it like this:\n"
"# env JACK_SAMPLE_RATE=44100 JACK_PERIOD_SIZE=1024 alsa_in (...)\n"
"#\n"
"\n"
"# ------------------------------------------------------\n"
"# playback device\n"
"pcm.aloopPlayback {\n"
"  type dmix\n"
"  ipc_key 1\n"
"  ipc_key_add_uid true\n"
"  slave {\n"
"    pcm \"hw:Loopback,0,0\"\n"
"    format S32_LE\n"
"    rate {\n"
"      @func igetenv\n"
"      vars [ JACK_SAMPLE_RATE ]\n"
"      default 44100\n"
"    }\n"
"    period_size {\n"
"      @func igetenv\n"
"      vars [ JACK_PERIOD_SIZE ]\n"
"      default 1024\n"
"    }\n"
"    buffer_size 4096\n"
"  }\n"
"}\n"
"\n"
"# capture device\n"
"pcm.aloopCapture {\n"
"  type dsnoop\n"
"  ipc_key 2\n"
"  ipc_key_add_uid true\n"
"  slave {\n"
"    pcm \"hw:Loopback,0,1\"\n"
"    format S32_LE\n"
"    rate {\n"
"      @func igetenv\n"
"      vars [ JACK_SAMPLE_RATE ]\n"
"      default 44100\n"
"    }\n"
"    period_size {\n"
"      @func igetenv\n"
"      vars [ JACK_PERIOD_SIZE ]\n"
"      default 1024\n"
"    }\n"
"    buffer_size 4096\n"
"  }\n"
"}\n"
"\n"
"# duplex device\n"
"pcm.aloopDuplex {\n"
"  type asym\n"
"  playback.pcm \"aloopPlayback\"\n"
"  capture.pcm \"aloopCapture\"\n"
"}\n"
"\n"
"# ------------------------------------------------------\n"
"# default device\n"
"pcm.!default {\n"
"  type plug\n"
"  slave.pcm \"aloopDuplex\"\n"
"}\n"
"\n"
"# ------------------------------------------------------\n"
"# alsa_in -j alsa_in -dcloop -q 1\n"
"pcm.cloop {\n"
"  type dsnoop\n"
"  ipc_key 3\n"
"  ipc_key_add_uid true\n"
"  slave {\n"
"    pcm \"hw:Loopback,1,0\"\n"
"    channels 2\n"
"    format S32_LE\n"
"    rate {\n"
"      @func igetenv\n"
"      vars [ JACK_SAMPLE_RATE ]\n"
"      default 44100\n"
"    }\n"
"    period_size {\n"
"      @func igetenv\n"
"      vars [ JACK_PERIOD_SIZE ]\n"
"      default 1024\n"
"    }\n"
"    buffer_size 32768\n"
"  }\n"
"}\n"
"\n"
"# ------------------------------------------------------\n"
"# alsa_out -j alsa_out -dploop -q 1\n"
"pcm.ploop {\n"
"  type plug\n"
"  slave.pcm \"hw:Loopback,1,1\"\n"
"}")

asoundrc_aloop_check = asoundrc_aloop.split("pcm.aloopPlayback", 1)[0]

asoundrc_jack = (""
"pcm.!default {\n"
"    type plug\n"
"    slave { pcm \"jack\" }\n"
"}\n"
"\n"
"pcm.jack {\n"
"    type jack\n"
"    playback_ports {\n"
"        0 system:playback_1\n"
"        1 system:playback_2\n"
"    }\n"
"    capture_ports {\n"
"        0 system:capture_1\n"
"        1 system:capture_2\n"
"    }\n"
"}\n"
"\n"
"ctl.mixer0 {\n"
"    type hw\n"
"    card 0\n"
"}")

asoundrc_pulse = (""
"pcm.!default {\n"
"    type plug\n"
"    slave { pcm \"pulse\" }\n"
"}\n"
"\n"
"pcm.pulse {\n"
"    type pulse\n"
"}\n"
"\n"
"ctl.mixer0 {\n"
"    type hw\n"
"    card 0\n"
"}")

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

def isAlsaAudioBridged():
    global jackClientIdALSA
    return bool(jackClientIdALSA != -1)

def isPulseAudioStarted():
    return bool("pulseaudio" in getProcList())

def isPulseAudioBridged():
    global jackClientIdPulseCapture, jackClientIdPulsePlayback
    return bool(jackClientIdPulseCapture != -1 or jackClientIdPulsePlayback != -1)

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

# Wait while JACK restarts
class ForceRestartThread(QThread):
    progressChanged = pyqtSignal(int)

    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.m_wasStarted = False
        self.m_a2jExportHW = False

    def wasJackStarted(self):
        return self.m_wasStarted

    def startA2J(self):
        gDBus.a2j.set_hw_export(self.m_a2jExportHW)
        gDBus.a2j.start()

    def run(self):
        # Not started yet
        self.m_wasStarted = False
        self.progressChanged.emit(0)

        # Stop JACK safely first, if possible
        runFunctionInMainThread(tryCloseJackDBus)
        self.progressChanged.emit(20)

        # Kill All
        stopAllAudioProcesses(False)
        self.progressChanged.emit(30)

        # Connect to jackdbus
        runFunctionInMainThread(self.parent().DBusReconnect)

        if not gDBus.jack:
            return

        for x in range(30):
            self.progressChanged.emit(30+x*2)
            procsList = getProcList()
            if "jackdbus" in procsList:
                break
            else:
                sleep(0.1)

        self.progressChanged.emit(90)

        # Start it
        runFunctionInMainThread(gDBus.jack.StartServer)
        self.progressChanged.emit(93)

        # If we made it this far, then JACK is started
        self.m_wasStarted = True

        # Start bridges according to user settings

        # ALSA-Audio
        if GlobalSettings.value("ALSA-Audio/BridgeIndexType", iAlsaFileNone, type=int) == iAlsaFileLoop:
            startAlsaAudioLoopBridge()
            sleep(0.5)

        self.progressChanged.emit(94)

        # ALSA-MIDI
        if GlobalSettings.value("A2J/AutoStart", True, type=bool) and gDBus.a2j and not bool(gDBus.a2j.is_started()):
            self.m_a2jExportHW = GlobalSettings.value("A2J/ExportHW", True, type=bool)
            runFunctionInMainThread(self.startA2J)

        self.progressChanged.emit(96)

        # PulseAudio
        if GlobalSettings.value("Pulse2JACK/AutoStart", True, type=bool) and not isPulseAudioBridged():
            inputs  = GlobalSettings.value("Pulse2JACK/CaptureChannels",  -1, type=int)
            outputs = GlobalSettings.value("Pulse2JACK/PlaybackChannels", -1, type=int)
            
            os.system("cadence-pulse2jack -c %s -p %s" % (str(inputs), str(outputs)))

        self.progressChanged.emit(100)

# Force Restart Dialog
class ForceWaitDialog(QDialog, ui_cadence_rwait.Ui_Dialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.setWindowFlags(Qt.Dialog|Qt.WindowCloseButtonHint)

        self.rThread = ForceRestartThread(self)
        self.rThread.start()

        self.rThread.progressChanged.connect(self.progressBar.setValue)
        self.rThread.finished.connect(self.slot_rThreadFinished)

    def DBusReconnect(self):
        self.parent().DBusReconnect()

    @pyqtSlot()
    def slot_rThreadFinished(self):
        self.close()

        if self.rThread.wasJackStarted():
            QMessageBox.information(self, self.tr("Info"), self.tr("JACK was re-started sucessfully"))
        else:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Could not start JACK!"))

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Additional JACK options
class ToolBarJackDialog(QDialog, ui_cadence_tb_jack.Ui_Dialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.m_ladishLoaded = False

        if haveDBus:
            if GlobalSettings.value("JACK/AutoLoadLadishStudio", False, type=bool):
                self.rb_ladish.setChecked(True)
                self.m_ladishLoaded = True
            elif "org.ladish" in gDBus.bus.list_names():
                self.m_ladishLoaded = True
        else:
            self.rb_ladish.setEnabled(False)
            self.rb_jack.setChecked(True)

        if self.m_ladishLoaded:
            self.fillStudioNames()

        self.accepted.connect(self.slot_setOptions)
        self.rb_ladish.clicked.connect(self.slot_maybeFillStudioNames)

    def fillStudioNames(self):
        gDBus.ladish_control = gDBus.bus.get_object("org.ladish", "/org/ladish/Control")

        ladishStudioName = dbus.String(GlobalSettings.value("JACK/LadishStudioName", "", type=str))
        ladishStudioListDump = gDBus.ladish_control.GetStudioList()

        if len(ladishStudioListDump) == 0:
            self.rb_ladish.setEnabled(False)
            self.rb_jack.setChecked(True)
        else:
            i=0
            for thisStudioName, thisStudioDict in ladishStudioListDump:
                self.cb_studio_name.addItem(thisStudioName)
                if ladishStudioName and thisStudioName == ladishStudioName:
                    self.cb_studio_name.setCurrentIndex(i)
                i += 1

    @pyqtSlot()
    def slot_maybeFillStudioNames(self):
        if not self.m_ladishLoaded:
            self.fillStudioNames()
            self.m_ladishLoaded = True

    @pyqtSlot()
    def slot_setOptions(self):
        GlobalSettings.setValue("JACK/AutoLoadLadishStudio", self.rb_ladish.isChecked())
        GlobalSettings.setValue("JACK/LadishStudioName", self.cb_studio_name.currentText())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Additional ALSA Audio options
class ToolBarAlsaAudioDialog(QDialog, ui_cadence_tb_alsa.Ui_Dialog):
    def __init__(self, parent, customMode):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.asoundrcFile = os.path.join(HOME, ".asoundrc")
        self.fCustomMode  = customMode

        if customMode:
            asoundrcFd   = open(self.asoundrcFile, "r")
            asoundrcRead = asoundrcFd.read().strip()
            asoundrcFd.close()
            self.textBrowser.setPlainText(asoundrcRead)
            self.stackedWidget.setCurrentIndex(0)
            self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel)
        else:
            self.textBrowser.hide()
            self.stackedWidget.setCurrentIndex(1)
            self.adjustSize()

            self.spinBox.setValue(GlobalSettings.value("ALSA-Audio/BridgeChannels", 2, type=int))

            if GlobalSettings.value("ALSA-Audio/BridgeTool", "alsa_in", type=str) == "zita":
                self.comboBox.setCurrentIndex(1)
            else:
                self.comboBox.setCurrentIndex(0)

            self.accepted.connect(self.slot_setOptions)

    @pyqtSlot()
    def slot_setOptions(self):
        channels = self.spinBox.value()
        GlobalSettings.setValue("ALSA-Audio/BridgeChannels", channels)
        GlobalSettings.setValue("ALSA-Audio/BridgeTool", "zita" if (self.comboBox.currentIndex() == 1) else "alsa_in")

        asoundrcFd = open(self.asoundrcFile, "w")
        asoundrcFd.write(asoundrc_aloop.replace("channels 2\n", "channels %i\n" % channels) + "\n")
        asoundrcFd.close()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Additional ALSA MIDI options
class ToolBarA2JDialog(QDialog, ui_cadence_tb_a2j.Ui_Dialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.cb_export_hw.setChecked(GlobalSettings.value("A2J/ExportHW", True, type=bool))

        self.accepted.connect(self.slot_setOptions)

    @pyqtSlot()
    def slot_setOptions(self):
        GlobalSettings.setValue("A2J/ExportHW", self.cb_export_hw.isChecked())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Additional PulseAudio options
class ToolBarPADialog(QDialog, ui_cadence_tb_pa.Ui_Dialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        
        self.spinBoxPulseInputs.setValue(GlobalSettings.value("Pulse2JACK/CaptureChannels", 2, type=int))
        self.spinBoxPulseOutputs.setValue(GlobalSettings.value("Pulse2JACK/PlaybackChannels", 2, type=int))

        self.accepted.connect(self.slot_setOptions)

    @pyqtSlot()
    def slot_setOptions(self):
        GlobalSettings.setValue("Pulse2JACK/CaptureChannels", self.spinBoxPulseInputs.value())
        GlobalSettings.setValue("Pulse2JACK/PlaybackChannels", self.spinBoxPulseOutputs.value())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Main Window
class CadenceMainW(QMainWindow, ui_cadence.Ui_CadenceMainW):
    DBusJackServerStartedCallback = pyqtSignal()
    DBusJackServerStoppedCallback = pyqtSignal()
    DBusJackClientAppearedCallback = pyqtSignal(int, str)
    DBusJackClientDisappearedCallback = pyqtSignal(int)
    DBusA2JBridgeStartedCallback = pyqtSignal()
    DBusA2JBridgeStoppedCallback = pyqtSignal()

    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()
    SIGUSR2 = pyqtSignal()

    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Cadence")
        self.loadSettings(True)

        self.pix_apply   = QIcon(getIcon("dialog-ok-apply", 16)).pixmap(16, 16)
        self.pix_cancel  = QIcon(getIcon("dialog-cancel", 16)).pixmap(16, 16)
        self.pix_error   = QIcon(getIcon("dialog-error", 16)).pixmap(16, 16)
        self.pix_warning = QIcon(getIcon("dialog-warning", 16)).pixmap(16, 16)

        self.m_lastAlsaIndexType = -2 # invalid

        if jacklib and not jacklib.JACK2:
            self.b_jack_switchmaster.setEnabled(False)

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
        # Set-up GUI (System Status)

        self.m_availGovPath = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"
        self.m_curGovPath   = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
        self.m_curGovPaths  = []
        self.m_curGovCPUs   = []

        try:
            fBus   = dbus.SystemBus(mainloop=gDBus.loop)
            fProxy = fBus.get_object("com.ubuntu.IndicatorCpufreqSelector", "/Selector", introspect=False)
            haveFreqSelector = True
        except:
            haveFreqSelector = False

        if haveFreqSelector and os.path.exists(self.m_availGovPath) and os.path.exists(self.m_curGovPath):
            self.m_govWatcher = QFileSystemWatcher(self)
            self.m_govWatcher.addPath(self.m_curGovPath)
            self.m_govWatcher.fileChanged.connect(self.slot_governorFileChanged)
            QTimer.singleShot(0, self.slot_governorFileChanged)

            availGovFd   = open(self.m_availGovPath, "r")
            availGovRead = availGovFd.read().strip()
            availGovFd.close()

            self.m_availGovList = availGovRead.split(" ")
            for availGov in self.m_availGovList:
                self.cb_cpufreq.addItem(availGov)

            for root, dirs, files in os.walk("/sys/devices/system/cpu/"):
                for dir_ in [dir_ for dir_ in dirs if dir_.startswith("cpu")]:
                    if not dir_.replace("cpu", "", 1).isdigit():
                        continue

                    cpuGovPath = os.path.join(root, dir_, "cpufreq", "scaling_governor")

                    if os.path.exists(cpuGovPath):
                        self.m_curGovPaths.append(cpuGovPath)
                        self.m_curGovCPUs.append(int(dir_.replace("cpu", "", 1)))

            self.cb_cpufreq.setCurrentIndex(-1)

        else:
            self.m_govWatcher = None
            self.cb_cpufreq.setEnabled(False)
            self.label_cpufreq.setEnabled(False)

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
        # Set-up GUI (JACK Bridges)

        if not havePulseAudio:
            self.toolBox_pulseaudio.setEnabled(False)
            self.label_bridge_pulse.setText(self.tr("PulseAudio is not installed"))

        # Not available in cxfreeze builds
        if sys.argv[0].endswith("/cadence"):
            self.groupBox_bridges.setEnabled(False)
            self.cb_jack_autostart.setEnabled(False)
            self.tb_jack_options.setEnabled(False)

        # -------------------------------------------------------------
        # Set-up GUI (Tweaks)

        self.settings_changed_types = []
        self.frame_tweaks_settings.setVisible(False)

        for i in range(self.tw_tweaks.rowCount()):
            self.tw_tweaks.item(i, 0).setTextAlignment(Qt.AlignCenter)

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

            for bsize in BUFFER_SIZE_LIST:
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

        if haveDBus:
            self.systray.addAction("jack_start", self.tr("Start JACK"))
            self.systray.addAction("jack_stop", self.tr("Stop JACK"))
            self.systray.addAction("jack_configure", self.tr("Configure JACK"))
            self.systray.addSeparator("sep1")

            self.systray.addMenu("alsa", self.tr("ALSA Audio Bridge"))
            self.systray.addMenuAction("alsa", "alsa_start", self.tr("Start"))
            self.systray.addMenuAction("alsa", "alsa_stop", self.tr("Stop"))
            self.systray.addMenu("a2j", self.tr("ALSA MIDI Bridge"))
            self.systray.addMenuAction("a2j", "a2j_start", self.tr("Start"))
            self.systray.addMenuAction("a2j", "a2j_stop", self.tr("Stop"))
            self.systray.addMenuAction("a2j", "a2j_export_hw", self.tr("Export Hardware Ports..."))
            self.systray.addMenu("pulse", self.tr("PulseAudio Bridge"))
            self.systray.addMenuAction("pulse", "pulse_start", self.tr("Start"))
            self.systray.addMenuAction("pulse", "pulse_stop", self.tr("Stop"))

            self.systray.setActionIcon("jack_start", "media-playback-start")
            self.systray.setActionIcon("jack_stop", "media-playback-stop")
            self.systray.setActionIcon("jack_configure", "configure")
            self.systray.setActionIcon("alsa_start", "media-playback-start")
            self.systray.setActionIcon("alsa_stop", "media-playback-stop")
            self.systray.setActionIcon("a2j_start", "media-playback-start")
            self.systray.setActionIcon("a2j_stop", "media-playback-stop")
            self.systray.setActionIcon("pulse_start", "media-playback-start")
            self.systray.setActionIcon("pulse_stop", "media-playback-stop")

            self.systray.connect("jack_start", self.slot_JackServerStart)
            self.systray.connect("jack_stop", self.slot_JackServerStop)
            self.systray.connect("jack_configure", self.slot_JackServerConfigure)
            self.systray.connect("alsa_start", self.slot_AlsaBridgeStart)
            self.systray.connect("alsa_stop", self.slot_AlsaBridgeStop)
            self.systray.connect("a2j_start", self.slot_A2JBridgeStart)
            self.systray.connect("a2j_stop", self.slot_A2JBridgeStop)
            self.systray.connect("a2j_export_hw", self.slot_A2JBridgeExportHW)
            self.systray.connect("pulse_start", self.slot_PulseAudioBridgeStart)
            self.systray.connect("pulse_stop", self.slot_PulseAudioBridgeStop)

        self.systray.addMenu("tools", self.tr("Tools"))
        self.systray.addMenuAction("tools", "app_catarina", "Catarina")
        self.systray.addMenuAction("tools", "app_catia", "Catia")
        self.systray.addMenuAction("tools", "app_claudia", "Claudia")
        self.systray.addMenuSeparator("tools", "tools_sep")
        self.systray.addMenuAction("tools", "app_logs", "Logs")
        self.systray.addMenuAction("tools", "app_meter_in", "Meter (Inputs)")
        self.systray.addMenuAction("tools", "app_meter_out", "Meter (Output)")
        self.systray.addMenuAction("tools", "app_render", "Render")
        self.systray.addMenuAction("tools", "app_xy-controller", "XY-Controller")
        self.systray.addSeparator("sep2")

        self.systray.connect("app_catarina", self.func_start_catarina)
        self.systray.connect("app_catia", self.func_start_catia)
        self.systray.connect("app_claudia", self.func_start_claudia)
        self.systray.connect("app_logs", self.func_start_logs)
        self.systray.connect("app_meter_in", self.func_start_jackmeter_in)
        self.systray.connect("app_meter_out", self.func_start_jackmeter)
        self.systray.connect("app_render",  self.func_start_render)
        self.systray.connect("app_xy-controller", self.func_start_xycontroller)

        self.systray.setToolTip("Cadence")
        self.systray.show()

        # -------------------------------------------------------------
        # Set-up connections

        self.b_jack_start.clicked.connect(self.slot_JackServerStart)
        self.b_jack_stop.clicked.connect(self.slot_JackServerStop)
        self.b_jack_restart.clicked.connect(self.slot_JackServerForceRestart)
        self.b_jack_configure.clicked.connect(self.slot_JackServerConfigure)
        self.b_jack_switchmaster.clicked.connect(self.slot_JackServerSwitchMaster)
        self.tb_jack_options.clicked.connect(self.slot_JackOptions)

        self.b_alsa_start.clicked.connect(self.slot_AlsaBridgeStart)
        self.b_alsa_stop.clicked.connect(self.slot_AlsaBridgeStop)
        self.cb_alsa_type.currentIndexChanged[int].connect(self.slot_AlsaBridgeChanged)
        self.tb_alsa_options.clicked.connect(self.slot_AlsaAudioBridgeOptions)

        self.b_a2j_start.clicked.connect(self.slot_A2JBridgeStart)
        self.b_a2j_stop.clicked.connect(self.slot_A2JBridgeStop)
        self.b_a2j_export_hw.clicked.connect(self.slot_A2JBridgeExportHW)
        self.tb_a2j_options.clicked.connect(self.slot_A2JBridgeOptions)

        self.b_pulse_update.clicked.connect(self.slot_PulseAudioBridgeStart)
        self.b_pulse_start.clicked.connect(self.slot_PulseAudioBridgeStart)
        self.b_pulse_stop.clicked.connect(self.slot_PulseAudioBridgeStop)
        self.tb_pulse_options.clicked.connect(self.slot_PulseAudioBridgeOptions)

        self.pic_catia.clicked.connect(self.func_start_catia)
        self.pic_claudia.clicked.connect(self.func_start_claudia)
        self.pic_meter_in.clicked.connect(self.func_start_jackmeter_in)
        self.pic_meter_out.clicked.connect(self.func_start_jackmeter)
        self.pic_logs.clicked.connect(self.func_start_logs)
        self.pic_render.clicked.connect(self.func_start_render)
        self.pic_xycontroller.clicked.connect(self.func_start_xycontroller)

        self.b_tweaks_apply_now.clicked.connect(self.slot_tweaksApply)

        self.b_tweak_plugins_add.clicked.connect(self.slot_tweakPluginAdd)
        self.b_tweak_plugins_change.clicked.connect(self.slot_tweakPluginChange)
        self.b_tweak_plugins_remove.clicked.connect(self.slot_tweakPluginRemove)
        self.b_tweak_plugins_reset.clicked.connect(self.slot_tweakPluginReset)
        self.tb_tweak_plugins.currentChanged.connect(self.slot_tweakPluginTypeChanged)
        self.list_LADSPA.currentRowChanged.connect(self.slot_tweakPluginsLadspaRowChanged)
        self.list_DSSI.currentRowChanged.connect(self.slot_tweakPluginsDssiRowChanged)
        self.list_LV2.currentRowChanged.connect(self.slot_tweakPluginsLv2RowChanged)
        self.list_VST.currentRowChanged.connect(self.slot_tweakPluginsVstRowChanged)

        self.ch_app_image.clicked.connect(self.slot_tweaksSettingsChanged_apps)
        self.cb_app_image.highlighted.connect(self.slot_tweakAppImageHighlighted)
        self.cb_app_image.currentIndexChanged[int].connect(self.slot_tweakAppImageChanged)
        self.ch_app_music.clicked.connect(self.slot_tweaksSettingsChanged_apps)
        self.cb_app_music.highlighted.connect(self.slot_tweakAppMusicHighlighted)
        self.cb_app_music.currentIndexChanged[int].connect(self.slot_tweakAppMusicChanged)
        self.ch_app_video.clicked.connect(self.slot_tweaksSettingsChanged_apps)
        self.cb_app_video.highlighted.connect(self.slot_tweakAppVideoHighlighted)
        self.cb_app_video.currentIndexChanged[int].connect(self.slot_tweakAppVideoChanged)
        self.ch_app_text.clicked.connect(self.slot_tweaksSettingsChanged_apps)
        self.cb_app_text.highlighted.connect(self.slot_tweakAppTextHighlighted)
        self.cb_app_text.currentIndexChanged[int].connect(self.slot_tweakAppTextChanged)
        self.ch_app_browser.clicked.connect(self.slot_tweaksSettingsChanged_apps)
        self.cb_app_browser.highlighted.connect(self.slot_tweakAppBrowserHighlighted)
        self.cb_app_browser.currentIndexChanged[int].connect(self.slot_tweakAppBrowserChanged)

        self.sb_wineasio_ins.valueChanged.connect(self.slot_tweaksSettingsChanged_wineasio)
        self.sb_wineasio_outs.valueChanged.connect(self.slot_tweaksSettingsChanged_wineasio)
        self.cb_wineasio_hw.clicked.connect(self.slot_tweaksSettingsChanged_wineasio)
        self.cb_wineasio_autostart.clicked.connect(self.slot_tweaksSettingsChanged_wineasio)
        self.cb_wineasio_fixed_bsize.clicked.connect(self.slot_tweaksSettingsChanged_wineasio)
        self.cb_wineasio_bsizes.currentIndexChanged[int].connect(self.slot_tweaksSettingsChanged_wineasio)

        # org.jackaudio.JackControl
        self.DBusJackServerStartedCallback.connect(self.slot_DBusJackServerStartedCallback)
        self.DBusJackServerStoppedCallback.connect(self.slot_DBusJackServerStoppedCallback)

        # org.jackaudio.JackPatchbay
        self.DBusJackClientAppearedCallback.connect(self.slot_DBusJackClientAppearedCallback)
        self.DBusJackClientDisappearedCallback.connect(self.slot_DBusJackClientDisappearedCallback)

        # org.gna.home.a2jmidid.control
        self.DBusA2JBridgeStartedCallback.connect(self.slot_DBusA2JBridgeStartedCallback)
        self.DBusA2JBridgeStoppedCallback.connect(self.slot_DBusA2JBridgeStoppedCallback)

        # -------------------------------------------------------------

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.m_timer500  = None
        self.m_timer2000 = self.startTimer(2000)

        self.DBusReconnect()

        if haveDBus:
            gDBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword='dest', path_keyword='path',
                member_keyword='member', interface_keyword='interface', sender_keyword='sender', )

    def DBusReconnect(self):
        if haveDBus:
            try:
                gDBus.jack     = gDBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
                gDBus.patchbay = dbus.Interface(gDBus.jack, "org.jackaudio.JackPatchbay")
                jacksettings.initBus(gDBus.bus)
            except:
                gDBus.jack     = None
                gDBus.patchbay = None

            try:
                gDBus.a2j = dbus.Interface(gDBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            except:
                gDBus.a2j = None

        if gDBus.jack:
            if gDBus.jack.IsStarted():
                # Check for pulseaudio in jack graph
                try:
                    version, groups, conns = gDBus.patchbay.GetGraph(0)
                except:
                    version, groups, conns = (list(), list(), list())

                for group_id, group_name, ports in groups:
                    if group_name == "alsa2jack":
                        global jackClientIdALSA
                        jackClientIdALSA = group_id
                    elif group_name == "PulseAudio JACK Source":
                        global jackClientIdPulseCapture
                        jackClientIdPulseCapture = group_id
                    elif group_name == "PulseAudio JACK Sink":
                        global jackClientIdPulsePlayback
                        jackClientIdPulsePlayback = group_id

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
            self.b_jack_switchmaster.setEnabled(False)
            self.groupBox_bridges.setEnabled(False)

        if gDBus.a2j:
            try:
                started = gDBus.a2j.is_started()
            except:
                started = False

            if started:
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.toolBox_alsamidi.setEnabled(False)
            self.cb_a2j_autostart.setChecked(False)
            self.label_bridge_a2j.setText("ALSA MIDI Bridge is not installed")
            self.settings.setValue("A2J/AutoStart", False)

        self.updateSystrayTooltip()

    def DBusSignalReceiver(self, *args, **kwds):
        if kwds['interface'] == "org.freedesktop.DBus" and kwds['path'] == "/org/freedesktop/DBus" and kwds['member'] == "NameOwnerChanged":
            appInterface, appId, newId = args

            if not newId:
                # Something crashed
                if appInterface == "org.jackaudio.service":
                    QTimer.singleShot(0, self.slot_handleCrash_jack)
                elif appInterface == "org.gna.home.a2jmidid":
                    QTimer.singleShot(0, self.slot_handleCrash_a2j)

        elif kwds['interface'] == "org.jackaudio.JackControl":
            if DEBUG: print("org.jackaudio.JackControl", kwds['member'])
            if kwds['member'] == "ServerStarted":
                self.DBusJackServerStartedCallback.emit()
            elif kwds['member'] == "ServerStopped":
                self.DBusJackServerStoppedCallback.emit()

        elif kwds['interface'] == "org.jackaudio.JackPatchbay":
            if gDBus.patchbay and kwds['path'] == gDBus.patchbay.object_path:
                if DEBUG: print("org.jackaudio.JackPatchbay,", kwds['member'])
                if kwds['member'] == "ClientAppeared":
                    self.DBusJackClientAppearedCallback.emit(args[iJackClientId], args[iJackClientName])
                elif kwds['member'] == "ClientDisappeared":
                    self.DBusJackClientDisappearedCallback.emit(args[iJackClientId])

        elif kwds['interface'] == "org.gna.home.a2jmidid.control":
            if DEBUG: print("org.gna.home.a2jmidid.control", kwds['member'])
            if kwds['member'] == "bridge_started":
                self.DBusA2JBridgeStartedCallback.emit()
            elif kwds['member'] == "bridge_stopped":
                self.DBusA2JBridgeStoppedCallback.emit()

    def jackStarted(self):
        self.m_last_dsp_load = gDBus.jack.GetLoad()
        self.m_last_xruns    = gDBus.jack.GetXruns()
        self.m_last_buffer_size = gDBus.jack.GetBufferSize()

        self.b_jack_start.setEnabled(False)
        self.b_jack_stop.setEnabled(True)
        self.b_jack_switchmaster.setEnabled(True)
        self.systray.setActionEnabled("jack_start", False)
        self.systray.setActionEnabled("jack_stop", True)

        self.label_jack_status.setText("Started")
        self.label_jack_status_ico.setPixmap(self.pix_apply)

        if gDBus.jack.IsRealtime():
            self.label_jack_realtime.setText("Yes")
            self.label_jack_realtime_ico.setPixmap(self.pix_apply)
        else:
            self.label_jack_realtime.setText("No")
            self.label_jack_realtime_ico.setPixmap(self.pix_cancel)

        self.label_jack_dsp.setText("%.2f%%" % self.m_last_dsp_load)
        self.label_jack_xruns.setText(str(self.m_last_xruns))
        self.label_jack_bfsize.setText("%i samples" % self.m_last_buffer_size)
        self.label_jack_srate.setText("%i Hz" % gDBus.jack.GetSampleRate())
        self.label_jack_latency.setText("%.1f ms" % gDBus.jack.GetLatency())

        self.m_timer500 = self.startTimer(500)

        if gDBus.a2j and not gDBus.a2j.is_started():
            self.b_a2j_start.setEnabled(True)
            self.systray.setActionEnabled("a2j_start", True)

        self.checkAlsaAudio()
        self.checkPulseAudio()

    def jackStopped(self):
        if self.m_timer500:
            self.killTimer(self.m_timer500)
            self.m_timer500 = None

        self.m_last_dsp_load = None
        self.m_last_xruns    = None
        self.m_last_buffer_size = None

        self.b_jack_start.setEnabled(True)
        self.b_jack_stop.setEnabled(False)
        self.b_jack_switchmaster.setEnabled(False)

        if haveDBus:
            self.systray.setActionEnabled("jack_start", True)
            self.systray.setActionEnabled("jack_stop", False)

        self.label_jack_status.setText("Stopped")
        self.label_jack_status_ico.setPixmap(self.pix_cancel)

        self.label_jack_dsp.setText("---")
        self.label_jack_xruns.setText("---")
        self.label_jack_bfsize.setText("---")
        self.label_jack_srate.setText("---")
        self.label_jack_latency.setText("---")

        if gDBus.a2j:
            self.b_a2j_start.setEnabled(False)
            self.systray.setActionEnabled("a2j_start", False)

        global jackClientIdALSA, jackClientIdPulseCapture, jackClientIdPulsePlayback
        jackClientIdALSA          = -1
        jackClientIdPulseCapture  = -1
        jackClientIdPulsePlayback = -1 

        if haveDBus:
            self.checkAlsaAudio()
            self.checkPulseAudio()

    def a2jStarted(self):
        self.b_a2j_start.setEnabled(False)
        self.b_a2j_stop.setEnabled(True)
        self.b_a2j_export_hw.setEnabled(False)
        self.systray.setActionEnabled("a2j_start", False)
        self.systray.setActionEnabled("a2j_stop", True)
        self.systray.setActionEnabled("a2j_export_hw", False)
        self.label_bridge_a2j.setText(self.tr("ALSA MIDI Bridge is running"))

    def a2jStopped(self):
        jackRunning = bool(gDBus.jack and gDBus.jack.IsStarted())
        self.b_a2j_start.setEnabled(jackRunning)
        self.b_a2j_stop.setEnabled(False)
        self.b_a2j_export_hw.setEnabled(True)
        self.systray.setActionEnabled("a2j_start", jackRunning)
        self.systray.setActionEnabled("a2j_stop", False)
        self.systray.setActionEnabled("a2j_export_hw", True)
        self.label_bridge_a2j.setText(self.tr("ALSA MIDI Bridge is stopped"))

    def checkAlsaAudio(self):
        asoundrcFile = os.path.join(HOME, ".asoundrc")

        if not os.path.exists(asoundrcFile):
            self.b_alsa_start.setEnabled(False)
            self.b_alsa_stop.setEnabled(False)
            self.cb_alsa_type.setCurrentIndex(iAlsaFileNone)
            self.tb_alsa_options.setEnabled(False)
            self.label_bridge_alsa.setText(self.tr("No bridge in use"))
            self.m_lastAlsaIndexType = -1 # null
            return

        asoundrcFd   = open(asoundrcFile, "r")
        asoundrcRead = asoundrcFd.read().strip()
        asoundrcFd.close()

        if asoundrcRead.startswith(asoundrc_aloop_check):
            if isAlsaAudioBridged():
                self.b_alsa_start.setEnabled(False)
                self.b_alsa_stop.setEnabled(True)
                self.systray.setActionEnabled("alsa_start", False)
                self.systray.setActionEnabled("alsa_stop", True)
                self.label_bridge_alsa.setText(self.tr("Using Cadence snd-aloop daemon, started"))
            else:
                try:
                    jackRunning = bool(gDBus.jack and gDBus.jack.IsStarted())
                except:
                    jackRunning = False
                self.b_alsa_start.setEnabled(jackRunning)
                self.b_alsa_stop.setEnabled(False)
                self.systray.setActionEnabled("alsa_start", jackRunning)
                self.systray.setActionEnabled("alsa_stop", False)
                self.label_bridge_alsa.setText(self.tr("Using Cadence snd-aloop daemon, stopped"))

            self.cb_alsa_type.setCurrentIndex(iAlsaFileLoop)
            self.tb_alsa_options.setEnabled(True)

        elif asoundrcRead == asoundrc_jack:
            self.b_alsa_start.setEnabled(False)
            self.b_alsa_stop.setEnabled(False)
            self.systray.setActionEnabled("alsa_start", False)
            self.systray.setActionEnabled("alsa_stop", False)
            self.cb_alsa_type.setCurrentIndex(iAlsaFileJACK)
            self.tb_alsa_options.setEnabled(False)
            self.label_bridge_alsa.setText(self.tr("Using JACK plugin bridge (Always on)"))

        elif asoundrcRead == asoundrc_pulse:
            self.b_alsa_start.setEnabled(False)
            self.b_alsa_stop.setEnabled(False)
            self.systray.setActionEnabled("alsa_start", False)
            self.systray.setActionEnabled("alsa_stop", False)
            self.cb_alsa_type.setCurrentIndex(iAlsaFilePulse)
            self.tb_alsa_options.setEnabled(False)
            self.label_bridge_alsa.setText(self.tr("Using PulseAudio plugin bridge (Always on)"))

        else:
            self.b_alsa_start.setEnabled(False)
            self.b_alsa_stop.setEnabled(False)
            self.systray.setActionEnabled("alsa_start", False)
            self.systray.setActionEnabled("alsa_stop", False)
            self.cb_alsa_type.addItem(self.tr("Custom"))
            self.cb_alsa_type.setCurrentIndex(iAlsaFileMax)
            self.tb_alsa_options.setEnabled(True)
            self.label_bridge_alsa.setText(self.tr("Using custom asoundrc, not managed by Cadence"))

        self.m_lastAlsaIndexType = self.cb_alsa_type.currentIndex()

    def checkPulseAudio(self):
        if not havePulseAudio:
            self.systray.setActionEnabled("pulse_start", False)
            self.systray.setActionEnabled("pulse_stop", False)
            return

        if isPulseAudioStarted():
            if isPulseAudioBridged():
                self.b_pulse_update.setEnabled(True)
                self.b_pulse_start.setEnabled(False)
                self.b_pulse_stop.setEnabled(True)
                self.systray.setActionEnabled("pulse_start", False)
                self.systray.setActionEnabled("pulse_stop", True)
                self.label_bridge_pulse.setText(self.tr("PulseAudio is started and bridged to JACK"))
            else:
                jackRunning = bool(gDBus.jack and gDBus.jack.IsStarted())
                self.b_pulse_update.setEnabled(jackRunning)
                self.b_pulse_start.setEnabled(jackRunning)
                self.b_pulse_stop.setEnabled(False)
                self.systray.setActionEnabled("pulse_start", jackRunning)
                self.systray.setActionEnabled("pulse_stop", False)
                self.label_bridge_pulse.setText(self.tr("PulseAudio is started but not bridged"))
        else:
            jackRunning = bool(gDBus.jack and gDBus.jack.IsStarted())
            self.b_pulse_update.setEnabled(False)
            self.b_pulse_start.setEnabled(jackRunning)
            self.b_pulse_stop.setEnabled(False)
            self.systray.setActionEnabled("pulse_start", jackRunning)
            self.systray.setActionEnabled("pulse_stop", False)
            self.label_bridge_pulse.setText(self.tr("PulseAudio is not started"))

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

    def updateSystrayTooltip(self):
        systrayText  = "Cadence<br/>"
        systrayText += "<font size=\"-1\">"
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("JACK Status"), self.label_jack_status.text())
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("Realtime"), self.label_jack_realtime.text())
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("DSP Load"), self.label_jack_dsp.text())
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("Xruns"), self.label_jack_xruns.text())
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("Buffer Size"), self.label_jack_bfsize.text())
        systrayText += "<b>%s:</b>&nbsp;%s<br/>" % (self.tr("Sample Rate"), self.label_jack_srate.text())
        systrayText += "<b>%s:</b>&nbsp;%s" % (self.tr("Block Latency"), self.label_jack_latency.text())
        systrayText += "</font><font size=\"-2\"><br/></font>"

        self.systray.setToolTip(systrayText)

    @pyqtSlot()
    def func_start_catarina(self):
        self.func_start_tool("catarina")

    @pyqtSlot()
    def func_start_catia(self):
        self.func_start_tool("catia")

    @pyqtSlot()
    def func_start_claudia(self):
        self.func_start_tool("claudia")

    @pyqtSlot()
    def func_start_logs(self):
        self.func_start_tool("cadence-logs")

    @pyqtSlot()
    def func_start_jackmeter(self):
        self.func_start_tool("cadence-jackmeter")

    @pyqtSlot()
    def func_start_jackmeter_in(self):
        self.func_start_tool("cadence-jackmeter -in")

    @pyqtSlot()
    def func_start_render(self):
        self.func_start_tool("cadence-render")

    @pyqtSlot()
    def func_start_xycontroller(self):
        self.func_start_tool("cadence-xycontroller")

    def func_start_tool(self, tool):
        if sys.argv[0].endswith(".py"):
            if tool == "cadence-logs":
                tool = "logs"
            elif tool == "cadence-render":
                tool = "render"

            stool = tool.split(" ", 1)[0]

            if stool in ("cadence-jackmeter", "cadence-xycontroller"):
                python    = ""
                localPath = os.path.join(sys.path[0], "..", "c++", stool.replace("cadence-", ""))

                if os.path.exists(os.path.join(localPath, stool)):
                    base = localPath + os.sep
                else:
                    base = ""

            else:
                python = sys.executable
                tool  += ".py"
                base   = sys.argv[0].rsplit("cadence.py", 1)[0]

                if python:
                    python += " "

            cmd = "%s%s%s &" % (python, base, tool)

            print(cmd)
            os.system(cmd)

        elif sys.argv[0].endswith("/cadence"):
            base = sys.argv[0].rsplit("/cadence", 1)[0]
            os.system("%s/%s &" % (base, tool))

        else:
            os.system("%s &" % tool)

    def func_settings_changed(self, stype):
        if stype not in self.settings_changed_types:
            self.settings_changed_types.append(stype)
        self.frame_tweaks_settings.setVisible(True)

    @pyqtSlot()
    def slot_DBusJackServerStartedCallback(self):
        self.jackStarted()

    @pyqtSlot()
    def slot_DBusJackServerStoppedCallback(self):
        self.jackStopped()

    @pyqtSlot(int, str)
    def slot_DBusJackClientAppearedCallback(self, group_id, group_name):
        if group_name == "alsa2jack":
            global jackClientIdALSA
            jackClientIdALSA = group_id
            self.checkAlsaAudio()
        elif group_name == "PulseAudio JACK Source":
            global jackClientIdPulseCapture
            jackClientIdPulseCapture = group_id
            self.checkPulseAudio()
        elif group_name == "PulseAudio JACK Sink":
            global jackClientIdPulsePlayback
            jackClientIdPulsePlayback = group_id
            self.checkPulseAudio()

    @pyqtSlot(int)
    def slot_DBusJackClientDisappearedCallback(self, group_id):
        global jackClientIdALSA, jackClientIdPulseCapture, jackClientIdPulsePlayback
        if group_id == jackClientIdALSA:
            jackClientIdALSA = -1
            self.checkAlsaAudio()
        elif group_id == jackClientIdPulseCapture:
            jackClientIdPulseCapture = -1
            self.checkPulseAudio()
        elif group_id == jackClientIdPulsePlayback:
            jackClientIdPulsePlayback = -1
            self.checkPulseAudio()

    @pyqtSlot()
    def slot_DBusA2JBridgeStartedCallback(self):
        self.a2jStarted()

    @pyqtSlot()
    def slot_DBusA2JBridgeStoppedCallback(self):
        self.a2jStopped()

    @pyqtSlot()
    def slot_JackServerStart(self):
        self.saveSettings()
        try:
            gDBus.jack.StartServer()
        except:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to start JACK, please check the logs for more information."))

    @pyqtSlot()
    def slot_JackServerStop(self):
        try:
            gDBus.jack.StopServer()
        except:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to stop JACK, please check the logs for more information."))

    @pyqtSlot()
    def slot_JackServerForceRestart(self):
        if gDBus.jack.IsStarted():
            ask = CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
                                   self.tr("This will force kill all JACK applications!<br>Make sure to save your projects before continue."),
                                   self.tr("Are you sure you want to force the restart of JACK?"))

            if ask != QMessageBox.Yes:
                return

        if self.m_timer500:
            self.killTimer(self.m_timer500)
            self.m_timer500 = None

        self.saveSettings()
        ForceWaitDialog(self).exec_()

    @pyqtSlot()
    def slot_JackServerConfigure(self):
        jacksettingsW = jacksettings.JackSettingsW(self)
        jacksettingsW.exec_()
        del jacksettingsW

    @pyqtSlot()
    def slot_JackServerSwitchMaster(self):
        try:
            gDBus.jack.SwitchMaster()
        except:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to switch JACK master, please check the logs for more information."))
            return

        self.jackStarted()

    @pyqtSlot()
    def slot_JackOptions(self):
        ToolBarJackDialog(self).exec_()

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if gDBus.jack:
            gDBus.jack.ResetXruns()

    @pyqtSlot()
    def slot_AlsaBridgeStart(self):
        self.slot_AlsaBridgeStop()
        startAlsaAudioLoopBridge()

    @pyqtSlot()
    def slot_AlsaBridgeStop(self):
        checkFile = "/tmp/.cadence-aloop-daemon.x"
        if os.path.exists(checkFile):
            os.remove(checkFile)

    @pyqtSlot(int)
    def slot_AlsaBridgeChanged(self, index):
        if self.m_lastAlsaIndexType == -2 or self.m_lastAlsaIndexType == index:
            return

        if self.m_lastAlsaIndexType == iAlsaFileMax:
            ask = CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
                                   self.tr(""
                                           "You're using a custom ~/.asoundrc file not managed by Cadence.<br/>"
                                           "By choosing to use a Cadence ALSA-Audio bridge, <b>the file will be replaced</b>."
                                           ""),
                                   self.tr("Are you sure you want to do this?"))

            if ask == QMessageBox.Yes:
                self.cb_alsa_type.blockSignals(True)
                self.cb_alsa_type.removeItem(iAlsaFileMax)
                self.cb_alsa_type.setCurrentIndex(index)
                self.cb_alsa_type.blockSignals(False)
            else:
                self.cb_alsa_type.blockSignals(True)
                self.cb_alsa_type.setCurrentIndex(iAlsaFileMax)
                self.cb_alsa_type.blockSignals(False)
                return

        asoundrcFile = os.path.join(HOME, ".asoundrc")

        if index == iAlsaFileNone:
            os.remove(asoundrcFile)

        elif index == iAlsaFileLoop:
            asoundrcFd = open(asoundrcFile, "w")
            asoundrcFd.write(asoundrc_aloop+"\n")
            asoundrcFd.close()

        elif index == iAlsaFileJACK:
            asoundrcFd = open(asoundrcFile, "w")
            asoundrcFd.write(asoundrc_jack+"\n")
            asoundrcFd.close()

        elif index == iAlsaFilePulse:
            asoundrcFd = open(asoundrcFile, "w")
            asoundrcFd.write(asoundrc_pulse+"\n")
            asoundrcFd.close()

        else:
            print("Cadence::AlsaBridgeChanged(%i) - invalid index" % index)

        self.checkAlsaAudio()

    @pyqtSlot()
    def slot_AlsaAudioBridgeOptions(self):
        ToolBarAlsaAudioDialog(self, (self.cb_alsa_type.currentIndex() != iAlsaFileLoop)).exec_()

    @pyqtSlot()
    def slot_A2JBridgeStart(self):
        gDBus.a2j.start()

    @pyqtSlot()
    def slot_A2JBridgeStop(self):
        gDBus.a2j.stop()

    @pyqtSlot()
    def slot_A2JBridgeExportHW(self):
        ask = QMessageBox.question(self, self.tr("ALSA MIDI Hardware Export"), self.tr("Enable Hardware Export on the ALSA MIDI Bridge?"), QMessageBox.Yes|QMessageBox.No|QMessageBox.Cancel, QMessageBox.Yes)

        if ask == QMessageBox.Yes:
            gDBus.a2j.set_hw_export(True)
        elif ask == QMessageBox.No:
            gDBus.a2j.set_hw_export(False)

    @pyqtSlot()
    def slot_A2JBridgeOptions(self):
        ToolBarA2JDialog(self).exec_()

    @pyqtSlot()
    def slot_PulseAudioBridgeStart(self):
        inputs  = GlobalSettings.value("Pulse2JACK/CaptureChannels",  2, type=int)
        outputs = GlobalSettings.value("Pulse2JACK/PlaybackChannels", 2, type=int)
        
        os.system("cadence-pulse2jack -c %s -p %s" % (str(inputs), str(outputs)))

    @pyqtSlot()
    def slot_PulseAudioBridgeStop(self):
        os.system("pulseaudio -k")

    @pyqtSlot()
    def slot_PulseAudioBridgeOptions(self):
        ToolBarPADialog(self).exec_()

    @pyqtSlot()
    def slot_handleCrash_jack(self):
        self.DBusReconnect()

    @pyqtSlot()
    def slot_handleCrash_a2j(self):
        pass

    @pyqtSlot(str)
    def slot_changeGovernorMode(self, newMode):
        bus   = dbus.SystemBus(mainloop=gDBus.loop)
        #proxy = bus.get_object("org.cadence.CpufreqSelector", "/Selector", introspect=False)
        #print(proxy.hello())
        proxy = bus.get_object("com.ubuntu.IndicatorCpufreqSelector", "/Selector", introspect=False)
        proxy.SetGovernor(self.m_curGovCPUs, newMode, dbus_interface="com.ubuntu.IndicatorCpufreqSelector")

    @pyqtSlot()
    def slot_governorFileChanged(self):
        curGovFd   = open(self.m_curGovPath, "r")
        curGovRead = curGovFd.read().strip()
        curGovFd.close()

        customTr = self.tr("Custom")

        if self.cb_cpufreq.currentIndex() == -1:
            # First init
            self.cb_cpufreq.currentIndexChanged[str].connect(self.slot_changeGovernorMode)

        self.cb_cpufreq.blockSignals(True)

        if curGovRead in self.m_availGovList:
            self.cb_cpufreq.setCurrentIndex(self.m_availGovList.index(curGovRead))

            if customTr in self.m_availGovList:
                self.m_availGovList.remove(customTr)
        else:
            if customTr not in self.m_availGovList:
                self.cb_cpufreq.addItem(customTr)
                self.m_availGovList.append(customTr)

            self.cb_cpufreq.setCurrentIndex(len(self.m_availGovList)-1)

        self.cb_cpufreq.blockSignals(False)

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
    def slot_tweakAppImageChanged(self, ignored):
        self.setAppDetails(self.cb_app_image.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppMusicHighlighted(self, index):
        self.setAppDetails(self.cb_app_music.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppMusicChanged(self, ignored):
        self.setAppDetails(self.cb_app_music.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppVideoHighlighted(self, index):
        self.setAppDetails(self.cb_app_video.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppVideoChanged(self, ignored):
        self.setAppDetails(self.cb_app_video.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppTextHighlighted(self, index):
        self.setAppDetails(self.cb_app_text.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppTextChanged(self, ignored):
        self.setAppDetails(self.cb_app_text.currentText())
        self.func_settings_changed("apps")

    @pyqtSlot(int)
    def slot_tweakAppBrowserHighlighted(self, index):
        self.setAppDetails(self.cb_app_browser.itemText(index))

    @pyqtSlot(int)
    def slot_tweakAppBrowserChanged(self, ignored):
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
        GlobalSettings.setValue("ALSA-Audio/BridgeIndexType", self.cb_alsa_type.currentIndex())
        GlobalSettings.setValue("A2J/AutoStart", self.cb_a2j_autostart.isChecked())
        GlobalSettings.setValue("Pulse2JACK/AutoStart", (havePulseAudio and self.cb_pulse_autostart.isChecked()))

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", b""))

        usingAlsaLoop = bool(GlobalSettings.value("ALSA-Audio/BridgeIndexType", iAlsaFileNone, type=int) == iAlsaFileLoop)

        self.cb_jack_autostart.setChecked(GlobalSettings.value("JACK/AutoStart", wantJackStart, type=bool))
        self.cb_a2j_autostart.setChecked(GlobalSettings.value("A2J/AutoStart", True, type=bool))
        self.cb_pulse_autostart.setChecked(GlobalSettings.value("Pulse2JACK/AutoStart", havePulseAudio and not usingAlsaLoop, type=bool))

    def timerEvent(self, event):
        if event.timerId() == self.m_timer500:
            if gDBus.jack and self.m_last_dsp_load != None:
                next_dsp_load = gDBus.jack.GetLoad()
                next_xruns    = gDBus.jack.GetXruns()
                needUpdateTip = False

                if self.m_last_dsp_load != next_dsp_load:
                    self.m_last_dsp_load = next_dsp_load
                    self.label_jack_dsp.setText("%.2f%%" % self.m_last_dsp_load)
                    needUpdateTip = True

                if self.m_last_xruns != next_xruns:
                    self.m_last_xruns = next_xruns
                    self.label_jack_xruns.setText(str(self.m_last_xruns))
                    needUpdateTip = True

                if needUpdateTip:
                    self.updateSystrayTooltip()

        elif event.timerId() == self.m_timer2000:
            if gDBus.jack and self.m_last_buffer_size != None:
                next_buffer_size = gDBus.jack.GetBufferSize()

                if self.m_last_buffer_size != next_buffer_size:
                    self.m_last_buffer_size = next_buffer_size
                    self.label_jack_bfsize.setText("%i samples" % self.m_last_buffer_size)
                    self.label_jack_latency.setText("%.1f ms" % gDBus.jack.GetLatency())

            else:
                self.update()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        self.systray.handleQtCloseEvent(event)

# ------------------------------------------------------------------------------------------------------------

def runFunctionInMainThread(task):
    waiter = QSemaphore(1)

    def taskInMainThread():
        task()
        waiter.release()

    QTimer.singleShot(0, taskInMainThread)
    waiter.tryAcquire()

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Cadence")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/cadence.svg"))

    if haveDBus:
        gDBus.loop = DBusQtMainLoop(set_as_default=True)
        gDBus.bus  = dbus.SessionBus(mainloop=gDBus.loop)

    initSystemChecks()

    # Show GUI
    gui = CadenceMainW()

    # Set-up custom signal handling
    setUpSignals(gui)

    if "--minimized" in app.arguments():
        gui.hide()
        gui.systray.setActionText("show", gui.tr("Restore"))
        app.setQuitOnLastWindowClosed(False)
    else:
        gui.show()

    # Exit properly
    sys.exit(gui.systray.exec_(app))

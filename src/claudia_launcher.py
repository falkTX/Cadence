#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ... TODO
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

from random import randint

if True:
    from PyQt5.QtCore import pyqtSlot, Qt, QTimer, QSettings
    from PyQt5.QtWidgets import QMainWindow, QTableWidgetItem, QWidget
else:
    from PyQt4.QtCore import pyqtSlot, Qt, QTimer, QSettings
    from PyQt4.QtGui import QMainWindow, QTableWidgetItem, QWidget

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import claudia_database as database
import ui_claudia_launcher
from shared import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Carla)

try:
    from carla_utils import *
    haveCarla = True
except:
    haveCarla = False

# ------------------------------------------------------------------------------------------------------------
# Safe import getoutput

if sys.version_info >= (3, 0):
    from subprocess import getoutput
else:
    from commands import getoutput

# ------------------------------------------------------------------------------------------------------------
# Debug Mode

SHOW_ALL = False

# ------------------------------------------------------------------------------------------------------------
# Tab Indexes

iTabDAW        = 0
iTabHost       = 1
iTabInstrument = 2
iTabBristol    = 3
iTabPlugin     = 4
iTabEffect     = 5
iTabTool       = 6

EXTRA_ICON_PATHS = [
    "/usr/share/icons",
    "/usr/share/pixmaps",
    "/usr/local/share/pixmaps"
]

# ------------------------------------------------------------------------------------------------------------
# XIcon class

class XIcon(object):
    def __init__(self):
        object.__init__(self)

    def addIconPath(self, path):
        iconPaths = QIcon.themeSearchPaths()
        iconPaths.append(path)
        QIcon.setThemeSearchPaths(iconPaths)

    def getIcon(self, name):
        if os.path.exists(name):
            icon = QIcon(name)
        else:
            icon = QIcon.fromTheme(name)

        if icon.isNull():
            for iEXTRA_PATH in EXTRA_ICON_PATHS:
                if os.path.exists(os.path.join(iEXTRA_PATH, name + ".png")):
                    icon = QIcon(os.path.join(iEXTRA_PATH, name + ".png"))
                    break
                elif os.path.exists(os.path.join(iEXTRA_PATH, name + ".svg")):
                    icon = QIcon(os.path.join(iEXTRA_PATH, name + ".svg"))
                    break
                elif os.path.exists(os.path.join(iEXTRA_PATH, name + ".xpm")):
                    icon = QIcon(os.path.join(iEXTRA_PATH, name + ".xpm"))
                    break
            else:
                print("XIcon::getIcon(%s) - Failed to find icon" % name)

        return icon

# ------------------------------------------------------------------------------------------------------------
# Launcher object

class ClaudiaLauncher(QWidget, ui_claudia_launcher.Ui_ClaudiaLauncherW):
    def __init__(self, parent):
        QWidget.__init__(self, parent)
        self.setupUi(self)

        self._parent   = None
        self._settings = None
        self.m_ladish_only = False

        self.listDAW.setColumnWidth(0, 22)
        self.listDAW.setColumnWidth(1, 225)
        self.listDAW.setColumnWidth(2, 150)
        self.listHost.setColumnWidth(0, 22)
        self.listHost.setColumnWidth(1, 225)
        self.listHost.setColumnWidth(2, 100)
        self.listHost.setColumnWidth(3, 100)
        self.listInstrument.setColumnWidth(0, 22)
        self.listInstrument.setColumnWidth(1, 225)
        self.listInstrument.setColumnWidth(2, 150)
        self.listBristol.setColumnWidth(0, 22)
        self.listBristol.setColumnWidth(1, 150)
        self.listPlugin.setColumnWidth(0, 225)
        self.listPlugin.setColumnWidth(1, 175)
        self.listEffect.setColumnWidth(0, 22)
        self.listEffect.setColumnWidth(1, 225)
        self.listEffect.setColumnWidth(2, 150)
        self.listTool.setColumnWidth(0, 22)
        self.listTool.setColumnWidth(1, 225)
        self.listTool.setColumnWidth(2, 150)

        # For the custom icons
        self.ClaudiaIcons = XIcon()

        self.icon_yes = QIcon(self.getIcon("dialog-ok-apply"))
        self.icon_no  = QIcon(self.getIcon("dialog-cancel"))

        self.m_lastThemeName = QIcon.themeName()

        # Copy our icons, so we can then set the fallback icon theme as the current theme
        iconPath = os.path.join(TMP, ".claudia-icons")

        if not os.path.exists(iconPath):
            os.mkdir(iconPath)

        syspath = sys.path[0]
        if os.path.exists(os.path.join(syspath, "..", "icons")):
            os.system("cp -r '%s' '%s'" % (os.path.join(syspath, "..", "icons", "claudia-hicolor"), iconPath))
        elif os.path.exists(os.path.join(syspath, "..", "data", "icons")):
            os.system("cp -r '%s' '%s'" % (os.path.join(syspath, "..", "data", "icons", "claudia-hicolor"), iconPath))

        os.system("sed -i 's/X-CURRENT-THEME-X/%s/' '%s'" % (self.m_lastThemeName, os.path.join(iconPath, "claudia-hicolor", "index.theme")))

        self.ClaudiaIcons.addIconPath(iconPath)
        QIcon.setThemeName("claudia-hicolor")

        self.clearInfo_DAW()
        self.clearInfo_Host()
        self.clearInfo_Intrument()
        self.clearInfo_Bristol()
        self.clearInfo_Plugin()
        self.clearInfo_Effect()
        self.clearInfo_Tool()

        self.refreshAll()

        self.tabWidget.currentChanged.connect(self.slot_checkSelectedTab)

        self.listDAW.currentCellChanged.connect(self.slot_checkSelectedDAW)
        self.listHost.currentCellChanged.connect(self.slot_checkSelectedHost)
        self.listInstrument.currentCellChanged.connect(self.slot_checkSelectedInstrument)
        self.listBristol.currentCellChanged.connect(self.slot_checkSelectedBristol)
        self.listPlugin.currentCellChanged.connect(self.slot_checkSelectedPlugin)
        self.listEffect.currentCellChanged.connect(self.slot_checkSelectedEffect)
        self.listTool.currentCellChanged.connect(self.slot_checkSelectedTool)
        self.listDAW.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listHost.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listInstrument.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listBristol.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listPlugin.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listEffect.cellDoubleClicked.connect(self.slot_doubleClickedList)
        self.listTool.cellDoubleClicked.connect(self.slot_doubleClickedList)

    def getSelectedAppAndBinary(self):
        tabIndex = self.tabWidget.currentIndex()

        if tabIndex == iTabDAW:
            item = self.listDAW.item(self.listDAW.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iDAW_AppName], item[database.iDAW_Binary])

        if tabIndex == iTabHost:
            item = self.listHost.item(self.listHost.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iHost_AppName], item[database.iHost_Binary])

        if tabIndex == iTabInstrument:
            item = self.listInstrument.item(self.listInstrument.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iInstrument_AppName], item[database.iInstrument_Binary])

        if tabIndex == iTabBristol:
            item = self.listBristol.item(self.listBristol.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iBristol_AppName], "startBristol -audio jack -midi jack -%s" % item[database.iBristol_ShortName])

        if tabIndex == iTabPlugin:
            plugin = self.listPlugin.item(self.listPlugin.currentRow(), 0).data(Qt.UserRole)
            return (plugin["name"], "carla-single lv2 %s" % plugin["label"])

        if tabIndex == iTabEffect:
            item = self.listEffect.item(self.listEffect.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iEffect_AppName], item[database.iEffect_Binary])

        if tabIndex == iTabTool:
            item = self.listTool.item(self.listTool.currentRow(), 0).data(Qt.UserRole)
            return (item[database.iTool_AppName], item[database.iTool_Binary])

        return ("", "")

    def startApp(self):
        app, binary = self.getSelectedAppAndBinary()

        if not (app and binary):
            return

        os.system("cd '%s' && %s &" % (self.callback_getProjectFolder(), binary))

    def addAppToLADISH(self):
        app, binary = self.getSelectedAppAndBinary()

        if not (app and binary):
            return

        if binary.startswith("startBristol") or binary.startswith("carla-single"):
            self.createAppTemplate(app, binary)

        elif binary in ("ardour",
                        "ardour3",
                        "hydrogen",
                        "jacker",
                        "lmms",
                        "muse",
                        "non-sequencer",
                        "non-timeline",
                        "qtractor",
                        "rosegarden",
                        "seq24",
                        "calfjackhost",
                        "carla",
                        "jack-rack",
                        "qsampler",
                        "jack-mixer"):
            self.createAppTemplate(app, binary)

        else:
            appBus = self.callback_getAppBus()
            appBus.RunCustom2(False, binary, app, "0")

    def createAppTemplate(self, app, binary):
        rand_check  = randint(1, 99999)
        proj_bpm    = str(self.callback_getBPM())
        proj_srate  = str(self.callback_getSampleRate())
        proj_folder = self.callback_getProjectFolder()

        tmplte_dir  = None
        tmplte_file = None
        tmplte_cmd  = ""
        tmplte_lvl  = "0"

        syspath = sys.path[0]
        if os.path.exists(os.path.join(syspath, "..", "templates")):
            tmplte_dir = os.path.join(syspath, "..", "templates")
        elif os.path.exists(os.path.join(syspath, "..", "data", "templates")):
            tmplte_dir = os.path.join(syspath, "..", "data", "templates")
        else:
            app = None
            tmplte_cmd = binary
            print("ClaudiaLauncher::createAppTemplate() - Failed to find template dir")
            return False

        if not os.path.exists(proj_folder):
            os.mkdir(proj_folder)

        if binary.startswith("startBristol"):
            module = binary.replace("startBristol -audio jack -midi jack -", "")
            tmplte_folder = os.path.join(proj_folder, "bristol_%s_%i" % (module, rand_check))
            os.mkdir(tmplte_folder)

            if self.callback_isLadishRoom():
                tmplte_folder = os.path.basename(tmplte_folder)

            tmplte_cmd  = binary
            tmplte_cmd += " -emulate %s" % module
            tmplte_cmd += " -cache '%s'" % tmplte_folder
            tmplte_cmd += " -memdump '%s'" % tmplte_folder
            tmplte_cmd += " -import '%s'" % os.path.join(tmplte_folder, "memory")
            tmplte_cmd += " -exec"
            tmplte_lvl  = "1"

        elif binary.startswith("carla-single"):
            tmplte_cmd = binary + " " + proj_folder
            tmplte_lvl = "1"

        elif binary == "ardour":
            tmplte_folder = os.path.join(proj_folder, "Ardour2_%i" % rand_check)
            tmplte_file   = os.path.join(tmplte_folder, "Ardour2_%i.ardour" % rand_check)
            os.mkdir(tmplte_folder)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Ardour2", "Ardour2.ardour"), tmplte_file))
            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Ardour2", "instant.xml"), tmplte_folder))
            os.mkdir(os.path.join(tmplte_folder, "analysis"))
            os.mkdir(os.path.join(tmplte_folder, "dead_sounds"))
            os.mkdir(os.path.join(tmplte_folder, "export"))
            os.mkdir(os.path.join(tmplte_folder, "interchange"))
            os.mkdir(os.path.join(tmplte_folder, "interchange", "Ardour"))
            os.mkdir(os.path.join(tmplte_folder, "interchange", "Ardour", "audiofiles"))
            os.mkdir(os.path.join(tmplte_folder, "peaks"))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_folder) if self.callback_isLadishRoom() else tmplte_folder)

        elif binary == "ardour3":
            projName = "Ardour3_%i" % rand_check
            tmplte_folder = os.path.join(proj_folder, projName)
            tmplte_file   = os.path.join(tmplte_folder, "%s.ardour" % projName)
            os.mkdir(tmplte_folder)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Ardour3", "Ardour3.ardour"), tmplte_file))
            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Ardour3", "instant.xml"), tmplte_folder))
            os.mkdir(os.path.join(tmplte_folder, "analysis"))
            os.mkdir(os.path.join(tmplte_folder, "dead"))
            os.mkdir(os.path.join(tmplte_folder, "export"))
            os.mkdir(os.path.join(tmplte_folder, "externals"))
            os.mkdir(os.path.join(tmplte_folder, "interchange"))
            os.mkdir(os.path.join(tmplte_folder, "interchange", projName))
            os.mkdir(os.path.join(tmplte_folder, "interchange", projName, "audiofiles"))
            os.mkdir(os.path.join(tmplte_folder, "interchange", projName, "midifiles"))
            os.mkdir(os.path.join(tmplte_folder, "peaks"))
            os.mkdir(os.path.join(tmplte_folder, "plugins"))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_folder) if self.callback_isLadishRoom() else tmplte_folder)

            if self.callback_isLadishRoom():
                tmplte_lvl = "jacksession"

        elif binary == "hydrogen":
            tmplte_file = os.path.join(proj_folder, "Hydrogen_%i.h2song" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Hydrogen.h2song"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " -s '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

            if self.callback_isLadishRoom():
                tmplte_lvl = "jacksession"
            else:
                tmplte_lvl = "1"

        elif binary == "jacker":
            tmplte_file = os.path.join(proj_folder, "Jacker_%i.jsong" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Jacker.jsong"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

            if database.USING_KXSTUDIO:
                tmplte_lvl  = "1"

            # No decimal bpm support
            proj_bpm = proj_bpm.split(".")[0]

        elif binary == "lmms":
            tmplte_file = os.path.join(proj_folder, "LMMS_%i.mmp" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "LMMS.mmp"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

            # No decimal bpm support
            proj_bpm = proj_bpm.split(".",1)[0]

        elif binary == "muse":
            tmplte_file = os.path.join(proj_folder, "MusE_%i.med" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "MusE.med"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

        elif binary == "non-sequencer":
            tmplte_file_r = os.path.join(proj_folder, "Non-Sequencer_%i.non" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Non-Sequencer.non"), tmplte_file_r))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file_r) if self.callback_isLadishRoom() else tmplte_file_r)

        elif binary == "non-timeline":
            tmplte_folder = os.path.join(proj_folder, "Non-Timeline_%i" % rand_check)
            os.mkdir(tmplte_folder)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Non-Timeline", "history"), tmplte_folder))
            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Non-Timeline", "info"), tmplte_folder))
            os.mkdir(os.path.join(tmplte_folder, "sources"))

            os.system('sed -i "s/X_SR_X-CLAUDIA-X_SR_X/%s/" "%s"' % (proj_srate, os.path.join(tmplte_folder, "info")))
            os.system('sed -i "s/X_BPM_X-CLAUDIA-X_BPM_X/%s/" "%s"' % (proj_bpm, os.path.join(tmplte_folder, "history")))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_folder) if self.callback_isLadishRoom() else tmplte_folder)

        elif binary == "qtractor":
            tmplte_file = os.path.join(proj_folder, "Qtractor_%i.qtr" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Qtractor.qtr"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)
            tmplte_lvl  = "1"

        elif binary == "rosegarden":
            tmplte_file = os.path.join(proj_folder, "Rosegarden_%i.rg" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Rosegarden.rg"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)
            tmplte_lvl  = "1"

        elif binary == "seq24":
            tmplte_file_r = os.path.join(proj_folder, "Seq24_%i.midi" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Seq24.midi"), tmplte_file_r))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file_r) if self.callback_isLadishRoom() else tmplte_file_r)
            tmplte_lvl  = "1"

        elif binary == "calfjackhost":
            tmplte_file = os.path.join(proj_folder, "CalfJackHost_%i" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "CalfJackHost"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " --load '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)
            tmplte_lvl  = "1"

        elif binary == "carla":
            tmplte_file = os.path.join(proj_folder, "Carla_%i.carxp" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Carla.carxp"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)
            tmplte_lvl  = "1"

        elif binary == "jack-rack":
            tmplte_file = os.path.join(proj_folder, "Jack-Rack_%i.xml" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Jack-Rack.xml"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

        elif binary == "qsampler":
            tmplte_file = os.path.join(proj_folder, "Qsampler_%i.lscp" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Qsampler.lscp"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

        elif binary == "jack-mixer":
            tmplte_file = os.path.join(proj_folder, "Jack-Mixer_%i.xml" % rand_check)

            os.system("cp '%s' '%s'" % (os.path.join(tmplte_dir, "Jack-Mixer.xml"), tmplte_file))

            tmplte_cmd  = binary
            tmplte_cmd += " -c '%s'" % (os.path.basename(tmplte_file) if self.callback_isLadishRoom() else tmplte_file)

        else:
            print("ClaudiaLauncher::createAppTemplate(%s) - Failed to parse app name" % app)
            return False

        if tmplte_file is not None:
            os.system('sed -i "s|X_SR_X-CLAUDIA-X_SR_X|%s|" "%s"' % (proj_srate, tmplte_file))
            os.system('sed -i "s|X_BPM_X-CLAUDIA-X_BPM_X|%s|" "%s"' % (proj_bpm, tmplte_file))
            os.system('sed -i "s|X_FOLDER_X-CLAUDIA-X_FOLDER_X|%s|" "%s"' % (proj_folder.replace("|", "\|").replace("$", "\$"), tmplte_file))

        appBus = self.callback_getAppBus()
        appBus.RunCustom2(False, tmplte_cmd, app, tmplte_lvl)
        return True

    def parentR(self):
        return self._parent

    def settings(self):
        return self._settings

    def getIcon(self, icon):
        return self.ClaudiaIcons.getIcon(icon)

    def getIconForYesNo(self, yesno):
        return self.icon_yes if yesno else self.icon_no

    def setCallbackApp(self, parent, settings, ladish_only):
        self._parent = parent
        self._settings = settings
        self.m_ladish_only = ladish_only

    def clearInfo_DAW(self):
        self.ico_app_daw.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_daw.setText("App Name")
        self.ico_ladspa_daw.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_dssi_daw.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_lv2_daw.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_vst_daw.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_vst_mode_daw.setText("")
        self.ico_jack_transport_daw.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_midi_mode_daw.setText("---")
        self.label_session_level_daw.setText(database.LEVEL_0)
        self.frame_DAW.setEnabled(False)
        self.showDoc_DAW("", "")

    def clearInfo_Host(self):
        self.ico_app_host.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_host.setText("App Name")
        self.ico_ladspa_host.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_dssi_host.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_lv2_host.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_vst_host.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_vst_mode_host.setText("")
        self.label_midi_mode_host.setText("---")
        self.label_session_level_host.setText(database.LEVEL_0)
        self.frame_Host.setEnabled(False)
        self.showDoc_Host("", "")

    def clearInfo_Intrument(self):
        self.ico_app_ins.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_ins.setText("App Name")
        self.ico_builtin_fx_ins.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_audio_input_ins.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_midi_mode_ins.setText("---")
        self.label_session_level_ins.setText(database.LEVEL_0)
        self.frame_Instrument.setEnabled(False)
        self.showDoc_Instrument("", "")

    def clearInfo_Bristol(self):
        self.ico_app_bristol.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_bristol.setText("App Name")
        self.ico_builtin_fx_bristol.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.ico_audio_input_bristol.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_midi_mode_bristol.setText("---")
        self.label_session_level_bristol.setText(database.LEVEL_0)
        self.frame_Bristol.setEnabled(False)
        self.showDoc_Bristol("", "")

    def clearInfo_Plugin(self):
        self.ico_app_plugin.setPixmap(self.getIcon("lv2").pixmap(48, 48))
        self.label_name_plugin.setText("Plugin Name")
        self.label_plugin_audio_ins.setText("0")
        self.label_plugin_audio_outs.setText("0")
        self.label_plugin_midi_ins.setText("0")
        self.label_plugin_midi_outs.setText("0")
        self.label_plugin_parameter_ins.setText("0")
        self.label_plugin_parameter_outs.setText("0")
        self.frame_Plugin.setEnabled(False)
        self.showDoc_Plugin("", "")

    def clearInfo_Effect(self):
        self.ico_app_effect.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_effect.setText("App Name")
        self.ico_stereo_effect.setPixmap(self.getIconForYesNo(False).pixmap(16, 16))
        self.label_midi_mode_effect.setText("---")
        self.label_session_level_effect.setText(database.LEVEL_0)
        self.frame_Effect.setEnabled(False)
        self.showDoc_Effect("", "")

    def clearInfo_Tool(self):
        self.ico_app_tool.setPixmap(self.getIcon("start-here").pixmap(48, 48))
        self.label_name_tool.setText("App Name")
        self.label_midi_mode_tool.setText("---")
        self.label_session_level_tool.setText(database.LEVEL_0)
        self.frame_Tool.setEnabled(False)
        self.showDoc_Tool("", "")

    def showDoc_DAW(self, doc, web):
        self.url_documentation_daw.setVisible(bool(doc))
        self.url_website_daw.setVisible(bool(web))
        self.label_no_help_daw.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_daw, doc)
        if web: self.setWebUrl(self.url_website_daw, web)

    def showDoc_Host(self, doc, web):
        self.url_documentation_host.setVisible(bool(doc))
        self.url_website_host.setVisible(bool(web))
        self.label_no_help_host.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_host, doc)
        if web: self.setWebUrl(self.url_website_host, web)

    def showDoc_Instrument(self, doc, web):
        self.url_documentation_ins.setVisible(bool(doc))
        self.url_website_ins.setVisible(bool(web))
        self.label_no_help_ins.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_ins, doc)
        if web: self.setWebUrl(self.url_website_ins, web)

    def showDoc_Bristol(self, doc, web):
        self.url_documentation_bristol.setVisible(bool(doc))
        self.url_website_bristol.setVisible(bool(web))
        self.label_no_help_bristol.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_bristol, doc)
        if web: self.setWebUrl(self.url_website_bristol, web)

    def showDoc_Plugin(self, doc, web):
        self.url_documentation_plugin.setVisible(bool(doc))
        self.url_website_plugin.setVisible(bool(web))
        self.label_no_help_plugin.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_plugin, doc)
        if web: self.setWebUrl(self.url_website_plugin, web)

    def showDoc_Effect(self, doc, web):
        self.url_documentation_effect.setVisible(bool(doc))
        self.url_website_effect.setVisible(bool(web))
        self.label_no_help_effect.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_effect, doc)
        if web: self.setWebUrl(self.url_website_effect, web)

    def showDoc_Tool(self, doc, web):
        self.url_documentation_tool.setVisible(bool(doc))
        self.url_website_tool.setVisible(bool(web))
        self.label_no_help_tool.setVisible(not(doc or web))

        if doc: self.setDocUrl(self.url_documentation_tool, doc)
        if web: self.setWebUrl(self.url_website_tool, web)

    def setDocUrl(self, item, link):
        item.setText(self.tr("<a href='%s'>Documentation</a>" % link))

    def setWebUrl(self, item, link):
        item.setText(self.tr("<a href='%s'>WebSite</a>" % link))

    def clearAll(self):
        self.listDAW.clearContents()
        self.listHost.clearContents()
        self.listInstrument.clearContents()
        self.listBristol.clearContents()
        self.listPlugin.clearContents()
        self.listEffect.clearContents()
        self.listTool.clearContents()
        for x in range(self.listDAW.rowCount()):
            self.listDAW.removeRow(0)
        for x in range(self.listHost.rowCount()):
            self.listHost.removeRow(0)
        for x in range(self.listInstrument.rowCount()):
            self.listInstrument.removeRow(0)
        for x in range(self.listBristol.rowCount()):
            self.listBristol.removeRow(0)
        for x in range(self.listPlugin.rowCount()):
            self.listPlugin.removeRow(0)
        for x in range(self.listEffect.rowCount()):
            self.listEffect.removeRow(0)
        for x in range(self.listTool.rowCount()):
            self.listTool.removeRow(0)

    def refreshAll(self):
        self.clearAll()
        pkglist = []

        if not SHOW_ALL:
            if os.path.exists("/usr/bin/yaourt"):
                pkg_out = getoutput("env LANG=C LC_ALL=C /usr/bin/yaourt -Qsq 2>/dev/null").split("\n")
                for package in pkg_out:
                    pkglist.append(package)

            elif os.path.exists("/usr/bin/pacman"):
                pkg_out = getoutput("env LANG=C LC_ALL=C /usr/bin/pacman -Qsq 2>/dev/null").split("\n")
                for package in pkg_out:
                    pkglist.append(package)

            elif os.path.exists("/usr/bin/dpkg"):
                pkg_out = getoutput("env LANG=C LC_ALL=C /usr/bin/dpkg --get-selections 2>/dev/null").split("\n")
                for pkg_info in pkg_out:
                    package, installed = pkg_info.rsplit("\t", 1)
                    if installed == "install":
                        pkglist.append(package.strip())

            elif os.path.exists("/bin/rpm"):
                pkg_out = getoutput("env LANG=C /bin/rpm -qa --qf \"%{NAME}\n\" 2>/dev/null").split("\n")
                for package in pkg_out:
                    pkglist.append(package)

            if not "bristol" in pkglist:
                self.tabWidget.setTabEnabled(iTabBristol, False)

            if not "carla-git" in pkglist:
                self.tabWidget.setTabEnabled(iTabPlugin, False)

        last_pos = 0
        for item in database.list_DAW:
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_name = QTableWidgetItem(AppName)
                w_type = QTableWidgetItem(Type)
                w_save = QTableWidgetItem(Save)

                self.listDAW.insertRow(last_pos)
                self.listDAW.setItem(last_pos, 0, w_icon)
                self.listDAW.setItem(last_pos, 1, w_name)
                self.listDAW.setItem(last_pos, 2, w_type)
                self.listDAW.setItem(last_pos, 3, w_save)

                last_pos += 1

        last_pos = 0
        for item in database.list_Host:
            Package, AppName, Instruments, Effects, Binary, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_name = QTableWidgetItem(AppName)
                w_h_in = QTableWidgetItem(Instruments)
                w_h_ef = QTableWidgetItem(Effects)
                w_save = QTableWidgetItem(Save)

                self.listHost.insertRow(last_pos)
                self.listHost.setItem(last_pos, 0, w_icon)
                self.listHost.setItem(last_pos, 1, w_name)
                self.listHost.setItem(last_pos, 2, w_h_in)
                self.listHost.setItem(last_pos, 3, w_h_ef)
                self.listHost.setItem(last_pos, 4, w_save)

                last_pos += 1

        last_pos = 0
        for item in database.list_Instrument:
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_name = QTableWidgetItem(AppName)
                w_type = QTableWidgetItem(Type)
                w_save = QTableWidgetItem(Save)

                self.listInstrument.insertRow(last_pos)
                self.listInstrument.setItem(last_pos, 0, w_icon)
                self.listInstrument.setItem(last_pos, 1, w_name)
                self.listInstrument.setItem(last_pos, 2, w_type)
                self.listInstrument.setItem(last_pos, 3, w_save)

                last_pos += 1

        last_pos = 0
        for item in database.list_Bristol:
            Package, FullName, Type, ShortName, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_fullname = QTableWidgetItem(FullName)
                w_shortname = QTableWidgetItem(ShortName)

                self.listBristol.insertRow(last_pos)
                self.listBristol.setItem(last_pos, 0, w_icon)
                self.listBristol.setItem(last_pos, 1, w_shortname)
                self.listBristol.setItem(last_pos, 2, w_fullname)

                last_pos += 1

        last_pos = 0
        for item in database.list_Effect:
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_name = QTableWidgetItem(AppName)
                w_type = QTableWidgetItem(Type)
                w_save = QTableWidgetItem(Save)

                self.listEffect.insertRow(last_pos)
                self.listEffect.setItem(last_pos, 0, w_icon)
                self.listEffect.setItem(last_pos, 1, w_name)
                self.listEffect.setItem(last_pos, 2, w_type)
                self.listEffect.setItem(last_pos, 3, w_save)

                last_pos += 1

        last_pos = 0
        for item in database.list_Tool:
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item
            if SHOW_ALL or Package in pkglist:
                w_icon = QTableWidgetItem("")
                w_icon.setData(Qt.UserRole, item)
                w_icon.setIcon(QIcon(self.getIcon(Icon)))
                w_name = QTableWidgetItem(AppName)
                w_type = QTableWidgetItem(Type)
                w_save = QTableWidgetItem(Save)

                self.listTool.insertRow(last_pos)
                self.listTool.setItem(last_pos, 0, w_icon)
                self.listTool.setItem(last_pos, 1, w_name)
                self.listTool.setItem(last_pos, 2, w_type)
                self.listTool.setItem(last_pos, 3, w_save)

                last_pos += 1

        if haveCarla and os.path.exists("/usr/lib/carla/libcarla_utils.so"):
            utils = CarlaUtils("/usr/lib/carla/libcarla_utils.so")
            last_pos = 0
            lv2path = os.getenv("LV2_PATH", "~/.lv2:/usr/lib/lv2:/usr/local/lib/lv2")
            for i in range(utils.get_cached_plugin_count(PLUGIN_LV2, lv2path)):
                plugin = utils.get_cached_plugin_info(PLUGIN_LV2, i)

                if (plugin["hints"] & PLUGIN_HAS_CUSTOM_UI) == 0:
                    continue

                catgtext = ""

                if plugin["category"] == PLUGIN_CATEGORY_SYNTH:
                    catgtext = "Synth"
                elif plugin["category"] == PLUGIN_CATEGORY_DELAY:
                    catgtext = "Delay"
                elif plugin["category"] == PLUGIN_CATEGORY_EQ:
                    catgtext = "Equalizer"
                elif plugin["category"] == PLUGIN_CATEGORY_FILTER:
                    catgtext = "Filter"
                elif plugin["category"] == PLUGIN_CATEGORY_DISTORTION:
                    catgtext = "Distortion"
                elif plugin["category"] == PLUGIN_CATEGORY_DYNAMICS:
                    catgtext = "Dynamics"
                elif plugin["category"] == PLUGIN_CATEGORY_MODULATOR:
                    catgtext = "Modulator"
                elif plugin["category"] == PLUGIN_CATEGORY_UTILITY:
                    catgtext = "Utility"
                elif plugin["category"] == PLUGIN_CATEGORY_OTHER:
                    catgtext = "Other"

                if not plugin["category"]:
                    if plugin["hints"] & PLUGIN_IS_SYNTH:
                        catgtext = "Synth"
                    else:
                        catgtext = "FX"

                w_name = QTableWidgetItem(plugin["name"])
                w_auth = QTableWidgetItem(plugin["maker"])
                w_catg = QTableWidgetItem(catgtext)

                w_name.setData(Qt.UserRole, plugin)

                self.listPlugin.insertRow(last_pos)
                self.listPlugin.setItem(last_pos, 0, w_name)
                self.listPlugin.setItem(last_pos, 1, w_auth)
                self.listPlugin.setItem(last_pos, 2, w_catg)

                last_pos += 1

        self.listDAW.setCurrentCell(-1, -1)
        self.listHost.setCurrentCell(-1, -1)
        self.listInstrument.setCurrentCell(-1, -1)
        self.listBristol.setCurrentCell(-1, -1)
        self.listPlugin.setCurrentCell(-1, -1)
        self.listEffect.setCurrentCell(-1, -1)
        self.listTool.setCurrentCell(-1, -1)

        self.listDAW.sortByColumn(1, Qt.AscendingOrder)
        self.listHost.sortByColumn(1, Qt.AscendingOrder)
        self.listInstrument.sortByColumn(1, Qt.AscendingOrder)
        self.listBristol.sortByColumn(2, Qt.AscendingOrder)
        self.listPlugin.sortByColumn(0, Qt.AscendingOrder)
        self.listEffect.sortByColumn(1, Qt.AscendingOrder)
        self.listTool.sortByColumn(1, Qt.AscendingOrder)

    @pyqtSlot(int)
    def slot_checkSelectedTab(self, tabIndex):
        if tabIndex == iTabDAW:
            test_selected = (len(self.listDAW.selectedItems()) > 0)
        elif tabIndex == iTabHost:
            test_selected = (len(self.listHost.selectedItems()) > 0)
        elif tabIndex == iTabInstrument:
            test_selected = (len(self.listInstrument.selectedItems()) > 0)
        elif tabIndex == iTabBristol:
            test_selected = (len(self.listBristol.selectedItems()) > 0)
        elif tabIndex == iTabPlugin:
            test_selected = (len(self.listPlugin.selectedItems()) > 0)
        elif tabIndex == iTabEffect:
            test_selected = (len(self.listEffect.selectedItems()) > 0)
        elif tabIndex == iTabTool:
            test_selected = (len(self.listTool.selectedItems()) > 0)
        else:
            test_selected = False

        self.callback_checkGUI(test_selected)

    @pyqtSlot(int)
    def slot_checkSelectedDAW(self, row):
        if row >= 0:
            item = self.listDAW.item(row, 0).data(Qt.UserRole)
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item

            self.frame_DAW.setEnabled(True)
            self.ico_app_daw.setPixmap(QIcon(self.getIcon(Icon)).pixmap(48, 48))
            self.ico_ladspa_daw.setPixmap(QIcon(self.getIconForYesNo(Features[0])).pixmap(16, 16))
            self.ico_dssi_daw.setPixmap(QIcon(self.getIconForYesNo(Features[1])).pixmap(16, 16))
            self.ico_lv2_daw.setPixmap(QIcon(self.getIconForYesNo(Features[2])).pixmap(16, 16))
            self.ico_vst_daw.setPixmap(QIcon(self.getIconForYesNo(Features[3])).pixmap(16, 16))
            self.ico_jack_transport_daw.setPixmap(QIcon(self.getIconForYesNo(Features[5])).pixmap(16, 16))
            self.label_name_daw.setText(AppName)
            self.label_vst_mode_daw.setText(Features[4])
            self.ico_midi_mode_daw.setPixmap(QIcon(self.getIconForYesNo(Features[6])).pixmap(16, 16))
            self.label_midi_mode_daw.setText(Features[7])
            self.label_session_level_daw.setText(Level)

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_DAW(Docs0, Docs[1])
        else:
            self.clearInfo_DAW()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedHost(self, row):
        if row >= 0:
            item = self.listHost.item(row, 0).data(Qt.UserRole)
            Package, AppName, Instruments, Effects, Binary, Icon, Save, Level, Features, Docs = item

            self.frame_Host.setEnabled(True)
            self.ico_app_host.setPixmap(self.getIcon(Icon).pixmap(48, 48))
            self.ico_internal_host.setPixmap(self.getIconForYesNo(Features[0]).pixmap(16, 16))
            self.ico_ladspa_host.setPixmap(self.getIconForYesNo(Features[1]).pixmap(16, 16))
            self.ico_dssi_host.setPixmap(self.getIconForYesNo(Features[2]).pixmap(16, 16))
            self.ico_lv2_host.setPixmap(self.getIconForYesNo(Features[3]).pixmap(16, 16))
            self.ico_vst_host.setPixmap(self.getIconForYesNo(Features[4]).pixmap(16, 16))
            self.label_name_host.setText(AppName)
            self.label_vst_mode_host.setText(Features[5])
            self.label_midi_mode_host.setText(Features[6])
            self.label_session_level_host.setText(str(Level))

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_Host(Docs0, Docs[1])
        else:
            self.clearInfo_DAW()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedInstrument(self, row):
        if row >= 0:
            item = self.listInstrument.item(row, 0).data(Qt.UserRole)
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item

            self.frame_Instrument.setEnabled(True)
            self.ico_app_ins.setPixmap(self.getIcon(Icon).pixmap(48, 48))
            self.ico_builtin_fx_ins.setPixmap(self.getIconForYesNo(Features[0]).pixmap(16, 16))
            self.ico_audio_input_ins.setPixmap(self.getIconForYesNo(Features[1]).pixmap(16, 16))
            self.label_name_ins.setText(AppName)
            self.label_midi_mode_ins.setText(Features[2])
            self.label_session_level_ins.setText(str(Level))

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_Instrument(Docs0, Docs[1])
        else:
            self.clearInfo_Intrument()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedBristol(self, row):
        if row >= 0:
            item = self.listBristol.item(row, 0).data(Qt.UserRole)
            Package, AppName, Type, ShortName, Icon, Save, Level, Features, Docs = item

            self.frame_Bristol.setEnabled(True)
            self.ico_app_bristol.setPixmap(self.getIcon(Icon).pixmap(48, 48))
            self.ico_builtin_fx_bristol.setPixmap(self.getIconForYesNo(Features[0]).pixmap(16, 16))
            self.ico_audio_input_bristol.setPixmap(self.getIconForYesNo(Features[1]).pixmap(16, 16))
            self.label_name_bristol.setText(AppName)
            self.label_midi_mode_bristol.setText(Features[2])
            self.label_session_level_bristol.setText(str(Level))

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_Bristol(Docs0, Docs[1])
        else:
            self.clearInfo_Bristol()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedPlugin(self, row):
        if row >= 0:
            plugin = self.listPlugin.item(row, 0).data(Qt.UserRole)

            self.frame_Plugin.setEnabled(True)
            self.ico_app_bristol.setPixmap(self.getIcon("lv2").pixmap(48, 48))
            self.label_name_plugin.setText(plugin["name"])
            self.label_plugin_audio_ins.setText(str(plugin["audioIns"]))
            self.label_plugin_audio_outs.setText(str(plugin["audioOuts"]))
            self.label_plugin_midi_ins.setText(str(plugin["midiIns"]))
            self.label_plugin_midi_outs.setText(str(plugin["midiOuts"]))
            self.label_plugin_parameter_ins.setText(str(plugin["parameterIns"]))
            self.label_plugin_parameter_outs.setText(str(plugin["parameterOuts"]))

            #Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            #self.showDoc_Plugin(Docs0, Docs[1])
        else:
            self.clearInfo_Plugin()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedEffect(self, row):
        if row >= 0:
            item = self.listEffect.item(row, 0).data(Qt.UserRole)
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item

            self.frame_Effect.setEnabled(True)
            self.ico_app_effect.setPixmap(self.getIcon(Icon).pixmap(48, 48))
            self.ico_stereo_effect.setPixmap(self.getIconForYesNo(Features[0]).pixmap(16, 16))
            self.label_name_effect.setText(AppName)
            self.label_midi_mode_effect.setText(Features[1])
            self.label_session_level_effect.setText(str(Level))

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_Effect(Docs0, Docs[1])
        else:
            self.clearInfo_Effect()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot(int)
    def slot_checkSelectedTool(self, row):
        if row >= 0:
            item = self.listTool.item(row, 0).data(Qt.UserRole)
            Package, AppName, Type, Binary, Icon, Save, Level, Features, Docs = item

            self.frame_Tool.setEnabled(True)
            self.ico_app_tool.setPixmap(self.getIcon(Icon).pixmap(48, 48))
            self.label_name_tool.setText(AppName)
            self.label_midi_mode_tool.setText(Features[0])
            self.ico_jack_transport_tool.setPixmap(self.getIconForYesNo(Features[1]).pixmap(16, 16))
            self.label_session_level_tool.setText(str(Level))

            Docs0 = Docs[0] if (os.path.exists(Docs[0].replace("file://", ""))) else ""
            self.showDoc_Tool(Docs0, Docs[1])
        else:
            self.clearInfo_Tool()

        self.callback_checkGUI(row >= 0)

    @pyqtSlot()
    def slot_doubleClickedList(self):
        if self.m_ladish_only:
            self.addAppToLADISH()
        else:
            self.startApp()

    def saveSettings(self):
        if self.settings():
            self.settings().setValue("SplitterDAW", self.splitter_DAW.saveState())
            self.settings().setValue("SplitterHost", self.splitter_Host.saveState())
            self.settings().setValue("SplitterInstrument", self.splitter_Instrument.saveState())
            self.settings().setValue("SplitterBristol", self.splitter_Bristol.saveState())
            self.settings().setValue("SplitterPlugin", self.splitter_Plugin.saveState())
            self.settings().setValue("SplitterEffect", self.splitter_Effect.saveState())
            self.settings().setValue("SplitterTool", self.splitter_Tool.saveState())

        QIcon.setThemeName(self.m_lastThemeName)

    def loadSettings(self):
        if self.settings() and self.settings().contains("SplitterPlugin"):
            self.splitter_DAW.restoreState(self.settings().value("SplitterDAW", ""))
            self.splitter_Host.restoreState(self.settings().value("SplitterHost", ""))
            self.splitter_Instrument.restoreState(self.settings().value("SplitterInstrument", ""))
            self.splitter_Bristol.restoreState(self.settings().value("SplitterBristol", ""))
            self.splitter_Plugin.restoreState(self.settings().value("SplitterPlugin", ""))
            self.splitter_Effect.restoreState(self.settings().value("SplitterEffect", ""))
            self.splitter_Tool.restoreState(self.settings().value("SplitterTool", ""))
        else: # First-Run?
            self.splitter_DAW.setSizes([500, 200])
            self.splitter_Host.setSizes([500, 200])
            self.splitter_Instrument.setSizes([500, 200])
            self.splitter_Bristol.setSizes([500, 200])
            self.splitter_Plugin.setSizes([500, 200])
            self.splitter_Effect.setSizes([500, 200])
            self.splitter_Tool.setSizes([500, 200])

    # ----------------------------------------
    # Callbacks

    def callback_checkGUI(self, selected):
        if self.parentR():
            self.parentR().callback_checkGUI(selected)

    def callback_getProjectFolder(self):
        if self.parentR():
            return self.parentR().callback_getProjectFolder()
        return HOME

    def callback_getAppBus(self):
        return self.parentR().callback_getAppBus()

    def callback_getBPM(self):
        return self.parentR().callback_getBPM()

    def callback_getSampleRate(self):
        return self.parentR().callback_getSampleRate()

    def callback_isLadishRoom(self):
        return self.parentR().callback_isLadishRoom()

#--------------- main ------------------
if __name__ == '__main__':
    import dbus
    from signal import signal, SIG_IGN, SIGUSR1
    from PyQt5.QtWidgets import QApplication
    import jacklib, ui_claudia_launcher_app

    # DBus connections
    class DBus(object):
        __slots__ = [
            'loopBus',
            'controlBus',
            'studioBus',
            'appBus',
            ]

    DBus = DBus()

    class ClaudiaLauncherApp(QMainWindow, ui_claudia_launcher_app.Ui_ClaudiaLauncherApp):
        def __init__(self, parent=None):
            QMainWindow.__init__(self, parent)
            self.setupUi(self)

            self.settings = QSettings("Cadence", "Claudia-Launcher")
            self.launcher.setCallbackApp(self, self.settings, False)
            self.loadSettings()

            self.test_url = True
            self.test_selected = False
            self.studio_root_folder = HOME

            # Check for JACK
            self.jack_client = jacklib.client_open("klaudia", jacklib.JackNoStartServer, None)
            if not self.jack_client:
                QTimer.singleShot(0, self.slot_showJackError)
                return

            # Set-up GUI
            self.b_start.setIcon(self.getIcon("go-next"))
            self.b_add.setIcon(self.getIcon("list-add"))
            self.b_refresh.setIcon(self.getIcon("view-refresh"))
            self.b_open.setIcon(self.getIcon("document-open"))
            self.b_start.setEnabled(False)
            self.b_add.setEnabled(False)

            self.le_url.setText(self.studio_root_folder)
            self.co_sample_rate.addItem(str(self.getJackSampleRate()))
            self.sb_bpm.setValue(self.getJackBPM())

            self.refreshStudioList()

            if DBus.controlBus:
                self.slot_enableLADISH(True)
            else:
                for iPATH in PATH:
                    if os.path.exists(os.path.join(iPATH, "ladishd")):
                        break
                else:
                    self.slot_enableLADISH(False)

            self.b_start.clicked.connect(self.slot_startApp)
            self.b_add.clicked.connect(self.slot_addAppToLADISH)
            self.b_refresh.clicked.connect(self.slot_refreshStudioList)

            self.co_ladi_room.currentIndexChanged[int].connect(self.slot_checkSelectedRoom)
            self.groupLADISH.toggled.connect(self.slot_enableLADISH)

            self.le_url.textChanged.connect(self.slot_checkFolderUrl)
            self.b_open.clicked.connect(self.slot_getAndSetPath)

        def getIcon(self, icon):
            return self.launcher.getIcon(icon)

        def getJackBPM(self):
            pos = jacklib.jack_position_t()
            pos.valid = 0
            jacklib.transport_query(self.jack_client, jacklib.pointer(pos))

            if pos.valid & jacklib.JackPositionBBT:
                return pos.beats_per_minute
            return 120.0

        def getJackSampleRate(self):
            return jacklib.get_sample_rate(self.jack_client)

        def refreshStudioList(self):
            self.co_ladi_room.clear()
            self.co_ladi_room.addItem("<Studio Root>")
            if DBus.controlBus:
                studio_bus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Studio")
                studio_list_dump = studio_bus.GetRoomList()
                for i in range(len(studio_list_dump)):
                    self.co_ladi_room.addItem("%s - %s" % (
                    str(studio_list_dump[i][0]).replace("/org/ladish/Room", ""), studio_list_dump[i][1]['name']))

        # ----------------------------------------
        # Callbacks

        def callback_checkGUI(self, test_selected=None):
            if test_selected != None:
                self.test_selected = test_selected

            if self.test_url and self.test_selected:
                self.b_add.setEnabled(bool(DBus.controlBus))
                self.b_start.setEnabled(True)
            else:
                self.b_add.setEnabled(False)
                self.b_start.setEnabled(False)

        def callback_getProjectFolder(self):
            return self.le_url.text()

        def callback_getAppBus(self):
            return DBus.appBus

        def callback_getBPM(self):
            return self.getJackBPM()

        def callback_getSampleRate(self):
            return self.getJackSampleRate()

        def callback_isLadishRoom(self):
            return not self.le_url.isEnabled()

        # ----------------------------------------

        @pyqtSlot(int)
        def slot_checkSelectedRoom(self, co_n):
            if co_n == -1 or not DBus.controlBus:
                pass
            elif co_n == 0:
                DBus.studioBus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Studio")
                DBus.appBus = dbus.Interface(DBus.studioBus, 'org.ladish.AppSupervisor')
                self.b_open.setEnabled(True)
                self.le_url.setEnabled(True)
                self.le_url.setText(self.studio_root_folder)
            else:
                room_number = self.co_ladi_room.currentText().split(" ")[0]
                room_name = "/org/ladish/Room" + room_number
                DBus.studioBus = DBus.loopBus.get_object("org.ladish", room_name)
                DBus.appBus = dbus.Interface(DBus.studioBus, 'org.ladish.AppSupervisor')
                room_properties = DBus.studioBus.GetProjectProperties()
                if len(room_properties[1]) > 0:
                    self.b_open.setEnabled(False)
                    self.le_url.setEnabled(False)
                    self.le_url.setText(room_properties[1]['dir'])
                else:
                    self.b_open.setEnabled(True)
                    self.le_url.setEnabled(True)
                    self.studio_root_folder = self.le_url.text()

        @pyqtSlot(str)
        def slot_checkFolderUrl(self, url):
            if os.path.exists(url):
                self.test_url = True
                if self.le_url.isEnabled():
                    self.studio_root_folder = url
            else:
                self.test_url = False
            self.callback_checkGUI()

        @pyqtSlot(bool)
        def slot_enableLADISH(self, yesno):
            self.groupLADISH.setCheckable(False)

            if yesno:
                try:
                    DBus.controlBus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Control")
                    self.groupLADISH.setTitle(self.tr("LADISH is enabled"))
                except:
                    self.groupLADISH.setEnabled(False)
                    self.groupLADISH.setTitle(self.tr("LADISH is sick"))
                    return

                DBus.studioBus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Studio")
                DBus.appBus = dbus.Interface(DBus.studioBus, 'org.ladish.AppSupervisor')

                self.refreshStudioList()
                self.callback_checkGUI()

            else:
                self.groupLADISH.setEnabled(False)
                self.groupLADISH.setTitle(self.tr("LADISH is not available"))

        @pyqtSlot()
        def slot_startApp(self):
            self.launcher.startApp()

        @pyqtSlot()
        def slot_addAppToLADISH(self):
            self.launcher.addAppToLADISH()

        @pyqtSlot()
        def slot_getAndSetPath(self):
            getAndSetPath(self, self.le_url.text(), self.le_url)

        @pyqtSlot()
        def slot_refreshStudioList(self):
            self.refreshStudioList()

        @pyqtSlot()
        def slot_showJackError(self):
            QMessageBox.critical(self, self.tr("Error"),
                self.tr("JACK is not started!\nPlease start it first, then re-run Claudia-Launcher again."))
            self.close()

        def saveSettings(self):
            self.settings.setValue("Geometry", self.saveGeometry())
            self.launcher.saveSettings()

        def loadSettings(self):
            self.restoreGeometry(self.settings.value("Geometry", b""))
            self.launcher.loadSettings()

        def closeEvent(self, event):
            self.saveSettings()
            if self.jack_client:
                jacklib.client_close(self.jack_client)
            QMainWindow.closeEvent(self, event)

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Claudia-Launcher")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/claudia-launcher.svg"))

    # Do not close on SIGUSR1
    signal(SIGUSR1, SIG_IGN)

    # Connect to DBus
    DBus.loopBus = dbus.SessionBus()

    if "org.ladish" in DBus.loopBus.list_names():
        DBus.controlBus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Control")
        DBus.studioBus = DBus.loopBus.get_object("org.ladish", "/org/ladish/Studio")
        DBus.appBus = dbus.Interface(DBus.studioBus, "org.ladish.AppSupervisor")
    else:
        DBus.controlBus = None
        DBus.studioBus = None
        DBus.appBus = None

    # Show GUI
    gui = ClaudiaLauncherApp()
    gui.show()

    # App-Loop
    sys.exit(app.exec_())

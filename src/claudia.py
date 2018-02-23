#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LADISH frontend
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

from time import ctime

if True:
    from PyQt5.QtCore import QPointF
    from PyQt5.QtWidgets import QAction, QApplication, QCheckBox, QHBoxLayout, QVBoxLayout, QTableWidgetItem, QTreeWidgetItem
else:
    from PyQt4.QtCore import QPointF
    from PyQt4.QtGui import QAction, QApplication, QCheckBox, QHBoxLayout, QVBoxLayout, QTableWidgetItem, QTreeWidgetItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import claudia_launcher
import systray
import ui_claudia
import ui_claudia_studioname
import ui_claudia_studiolist
import ui_claudia_createroom
import ui_claudia_projectname
import ui_claudia_projectproperties
import ui_claudia_runcustom
from shared_canvasjack import *
from shared_settings import *

# ------------------------------------------------------------------------------------------------------------
# Try Import DBus

try:
    import dbus
    from dbus.mainloop.pyqt5 import DBusQtMainLoop
    haveDBus = True
except:
    haveDBus = False

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    from PyQt5.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Static Variables

# NOTE - set to true when supported
USE_CLAUDIA_ADD_NEW = True

# internal indexes
iConnId     = 0
iConnOutput = 1
iConnInput  = 2

iAppCommand  = 0
iAppName     = 1
iAppTerminal = 2
iAppLevel    = 3
iAppActive   = 4

iAppPropName        = 0
iAppPropDescription = 1
iAppPropNotes       = 2
iAppPropSaveNow     = 3

iItemPropNumber   = 0
iItemPropName     = 1
iItemPropActive   = 2
iItemPropTerminal = 3
iItemPropLevel    = 4

iItemPropRoomPath = 0
iItemPropRoomName = 1

# jackdbus indexes
iGraphVersion    = 0
iJackClientId    = 1
iJackClientName  = 2
iJackPortId      = 3
iJackPortName    = 4
iJackPortNewName = 5
iJackPortFlags   = 5
iJackPortType    = 6

iRenamedId      = 1
iRenamedOldName = 2
iRenamedNewName = 3

iSourceClientId   = 1
iSourceClientName = 2
iSourcePortId     = 3
iSourcePortName   = 4
iTargetClientId   = 5
iTargetClientName = 6
iTargetPortId     = 7
iTargetPortName   = 8
iJackConnId       = 9

# ladish indexes
iStudioListName = 0
iStudioListDict = 1

iStudioRenamedName = 0

iRoomAppearedPath = 0
iRoomAppearedDict = 1

iProjChangedId   = 0
iProjChangedDict = 1

iAppChangedNumber   = 1
iAppChangedName     = 2
iAppChangedActive   = 3
iAppChangedTerminal = 4
iAppChangedLevel    = 5

# internal defines
ITEM_TYPE_NULL       = 0
ITEM_TYPE_STUDIO     = 1
ITEM_TYPE_STUDIO_APP = 2
ITEM_TYPE_ROOM       = 3
ITEM_TYPE_ROOM_APP   = 4

# C defines
JACKDBUS_PORT_FLAG_INPUT       = 0x01
JACKDBUS_PORT_FLAG_OUTPUT      = 0x02
JACKDBUS_PORT_FLAG_PHYSICAL    = 0x04
JACKDBUS_PORT_FLAG_CAN_MONITOR = 0x08
JACKDBUS_PORT_FLAG_TERMINAL    = 0x10

JACKDBUS_PORT_TYPE_AUDIO = 0
JACKDBUS_PORT_TYPE_MIDI  = 1

GRAPH_DICT_OBJECT_TYPE_GRAPH  = 0
GRAPH_DICT_OBJECT_TYPE_CLIENT = 1
GRAPH_DICT_OBJECT_TYPE_PORT   = 2
GRAPH_DICT_OBJECT_TYPE_CONNECTION = 3

URI_A2J_PORT       = "http://ladish.org/ns/a2j"
URI_CANVAS_WIDTH   = "http://ladish.org/ns/canvas/width"
URI_CANVAS_HEIGHT  = "http://ladish.org/ns/canvas/height"
URI_CANVAS_X       = "http://ladish.org/ns/canvas/x"
URI_CANVAS_Y       = "http://ladish.org/ns/canvas/y"
URI_CANVAS_SPLIT   = "http://kxstudio.sf.net/ns/canvas/split"
URI_CANVAS_X_SPLIT = "http://kxstudio.sf.net/ns/canvas/x_split"
URI_CANVAS_Y_SPLIT = "http://kxstudio.sf.net/ns/canvas/y_split"
URI_CANVAS_ICON    = "http://kxstudio.sf.net/ns/canvas/icon"

DEFAULT_CANVAS_WIDTH  = 3100
DEFAULT_CANVAS_HEIGHT = 2400

RECENT_PROJECTS_STORE_MAX_ITEMS = 50

# ------------------------------------------------------------------------------------------------------------
# Set default project folder

DEFAULT_PROJECT_FOLDER = os.path.join(HOME, "ladish-projects")
setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)

# ------------------------------------------------------------------------------------------------------------
# Studio Name Dialog

class StudioNameW(QDialog):
    NEW = 1
    RENAME = 2
    SAVE_AS = 3

    def __init__(self, parent, mode):
        QDialog.__init__(self, parent)
        self.ui = ui_claudia_studioname.Ui_StudioNameW()
        self.ui.setupUi(self)

        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if mode == self.NEW:
            self.setWindowTitle(self.tr("New studio"))
        elif mode == self.RENAME:
            self.setWindowTitle(self.tr("Rename studio"))
        elif mode == self.SAVE_AS:
            self.setWindowTitle(self.tr("Save studio as"))

        self.fMode = mode
        self.fStudioList = []

        if mode == self.RENAME and bool(gDBus.ladish_control.IsStudioLoaded()):
            currentName = str(gDBus.ladish_studio.GetName())
            self.fStudioList.append(currentName)
            self.ui.le_name.setText(currentName)

        studioList = gDBus.ladish_control.GetStudioList()
        for studio in studioList:
            self.fStudioList.append(str(studio[iStudioListName]))

        self.accepted.connect(self.slot_setReturn)
        self.ui.le_name.textChanged.connect(self.slot_checkText)

        self.fRetStudioName = ""

    @pyqtSlot(str)
    def slot_checkText(self, text):
        if self.fMode == self.SAVE_AS:
            check = bool(text)
        else:
            check = bool(text and text not in self.fStudioList)

        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.fRetStudioName = self.ui.le_name.text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Studio List Dialog
class StudioListW(QDialog, ui_claudia_studiolist.Ui_StudioListW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
        self.tableWidget.setColumnWidth(0, 125)

        index = 0
        studio_list = gDBus.ladish_control.GetStudioList()
        for studio in studio_list:
            name = str(studio[iStudioListName])
            date = ctime(float(studio[iStudioListDict]["Modification Time"]))

            w_name = QTableWidgetItem(name)
            w_date = QTableWidgetItem(date)
            self.tableWidget.insertRow(index)
            self.tableWidget.setItem(index, 0, w_name)
            self.tableWidget.setItem(index, 1, w_date)

            index += 1

        self.accepted.connect(self.slot_setReturn)
        self.tableWidget.cellDoubleClicked.connect(self.accept)
        self.tableWidget.currentCellChanged.connect(self.slot_checkSelection)

        if self.tableWidget.rowCount() > 0:
            self.tableWidget.setCurrentCell(0, 0)

        self.ret_studio_name = ""

    @pyqtSlot(int)
    def slot_checkSelection(self, row):
        check = bool(row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.tableWidget.rowCount() >= 0:
            self.ret_studio_name = self.tableWidget.item(self.tableWidget.currentRow(), 0).text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Create Room Dialog
class CreateRoomW(QDialog, ui_claudia_createroom.Ui_CreateRoomW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        templates_list = gDBus.ladish_control.GetRoomTemplateList()
        for template_name, template_dict in templates_list:
            self.lw_templates.addItem(template_name)

        self.accepted.connect(self.slot_setReturn)
        self.le_name.textChanged.connect(self.slot_checkText)

        if self.lw_templates.count() > 0:
            self.lw_templates.setCurrentRow(0)

        self.ret_room_name = ""
        self.ret_room_template = ""

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text and self.lw_templates.currentRow() >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.lw_templates.count() > 0:
            self.ret_room_name = self.le_name.text()
            self.ret_room_template = self.lw_templates.currentItem().text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Project Name Dialog
class ProjectNameW(QDialog, ui_claudia_projectname.Ui_ProjectNameW):
    NEW = 1
    SAVE_AS = 2

    def __init__(self, parent, mode, proj_folder, path="", name=""):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if mode == self.NEW:
            self.setWindowTitle(self.tr("New project"))
        elif mode == self.SAVE_AS:
            self.setWindowTitle(self.tr("Save project as"))
            self.le_path.setText(path)
            self.le_name.setText(name)
            self.checkText(path, name)

        self.m_proj_folder = proj_folder

        self.accepted.connect(self.slot_setReturn)
        self.b_open.clicked.connect(self.slot_checkFolder)
        self.le_path.textChanged.connect(self.slot_checkText_path)
        self.le_name.textChanged.connect(self.slot_checkText_name)

        self.ret_project_name = ""
        self.ret_project_path = ""

    def checkText(self, name, path):
        check = bool(name and path and os.path.exists(path) and os.path.isdir(path))
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_checkFolder(self):
        # Create default project folder if the project has not been set yet
        if not self.le_path.text():
            if not os.path.exists(self.m_proj_folder):
                os.mkdir(self.m_proj_folder)

        if self.le_path.text():
            proj_path = self.le_path.text()
        else:
            proj_path = self.m_proj_folder

        getAndSetPath(self, proj_path, self.le_path)

    @pyqtSlot(str)
    def slot_checkText_name(self, text):
        self.checkText(text, self.le_path.text())

    @pyqtSlot(str)
    def slot_checkText_path(self, text):
        self.checkText(self.le_name.text(), text)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_project_name = self.le_name.text()
        self.ret_project_path = self.le_path.text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Project Properties Dialog
class ProjectPropertiesW(QDialog, ui_claudia_projectproperties.Ui_ProjectPropertiesW):
    def __init__(self, parent, name, description, notes):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.m_default_name = name
        self.m_last_name = name

        self.accepted.connect(self.slot_setReturn)
        self.le_name.textChanged.connect(self.slot_checkText_name)
        self.cb_save_now.clicked.connect(self.slot_checkSaveNow)

        self.le_name.setText(name)
        self.le_description.setText(description)
        self.le_notes.setPlainText(notes)

        self.ret_obj = None

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_obj = [None, None, None, None]
        self.ret_obj[iAppPropName] = self.le_name.text()
        self.ret_obj[iAppPropDescription] = self.le_description.text()
        self.ret_obj[iAppPropNotes] = self.le_notes.toPlainText() # plainText()
        self.ret_obj[iAppPropSaveNow] = self.cb_save_now.isChecked()

    @pyqtSlot(bool)
    def slot_checkSaveNow(self, save):
        if save:
            self.le_name.setText(self.m_last_name)
        else:
            self.m_last_name = self.le_name.text()
            self.le_name.setText(self.m_default_name)

    @pyqtSlot(str)
    def slot_checkText_name(self, text):
        check = bool(text)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Run Custom App Dialog
class RunCustomW(QDialog, ui_claudia_runcustom.Ui_RunCustomW):
    def __init__(self, parent, isRoom, app_obj=None):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(bool(app_obj))

        if app_obj:
            self.le_command.setText(app_obj[iAppCommand])
            self.le_name.setText(app_obj[iAppName])
            self.cb_terminal.setChecked(app_obj[iAppTerminal])

            level = app_obj[iAppLevel]
            if level == "0":
                self.rb_level_0.setChecked(True)
            elif level == "1":
                self.rb_level_1.setChecked(True)
            elif level == "lash":
                self.rb_level_lash.setChecked(True)
            elif level == "jacksession":
                self.rb_level_js.setChecked(True)
            else:
                self.rb_level_0.setChecked(True)

            if app_obj[iAppActive]:
                self.le_command.setEnabled(False)
                self.cb_terminal.setEnabled(False)
                self.rb_level_0.setEnabled(False)
                self.rb_level_1.setEnabled(False)
                self.rb_level_lash.setEnabled(False)
                self.rb_level_js.setEnabled(False)
        else:
            self.rb_level_0.setChecked(True)

        if not isRoom:
            self.rb_level_lash.setEnabled(False)
            self.rb_level_js.setEnabled(False)

        self.accepted.connect(self.slot_setReturn)
        self.le_command.textChanged.connect(self.slot_checkText)

        self.ret_app_obj = None

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.rb_level_0.isChecked():
            level = "0"
        elif self.rb_level_1.isChecked():
            level = "1"
        elif self.rb_level_lash.isChecked():
            level = "lash"
        elif self.rb_level_js.isChecked():
            level = "jacksession"
        else:
            return

        self.ret_app_obj = [None, None, None, None, None]
        self.ret_app_obj[iAppCommand] = self.le_command.text()
        self.ret_app_obj[iAppName] = self.le_name.text()
        self.ret_app_obj[iAppTerminal] = self.cb_terminal.isChecked()
        self.ret_app_obj[iAppLevel] = level
        self.ret_app_obj[iAppActive] = False

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Add Application Dialog
class ClaudiaLauncherW(QDialog):
    def __init__(self, parent, appBus, proj_folder, is_room, bpm, sample_rate):
        QDialog.__init__(self, parent)

        self.launcher  = claudia_launcher.ClaudiaLauncher(self)
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Close, Qt.Horizontal, self)
        self.checkBox  = QCheckBox(self)
        self.checkBox.setText(self.tr("Auto-close"))
        self.checkBox.setChecked(True)

        self.layoutV = QVBoxLayout(self)
        self.layoutH = QHBoxLayout()

        #if QDialogButtonBox.ButtonLayout:
        self.layoutH.addWidget(self.checkBox)
        self.layoutH.addWidget(self.buttonBox)
        #else:
            #self.layoutH.addWidget(self.buttonBox)
            #self.layoutH.addWidget(self.checkBox)

        self.layoutV.addWidget(self.launcher)
        self.layoutV.addLayout(self.layoutH)

        self.settings = QSettings("Cadence", "Claudia-Launcher")
        self.launcher.setCallbackApp(self, self.settings, True)
        self.loadSettings()
        self.setWindowTitle("Claudia Launcher")

        self.m_appBus = appBus
        self.m_proj_folder = proj_folder
        self.m_is_room = is_room
        self.m_bpm = bpm
        self.m_sampleRate = sample_rate

        self.test_url = True
        self.test_selected = False

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.buttonBox.button(QDialogButtonBox.Ok).clicked.connect(self.slot_addAppToLADISH)
        self.buttonBox.button(QDialogButtonBox.Close).clicked.connect(self.reject)

    # ----------------------------------------
    # Callbacks

    def callback_checkGUI(self, test_selected=None):
        if test_selected != None:
            self.test_selected = test_selected

        if self.test_url and self.test_selected:
            self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
        else:
            self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    def callback_getProjectFolder(self):
        return self.m_proj_folder

    def callback_getAppBus(self):
        return self.m_appBus

    def callback_getBPM(self):
        if self.m_bpm < 30:
            return 120.0
        else:
            return self.m_bpm

    def callback_getSampleRate(self):
        return self.m_sampleRate

    def callback_isLadishRoom(self):
        return self.m_is_room

    # ----------------------------------------

    @pyqtSlot()
    def slot_addAppToLADISH(self):
        self.launcher.addAppToLADISH()

        if self.checkBox.isChecked():
            self.accept()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.launcher.saveSettings()

    def loadSettings(self):
        if self.settings.contains("Geometry"):
            self.restoreGeometry(self.settings.value("Geometry", b""))
        else:
            self.resize(850, 500)
        self.launcher.loadSettings()

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Claudia Main Window

class ClaudiaMainW(AbstractCanvasJackClass):
    DBusCrashCallback = pyqtSignal(str)
    DBusServerStartedCallback = pyqtSignal()
    DBusServerStoppedCallback = pyqtSignal()
    DBusClientAppearedCallback = pyqtSignal(int, str)
    DBusClientDisappearedCallback = pyqtSignal(int)
    DBusClientRenamedCallback = pyqtSignal(int, str)
    DBusPortAppearedCallback = pyqtSignal(int, int, str, int, int)
    DBusPortDisppearedCallback = pyqtSignal(int)
    DBusPortRenamedCallback = pyqtSignal(int, str)
    DBusPortsConnectedCallback = pyqtSignal(int, int, int)
    DBusPortsDisconnectedCallback = pyqtSignal(int)
    DBusStudioAppearedCallback = pyqtSignal()
    DBusStudioDisappearedCallback = pyqtSignal()
    DBusQueueExecutionHaltedCallback = pyqtSignal()
    DBusCleanExitCallback = pyqtSignal()
    DBusStudioStartedCallback = pyqtSignal()
    DBusStudioStoppedCallback = pyqtSignal()
    DBusStudioRenamedCallback = pyqtSignal(str)
    DBusStudioCrashedCallback = pyqtSignal()
    DBusRoomAppearedCallback = pyqtSignal(str, str)
    DBusRoomDisappearedCallback = pyqtSignal(str)
    DBusRoomChangedCallback = pyqtSignal()
    DBusProjectPropertiesChanged = pyqtSignal(str, str)
    DBusAppAdded2Callback = pyqtSignal(str, int, str, bool, bool, str)
    DBusAppRemovedCallback = pyqtSignal(str, int)
    DBusAppStateChanged2Callback = pyqtSignal(str, int, str, bool, bool, str)

    def __init__(self, parent=None):
        AbstractCanvasJackClass.__init__(self, "Claudia", ui_claudia.Ui_ClaudiaMainW, parent)

        self.m_lastItemType = None
        self.m_lastRoomPath = None

        self.m_crashedJACK   = False
        self.m_crashedLADISH = False

        self.loadSettings(True)

        # -------------------------------------------------------------
        # Set-up GUI

        setIcons(self, ["canvas", "jack", "transport"])

        self.ui.act_studio_new.setIcon(getIcon("document-new"))
        self.ui.menu_studio_load.setIcon(getIcon("document-open"))
        self.ui.act_studio_start.setIcon(getIcon("media-playback-start"))
        self.ui.act_studio_stop.setIcon(getIcon("media-playback-stop"))
        self.ui.act_studio_rename.setIcon(getIcon("edit-rename"))
        self.ui.act_studio_save.setIcon(getIcon("document-save"))
        self.ui.act_studio_save_as.setIcon(getIcon("document-save-as"))
        self.ui.act_studio_unload.setIcon(getIcon("window-close"))
        self.ui.menu_studio_delete.setIcon(getIcon("edit-delete"))
        self.ui.b_studio_new.setIcon(getIcon("document-new"))
        self.ui.b_studio_load.setIcon(getIcon("document-open"))
        self.ui.b_studio_save.setIcon(getIcon("document-save"))
        self.ui.b_studio_save_as.setIcon(getIcon("document-save-as"))
        self.ui.b_studio_start.setIcon(getIcon("media-playback-start"))
        self.ui.b_studio_stop.setIcon(getIcon("media-playback-stop"))

        self.ui.act_room_create.setIcon(getIcon("list-add"))
        self.ui.menu_room_delete.setIcon(getIcon("edit-delete"))

        self.ui.act_project_new.setIcon(getIcon("document-new"))
        self.ui.menu_project_load.setIcon(getIcon("document-open"))
        self.ui.act_project_save.setIcon(getIcon("document-save"))
        self.ui.act_project_save_as.setIcon(getIcon("document-save-as"))
        self.ui.act_project_unload.setIcon(getIcon("window-close"))
        self.ui.act_project_properties.setIcon(getIcon("edit-rename"))
        self.ui.b_project_new.setIcon(getIcon("document-new"))
        self.ui.b_project_load.setIcon(getIcon("document-open"))
        self.ui.b_project_save.setIcon(getIcon("document-save"))
        self.ui.b_project_save_as.setIcon(getIcon("document-save-as"))

        self.ui.act_app_add_new.setIcon(getIcon("list-add"))
        self.ui.act_app_run_custom.setIcon(getIcon("system-run"))

        self.ui.act_tools_reactivate_ladishd.setIcon(getIcon("view-refresh"))
        self.ui.act_quit.setIcon(getIcon("application-exit"))
        self.ui.act_settings_configure.setIcon(getIcon("configure"))

        self.ui.cb_buffer_size.clear()
        self.ui.cb_sample_rate.clear()

        for bufferSize in BUFFER_SIZE_LIST:
            self.ui.cb_buffer_size.addItem(str(bufferSize))

        for sampleRate in SAMPLE_RATE_LIST:
            self.ui.cb_sample_rate.addItem(str(sampleRate))

        # -------------------------------------------------------------
        # Set-up Systray

        if self.fSavedSettings["Main/UseSystemTray"]:
            self.systray = systray.GlobalSysTray(self, "Claudia", "claudia")

            self.systray.addAction("studio_new", self.tr("New Studio..."))
            self.systray.addSeparator("sep1")
            self.systray.addAction("studio_start", self.tr("Start Studio"))
            self.systray.addAction("studio_stop", self.tr("Stop Studio"))
            self.systray.addSeparator("sep2")
            self.systray.addAction("studio_save", self.tr("Save Studio"))
            self.systray.addAction("studio_save_as", self.tr("Save Studio As..."))
            self.systray.addAction("studio_rename", self.tr("Rename Studio..."))
            self.systray.addAction("studio_unload", self.tr("Unload Studio"))
            self.systray.addSeparator("sep3")
            self.systray.addMenu("tools", self.tr("Tools"))
            self.systray.addMenuAction("tools", "tools_configure_jack", self.tr("Configure JACK"))
            self.systray.addMenuAction("tools", "tools_render", self.tr("JACK Render"))
            self.systray.addMenuAction("tools", "tools_logs", self.tr("Logs"))
            self.systray.addMenuSeparator("tools", "tools_sep")
            self.systray.addMenuAction("tools", "tools_clear_xruns", self.tr("Clear Xruns"))
            self.systray.addAction("configure", self.tr("Configure Claudia"))

            self.systray.setActionIcon("studio_new", "document-new")
            self.systray.setActionIcon("studio_start", "media-playback-start")
            self.systray.setActionIcon("studio_stop", "media-playback-stop")
            self.systray.setActionIcon("studio_save", "document-save")
            self.systray.setActionIcon("studio_save_as", "document-save-as")
            self.systray.setActionIcon("studio_rename", "edit-rename")
            self.systray.setActionIcon("studio_unload", "dialog-close")
            self.systray.setActionIcon("tools_configure_jack", "configure")
            self.systray.setActionIcon("tools_render", "media-record")
            self.systray.setActionIcon("tools_clear_xruns", "edit-clear")
            self.systray.setActionIcon("configure", "configure")

            self.systray.connect("studio_new", self.slot_studio_new)
            self.systray.connect("studio_start", self.slot_studio_start)
            self.systray.connect("studio_stop", self.slot_studio_stop)
            self.systray.connect("studio_save", self.slot_studio_save)
            self.systray.connect("studio_save_as", self.slot_studio_save_as)
            self.systray.connect("studio_rename", self.slot_studio_rename)
            self.systray.connect("studio_unload", self.slot_studio_unload)
            self.systray.connect("tools_configure_jack", self.slot_showJackSettings)
            self.systray.connect("tools_render", self.slot_showRender)
            self.systray.connect("tools_logs", self.slot_showLogs)
            self.systray.connect("tools_clear_xruns", self.slot_JackClearXruns)
            self.systray.connect("configure", self.slot_configureClaudia)

            self.systray.setToolTip("LADISH Frontend")
            self.systray.show()

        else:
            self.systray = None

        # -------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self.ui.graphicsView)
        self.ui.graphicsView.setScene(self.scene)
        self.ui.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.fSavedSettings["Canvas/Antialiasing"] == patchcanvas.ANTIALIASING_FULL))
        if self.fSavedSettings["Canvas/UseOpenGL"] and hasGL:
            self.ui.graphicsView.setViewport(QGLWidget(self.ui.graphicsView))
            self.ui.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.fSavedSettings["Canvas/HighQualityAntialiasing"])

        pOptions = patchcanvas.options_t()
        pOptions.theme_name       = self.fSavedSettings["Canvas/Theme"]
        pOptions.auto_hide_groups = self.fSavedSettings["Canvas/AutoHideGroups"]
        pOptions.use_bezier_lines = self.fSavedSettings["Canvas/UseBezierLines"]
        pOptions.antialiasing     = self.fSavedSettings["Canvas/Antialiasing"]
        pOptions.eyecandy         = self.fSavedSettings["Canvas/EyeCandy"]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = True
        pFeatures.port_info    = True
        pFeatures.port_rename  = True
        pFeatures.handle_group_pos = False

        patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Claudia", self.scene, self.canvasCallback, DEBUG)

        patchcanvas.setCanvasSize(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        patchcanvas.setInitialPos(DEFAULT_CANVAS_WIDTH / 2, DEFAULT_CANVAS_HEIGHT / 2)
        self.ui.graphicsView.setSceneRect(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        # -------------------------------------------------------------
        # Set-up Canvas Preview

        self.ui.miniCanvasPreview.setRealParent(self)
        self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.ui.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        QTimer.singleShot(100, self.slot_miniCanvasInit)

        # -------------------------------------------------------------
        # Check DBus

        if gDBus.jack.IsStarted():
            self.jackStarted()
        else:
            self.jackStopped()

        if gDBus.a2j:
            if gDBus.a2j.is_started():
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.ui.act_tools_a2j_start.setEnabled(False)
            self.ui.act_tools_a2j_stop.setEnabled(False)
            self.ui.act_tools_a2j_export_hw.setEnabled(False)
            self.ui.menu_A2J_Bridge.setEnabled(False)

        if gDBus.ladish_control.IsStudioLoaded():
            self.studioLoaded()
            if gDBus.ladish_studio.IsStarted():
                self.studioStarted()
                self.initPorts()
            else:
                self.studioStopped()
        else:
            self.studioUnloaded()

        # -------------------------------------------------------------
        # Set-up Timers

        self.m_timer120 = self.startTimer(self.fSavedSettings["Main/RefreshInterval"])
        self.m_timer600 = self.startTimer(self.fSavedSettings["Main/RefreshInterval"] * 5)

        # -------------------------------------------------------------
        # Set-up Connections

        self.setCanvasConnections()
        self.setJackConnections(["jack", "transport", "misc"])

        self.ui.act_studio_new.triggered.connect(self.slot_studio_new)
        self.ui.act_studio_start.triggered.connect(self.slot_studio_start)
        self.ui.act_studio_stop.triggered.connect(self.slot_studio_stop)
        self.ui.act_studio_save.triggered.connect(self.slot_studio_save)
        self.ui.act_studio_save_as.triggered.connect(self.slot_studio_save_as)
        self.ui.act_studio_rename.triggered.connect(self.slot_studio_rename)
        self.ui.act_studio_unload.triggered.connect(self.slot_studio_unload)
        self.ui.act_tools_a2j_start.triggered.connect(self.slot_A2JBridgeStart)
        self.ui.act_tools_a2j_stop.triggered.connect(self.slot_A2JBridgeStop)
        self.ui.act_tools_a2j_export_hw.triggered.connect(self.slot_A2JBridgeExportHW)
        self.ui.b_studio_new.clicked.connect(self.slot_studio_new)
        self.ui.b_studio_load.clicked.connect(self.slot_studio_load_b)
        self.ui.b_studio_save.clicked.connect(self.slot_studio_save)
        self.ui.b_studio_save_as.clicked.connect(self.slot_studio_save_as)
        self.ui.b_studio_start.clicked.connect(self.slot_studio_start)
        self.ui.b_studio_stop.clicked.connect(self.slot_studio_stop)
        self.ui.menu_studio_load.aboutToShow.connect(self.slot_updateMenuStudioList_Load)
        self.ui.menu_studio_delete.aboutToShow.connect(self.slot_updateMenuStudioList_Delete)

        self.ui.act_room_create.triggered.connect(self.slot_room_create)
        self.ui.menu_room_delete.aboutToShow.connect(self.slot_updateMenuRoomList)

        self.ui.act_project_new.triggered.connect(self.slot_project_new)
        self.ui.act_project_save.triggered.connect(self.slot_project_save)
        self.ui.act_project_save_as.triggered.connect(self.slot_project_save_as)
        self.ui.act_project_unload.triggered.connect(self.slot_project_unload)
        self.ui.act_project_properties.triggered.connect(self.slot_project_properties)
        self.ui.b_project_new.clicked.connect(self.slot_project_new)
        self.ui.b_project_load.clicked.connect(self.slot_project_load)
        self.ui.b_project_save.clicked.connect(self.slot_project_save)
        self.ui.b_project_save_as.clicked.connect(self.slot_project_save_as)
        self.ui.menu_project_load.aboutToShow.connect(self.slot_updateMenuProjectList)

        self.ui.act_app_add_new.triggered.connect(self.slot_app_add_new)
        self.ui.act_app_run_custom.triggered.connect(self.slot_app_run_custom)

        self.ui.treeWidget.itemSelectionChanged.connect(self.slot_checkCurrentRoom)
        #self.ui.treeWidget.itemPressed.connect(self.slot_checkCurrentRoom)
        self.ui.treeWidget.itemDoubleClicked.connect(self.slot_doubleClickedAppList)
        self.ui.treeWidget.customContextMenuRequested.connect(self.slot_showAppListCustomMenu)

        self.ui.miniCanvasPreview.miniCanvasMoved.connect(self.slot_miniCanvasMoved)

        self.ui.graphicsView.horizontalScrollBar().valueChanged.connect(self.slot_horizontalScrollBarChanged)
        self.ui.graphicsView.verticalScrollBar().valueChanged.connect(self.slot_verticalScrollBarChanged)

        self.scene.sceneGroupMoved.connect(self.slot_canvasItemMoved)
        self.scene.scaleChanged.connect(self.slot_canvasScaleChanged)

        self.ui.act_settings_configure.triggered.connect(self.slot_configureClaudia)

        self.ui.act_help_about.triggered.connect(self.slot_aboutClaudia)
        self.ui.act_help_about_qt.triggered.connect(app.aboutQt)

        # org.freedesktop.DBus
        self.DBusCrashCallback.connect(self.slot_DBusCrashCallback)

        # org.jackaudio.JackControl
        self.DBusServerStartedCallback.connect(self.slot_DBusServerStartedCallback)
        self.DBusServerStoppedCallback.connect(self.slot_DBusServerStoppedCallback)

        # org.jackaudio.JackPatchbay
        self.DBusClientAppearedCallback.connect(self.slot_DBusClientAppearedCallback)
        self.DBusClientDisappearedCallback.connect(self.slot_DBusClientDisappearedCallback)
        self.DBusClientRenamedCallback.connect(self.slot_DBusClientRenamedCallback)
        self.DBusPortAppearedCallback.connect(self.slot_DBusPortAppearedCallback)
        self.DBusPortDisppearedCallback.connect(self.slot_DBusPortDisppearedCallback)
        self.DBusPortRenamedCallback.connect(self.slot_DBusPortRenamedCallback)
        self.DBusPortsConnectedCallback.connect(self.slot_DBusPortsConnectedCallback)
        self.DBusPortsDisconnectedCallback.connect(self.slot_DBusPortsDisconnectedCallback)

        # org.ladish.Control
        self.DBusStudioAppearedCallback.connect(self.slot_DBusStudioAppearedCallback)
        self.DBusStudioDisappearedCallback.connect(self.slot_DBusStudioDisappearedCallback)
        self.DBusQueueExecutionHaltedCallback.connect(self.slot_DBusQueueExecutionHaltedCallback)
        self.DBusCleanExitCallback.connect(self.slot_DBusCleanExitCallback)

        # org.ladish.Studio
        self.DBusStudioStartedCallback.connect(self.slot_DBusStudioStartedCallback)
        self.DBusStudioStoppedCallback.connect(self.slot_DBusStudioStoppedCallback)
        self.DBusStudioRenamedCallback.connect(self.slot_DBusStudioRenamedCallback)
        self.DBusStudioCrashedCallback.connect(self.slot_DBusStudioCrashedCallback)
        self.DBusRoomAppearedCallback.connect(self.slot_DBusRoomAppearedCallback)
        self.DBusRoomDisappearedCallback.connect(self.slot_DBusRoomDisappearedCallback)
        #self.DBusRoomChangedCallback.connect(self.slot_DBusRoomChangedCallback)

        # org.ladish.Room
        self.DBusProjectPropertiesChanged.connect(self.slot_DBusProjectPropertiesChanged)

        # org.ladish.AppSupervisor
        self.DBusAppAdded2Callback.connect(self.slot_DBusAppAdded2Callback)
        self.DBusAppRemovedCallback.connect(self.slot_DBusAppRemovedCallback)
        self.DBusAppStateChanged2Callback.connect(self.slot_DBusAppStateChanged2Callback)

        # JACK
        self.BufferSizeCallback.connect(self.slot_JackBufferSizeCallback)
        self.SampleRateCallback.connect(self.slot_JackSampleRateCallback)
        self.ShutdownCallback.connect(self.slot_JackShutdownCallback)

        # -------------------------------------------------------------
        # Set-up DBus

        gDBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword="dest", path_keyword="path",
            member_keyword="member", interface_keyword="interface", sender_keyword="sender")

        # -------------------------------------------------------------

    def canvasCallback(self, action, value1, value2, value_str):
        if action == patchcanvas.ACTION_GROUP_INFO:
            pass

        elif action == patchcanvas.ACTION_GROUP_RENAME:
            group_id = value1
            group_name = value_str
            gDBus.ladish_manager.RenameClient(group_id, group_name)

        elif action == patchcanvas.ACTION_GROUP_SPLIT:
            group_id = value1
            gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "true")

            patchcanvas.splitGroup(group_id)
            self.ui.miniCanvasPreview.update()

        elif action == patchcanvas.ACTION_GROUP_JOIN:
            group_id = value1
            gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "false")

            patchcanvas.joinGroup(group_id)
            self.ui.miniCanvasPreview.update()

        elif action == patchcanvas.ACTION_PORT_INFO:
            this_port_id = value1
            breakNow = False

            port_id    = 0
            port_flags = 0
            port_name  = ""
            port_type_jack = 0

            version, groups, conns = gDBus.patchbay.GetGraph(0)

            for group in groups:
                group_id, group_name, ports = group

                for port in ports:
                    port_id, port_name, port_flags, port_type_jack = port

                    if this_port_id == port_id:
                        breakNow = True
                        break

                if breakNow:
                    break

            else:
                return

            flags = []
            if port_flags & JACKDBUS_PORT_FLAG_INPUT:
                flags.append(self.tr("Input"))
            if port_flags & JACKDBUS_PORT_FLAG_OUTPUT:
                flags.append(self.tr("Output"))
            if port_flags & JACKDBUS_PORT_FLAG_PHYSICAL:
                flags.append(self.tr("Physical"))
            if port_flags & JACKDBUS_PORT_FLAG_CAN_MONITOR:
                flags.append(self.tr("Can Monitor"))
            if port_flags & JACKDBUS_PORT_FLAG_TERMINAL:
                flags.append(self.tr("Terminal"))

            flags_text = ""
            for flag in flags:
                if flags_text:
                    flags_text += " | "
                flags_text += flag

            if port_type_jack == JACKDBUS_PORT_TYPE_AUDIO:
                type_text = self.tr("Audio")
            elif port_type_jack == JACKDBUS_PORT_TYPE_MIDI:
                type_text = self.tr("MIDI")
            else:
                type_text = self.tr("Unknown")

            port_full_name = "%s:%s" % (group_name, port_name)

            info = self.tr(""
                           "<table>"
                           "<tr><td align='right'><b>Group ID:</b></td><td>&nbsp;%i</td></tr>"
                           "<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%s</td></tr>"
                           "<tr><td align='right'><b>Port ID:</b></td><td>&nbsp;%i</i></td></tr>"
                           "<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%s</td></tr>"
                           "<tr><td align='right'><b>Full Port Name:</b></td><td>&nbsp;%s</td></tr>"
                           "<tr><td colspan='2'>&nbsp;</td></tr>"
                           "<tr><td align='right'><b>Port Flags:</b></td><td>&nbsp;%s</td></tr>"
                           "<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%s</td></tr>"
                           "</table>"
            % (group_id, group_name, port_id, port_name, port_full_name, flags_text, type_text))

            QMessageBox.information(self, self.tr("Port Information"), info)

        elif action == patchcanvas.ACTION_PORT_RENAME:
            port_id = value1
            port_name = value_str
            gDBus.ladish_manager.RenamePort(port_id, port_name)

        elif action == patchcanvas.ACTION_PORTS_CONNECT:
            port_a = value1
            port_b = value2
            gDBus.patchbay.ConnectPortsByID(port_a, port_b)

        elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
            connection_id = value1
            gDBus.patchbay.DisconnectPortsByConnectionID(connection_id)

    def init_jack(self):
        self.fXruns = -1
        self.fNextSampleRate = 0.0

        self.fLastBPM = None
        self.fLastTransportState = None

        bufferSize = int(jacklib.get_buffer_size(gJack.client))
        sampleRate = int(jacklib.get_sample_rate(gJack.client))
        realtime = bool(int(jacklib.is_realtime(gJack.client)))

        self.ui_setBufferSize(bufferSize)
        self.ui_setSampleRate(sampleRate)
        self.ui_setRealTime(realtime)

        self.refreshDSPLoad()
        self.refreshTransport()
        self.refreshXruns()

        self.init_callbacks()

        jacklib.activate(gJack.client)

    def init_callbacks(self):
        jacklib.set_buffer_size_callback(gJack.client, self.JackBufferSizeCallback, None)
        jacklib.set_sample_rate_callback(gJack.client, self.JackSampleRateCallback, None)
        jacklib.on_shutdown(gJack.client, self.JackShutdownCallback, None)

    def init_studio(self):
        self.ui.treeWidget.clear()

        studio_item = QTreeWidgetItem(ITEM_TYPE_STUDIO)
        studio_item.setText(0, str(gDBus.ladish_studio.GetName()))
        self.ui.treeWidget.insertTopLevelItem(0, studio_item)
        self.ui.treeWidget.setCurrentItem(studio_item)

        self.m_lastItemType = ITEM_TYPE_STUDIO
        self.m_lastRoomPath = None

        self.init_apps()

    def init_apps(self):
        studio_iface = dbus.Interface(gDBus.ladish_studio, 'org.ladish.AppSupervisor')
        studio_item = self.ui.treeWidget.topLevelItem(0)

        graph_version, app_list = studio_iface.GetAll2()

        for app in app_list:
            number, name, active, terminal, level = app

            prop_obj = [None, None, None, None, None]
            prop_obj[iItemPropNumber]   = int(number)
            prop_obj[iItemPropName]     = str(name)
            prop_obj[iItemPropActive]   = bool(active)
            prop_obj[iItemPropTerminal] = bool(terminal)
            prop_obj[iItemPropLevel]    = str(level)

            text = "["
            if level.isdigit():
                text += "L%s" % level
            elif level == "jacksession":
                text += "JS"
            else:
                text += level.upper()
            text += "] "
            if not active:
                text += "(inactive) "
            text += name

            item = QTreeWidgetItem(ITEM_TYPE_STUDIO_APP)
            item.properties = prop_obj
            item.setText(0, text)
            studio_item.addChild(item)

        room_list = gDBus.ladish_studio.GetRoomList()

        for room in room_list:
            room_path, room_dict = room
            ladish_room = gDBus.bus.get_object("org.ladish", room_path)
            room_name   = ladish_room.GetName()

            room_app_iface = dbus.Interface(ladish_room, 'org.ladish.AppSupervisor')
            room_item = self.room_add(room_path, room_name)

            graph_version, app_list = room_app_iface.GetAll2()

            for app in app_list:
                number, name, active, terminal, level = app

                prop_obj = [None, None, None, None, None]
                prop_obj[iItemPropNumber] = int(number)
                prop_obj[iItemPropName] = str(name)
                prop_obj[iItemPropActive] = bool(active)
                prop_obj[iItemPropTerminal] = bool(terminal)
                prop_obj[iItemPropLevel] = str(level)

                text = "["
                if level.isdigit():
                    text += "L%s" % level
                elif level == "jacksession":
                    text += "JS"
                else:
                    text += level.upper()
                text += "] "
                if not active:
                    text += "(inactive) "
                text += name

                item = QTreeWidgetItem(ITEM_TYPE_ROOM_APP)
                item.properties = prop_obj
                item.setText(0, text)
                room_item.addChild(item)

        self.ui.treeWidget.expandAll()

    def initPorts(self):
        if not (gJack.client and gDBus.patchbay):
            return

        version, groups, conns = gDBus.patchbay.GetGraph(0)

        # Graph Ports
        for group in groups:
            group_id, group_name, ports = group
            self.canvas_add_group(int(group_id), str(group_name))

            for port in ports:
                port_id, port_name, port_flags, port_type_jack = port

                if port_flags & JACKDBUS_PORT_FLAG_INPUT:
                    port_mode = patchcanvas.PORT_MODE_INPUT
                elif port_flags & JACKDBUS_PORT_FLAG_OUTPUT:
                    port_mode = patchcanvas.PORT_MODE_OUTPUT
                else:
                    port_mode = patchcanvas.PORT_MODE_NULL

                if port_type_jack == JACKDBUS_PORT_TYPE_AUDIO:
                    port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
                elif port_type_jack == JACKDBUS_PORT_TYPE_MIDI:
                    if gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes":
                        port_type = patchcanvas.PORT_TYPE_MIDI_A2J
                    else:
                        port_type = patchcanvas.PORT_TYPE_MIDI_JACK
                else:
                    port_type = patchcanvas.PORT_TYPE_NULL

                self.canvas_add_port(int(group_id), int(port_id), str(port_name), port_mode, port_type)

        # Graph Connections
        for conn in conns:
            source_group_id, source_group_name, source_port_id, source_port_name, target_group_id, target_group_name, target_port_id, target_port_name, conn_id = conn
            self.canvas_connect_ports(int(conn_id), int(source_port_id), int(target_port_id))

        QTimer.singleShot(1000 if (self.fSavedSettings['Canvas/EyeCandy']) else 0, self.ui.miniCanvasPreview.update)

    def room_add(self, room_path, room_name):
        room_index  = int(room_path.replace("/org/ladish/Room", ""))
        room_object = gDBus.bus.get_object("org.ladish", room_path)
        room_project_properties = room_object.GetProjectProperties()

        # Remove old unused item if needed
        iItem = self.ui.treeWidget.topLevelItem(room_index)
        if iItem and not iItem.isVisible():
            self.ui.treeWidget.takeTopLevelItem(room_index)

        # Insert padding of items if needed
        for i in range(room_index):
            if not self.ui.treeWidget.topLevelItem(i):
                fake_item = QTreeWidgetItem(ITEM_TYPE_NULL)
                self.ui.treeWidget.insertTopLevelItem(i, fake_item)
                fake_item.setHidden(True)

        graph_version, project_properties = room_project_properties

        if len(project_properties) > 0:
            item_string = " (%s)" % project_properties['name']
        else:
            item_string = ""

        prop_obj = [None, None]
        prop_obj[iItemPropRoomPath] = room_path
        prop_obj[iItemPropRoomName] = room_name

        item = QTreeWidgetItem(ITEM_TYPE_ROOM)
        item.properties = prop_obj
        item.setText(0, "%s%s" % (room_name, item_string))

        self.ui.treeWidget.insertTopLevelItem(room_index, item)
        self.ui.treeWidget.expandItem(item)

        return item

    def canvas_add_group(self, groupId, groupName):
        # TODO - request ladish client type

        #if (False):
            #icon  = patchcanvas.ICON_HARDWARE
            #split = patchcanvas.SPLIT_NO
        #elif (False):
            #icon  = patchcanvas.ICON_LADISH_ROOM
            #split = patchcanvas.SPLIT_NO
        #else:

        splitTry = gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, groupId, URI_CANVAS_SPLIT)

        if splitTry == "true":
            groupSplit = patchcanvas.SPLIT_YES
        elif splitTry == "false":
            groupSplit = patchcanvas.SPLIT_NO
        else:
            groupSplit = patchcanvas.SPLIT_UNDEF

        groupIcon = patchcanvas.ICON_APPLICATION

        if gJack.client:
            ret, data, dataSize = jacklib.custom_get_data(gJack.client, groupName, URI_CANVAS_ICON)

            if ret == 0:
                iconName = voidptr2str(data)
                jacklib.free(data)

                if iconName == "hardware":
                    groupIcon = patchcanvas.ICON_HARDWARE
                    if groupSplit == patchcanvas.SPLIT_UNDEF:
                        groupSplit = patchcanvas.SPLIT_YES
                #elif iconName =="carla":
                    #groupIcon = patchcanvas.ICON_CARLA
                elif iconName =="distrho":
                    groupIcon = patchcanvas.ICON_DISTRHO
                elif iconName =="file":
                    groupIcon = patchcanvas.ICON_FILE
                elif iconName =="plugin":
                    groupIcon = patchcanvas.ICON_PLUGIN

        patchcanvas.addGroup(groupId, groupName, groupSplit, groupIcon)

        x  = gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, groupId, URI_CANVAS_X)
        y  = gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, groupId, URI_CANVAS_Y)
        x2 = gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, groupId, URI_CANVAS_X_SPLIT)
        y2 = gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, groupId, URI_CANVAS_Y_SPLIT)

        if x != None and y != None:
            if x2 is None: x2 = "%f" % (float(x) + 50)
            if y2 is None: y2 = "%f" % (float(y) + 50)
            patchcanvas.setGroupPosFull(groupId, float(x), float(y), float(x2), float(y2))

        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_remove_group(self, group_id):
        patchcanvas.removeGroup(group_id)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_rename_group(self, group_id, new_group_name):
        patchcanvas.renameGroup(group_id, new_group_name)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_add_port(self, group_id, port_id, port_name, port_mode, port_type):
        patchcanvas.addPort(group_id, port_id, port_name, port_mode, port_type)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_remove_port(self, port_id):
        patchcanvas.removePort(port_id)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_rename_port(self, port_id, new_port_name):
        patchcanvas.renamePort(port_id, new_port_name)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_connect_ports(self, connection_id, port_a, port_b):
        patchcanvas.connectPorts(connection_id, port_a, port_b)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def canvas_disconnect_ports(self, connection_id):
        patchcanvas.disconnectPorts(connection_id)
        QTimer.singleShot(0, self.ui.miniCanvasPreview.update)

    def jackStarted(self):
        if jacksettings.needsInit():
            self.DBusReconnect()

        if not gJack.client:
            gJack.client = jacklib.client_open("claudia", jacklib.JackNoStartServer, None)
            if not gJack.client:
                return self.jackStopped()

        canRender = render.canRender()

        self.ui.act_jack_render.setEnabled(canRender)
        self.ui.b_jack_render.setEnabled(canRender)
        self.menuJackTransport(True)
        self.menuA2JBridge(False)

        self.ui.cb_buffer_size.setEnabled(True)
        self.ui.cb_sample_rate.setEnabled(True) # jacksettings.getSampleRate() != -1

        if self.systray:
            self.systray.setActionEnabled("tools_render", canRender)

        self.ui.pb_dsp_load.setMaximum(100)
        self.ui.pb_dsp_load.setValue(0)
        self.ui.pb_dsp_load.update()

        self.init_jack()

        self.m_crashedJACK = False

    def jackStopped(self):
        #self.DBusReconnect()

        # client already closed
        gJack.client = None

        if self.fNextSampleRate:
            self.jack_setSampleRate(self.fNextSampleRate)

        bufferSize = jacksettings.getBufferSize()
        sampleRate = jacksettings.getSampleRate()
        bufferSizeTest = bool(bufferSize != -1)
        sampleRateTest = bool(sampleRate != -1)

        if bufferSizeTest:
            self.ui_setBufferSize(bufferSize)

        if sampleRateTest:
            self.ui_setSampleRate(sampleRate)

        self.ui_setRealTime(jacksettings.isRealtime())
        self.ui_setXruns(-1)

        self.ui.cb_buffer_size.setEnabled(bufferSizeTest)
        self.ui.cb_sample_rate.setEnabled(sampleRateTest)

        self.ui.act_jack_render.setEnabled(False)
        self.ui.b_jack_render.setEnabled(False)
        self.menuJackTransport(False)
        self.menuA2JBridge(False)

        if self.systray:
            self.systray.setActionEnabled("tools_render", False)

        if self.fCurTransportView == TRANSPORT_VIEW_HMS:
            self.ui.label_time.setText("00:00:00")
        elif self.fCurTransportView == TRANSPORT_VIEW_BBT:
            self.ui.label_time.setText("000|0|0000")
        elif self.fCurTransportView == TRANSPORT_VIEW_FRAMES:
            self.ui.label_time.setText("000'000'000")

        self.ui.pb_dsp_load.setValue(0)
        self.ui.pb_dsp_load.setMaximum(0)
        self.ui.pb_dsp_load.update()

    def studioStarted(self):
        self.ui.act_studio_start.setEnabled(False)
        self.ui.act_studio_stop.setEnabled(True)
        self.ui.act_studio_save.setEnabled(True)
        self.ui.act_studio_save_as.setEnabled(True)

        self.ui.b_studio_save.setEnabled(True)
        self.ui.b_studio_save_as.setEnabled(True)
        self.ui.b_studio_start.setEnabled(False)
        self.ui.b_studio_stop.setEnabled(True)

        if self.systray:
            self.systray.setActionEnabled("studio_start", False)
            self.systray.setActionEnabled("studio_stop", True)
            self.systray.setActionEnabled("studio_save", True)
            self.systray.setActionEnabled("studio_save_as", True)

    def studioStopped(self):
        self.ui.act_studio_start.setEnabled(True)
        self.ui.act_studio_stop.setEnabled(False)
        self.ui.act_studio_save.setEnabled(False)
        self.ui.act_studio_save_as.setEnabled(False)

        self.ui.b_studio_save.setEnabled(False)
        self.ui.b_studio_save_as.setEnabled(False)
        self.ui.b_studio_start.setEnabled(True)
        self.ui.b_studio_stop.setEnabled(False)

        if self.systray:
            self.systray.setActionEnabled("studio_start", True)
            self.systray.setActionEnabled("studio_stop", False)
            self.systray.setActionEnabled("studio_save", False)
            self.systray.setActionEnabled("studio_save_as", False)

    def studioLoaded(self):
        gDBus.ladish_studio  = gDBus.bus.get_object("org.ladish", "/org/ladish/Studio")
        gDBus.ladish_graph   = dbus.Interface(gDBus.ladish_studio, 'org.ladish.GraphDict')
        gDBus.ladish_manager = dbus.Interface(gDBus.ladish_studio, 'org.ladish.GraphManager')
        gDBus.ladish_app_iface = dbus.Interface(gDBus.ladish_studio, 'org.ladish.AppSupervisor')
        gDBus.patchbay = dbus.Interface(gDBus.ladish_studio, 'org.jackaudio.JackPatchbay')

        self.ui.label_first_time.setVisible(False)
        self.ui.graphicsView.setVisible(True)
        self.ui.miniCanvasPreview.setVisible(True)
        #if (self.ui.miniCanvasPreview.is_initiated):
            #self.checkMiniCanvasSize()

        self.ui.menu_Room.setEnabled(True)
        self.ui.menu_Project.setEnabled(False)
        self.ui.menu_Application.setEnabled(True)
        self.ui.group_project.setEnabled(False)

        self.ui.act_studio_rename.setEnabled(True)
        self.ui.act_studio_unload.setEnabled(True)

        if self.systray:
            self.systray.setActionEnabled("studio_rename", True)
            self.systray.setActionEnabled("studio_unload", True)

        self.init_studio()

        self.m_crashedLADISH = False

    def studioUnloaded(self):
        gDBus.ladish_studio  = None
        gDBus.ladish_graph   = None
        gDBus.ladish_manager = None
        gDBus.ladish_app_iface = None
        gDBus.patchbay = None

        self.m_lastItemType = None
        self.m_lastRoomPath = None

        self.ui.label_first_time.setVisible(True)
        self.ui.graphicsView.setVisible(False)
        self.ui.miniCanvasPreview.setVisible(False)

        self.ui.menu_Room.setEnabled(False)
        self.ui.menu_Project.setEnabled(False)
        self.ui.menu_Application.setEnabled(False)
        self.ui.group_project.setEnabled(False)

        self.ui.act_studio_start.setEnabled(False)
        self.ui.act_studio_stop.setEnabled(False)
        self.ui.act_studio_rename.setEnabled(False)
        self.ui.act_studio_save.setEnabled(False)
        self.ui.act_studio_save_as.setEnabled(False)
        self.ui.act_studio_unload.setEnabled(False)

        self.ui.b_studio_save.setEnabled(False)
        self.ui.b_studio_save_as.setEnabled(False)
        self.ui.b_studio_start.setEnabled(False)
        self.ui.b_studio_stop.setEnabled(False)

        if self.systray:
            self.systray.setActionEnabled("studio_start", False)
            self.systray.setActionEnabled("studio_stop", False)
            self.systray.setActionEnabled("studio_rename", False)
            self.systray.setActionEnabled("studio_save", False)
            self.systray.setActionEnabled("studio_save_as", False)
            self.systray.setActionEnabled("studio_unload", False)

        self.ui.treeWidget.clear()

        patchcanvas.clear()

    def a2jStarted(self):
        self.menuA2JBridge(True)

    def a2jStopped(self):
        self.menuA2JBridge(False)

    def menuJackTransport(self, enabled):
        self.ui.act_transport_play.setEnabled(enabled)
        self.ui.act_transport_stop.setEnabled(enabled)
        self.ui.act_transport_backwards.setEnabled(enabled)
        self.ui.act_transport_forwards.setEnabled(enabled)
        self.ui.menu_Transport.setEnabled(enabled)
        self.ui.group_transport.setEnabled(enabled)

    def menuA2JBridge(self, started):
        if not gDBus.jack.IsStarted():
            self.ui.act_tools_a2j_start.setEnabled(False)
            self.ui.act_tools_a2j_stop.setEnabled(False)
            self.ui.act_tools_a2j_export_hw.setEnabled(gDBus.a2j and not gDBus.a2j.is_started())
        else:
            self.ui.act_tools_a2j_start.setEnabled(not started)
            self.ui.act_tools_a2j_stop.setEnabled(started)
            self.ui.act_tools_a2j_export_hw.setEnabled(not started)

    def DBusSignalReceiver(self, *args, **kwds):
        if kwds['interface'] == "org.freedesktop.DBus" and kwds['path'] == "/org/freedesktop/DBus" and kwds['member'] == "NameOwnerChanged":
            appInterface, appId, newId = args
            #print("appInterface crashed", appInterface)

            if not newId:
                # Something crashed
                if appInterface == "org.gna.home.a2jmidid":
                    QTimer.singleShot(0, self.slot_handleCrash_a2j)
                elif appInterface in ("org.jackaudio.service", "org.ladish"):
                    # Prevent any more dbus calls
                    gDBus.jack = None
                    gJack.client = None
                    jacksettings.initBus(None)
                    self.DBusCrashCallback.emit(appInterface)

        elif kwds['interface'] == "org.jackaudio.JackControl":
            if DEBUG: print("DBus signal @org.jackaudio.JackControl,", kwds['member'])
            if kwds['member'] == "ServerStarted":
                self.DBusServerStartedCallback.emit()
            elif kwds['member'] == "ServerStopped":
                self.DBusServerStoppedCallback.emit()

        elif kwds['interface'] == "org.jackaudio.JackPatchbay":
            if gDBus.patchbay and kwds['path'] == gDBus.patchbay.object_path:
                if DEBUG: print("DBus signal @org.jackaudio.JackPatchbay,", kwds['member'])
                if kwds['member'] == "ClientAppeared":
                    self.DBusClientAppearedCallback.emit(args[iJackClientId], args[iJackClientName])
                elif kwds['member'] == "ClientDisappeared":
                    self.DBusClientDisappearedCallback.emit(args[iJackClientId])
                elif kwds['member'] == "ClientRenamed":
                    self.DBusClientRenamedCallback.emit(args[iRenamedId], args[iRenamedNewName])
                elif kwds['member'] == "PortAppeared":
                    self.DBusPortAppearedCallback.emit(args[iJackClientId], args[iJackPortId], args[iJackPortName], args[iJackPortFlags], args[iJackPortType])
                elif kwds['member'] == "PortDisappeared":
                    self.DBusPortDisppearedCallback.emit(args[iJackPortId])
                elif kwds['member'] == "PortRenamed":
                    self.DBusPortRenamedCallback.emit(args[iJackPortId], args[iJackPortNewName])
                elif kwds['member'] == "PortsConnected":
                    self.DBusPortsConnectedCallback.emit(args[iJackConnId], args[iSourcePortId], args[iTargetPortId])
                elif kwds['member'] == "PortsDisconnected":
                    self.DBusPortsDisconnectedCallback.emit(args[iJackConnId])

        elif kwds['interface'] == "org.ladish.Control":
            if DEBUG: print("DBus signal @org.ladish.Control,", kwds['member'])
            if kwds['member'] == "StudioAppeared":
                self.DBusStudioAppearedCallback.emit()
            elif kwds['member'] == "StudioDisappeared":
                self.DBusStudioDisappearedCallback.emit()
            elif kwds['member'] == "QueueExecutionHalted":
                self.DBusQueueExecutionHaltedCallback.emit()
            elif kwds['member'] == "CleanExit":
                self.DBusCleanExitCallback.emit()

        elif kwds['interface'] == "org.ladish.Studio":
            if DEBUG: print("DBus signal @org.ladish.Studio,", kwds['member'])
            if kwds['member'] == "StudioStarted":
                self.DBusStudioStartedCallback.emit()
            elif kwds['member'] == "StudioStopped":
                self.DBusStudioStoppedCallback.emit()
            elif kwds['member'] == "StudioRenamed":
                self.DBusStudioRenamedCallback.emit(args[iStudioRenamedName])
            elif kwds['member'] == "StudioCrashed":
                self.DBusStudioCrashedCallback.emit()
            elif kwds['member'] == "RoomAppeared":
                self.DBusRoomAppearedCallback.emit(args[iRoomAppearedPath], args[iRoomAppearedDict]['name'])
            elif kwds['member'] == "RoomDisappeared":
                self.DBusRoomDisappearedCallback.emit(args[iRoomAppearedPath])
            #elif kwds['member'] == "RoomChanged":
                #self.DBusRoomChangedCallback.emit()

        elif kwds['interface'] == "org.ladish.Room":
            if DEBUG: print("DBus signal @org.ladish.Room,", kwds['member'])
            if kwds['member'] == "ProjectPropertiesChanged":
                if "name" in args[iProjChangedDict].keys():
                    self.DBusProjectPropertiesChanged.emit(kwds['path'], args[iProjChangedDict]['name'])
                else:
                    self.DBusProjectPropertiesChanged.emit(kwds['path'], "")

        elif kwds['interface'] == "org.ladish.AppSupervisor":
            if DEBUG: print("DBus signal @org.ladish.AppSupervisor,", kwds['member'])
            if kwds['member'] == "AppAdded2":
                self.DBusAppAdded2Callback.emit(kwds['path'], args[iAppChangedNumber], args[iAppChangedName], args[iAppChangedActive], args[iAppChangedTerminal], args[iAppChangedLevel])
            elif kwds['member'] == "AppRemoved":
                self.DBusAppRemovedCallback.emit(kwds['path'], args[iAppChangedNumber])
            elif kwds['member'] == "AppStateChanged2":
                self.DBusAppStateChanged2Callback.emit(kwds['path'], args[iAppChangedNumber], args[iAppChangedName], args[iAppChangedActive], args[iAppChangedTerminal], args[iAppChangedLevel])

        elif kwds['interface'] == "org.gna.home.a2jmidid.control":
            if kwds['member'] == "bridge_started":
                self.a2jStarted()
            elif kwds['member'] == "bridge_stopped":
                self.a2jStopped()

    def DBusReconnect(self):
        gDBus.jack = gDBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
        gDBus.ladish_control = gDBus.bus.get_object("org.ladish", "/org/ladish/Control")
        gDBus.ladish_studio = None
        gDBus.ladish_room = None
        gDBus.ladish_graph = None
        gDBus.ladish_manager = None
        gDBus.ladish_app_iface = None
        gDBus.patchbay = None

        try:
            gDBus.ladish_app_daemon = gDBus.bus.get_object("org.ladish.appdb", "/")
        except:
            gDBus.ladish_app_daemon = None

        try:
            gDBus.a2j = dbus.Interface(gDBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
        except:
            gDBus.a2j = None

        jacksettings.initBus(gDBus.bus)

    def refreshXruns(self):
        if not gDBus.jack:
            #if not self.m_crashedJACK:
                #self.DBusReconnect()
            return

        xruns = int(gDBus.jack.GetXruns())
        if self.fXruns != xruns:
            self.ui_setXruns(xruns)
            self.fXruns = xruns

    def JackBufferSizeCallback(self, buffer_size, arg):
        if DEBUG: print("JackBufferSizeCallback(%i)" % buffer_size)
        self.BufferSizeCallback.emit(buffer_size)
        return 0

    def JackSampleRateCallback(self, sample_rate, arg):
        if DEBUG: print("JackSampleRateCallback(%i)" % sample_rate)
        self.SampleRateCallback.emit(sample_rate)
        return 0

    def JackShutdownCallback(self, arg):
        if DEBUG: print("JackShutdownCallback")
        self.ShutdownCallback.emit()
        return 0

    @pyqtSlot()
    def slot_studio_new(self):
        dialog = StudioNameW(self, StudioNameW.NEW)
        if dialog.exec_():
            gDBus.ladish_control.NewStudio(dialog.fRetStudioName)

    @pyqtSlot()
    def slot_studio_load_b(self):
        dialog = StudioListW(self)
        if dialog.exec_():
            gDBus.ladish_control.LoadStudio(dialog.ret_studio_name)

    @pyqtSlot()
    def slot_studio_load_m(self):
        studio_name = self.sender().text()
        if studio_name:
            gDBus.ladish_control.LoadStudio(studio_name)

    @pyqtSlot()
    def slot_studio_start(self):
        gDBus.ladish_studio.Start()

    @pyqtSlot()
    def slot_studio_stop(self):
        gDBus.ladish_studio.Stop()

    @pyqtSlot()
    def slot_studio_rename(self):
        dialog = StudioNameW(self, StudioNameW.RENAME)
        if dialog.exec_():
            gDBus.ladish_studio.Rename(dialog.fRetStudioName)

    @pyqtSlot()
    def slot_studio_save(self):
        gDBus.ladish_studio.Save()

    @pyqtSlot()
    def slot_studio_save_as(self):
        dialog = StudioNameW(self, StudioNameW.SAVE_AS)
        if dialog.exec_():
            gDBus.ladish_studio.SaveAs(dialog.fRetStudioName)

    @pyqtSlot()
    def slot_studio_unload(self):
        gDBus.ladish_studio.Unload()

    @pyqtSlot()
    def slot_studio_delete_m(self):
        studio_name = self.sender().text()
        if studio_name:
            gDBus.ladish_control.DeleteStudio(studio_name)

    @pyqtSlot()
    def slot_room_create(self):
        dialog = CreateRoomW(self)
        if dialog.exec_():
            gDBus.ladish_studio.CreateRoom(dialog.ret_room_name, dialog.ret_room_template)

    @pyqtSlot()
    def slot_room_delete_m(self):
        room_name = self.sender().text()
        if room_name:
            gDBus.ladish_studio.DeleteRoom(room_name)

    @pyqtSlot()
    def slot_project_new(self):
        dialog = ProjectNameW(self, ProjectNameW.NEW, self.fSavedSettings["Main/DefaultProjectFolder"])
        if dialog.exec_():
            # Check if a project is already loaded, if yes unload it first
            project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()
            if len(project_properties) > 0:
                gDBus.ladish_room.UnloadProject()
            gDBus.ladish_room.SaveProject(dialog.ret_project_path, dialog.ret_project_name)

    @pyqtSlot()
    def slot_project_load(self):
        project_path = QFileDialog.getExistingDirectory(self, self.tr("Open Project"), self.fSavedSettings["Main/DefaultProjectFolder"])
        if project_path:
            if os.path.exists(os.path.join(project_path, "ladish-project.xml")):
                gDBus.ladish_room.LoadProject(project_path)
            else:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("The selected folder does not contain a ladish project"))

    @pyqtSlot()
    def slot_project_load_m(self):
        act_x_text = self.sender().text()
        if act_x_text:
            proj_path = "/" + act_x_text.rsplit("[/", 1)[-1].rsplit("]", 1)[0]
            gDBus.ladish_room.LoadProject(proj_path)

    @pyqtSlot()
    def slot_project_save(self):
        project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()
        if len(project_properties) > 0:
            path = dbus.String(project_properties['dir'])
            name = dbus.String(project_properties['name'])
            gDBus.ladish_room.SaveProject(path, name)
        else:
            self.slot_project_new()

    @pyqtSlot()
    def slot_project_save_as(self):
        project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()

        if len(project_properties) > 0:
            path = str(project_properties['dir'])
            name = str(project_properties['name'])
            dialog = ProjectNameW(self, ProjectNameW.SAVE_AS, self.fSavedSettings["Main/DefaultProjectFolder"], path, name)

            if dialog.exec_():
                gDBus.ladish_room.SaveProject(dialog.ret_project_path, dialog.ret_project_name)

        else:
            self.slot_project_new()

    @pyqtSlot()
    def slot_project_unload(self):
        gDBus.ladish_room.UnloadProject()

    @pyqtSlot()
    def slot_project_properties(self):
        project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()

        path = str(project_properties['dir'])
        name = str(project_properties['name'])

        if "description" in project_properties.keys():
            description = str(project_properties['description'])
        else:
            description = ""

        if "notes" in project_properties.keys():
            notes = str(project_properties['notes'])
        else:
            notes = ""

        dialog = ProjectPropertiesW(self, name, description, notes)

        if dialog.exec_():
            gDBus.ladish_room.SetProjectDescription(dialog.ret_obj[iAppPropDescription])
            gDBus.ladish_room.SetProjectNotes(dialog.ret_obj[iAppPropNotes])

            if dialog.ret_obj[iAppPropSaveNow]:
                gDBus.ladish_room.SaveProject(path, dialog.ret_obj[iAppPropName])

    @pyqtSlot()
    def slot_app_add_new(self):
        proj_folder = ""

        if self.m_lastItemType == ITEM_TYPE_STUDIO or self.m_lastItemType == ITEM_TYPE_STUDIO_APP:
            proj_folder = self.fSavedSettings['Main/DefaultProjectFolder']
            is_room = False

        elif self.m_lastItemType == ITEM_TYPE_ROOM or self.m_lastItemType == ITEM_TYPE_ROOM_APP:
            project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()

            if len(project_properties) > 0:
                proj_folder = str(project_properties['dir'])
                is_room = True
            else:
                proj_folder = self.fSavedSettings['Main/DefaultProjectFolder']
                is_room = False

        else:
            print("Invalid m_last_item_type value")
            return

        dialog = ClaudiaLauncherW(self, gDBus.ladish_app_iface, proj_folder, is_room, self.fLastBPM, self.fSampleRate)
        dialog.exec_()

    @pyqtSlot()
    def slot_app_run_custom(self):
        dialog = RunCustomW(self, bool(self.m_lastItemType in (ITEM_TYPE_ROOM, ITEM_TYPE_ROOM_APP)))
        if dialog.exec_() and dialog.ret_app_obj:
            app_obj = dialog.ret_app_obj
            gDBus.ladish_app_iface.RunCustom2(app_obj[iAppTerminal], app_obj[iAppCommand], app_obj[iAppName], app_obj[iAppLevel])

    @pyqtSlot()
    def slot_checkCurrentRoom(self):
        item = self.ui.treeWidget.currentItem()
        room_path = None

        if not item:
            return

        if item.type() in (ITEM_TYPE_STUDIO, ITEM_TYPE_STUDIO_APP):
            self.ui.menu_Project.setEnabled(False)
            self.ui.group_project.setEnabled(False)
            self.ui.menu_Application.setEnabled(True)

            gDBus.ladish_room = None
            gDBus.ladish_app_iface = dbus.Interface(gDBus.ladish_studio, "org.ladish.AppSupervisor")
            ITEM_TYPE = ITEM_TYPE_STUDIO

        elif item.type() in (ITEM_TYPE_ROOM, ITEM_TYPE_ROOM_APP):
            self.ui.menu_Project.setEnabled(True)
            self.ui.group_project.setEnabled(True)

            if item.type() == ITEM_TYPE_ROOM:
                room_path = item.properties[iItemPropRoomPath]
            elif item.type() == ITEM_TYPE_ROOM_APP:
                room_path = item.parent().properties[iItemPropRoomPath]
            else:
                return

            gDBus.ladish_room = gDBus.bus.get_object("org.ladish", room_path)
            gDBus.ladish_app_iface = dbus.Interface(gDBus.ladish_room, "org.ladish.AppSupervisor")
            ITEM_TYPE = ITEM_TYPE_ROOM

            project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()

            has_project = bool(len(project_properties) > 0)
            self.ui.act_project_save.setEnabled(has_project)
            self.ui.act_project_save_as.setEnabled(has_project)
            self.ui.act_project_unload.setEnabled(has_project)
            self.ui.act_project_properties.setEnabled(has_project)
            self.ui.b_project_save.setEnabled(has_project)
            self.ui.b_project_save_as.setEnabled(has_project)
            self.ui.menu_Application.setEnabled(has_project)

        else:
            return

        if ITEM_TYPE != self.m_lastItemType or room_path != self.m_lastRoomPath:
            if ITEM_TYPE == ITEM_TYPE_STUDIO:
                object_path = gDBus.ladish_studio
            elif ITEM_TYPE == ITEM_TYPE_ROOM:
                object_path = gDBus.ladish_room
            else:
                return

            patchcanvas.clear()
            gDBus.patchbay = dbus.Interface(object_path, 'org.jackaudio.JackPatchbay')
            gDBus.ladish_graph = dbus.Interface(object_path, 'org.ladish.GraphDict')
            gDBus.ladish_manager = dbus.Interface(object_path, 'org.ladish.GraphManager')
            self.initPorts()

        self.m_lastItemType = ITEM_TYPE
        self.m_lastRoomPath = room_path

    @pyqtSlot(QTreeWidgetItem, int)
    def slot_doubleClickedAppList(self, item, row):
        if item.type() in (ITEM_TYPE_STUDIO_APP, ITEM_TYPE_ROOM_APP):
            if item.properties[iItemPropActive]:
                gDBus.ladish_app_iface.StopApp(item.properties[iItemPropNumber])
            else:
                gDBus.ladish_app_iface.StartApp(item.properties[iItemPropNumber])

    @pyqtSlot()
    def slot_updateMenuStudioList_Load(self):
        self.ui.menu_studio_load.clear()

        studio_list = gDBus.ladish_control.GetStudioList()
        if len(studio_list) == 0:
            act_no_studio = QAction(self.tr("Empty studio list"), self.ui.menu_studio_load)
            act_no_studio.setEnabled(False)
            self.ui.menu_studio_load.addAction(act_no_studio)
        else:
            for studio in studio_list:
                studio_name  = str(studio[iStudioListName])
                act_x_studio = QAction(studio_name, self.ui.menu_studio_load)
                self.ui.menu_studio_load.addAction(act_x_studio)
                act_x_studio.triggered.connect(self.slot_studio_load_m)

    @pyqtSlot()
    def slot_updateMenuStudioList_Delete(self):
        self.ui.menu_studio_delete.clear()

        studio_list = gDBus.ladish_control.GetStudioList()
        if len(studio_list) == 0:
            act_no_studio = QAction(self.tr("Empty studio list"), self.ui.menu_studio_delete)
            act_no_studio.setEnabled(False)
            self.ui.menu_studio_delete.addAction(act_no_studio)
        else:
            for studio in studio_list:
                studio_name = str(studio[iStudioListName])
                act_x_studio = QAction(studio_name, self.ui.menu_studio_delete)
                self.ui.menu_studio_delete.addAction(act_x_studio)
                act_x_studio.triggered.connect(self.slot_studio_delete_m)

    @pyqtSlot()
    def slot_updateMenuRoomList(self):
        self.ui.menu_room_delete.clear()
        if gDBus.ladish_control.IsStudioLoaded():
            room_list = gDBus.ladish_studio.GetRoomList()
            if len(room_list) == 0:
                self.createEmptyMenuRoomActon()
            else:
                for room_path, room_dict in room_list:
                    ladish_room = gDBus.bus.get_object("org.ladish", room_path)
                    room_name = ladish_room.GetName()
                    act_x_room = QAction(room_name, self.ui.menu_room_delete)
                    self.ui.menu_room_delete.addAction(act_x_room)
                    act_x_room.triggered.connect(self.slot_room_delete_m)
        else:
            self.createEmptyMenuRoomActon()

    def createEmptyMenuRoomActon(self):
        act_no_room = QAction(self.tr("Empty room list"), self.ui.menu_room_delete)
        act_no_room.setEnabled(False)
        self.ui.menu_room_delete.addAction(act_no_room)

    @pyqtSlot()
    def slot_updateMenuProjectList(self):
        self.ui.menu_project_load.clear()
        act_project_load = QAction(self.tr("Load from folder..."), self.ui.menu_project_load)
        self.ui.menu_project_load.addAction(act_project_load)
        act_project_load.triggered.connect(self.slot_project_load)

        ladish_recent_iface = dbus.Interface(gDBus.ladish_room, "org.ladish.RecentItems")
        proj_list = ladish_recent_iface.get(RECENT_PROJECTS_STORE_MAX_ITEMS)

        if len(proj_list) > 0:
            self.ui.menu_project_load.addSeparator()
            for proj_path, proj_dict in proj_list:
                if "name" in proj_dict.keys():
                    proj_name = proj_dict['name']
                else:
                    continue

                act_x_text = "%s [%s]" % (proj_name, proj_path)
                act_x_proj = QAction(act_x_text, self.ui.menu_project_load)
                self.ui.menu_project_load.addAction(act_x_proj)
                act_x_proj.triggered.connect(self.slot_project_load_m)

    @pyqtSlot()
    def slot_showAppListCustomMenu(self):
        item = self.ui.treeWidget.currentItem()
        if item:
            cMenu = QMenu()
            if item.type() == ITEM_TYPE_STUDIO:
                act_x_add_new = cMenu.addAction(self.tr("Add New..."))
                act_x_run_custom = cMenu.addAction(self.tr("Run Custom..."))
                cMenu.addSeparator()
                act_x_create_room = cMenu.addAction(self.tr("Create Room..."))

                act_x_add_new.setIcon(QIcon.fromTheme("list-add", QIcon(":/16x16/list-add.png")))
                act_x_run_custom.setIcon(QIcon.fromTheme("system-run", QIcon(":/16x16/system-run.png")))
                act_x_create_room.setIcon(QIcon.fromTheme("list-add", QIcon(":/16x16/list-add.png")))
                act_x_add_new.setEnabled(self.ui.act_app_add_new.isEnabled())

            elif item.type() == ITEM_TYPE_ROOM:
                act_x_add_new = cMenu.addAction(self.tr("Add New..."))
                act_x_run_custom = cMenu.addAction(self.tr("Run Custom..."))
                cMenu.addSeparator()
                act_x_new = cMenu.addAction(self.tr("New Project..."))
                cMenu.addMenu(self.ui.menu_project_load)
                act_x_save = cMenu.addAction(self.tr("Save Project"))
                act_x_save_as = cMenu.addAction(self.tr("Save Project As..."))
                act_x_unload = cMenu.addAction(self.tr("Unload Project"))
                cMenu.addSeparator()
                act_x_properties = cMenu.addAction(self.tr("Project Properties..."))
                cMenu.addSeparator()
                act_x_delete_room = cMenu.addAction(self.tr("Delete Room"))

                act_x_add_new.setIcon(QIcon.fromTheme("list-add", QIcon(":/16x16/list-add.png")))
                act_x_run_custom.setIcon(QIcon.fromTheme("system-run", QIcon(":/16x16/system-run.png")))
                act_x_new.setIcon(QIcon.fromTheme("document-new", QIcon(":/16x16/document-new.png")))
                act_x_save.setIcon(QIcon.fromTheme("document-save", QIcon(":/16x16/document-save.png")))
                act_x_save_as.setIcon(QIcon.fromTheme("document-save-as", QIcon(":/16x16/document-save-as.png")))
                act_x_unload.setIcon(QIcon.fromTheme("window-close", QIcon(":/16x16/dialog-close.png")))
                act_x_properties.setIcon(QIcon.fromTheme("edit-rename", QIcon(":/16x16/edit-rename.png")))
                act_x_delete_room.setIcon(QIcon.fromTheme("edit-delete", QIcon(":/16x16/edit-delete.png")))

                act_x_add_new.setEnabled(self.ui.menu_Application.isEnabled() and self.ui.act_app_add_new.isEnabled())

                project_graph_version, project_properties = gDBus.ladish_room.GetProjectProperties()

                if len(project_properties) == 0:
                    act_x_run_custom.setEnabled(False)
                    act_x_save.setEnabled(False)
                    act_x_save_as.setEnabled(False)
                    act_x_unload.setEnabled(False)
                    act_x_properties.setEnabled(False)

            elif item.type() in (ITEM_TYPE_STUDIO_APP, ITEM_TYPE_ROOM_APP):
                if item.properties[iItemPropActive]:
                    act_x_start = None
                    act_x_stop = cMenu.addAction(self.tr("Stop"))
                    act_x_kill = cMenu.addAction(self.tr("Kill"))
                    act_x_stop.setIcon(QIcon.fromTheme("media-playback-stop", QIcon(":/16x16/media-playback-stop.png")))
                    act_x_kill.setIcon(QIcon.fromTheme("dialog-close", QIcon(":/16x16/dialog-close.png")))
                else:
                    act_x_start = cMenu.addAction(self.tr("Start"))
                    act_x_stop = None
                    act_x_kill = None
                    act_x_start.setIcon(QIcon.fromTheme("media-playback-start", QIcon(":/16x16/media-playback-start.png")))
                act_x_properties = cMenu.addAction(self.tr("Properties"))
                cMenu.addSeparator()
                act_x_remove = cMenu.addAction(self.tr("Remove"))
                act_x_properties.setIcon(QIcon.fromTheme("edit-rename", QIcon(":/16x16/edit-rename.png")))
                act_x_remove.setIcon(QIcon.fromTheme("edit-delete", QIcon(":/16x16/edit-delete.png")))

            else:
                return

        act_x_sel = cMenu.exec_(QCursor.pos())

        if act_x_sel:
            if item.type() == ITEM_TYPE_STUDIO:
                if act_x_sel == act_x_add_new:
                    self.slot_app_add_new()
                elif act_x_sel == act_x_run_custom:
                    self.slot_app_run_custom()
                elif act_x_sel == act_x_create_room:
                    self.slot_room_create()

            elif item.type() == ITEM_TYPE_ROOM:
                if act_x_sel == act_x_add_new:
                    self.slot_app_add_new()
                elif act_x_sel == act_x_run_custom:
                    self.slot_app_run_custom()
                elif act_x_sel == act_x_new:
                    self.slot_project_new()
                elif act_x_sel == act_x_save:
                    self.slot_project_save()
                elif act_x_sel == act_x_save_as:
                    self.slot_project_save_as()
                elif act_x_sel == act_x_unload:
                    self.slot_project_unload()
                elif act_x_sel == act_x_properties:
                    self.slot_project_properties()
                elif act_x_sel == act_x_delete_room:
                    room_name = gDBus.ladish_room.GetName()
                    gDBus.ladish_studio.DeleteRoom(room_name)

            elif item.type() in (ITEM_TYPE_STUDIO_APP, ITEM_TYPE_ROOM_APP):
                number = item.properties[iItemPropNumber]

                if act_x_sel == act_x_start:
                    gDBus.ladish_app_iface.StartApp(number)
                elif act_x_sel == act_x_stop:
                    gDBus.ladish_app_iface.StopApp(number)
                elif act_x_sel == act_x_kill:
                    gDBus.ladish_app_iface.KillApp(number)
                elif act_x_sel == act_x_properties:
                    name, command, active, terminal, level = gDBus.ladish_app_iface.GetAppProperties2(number)

                    app_obj = [None, None, None, None, None]
                    app_obj[iAppCommand]  = str(command)
                    app_obj[iAppName]     = str(name)
                    app_obj[iAppTerminal] = bool(terminal)
                    app_obj[iAppLevel]    = str(level)
                    app_obj[iAppActive]   = bool(active)

                    dialog = RunCustomW(self, bool(item.type() == ITEM_TYPE_ROOM_APP), app_obj)
                    dialog.setWindowTitle(self.tr("App properties"))
                    if dialog.exec_():
                        app_obj = dialog.ret_app_obj
                        gDBus.ladish_app_iface.SetAppProperties2(number, app_obj[iAppName], app_obj[iAppCommand], app_obj[iAppTerminal], app_obj[iAppLevel])

                elif act_x_sel == act_x_remove:
                    gDBus.ladish_app_iface.RemoveApp(number)

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.ui.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        if split_mode == patchcanvas.PORT_MODE_INPUT:
            canvas_x = URI_CANVAS_X_SPLIT
            canvas_y = URI_CANVAS_Y_SPLIT
        else:
            canvas_x = URI_CANVAS_X
            canvas_y = URI_CANVAS_Y

        gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, canvas_x, str(pos.x()))
        gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, canvas_y, str(pos.y()))

        self.ui.miniCanvasPreview.update()

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosX(xp)

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosY(yp)

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        settings = QSettings()
        self.ui.graphicsView.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH / 3, type=int))
        self.ui.graphicsView.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT * 3 / 8, type=int))

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.ui.graphicsView.horizontalScrollBar().setValue(xp * DEFAULT_CANVAS_WIDTH)
        self.ui.graphicsView.verticalScrollBar().setValue(yp * DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.ui.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.ui.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.ui.miniCanvasPreview.setViewSize(float(self.ui.graphicsView.width()) / DEFAULT_CANVAS_WIDTH, float(self.ui.graphicsView.height()) / DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_handleCrash_jack(self):
        patchcanvas.clear()
        self.DBusReconnect()
        self.studioUnloaded()

        if gDBus.a2j:
            if gDBus.a2j.is_started():
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.ui.act_tools_a2j_start.setEnabled(False)
            self.ui.act_tools_a2j_stop.setEnabled(False)
            self.ui.act_tools_a2j_export_hw.setEnabled(False)
            self.ui.menu_A2J_Bridge.setEnabled(False)

    @pyqtSlot()
    def slot_handleCrash_ladish(self):
        self.ui.treeWidget.clear()
        patchcanvas.clear()
        self.DBusReconnect()
        QMessageBox.warning(self, self.tr("Error"), self.tr("ladish daemon has crashed"))

    @pyqtSlot()
    def slot_handleCrash_studio(self):
        QMessageBox.warning(self, self.tr("Error"), self.tr("jackdbus has crashed"))

    @pyqtSlot()
    def slot_handleCrash_a2j(self):
        try:
            gDBus.a2j = dbus.Interface(gDBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
        except:
            gDBus.a2j = None

        if gDBus.a2j:
            if gDBus.a2j.is_started():
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.ui.act_tools_a2j_start.setEnabled(False)
            self.ui.act_tools_a2j_stop.setEnabled(False)
            self.ui.act_tools_a2j_export_hw.setEnabled(False)
            self.ui.menu_A2J_Bridge.setEnabled(False)

    @pyqtSlot(str)
    def slot_DBusCrashCallback(self, appInterface):
        if appInterface == "org.jackaudio.service":
            if not (self.m_crashedJACK or self.m_crashedLADISH):
                self.m_crashedJACK = True
                QTimer.singleShot(1000, self.slot_handleCrash_jack)
        elif appInterface == "org.ladish":
            if not (self.m_crashedJACK or self.m_crashedLADISH):
                self.m_crashedLADISH = True
                QTimer.singleShot(1000, self.slot_handleCrash_ladish)

    @pyqtSlot()
    def slot_DBusServerStartedCallback(self):
        self.jackStarted()

    @pyqtSlot()
    def slot_DBusServerStoppedCallback(self):
        self.jackStopped()

    @pyqtSlot(int, str)
    def slot_DBusClientAppearedCallback(self, group_id, group_name):
        self.canvas_add_group(group_id, group_name)

    @pyqtSlot(int)
    def slot_DBusClientDisappearedCallback(self, group_id):
        self.canvas_remove_group(group_id)

    @pyqtSlot(int, str)
    def slot_DBusClientRenamedCallback(self, group_id, new_group_name):
        self.canvas_rename_group(group_id, new_group_name)

    @pyqtSlot(int, int, str, int, int)
    def slot_DBusPortAppearedCallback(self, group_id, port_id, port_name, port_flags, port_type_jack):
        if port_flags & JACKDBUS_PORT_FLAG_INPUT:
            port_mode = patchcanvas.PORT_MODE_INPUT
        elif port_flags & JACKDBUS_PORT_FLAG_OUTPUT:
            port_mode = patchcanvas.PORT_MODE_OUTPUT
        else:
            port_mode = patchcanvas.PORT_MODE_NULL

        if port_type_jack == JACKDBUS_PORT_TYPE_AUDIO:
            port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif port_type_jack == JACKDBUS_PORT_TYPE_MIDI:
            if gDBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes":
                port_type = patchcanvas.PORT_TYPE_MIDI_A2J
            else:
                port_type = patchcanvas.PORT_TYPE_MIDI_JACK
        else:
            port_type = patchcanvas.PORT_TYPE_NULL

        self.canvas_add_port(group_id, port_id, port_name, port_mode, port_type)

    @pyqtSlot(int)
    def slot_DBusPortDisppearedCallback(self, port_id):
        self.canvas_remove_port(port_id)

    @pyqtSlot(int, str)
    def slot_DBusPortRenamedCallback(self, port_id, new_port_name):
        self.canvas_rename_port(port_id, new_port_name)

    @pyqtSlot(int, int, int)
    def slot_DBusPortsConnectedCallback(self, connection_id, source_port_id, target_port_id):
        self.canvas_connect_ports(connection_id, source_port_id, target_port_id)

    @pyqtSlot(int)
    def slot_DBusPortsDisconnectedCallback(self, connection_id):
        self.canvas_disconnect_ports(connection_id)

    @pyqtSlot()
    def slot_DBusStudioAppearedCallback(self):
        self.studioLoaded()
        if gDBus.ladish_studio.IsStarted():
            self.studioStarted()
        else:
            self.studioStopped()

    @pyqtSlot()
    def slot_DBusStudioDisappearedCallback(self):
        self.studioUnloaded()

    @pyqtSlot()
    def slot_DBusQueueExecutionHaltedCallback(self):
        log_path = os.path.join(HOME, ".log", "ladish", "ladish.log")
        if os.path.exists(log_path):
            log_file = open(log_path)
            log_text = logs.fixLogText(log_file.read().split("ERROR: ")[-1].split("\n")[0])
            log_file.close()
        else:
            log_text = None

        msgbox = QMessageBox(QMessageBox.Critical, self.tr("Execution Halted"),
            self.tr("Something went wrong with ladish so the last action was not sucessful.\n"), QMessageBox.Ok, self)

        if log_text:
            msgbox.setInformativeText(self.tr("You can check the ladish log file (or click in the 'Show Details' button) to find out what went wrong."))
            msgbox.setDetailedText(log_text)
        else:
            msgbox.setInformativeText(self.tr("You can check the ladish log file to find out what went wrong."))

        msgbox.show()

    @pyqtSlot()
    def slot_DBusCleanExitCallback(self):
        pass # TODO
        #self.timer1000.stop()
        #QTimer.singleShot(1000, self.DBusReconnect)
        #QTimer.singleShot(1500, self.timer1000.start)

    @pyqtSlot()
    def slot_DBusStudioStartedCallback(self):
        self.studioStarted()

    @pyqtSlot()
    def slot_DBusStudioStoppedCallback(self):
        self.studioStopped()

    @pyqtSlot(str)
    def slot_DBusStudioRenamedCallback(self, new_name):
        self.ui.treeWidget.topLevelItem(0).setText(0, new_name)

    @pyqtSlot()
    def slot_DBusStudioCrashedCallback(self):
        QTimer.singleShot(0, self.slot_handleCrash_studio)

    @pyqtSlot(str, str)
    def slot_DBusRoomAppearedCallback(self, room_path, room_name):
        self.room_add(room_path, room_name)

    @pyqtSlot(str)
    def slot_DBusRoomDisappearedCallback(self, room_path):
        for i in range(self.ui.treeWidget.topLevelItemCount()):
            item = self.ui.treeWidget.topLevelItem(i)

            if i == 0:
                continue

            if item and item.type() == ITEM_TYPE_ROOM and item.properties[iItemPropRoomPath] == room_path:
                for j in range(item.childCount()):
                    item.takeChild(j)

                self.ui.treeWidget.takeTopLevelItem(i)
                break

        else:
            print("Claudia - room delete failed")

    @pyqtSlot()
    def slot_DBusRoomChangedCallback(self):
        # Unused in ladish v1.0
        return

    @pyqtSlot(str, str)
    def slot_DBusProjectPropertiesChanged(self, path, name):
        has_project = bool(name)

        if has_project:
            item_string = " (%s)" % name
        else:
            item_string = ""

        self.ui.act_project_save.setEnabled(has_project)
        self.ui.act_project_save_as.setEnabled(has_project)
        self.ui.act_project_unload.setEnabled(has_project)
        self.ui.act_project_properties.setEnabled(has_project)
        self.ui.b_project_save.setEnabled(has_project)
        self.ui.b_project_save_as.setEnabled(has_project)
        self.ui.menu_Application.setEnabled(has_project)

        if path == "/org/ladish/Studio":
            top_level_item = self.ui.treeWidget.topLevelItem(0)
            room_name = ""

        else:
            for i in range(self.ui.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.ui.treeWidget.topLevelItem(i)
                if top_level_item and top_level_item.type() == ITEM_TYPE_ROOM and top_level_item.properties[iItemPropRoomPath] == path:
                    room_name = top_level_item.properties[iItemPropRoomName]
                    break
            else:
                return

        top_level_item.setText(0, "%s%s" % (room_name, item_string))

    @pyqtSlot(str, int, str, bool, bool, str)
    def slot_DBusAppAdded2Callback(self, path, number, name, active, terminal, level):
        if path == "/org/ladish/Studio":
            ITEM_TYPE = ITEM_TYPE_STUDIO_APP
            top_level_item = self.ui.treeWidget.topLevelItem(0)
        else:
            ITEM_TYPE = ITEM_TYPE_ROOM_APP
            for i in range(self.ui.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.ui.treeWidget.topLevelItem(i)
                if top_level_item and top_level_item.type() == ITEM_TYPE_ROOM and top_level_item.properties[iItemPropRoomPath] == path:
                    break
            else:
                return

        for i in range(top_level_item.childCount()):
            if top_level_item.child(i).properties[iItemPropNumber] == number:
                # App was added before, probably during reload/init
                return

        prop_obj = [None, None, None, None, None]
        prop_obj[iItemPropNumber]   = number
        prop_obj[iItemPropName]     = name
        prop_obj[iItemPropActive]   = active
        prop_obj[iItemPropTerminal] = terminal
        prop_obj[iItemPropLevel]    = level

        text = "["
        if level.isdigit():
            text += "L%s" % level
        elif level == "jacksession":
            text += "JS"
        else:
            text += level.upper()
        text += "] "
        if not active:
            text += "(inactive) "
        text += name

        item = QTreeWidgetItem(ITEM_TYPE)
        item.properties = prop_obj
        item.setText(0, text)
        top_level_item.addChild(item)

    @pyqtSlot(str, int)
    def slot_DBusAppRemovedCallback(self, path, number):
        if path == "/org/ladish/Studio":
            top_level_item = self.ui.treeWidget.topLevelItem(0)
        else:
            for i in range(self.ui.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.ui.treeWidget.topLevelItem(i)
                if top_level_item and top_level_item.type() == ITEM_TYPE_ROOM and top_level_item.properties[iItemPropRoomPath] == path:
                    break
            else:
                return

        for i in range(top_level_item.childCount()):
            if top_level_item.child(i).properties[iItemPropNumber] == number:
                top_level_item.takeChild(i)
                break

    @pyqtSlot(str, int, str, bool, bool, str)
    def slot_DBusAppStateChanged2Callback(self, path, number, name, active, terminal, level):
        if path == "/org/ladish/Studio":
            top_level_item = self.ui.treeWidget.topLevelItem(0)
        else:
            for i in range(self.ui.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.ui.treeWidget.topLevelItem(i)
                if top_level_item and top_level_item.type() == ITEM_TYPE_ROOM and top_level_item.properties[iItemPropRoomPath] == path:
                    break
            else:
                return

        prop_obj = [None, None, None, None, None]
        prop_obj[iItemPropNumber]   = number
        prop_obj[iItemPropName]     = name
        prop_obj[iItemPropActive]   = active
        prop_obj[iItemPropTerminal] = terminal
        prop_obj[iItemPropLevel]    = level

        text = "["
        if level.isdigit():
            text += "L%s" % level
        elif level == "jacksession":
            text += "JS"
        else:
            text += level.upper()
        text += "] "
        if not active:
            text += "(inactive) "
        text += name

        for i in range(top_level_item.childCount()):
            item = top_level_item.child(i)
            if item.properties[iItemPropNumber] == number:
                item.properties = prop_obj
                item.setText(0, text)
                break

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if gJack.client:
            gDBus.jack.ResetXruns()

    @pyqtSlot(int)
    def slot_JackBufferSizeCallback(self, bufferSize):
        self.ui_setBufferSize(bufferSize)

    @pyqtSlot(int)
    def slot_JackSampleRateCallback(self, sampleRate):
        self.ui_setSampleRate(sampleRate)

    @pyqtSlot()
    def slot_JackShutdownCallback(self):
        self.jackStopped()

    @pyqtSlot()
    def slot_A2JBridgeStart(self):
        ret = False
        if gDBus.a2j:
            ret = bool(gDBus.a2j.start())
        return ret

    @pyqtSlot()
    def slot_A2JBridgeStop(self):
        ret = False
        if gDBus.a2j:
            ret = bool(gDBus.a2j.stop())
        return ret

    @pyqtSlot()
    def slot_A2JBridgeExportHW(self):
        if gDBus.a2j:
            ask = QMessageBox.question(self, self.tr("A2J Hardware Export"), self.tr("Enable Hardware Export on the A2J Bridge?"), QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel, QMessageBox.No)
            if ask == QMessageBox.Yes:
                gDBus.a2j.set_hw_export(True)
            elif ask == QMessageBox.No:
                gDBus.a2j.set_hw_export(False)

    @pyqtSlot()
    def slot_configureClaudia(self):
        # Save groups position now
        if gDBus.patchbay:
            version, groups, conns = gDBus.patchbay.GetGraph(0)

            for group in groups:
                group_id, group_name, ports = group

                group_pos_i = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_OUTPUT)
                group_pos_o = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_INPUT)

                gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X, str(group_pos_o.x()))
                gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y, str(group_pos_o.y()))
                gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X_SPLIT, str(group_pos_i.x()))
                gDBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y_SPLIT, str(group_pos_i.y()))

        try:
            ladish_config = gDBus.bus.get_object("org.ladish.conf", "/org/ladish/conf")
        except:
            ladish_config = None

        if ladish_config:
            try:
                key_notify = bool(ladish_config.get(LADISH_CONF_KEY_DAEMON_NOTIFY)[0] == "true")
            except:
                key_notify = LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT

            try:
                key_shell = str(ladish_config.get(LADISH_CONF_KEY_DAEMON_SHELL)[0])
            except:
                key_shell = LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT

            try:
                key_terminal = str(ladish_config.get(LADISH_CONF_KEY_DAEMON_TERMINAL)[0])
            except:
                key_terminal = LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT

            try:
                key_studio_autostart = bool(ladish_config.get(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART)[0] == "true")
            except:
                key_studio_autostart = LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT

            try:
                key_js_save_delay = int(ladish_config.get(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY)[0])
            except:
                key_js_save_delay = LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT

            settings = QSettings()
            settings.setValue(LADISH_CONF_KEY_DAEMON_NOTIFY, key_notify)
            settings.setValue(LADISH_CONF_KEY_DAEMON_SHELL, key_shell)
            settings.setValue(LADISH_CONF_KEY_DAEMON_TERMINAL, key_terminal)
            settings.setValue(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, key_studio_autostart)
            settings.setValue(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, key_js_save_delay)
            del settings

        dialog = SettingsW(self, "claudia", hasGL)

        if not ladish_config:
            dialog.ui.lw_page.hideRow(2)

        if dialog.exec_():
            if ladish_config:
                settings = QSettings()
                ladish_config.set(LADISH_CONF_KEY_DAEMON_NOTIFY, "true" if (settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool)) else "false")
                ladish_config.set(LADISH_CONF_KEY_DAEMON_SHELL, settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
                ladish_config.set(LADISH_CONF_KEY_DAEMON_TERMINAL, settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
                ladish_config.set(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, "true" if (settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool)) else "false")
                ladish_config.set(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, str(settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int)))
                del settings

            self.loadSettings(False)
            patchcanvas.clear()

            pOptions = patchcanvas.options_t()
            pOptions.theme_name       = self.fSavedSettings["Canvas/Theme"]
            pOptions.auto_hide_groups = self.fSavedSettings["Canvas/AutoHideGroups"]
            pOptions.use_bezier_lines = self.fSavedSettings["Canvas/UseBezierLines"]
            pOptions.antialiasing     = self.fSavedSettings["Canvas/Antialiasing"]
            pOptions.eyecandy         = self.fSavedSettings["Canvas/EyeCandy"]

            pFeatures = patchcanvas.features_t()
            pFeatures.group_info   = False
            pFeatures.group_rename = True
            pFeatures.port_info    = True
            pFeatures.port_rename  = True
            pFeatures.handle_group_pos = False

            patchcanvas.setOptions(pOptions)
            patchcanvas.setFeatures(pFeatures)
            patchcanvas.init("Claudia", self.scene, self.canvasCallback, DEBUG)

            self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())

            if gDBus.ladish_control.IsStudioLoaded() and gDBus.ladish_studio and gDBus.ladish_studio.IsStarted():
                self.initPorts()

    @pyqtSlot()
    def slot_aboutClaudia(self):
        QMessageBox.about(self, self.tr("About Claudia"), self.tr("<h3>Claudia</h3>"
                                                                  "<br>Version %s"
                                                                  "<br>Claudia is a Graphical User Interface to LADISH.<br>"
                                                                  "<br>Copyright (C) 2010-2018 falkTX" % VERSION))

    def saveSettings(self):
        settings = QSettings()

        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("SplitterSizes", self.ui.splitter.saveState())
        settings.setValue("ShowToolbar", self.ui.frame_toolbar.isEnabled())
        settings.setValue("ShowStatusbar", self.ui.frame_statusbar.isEnabled())
        settings.setValue("TransportView", self.fCurTransportView)
        settings.setValue("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().value())

    def loadSettings(self, geometry):
        settings = QSettings()

        if geometry:
            self.restoreGeometry(settings.value("Geometry", b""))

            splitterSizes = settings.value("SplitterSizes", "")
            if splitterSizes:
                self.ui.splitter.restoreState(splitterSizes)
            else:
                self.ui.splitter.setSizes((100, 400))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.frame_toolbar.setVisible(showToolbar)

            showStatusbar = settings.value("ShowStatusbar", True, type=bool)
            self.ui.act_settings_show_statusbar.setChecked(showStatusbar)
            self.ui.frame_statusbar.setVisible(showStatusbar)

            self.setTransportView(settings.value("TransportView", TRANSPORT_VIEW_HMS, type=int))

        self.fSavedSettings = {
            "Main/DefaultProjectFolder": settings.value("Main/DefaultProjectFolder", DEFAULT_PROJECT_FOLDER, type=str),
            "Main/UseSystemTray": settings.value("Main/UseSystemTray", True, type=bool),
            "Main/CloseToTray": settings.value("Main/CloseToTray", False, type=bool),
            "Main/RefreshInterval": settings.value("Main/RefreshInterval", 120, type=int),
            "Canvas/Theme": settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/HighQualityAntialiasing": settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

        self.ui.act_app_add_new.setEnabled(USE_CLAUDIA_ADD_NEW)

    def resizeEvent(self, event):
        QTimer.singleShot(0, self.slot_miniCanvasCheckSize)
        QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.m_timer120:
            if gJack.client:
                self.refreshTransport()
                self.refreshXruns()
        elif event.timerId() == self.m_timer600:
            if gJack.client:
                self.refreshDSPLoad()
            else:
                self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        if self.systray:
            if self.fSavedSettings["Main/CloseToTray"]:
                if self.systray.handleQtCloseEvent(event):
                    patchcanvas.clear()
                return
            self.systray.close()
        patchcanvas.clear()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Claudia")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/claudia.svg"))

    if not haveDBus:
        QMessageBox.critical(None, app.translate("ClaudiaMainW", "Error"), app.translate("ClaudiaMainW",
            "DBus is not available, Claudia cannot start without it!"))
        sys.exit(1)

    gDBus.loop = DBusQtMainLoop(set_as_default=True)
    gDBus.bus  = dbus.SessionBus(mainloop=gDBus.loop)
    gDBus.jack = gDBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
    gDBus.ladish_control = gDBus.bus.get_object("org.ladish", "/org/ladish/Control")

    try:
        gDBus.ladish_app_daemon = gDBus.bus.get_object("org.ladish.appdb", "/")
    except:
        gDBus.ladish_app_daemon = None

    try:
        gDBus.a2j = dbus.Interface(gDBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
    except:
        gDBus.a2j = None

    jacksettings.initBus(gDBus.bus)

    # Show GUI
    gui = ClaudiaMainW()

    if gui.systray and "--minimized" in app.arguments():
        gui.hide()
        gui.systray.setActionText("show", gui.tr("Restore"))
    else:
        gui.show()

    # Set-up custom signal handling
    setUpSignals(gui)

    # App-Loop
    if gui.systray:
        ret = gui.systray.exec_(app)
    else:
        ret = app.exec_()

    # Close Jack
    if gJack.client:
        jacklib.deactivate(gJack.client)
        jacklib.client_close(gJack.client)

    # Exit properly
    sys.exit(ret)

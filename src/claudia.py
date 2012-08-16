#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LADISH frontend
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
from time import ctime
from PyQt4.QtCore import QPointF, QSettings
from PyQt4.QtGui import QAction, QApplication, QMainWindow, QVBoxLayout, QTableWidgetItem, QTreeWidgetItem

# Imports (Custom Stuff)
import claudia_launcher
import systray
import ui_claudia
import ui_claudia_studioname, ui_claudia_studiolist
import ui_claudia_createroom, ui_claudia_projectname, ui_claudia_projectproperties
import ui_claudia_runcustom
from shared_jack import *
from shared_canvas import *
from shared_settings import *

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

try:
    import dbus
    from dbus.mainloop.qt import DBusQtMainLoop
    haveDBus = True
except:
    haveDBus = False

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
JACKDBUS_PORT_FLAG_INPUT       = 0x00000001
JACKDBUS_PORT_FLAG_OUTPUT      = 0x00000002
JACKDBUS_PORT_FLAG_PHYSICAL    = 0x00000004
JACKDBUS_PORT_FLAG_CAN_MONITOR = 0x00000008
JACKDBUS_PORT_FLAG_TERMINAL    = 0x00000010

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
URI_CANVAS_SPLIT   = "http://kxstudio.sourceforge.net/ns/canvas/split"
URI_CANVAS_X_SPLIT = "http://kxstudio.sourceforge.net/ns/canvas/x_split"
URI_CANVAS_Y_SPLIT = "http://kxstudio.sourceforge.net/ns/canvas/y_split"

DEFAULT_CANVAS_WIDTH  = 3100
DEFAULT_CANVAS_HEIGHT = 2400

RECENT_PROJECTS_STORE_MAX_ITEMS = 50

# set default project folder
DEFAULT_PROJECT_FOLDER = os.path.join(HOME, "ladish-projects")
setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)

# Studio Name Dialog
class StudioNameW(QDialog, ui_claudia_studioname.Ui_StudioNameW):
    NEW = 1
    RENAME = 2
    SAVE_AS = 3

    def __init__(self, parent, mode):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if mode == self.NEW:
            self.setWindowTitle(self.tr("New studio"))
        elif mode == self.RENAME:
            self.setWindowTitle(self.tr("Rename studio"))
        elif mode == self.SAVE_AS:
            self.setWindowTitle(self.tr("Save studio as"))

        self.m_mode = mode
        self.m_studio_list = []

        if mode == self.RENAME and bool(DBus.ladish_control.IsStudioLoaded()):
            current_name = str(DBus.ladish_studio.GetName())
            self.m_studio_list.append(current_name)
            self.le_name.setText(current_name)

        studio_list = DBus.ladish_control.GetStudioList()
        for studio in studio_list:
            self.m_studio_list.append(str(studio[iStudioListName]))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.ret_studio_name = ""

    @pyqtSlot(str)
    def slot_checkText(self, text):
        if self.m_mode == self.SAVE_AS:
            check = bool(text)
        else:
            check = bool(text and text not in self.m_studio_list)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_studio_name = self.le_name.text()

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
        studio_list = DBus.ladish_control.GetStudioList()
        for studio in studio_list:
            name = str(studio[iStudioListName])
            date = ctime(float(studio[iStudioListDict]["Modification Time"]))

            w_name = QTableWidgetItem(name)
            w_date = QTableWidgetItem(date)
            self.tableWidget.insertRow(index)
            self.tableWidget.setItem(index, 0, w_name)
            self.tableWidget.setItem(index, 1, w_date)

            index += 1

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.tableWidget, SIGNAL("cellDoubleClicked(int, int)"), SLOT("accept()"))
        self.connect(self.tableWidget, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkSelection(int)"))

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

        templates_list = DBus.ladish_control.GetRoomTemplateList()
        for template_name, template_dict in templates_list:
            self.lw_templates.addItem(template_name)

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

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

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.b_open, SIGNAL("clicked()"), SLOT("slot_checkFolder()"))
        self.connect(self.le_path, SIGNAL("textChanged(QString)"), SLOT("slot_checkText_path(QString)"))
        self.connect(self.le_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText_name(QString)"))

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

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText_name(QString)"))
        self.connect(self.cb_save_now, SIGNAL("clicked(bool)"), SLOT("slot_checkSaveNow(bool)"))

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

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_command, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

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
        self.buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel, Qt.Horizontal, self)

        self.layoutR = QVBoxLayout(self)
        self.layoutR.addWidget(self.launcher)
        self.layoutR.addWidget(self.buttonBox)

        self.settings = QSettings("Cadence", "Claudia-Launcher")
        self.launcher.setCallbackApp(self, self.settings, True)
        self.loadSettings()

        self.m_appBus = appBus
        self.m_proj_folder = proj_folder
        self.m_is_room = is_room
        self.m_bpm = bpm
        self.m_sample_rate = sample_rate

        self.test_url = True
        self.test_selected = False

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.connect(self.buttonBox.button(QDialogButtonBox.Ok), SIGNAL("clicked()"), SLOT("slot_addAppToLADISH()"))
        self.connect(self.buttonBox.button(QDialogButtonBox.Cancel), SIGNAL("clicked()"), self, SLOT("reject()"))

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
        return self.m_sample_rate

    def callback_isLadishRoom(self):
        return self.m_is_room

    # ----------------------------------------

    @pyqtSlot()
    def slot_addAppToLADISH(self):
        self.launcher.addAppToLADISH()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.launcher.saveSettings()

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))
        self.launcher.loadSettings()

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Main Window
class ClaudiaMainW(QMainWindow, ui_claudia.Ui_ClaudiaMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Claudia")
        self.loadSettings(True)

        setIcons(self, ["canvas", "jack", "transport"])

        self.act_studio_new.setIcon(getIcon("document-new"))
        self.menu_studio_load.setIcon(getIcon("document-open"))
        self.act_studio_start.setIcon(getIcon("media-playback-start"))
        self.act_studio_stop.setIcon(getIcon("media-playback-stop"))
        self.act_studio_rename.setIcon(getIcon("edit-rename"))
        self.act_studio_save.setIcon(getIcon("document-save"))
        self.act_studio_save_as.setIcon(getIcon("document-save-as"))
        self.act_studio_unload.setIcon(getIcon("window-close"))
        self.menu_studio_delete.setIcon(getIcon("edit-delete"))
        self.b_studio_new.setIcon(getIcon("document-new"))
        self.b_studio_load.setIcon(getIcon("document-open"))
        self.b_studio_save.setIcon(getIcon("document-save"))
        self.b_studio_save_as.setIcon(getIcon("document-save-as"))

        self.act_room_create.setIcon(getIcon("list-add"))
        self.menu_room_delete.setIcon(getIcon("edit-delete"))

        self.act_project_new.setIcon(getIcon("document-new"))
        self.menu_project_load.setIcon(getIcon("document-open"))
        self.act_project_save.setIcon(getIcon("document-save"))
        self.act_project_save_as.setIcon(getIcon("document-save-as"))
        self.act_project_unload.setIcon(getIcon("window-close"))
        self.act_project_properties.setIcon(getIcon("edit-rename"))
        self.b_project_new.setIcon(getIcon("document-new"))
        self.b_project_load.setIcon(getIcon("document-open"))
        self.b_project_save.setIcon(getIcon("document-save"))
        self.b_project_save_as.setIcon(getIcon("document-save-as"))

        self.act_app_add_new.setIcon(getIcon("list-add"))
        self.act_app_run_custom.setIcon(getIcon("system-run"))

        self.act_tools_reactivate_ladishd.setIcon(getIcon("view-refresh"))
        self.act_quit.setIcon(getIcon("application-exit"))
        self.act_settings_configure.setIcon(getIcon("configure"))

        # Global Systray
        if self.m_savedSettings["Main/UseSystemTray"]:
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
            self.systray.connect("tools_configure_jack", lambda: slot_showJackSettings(self))
            self.systray.connect("tools_render", lambda: slot_showRender(self))
            self.systray.connect("tools_logs", lambda: slot_showLogs(self))
            self.systray.connect("tools_clear_xruns", self.slot_JackClearXruns)
            self.systray.connect("configure", self.slot_configureClaudia)

            self.systray.setToolTip("LADISH Frontend")
            self.systray.show()

        else:
            self.systray = None

        self.m_xruns = -1
        self.m_buffer_size = 0
        self.m_sample_rate = 0
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        self.m_last_item_type = None
        self.m_last_room_path = None

        self.cb_buffer_size.clear()
        self.cb_sample_rate.clear()

        for buffer_size in buffer_sizes:
            self.cb_buffer_size.addItem(str(buffer_size))

        for sample_rate in sample_rates:
            self.cb_sample_rate.addItem(str(sample_rate))

        self.scene = patchcanvas.PatchScene(self, self.graphicsView)
        self.graphicsView.setScene(self.scene)
        self.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.m_savedSettings["Canvas/Antialiasing"] == patchcanvas.ANTIALIASING_FULL))
        self.graphicsView.setRenderHint(QPainter.TextAntialiasing, self.m_savedSettings["Canvas/TextAntialiasing"])
        if self.m_savedSettings["Canvas/UseOpenGL"] and hasGL:
            self.graphicsView.setViewport(QGLWidget(self.graphicsView))
            self.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.m_savedSettings["Canvas/HighQualityAntialiasing"])

        p_options = patchcanvas.options_t()
        p_options.theme_name       = self.m_savedSettings["Canvas/Theme"]
        p_options.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
        p_options.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
        p_options.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
        p_options.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

        p_features = patchcanvas.features_t()
        p_features.group_info   = False
        p_features.group_rename = True
        p_features.port_info    = True
        p_features.port_rename  = True
        p_features.handle_group_pos = False

        patchcanvas.setOptions(p_options)
        patchcanvas.setFeatures(p_features)
        patchcanvas.init("Claudia", self.scene, self.canvasCallback, DEBUG)

        patchcanvas.setInitialPos(DEFAULT_CANVAS_WIDTH / 2, DEFAULT_CANVAS_HEIGHT / 2)
        patchcanvas.setCanvasSize(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        self.graphicsView.setSceneRect(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        self.miniCanvasPreview.setRealParent(self)
        self.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        if DBus.jack.IsStarted():
            self.jackStarted()
        else:
            self.jackStopped()

        if DBus.ladish_control.IsStudioLoaded():
            self.studioLoaded()
            if DBus.ladish_studio.IsStarted():
                self.studioStarted()
                self.init_ports()
            else:
                self.studioStopped()
        else:
            self.studioUnloaded()

        self.m_timer120 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])
        self.m_timer600 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 5)

        setCanvasConnections(self)
        setJackConnections(self, ["jack", "transport", "misc"])

        self.connect(self.act_studio_new, SIGNAL("triggered()"), SLOT("slot_studio_new()"))
        self.connect(self.act_studio_start, SIGNAL("triggered()"), SLOT("slot_studio_start()"))
        self.connect(self.act_studio_stop, SIGNAL("triggered()"), SLOT("slot_studio_stop()"))
        self.connect(self.act_studio_save, SIGNAL("triggered()"), SLOT("slot_studio_save()"))
        self.connect(self.act_studio_save_as, SIGNAL("triggered()"), SLOT("slot_studio_save_as()"))
        self.connect(self.act_studio_rename, SIGNAL("triggered()"), SLOT("slot_studio_rename()"))
        self.connect(self.act_studio_unload, SIGNAL("triggered()"), SLOT("slot_studio_unload()"))
        self.connect(self.b_studio_new, SIGNAL("clicked()"), SLOT("slot_studio_new()"))
        self.connect(self.b_studio_load, SIGNAL("clicked()"), SLOT("slot_studio_load_b()"))
        self.connect(self.b_studio_save, SIGNAL("clicked()"), SLOT("slot_studio_save()"))
        self.connect(self.b_studio_save_as, SIGNAL("clicked()"), SLOT("slot_studio_save_as()"))
        self.connect(self.menu_studio_load, SIGNAL("aboutToShow()"), SLOT("slot_updateMenuStudioList_Load()"))
        self.connect(self.menu_studio_delete, SIGNAL("aboutToShow()"), SLOT("slot_updateMenuStudioList_Delete()"))

        self.connect(self.act_room_create, SIGNAL("triggered()"), SLOT("slot_room_create()"))
        self.connect(self.menu_room_delete, SIGNAL("aboutToShow()"), SLOT("slot_updateMenuRoomList()"))

        self.connect(self.act_project_new, SIGNAL("triggered()"), SLOT("slot_project_new()"))
        self.connect(self.act_project_save, SIGNAL("triggered()"), SLOT("slot_project_save()"))
        self.connect(self.act_project_save_as, SIGNAL("triggered()"), SLOT("slot_project_save_as()"))
        self.connect(self.act_project_unload, SIGNAL("triggered()"), SLOT("slot_project_unload()"))
        self.connect(self.act_project_properties, SIGNAL("triggered()"), SLOT("slot_project_properties()"))
        self.connect(self.b_project_new, SIGNAL("clicked()"), SLOT("slot_project_new()"))
        self.connect(self.b_project_load, SIGNAL("clicked()"), SLOT("slot_project_load()"))
        self.connect(self.b_project_save, SIGNAL("clicked()"), SLOT("slot_project_save()"))
        self.connect(self.b_project_save_as, SIGNAL("clicked()"), SLOT("slot_project_save_as()"))
        self.connect(self.menu_project_load, SIGNAL("aboutToShow()"), SLOT("slot_updateMenuProjectList()"))

        self.connect(self.act_app_add_new, SIGNAL("triggered()"), SLOT("slot_app_add_new()"))
        self.connect(self.act_app_run_custom, SIGNAL("triggered()"), SLOT("slot_app_run_custom()"))

        self.connect(self.treeWidget, SIGNAL("itemSelectionChanged()"), SLOT("slot_checkCurrentRoom()"))
        #self.connect(self.treeWidget, SIGNAL("itemPressed(QTreeWidgetItem*, int)"), SLOT("slot_checkCurrentRoom()"))
        self.connect(self.treeWidget, SIGNAL("itemDoubleClicked(QTreeWidgetItem*, int)"), SLOT("slot_doubleClickedAppList(QTreeWidgetItem*, int)"))
        self.connect(self.treeWidget, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showAppListCustomMenu()"))

        self.connect(self.miniCanvasPreview, SIGNAL("miniCanvasMoved(double, double)"), SLOT("slot_miniCanvasMoved(double, double)"))

        self.connect(self.graphicsView.horizontalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_horizontalScrollBarChanged(int)"))
        self.connect(self.graphicsView.verticalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_verticalScrollBarChanged(int)"))

        self.connect(self.scene, SIGNAL("sceneGroupMoved(int, int, QPointF)"), SLOT("slot_canvasItemMoved(int, int, QPointF)"))
        self.connect(self.scene, SIGNAL("scaleChanged(double)"), SLOT("slot_canvasScaleChanged(double)"))

        self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureClaudia()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutClaudia()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        # org.freedesktop.DBus
        self.connect(self, SIGNAL("DBusCrashCallback(QString)"), SLOT("slot_DBusCrashCallback(QString)"))

        # org.jackaudio.JackControl
        self.connect(self, SIGNAL("DBusServerStartedCallback()"), SLOT("slot_DBusServerStartedCallback()"))
        self.connect(self, SIGNAL("DBusServerStoppedCallback()"), SLOT("slot_DBusServerStoppedCallback()"))

        # org.jackaudio.JackPatchbay
        self.connect(self, SIGNAL("DBusClientAppearedCallback(int, QString)"), SLOT("slot_DBusClientAppearedCallback(int, QString)"))
        self.connect(self, SIGNAL("DBusClientDisappearedCallback(int)"), SLOT("slot_DBusClientDisappearedCallback(int)"))
        self.connect(self, SIGNAL("DBusClientRenamedCallback(int, QString)"), SLOT("slot_DBusClientRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("DBusPortAppearedCallback(int, int, QString, int, int)"), SLOT("slot_DBusPortAppearedCallback(int, int, QString, int, int)"))
        self.connect(self, SIGNAL("DBusPortDisppearedCallback(int)"), SLOT("slot_DBusPortDisppearedCallback(int)"))
        self.connect(self, SIGNAL("DBusPortRenamedCallback(int, QString)"), SLOT("slot_DBusPortRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("DBusPortsConnectedCallback(int, int, int)"), SLOT("slot_DBusPortsConnectedCallback(int, int, int)"))
        self.connect(self, SIGNAL("DBusPortsDisconnectedCallback(int)"), SLOT("slot_DBusPortsDisconnectedCallback(int)"))

        # org.ladish.Control
        self.connect(self, SIGNAL("DBusStudioAppearedCallback()"), SLOT("slot_DBusStudioAppearedCallback()"))
        self.connect(self, SIGNAL("DBusStudioDisappearedCallback()"), SLOT("slot_DBusStudioDisappearedCallback()"))
        self.connect(self, SIGNAL("DBusQueueExecutionHaltedCallback()"), SLOT("slot_DBusQueueExecutionHaltedCallback()"))
        self.connect(self, SIGNAL("DBusCleanExitCallback()"), SLOT("slot_DBusCleanExitCallback()"))

        # org.ladish.Studio
        self.connect(self, SIGNAL("DBusStudioStartedCallback()"), SLOT("slot_DBusStudioStartedCallback()"))
        self.connect(self, SIGNAL("DBusStudioStoppedCallback()"), SLOT("slot_DBusStudioStoppedCallback()"))
        self.connect(self, SIGNAL("DBusStudioRenamedCallback(QString)"), SLOT("slot_DBusStudioRenamedCallback(QString)"))
        self.connect(self, SIGNAL("DBusStudioCrashedCallback()"), SLOT("slot_DBusStudioCrashedCallback()"))
        self.connect(self, SIGNAL("DBusRoomAppearedCallback(QString, QString)"), SLOT("slot_DBusRoomAppearedCallback(QString, QString)"))
        self.connect(self, SIGNAL("DBusRoomDisappearedCallback(QString)"), SLOT("slot_DBusRoomDisappearedCallback(QString)"))
        #self.connect(self, SIGNAL("DBusRoomChangedCallback()"), SLOT("slot_DBusRoomChangedCallback()"))

        # org.ladish.Room
        self.connect(self, SIGNAL("DBusProjectPropertiesChanged(QString, QString)"), SLOT("slot_DBusProjectPropertiesChanged(QString, QString)"))

        # org.ladish.AppSupervisor
        self.connect(self, SIGNAL("DBusAppAdded2Callback(QString, int, QString, bool, bool, QString)"), SLOT("slot_DBusAppAdded2Callback(QString, int, QString, bool, bool, QString)"))
        self.connect(self, SIGNAL("DBusAppRemovedCallback(QString, int)"), SLOT("slot_DBusAppRemovedCallback(QString, int)"))
        self.connect(self, SIGNAL("DBusAppStateChanged2Callback(QString, int, QString, bool, bool, QString)"), SLOT("slot_DBusAppStateChanged2Callback(QString, int, QString, bool, bool, QString)"))

        # JACK
        self.connect(self, SIGNAL("JackBufferSizeCallback(int)"), SLOT("slot_JackBufferSizeCallback(int)"))
        self.connect(self, SIGNAL("JackSampleRateCallback(int)"), SLOT("slot_JackSampleRateCallback(int)"))
        self.connect(self, SIGNAL("JackShutdownCallback()"), SLOT("slot_JackShutdownCallback()"))

        # DBus Stuff
        DBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword='dest', path_keyword='path',
            member_keyword='member', interface_keyword='interface', sender_keyword='sender', )

        QTimer.singleShot(100, self, SLOT("slot_miniCanvasInit()"))

    def canvasCallback(self, action, value1, value2, value_str):
        if action == patchcanvas.ACTION_GROUP_INFO:
            pass

        elif action == patchcanvas.ACTION_GROUP_RENAME:
            group_id = value1
            group_name = value_str
            DBus.ladish_manager.RenameClient(group_id, group_name)

        elif action == patchcanvas.ACTION_GROUP_SPLIT:
            group_id = value1
            DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "true")

            patchcanvas.splitGroup(group_id)
            self.miniCanvasPreview.update()

        elif action == patchcanvas.ACTION_GROUP_JOIN:
            group_id = value1
            DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "false")

            patchcanvas.joinGroup(group_id)
            self.miniCanvasPreview.update()

        elif action == patchcanvas.ACTION_PORT_INFO:
            this_port_id = value1
            breakNow = False

            port_id    = 0
            port_flags = 0
            port_name  = ""
            port_type_jack = 0

            version, groups, conns = DBus.patchbay.GetGraph(0)

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
            DBus.ladish_manager.RenamePort(port_id, port_name)

        elif action == patchcanvas.ACTION_PORTS_CONNECT:
            port_a = value1
            port_b = value2
            DBus.patchbay.ConnectPortsByID(port_a, port_b)

        elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
            connection_id = value1
            DBus.patchbay.DisconnectPortsByConnectionID(connection_id)

    def init_jack(self):
        self.m_xruns = -1
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        buffer_size = int(jacklib.get_buffer_size(jack.client))
        sample_rate = int(jacklib.get_sample_rate(jack.client))
        realtime = bool(int(jacklib.is_realtime(jack.client)))

        setBufferSize(self, buffer_size)
        setSampleRate(self, sample_rate)
        setRealTime(self, realtime)

        refreshDSPLoad(self)
        refreshTransport(self)
        self.refreshXruns()

        self.init_callbacks()

        jacklib.activate(jack.client)

    def init_callbacks(self):
        jacklib.set_buffer_size_callback(jack.client, self.JackBufferSizeCallback, None)
        jacklib.set_sample_rate_callback(jack.client, self.JackSampleRateCallback, None)
        jacklib.on_shutdown(jack.client, self.JackShutdownCallback, None)

    def init_studio(self):
        self.treeWidget.clear()

        studio_item = QTreeWidgetItem(ITEM_TYPE_STUDIO)
        studio_item.setText(0, str(DBus.ladish_studio.GetName()))
        self.treeWidget.insertTopLevelItem(0, studio_item)
        self.treeWidget.setCurrentItem(studio_item)

        self.m_last_item_type = ITEM_TYPE_STUDIO
        self.m_last_room_path = None

        self.init_apps()

    def init_apps(self):
        studio_iface = dbus.Interface(DBus.ladish_studio, 'org.ladish.AppSupervisor')
        studio_item = self.treeWidget.topLevelItem(0)

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

        room_list = DBus.ladish_studio.GetRoomList()

        for room in room_list:
            room_path, room_dict = room
            ladish_room = DBus.bus.get_object("org.ladish", room_path)
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

        self.treeWidget.expandAll()

    def init_ports(self):
        #FIXME
        if (not jack.client or not DBus.patchbay):
            return

        version, groups, conns = DBus.patchbay.GetGraph(0)

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
                    if DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes":
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

        QTimer.singleShot(1000 if (self.m_savedSettings['Canvas/EyeCandy']) else 0, self.miniCanvasPreview, SLOT("update()"))

    def room_add(self, room_path, room_name):
        room_index  = int(room_path.replace("/org/ladish/Room", ""))
        room_object = DBus.bus.get_object("org.ladish", room_path)
        room_project_properties = room_object.GetProjectProperties()

        # Remove old unused item if needed
        iItem = self.treeWidget.topLevelItem(room_index)
        if iItem and not iItem.isVisible():
            self.treeWidget.takeTopLevelItem(room_index)

        # Insert padding of items if needed
        for i in range(room_index):
            if not self.treeWidget.topLevelItem(i):
                fake_item = QTreeWidgetItem(ITEM_TYPE_NULL)
                self.treeWidget.insertTopLevelItem(i, fake_item)
                fake_item.setHidden(True)

        graph_version, project_properties = room_project_properties

        if len(project_properties) > 0:
            item_string = " (%s)" % (project_properties['name'])
        else:
            item_string = ""

        prop_obj = [None, None]
        prop_obj[iItemPropRoomPath] = room_path
        prop_obj[iItemPropRoomName] = room_name

        item = QTreeWidgetItem(ITEM_TYPE_ROOM)
        item.properties = prop_obj
        item.setText(0, "%s%s" % (room_name, item_string))

        self.treeWidget.insertTopLevelItem(room_index, item)
        self.treeWidget.expandItem(item)

        return item

    def canvas_add_group(self, group_id, group_name):
        # TODO - request ladish client type

        #if (False):
            #icon  = patchcanvas.ICON_HARDWARE
            #split = patchcanvas.SPLIT_NO
        #elif (False):
            #icon  = patchcanvas.ICON_LADISH_ROOM
            #split = patchcanvas.SPLIT_NO
        #else:
        icon = patchcanvas.ICON_APPLICATION

        split_try = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT)
        if split_try == "true":
            split = patchcanvas.SPLIT_YES
        elif split_try == "false":
            split = patchcanvas.SPLIT_NO
        else:
            split = patchcanvas.SPLIT_UNDEF

        patchcanvas.addGroup(group_id, group_name, split, icon)

        x  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X)
        y  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y)
        x2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X_SPLIT)
        y2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y_SPLIT)

        if x != None and y != None:
            if x2 is None: x2 = "%f" % (float(x) + 50)
            if y2 is None: y2 = "%f" % (float(y) + 50)
            patchcanvas.setGroupPosFull(group_id, float(x), float(y), float(x2), float(y2))

        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_remove_group(self, group_id):
        patchcanvas.removeGroup(group_id)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_rename_group(self, group_id, new_group_name):
        patchcanvas.renameGroup(group_id, new_group_name)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_add_port(self, group_id, port_id, port_name, port_mode, port_type):
        patchcanvas.addPort(group_id, port_id, port_name, port_mode, port_type)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_remove_port(self, port_id):
        patchcanvas.removePort(port_id)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_rename_port(self, port_id, new_port_name):
        patchcanvas.renamePort(port_id, new_port_name)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_connect_ports(self, connection_id, port_a, port_b):
        patchcanvas.connectPorts(connection_id, port_a, port_b)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_disconnect_ports(self, connection_id):
        patchcanvas.disconnectPorts(connection_id)
        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def jackStarted(self):
        #self.DBusReconnect()

        if not jack.client:
            jack.client = jacklib.client_open("claudia", jacklib.JackNoStartServer, None)
            if not jack.client:
                return self.jackStopped()

        self.act_jack_render.setEnabled(canRender)
        self.b_jack_render.setEnabled(canRender)
        self.menuJackTransport(True)

        self.cb_buffer_size.setEnabled(True)
        self.cb_sample_rate.setEnabled(True) # jacksettings.getSampleRate() != -1

        if self.systray:
            self.systray.setActionEnabled("tools_render", canRender)

        self.pb_dsp_load.setMaximum(100)
        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.update()

        self.init_jack()

    def jackStopped(self):
        #self.DBusReconnect()

        # client already closed
        jack.client = None

        if self.m_next_sample_rate:
            jack_sample_rate(self, self.m_next_sample_rate)

        buffer_size = jacksettings.getBufferSize()
        sample_rate = jacksettings.getSampleRate()
        buffer_size_test = bool(buffer_size != -1)
        sample_rate_test = bool(sample_rate != -1)

        if buffer_size_test:
            setBufferSize(self, buffer_size)

        if sample_rate_test:
            setSampleRate(self, sample_rate)

        setRealTime(self, jacksettings.isRealtime())
        setXruns(self, -1)

        self.cb_buffer_size.setEnabled(buffer_size_test)
        self.cb_sample_rate.setEnabled(sample_rate_test)

        self.act_jack_render.setEnabled(False)
        self.b_jack_render.setEnabled(False)
        self.menuJackTransport(False)

        if self.systray:
            self.systray.setActionEnabled("tools_render", False)

        if self.m_selected_transport_view == TRANSPORT_VIEW_HMS:
            self.label_time.setText("00:00:00")
        elif self.m_selected_transport_view == TRANSPORT_VIEW_BBT:
            self.label_time.setText("000|0|0000")
        elif self.m_selected_transport_view == TRANSPORT_VIEW_FRAMES:
            self.label_time.setText("000'000'000")

        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.setMaximum(0)
        self.pb_dsp_load.update()

    def studioStarted(self):
        self.act_studio_start.setEnabled(False)
        self.act_studio_stop.setEnabled(True)
        self.act_studio_save.setEnabled(True)
        self.act_studio_save_as.setEnabled(True)

        self.b_studio_save.setEnabled(True)
        self.b_studio_save_as.setEnabled(True)

        if self.systray:
            self.systray.setActionEnabled("studio_start", False)
            self.systray.setActionEnabled("studio_stop", True)
            self.systray.setActionEnabled("studio_save", True)
            self.systray.setActionEnabled("studio_save_as", True)

    def studioStopped(self):
        self.act_studio_start.setEnabled(True)
        self.act_studio_stop.setEnabled(False)
        self.act_studio_save.setEnabled(False)
        self.act_studio_save_as.setEnabled(False)

        self.b_studio_save.setEnabled(False)
        self.b_studio_save_as.setEnabled(False)

        if self.systray:
            self.systray.setActionEnabled("studio_start", True)
            self.systray.setActionEnabled("studio_stop", False)
            self.systray.setActionEnabled("studio_save", False)
            self.systray.setActionEnabled("studio_save_as", False)

    def studioLoaded(self):
        DBus.ladish_studio  = DBus.bus.get_object("org.ladish", "/org/ladish/Studio")
        DBus.ladish_graph   = dbus.Interface(DBus.ladish_studio, 'org.ladish.GraphDict')
        DBus.ladish_manager = dbus.Interface(DBus.ladish_studio, 'org.ladish.GraphManager')
        DBus.ladish_app_iface = dbus.Interface(DBus.ladish_studio, 'org.ladish.AppSupervisor')
        DBus.patchbay = dbus.Interface(DBus.ladish_studio, 'org.jackaudio.JackPatchbay')

        self.label_first_time.setVisible(False)
        self.graphicsView.setVisible(True)
        self.miniCanvasPreview.setVisible(True)
        #if (self.miniCanvasPreview.is_initiated):
            #self.checkMiniCanvasSize()

        self.menu_Room.setEnabled(True)
        self.menu_Project.setEnabled(False)
        self.menu_Application.setEnabled(True)
        self.group_project.setEnabled(False)

        self.act_studio_rename.setEnabled(True)
        self.act_studio_unload.setEnabled(True)

        if self.systray:
            self.systray.setActionEnabled("studio_rename", True)
            self.systray.setActionEnabled("studio_unload", True)

        self.init_studio()

    def studioUnloaded(self):
        DBus.ladish_studio  = None
        DBus.ladish_graph   = None
        DBus.ladish_manager = None
        DBus.ladish_app_iface = None
        DBus.patchbay = None

        self.m_last_item_type = None
        self.m_last_room_path = None

        self.label_first_time.setVisible(True)
        self.graphicsView.setVisible(False)
        self.miniCanvasPreview.setVisible(False)

        self.menu_Room.setEnabled(False)
        self.menu_Project.setEnabled(False)
        self.menu_Application.setEnabled(False)
        self.group_project.setEnabled(False)

        self.act_studio_start.setEnabled(False)
        self.act_studio_stop.setEnabled(False)
        self.act_studio_rename.setEnabled(False)
        self.act_studio_save.setEnabled(False)
        self.act_studio_save_as.setEnabled(False)
        self.act_studio_unload.setEnabled(False)

        self.b_studio_save.setEnabled(False)
        self.b_studio_save_as.setEnabled(False)

        if self.systray:
            self.systray.setActionEnabled("studio_start", False)
            self.systray.setActionEnabled("studio_stop", False)
            self.systray.setActionEnabled("studio_rename", False)
            self.systray.setActionEnabled("studio_save", False)
            self.systray.setActionEnabled("studio_save_as", False)
            self.systray.setActionEnabled("studio_unload", False)

        self.treeWidget.clear()

        patchcanvas.clear()

    def menuJackTransport(self, enabled):
        self.act_transport_play.setEnabled(enabled)
        self.act_transport_stop.setEnabled(enabled)
        self.act_transport_backwards.setEnabled(enabled)
        self.act_transport_forwards.setEnabled(enabled)
        self.menu_Transport.setEnabled(enabled)
        self.group_transport.setEnabled(enabled)

    def DBusSignalReceiver(self, *args, **kwds):
        if kwds['interface'] == "org.freedesktop.DBus" and kwds['path'] == "/org/freedesktop/DBus" and kwds['member'] == "NameOwnerChanged":
            appInterface, appId, newId = args
            #print("appInterface crashed", appInterface)

            if not newId:
                # Something crashed
                if appInterface in ("org.jackaudio.service", "org.ladish"):
                    self.emit(SIGNAL("DBusCrashCallback(QString)"), appInterface)

        elif kwds['interface'] == "org.jackaudio.JackControl":
            if DEBUG: print("DBus signal @org.jackaudio.JackControl,", kwds['member'])
            if kwds['member'] == "ServerStarted":
                self.emit(SIGNAL("DBusServerStartedCallback()"))
            elif kwds['member'] == "ServerStopped":
                self.emit(SIGNAL("DBusServerStoppedCallback()"))

        elif kwds['interface'] == "org.jackaudio.JackPatchbay":
            if DBus.patchbay and kwds['path'] == DBus.patchbay.object_path:
                if DEBUG: print("DBus signal @org.jackaudio.JackPatchbay,", kwds['member'])
                if kwds['member'] == "ClientAppeared":
                    self.emit(SIGNAL("DBusClientAppearedCallback(int, QString)"), args[iJackClientId], args[iJackClientName])
                elif kwds['member'] == "ClientDisappeared":
                    self.emit(SIGNAL("DBusClientDisappearedCallback(int)"), args[iJackClientId])
                elif kwds['member'] == "ClientRenamed":
                    self.emit(SIGNAL("DBusClientRenamedCallback(int, QString)"), args[iRenamedId], args[iRenamedNewName])
                elif kwds['member'] == "PortAppeared":
                    self.emit(SIGNAL("DBusPortAppearedCallback(int, int, QString, int, int)"), args[iJackClientId], args[iJackPortId], args[iJackPortName], args[iJackPortFlags], args[iJackPortType])
                elif kwds['member'] == "PortDisappeared":
                    self.emit(SIGNAL("DBusPortDisppearedCallback(int)"), args[iJackPortId])
                elif kwds['member'] == "PortRenamed":
                    self.emit(SIGNAL("DBusPortRenamedCallback(int, QString)"), args[iJackPortId], args[iJackPortNewName])
                elif kwds['member'] == "PortsConnected":
                    self.emit(SIGNAL("DBusPortsConnectedCallback(int, int, int)"), args[iJackConnId], args[iSourcePortId], args[iTargetPortId])
                elif kwds['member'] == "PortsDisconnected":
                    self.emit(SIGNAL("DBusPortsDisconnectedCallback(int)"), args[iJackConnId])

        elif kwds['interface'] == "org.ladish.Control":
            if DEBUG: print("DBus signal @org.ladish.Control,", kwds['member'])
            if kwds['member'] == "StudioAppeared":
                self.emit(SIGNAL("DBusStudioAppearedCallback()"))
            elif kwds['member'] == "StudioDisappeared":
                self.emit(SIGNAL("DBusStudioDisappearedCallback()"))
            elif kwds['member'] == "QueueExecutionHalted":
                self.emit(SIGNAL("DBusQueueExecutionHaltedCallback()"))
            elif kwds['member'] == "CleanExit":
                self.emit(SIGNAL("DBusCleanExitCallback()"))

        elif kwds['interface'] == "org.ladish.Studio":
            if DEBUG: print("DBus signal @org.ladish.Studio,", kwds['member'])
            if kwds['member'] == "StudioStarted":
                self.emit(SIGNAL("DBusStudioStartedCallback()"))
            elif kwds['member'] == "StudioStopped":
                self.emit(SIGNAL("DBusStudioStoppedCallback()"))
            elif kwds['member'] == "StudioRenamed":
                self.emit(SIGNAL("DBusStudioRenamedCallback(QString)"), args[iStudioRenamedName])
            elif kwds['member'] == "StudioCrashed":
                self.emit(SIGNAL("DBusStudioCrashedCallback()"))
            elif kwds['member'] == "RoomAppeared":
                self.emit(SIGNAL("DBusRoomAppearedCallback(QString, QString)"), args[iRoomAppearedPath], args[iRoomAppearedDict]['name'])
            elif kwds['member'] == "RoomDisappeared":
                self.emit(SIGNAL("DBusRoomDisappearedCallback(QString)"), args[iRoomAppearedPath])
            #elif kwds['member'] == "RoomChanged":
                #self.emit(SIGNAL("DBusRoomChangedCallback()"))

        elif kwds['interface'] == "org.ladish.Room":
            if DEBUG: print("DBus signal @org.ladish.Room,", kwds['member'])
            if kwds['member'] == "ProjectPropertiesChanged":
                if "name" in args[iProjChangedDict].keys():
                    self.emit(SIGNAL("DBusProjectPropertiesChanged(QString, QString)"), kwds['path'], args[iProjChangedDict]['name'])
                else:
                    self.emit(SIGNAL("DBusProjectPropertiesChanged(QString, QString)"), kwds['path'], "")

        elif kwds['interface'] == "org.ladish.AppSupervisor":
            if DEBUG: print("DBus signal @org.ladish.AppSupervisor,", kwds['member'])
            if kwds['member'] == "AppAdded2":
                self.emit(SIGNAL("DBusAppAdded2Callback(QString, int, QString, bool, bool, QString)"), kwds['path'], args[iAppChangedNumber], args[iAppChangedName], args[iAppChangedActive], args[iAppChangedTerminal], args[iAppChangedLevel])
            elif kwds['member'] == "AppRemoved":
                self.emit(SIGNAL("DBusAppRemovedCallback(QString, int)"), kwds['path'], args[iAppChangedNumber])
            elif kwds['member'] == "AppStateChanged2":
                self.emit(SIGNAL("DBusAppStateChanged2Callback(QString, int, QString, bool, bool, QString)"), kwds['path'], args[iAppChangedNumber], args[iAppChangedName], args[iAppChangedActive], args[iAppChangedTerminal], args[iAppChangedLevel])

    def DBusReconnect(self):
        DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
        DBus.ladish_control = DBus.bus.get_object("org.ladish", "/org/ladish/Control")
        DBus.ladish_studio = None
        DBus.ladish_room = None
        DBus.ladish_graph = None
        DBus.ladish_manager = None
        DBus.ladish_app_iface = None
        DBus.patchbay = None

        try:
            DBus.ladish_app_daemon = DBus.bus.get_object("org.ladish.appdb", "/")
        except:
            DBus.ladish_app_daemon = None

        jacksettings.initBus(DBus.bus)

    def refreshXruns(self):
        xruns = int(DBus.jack.GetXruns())
        if self.m_xruns != xruns:
            setXruns(self, xruns)
            self.m_xruns = xruns

    def JackBufferSizeCallback(self, buffer_size, arg):
        if DEBUG: print("JackBufferSizeCallback(%i)" % buffer_size)
        self.emit(SIGNAL("JackBufferSizeCallback(int)"), buffer_size)
        return 0

    def JackSampleRateCallback(self, sample_rate, arg):
        if DEBUG: print("JackSampleRateCallback(%i)" % sample_rate)
        self.emit(SIGNAL("JackSampleRateCallback(int)"), sample_rate)
        return 0

    def JackShutdownCallback(self, arg):
        if DEBUG: print("JackShutdownCallback()")
        self.emit(SIGNAL("JackShutdownCallback()"))
        return 0

    @pyqtSlot()
    def slot_studio_new(self):
        dialog = StudioNameW(self, StudioNameW.NEW)
        if dialog.exec_():
            DBus.ladish_control.NewStudio(dialog.ret_studio_name)

    @pyqtSlot()
    def slot_studio_load_b(self):
        dialog = StudioListW(self)
        if dialog.exec_():
            DBus.ladish_control.LoadStudio(dialog.ret_studio_name)

    @pyqtSlot()
    def slot_studio_load_m(self):
        studio_name = self.sender().text()
        if studio_name:
            DBus.ladish_control.LoadStudio(studio_name)

    @pyqtSlot()
    def slot_studio_start(self):
        DBus.ladish_studio.Start()

    @pyqtSlot()
    def slot_studio_stop(self):
        DBus.ladish_studio.Stop()

    @pyqtSlot()
    def slot_studio_rename(self):
        dialog = StudioNameW(self, StudioNameW.RENAME)
        if dialog.exec_():
            DBus.ladish_studio.Rename(dialog.ret_studio_name)

    @pyqtSlot()
    def slot_studio_save(self):
        DBus.ladish_studio.Save()

    @pyqtSlot()
    def slot_studio_save_as(self):
        dialog = StudioNameW(self, StudioNameW.SAVE_AS)
        if dialog.exec_():
            DBus.ladish_studio.SaveAs(dialog.ret_studio_name)

    @pyqtSlot()
    def slot_studio_unload(self):
        DBus.ladish_studio.Unload()

    @pyqtSlot()
    def slot_studio_delete_m(self):
        studio_name = self.sender().text()
        if studio_name:
            DBus.ladish_control.DeleteStudio(studio_name)

    @pyqtSlot()
    def slot_room_create(self):
        dialog = CreateRoomW(self)
        if dialog.exec_():
            DBus.ladish_studio.CreateRoom(dialog.ret_room_name, dialog.ret_room_template)

    @pyqtSlot()
    def slot_room_delete_m(self):
        room_name = self.sender().text()
        if room_name:
            DBus.ladish_studio.DeleteRoom(room_name)

    @pyqtSlot()
    def slot_project_new(self):
        dialog = ProjectNameW(self, ProjectNameW.NEW, self.m_savedSettings["Main/DefaultProjectFolder"])
        if dialog.exec_():
            # Check if a project is already loaded, if yes unload it first
            project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()
            if len(project_properties) > 0:
                DBus.ladish_room.UnloadProject()
            DBus.ladish_room.SaveProject(dialog.ret_project_path, dialog.ret_project_name)

    @pyqtSlot()
    def slot_project_load(self):
        project_path = QFileDialog.getExistingDirectory(self, self.tr("Open Project"), self.m_savedSettings["Main/DefaultProjectFolder"])
        if project_path:
            if os.path.exists(os.path.join(project_path, "ladish-project.xml")):
                DBus.ladish_room.LoadProject(project_path)
            else:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("The selected folder does not contain a ladish project"))

    @pyqtSlot()
    def slot_project_load_m(self):
        act_x_text = self.sender().text()
        if act_x_text:
            proj_path = "/" + act_x_text.rsplit("[/", 1)[-1].rsplit("]", 1)[0]
            DBus.ladish_room.LoadProject(proj_path)

    @pyqtSlot()
    def slot_project_save(self):
        project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()
        if len(project_properties) > 0:
            path = dbus.String(project_properties['dir'])
            name = dbus.String(project_properties['name'])
            DBus.ladish_room.SaveProject(path, name)
        else:
            self.slot_project_new()

    @pyqtSlot()
    def slot_project_save_as(self):
        project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()

        if len(project_properties) > 0:
            path = str(project_properties['dir'])
            name = str(project_properties['name'])
            dialog = ProjectNameW(self, ProjectNameW.SAVE_AS, self.m_savedSettings["Main/DefaultProjectFolder"], path, name)

            if dialog.exec_():
                DBus.ladish_room.SaveProject(dialog.ret_project_path, dialog.ret_project_name)

        else:
            self.slot_project_new()

    @pyqtSlot()
    def slot_project_unload(self):
        DBus.ladish_room.UnloadProject()

    @pyqtSlot()
    def slot_project_properties(self):
        project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()

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
            DBus.ladish_room.SetProjectDescription(dialog.ret_obj[iAppPropDescription])
            DBus.ladish_room.SetProjectNotes(dialog.ret_obj[iAppPropNotes])

            if dialog.ret_obj[iAppPropSaveNow]:
                DBus.ladish_room.SaveProject(path, dialog.ret_obj[iAppPropName])

    @pyqtSlot()
    def slot_app_add_new(self):
        proj_folder = ""

        if self.m_last_item_type == ITEM_TYPE_STUDIO or self.m_last_item_type == ITEM_TYPE_STUDIO_APP:
            proj_folder = self.m_savedSettings['Main/DefaultProjectFolder']
            is_room = False

        elif self.m_last_item_type == ITEM_TYPE_ROOM or self.m_last_item_type == ITEM_TYPE_ROOM_APP:
            project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()

            if len(project_properties) > 0:
                proj_folder = str(project_properties['dir'])
                is_room = True
            else:
                proj_folder = self.m_savedSettings['Main/DefaultProjectFolder']
                is_room = False

        else:
            print("Invalid m_last_item_type value")
            return

        dialog = ClaudiaLauncherW(self, DBus.ladish_app_iface, proj_folder, is_room, self.m_last_bpm, self.m_sample_rate)
        dialog.exec_()

    @pyqtSlot()
    def slot_app_run_custom(self):
        dialog = RunCustomW(self, bool(self.m_last_item_type in (ITEM_TYPE_ROOM, ITEM_TYPE_ROOM_APP)))
        if dialog.exec_() and dialog.ret_app_obj:
            app_obj = dialog.ret_app_obj
            DBus.ladish_app_iface.RunCustom2(app_obj[iAppTerminal], app_obj[iAppCommand], app_obj[iAppName], app_obj[iAppLevel])

    @pyqtSlot()
    def slot_checkCurrentRoom(self):
        item = self.treeWidget.currentItem()
        room_path = None

        if not item:
            return

        if item.type() in (ITEM_TYPE_STUDIO, ITEM_TYPE_STUDIO_APP):
            self.menu_Project.setEnabled(False)
            self.group_project.setEnabled(False)
            self.menu_Application.setEnabled(True)

            DBus.ladish_room = None
            DBus.ladish_app_iface = dbus.Interface(DBus.ladish_studio, "org.ladish.AppSupervisor")
            ITEM_TYPE = ITEM_TYPE_STUDIO

        elif item.type() in (ITEM_TYPE_ROOM, ITEM_TYPE_ROOM_APP):
            self.menu_Project.setEnabled(True)
            self.group_project.setEnabled(True)

            if item.type() == ITEM_TYPE_ROOM:
                room_path = item.properties[iItemPropRoomPath]
            elif item.type() == ITEM_TYPE_ROOM_APP:
                room_path = item.parent().properties[iItemPropRoomPath]
            else:
                return

            DBus.ladish_room = DBus.bus.get_object("org.ladish", room_path)
            DBus.ladish_app_iface = dbus.Interface(DBus.ladish_room, "org.ladish.AppSupervisor")
            ITEM_TYPE = ITEM_TYPE_ROOM

            project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()

            has_project = bool(len(project_properties) > 0)
            self.act_project_save.setEnabled(has_project)
            self.act_project_save_as.setEnabled(has_project)
            self.act_project_unload.setEnabled(has_project)
            self.act_project_properties.setEnabled(has_project)
            self.b_project_save.setEnabled(has_project)
            self.b_project_save_as.setEnabled(has_project)
            self.menu_Application.setEnabled(has_project)

        else:
            return

        if ITEM_TYPE != self.m_last_item_type or room_path != self.m_last_room_path:
            if ITEM_TYPE == ITEM_TYPE_STUDIO:
                object_path = DBus.ladish_studio
            elif ITEM_TYPE == ITEM_TYPE_ROOM:
                object_path = DBus.ladish_room
            else:
                return

            patchcanvas.clear()
            DBus.patchbay = dbus.Interface(object_path, 'org.jackaudio.JackPatchbay')
            DBus.ladish_graph = dbus.Interface(object_path, 'org.ladish.GraphDict')
            DBus.ladish_manager = dbus.Interface(object_path, 'org.ladish.GraphManager')
            self.init_ports()

        self.m_last_item_type = ITEM_TYPE
        self.m_last_room_path = room_path

    @pyqtSlot(QTreeWidgetItem, int)
    def slot_doubleClickedAppList(self, item, row):
        if item.type() in (ITEM_TYPE_STUDIO_APP, ITEM_TYPE_ROOM_APP):
            if item.properties[iItemPropActive]:
                DBus.ladish_app_iface.StopApp(item.properties[iItemPropNumber])
            else:
                DBus.ladish_app_iface.StartApp(item.properties[iItemPropNumber])

    @pyqtSlot()
    def slot_updateMenuStudioList_Load(self):
        self.menu_studio_load.clear()

        studio_list = DBus.ladish_control.GetStudioList()
        if len(studio_list) == 0:
            act_no_studio = QAction(self.tr("Empty studio list"), self.menu_studio_load)
            act_no_studio.setEnabled(False)
            self.menu_studio_load.addAction(act_no_studio)
        else:
            for studio in studio_list:
                studio_name  = str(studio[iStudioListName])
                act_x_studio = QAction(studio_name, self.menu_studio_load)
                self.menu_studio_load.addAction(act_x_studio)
                self.connect(act_x_studio, SIGNAL("triggered()"), SLOT("slot_studio_load_m()"))

    @pyqtSlot()
    def slot_updateMenuStudioList_Delete(self):
        self.menu_studio_delete.clear()

        studio_list = DBus.ladish_control.GetStudioList()
        if len(studio_list) == 0:
            act_no_studio = QAction(self.tr("Empty studio list"), self.menu_studio_delete)
            act_no_studio.setEnabled(False)
            self.menu_studio_delete.addAction(act_no_studio)
        else:
            for studio in studio_list:
                studio_name = str(studio[iStudioListName])
                act_x_studio = QAction(studio_name, self.menu_studio_delete)
                self.menu_studio_delete.addAction(act_x_studio)
                self.connect(act_x_studio, SIGNAL("triggered()"), SLOT("slot_studio_delete_m()"))

    @pyqtSlot()
    def slot_updateMenuRoomList(self):
        self.menu_room_delete.clear()
        if DBus.ladish_control.IsStudioLoaded():
            room_list = DBus.ladish_studio.GetRoomList()
            if len(room_list) == 0:
                self.createEmptyMenuRoomActon()
            else:
                for room_path, room_dict in room_list:
                    ladish_room = DBus.bus.get_object("org.ladish", room_path)
                    room_name = ladish_room.GetName()
                    act_x_room = QAction(room_name, self.menu_room_delete)
                    self.menu_room_delete.addAction(act_x_room)
                    self.connect(act_x_room, SIGNAL("triggered()"), SLOT("slot_room_delete_m()"))
        else:
            self.createEmptyMenuRoomActon()

    def createEmptyMenuRoomActon(self):
        act_no_room = QAction(self.tr("Empty room list"), self.menu_room_delete)
        act_no_room.setEnabled(False)
        self.menu_room_delete.addAction(act_no_room)

    @pyqtSlot()
    def slot_updateMenuProjectList(self):
        self.menu_project_load.clear()
        act_project_load = QAction(self.tr("Load from folder..."), self.menu_project_load)
        self.menu_project_load.addAction(act_project_load)
        self.connect(act_project_load, SIGNAL("triggered()"), SLOT("slot_project_load()"))

        ladish_recent_iface = dbus.Interface(DBus.ladish_room, "org.ladish.RecentItems")
        proj_list = ladish_recent_iface.get(RECENT_PROJECTS_STORE_MAX_ITEMS)

        if len(proj_list) > 0:
            self.menu_project_load.addSeparator()
            for proj_path, proj_dict in proj_list:
                if "name" in proj_dict.keys():
                    proj_name = proj_dict['name']
                else:
                    continue

                act_x_text = "%s [%s]" % (proj_name, proj_path)
                act_x_proj = QAction(act_x_text, self.menu_project_load)
                self.menu_project_load.addAction(act_x_proj)
                self.connect(act_x_proj, SIGNAL("triggered()"), SLOT("slot_project_load_m()"))

    @pyqtSlot()
    def slot_showAppListCustomMenu(self):
        item = self.treeWidget.currentItem()
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
                act_x_add_new.setEnabled(self.act_app_add_new.isEnabled())

            elif item.type() == ITEM_TYPE_ROOM:
                act_x_add_new = cMenu.addAction(self.tr("Add New..."))
                act_x_run_custom = cMenu.addAction(self.tr("Run Custom..."))
                cMenu.addSeparator()
                act_x_new = cMenu.addAction(self.tr("New Project..."))
                cMenu.addMenu(self.menu_project_load)
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

                act_x_add_new.setEnabled(self.menu_Application.isEnabled() and self.act_app_add_new.isEnabled())

                project_graph_version, project_properties = DBus.ladish_room.GetProjectProperties()

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
                    pass #self.slot_app_add_new()
                elif act_x_sel == act_x_run_custom:
                    self.slot_app_run_custom()
                elif act_x_sel == act_x_create_room:
                    self.slot_room_create()

            elif item.type() == ITEM_TYPE_ROOM:
                if act_x_sel == act_x_add_new:
                    pass # self.slot_app_add_new
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
                    room_name = DBus.ladish_room.GetName()
                    DBus.ladish_studio.DeleteRoom(room_name)

            elif item.type() in (ITEM_TYPE_STUDIO_APP, ITEM_TYPE_ROOM_APP):
                number = item.properties[iItemPropNumber]

                if act_x_sel == act_x_start:
                    DBus.ladish_app_iface.StartApp(number)
                elif act_x_sel == act_x_stop:
                    DBus.ladish_app_iface.StopApp(number)
                elif act_x_sel == act_x_kill:
                    DBus.ladish_app_iface.KillApp(number)
                elif act_x_sel == act_x_properties:
                    name, command, active, terminal, level = DBus.ladish_app_iface.GetAppProperties2(number)

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
                        DBus.ladish_app_iface.SetAppProperties2(number, app_obj[iAppName], app_obj[iAppCommand], app_obj[iAppTerminal], app_obj[iAppLevel])

                elif act_x_sel == act_x_remove:
                    DBus.ladish_app_iface.RemoveApp(number)

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        if split_mode == patchcanvas.PORT_MODE_INPUT:
            canvas_x = URI_CANVAS_X_SPLIT
            canvas_y = URI_CANVAS_Y_SPLIT
        else:
            canvas_x = URI_CANVAS_X
            canvas_y = URI_CANVAS_Y

        DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, canvas_x, str(pos.x()))
        DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, canvas_y, str(pos.y()))

        self.miniCanvasPreview.update()

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        maximum = self.graphicsView.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.miniCanvasPreview.setViewPosX(xp)

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.graphicsView.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.miniCanvasPreview.setViewPosY(yp)

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        self.graphicsView.horizontalScrollBar().setValue(self.settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH / 3, type=int))
        self.graphicsView.verticalScrollBar().setValue(self.settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT * 3 / 8, type=int))

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.graphicsView.horizontalScrollBar().setValue(xp * DEFAULT_CANVAS_WIDTH)
        self.graphicsView.verticalScrollBar().setValue(yp * DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.miniCanvasPreview.setViewSize(float(self.graphicsView.width()) / DEFAULT_CANVAS_WIDTH, float(self.graphicsView.height()) / DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_handleCrash_jack(self):
        patchcanvas.clear()
        self.DBusReconnect()
        self.studioUnloaded()

    @pyqtSlot()
    def slot_handleCrash_ladish(self):
        QMessageBox.warning(self, self.tr("Error"), self.tr("ladish daemon has crashed"))
        self.treeWidget.clear()
        patchcanvas.clear()
        self.DBusReconnect()

    @pyqtSlot()
    def slot_handleCrash_studio(self):
        QMessageBox.warning(self, self.tr("Error"), self.tr("jackdbus has crashed"))

    @pyqtSlot(str)
    def slot_DBusCrashCallback(self, appInterface):
        if appInterface == "org.jackaudio.service":
            QTimer.singleShot(0, self, SLOT("slot_handleCrash_jack()"))
        elif appInterface == "org.ladish":
            QTimer.singleShot(0, self, SLOT("slot_handleCrash_ladish()"))

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
            if DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes":
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
        if DBus.ladish_studio.IsStarted():
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
        self.treeWidget.topLevelItem(0).setText(0, new_name)

    @pyqtSlot()
    def slot_DBusStudioCrashedCallback(self):
        QTimer.singleShot(0, self, SLOT("slot_handleCrash_studio()"))

    @pyqtSlot(str, str)
    def slot_DBusRoomAppearedCallback(self, room_path, room_name):
        self.room_add(room_path, room_name)

    @pyqtSlot(str)
    def slot_DBusRoomDisappearedCallback(self, room_path):
        for i in range(self.treeWidget.topLevelItemCount()):
            item = self.treeWidget.topLevelItem(i)

            if i == 0:
                continue

            if item and item.type() == ITEM_TYPE_ROOM and item.properties[iItemPropRoomPath] == room_path:
                for j in range(item.childCount()):
                    item.takeChild(j)

                self.treeWidget.takeTopLevelItem(i)
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

        self.act_project_save.setEnabled(has_project)
        self.act_project_save_as.setEnabled(has_project)
        self.act_project_unload.setEnabled(has_project)
        self.act_project_properties.setEnabled(has_project)
        self.b_project_save.setEnabled(has_project)
        self.b_project_save_as.setEnabled(has_project)
        self.menu_Application.setEnabled(has_project)

        if path == "/org/ladish/Studio":
            top_level_item = self.treeWidget.topLevelItem(0)
            room_name = ""

        else:
            for i in range(self.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.treeWidget.topLevelItem(i)
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
            top_level_item = self.treeWidget.topLevelItem(0)
        else:
            ITEM_TYPE = ITEM_TYPE_ROOM_APP
            for i in range(self.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.treeWidget.topLevelItem(i)
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
            top_level_item = self.treeWidget.topLevelItem(0)
        else:
            for i in range(self.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.treeWidget.topLevelItem(i)
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
            top_level_item = self.treeWidget.topLevelItem(0)
        else:
            for i in range(self.treeWidget.topLevelItemCount()):
                if i == 0:
                    continue
                top_level_item = self.treeWidget.topLevelItem(i)
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
        if jack.client:
            DBus.jack.ResetXruns()

    @pyqtSlot(int)
    def slot_JackBufferSizeCallback(self, buffer_size):
        setBufferSize(self, buffer_size)

    @pyqtSlot(int)
    def slot_JackSampleRateCallback(self, sample_rate):
        setSampleRate(self, sample_rate)

    @pyqtSlot()
    def slot_JackShutdownCallback(self):
        self.jackStopped()

    @pyqtSlot()
    def slot_configureClaudia(self):
        # Save groups position now
        if DBus.patchbay:
            version, groups, conns = DBus.patchbay.GetGraph(0)

            for group in groups:
                group_id, group_name, ports = group

                group_pos_i = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_OUTPUT)
                group_pos_o = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_INPUT)

                DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X, str(group_pos_o.x()))
                DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y, str(group_pos_o.y()))
                DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X_SPLIT, str(group_pos_i.x()))
                DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y_SPLIT, str(group_pos_i.y()))

        try:
            ladish_config = DBus.bus.get_object("org.ladish.conf", "/org/ladish/conf")
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

            self.settings.setValue(LADISH_CONF_KEY_DAEMON_NOTIFY, key_notify)
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_SHELL, key_shell)
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_TERMINAL, key_terminal)
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, key_studio_autostart)
            self.settings.setValue(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, key_js_save_delay)

        dialog = SettingsW(self, "claudia", hasGL)

        if not ladish_config:
            dialog.lw_page.hideRow(2)

        if dialog.exec_():
            if ladish_config:
                ladish_config.set(LADISH_CONF_KEY_DAEMON_NOTIFY, "true" if (self.settings.value(LADISH_CONF_KEY_DAEMON_NOTIFY, LADISH_CONF_KEY_DAEMON_NOTIFY_DEFAULT, type=bool)) else "false")
                ladish_config.set(LADISH_CONF_KEY_DAEMON_SHELL, self.settings.value(LADISH_CONF_KEY_DAEMON_SHELL, LADISH_CONF_KEY_DAEMON_SHELL_DEFAULT, type=str))
                ladish_config.set(LADISH_CONF_KEY_DAEMON_TERMINAL, self.settings.value(LADISH_CONF_KEY_DAEMON_TERMINAL, LADISH_CONF_KEY_DAEMON_TERMINAL_DEFAULT, type=str))
                ladish_config.set(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, "true" if (self.settings.value(LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART, LADISH_CONF_KEY_DAEMON_STUDIO_AUTOSTART_DEFAULT, type=bool)) else "false")
                ladish_config.set(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, str(self.settings.value(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT, type=int)))

            self.loadSettings(False)
            patchcanvas.clear()

            p_options = patchcanvas.options_t()
            p_options.theme_name       = self.m_savedSettings["Canvas/Theme"]
            p_options.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
            p_options.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
            p_options.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
            p_options.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

            p_features = patchcanvas.features_t()
            p_features.group_info   = False
            p_features.group_rename = True
            p_features.port_info    = True
            p_features.port_rename  = True
            p_features.handle_group_pos = False

            patchcanvas.setOptions(p_options)
            patchcanvas.setFeatures(p_features)
            patchcanvas.init("Claudia", self.scene, self.canvasCallback, DEBUG)

            self.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())

            if DBus.ladish_control.IsStudioLoaded() and DBus.ladish_studio and DBus.ladish_studio.IsStarted():
                self.init_ports()

    @pyqtSlot()
    def slot_aboutClaudia(self):
        QMessageBox.about(self, self.tr("About Claudia"), self.tr("<h3>Claudia</h3>"
                                                                  "<br>Version %s"
                                                                  "<br>Claudia is a Graphical User Interface to LADISH.<br>"
                                                                  "<br>Copyright (C) 2010-2012 falkTX" % VERSION))

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("SplitterSizes", self.splitter.saveState())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isEnabled())
        self.settings.setValue("ShowStatusbar", self.frame_statusbar.isEnabled())
        self.settings.setValue("TransportView", self.m_selected_transport_view)
        self.settings.setValue("HorizontalScrollBarValue", self.graphicsView.horizontalScrollBar().value())
        self.settings.setValue("VerticalScrollBarValue", self.graphicsView.verticalScrollBar().value())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

            splitter_sizes = self.settings.value("SplitterSizes", "")
            if splitter_sizes:
                self.splitter.restoreState(splitter_sizes)
            else:
                self.splitter.setSizes((100, 400))

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.frame_toolbar.setVisible(show_toolbar)

            show_statusbar = self.settings.value("ShowStatusbar", True, type=bool)
            self.act_settings_show_statusbar.setChecked(show_statusbar)
            self.frame_statusbar.setVisible(show_statusbar)

            setTransportView(self, self.settings.value("TransportView", TRANSPORT_VIEW_HMS, type=int))

        self.m_savedSettings = {
            "Main/DefaultProjectFolder": self.settings.value("Main/DefaultProjectFolder", DEFAULT_PROJECT_FOLDER, type=str),
            "Main/UseSystemTray": self.settings.value("Main/UseSystemTray", True, type=bool),
            "Main/CloseToTray": self.settings.value("Main/CloseToTray", False, type=bool),
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int),
            "Canvas/Theme": self.settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": self.settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": self.settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": self.settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": self.settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": self.settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/TextAntialiasing": self.settings.value("Canvas/TextAntialiasing", True, type=bool),
            "Canvas/HighQualityAntialiasing": self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

        self.act_app_add_new.setEnabled(USE_CLAUDIA_ADD_NEW)

    def resizeEvent(self, event):
        QTimer.singleShot(0, self, SLOT("slot_miniCanvasCheckSize()"))
        QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.m_timer120:
            if jack.client:
                refreshTransport(self)
                self.refreshXruns()
        elif event.timerId() == self.m_timer600:
            if jack.client:
                refreshDSPLoad(self)
            else:
                self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        if self.systray:
            #if self.saved_settings["Main/CloseToTray"] and self.systray.isTrayAvailable() and self.isVisible():
                #self.hide()
                #self.systray.setActionText("show", QStringStr(gui.tr("Restore")))
                #event.ignore()
                #return
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

    DBus.loop = DBusQtMainLoop(set_as_default=True)
    DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)
    DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
    DBus.ladish_control = DBus.bus.get_object("org.ladish", "/org/ladish/Control")

    try:
        DBus.ladish_app_daemon = DBus.bus.get_object("org.ladish.appdb", "/")
    except:
        DBus.ladish_app_daemon = None

    jacksettings.initBus(DBus.bus)

    # Show GUI
    gui = ClaudiaMainW()

    #if (gui.systray and "--minimized" in app.arguments()):
    #gui.hide()
    #gui.systray.setActionText("show", QStringStr(gui.tr("Restore")))
    #else:
    gui.show()

    # Set-up custom signal handling
    setUpSignals(gui)

    # App-Loop
    #if gui.systray:
        #ret = gui.systray.exec_(app)
    #else:
    ret = app.exec_()

    # Close Jack
    if jack.client:
        jacklib.deactivate(jack.client)
        jacklib.client_close(jack.client)

    # Exit properly
    sys.exit(ret)

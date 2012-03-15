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
import dbus
from dbus.mainloop.qt import DBusQtMainLoop
from time import ctime
from PyQt4.QtCore import QPointF, QSettings
from PyQt4.QtGui import QApplication, QMainWindow, QTableWidgetItem, QTreeWidgetItem

# Imports (Custom Stuff)
import ui_claudia
import ui_claudia_studioname, ui_claudia_studiolist
from shared_jack import *
from shared_canvas import *
from shared_settings import *

try:
  from PyQt4.QtOpenGL import QGLWidget
  hasGL = True
except:
  hasGL = False

# internal defines
ITEM_TYPE_STUDIO     = 0
ITEM_TYPE_STUDIO_APP = 1
ITEM_TYPE_ROOM       = 2
ITEM_TYPE_ROOM_APP   = 3

# jackdbus defines
iGraphVersion    = 0
iJackClientId    = 1
iJackClientName  = 2
iJackPortId      = 3
iJackPortName    = 4
iJackPortNewName = 5
iJackPortFlags   = 5
iJackPortType    = 6

iSourceClientId   = 1
iSourceClientName = 2
iSourcePortId     = 3
iSourcePortName   = 4
iTargetClientId   = 5
iTargetClientName = 6
iTargetPortId     = 7
iTargetPortName   = 8

# ladish defines
iStudioListName = 0
iStudioListDict = 1

iStudioRenamedName = 0

DEFAULT_CANVAS_WIDTH  = 3100
DEFAULT_CANVAS_HEIGHT = 2400

GRAPH_DICT_OBJECT_TYPE_GRAPH  = 0
GRAPH_DICT_OBJECT_TYPE_CLIENT = 1
GRAPH_DICT_OBJECT_TYPE_PORT   = 2
GRAPH_DICT_OBJECT_TYPE_CONNECTION = 3

RECENT_PROJECTS_STORE_MAX_ITEMS = 50

URI_A2J_PORT       = "http://ladish.org/ns/a2j"
URI_CANVAS_WIDTH   = "http://ladish.org/ns/canvas/width"
URI_CANVAS_HEIGHT  = "http://ladish.org/ns/canvas/height"
URI_CANVAS_X       = "http://ladish.org/ns/canvas/x"
URI_CANVAS_Y       = "http://ladish.org/ns/canvas/y"
URI_CANVAS_SPLIT   = "http://kxstudio.sourceforge.net/ns/canvas/split"
URI_CANVAS_X_SPLIT = "http://kxstudio.sourceforge.net/ns/canvas/x_split"
URI_CANVAS_Y_SPLIT = "http://kxstudio.sourceforge.net/ns/canvas/y_split"

# set default project folder
DEFAULT_PROJECT_FOLDER = os.path.join(HOME, "ladish-projects")
setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)

# Studio Name Dialog
class StudioNameW(QDialog, ui_claudia_studioname.Ui_StudioNameW):

    NEW     = 1
    RENAME  = 2
    SAVE_AS = 3

    def __init__(self, parent, mode):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if (mode == self.NEW):
          self.setWindowTitle(self.tr("New studio"))
        elif (mode == self.RENAME):
          self.setWindowTitle(self.tr("Rename studio"))
        elif (mode == self.SAVE_AS):
          self.setWindowTitle(self.tr("Save studio as"))

        self.m_mode = mode
        self.m_studio_list = []

        if (mode == self.RENAME and bool(DBus.ladish_control.IsStudioLoaded())):
          current_name = str(DBus.ladish_studio.GetName())
          self.m_studio_list.append(current_name)
          self.le_name.setText(current_name)

        studio_list = DBus.ladish_control.GetStudioList()
        for studio in studio_list:
          self.m_studio_list.append(str(studio[iStudioListName]))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_name, SIGNAL("textChanged(QString)"),  SLOT("slot_checkText(QString)"))

        self.ret_studio_name = ""

    @pyqtSlot(str)
    def slot_checkText(self, text):
        if (self.m_mode == self.SAVE_AS):
          check = bool(text)
        else:
          check = bool(text and text not in self.m_studio_list)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_studio_name = self.le_name.text()

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

        if (self.tableWidget.rowCount() > 0):
          self.tableWidget.setCurrentCell(0, 0)

        self.ret_studio_name = ""

    @pyqtSlot(int)
    def slot_checkSelection(self, row):
        check = bool(row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if (self.tableWidget.rowCount() >= 0):
          self.ret_studio_name = self.tableWidget.item(self.tableWidget.currentRow(), 0).text()

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

        #self.systray = None

        #self.m_group_list = []
        #self.m_group_split_list = []
        #self.m_connection_list = []
        #self.m_last_connection_id = 1

        self.m_xruns = -1
        self.m_buffer_size = 0
        self.m_sample_rate = 0
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        #self.m_last_item_type = None
        #self.m_last_room_path = None

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
        if (self.m_savedSettings["Canvas/UseOpenGL"] and hasGL):
          self.graphicsView.setViewport(QGLWidget(self.graphicsView))
          self.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.m_savedSettings["Canvas/HighQualityAntialiasing"])

        p_options = patchcanvas.options_t()
        p_options.theme_name       = self.m_savedSettings["Canvas/Theme"]
        p_options.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
        p_options.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
        p_options.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
        p_options.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

        p_features = patchcanvas.features_t()
        p_features.group_info       = False
        p_features.group_rename     = False
        p_features.port_info        = True
        p_features.port_rename      = False
        p_features.handle_group_pos = False

        patchcanvas.setOptions(p_options)
        patchcanvas.setFeatures(p_features)
        patchcanvas.init(self.scene, self.canvasCallback, DEBUG)

        patchcanvas.setInitialPos(DEFAULT_CANVAS_WIDTH/2, DEFAULT_CANVAS_HEIGHT/2)
        patchcanvas.setCanvasSize(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        self.graphicsView.setSceneRect(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        self.miniCanvasPreview.setRealParent(self)
        self.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        if (DBus.jack.IsStarted()):
          self.jackStarted()
        else:
          self.jackStopped()

        if (DBus.ladish_control.IsStudioLoaded()):
          self.studioLoaded()
          if (DBus.ladish_studio.IsStarted()):
            self.studioStarted()
            self.init_ports()
          else:
            self.studioStopped()
        else:
          self.studioUnloaded()

        self.m_timer120 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])
        self.m_timer600 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"]*5)

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

        #self.connect(self.act_room_create, SIGNAL("triggered()"), self.func_room_create)

        #self.connect(self.act_project_new, SIGNAL("triggered()"), self.func_project_new)
        #self.connect(self.act_project_save, SIGNAL("triggered()"), self.func_project_save)
        #self.connect(self.act_project_save_as, SIGNAL("triggered()"), self.func_project_save_as)
        #self.connect(self.act_project_unload, SIGNAL("triggered()"), self.func_project_unload)
        #self.connect(self.act_project_properties, SIGNAL("triggered()"), self.func_project_properties)
        #self.connect(self.b_project_new, SIGNAL("clicked()"), self.func_project_new)
        #self.connect(self.b_project_load, SIGNAL("clicked()"), self.func_project_load)
        #self.connect(self.b_project_save, SIGNAL("clicked()"), self.func_project_save)
        #self.connect(self.b_project_save_as, SIGNAL("clicked()"), self.func_project_save_as)

        #self.connect(self.act_app_add_new, SIGNAL("triggered()"), self.func_app_add_new)
        #self.connect(self.act_app_run_custom, SIGNAL("triggered()"), self.func_app_run_custom)

        #self.connect(self.menu_studio_load, SIGNAL("aboutToShow()"), lambda menu=self.menu_studio_load,
                     #function=self.func_studio_load_m: self.updateMenuStudioList(menu, function))
        #self.connect(self.menu_studio_delete, SIGNAL("aboutToShow()"), lambda menu=self.menu_studio_delete,
                     #function=self.func_studio_delete_m: self.updateMenuStudioList(menu, function))
        #self.connect(self.menu_room_delete, SIGNAL("aboutToShow()"), self.updateMenuRoomList)
        #self.connect(self.menu_project_load, SIGNAL("aboutToShow()"), self.updateMenuProjectList)

        #self.connect(self.treeWidget, SIGNAL("itemSelectionChanged()"), self.checkCurrentRoom)
        ##self.connect(self.treeWidget, SIGNAL("itemPressed(QTreeWidgetItem*, int)"), self.checkCurrentRoom)
        #self.connect(self.treeWidget, SIGNAL("itemDoubleClicked(QTreeWidgetItem*, int)"), self.doubleClickedAppList)
        #self.connect(self.treeWidget, SIGNAL("customContextMenuRequested(QPoint)"), self.showAppListCustomMenu)

        self.connect(self.miniCanvasPreview, SIGNAL("miniCanvasMoved(double, double)"), SLOT("slot_miniCanvasMoved(double, double)"))

        self.connect(self.graphicsView.horizontalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_horizontalScrollBarChanged(int)"))
        self.connect(self.graphicsView.verticalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_verticalScrollBarChanged(int)"))

        self.connect(self.scene, SIGNAL("sceneGroupMoved(int, int, QPointF)"), SLOT("slot_canvasItemMoved(int, int, QPointF)"))
        self.connect(self.scene, SIGNAL("scaleChanged(double)"), SLOT("slot_canvasScaleChanged(double)"))

        self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureClaudia()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutClaudia()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        # org.jackaudio.JackControl
        self.connect(self, SIGNAL("DBusServerStartedCallback()"), SLOT("slot_DBusServerStartedCallback()"))
        self.connect(self, SIGNAL("DBusServerStoppedCallback()"), SLOT("slot_DBusServerStoppedCallback()"))

        # org.jackaudio.JackPatchbay
        self.connect(self, SIGNAL("DBusClientAppearedCallback(int, QString)"), SLOT("slot_DBusClientAppearedCallback(int, QString)"))
        self.connect(self, SIGNAL("DBusClientDisappearedCallback(int)"), SLOT("slot_DBusClientDisappearedCallback(int)"))
        self.connect(self, SIGNAL("DBusPortAppearedCallback(int, int, QString, int, int)"), SLOT("slot_DBusPortAppearedCallback(int, int, QString, int, int)"))
        self.connect(self, SIGNAL("DBusPortDisppearedCallback(int)"), SLOT("slot_DBusPortDisppearedCallback(int)"))
        self.connect(self, SIGNAL("DBusPortRenamedCallback(int, QString)"), SLOT("slot_DBusPortRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("DBusPortsConnectedCallback(int, int)"), SLOT("slot_DBusPortsConnectedCallback(int, int)"))
        self.connect(self, SIGNAL("DBusPortsDisconnectedCallback(int, int)"), SLOT("slot_DBusPortsDisconnectedCallback(int, int)"))

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

        #self.connect(self, SIGNAL("DBus()"), SLOT("slot_DBus()"))

        # JACK
        self.connect(self, SIGNAL("JackBufferSizeCallback(int)"), SLOT("slot_JackBufferSizeCallback(int)"))
        self.connect(self, SIGNAL("JackSampleRateCallback(int)"), SLOT("slot_JackSampleRateCallback(int)"))
        self.connect(self, SIGNAL("JackShutdownCallback()"), SLOT("slot_JackShutdownCallback()"))

        # DBus Stuff
        DBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword='dest', path_keyword='path',
                        member_keyword='member', interface_keyword='interface', sender_keyword='sender', )

        QTimer.singleShot(100, self, SLOT("slot_miniCanvasInit()"))

    def canvasCallback(self, action, value1, value2, value_str):
        if (action == patchcanvas.ACTION_GROUP_INFO):
          pass

        elif (action == patchcanvas.ACTION_GROUP_RENAME):
          pass

        elif (action == patchcanvas.ACTION_GROUP_SPLIT):
          group_id = value1
          DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "true")

          patchcanvas.splitGroup(group_id)
          self.miniCanvasPreview.update()

        elif (action == patchcanvas.ACTION_GROUP_JOIN):
          group_id = value1
          DBus.ladish_graph.Set(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT, "false")

          patchcanvas.joinGroup(group_id)
          self.miniCanvasPreview.update()

        #elif (action == patchcanvas.ACTION_PORT_INFO):
          #this_port_id = value1

          #graph_dump = DBus.patchbay.GetGraph(0)
          #graph_version = graph_dump[0]
          #graph_ports = graph_dump[1]
          #graph_conns = graph_dump[2]

          ## Graph Ports
          #for i in range(len(graph_ports)):
            #group_id    = graph_ports[i][0]
            #group_name  = graph_ports[i][1]
            #group_ports = graph_ports[i][2]

            #for j in range(len(group_ports)):
              #port_id    = group_ports[j][0]
              #port_name  = group_ports[j][1]
              #port_flags = group_ports[j][2]
              #port_type  = group_ports[j][3]
              #if (this_port_id == port_id):
                #break

            #if (this_port_id == port_id):
              #break

          #if (DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, this_port_id, URI_A2J_PORT) == "yes"):
            #port_flags_ = self.get_real_a2j_port_flags(group_name, port_name, jacklib.PortIsInput if (port_flags & jacklib.PortIsInput) else jacklib.PortIsOutput)
            #if (port_flags_ != 0):
              #port_flags = port_flags_

          #flags = []
          #if (port_flags & jacklib.PortIsInput):
            #flags.append(self.tr("Input"))
          #if (port_flags & jacklib.PortIsOutput):
            #flags.append(self.tr("Output"))
          #if (port_flags & jacklib.PortIsPhysical):
            #flags.append(self.tr("Physical"))
          #if (port_flags & jacklib.PortCanMonitor):
            #flags.append(self.tr("Can Monitor"))
          #if (port_flags & jacklib.PortIsTerminal):
            #flags.append(self.tr("Terminal"))

          #flags_text = ""
          #for i in range(len(flags)):
            #if (flags_text): flags_text+= " | "
            #flags_text += flags[i]

          #if (port_type == jacklib.AUDIO):
            #type_text = self.tr("Audio")
          #elif (port_type == jacklib.MIDI):
            #type_text = self.tr("MIDI")
          #else:
            #type_text = self.tr("Unknown")

          #port_full_name = group_name+":"+port_name

          #info = self.tr(""
                  #"<table>"
                  #"<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%1</td></tr>"
                  #"<tr><td align='right'><b>Group ID:</b></td><td>&nbsp;%2</td></tr>"
                  #"<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%3</td></tr>"
                  #"<tr><td align='right'><b>Port ID:</b></td><td>&nbsp;%4</i></td></tr>"
                  #"<tr><td align='right'><b>Full Port Name:</b></td><td>&nbsp;%5</td></tr>"
                  #"<tr><td colspan='2'>&nbsp;</td></tr>"
                  #"<tr><td align='right'><b>Port Flags:</b></td><td>&nbsp;%6</td></tr>"
                  #"<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%7</td></tr>"
                  #"</table>"
                  #).arg(group_name).arg(group_id).arg(port_name).arg(port_id).arg(port_full_name).arg(flags_text).arg(type_text)

          #QMessageBox.information(self, self.tr("Port Information"), info)

        #elif (action == patchcanvas.ACTION_PORT_RENAME):
          #pass

        #elif (action == patchcanvas.ACTION_PORTS_CONNECT):
          #port_a = value1
          #port_b = value2
          #DBus.patchbay.ConnectPortsByID(port_a, port_b)

        #elif (action == patchcanvas.ACTION_PORTS_DISCONNECT):
          #connection_id = value1

          #for i in range(len(self.connection_list)):
            #if (connection_id == self.connection_list[i][0]):
              #port_a = self.connection_list[i][1]
              #port_b = self.connection_list[i][2]
              #break
          #else:
            #return

          #DBus.patchbay.DisconnectPortsByID(port_a, port_b)

    def init_jack(self):
        self.m_xruns = -1
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        buffer_size = int(jacklib.get_buffer_size(jack.client))
        sample_rate = int(jacklib.get_sample_rate(jack.client))
        realtime    = bool(int(jacklib.is_realtime(jack.client)))

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
        #self.m_last_item_type = ITEM_TYPE_STUDIO
        #self.m_last_room_path = None

        self.init_apps()

    def init_apps(self):
        pass

    def init_ports(self):
        if (not jack.client or not DBus.patchbay):
          return

        #self.last_connection_id = 1
        #self.group_list = []
        #self.group_split_list = []
        #self.connection_list = []

        version, groups, conns = DBus.patchbay.GetGraph(0)

        # Graph Ports
        for group in groups:
          group_id, group_name, ports = group
          self.canvas_add_group(int(group_id), str(group_name))

          for port in ports:
            port_id, port_name, port_flags, port_type_jack = port

            if (port_flags & jacklib.JackPortIsInput):
              port_mode = patchcanvas.PORT_MODE_INPUT
            elif (port_flags & jacklib.JackPortIsOutput):
              port_mode = patchcanvas.PORT_MODE_OUTPUT
            else:
              port_mode = patchcanvas.PORT_MODE_NULL

            if (port_type_jack == 0):   # Audio
              port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
            elif (port_type_jack == 1): # MIDI
              if (DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes"):
                port_type = patchcanvas.PORT_TYPE_MIDI_A2J
              else:
                port_type = patchcanvas.PORT_TYPE_MIDI_JACK
            else:
              port_type = patchcanvas.PORT_TYPE_NULL

            self.canvas_add_port(int(group_id), int(port_id), str(port_name), port_mode, port_type)

        ## Graph Connections
        #for i in range(len(graph_conns)):
          #conn_source_id   = int(graph_conns[i][0])
          #conn_source_name = QStringStr(graph_conns[i][1])
          #conn_source_port_id   = int(graph_conns[i][2])
          #conn_source_port_name = QStringStr(graph_conns[i][3])
          #conn_target_id   = int(graph_conns[i][4])
          #conn_target_name = QStringStr(graph_conns[i][5])
          #conn_target_port_id   = int(graph_conns[i][6])
          #conn_target_port_name = QStringStr(graph_conns[i][7])

          #self.canvas_connect_ports(conn_source_port_id, conn_target_port_id)

        ## Recheck all group positions
        #for i in range(len(self.group_list)):
          #group_id = self.group_list[i][0]

          #x  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X)
          #y  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y)
          #x2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X_SPLIT)
          #y2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y_SPLIT)

          #if (x != None and y != None):
            #if (x2 == None): x2 = float(x)+50
            #if (y2 == None): y2 = float(y)+50
            #patchcanvas.setGroupPos(group_id, float(x), float(y), float(x2), float(y2))

        #QTimer.singleShot(1000 if (self.saved_settings['Canvas/FancyEyeCandy']) else 0, self.miniCanvasPreview.update)

    def canvas_add_group(self, group_id, group_name):
        room_list_names = [] #self.get_room_list_names()

        if (group_name in ("Hardware Playback", "Hardware Capture")):
          icon  = patchcanvas.ICON_HARDWARE
          split = patchcanvas.SPLIT_NO
        elif (group_name in room_list_names or group_name in ("Capture", "Playback")):
          icon  = patchcanvas.ICON_LADISH_ROOM
          split = patchcanvas.SPLIT_NO
        else:
          icon  = patchcanvas.ICON_APPLICATION

          split_try = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_SPLIT)
          if (split_try == "true"):
            split = patchcanvas.SPLIT_YES
          elif (split_try == "false"):
            split = patchcanvas.SPLIT_NO
          else:
            split = patchcanvas.SPLIT_UNDEF

        patchcanvas.addGroup(group_id, group_name, split, icon)
        #self.group_list.append([group_id, group_name])

        x  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X)
        y  = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y)
        x2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_X_SPLIT)
        y2 = DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_CLIENT, group_id, URI_CANVAS_Y_SPLIT)

        if (x != None and y != None):
          if (x2 == None): x2 = "%f" % (float(x)+50)
          if (y2 == None): y2 = "%f" % (float(y)+50)
          patchcanvas.setGroupPos(group_id, float(x), float(y), float(x2), float(y2))

        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

    def canvas_add_port(self, group_id, port_id, port_name, port_mode, port_type): # , group_id, group_name, split, do_pos=True
        #for i in range(len(self.group_list)):
          #if (group_id == self.group_list[i][0]):
            #break
        #else: #Hack for clients started with no ports
          #self.canvas_add_group(group_id, group_name, do_pos)

        #if (DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes"):
          #port_type = patchcanvas.PORT_TYPE_MIDI_A2J

          #flags = self.get_real_a2j_port_flags(group_name, port_name, port_mode)
          #if (flags & jacklib.PortIsPhysical):
            #split = True

        patchcanvas.addPort(group_id, port_id, port_name, port_mode, port_type)

        QTimer.singleShot(0, self.miniCanvasPreview, SLOT("update()"))

        #if not group_id in self.group_split_list:
          #if (split):
            #patchcanvas.splitGroup(group_id)
            #patchcanvas.setGroupIcon(group_id, patchcanvas.ICON_HARDWARE)
          #elif ( group_name == "Hardware Capture" or
                 #group_name == "Hardware Playback" or
                 #group_name == "Capture" or
                 #group_name == "Playback"
               #):
            #patchcanvas.setGroupIcon(group_id, patchcanvas.ICON_HARDWARE)
          #self.group_split_list.append(group_id)

    def jackStarted(self):
        #self.DBusReconnect()

        if (not jack.client):
          jack.client = jacklib.client_open("claudia", jacklib.JackNoStartServer, None)
          if (not jack.client):
            return self.jackStopped()

        self.act_jack_render.setEnabled(canRender)
        self.b_jack_render.setEnabled(canRender)
        self.menu_Transport.setEnabled(True)
        self.group_transport.setEnabled(True)

        self.cb_buffer_size.setEnabled(True)
        self.cb_sample_rate.setEnabled(True) # jacksettings.getSampleRate() != -1

        #if (self.systray):
          #self.systray.setActionEnabled("tools_render", canRender)

        self.pb_dsp_load.setMaximum(100)
        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.update()

        self.init_jack()

    def jackStopped(self):
        #self.DBusReconnect()

        # client already closed
        jack.client = None

        if (self.m_next_sample_rate):
          jack_sample_rate(self, self.m_next_sample_rate)

        buffer_size = jacksettings.getBufferSize()
        sample_rate = jacksettings.getSampleRate()
        buffer_size_test = bool(buffer_size != -1)
        sample_rate_test = bool(sample_rate != -1)

        if (buffer_size_test):
          setBufferSize(self, buffer_size)

        if (sample_rate_test):
          setSampleRate(self, sample_rate)

        setRealTime(self, jacksettings.isRealtime())
        setXruns(self, -1)

        self.cb_buffer_size.setEnabled(buffer_size_test)
        self.cb_sample_rate.setEnabled(sample_rate_test)

        self.act_jack_render.setEnabled(False)
        self.b_jack_render.setEnabled(False)
        self.menu_Transport.setEnabled(False)
        self.group_transport.setEnabled(False)

        #if (self.systray):
          #self.systray.setActionEnabled("tools_render", False)

        if (self.m_selected_transport_view == TRANSPORT_VIEW_HMS):
          self.label_time.setText("00:00:00")
        elif (self.m_selected_transport_view == TRANSPORT_VIEW_BBT):
          self.label_time.setText("000|0|0000")
        elif (self.m_selected_transport_view == TRANSPORT_VIEW_FRAMES):
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

        #if (self.systray):
          #self.systray.setActionEnabled("studio_start", False)
          #self.systray.setActionEnabled("studio_stop", True)
          #self.systray.setActionEnabled("studio_save", True)
          #self.systray.setActionEnabled("studio_save_as", True)

    def studioStopped(self):
        self.act_studio_start.setEnabled(True)
        self.act_studio_stop.setEnabled(False)
        self.act_studio_save.setEnabled(False)
        self.act_studio_save_as.setEnabled(False)

        self.b_studio_save.setEnabled(False)
        self.b_studio_save_as.setEnabled(False)

        #if (self.systray):
          #self.systray.setActionEnabled("studio_start", True)
          #self.systray.setActionEnabled("studio_stop", False)
          #self.systray.setActionEnabled("studio_save", False)
          #self.systray.setActionEnabled("studio_save_as", False)

    def studioLoaded(self):
        DBus.ladish_studio = DBus.bus.get_object("org.ladish", "/org/ladish/Studio")
        DBus.ladish_graph = dbus.Interface(DBus.ladish_studio, 'org.ladish.GraphDict')
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

        #if (self.systray):
          #self.systray.setActionEnabled("studio_rename", True)
          #self.systray.setActionEnabled("studio_unload", True)

        self.init_studio()

    def studioUnloaded(self):
        DBus.patchbay = None
        DBus.ladish_studio = None
        DBus.ladish_graph = None
        DBus.ladish_app_iface = None

        #self.last_item_type = None
        #self.last_room_path = None

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

        #if (self.systray):
          #self.systray.setActionEnabled("studio_start", False)
          #self.systray.setActionEnabled("studio_stop", False)
          #self.systray.setActionEnabled("studio_rename", False)
          #self.systray.setActionEnabled("studio_save", False)
          #self.systray.setActionEnabled("studio_save_as", False)
          #self.systray.setActionEnabled("studio_unload", False)

        self.treeWidget.clear()

        patchcanvas.clear()

    def DBusSignalReceiver(self, *args, **kwds):
        if (kwds['interface'] == "org.jackaudio.JackControl"):
          if (DEBUG): print("DBus signal @org.jackaudio.JackControl,", kwds['member'])
          if (kwds['member'] == "ServerStarted"):
            self.emit(SIGNAL("DBusServerStartedCallback()"))
          elif (kwds['member'] == "ServerStopped"):
            self.emit(SIGNAL("DBusServerStoppedCallback()"))

        elif (kwds['interface'] == "org.jackaudio.JackPatchbay"):
          if (kwds['path'] == DBus.patchbay.object_path):
            if (DEBUG): print("DBus signal @org.jackaudio.JackPatchbay,", kwds['member'])
            if (kwds['member'] == "ClientAppeared"):
              self.emit(SIGNAL("DBusClientAppearedCallback(int, QString)"), args[iJackClientId], args[iJackClientName])
            elif (kwds['member'] == "ClientDisappeared"):
              self.emit(SIGNAL("DBusClientDisappearedCallback(int)"), args[iJackClientId])
            elif (kwds['member'] == "PortAppeared"):
              self.emit(SIGNAL("DBusPortAppearedCallback(int, int, QString, int, int)"), args[iJackClientId], args[iJackPortId], args[iJackPortName], args[iJackPortFlags], args[iJackPortType])
            elif (kwds['member'] == "PortDisappeared"):
              self.emit(SIGNAL("DBusPortDisppearedCallback(int)"), args[iJackPortId])
            elif (kwds['member'] == "PortRenamed"):
              self.emit(SIGNAL("DBusPortRenamedCallback(int, QString)"), args[iJackPortId], args[iJackPortNewName])
            elif (kwds['member'] == "PortsConnected"):
              self.emit(SIGNAL("DBusPortsConnectedCallback(int, int)"), args[iSourcePortId], args[iTargetPortId])
            elif (kwds['member'] == "PortsDisconnected"):
              self.emit(SIGNAL("DBusPortsDisconnectedCallback(int, int)"), args[iSourcePortId], args[iTargetPortId])

        elif (kwds['interface'] == "org.ladish.Control"):
          if (DEBUG): print("DBus signal @org.ladish.Control,", kwds['member'])
          if (kwds['member'] == "StudioAppeared"):
            self.emit(SIGNAL("DBusStudioAppearedCallback()"))
          elif (kwds['member'] == "StudioDisappeared"):
            self.emit(SIGNAL("DBusStudioDisappearedCallback()"))
          elif (kwds['member'] == "QueueExecutionHalted"):
            self.emit(SIGNAL("DBusQueueExecutionHaltedCallback()"))
          elif (kwds['member'] == "CleanExit"):
            self.emit(SIGNAL("DBusCleanExitCallback()"))

        elif (kwds['interface'] == "org.ladish.Studio"):
          if (DEBUG): print("DBus signal @org.ladish.Studio,", kwds['member'])
          if (kwds['member'] == "StudioStarted"):
            self.emit(SIGNAL("DBusStudioStartedCallback()"))
          elif (kwds['member'] == "StudioStopped"):
            self.emit(SIGNAL("DBusStudioStoppedCallback()"))
          elif (kwds['member'] == "StudioRenamed"):
            self.emit(SIGNAL("DBusStudioRenamedCallback(QString)"), args[iStudioRenamedName])
          elif (kwds['member'] == "StudioCrashed"):
            self.emit(SIGNAL("DBusStudioCrashedCallback()"))
          #elif (kwds['member'] == "RoomAppeared"):
            #self.signal_RoomAppeared(args)
          #elif (kwds['member'] == "RoomDisappeared"):
            #self.signal_RoomDisappeared(args)
          #elif (kwds['member'] == "RoomChanged"):
            #self.signal_RoomChanged(args)

        elif (kwds['interface'] == "org.ladish.Room"):
          if (DEBUG): print("DBus signal @org.ladish.Room,", kwds['member'])
          #if (kwds['member'] == "ProjectPropertiesChanged"):
            #self.signal_ProjectPropertiesChanged(kwds['path'], args)

        elif (kwds['interface'] == "org.ladish.AppSupervisor"):
          if (DEBUG): print("DBus signal @org.ladish.AppSupervisor,", kwds['member'])
          #if (kwds['member'] == "AppAdded"):
            #self.signal_AppAdded(kwds['path'], args)
          #elif (kwds['member'] == "AppRemoved"):
            #self.signal_AppRemoved(kwds['path'], args)
          #elif (kwds['member'] == "AppStateChanged"):
            #self.signal_AppStateChanged(kwds['path'], args)

    #def DBusReconnect(self):
        #DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)
        #DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
        #DBus.ladish_control = DBus.bus.get_object("org.ladish", "/org/ladish/Control")

    def refreshXruns(self):
        xruns = int(DBus.jack.GetXruns())
        if (self.m_xruns != xruns):
          setXruns(self, xruns)
          self.m_xruns = xruns

    def JackBufferSizeCallback(self, buffer_size, arg):
        if (DEBUG): print("JackBufferSizeCallback(%i)" % (buffer_size))
        self.emit(SIGNAL("JackBufferSizeCallback(int)"), buffer_size)
        return 0

    def JackSampleRateCallback(self, sample_rate, arg):
        if (DEBUG): print("JackSampleRateCallback(%i)" % (sample_rate))
        self.emit(SIGNAL("JackSampleRateCallback(int)"), sample_rate)
        return 0

    def JackShutdownCallback(self, arg):
        if (DEBUG): print("JackShutdownCallback()")
        self.emit(SIGNAL("JackShutdownCallback()"))
        return 0

    @pyqtSlot()
    def slot_studio_new(self):
        dialog = StudioNameW(self, StudioNameW.NEW)
        if (dialog.exec_()):
          DBus.ladish_control.NewStudio(dbus.String(dialog.ret_studio_name))

    #def func_studio_load_m(self, studio_name):
        #DBus.ladish_control.LoadStudio(studio_name)

    @pyqtSlot()
    def slot_studio_load_b(self):
        dialog = StudioListW(self)
        if (dialog.exec_()):
          DBus.ladish_control.LoadStudio(dbus.String(dialog.ret_studio_name))

    @pyqtSlot()
    def slot_studio_start(self):
        DBus.ladish_studio.Start()

    @pyqtSlot()
    def slot_studio_stop(self):
        DBus.ladish_studio.Stop()

    @pyqtSlot()
    def slot_studio_rename(self):
        dialog = StudioNameW(self, StudioNameW.RENAME)
        if (dialog.exec_()):
          DBus.ladish_studio.Rename(dbus.String(dialog.ret_studio_name))

    @pyqtSlot()
    def slot_studio_save(self):
        DBus.ladish_studio.Save()

    @pyqtSlot()
    def slot_studio_save_as(self):
        dialog = StudioNameW(self, StudioNameW.SAVE_AS)
        if (dialog.exec_()):
          DBus.ladish_studio.SaveAs(dbus.String(dialog.ret_studio_name))

    @pyqtSlot()
    def slot_studio_unload(self):
        DBus.ladish_studio.Unload()

    #def func_studio_delete_m(self, studio_name):
        #DBus.ladish_control.DeleteStudio(dbus.String(studio_name))

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        print(group_id, split_mode, pos)
        if (split_mode == patchcanvas.PORT_MODE_INPUT):
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
        if (maximum == 0):
          xp = 0
        else:
          xp = float(value)/maximum
        self.miniCanvasPreview.setViewPosX(xp)

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.graphicsView.verticalScrollBar().maximum()
        if (maximum == 0):
          yp = 0
        else:
          yp = float(value)/maximum
        self.miniCanvasPreview.setViewPosY(yp)

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        self.graphicsView.horizontalScrollBar().setValue(self.settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH/3, type=int))
        self.graphicsView.verticalScrollBar().setValue(self.settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT*3/8, type=int))

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.graphicsView.horizontalScrollBar().setValue(xp*DEFAULT_CANVAS_WIDTH)
        self.graphicsView.verticalScrollBar().setValue(yp*DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.miniCanvasPreview.setViewSize(float(self.graphicsView.width())/DEFAULT_CANVAS_WIDTH, float(self.graphicsView.height())/DEFAULT_CANVAS_HEIGHT)

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
    def slot_DBusClientDisappearedCallback(self):
        pass

    @pyqtSlot(int, int, str, int, int)
    def slot_DBusPortAppearedCallback(self, group_id, port_id, port_name, port_flags, port_type_jack):
        if (port_flags & jacklib.JackPortIsInput):
          port_mode = patchcanvas.PORT_MODE_INPUT
        elif (port_flags & jacklib.JackPortIsOutput):
          port_mode = patchcanvas.PORT_MODE_OUTPUT
        else:
          port_mode = patchcanvas.PORT_MODE_NULL

        if (port_type_jack == 0):   # Audio
          port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif (port_type_jack == 1): # MIDI
          if (DBus.ladish_graph.Get(GRAPH_DICT_OBJECT_TYPE_PORT, port_id, URI_A2J_PORT) == "yes"):
            port_type = patchcanvas.PORT_TYPE_MIDI_A2J
          else:
            port_type = patchcanvas.PORT_TYPE_MIDI_JACK
        else:
          port_type = patchcanvas.PORT_TYPE_NULL

        self.canvas_add_port(group_id, port_id, port_name, port_mode, port_type)

    @pyqtSlot(int)
    def slot_DBusPortDisppearedCallback(self, port_id):
        pass

    @pyqtSlot(int, str)
    def slot_DBusPortRenamedCallback(self, port_id, new_port_name):
        pass

    @pyqtSlot(int, int)
    def slot_DBusPortsConnectedCallback(self, source_port_id, target_port_id):
        pass

    @pyqtSlot(int, int)
    def slot_DBusPortsDisconnectedCallback(self, source_port_id, target_port_id):
        pass

    @pyqtSlot()
    def slot_DBusStudioAppearedCallback(self):
        self.studioLoaded()
        if (DBus.ladish_studio.IsStarted()):
          self.studioStarted()
        else:
          self.studioStopped()

    @pyqtSlot()
    def slot_DBusStudioDisappearedCallback(self):
        self.studioUnloaded()

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
        pass # TODO

    @pyqtSlot()
    def slot_DBusQueueExecutionHaltedCallback(self):
        log_path = os.path.join(HOME, ".log", "ladish", "ladish.log")
        if (os.path.exists(log_path)):
          log_file = open(log_path)
          log_text = logs.fixLogText(log_file.read().split("ERROR: ")[-1].split("\n")[0])
          log_file.close()
        else:
          log_text = None

        msgbox = QMessageBox(QMessageBox.Critical, self.tr("Execution Halted"),
                    self.tr("Something went wrong with ladish so the last action was not sucessful.\n"), QMessageBox.Ok, self)

        if (log_text):
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

    #@pyqtSlot()
    #def slot_DBus(self):
        

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if (jack.client):
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
        try:
          ladish_config = DBus.bus.get_object("org.ladish.conf", "/org/ladish/conf")
        except:
          ladish_config = None

        if (ladish_config):
          try:
            key_notify = bool(ladish_config.get(LADISH_CONF_KEY_DAEMON_NOTIFY)[0] == "true")
            print(ladish_config.get(LADISH_CONF_KEY_DAEMON_NOTIFY))
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

        if (ladish_config == None):
          dialog.lw_page.hideRow(2)

        if (dialog.exec_()):
          if (ladish_config):
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
          p_features.group_info       = False
          p_features.group_rename     = False
          p_features.port_info        = True
          p_features.port_rename      = False
          p_features.handle_group_pos = False

          patchcanvas.setOptions(p_options)
          patchcanvas.setFeatures(p_features)
          patchcanvas.init(self.scene, self.canvasCallback, DEBUG)

          if (DBus.ladish_control.IsStudioLoaded() and DBus.ladish_studio and DBus.ladish_studio.IsStarted()):
            self.init_ports()

    @pyqtSlot()
    def slot_aboutClaudia(self):
        QMessageBox.about(self, self.tr("About Claudia"), self.tr("<h3>Claudia</h3>"
            "<br>Version %s"
            "<br>Claudia is a Graphical User Interface to LADISH.<br>"
            "<br>Copyright (C) 2010-2012 falkTX" % (VERSION)))

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("SplitterSizes", self.splitter.saveState())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isVisible())
        self.settings.setValue("ShowStatusbar", self.frame_statusbar.isVisible())
        self.settings.setValue("TransportView", self.m_selected_transport_view)
        self.settings.setValue("HorizontalScrollBarValue", self.graphicsView.horizontalScrollBar().value())
        self.settings.setValue("VerticalScrollBarValue", self.graphicsView.verticalScrollBar().value())

    def loadSettings(self, geometry):
        if (geometry):
          self.restoreGeometry(self.settings.value("Geometry", ""))

          splitter_sizes = self.settings.value("SplitterSizes", "")
          if (splitter_sizes):
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
          "Canvas/HighQualityAntialiasing": self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool),
          "Apps/Database": self.settings.value("Apps/Database", "LADISH", type=str)
        }

        self.act_app_add_new.setEnabled(bool(self.m_savedSettings['Apps/Database'] == "LADISH" and DBus.ladish_app_daemon))

    def resizeEvent(self, event):
        QTimer.singleShot(0, self, SLOT("slot_miniCanvasCheckSize()"))
        QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if (event.timerId() == self.m_timer120):
          if (jack.client):
            refreshTransport(self)
            self.refreshXruns()
        elif (event.timerId() == self.m_timer600):
          if (jack.client):
            refreshDSPLoad(self)
          else:
            self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        #if (self.systray):
          #if (self.saved_settings["Main/CloseToTray"] and self.systray.isTrayAvailable() and self.isVisible()):
            #self.hide()
            #self.systray.setActionText("show", QStringStr(gui.tr("Restore")))
            #event.ignore()
            #return
          #self.systray.close()
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

    DBus.loop = DBusQtMainLoop(set_as_default=True)
    DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)
    DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
    DBus.ladish_control = DBus.bus.get_object("org.ladish", "/org/ladish/Control")

    try:
      DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
    except:
      DBus.a2j = None

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
    set_up_signals(gui)

    # App-Loop
    #if (gui.systray):
      #ret = gui.systray.exec_(app)
    #else:
    ret = app.exec_()

    # Close Jack
    if (jack.client):
      jacklib.deactivate(jack.client)
      jacklib.client_close(jack.client)

    # Exit properly
    sys.exit(ret)

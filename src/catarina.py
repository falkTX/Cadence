#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchCanvas test application
# Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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
from PyQt4.QtCore import pyqtSlot, Qt, QSettings
from PyQt4.QtGui import QApplication, QDialog, QDialogButtonBox, QPainter, QMainWindow

# Imports (Custom Stuff)
import patchcanvas
import ui_catarina, icons_rc
import ui_catarina_addgroup, ui_catarina_removegroup, ui_catarina_renamegroup
import ui_catarina_addport, ui_catarina_removeport, ui_catarina_renameport
import ui_catarina_connectports, ui_catarina_disconnectports
from shared import *

try:
  from PyQt4.QtOpenGL import QGLWidget
  hasGL = True
except:
  hasGL = False

iGroupId     = 0
iGroupName   = 1
iGroupSplit  = 2
iGroupIcon   = 3

iGroupPosId  = 0
iGroupPosX_o = 1
iGroupPosY_o = 2
iGroupPosX_i = 3
iGroupPosY_i = 4

iPortGroup   = 0
iPortId      = 1
iPortName    = 2
iPortMode    = 3
iPortType    = 4

iConnId      = 0
iConnOutput  = 1
iConnInput   = 2

# Add Group Dialog
class CatarinaAddGroupW(QDialog, ui_catarina_addgroup.Ui_CatarinaAddGroupW):
    def __init__(self, parent, group_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list_names = []

        for group in group_list:
          self.m_group_list_names.append(group[iGroupName])

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_group_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.ret_group_name  = ""
        self.ret_group_split = False

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text and text not in self.m_group_list_names)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_group_name  = self.le_group_name.text()
        self.ret_group_split = self.cb_split.isChecked()

# Remove Group Dialog
class CatarinaRemoveGroupW(QDialog, ui_catarina_removegroup.Ui_CatarinaRemoveGroupW):
    def __init__(self, parent, group_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        i = 0
        for group in group_list:
          twi_group_id    = QTableWidgetItem(str(group[iGroupId]))
          twi_group_name  = QTableWidgetItem(group[iGroupName])
          twi_group_split = QTableWidgetItem("Yes" if (group[iGroupSplit]) else "No")
          self.tw_group_list.insertRow(i)
          self.tw_group_list.setItem(i, 0, twi_group_id)
          self.tw_group_list.setItem(i, 1, twi_group_name)
          self.tw_group_list.setItem(i, 2, twi_group_split)
          i += 1

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.tw_group_list, SIGNAL("cellDoubleClicked(int, int)"), SLOT("accept()"))
        self.connect(self.tw_group_list, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkCell(int)"))

        self.ret_group_id = -1

    @pyqtSlot(int)
    def slot_checkCell(self, row):
        check = bool(row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_group_id = int(self.tw_group_list.item(self.tw_group_list.currentRow(), 0).text())

# Rename Group Dialog
class CatarinaRenameGroupW(QDialog, ui_catarina_renamegroup.Ui_CatarinaRenameGroupW):
    def __init__(self, parent, group_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        for group in group_list:
          self.cb_group_to_rename.addItem("%i - %s" % (group[iGroupId], group[iGroupName]))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.cb_group_to_rename, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkItem()"))
        self.connect(self.le_new_group_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.ret_group_id = -1
        self.ret_new_group_name = ""

    @pyqtSlot()
    def slot_checkItem(self, index):
        self.checkText(self.le_new_group_name.text())

    @pyqtSlot(str)
    def slot_checkText(self, text):
        if (self.cb_group_to_rename.count() > 0):
          group_name = self.cb_group_to_rename.currentText().split(" - ", 1)[1]
          check = bool(text and text != group_name)
          self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_group_id = int(self.cb_group_to_rename.currentText().split(" - ", 1)[0])
        self.ret_new_group_name = self.le_new_group_name.text()

# Main Window
class CatarinaMainW(QMainWindow, ui_catarina.Ui_CatarinaMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Catarina")
        self.loadSettings()

        self.act_project_new.setIcon(getIcon("document-new"))
        self.act_project_open.setIcon(getIcon("document-open"))
        self.act_project_save.setIcon(getIcon("document-save"))
        self.act_project_save_as.setIcon(getIcon("document-save-as"))
        self.b_project_new.setIcon(getIcon("document-new"))
        self.b_project_open.setIcon(getIcon("document-open"))
        self.b_project_save.setIcon(getIcon("document-save"))
        self.b_project_save_as.setIcon(getIcon("document-save-as"))

        self.act_patchbay_add_group.setIcon(getIcon("list-add"))
        self.act_patchbay_remove_group.setIcon(getIcon("edit-delete"))
        self.act_patchbay_rename_group.setIcon(getIcon("edit-rename"))
        self.act_patchbay_add_port.setIcon(getIcon("list-add"))
        self.act_patchbay_remove_port.setIcon(getIcon("list-remove"))
        self.act_patchbay_rename_port.setIcon(getIcon("edit-rename"))
        self.act_patchbay_connect_ports.setIcon(getIcon("network-connect"))
        self.act_patchbay_disconnect_ports.setIcon(getIcon("network-disconnect"))
        self.b_group_add.setIcon(getIcon("list-add"))
        self.b_group_remove.setIcon(getIcon("edit-delete"))
        self.b_group_rename.setIcon(getIcon("edit-rename"))
        self.b_port_add.setIcon(getIcon("list-add"))
        self.b_port_remove.setIcon(getIcon("list-remove"))
        self.b_port_rename.setIcon(getIcon("edit-rename"))
        self.b_ports_connect.setIcon(getIcon("network-connect"))
        self.b_ports_disconnect.setIcon(getIcon("network-disconnect"))

        setIcons(self, ("canvas",))

        self.scene = patchcanvas.PatchScene(self, self.graphicsView)
        self.graphicsView.setScene(self.scene)
        self.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.m_savedSettings["Canvas/Antialiasing"] == Qt.Checked))
        self.graphicsView.setRenderHint(QPainter.TextAntialiasing, self.m_savedSettings["Canvas/TextAntialiasing"])
        if (self.m_savedSettings["Canvas/UseOpenGL"] and hasGL):
          self.graphicsView.setViewport(QGLWidget(self.graphicsView))
          self.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.m_savedSettings["Canvas/HighQualityAntialiasing"])

        p_options = patchcanvas.options_t()
        p_options.theme_name       = self.m_savedSettings["Canvas/Theme"]
        p_options.bezier_lines     = self.m_savedSettings["Canvas/BezierLines"]
        p_options.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
        p_options.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
        p_options.fancy_eyecandy   = self.m_savedSettings["Canvas/FancyEyeCandy"]

        p_features = patchcanvas.features_t()
        p_features.group_info       = False
        p_features.group_rename     = True
        p_features.port_info        = True
        p_features.port_rename      = True
        p_features.handle_group_pos = False

        patchcanvas.set_options(p_options)
        patchcanvas.set_features(p_features)
        patchcanvas.init(self.scene, self.canvasCallback, DEBUG)

        self.connect(self.act_project_new, SIGNAL("triggered()"), SLOT("slot_projectNew()"))
        #self.connect(self.act_project_open, SIGNAL("triggered()"), SLOT("slot_projectOpen()"))
        #self.connect(self.act_project_save, SIGNAL("triggered()"), SLOT("slot_projectSave()"))
        #self.connect(self.act_project_save_as, SIGNAL("triggered()"), SLOT("slot_projectSaveAs()"))
        self.connect(self.b_project_new, SIGNAL("clicked()"), SLOT("slot_projectNew()"))
        #self.connect(self.b_project_open, SIGNAL("clicked()"), SLOT("slot_projectOpen()"))
        #self.connect(self.b_project_save, SIGNAL("clicked()"), SLOT("slot_projectSave()"))
        #self.connect(self.b_project_save_as, SIGNAL("clicked()"), SLOT("slot_projectSaveAs()"))
        self.connect(self.act_patchbay_add_group, SIGNAL("triggered()"), SLOT("slot_groupAdd()"))
        self.connect(self.act_patchbay_remove_group, SIGNAL("triggered()"), SLOT("slot_groupRemove()"))
        self.connect(self.act_patchbay_rename_group, SIGNAL("triggered()"), SLOT("slot_groupRename()"))
        #self.connect(self.act_patchbay_add_port, SIGNAL("triggered()"),SLOT("slot_portAdd()"))
        #self.connect(self.act_patchbay_remove_port, SIGNAL("triggered()"), SLOT("slot_portRemove()"))
        #self.connect(self.act_patchbay_rename_port, SIGNAL("triggered()"), SLOT("slot_portRename()"))
        #self.connect(self.act_patchbay_connect_ports, SIGNAL("triggered()"), SLOT("slot_connectPorts()"))
        #self.connect(self.act_patchbay_disconnect_ports, SIGNAL("triggered()"), SLOT("slot_disconnectPorts()"))
        self.connect(self.b_group_add, SIGNAL("clicked()"), SLOT("slot_groupAdd()"))
        self.connect(self.b_group_remove, SIGNAL("clicked()"), SLOT("slot_groupRemove()"))
        self.connect(self.b_group_rename, SIGNAL("clicked()"), SLOT("slot_groupRename()"))
        #self.connect(self.b_port_add, SIGNAL("clicked()"), SLOT("slot_portAdd()"))
        #self.connect(self.b_port_remove, SIGNAL("clicked()"), SLOT("slot_portRemove()"))
        #self.connect(self.b_port_rename, SIGNAL("clicked()"), SLOT("slot_portRename()"))
        #self.connect(self.b_ports_connect, SIGNAL("clicked()"), SLOT("slot_connectPorts()"))
        #self.connect(self.b_ports_disconnect, SIGNAL("clicked()"), SLOT("slot_disconnectPorts()"))

        #setCanvasConnections(self)

        #self.connect(self.act_settings_configure, SIGNAL("triggered()"), self.configureCatarina)

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCatarina()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        #self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_projectSave()"))

        # Dummy timer to keep events active
        self.m_updateTimer = self.startTimer(500)

        # Start Empty Project
        self.slot_projectNew()

    def canvasCallback(self, action, value1, value2, value_str):
        print(action, value1, value2, value_str)

    @pyqtSlot()
    def slot_projectNew(self):
        self.m_group_list = []
        self.m_group_list_pos = []
        self.m_port_list = []
        self.m_connection_list = []
        self.m_last_group_id = 1
        self.m_last_port_id = 1
        self.m_last_connection_id = 1
        self.m_save_path = None
        patchcanvas.clear()

    @pyqtSlot()
    def slot_groupAdd(self):
        dialog = CatarinaAddGroupW(self, self.m_group_list)
        if (dialog.exec_()):
          group_id    = self.m_last_group_id
          group_name  = dialog.ret_group_name
          group_split = dialog.ret_group_split
          group_icon  = patchcanvas.ICON_HARDWARE if (group_split) else patchcanvas.ICON_APPLICATION
          patchcanvas.addGroup(group_id, group_name, group_split, group_icon)

          group_obj = [None, None, None, None]
          group_obj[iGroupId]    = group_id
          group_obj[iGroupName]  = group_name
          group_obj[iGroupSplit] = group_split
          group_obj[iGroupIcon]  = group_icon

          self.m_group_list.append(group_obj)
          self.m_last_group_id += 1

    @pyqtSlot()
    def slot_groupRemove(self):
        dialog = CatarinaRemoveGroupW(self, self.m_group_list)
        if (dialog.exec_()):
          group_id = dialog.ret_group_id

          #for port in self.m_port_list:
            #if (group_id == port[iPortGroup]):
              #port_id = port[iPortId]

              #h = 0
              #for j in range(len(self.connection_list)):
                #if (self.connection_list[j-h][iConnOutput] == port_id or self.connection_list[j-h][iConnInput] == port_id):
                  #patchcanvas.disconnectPorts(self.connection_list[j-h][iConnId])
                  #self.connection_list.pop(j-h)
                  #h += 1

          #h = 0
          #for i in range(len(self.port_list)):
            #if (self.port_list[i-h][iPortGroup] == group_id):
              #port_id = self.port_list[i-h][iPortId]
              #patchcanvas.removePort(port_id)
              #self.port_list.pop(i-h)
              #h += 1

          #patchcanvas.removeGroup(group_id)

          #for i in range(len(self.group_list)):
            #if (self.group_list[i][iGroupId] == group_id):
              #self.group_list.pop(i)
              #break

    @pyqtSlot()
    def slot_groupRename(self):
        dialog = CatarinaRenameGroupW(self, self.m_group_list)
        if (dialog.exec_()):
          group_id       = dialog.ret_group_id
          new_group_name = dialog.ret_new_group_name
          patchcanvas.renameGroup(group_id, new_group_name)

          for group in self.m_group_list:
            if (group[iGroupId] == group_id):
              group[iGroupName] = new_group_name
              break

    @pyqtSlot()
    def slot_aboutCatarina(self):
        QMessageBox.about(self, self.tr("About Catarina"), self.tr("<h3>Catarina</h3>"
            "<br>Version %s"
            "<br>Catarina is a testing ground for the 'PatchCanvas' module.<br>"
            "<br>Copyright (C) 2010-2012 falkTX") % (VERSION))

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isVisible())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))

        show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
        self.act_settings_show_toolbar.setChecked(show_toolbar)
        self.frame_toolbar.setVisible(show_toolbar)

        self.m_savedSettings = {
          "Canvas/Theme": self.settings.value("Canvas/Theme", patchcanvas.getThemeName(patchcanvas.getDefaultTheme), type=str),
          "Canvas/BezierLines": self.settings.value("Canvas/BezierLines", True, type=bool),
          "Canvas/AutoHideGroups": self.settings.value("Canvas/AutoHideGroups", False, type=bool),
          "Canvas/FancyEyeCandy": self.settings.value("Canvas/FancyEyeCandy", False, type=bool),
          "Canvas/UseOpenGL": self.settings.value("Canvas/UseOpenGL", False, type=bool),
          "Canvas/Antialiasing": self.settings.value("Canvas/Antialiasing", Qt.PartiallyChecked, type=int),
          "Canvas/TextAntialiasing": self.settings.value("Canvas/TextAntialiasing", True, type=bool),
          "Canvas/HighQualityAntialiasing": self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

    def timerEvent(self, event):
        if (event.timerId() == self.m_updateTimer):
          self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Catarina")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("falkTX")
    app.setWindowIcon(QIcon(":/scalable/catarina.svg"))

    # Show GUI
    gui = CatarinaMainW()

    # Set-up custom signal handling
    set_up_signals(gui)

    #if (app.arguments().count() > 1):
      #gui.save_path = QStringStr(app.arguments()[1])
      #gui.prepareToloadFile()

    gui.show()

    # App-Loop
    sys.exit(app.exec_())

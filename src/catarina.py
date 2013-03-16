#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchCanvas test application
# Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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

from PyQt4.QtCore import pyqtSlot, QSettings
from PyQt4.QtGui import QApplication, QDialog, QDialogButtonBox, QTableWidgetItem
from PyQt4.QtXml import QDomDocument

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_catarina
import ui_catarina_addgroup
import ui_catarina_removegroup
import ui_catarina_renamegroup
import ui_catarina_addport
import ui_catarina_removeport
import ui_catarina_renameport
import ui_catarina_connectports
import ui_catarina_disconnectports
from shared_canvasjack import *
from shared_settings import *

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

iGroupId    = 0
iGroupName  = 1
iGroupSplit = 2
iGroupIcon  = 3

iGroupPosId  = 0
iGroupPosX_o = 1
iGroupPosY_o = 2
iGroupPosX_i = 3
iGroupPosY_i = 4

iPortGroup = 0
iPortId    = 1
iPortName  = 2
iPortMode  = 3
iPortType  = 4

iConnId     = 0
iConnOutput = 1
iConnInput  = 2

# ------------------------------------------------------------------------------------------------------------
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

        self.ret_group_name = ""
        self.ret_group_split = False

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text and text not in self.m_group_list_names)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_group_name = self.le_group_name.text()
        self.ret_group_split = self.cb_split.isChecked()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Remove Group Dialog

class CatarinaRemoveGroupW(QDialog, ui_catarina_removegroup.Ui_CatarinaRemoveGroupW):
    def __init__(self, parent, group_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        index = 0
        for group in group_list:
            twi_group_id = QTableWidgetItem(str(group[iGroupId]))
            twi_group_name = QTableWidgetItem(group[iGroupName])
            twi_group_split = QTableWidgetItem("Yes" if (group[iGroupSplit]) else "No")
            self.tw_group_list.insertRow(index)
            self.tw_group_list.setItem(index, 0, twi_group_id)
            self.tw_group_list.setItem(index, 1, twi_group_name)
            self.tw_group_list.setItem(index, 2, twi_group_split)
            index += 1

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
        if self.tw_group_list.rowCount() >= 0:
            self.ret_group_id = int(self.tw_group_list.item(self.tw_group_list.currentRow(), 0).text())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Rename Group Dialog

class CatarinaRenameGroupW(QDialog, ui_catarina_renamegroup.Ui_CatarinaRenameGroupW):
    def __init__(self, parent, group_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list_names = []

        for group in group_list:
            self.cb_group_to_rename.addItem("%i - %s" % (group[iGroupId], group[iGroupName]))
            self.m_group_list_names.append(group[iGroupName])

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.cb_group_to_rename, SIGNAL("currentIndexChanged(int)"), SLOT("slot_checkItem()"))
        self.connect(self.le_new_group_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.ret_group_id = -1
        self.ret_new_group_name = ""

    @pyqtSlot()
    def slot_checkItem(self):
        self.slot_checkText(self.le_new_group_name.text())

    @pyqtSlot(str)
    def slot_checkText(self, text):
        if self.cb_group_to_rename.count() > 0:
            group_name = self.cb_group_to_rename.currentText().split(" - ", 1)[1]
            check = bool(text and text != group_name and text not in self.m_group_list_names)
            self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        self.ret_group_id = int(self.cb_group_to_rename.currentText().split(" - ", 1)[0])
        self.ret_new_group_name = self.le_new_group_name.text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Add Port Dialog

class CatarinaAddPortW(QDialog, ui_catarina_addport.Ui_CatarinaAddPortW):
    def __init__(self, parent, group_list, port_id):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.sb_port_id.setValue(port_id)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        for group in group_list:
            self.cb_group.addItem("%i - %s" % (group[iGroupId], group[iGroupName]))

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.le_port_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.ret_group_id  = -1
        self.ret_port_name = ""
        self.ret_port_mode = patchcanvas.PORT_MODE_NULL
        self.ret_port_type = patchcanvas.PORT_TYPE_NULL

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.cb_group.count() > 0:
            self.ret_group_id = int(self.cb_group.currentText().split(" ", 1)[0])
            self.ret_port_name = self.le_port_name.text()
            self.ret_port_mode = patchcanvas.PORT_MODE_INPUT if self.rb_flags_input.isChecked() else patchcanvas.PORT_MODE_OUTPUT
            self.ret_port_type = self.cb_port_type.currentIndex() + 1 # 1, 2, 3 or 4 for patchcanvas types

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Remove Port Dialog

class CatarinaRemovePortW(QDialog, ui_catarina_removeport.Ui_CatarinaRemovePortW):
    def __init__(self, parent, group_list, port_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.tw_port_list.setColumnWidth(0, 25)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list = group_list
        self.m_port_list  = port_list

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.tw_port_list, SIGNAL("cellDoubleClicked(int, int)"), SLOT("accept()"))
        self.connect(self.tw_port_list, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkCell(int)"))
        self.connect(self.rb_input, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_output, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_audio_jack, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_jack, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_a2j, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_alsa, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))

        self.ret_port_id = -1
        self.reAddPorts()

    def reAddPorts(self):
        self.tw_port_list.clearContents()
        for x in range(self.tw_port_list.rowCount()):
            self.tw_port_list.removeRow(0)

        port_mode = patchcanvas.PORT_MODE_INPUT if (self.rb_input.isChecked()) else patchcanvas.PORT_MODE_OUTPUT

        if self.rb_audio_jack.isChecked():
            port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif self.rb_midi_jack.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_JACK
        elif self.rb_midi_a2j.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_A2J
        elif self.rb_midi_alsa.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_ALSA
        else:
            print("CatarinaRemovePortW::reAddPorts() - Invalid port type")
            return

        index = 0
        for port in self.m_port_list:
            if port[iPortMode] == port_mode and port[iPortType] == port_type:
                port_name = port[iPortName]
                group_name = self.findPortGroupName(port[iPortGroup])
                tw_port_id = QTableWidgetItem(str(port[iPortId]))
                tw_port_name = QTableWidgetItem("%s:%s" % (group_name, port_name))
                self.tw_port_list.insertRow(index)
                self.tw_port_list.setItem(index, 0, tw_port_id)
                self.tw_port_list.setItem(index, 1, tw_port_name)
                index += 1

    def findPortGroupName(self, group_id):
        for group in self.m_group_list:
            if group[iGroupId] == group_id:
                return group[iGroupName]
        return ""

    @pyqtSlot()
    def slot_reAddPorts(self):
        self.reAddPorts()

    @pyqtSlot(int)
    def slot_checkCell(self, row):
        check = bool(row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.tw_port_list.rowCount() > 0:
            self.ret_port_id = int(self.tw_port_list.item(self.tw_port_list.currentRow(), 0).text())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Rename Port Dialog

class CatarinaRenamePortW(QDialog, ui_catarina_renameport.Ui_CatarinaRenamePortW):
    def __init__(self, parent, group_list, port_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.tw_port_list.setColumnWidth(0, 25)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list = group_list
        self.m_port_list  = port_list

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.tw_port_list, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkCell()"))
        self.connect(self.le_new_name, SIGNAL("textChanged(QString)"), SLOT("slot_checkText(QString)"))

        self.connect(self.rb_input, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_output, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_audio_jack, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_jack, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_a2j, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))
        self.connect(self.rb_midi_alsa, SIGNAL("clicked()"), SLOT("slot_reAddPorts()"))

        self.ret_port_id = -1
        self.ret_new_port_name = ""
        self.reAddPorts()

    def reAddPorts(self):
        self.tw_port_list.clearContents()
        for x in range(self.tw_port_list.rowCount()):
            self.tw_port_list.removeRow(0)

        port_mode = patchcanvas.PORT_MODE_INPUT if (self.rb_input.isChecked()) else patchcanvas.PORT_MODE_OUTPUT

        if self.rb_audio_jack.isChecked():
            port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif self.rb_midi_jack.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_JACK
        elif self.rb_midi_a2j.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_A2J
        elif self.rb_midi_alsa.isChecked():
            port_type = patchcanvas.PORT_TYPE_MIDI_ALSA
        else:
            print("CatarinaRenamePortW::reAddPorts() - Invalid port type")
            return

        index = 0
        for port in self.m_port_list:
            if port[iPortMode] == port_mode and port[iPortType] == port_type:
                port_name = port[iPortName]
                group_name = self.findPortGroupName(port[iPortGroup])
                tw_port_id = QTableWidgetItem(str(port[iPortId]))
                tw_port_name = QTableWidgetItem("%s:%s" % (group_name, port_name))
                self.tw_port_list.insertRow(index)
                self.tw_port_list.setItem(index, 0, tw_port_id)
                self.tw_port_list.setItem(index, 1, tw_port_name)
                index += 1

        self.tw_port_list.setCurrentCell(0, 0)

    def findPortGroupName(self, group_id):
        for group in self.m_group_list:
            if group[iGroupId] == group_id:
                return group[iGroupName]
        return ""

    @pyqtSlot()
    def slot_reAddPorts(self):
        self.reAddPorts()

    @pyqtSlot()
    def slot_checkCell(self):
        self.slot_checkText(self.le_new_name.text())

    @pyqtSlot(str)
    def slot_checkText(self, text):
        check = bool(text and self.tw_port_list.currentRow() >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.tw_port_list.rowCount() > 0:
            self.ret_port_id = int(self.tw_port_list.item(self.tw_port_list.currentRow(), 0).text())
            self.ret_new_port_name = self.le_new_name.text()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Connect Ports Dialog

class CatarinaConnectPortsW(QDialog, ui_catarina_connectports.Ui_CatarinaConnectPortsW):
    def __init__(self, parent, group_list, port_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list = group_list
        self.m_port_list = port_list
        self.m_ports_audio_jack = []
        self.m_ports_midi_jack = []
        self.m_ports_midi_a2j = []
        self.m_ports_midi_alsa = []

        for port in self.m_port_list:
            if port[iPortType] == patchcanvas.PORT_TYPE_AUDIO_JACK:
                self.m_ports_audio_jack.append(port)
            elif port[iPortType] == patchcanvas.PORT_TYPE_MIDI_JACK:
                self.m_ports_midi_jack.append(port)
            elif port[iPortType] == patchcanvas.PORT_TYPE_MIDI_A2J:
                self.m_ports_midi_a2j.append(port)
            elif port[iPortType] == patchcanvas.PORT_TYPE_MIDI_ALSA:
                self.m_ports_midi_alsa.append(port)

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.rb_audio_jack, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_jack, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_a2j, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_alsa, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.lw_outputs, SIGNAL("currentRowChanged(int)"), SLOT("slot_checkOutSelection(int)"))
        self.connect(self.lw_inputs, SIGNAL("currentRowChanged(int)"), SLOT("slot_checkInSelection(int)"))

        self.ret_port_out_id = -1
        self.ret_port_in_id = -1
        self.slot_portTypeChanged()

    def showPorts(self, ports):
        self.lw_outputs.clear()
        self.lw_inputs.clear()

        for port in ports:
            if port[iPortMode] == patchcanvas.PORT_MODE_INPUT:
                self.lw_inputs.addItem("%i - %s:%s" % (port[iPortId], self.findPortGroupName(port[iPortGroup]), port[iPortName]))
            elif port[iPortMode] == patchcanvas.PORT_MODE_OUTPUT:
                self.lw_outputs.addItem("%i - %s:%s" % (port[iPortId], self.findPortGroupName(port[iPortGroup]), port[iPortName]))

    def findPortGroupName(self, group_id):
        for group in self.m_group_list:
            if group[iGroupId] == group_id:
                return group[iGroupName]
        return ""

    def checkSelection(self, out_row, in_row):
        check = bool(out_row >= 0 and in_row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_portTypeChanged(self):
        if self.rb_audio_jack.isChecked():
            ports = self.m_ports_audio_jack
        elif self.rb_midi_jack.isChecked():
            ports = self.m_ports_midi_jack
        elif self.rb_midi_a2j.isChecked():
            ports = self.m_ports_midi_a2j
        elif self.rb_midi_alsa.isChecked():
            ports = self.m_ports_midi_alsa
        else:
            print("CatarinaConnectPortstW::portTypeChanged() - Invalid port type")
            return

        self.showPorts(ports)

    @pyqtSlot(int)
    def slot_checkOutSelection(self, row):
        self.checkSelection(row, self.lw_inputs.currentRow())

    @pyqtSlot(int)
    def slot_checkInSelection(self, row):
        self.checkSelection(self.lw_outputs.currentRow(), row)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.lw_outputs.currentRow() >= 0 and self.lw_inputs.currentRow() >= 0:
            self.ret_port_out_id = int(self.lw_outputs.currentItem().text().split(" - ", 1)[0])
            self.ret_port_in_id = int(self.lw_inputs.currentItem().text().split(" - ", 1)[0])

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Disconnect Ports Dialog

class CatarinaDisconnectPortsW(QDialog, ui_catarina_disconnectports.Ui_CatarinaDisconnectPortsW):
    def __init__(self, parent, group_list, port_list, connection_list):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.tw_connections.setColumnWidth(0, 225)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.m_group_list = group_list
        self.m_port_list  = port_list
        self.m_connection_list = connection_list

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturn()"))
        self.connect(self.tw_connections, SIGNAL("cellDoubleClicked(int, int)"), SLOT("accept()"))
        self.connect(self.tw_connections, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkSelection(int)"))
        self.connect(self.rb_audio_jack, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_jack, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_a2j, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))
        self.connect(self.rb_midi_alsa, SIGNAL("clicked()"), SLOT("slot_portTypeChanged()"))

        self.ret_port_out_id = -1
        self.ret_port_in_id = -1
        self.slot_portTypeChanged()

    def showPorts(self, ptype):
        self.tw_connections.clearContents()
        for x in range(self.tw_connections.rowCount()):
            self.tw_connections.removeRow(0)

        index = 0
        for connection in self.m_connection_list:
            if self.findPortType(connection[iConnOutput]) == ptype:
                port_out_id = connection[iConnOutput]
                port_out_name = self.findPortName(port_out_id)

                port_in_id = connection[iConnInput]
                port_in_name = self.findPortName(port_in_id)

                tw_port_out = QTableWidgetItem("%i - %s" % (port_out_id, port_out_name))
                tw_port_in  = QTableWidgetItem("%i - %s" % (port_in_id, port_in_name))

                self.tw_connections.insertRow(index)
                self.tw_connections.setItem(index, 0, tw_port_out)
                self.tw_connections.setItem(index, 1, tw_port_in)
                index += 1

    def findPortName(self, port_id):
        for port in self.m_port_list:
            if port[iPortId] == port_id:
                return "%s:%s" % (self.findPortGroupName(port[iPortGroup]), port[iPortName])
        return ""

    def findPortType(self, port_id):
        for port in self.m_port_list:
            if port[iPortId] == port_id:
                return port[iPortType]
        return patchcanvas.PORT_TYPE_NULL

    def findPortGroupName(self, group_id):
        for group in self.m_group_list:
            if group[iGroupId] == group_id:
                return group[iGroupName]
        return ""

    @pyqtSlot()
    def slot_portTypeChanged(self):
        if self.rb_audio_jack.isChecked():
            ptype = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif self.rb_midi_jack.isChecked():
            ptype = patchcanvas.PORT_TYPE_MIDI_JACK
        elif self.rb_midi_a2j.isChecked():
            ptype = patchcanvas.PORT_TYPE_MIDI_A2J
        elif self.rb_midi_alsa.isChecked():
            ptype = patchcanvas.PORT_TYPE_MIDI_ALSA
        else:
            print("CatarinaDisconnectPortstW::portTypeChanged() - Invalid port type")
            return

        self.showPorts(ptype)

    @pyqtSlot(int)
    def slot_checkSelection(self, row):
        check = bool(row >= 0)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(check)

    @pyqtSlot()
    def slot_setReturn(self):
        if self.tw_connections.currentRow() >= 0:
            self.ret_port_out_id = int(self.tw_connections.item(self.tw_connections.currentRow(), 0).text().split(" - ", 1)[0])
            self.ret_port_in_id  = int(self.tw_connections.item(self.tw_connections.currentRow(), 1).text().split(" - ", 1)[0])

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CatarinaMainW(AbstractCanvasJackClass, ui_catarina.Ui_CatarinaMainW):
    def __init__(self, parent=None):
        AbstractCanvasJackClass.__init__(self, parent, "Catarina")
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Catarina")
        self.loadSettings(True)

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
        p_features.handle_group_pos = True

        patchcanvas.setOptions(p_options)
        patchcanvas.setFeatures(p_features)
        patchcanvas.init("Catarina", self.scene, self.canvasCallback, DEBUG)

        self.connect(self.act_project_new, SIGNAL("triggered()"), SLOT("slot_projectNew()"))
        self.connect(self.act_project_open, SIGNAL("triggered()"), SLOT("slot_projectOpen()"))
        self.connect(self.act_project_save, SIGNAL("triggered()"), SLOT("slot_projectSave()"))
        self.connect(self.act_project_save_as, SIGNAL("triggered()"), SLOT("slot_projectSaveAs()"))
        self.connect(self.b_project_new, SIGNAL("clicked()"), SLOT("slot_projectNew()"))
        self.connect(self.b_project_open, SIGNAL("clicked()"), SLOT("slot_projectOpen()"))
        self.connect(self.b_project_save, SIGNAL("clicked()"), SLOT("slot_projectSave()"))
        self.connect(self.b_project_save_as, SIGNAL("clicked()"), SLOT("slot_projectSaveAs()"))
        self.connect(self.act_patchbay_add_group, SIGNAL("triggered()"), SLOT("slot_groupAdd()"))
        self.connect(self.act_patchbay_remove_group, SIGNAL("triggered()"), SLOT("slot_groupRemove()"))
        self.connect(self.act_patchbay_rename_group, SIGNAL("triggered()"), SLOT("slot_groupRename()"))
        self.connect(self.act_patchbay_add_port, SIGNAL("triggered()"), SLOT("slot_portAdd()"))
        self.connect(self.act_patchbay_remove_port, SIGNAL("triggered()"), SLOT("slot_portRemove()"))
        self.connect(self.act_patchbay_rename_port, SIGNAL("triggered()"), SLOT("slot_portRename()"))
        self.connect(self.act_patchbay_connect_ports, SIGNAL("triggered()"), SLOT("slot_connectPorts()"))
        self.connect(self.act_patchbay_disconnect_ports, SIGNAL("triggered()"), SLOT("slot_disconnectPorts()"))
        self.connect(self.b_group_add, SIGNAL("clicked()"), SLOT("slot_groupAdd()"))
        self.connect(self.b_group_remove, SIGNAL("clicked()"), SLOT("slot_groupRemove()"))
        self.connect(self.b_group_rename, SIGNAL("clicked()"), SLOT("slot_groupRename()"))
        self.connect(self.b_port_add, SIGNAL("clicked()"), SLOT("slot_portAdd()"))
        self.connect(self.b_port_remove, SIGNAL("clicked()"), SLOT("slot_portRemove()"))
        self.connect(self.b_port_rename, SIGNAL("clicked()"), SLOT("slot_portRename()"))
        self.connect(self.b_ports_connect, SIGNAL("clicked()"), SLOT("slot_connectPorts()"))
        self.connect(self.b_ports_disconnect, SIGNAL("clicked()"), SLOT("slot_disconnectPorts()"))

        self.setCanvasConnections()

        self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCatarina()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCatarina()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_projectSave()"))

        # Dummy timer to keep events active
        self.m_updateTimer = self.startTimer(500)

        # Start Empty Project
        self.slot_projectNew()

    def canvasCallback(self, action, value1, value2, value_str):
        if action == patchcanvas.ACTION_GROUP_INFO:
            pass

        elif action == patchcanvas.ACTION_GROUP_RENAME:
            group_id = value1
            new_group_name = value_str

            for group in self.m_group_list:
                if group[iGroupName] == new_group_name:
                    QMessageBox.warning(self, self.tr("Warning"), self.tr("There is already a group with this name"))
                    return

            patchcanvas.renameGroup(group_id, new_group_name)

            for group in self.m_group_list:
                if group[iGroupId] == group_id:
                    group[iGroupName] = new_group_name
                    break

        elif action == patchcanvas.ACTION_GROUP_SPLIT:
            group_id = value1
            patchcanvas.splitGroup(group_id)

            for group in self.m_group_list:
                if group[iGroupId] == group_id:
                    group[iGroupSplit] = True
                    break

        elif action == patchcanvas.ACTION_GROUP_JOIN:
            group_id = value1
            patchcanvas.joinGroup(group_id)

            for group in self.m_group_list:
                if group[iGroupId] == group_id:
                    group[iGroupSplit] = False
                    break

        elif action == patchcanvas.ACTION_PORT_INFO:
            port_id = value1

            group_id   = 0
            group_name = ""
            port_name  = ""
            port_mode  = patchcanvas.PORT_MODE_NULL
            port_type  = patchcanvas.PORT_TYPE_NULL

            for port in self.m_port_list:
                if port[iPortId] == port_id:
                    group_id  = port[iPortGroup]
                    port_name = port[iPortName]
                    port_mode = port[iPortMode]
                    port_type = port[iPortType]
                    break

            for group in self.m_group_list:
                if group[iGroupId] == group_id:
                    group_name = group[iGroupName]
                    break

            if port_mode == patchcanvas.PORT_MODE_INPUT:
                mode_text = self.tr("Input")
            elif port_mode == patchcanvas.PORT_MODE_OUTPUT:
                mode_text = self.tr("Output")
            else:
                mode_text = self.tr("Unknown")

            if port_type == patchcanvas.PORT_TYPE_AUDIO_JACK:
                type_text = self.tr("JACK Audio")
            elif port_type == patchcanvas.PORT_TYPE_MIDI_JACK:
                type_text = self.tr("JACK MIDI")
            elif port_type == patchcanvas.PORT_TYPE_MIDI_A2J:
                type_text = self.tr("A2J MIDI")
            elif port_type == patchcanvas.PORT_TYPE_MIDI_ALSA:
                type_text = self.tr("ALSA MIDI")
            else:
                type_text = self.tr("Unknown")

            port_full_name = group_name + ":" + port_name

            info = self.tr(""
                           "<table>"
                           "<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%1</td></tr>"
                           "<tr><td align='right'><b>Group ID:</b></td><td>&nbsp;%2</td></tr>"
                           "<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%3</td></tr>"
                           "<tr><td align='right'><b>Port ID:</b></td><td>&nbsp;%4</i></td></tr>"
                           "<tr><td align='right'><b>Full Port Name:</b></td><td>&nbsp;%5</td></tr>"
                           "<tr><td colspan='2'>&nbsp;</td></tr>"
                           "<tr><td align='right'><b>Port Mode:</b></td><td>&nbsp;%6</td></tr>"
                           "<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%7</td></tr>"
                           "</table>"
            ).arg(group_name).arg(group_id).arg(port_name).arg(port_id).arg(port_full_name).arg(mode_text).arg(type_text)

            QMessageBox.information(self, self.tr("Port Information"), info)

        elif action == patchcanvas.ACTION_PORT_RENAME:
            port_id = value1
            new_port_name = value_str
            patchcanvas.renamePort(port_id, new_port_name)

            for port in self.m_port_list:
                if port[iPortId] == port_id:
                    port[iPortName] = new_port_name
                    break

        elif action == patchcanvas.ACTION_PORTS_CONNECT:
            connection_id = self.m_last_connection_id
            port_out_id   = value1
            port_in_id    = value2
            patchcanvas.connectPorts(connection_id, port_out_id, port_in_id)

            conn_obj = [None, None, None]
            conn_obj[iConnId]     = connection_id
            conn_obj[iConnOutput] = port_out_id
            conn_obj[iConnInput]  = port_in_id

            self.m_connection_list.append(conn_obj)
            self.m_last_connection_id += 1

        elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
            connection_id = value1
            patchcanvas.disconnectPorts(connection_id)

            for connection in self.m_connection_list:
                if connection[iConnId] == connection_id:
                    self.m_connection_list.remove(connection)
                    break

    def init_ports(self):
        for group in self.m_group_list:
            patchcanvas.addGroup(group[iGroupId], group[iGroupName], patchcanvas.SPLIT_YES if (group[iGroupSplit]) else patchcanvas.SPLIT_NO, group[iGroupIcon])

        for group_pos in self.m_group_list_pos:
            patchcanvas.setGroupPosFull(group_pos[iGroupPosId], group_pos[iGroupPosX_o], group_pos[iGroupPosY_o], group_pos[iGroupPosX_i], group_pos[iGroupPosY_i])

        for port in self.m_port_list:
            patchcanvas.addPort(port[iPortGroup], port[iPortId], port[iPortName], port[iPortMode], port[iPortType])

        for connection in self.m_connection_list:
            patchcanvas.connectPorts(connection[iConnId], connection[iConnOutput], connection[iConnInput])

        self.m_group_list_pos = []
        patchcanvas.updateZValues()

    def saveFile(self, path):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CATARINA>\n"
                   "<CATARINA VERSION='%s'>\n" % VERSION)

        content += " <Groups>\n"
        for i in range(len(self.m_group_list)):
            group = self.m_group_list[i]
            group_id    = group[iGroupId]
            group_name  = group[iGroupName]
            group_split = group[iGroupSplit]
            group_icon  = group[iGroupIcon]
            group_pos_i = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_INPUT)
            group_pos_o = patchcanvas.getGroupPos(group_id, patchcanvas.PORT_MODE_OUTPUT)
            content    += "  <g%i> <name>%s</name> <data>%i:%i:%i:%f:%f:%f:%f</data> </g%i>\n" % (i, group_name, group_id, group_split, group_icon, group_pos_o.x(), group_pos_o.y(), group_pos_i.x(), group_pos_i.y(), i)
        content += " </Groups>\n"

        content += " <Ports>\n"
        for i in range(len(self.m_port_list)):
            port = self.m_port_list[i]
            content += "  <p%i> <name>%s</name> <data>%i:%i:%i:%i</data> </p%i>\n" % (i, port[iPortName], port[iPortGroup], port[iPortId], port[iPortMode], port[iPortType], i)
        content += " </Ports>\n"

        content += " <Connections>\n"
        for i in range(len(self.m_connection_list)):
            connection = self.m_connection_list[i]
            content += "  <c%i>%i:%i:%i</c%i>\n" % (i, connection[iConnId], connection[iConnOutput], connection[iConnInput], i)
        content += " </Connections>\n"

        content += "</CATARINA>\n"

        try:
            fd = uopen(path, "w")
            fd.write(content)
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save file"))

    def loadFile(self, path):
        if not os.path.exists(path):
            QMessageBox.critical(self, self.tr("Error"), self.tr("The file '%s' does not exist" % path))
            self.m_save_path = None
            return

        try:
            fd = uopen(path, "r")
            readState = fd.read()
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load file"))
            self.m_save_path = None
            return

        self.m_save_path       = path
        self.m_group_list      = []
        self.m_group_list_pos  = []
        self.m_port_list       = []
        self.m_connection_list = []
        self.m_last_group_id   = 1
        self.m_last_port_id    = 1
        self.m_last_connection_id = 1

        xml = QDomDocument()
        xml.setContent(readState.encode("utf-8"))

        content = xml.documentElement()
        if content.tagName() != "CATARINA":
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Catarina file"))
            return

        # Get values from XML - the big code
        node = content.firstChild()
        while not node.isNull():
            if node.toElement().tagName() == "Groups":
                group_name = ""
                groups = node.toElement().firstChild()
                while not groups.isNull():
                    group = groups.toElement().firstChild()
                    while not group.isNull():
                        tag = group.toElement().tagName()
                        text = group.toElement().text()
                        if tag == "name":
                            group_name = text
                        elif tag == "data":
                            group_data = text.split(":")
                            if len(group_data) == 7 and group_data[0].isdigit() and group_data[1].isdigit() and group_data[2].isdigit() and isNumber(group_data[3]) and isNumber(group_data[4]) and isNumber(group_data[5]) and isNumber(group_data[6]):
                                group_obj = [None, None, None, None]
                                group_obj[iGroupId]    = int(group_data[0])
                                group_obj[iGroupName]  = group_name
                                group_obj[iGroupSplit] = int(group_data[1])
                                group_obj[iGroupIcon]  = int(group_data[2])

                                group_pos_obj = [None, None, None, None, None]
                                group_pos_obj[iGroupPosId]  = int(group_data[0])
                                group_pos_obj[iGroupPosX_o] = float(group_data[3])
                                group_pos_obj[iGroupPosY_o] = float(group_data[4])
                                group_pos_obj[iGroupPosX_i] = float(group_data[5])
                                group_pos_obj[iGroupPosY_i] = float(group_data[6])

                                self.m_group_list.append(group_obj)
                                self.m_group_list_pos.append(group_pos_obj)

                                group_id = group_obj[iGroupId]
                                if group_id > self.m_last_group_id:
                                    self.m_last_group_id = group_id + 1
                        group = group.nextSibling()
                    groups = groups.nextSibling()

            elif node.toElement().tagName() == "Ports":
                port_name = ""
                ports = node.toElement().firstChild()
                while not ports.isNull():
                    port = ports.toElement().firstChild()
                    while not port.isNull():
                        tag = port.toElement().tagName()
                        text = port.toElement().text()
                        if tag == "name":
                            port_name = text
                        elif tag == "data":
                            port_data = text.split(":")
                            if len(port_data) == 4 and port_data[0].isdigit() and port_data[1].isdigit() and port_data[2].isdigit() and port_data[3].isdigit():
                                new_port = [None, None, None, None, None]
                                new_port[iPortGroup] = int(port_data[0])
                                new_port[iPortId]    = int(port_data[1])
                                new_port[iPortName]  = port_name
                                new_port[iPortMode]  = int(port_data[2])
                                new_port[iPortType]  = int(port_data[3])
                                self.m_port_list.append(new_port)

                                if new_port[iPortId] > self.m_last_port_id:
                                    self.m_last_port_id = new_port[iPortId] + 1

                        port = port.nextSibling()
                    ports = ports.nextSibling()

            elif node.toElement().tagName() == "Connections":
                conns = node.toElement().firstChild()
                while not conns.isNull():
                    conn_data = conns.toElement().text().split(":")
                    if len(conn_data) == 3 and conn_data[0].isdigit() and conn_data[1].isdigit() and conn_data[2].isdigit():
                        conn_obj = [None, None, None]
                        conn_obj[iConnId]     = int(conn_data[0])
                        conn_obj[iConnOutput] = int(conn_data[1])
                        conn_obj[iConnInput]  = int(conn_data[2])

                        connection_id = conn_obj[iConnId]
                        self.m_connection_list.append(conn_obj)

                        if connection_id >= self.m_last_connection_id:
                            self.m_last_connection_id = connection_id + 1

                    conns = conns.nextSibling()
            node = node.nextSibling()

        self.m_last_group_id += 1
        self.m_last_port_id  += 1
        self.m_last_connection_id += 1

        patchcanvas.clear()
        self.init_ports()

        self.scene.zoom_fit()
        self.scene.zoom_reset()

    @pyqtSlot()
    def slot_projectNew(self):
        self.m_group_list      = []
        self.m_group_list_pos  = []
        self.m_port_list       = []
        self.m_connection_list = []
        self.m_last_group_id = 1
        self.m_last_port_id  = 1
        self.m_last_connection_id = 1
        self.m_save_path = None
        patchcanvas.clear()

    @pyqtSlot()
    def slot_projectOpen(self):
        path = QFileDialog.getOpenFileName(self, self.tr("Load State"), filter=self.tr("Catarina XML Document (*.xml)"))
        if path:
            self.loadFile(path)

    @pyqtSlot()
    def slot_projectSave(self):
        if self.m_save_path:
            self.saveFile(self.m_save_path)
        else:
            self.slot_projectSaveAs()

    @pyqtSlot()
    def slot_projectSaveAs(self):
        path = QFileDialog.getSaveFileName(self, self.tr("Save State"), filter=self.tr("Catarina XML Document (*.xml)"))
        if path:
            self.m_save_path = path
            self.saveFile(path)

    @pyqtSlot()
    def slot_groupAdd(self):
        dialog = CatarinaAddGroupW(self, self.m_group_list)
        if dialog.exec_():
            group_id     = self.m_last_group_id
            group_name   = dialog.ret_group_name
            group_split  = dialog.ret_group_split
            group_splitR = patchcanvas.SPLIT_YES if group_split else patchcanvas.SPLIT_NO
            group_icon   = patchcanvas.ICON_HARDWARE if group_split else patchcanvas.ICON_APPLICATION
            patchcanvas.addGroup(group_id, group_name, group_splitR, group_icon)

            group_obj = [None, None, None, None]
            group_obj[iGroupId]    = group_id
            group_obj[iGroupName]  = group_name
            group_obj[iGroupSplit] = group_split
            group_obj[iGroupIcon]  = group_icon

            self.m_group_list.append(group_obj)
            self.m_last_group_id += 1

    @pyqtSlot()
    def slot_groupRemove(self):
        if len(self.m_group_list) > 0:
            dialog = CatarinaRemoveGroupW(self, self.m_group_list)
            if dialog.exec_():
                group_id = dialog.ret_group_id

                # Remove port connections first
                for port in self.m_port_list:
                    if port[iPortGroup] == group_id:
                        port_id = port[iPortId]

                        h = 0
                        for i in range(len(self.m_connection_list)):
                            connection = self.m_connection_list[i-h]
                            if connection[iConnOutput] == port_id or connection[iConnInput] == port_id:
                                patchcanvas.disconnectPorts(connection[iConnId])
                                self.m_connection_list.pop(i-h)
                                h += 1

                # Remove ports
                h = 0
                for i in range(len(self.m_port_list)):
                    port = self.m_port_list[i-h]
                    if port[iPortGroup] == group_id:
                        port_id = port[iPortId]

                        patchcanvas.removePort(port[iPortId])
                        self.m_port_list.pop(i-h)
                        h += 1

                # Now remove group
                patchcanvas.removeGroup(group_id)

                for group in self.m_group_list:
                    if group[iGroupId] == group_id:
                        self.m_group_list.remove(group)
                        break

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add a Group first!"))

    @pyqtSlot()
    def slot_groupRename(self):
        if len(self.m_group_list) > 0:
            dialog = CatarinaRenameGroupW(self, self.m_group_list)
            if dialog.exec_():
                group_id = dialog.ret_group_id
                new_group_name = dialog.ret_new_group_name
                patchcanvas.renameGroup(group_id, new_group_name)

                for group in self.m_group_list:
                    if group[iGroupId] == group_id:
                        group[iGroupName] = new_group_name
                        break

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add a Group first!"))

    @pyqtSlot()
    def slot_portAdd(self):
        if len(self.m_group_list) > 0:
            dialog = CatarinaAddPortW(self, self.m_group_list, self.m_last_port_id)
            if dialog.exec_():
                group_id  = dialog.ret_group_id
                port_name = dialog.ret_port_name
                port_mode = dialog.ret_port_mode
                port_type = dialog.ret_port_type
                patchcanvas.addPort(group_id, self.m_last_port_id, port_name, port_mode, port_type)

                new_port = [None, None, None, None, None]
                new_port[iPortGroup] = group_id
                new_port[iPortId]    = self.m_last_port_id
                new_port[iPortName]  = port_name
                new_port[iPortMode]  = port_mode
                new_port[iPortType]  = port_type

                self.m_port_list.append(new_port)
                self.m_last_port_id += 1

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add a Group first!"))

    @pyqtSlot()
    def slot_portRemove(self):
        if len(self.m_port_list) > 0:
            dialog = CatarinaRemovePortW(self, self.m_group_list, self.m_port_list)
            if dialog.exec_():
                port_id = dialog.ret_port_id

                h = 0
                for i in range(len(self.m_connection_list)):
                    connection = self.m_connection_list[i-h]
                    if connection[iConnOutput] == port_id or connection[iConnInput] == port_id:
                        patchcanvas.disconnectPorts(connection[iConnId])
                        self.m_connection_list.pop(i-h)
                        h += 1

                patchcanvas.removePort(port_id)

                for port in self.m_port_list:
                    if port[iPortId] == port_id:
                        self.m_port_list.remove(port)
                        break

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add a Port first!"))

    @pyqtSlot()
    def slot_portRename(self):
        if len(self.m_port_list) > 0:
            dialog = CatarinaRenamePortW(self, self.m_group_list, self.m_port_list)
            if dialog.exec_():
                port_id = dialog.ret_port_id
                new_port_name = dialog.ret_new_port_name
                patchcanvas.renamePort(port_id, new_port_name)

                for port in self.m_port_list:
                    if port[iPortId] == port_id:
                        port[iPortName] = new_port_name
                        break

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add a Port first!"))

    @pyqtSlot()
    def slot_connectPorts(self):
        if len(self.m_port_list) > 0:
            dialog = CatarinaConnectPortsW(self, self.m_group_list, self.m_port_list)
            if dialog.exec_():
                connection_id = self.m_last_connection_id
                port_out_id   = dialog.ret_port_out_id
                port_in_id    = dialog.ret_port_in_id

                for connection in self.m_connection_list:
                    if connection[iConnOutput] == port_out_id and connection[iConnInput] == port_in_id:
                        QMessageBox.warning(self, self.tr("Warning"), self.tr("Ports already connected!"))
                        return

                patchcanvas.connectPorts(connection_id, port_out_id, port_in_id)

                conn_obj = [None, None, None]
                conn_obj[iConnId]     = connection_id
                conn_obj[iConnOutput] = port_out_id
                conn_obj[iConnInput]  = port_in_id

                self.m_connection_list.append(conn_obj)
                self.m_last_connection_id += 1

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please add some Ports first!"))

    @pyqtSlot()
    def slot_disconnectPorts(self):
        if len(self.m_connection_list) > 0:
            dialog = CatarinaDisconnectPortsW(self, self.m_group_list, self.m_port_list, self.m_connection_list)
            if dialog.exec_():
                connection_id = -1
                port_out_id = dialog.ret_port_out_id
                port_in_id  = dialog.ret_port_in_id

                for connection in self.m_connection_list:
                    if connection[iConnOutput] == port_out_id and connection[iConnInput] == port_in_id:
                        connection_id = connection[iConnId]
                        self.m_connection_list.remove(connection)
                        break

                patchcanvas.disconnectPorts(connection_id)

        else:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Please make some Connections first!"))

    @pyqtSlot()
    def slot_configureCatarina(self):
        dialog = SettingsW(self, "catarina", hasGL)
        if dialog.exec_():
            self.loadSettings(False)
            patchcanvas.clear()

            p_options = patchcanvas.options_t()
            p_options.theme_name       = self.m_savedSettings["Canvas/Theme"]
            p_options.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
            p_options.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
            p_options.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
            p_options.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

            patchcanvas.setOptions(p_options)
            patchcanvas.init("Catarina", self.scene, self.canvasCallback, DEBUG)

            self.init_ports()

    @pyqtSlot()
    def slot_aboutCatarina(self):
        QMessageBox.about(self, self.tr("About Catarina"), self.tr("<h3>Catarina</h3>"
                                                                   "<br>Version %s"
                                                                   "<br>Catarina is a testing ground for the 'PatchCanvas' module.<br>"
                                                                   "<br>Copyright (C) 2010-2012 falkTX") % VERSION)

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isVisible())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.frame_toolbar.setVisible(show_toolbar)

        self.m_savedSettings = {
            "Canvas/Theme": self.settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": self.settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": self.settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": self.settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": self.settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": self.settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/TextAntialiasing": self.settings.value("Canvas/TextAntialiasing", True, type=bool),
            "Canvas/HighQualityAntialiasing": self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

    def timerEvent(self, event):
        if event.timerId() == self.m_updateTimer:
            self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        patchcanvas.clear()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Catarina")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/catarina.svg"))

    # Show GUI
    gui = CatarinaMainW()

    # Set-up custom signal handling
    setUpSignals(gui)

    gui.show()

    if len(app.arguments()) > 1:
        if not app.arguments()[0].endswith("Python.exe"):
            gui.loadFile(app.arguments()[1])
        elif len(app.arguments()) > 2:
            gui.loadFile(app.arguments()[2])

    # App-Loop
    sys.exit(app.exec_())

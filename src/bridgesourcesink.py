#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Custom QTableWidget that handles pulseaudio source and sinks
# Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

# ---------------------------------------------------------------------
# Imports (Global)

from collections import namedtuple

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QComboBox
from shared import *
from shared_cadence import GlobalSettings

# Python3/4 function name normalisation
try:
    range = xrange
except NameError:
    pass

PULSE_USER_CONFIG_DIR = os.getenv("PULSE_USER_CONFIG_DIR")
if not PULSE_USER_CONFIG_DIR:
    PULSE_USER_CONFIG_DIR = os.path.join(HOME, ".pulse")

if not os.path.exists(PULSE_USER_CONFIG_DIR):
    os.path.mkdir(PULSE_USER_CONFIG_DIR)

# a data class to hold the Sink/Source Data. Use strings in tuple for easy map(_make)
# but convert to type in table for editor
SSData = namedtuple('SSData', 'name s_type channels connected')


# ---------------------------------------------------------------------
# Extend QTableWidget to hold Sink/Source data

class BridgeSourceSink(QTableWidget):
    defaultPASourceData = SSData(
        name="PulseAudio JACK Source",
        s_type="source",
        channels="2",
        connected="true")

    defaultPASinkData = SSData(
        name="PulseAudio JACK Sink",
        s_type="sink",
        channels="2",
        connected="true")

    def __init__(self, parent):
        QTableWidget.__init__(self, parent)
        self.bridgeData = []
        if not GlobalSettings.contains("Pulse2JACK/PABridges"):
            self.initialise_settings()
        self.load_from_settings()

    def load_data_into_cells(self):
        self.setHorizontalHeaderLabels(['Name', 'Type', 'Channels', 'Auto Connect'])
        self.setRowCount(0)
        for data in self.bridgeData:
            row = self.rowCount()
            self.insertRow(row)
            # convert to types from strings so the default editors can be used
            self.setItem(row, 0, self.create_widget_item(data.name))
            self.setItem(row, 1, self.create_widget_item(data.s_type))
            # self.setCellWidget(row, 1, self.create_combo_item(data.s_type))
            self.setItem(row, 2, self.create_widget_item(int(data.channels)))
            self.setItem(row, 3, self.create_widget_item(data.connected in ['true', 'True', 'TRUE']))
        self.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

    def create_widget_item(self, i):
        item = QTableWidgetItem()
        item.setData(Qt.EditRole, i)
        return item

    def create_combo_item(self, v):
        comboBox = QComboBox()
        comboBox.addItems(["source", "sink"])
        comboBox.setCurrentIndex(0 if v == "source" else 1)
        return comboBox

    def defaults(self):
        self.bridgeData = [self.defaultPASourceData, self.defaultPASinkData]
        self.load_data_into_cells()

    def undo(self):
        self.load_from_settings()
        self.load_data_into_cells()

    def initialise_settings(self):
        GlobalSettings.setValue(
            "Pulse2JACK/PABridges",
            self.encode_bridge_data([self.defaultPASourceData, self.defaultPASinkData]))

    def load_from_settings(self):
        bridgeDataText = GlobalSettings.value("Pulse2JACK/PABridges")
        self.bridgeData = self.decode_bridge_data(bridgeDataText)

    def add_row(self):
        self.bridgeData.append(SSData(name="", s_type="source", channels="2", connected="false"))
        self.load_data_into_cells()
        self.editItem(self.item(self.rowCount() - 1, 0))

    def remove_row(self):
        currentRow = self.currentRow()
        del self.bridgeData[currentRow]
        self.load_data_into_cells()

    def save_bridges(self):
        self.bridgeData = []
        for row in range(0, self.rowCount()):
            new_name = self.item(row, 0).text()
            new_type = self.item(row, 1).text()
            new_channels = self.item(row, 2).text()
            new_conn = self.item(row, 3).text()
            self.bridgeData.append(
                SSData(name=new_name,
                       s_type=new_type,
                       channels=new_channels,
                       connected=new_conn))
        GlobalSettings.setValue("Pulse2JACK/PABridges", self.encode_bridge_data(self.bridgeData))
        conn_file_path = os.path.join(PULSE_USER_CONFIG_DIR, "jack-connections")
        conn_file = open(conn_file_path, "w")
        conn_file.write("\n".join(self.encode_bridge_data(self.bridgeData)))
        # Need an extra line at the end
        conn_file.write("\n")
        conn_file.close()

    # encode and decode from tuple so it isn't stored in the settings file as a type, and thus the
    # configuration is backwards compatible with versions that don't understand SSData types.
    # Uses PIPE symbol as separator
    # TODO: fail if any pipes in names as it will break the encode/decode of the data
    def encode_bridge_data(self, data):
        return list(map(lambda s: s.name + "|" + s.s_type + "|" + str(s.channels) + "|" + str(s.connected), data))

    def decode_bridge_data(self, data):
        return list(map(lambda d: SSData._make(d.split("|")), data))

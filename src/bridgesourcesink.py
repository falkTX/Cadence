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

from PyQt5.QtCore import Qt, QRegExp
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView, QComboBox, QLineEdit, QSpinBox, QPushButton, QCheckBox, QHBoxLayout, QWidget
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
        connected="True")

    defaultPASinkData = SSData(
        name="PulseAudio JACK Sink",
        s_type="sink",
        channels="2",
        connected="True")

    def __init__(self, parent):
        QTableWidget.__init__(self, parent)
        self.bridgeData = []
        if not GlobalSettings.contains("Pulse2JACK/PABridges"):
            self.initialise_settings()
        self.load_from_settings()

    def load_data_into_cells(self):
        self.setHorizontalHeaderLabels(['Name', 'Type', 'Channels', 'Conn?'])
        self.setRowCount(0)

        for data in self.bridgeData:
            row = self.rowCount()
            self.insertRow(row)

            # Name
            name_col = QLineEdit()
            name_col.setText(data.name)
            name_col.returnPressed.connect(self.enable_buttons)
            rx = QRegExp("[^|]+")
            validator = QRegExpValidator(rx, self)
            name_col.setValidator(validator)
            self.setCellWidget(row, 0, name_col)

            # Type
            combo_box = QComboBox()
            
            microphone_icon = QIcon.fromTheme('audio-input-microphone')
            if microphone_icon.isNull():
                microphone_icon = QIcon.fromTheme('microphone')
                
            loudspeaker_icon = QIcon.fromTheme('audio-volume-high')
            if loudspeaker_icon.isNull():
                loudspeaker_icon = QIcon.fromTheme('player-volume')
            
            combo_box.addItem(microphone_icon, "source")
            combo_box.addItem(loudspeaker_icon, "sink")
            
            combo_box.setCurrentIndex(0 if data.s_type == "source" else 1)
            combo_box.currentTextChanged.connect(self.enable_buttons)
            self.setCellWidget(row, 1, combo_box)

            # Channels
            chan_col = QSpinBox()
            chan_col.setValue(int(data.channels))
            chan_col.setMinimum(1)
            chan_col.valueChanged.connect(self.enable_buttons)
            self.setCellWidget(row, 2, chan_col)

            # Auto connect?
            auto_cb = QCheckBox()
            auto_cb.setObjectName("auto_cb")
            auto_cb.setCheckState(Qt.Checked if data.connected in ['true', 'True', 'TRUE'] else Qt.Unchecked)
            auto_cb.stateChanged.connect(self.enable_buttons)
            widget = QWidget()
            h_layout = QHBoxLayout(widget)
            h_layout.addWidget(auto_cb)
            h_layout.setAlignment(Qt.AlignCenter)
            h_layout.setContentsMargins(0, 0, 0, 0)
            widget.setLayout(h_layout)
            self.setCellWidget(row, 3, widget)
        self.horizontalHeader().setSectionResizeMode(QHeaderView.Fixed)

    def enable_buttons(self):
        # Can't work out how to tell the table that data has changed (to cause the buttons to become enabled),
        # so instead manually make the buttons enabled.
        for btn_name in ["b_bridge_save", "b_bridge_undo"]:
            self.parent().findChild(QPushButton, btn_name).setProperty("enabled", True)

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
        self.bridgeData.append(SSData(name="", s_type="source", channels="2", connected="False"))
        self.load_data_into_cells()
        self.editItem(self.item(self.rowCount() - 1, 0))

    def remove_row(self):
        del self.bridgeData[self.currentRow()]
        self.load_data_into_cells()

    def save_bridges(self):
        self.bridgeData = []
        for row in range(0, self.rowCount()):
            new_name = self.cellWidget(row, 0).property("text")
            new_type = self.cellWidget(row, 1).currentText()
            new_channels = self.cellWidget(row, 2).value()
            auto_cb = self.cellWidget(row, 3).findChild(QCheckBox, "auto_cb")
            new_conn = auto_cb.checkState() == Qt.Checked

            self.bridgeData.append(
                SSData(name=new_name,
                       s_type=new_type,
                       channels=new_channels,
                       connected=str(new_conn)))
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
    def encode_bridge_data(self, data):
        return list(map(lambda s: s.name + "|" + s.s_type + "|" + str(s.channels) + "|" + str(s.connected), data))

    def decode_bridge_data(self, data):
        return list(map(lambda d: SSData._make(d.split("|")), data))

    def resizeEvent(self, event):
        self.setColumnWidth(0, int(self.width() * 0.49))
        self.setColumnWidth(1, int(self.width() * 0.17))
        self.setColumnWidth(2, int(self.width() * 0.17))
        self.setColumnWidth(3, int(self.width() * 0.17))

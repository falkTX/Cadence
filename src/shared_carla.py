#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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
from copy import deepcopy
from sip import unwrapinstance
from PyQt4.QtCore import pyqtSlot, QSettings, QTimer
from PyQt4.QtGui import QColor, QCursor, QDialog, QFontMetrics, QFrame, QInputDialog, QMenu, QPainter, QVBoxLayout, QWidget
from PyQt4.QtXml import QDomDocument

# Imports (Custom)
import ui_carla_edit, ui_carla_parameter, ui_carla_plugin
from shared import *

# Carla Host object
class CarlaHostObject(object):
    __slots__ = [
        'Host',
        'gui'
    ]

Carla = CarlaHostObject()
Carla.Host = None
Carla.gui = None

# ------------------------------------------------------------------------------------------------
# Backend variables

# static max values
MAX_PLUGINS    = 99
MAX_PARAMETERS = 200

# plugin hints
PLUGIN_IS_BRIDGE   = 0x01
PLUGIN_IS_SYNTH    = 0x02
PLUGIN_HAS_GUI     = 0x04
PLUGIN_USES_CHUNKS = 0x08
PLUGIN_CAN_DRYWET  = 0x10
PLUGIN_CAN_VOLUME  = 0x20
PLUGIN_CAN_BALANCE = 0x40

# parameter hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# enum BinaryType
BINARY_NONE   = 0
BINARY_UNIX32 = 1
BINARY_UNIX64 = 2
BINARY_WIN32  = 3
BINARY_WIN64  = 4

# enum PluginType
PLUGIN_NONE   = 0
PLUGIN_LADSPA = 1
PLUGIN_DSSI   = 2
PLUGIN_LV2    = 3
PLUGIN_VST    = 4
PLUGIN_GIG    = 5
PLUGIN_SF2    = 6
PLUGIN_SFZ    = 7

# enum PluginCategory
PLUGIN_CATEGORY_NONE      = 0
PLUGIN_CATEGORY_SYNTH     = 1
PLUGIN_CATEGORY_DELAY     = 2 # also Reverb
PLUGIN_CATEGORY_EQ        = 3
PLUGIN_CATEGORY_FILTER    = 4
PLUGIN_CATEGORY_DYNAMICS  = 5 # Amplifier, Compressor, Gate
PLUGIN_CATEGORY_MODULATOR = 6 # Chorus, Flanger, Phaser
PLUGIN_CATEGORY_UTILITY   = 7 # Analyzer, Converter, Mixer
PLUGIN_CATEGORY_OTHER     = 8 # used to check if a plugin has a category

# enum ParameterType
PARAMETER_UNKNOWN = 0
PARAMETER_INPUT   = 1
PARAMETER_OUTPUT  = 2
PARAMETER_LATENCY = 3

# enum InternalParametersIndex
PARAMETER_ACTIVE = -1
PARAMETER_DRYWET = -2
PARAMETER_VOLUME = -3
PARAMETER_BALANCE_LEFT  = -4
PARAMETER_BALANCE_RIGHT = -5

# enum CustomDataType
CUSTOM_DATA_INVALID = 0
CUSTOM_DATA_STRING  = 1
CUSTOM_DATA_PATH    = 2
CUSTOM_DATA_CHUNK   = 3
CUSTOM_DATA_BINARY  = 4

# enum GuiType
GUI_NONE = 0
GUI_INTERNAL_QT4   = 1
GUI_INTERNAL_COCOA = 2
GUI_INTERNAL_HWND  = 3
GUI_INTERNAL_X11   = 4
GUI_EXTERNAL_LV2   = 5
GUI_EXTERNAL_SUIL  = 6
GUI_EXTERNAL_OSC   = 7

# enum OptionsType
OPTION_PROCESS_MODE           = 1
OPTION_MAX_PARAMETERS         = 2
OPTION_PREFER_UI_BRIDGES      = 3
OPTION_FORCE_STEREO           = 4
OPTION_PROCESS_HIGH_PRECISION = 5
OPTION_OSC_GUI_TIMEOUT        = 6
OPTION_USE_DSSI_CHUNKS        = 7
OPTION_PATH_LADSPA            = 8
OPTION_PATH_DSSI              = 9
OPTION_PATH_LV2               = 10
OPTION_PATH_VST               = 11
OPTION_PATH_GIG               = 12
OPTION_PATH_SF2               = 13
OPTION_PATH_SFZ               = 14
OPTION_PATH_BRIDGE_UNIX32     = 15
OPTION_PATH_BRIDGE_UNIX64     = 16
OPTION_PATH_BRIDGE_WIN32      = 17
OPTION_PATH_BRIDGE_WIN64      = 18
OPTION_PATH_BRIDGE_LV2_GTK2   = 19
OPTION_PATH_BRIDGE_LV2_QT4    = 20
OPTION_PATH_BRIDGE_LV2_X11    = 21
OPTION_PATH_BRIDGE_VST_X11    = 22

# enum CallbackType
CALLBACK_DEBUG                = 0
CALLBACK_PARAMETER_CHANGED    = 1 # parameter_id, 0, value
CALLBACK_PROGRAM_CHANGED      = 2 # program_id, 0, 0
CALLBACK_MIDI_PROGRAM_CHANGED = 3 # midi_program_id, 0, 0
CALLBACK_NOTE_ON              = 4 # key, velocity, 0
CALLBACK_NOTE_OFF             = 5 # key, velocity, 0
CALLBACK_SHOW_GUI             = 6 # show? (0|1, -1=quit), 0, 0
CALLBACK_RESIZE_GUI           = 7 # width, height, 0
CALLBACK_UPDATE               = 8
CALLBACK_RELOAD_INFO          = 9
CALLBACK_RELOAD_PARAMETERS    = 10
CALLBACK_RELOAD_PROGRAMS      = 11
CALLBACK_RELOAD_ALL           = 12
CALLBACK_QUIT                 = 13

# enum ProcessModeType
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2

# ------------------------------------------------------------------------------------------------
# Carla GUI stuff

PROCESS_MODE = PROCESS_MODE_MULTIPLE_CLIENTS

ICON_STATE_NULL = 0
ICON_STATE_WAIT = 1
ICON_STATE_OFF  = 2
ICON_STATE_ON   = 3

PALETTE_COLOR_NONE   = 0
PALETTE_COLOR_WHITE  = 1
PALETTE_COLOR_RED    = 2
PALETTE_COLOR_GREEN  = 3
PALETTE_COLOR_BLUE   = 4
PALETTE_COLOR_YELLOW = 5
PALETTE_COLOR_ORANGE = 6
PALETTE_COLOR_BROWN  = 7
PALETTE_COLOR_PINK   = 8

# Save support
save_state_dict = {
    'Type': "",
    'Name': "",
    'Label': "",
    'Binary': "",
    'UniqueID': 0,
    'Active': False,
    'DryWet': 1.0,
    'Volume': 1.0,
    'Balance-Left': -1.0,
    'Balance-Right': 1.0,
    'Parameters': [],
    'CurrentProgramIndex': -1,
    'CurrentProgramName': "",
    'CurrentMidiBank': -1,
    'CurrentMidiProgram': -1,
    'CustomData': [],
    'Chunk': None
}

save_state_parameter = {
    'index': 0,
    'name': "",
    'symbol': "",
    'value': 0.0,
    'midiChannel': 1,
    'midiCC': -1
}

save_state_custom_data = {
    'type': CUSTOM_DATA_INVALID,
    'key': "",
    'value': ""
}

def CustomDataType2String(dtype):
    if dtype == CUSTOM_DATA_STRING:
        return "string"
    elif dtype == CUSTOM_DATA_PATH:
        return "path"
    elif dtype == CUSTOM_DATA_CHUNK:
        return "chunk"
    elif dtype == CUSTOM_DATA_BINARY:
        return "binary"
    else:
        return "null"

def CustomDataString2Type(stype):
    if stype == "string":
        return CUSTOM_DATA_STRING
    elif stype == "path":
        return CUSTOM_DATA_PATH
    elif stype == "chunk":
        return CUSTOM_DATA_CHUNK
    elif stype == "binary":
        return CUSTOM_DATA_BINARY
    else:
        return CUSTOM_DATA_INVALID

def xmlSafeString(string, toXml):
    if toXml:
        return string.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;")
    else:
        return string.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"")

def getStateDictFromXML(xml_node):
    x_save_state_dict = deepcopy(save_state_dict)

    node = xml_node.firstChild()
    while not node.isNull():
        if (node.toElement().tagName() == "Info"):
            xml_info = node.toElement().firstChild()

            while not xml_info.isNull():
                tag = xml_info.toElement().tagName()
                text = xml_info.toElement().text().strip()

                if tag == "Type":
                    x_save_state_dict['Type'] = text
                elif tag == "Name":
                    x_save_state_dict['Name'] = xmlSafeString(text, False)
                elif tag in ("Label", "URI"):
                    x_save_state_dict['Label'] = xmlSafeString(text, False)
                elif tag == "Binary":
                    x_save_state_dict['Binary'] = xmlSafeString(text, False)
                elif tag == "UniqueID":
                    if text.isdigit():
                        x_save_state_dict['UniqueID'] = int(text)

                xml_info = xml_info.nextSibling()

        elif (node.toElement().tagName() == "Data"):
            xml_data = node.toElement().firstChild()

            while not xml_data.isNull():
                tag = xml_data.toElement().tagName()
                text = xml_data.toElement().text().strip()

                if tag == "Active":
                    x_save_state_dict['Active'] = bool(text == "Yes")
                elif tag == "DryWet":
                    if isNumber(text):
                        x_save_state_dict['DryWet'] = float(text)
                elif (tag == "Volume"):
                    if isNumber(text):
                        x_save_state_dict['Volume'] = float(text)
                elif (tag == "Balance-Left"):
                    if isNumber(text):
                        x_save_state_dict['Balance-Left'] = float(text)
                elif (tag == "Balance-Right"):
                    if isNumber(text):
                        x_save_state_dict['Balance-Right'] = float(text)

                elif tag == "CurrentProgramIndex":
                    if text.isdigit():
                        x_save_state_dict['CurrentProgramIndex'] = int(text)
                elif tag == "CurrentProgramName":
                    x_save_state_dict['CurrentProgramName'] = xmlSafeString(text, False)

                elif tag == "CurrentMidiBank":
                    if text.isdigit():
                        x_save_state_dict['CurrentMidiBank'] = int(text)
                elif tag == "CurrentMidiProgram":
                    if text.isdigit():
                        x_save_state_dict['CurrentMidiProgram'] = int(text)

                elif tag == "Parameter":
                    x_save_state_parameter = deepcopy(save_state_parameter)

                    xml_subdata = xml_data.toElement().firstChild()

                    while not xml_subdata.isNull():
                        ptag = xml_subdata.toElement().tagName()
                        ptext = xml_subdata.toElement().text().strip()

                        if ptag == "index":
                            if ptext.isdigit():
                                x_save_state_parameter['index'] = int(ptext)
                        elif ptag == "name":
                            x_save_state_parameter['name'] = xmlSafeString(ptext, False)
                        elif ptag == "symbol":
                            x_save_state_parameter['symbol'] = xmlSafeString(ptext, False)
                        elif ptag == "value":
                            if isNumber(ptext):
                                x_save_state_parameter['value'] = float(ptext)
                        elif ptag == "midiChannel":
                            if ptext.isdigit():
                                x_save_state_parameter['midiChannel'] = int(ptext)
                        elif ptag == "midiCC":
                            if ptext.isdigit():
                                x_save_state_parameter['midiCC'] = int(ptext)

                        xml_subdata = xml_subdata.nextSibling()

                    x_save_state_dict['Parameters'].append(x_save_state_parameter)

                elif (tag == "CustomData"):
                    x_save_state_custom_data = deepcopy(save_state_custom_data)

                    xml_subdata = xml_data.toElement().firstChild()

                    while not xml_subdata.isNull():
                        ctag = xml_subdata.toElement().tagName()
                        ctext = xml_subdata.toElement().text().strip()

                        if ctag == "type":
                            x_save_state_custom_data['type'] = CustomDataString2Type(ctext)
                        elif ctag == "key":
                            x_save_state_custom_data['key'] = xmlSafeString(ctext, False)
                        elif ctag == "value":
                            x_save_state_custom_data['value'] = xmlSafeString(ctext, False)

                        xml_subdata = xml_subdata.nextSibling()

                    x_save_state_dict['CustomData'].append(x_save_state_custom_data)

                elif tag == "Chunk":
                    x_save_state_dict['Chunk'] = text

                xml_data = xml_data.nextSibling()

        node = node.nextSibling()

    return x_save_state_dict

# ------------------------------------------------------------------------------------------------
# Common widgets

# Plugin GUI
class PluginGUI(QDialog):
    def __init__(self, parent, plugin_name, resizable):
        QDialog.__init__(self, parent)

        self.myLayout = QVBoxLayout(self)
        self.myLayout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.myLayout)

        self.resizable = resizable
        self.setNewSize(50, 50)

        self.setWindowTitle("%s (GUI)" % (plugin_name))

    def setNewSize(self, width, height):
        if (width < 30):
            width = 30
        if (height < 30):
            height = 30

        if (self.resizable):
            self.resize(width, height)
        else:
            self.setFixedSize(width, height)

    def hideEvent(self, event):
        # FIXME
        event.accept()
        self.close()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Single Plugin Parameter
class PluginParameter(QWidget, ui_carla_parameter.Ui_PluginParameter):
    def __init__(self, parent, pinfo, plugin_id):
        QWidget.__init__(self, parent)
        self.setupUi(self)

        self.ptype = pinfo['type']
        self.parameter_id = pinfo['index']
        self.hints = pinfo['hints']

        self.midi_cc = -1
        self.midi_channel = 1
        self.plugin_id = plugin_id

        self.add_MIDI_CCs_to_ComboBox()

        self.label.setText(pinfo['name'])

        if self.ptype == PARAMETER_INPUT:
            self.widget.set_minimum(pinfo['minimum'])
            self.widget.set_maximum(pinfo['maximum'])
            self.widget.set_default(pinfo['default'])
            self.widget.set_value(pinfo['current'], False)
            self.widget.set_label(pinfo['unit'])
            self.widget.set_step(pinfo['step'])
            self.widget.set_step_small(pinfo['stepSmall'])
            self.widget.set_step_large(pinfo['stepLarge'])
            self.widget.set_scalepoints(pinfo['scalepoints'], bool(pinfo['hints'] & PARAMETER_USES_SCALEPOINTS))

            if not self.hints & PARAMETER_IS_ENABLED:
                self.widget.set_read_only(True)
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

            elif not self.hints & PARAMETER_IS_AUTOMABLE:
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        elif self.ptype == PARAMETER_OUTPUT:
            self.widget.set_minimum(pinfo['minimum'])
            self.widget.set_maximum(pinfo['maximum'])
            self.widget.set_value(pinfo['current'], False)
            self.widget.set_label(pinfo['unit'])
            self.widget.set_read_only(True)

            if not self.hints & PARAMETER_IS_AUTOMABLE:
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        else:
            self.widget.setVisible(False)
            self.combo.setVisible(False)
            self.sb_channel.setVisible(False)

        self.set_parameter_midi_channel(pinfo['midiChannel'])
        self.set_parameter_midi_cc(pinfo['midiCC'])

        self.connect(self.widget, SIGNAL("valueChanged(double)"), SLOT("slot_valueChanged(double)"))
        self.connect(self.sb_channel, SIGNAL("valueChanged(int)"), SLOT("slot_midiChannelChanged(int)"))
        self.connect(self.combo, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiCcChanged(int)"))

        #if force_parameters_style:
        #self.widget.force_plastique_style()

        if self.hints & PARAMETER_USES_CUSTOM_TEXT:
            self.widget.set_text_call(self.textCallFunction)

        self.widget.updateAll()

    def set_default_value(self, value):
        self.widget.set_default(value)

    def set_parameter_value(self, value, send=True):
        self.widget.set_value(value, send)

    def set_parameter_midi_channel(self, channel):
        self.midi_channel = channel
        self.sb_channel.setValue(channel - 1)

    def set_parameter_midi_cc(self, cc_index):
        self.midi_cc = cc_index
        self.set_MIDI_CC_in_ComboBox(cc_index)

    def add_MIDI_CCs_to_ComboBox(self):
        for MIDI_CC in MIDI_CC_LIST:
            self.combo.addItem(MIDI_CC)

    def set_MIDI_CC_in_ComboBox(self, midi_cc):
        for i in range(len(MIDI_CC_LIST)):
            midi_cc_text = MIDI_CC_LIST[i].split(" ")[0]
            if int(midi_cc_text, 16) == midi_cc:
                cc_index = i
                break
        else:
            cc_index = -1

        cc_index += 1
        self.combo.setCurrentIndex(cc_index)

    def textCallFunction(self):
        return cString(Carla.Host.get_parameter_text(self.plugin_id, self.parameter_id))

    @pyqtSlot(float)
    def slot_valueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, double)"), self.parameter_id, value)

    @pyqtSlot(int)
    def slot_midiChannelChanged(self, channel):
        if self.midi_channel != channel:
            self.emit(SIGNAL("midiChannelChanged(int, int)"), self.parameter_id, channel)
        self.midi_channel = channel

    @pyqtSlot(int)
    def slot_midiCcChanged(self, cc_index):
        if cc_index <= 0:
            midi_cc = -1
        else:
            midi_cc_text = MIDI_CC_LIST[cc_index - 1].split(" ")[0]
            midi_cc = int(midi_cc_text, 16)

        if self.midi_cc != midi_cc:
            self.emit(SIGNAL("midiCcChanged(int, int)"), self.parameter_id, midi_cc)
        self.midi_cc = midi_cc

# Plugin Editor (Built-in)
class PluginEdit(QDialog, ui_carla_edit.Ui_PluginEdit):
    def __init__(self, parent, plugin_id):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.pinfo = None
        self.ptype = PLUGIN_NONE
        self.plugin_id = plugin_id

        self.parameter_count = 0
        self.parameter_list  = [] # type, id, widget
        self.parameter_list_to_update = [] # ids

        self.state_filename = None
        self.cur_program_index = -1
        self.cur_midi_program_index = -1

        self.tab_icon_off = QIcon(":/bitmaps/led_off.png")
        self.tab_icon_on  = QIcon(":/bitmaps/led_yellow.png")
        self.tab_icon_count = 0
        self.tab_icon_timers = []

        self.connect(self.b_save_state, SIGNAL("clicked()"), SLOT("slot_saveState()"))
        self.connect(self.b_load_state, SIGNAL("clicked()"), SLOT("slot_loadState()"))

        self.connect(self.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))
        self.connect(self.keyboard, SIGNAL("notesOn()"), SLOT("slot_notesOn()"))
        self.connect(self.keyboard, SIGNAL("notesOff()"), SLOT("slot_notesOff()"))

        self.connect(self.cb_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_programIndexChanged(int)"))
        self.connect(self.cb_midi_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiProgramIndexChanged(int)"))

        self.keyboard.setMode(self.keyboard.HORIZONTAL)
        self.keyboard.setOctaves(6)
        self.scrollArea.ensureVisible(self.keyboard.width() * 1 / 5, 0)
        self.scrollArea.setVisible(False)

        # TODO - not implemented yet
        self.b_reload_program.setEnabled(False)
        self.b_reload_midi_program.setEnabled(False)

        self.do_reload_all()

    def set_parameter_to_update(self, parameter_id):
        if parameter_id not in self.parameter_list_to_update:
            self.parameter_list_to_update.append(parameter_id)

    def set_parameter_midi_channel(self, parameter_id, channel):
        for ptype, pid, pwidget in self.parameter_list:
            if pid == parameter_id:
                pwidget.set_parameter_midi_channel(channel)
                break

    def set_parameter_midi_cc(self, parameter_id, midi_cc):
        for ptype, pid, pwidget in self.parameter_list:
            if pid == parameter_id:
                pwidget.set_parameter_midi_cc(midi_cc)
                break

    def set_program(self, program_id):
        self.cur_program_index = program_id
        self.cb_programs.setCurrentIndex(program_id)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def set_midi_program(self, midi_program_id):
        self.cur_midi_program_index = midi_program_id
        self.cb_midi_programs.setCurrentIndex(midi_program_id)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def do_update(self):
        # Update current program text
        if self.cb_programs.count() > 0:
            pindex = self.cb_programs.currentIndex()
            pname  = cString(Carla.Host.get_program_name(self.plugin_id, pindex))
            self.cb_programs.setItemText(pindex, pname)

        # Update current midi program text
        if self.cb_midi_programs.count() > 0:
            mpindex  = self.cb_midi_programs.currentIndex()
            oldtextI = self.cb_midi_programs.currentText().split(" ", 1)[0]
            mpname = "%s %s" % (oldtextI, cString(Carla.Host.get_midi_program_name(self.plugin_id, mpindex)))
            self.cb_midi_programs.setItemText(mpindex, mpname)

        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        QTimer.singleShot(0, self, SLOT("slot_checkOutputControlParameters()"))

    def do_reload_all(self):
        self.pinfo = Carla.Host.get_plugin_info(self.plugin_id)
        self.pinfo["binary"]    = cString(self.pinfo["binary"])
        self.pinfo["name"]      = cString(self.pinfo["name"])
        self.pinfo["label"]     = cString(self.pinfo["label"])
        self.pinfo["maker"]     = cString(self.pinfo["maker"])
        self.pinfo["copyright"] = cString(self.pinfo["copyright"])

        self.do_reload_info()
        self.do_reload_parameters()
        self.do_reload_programs()

    def do_reload_info(self):
        if self.ptype == PLUGIN_NONE and self.pinfo['type'] in (PLUGIN_DSSI, PLUGIN_LV2, PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
            self.tab_programs.setCurrentIndex(1)

        self.ptype = self.pinfo['type']
        real_plugin_name = cString(Carla.Host.get_real_plugin_name(self.plugin_id))

        self.le_name.setText(real_plugin_name)
        self.le_name.setToolTip(real_plugin_name)
        self.le_label.setText(self.pinfo['label'])
        self.le_label.setToolTip(self.pinfo['label'])
        self.le_maker.setText(self.pinfo['maker'])
        self.le_maker.setToolTip(self.pinfo['maker'])
        self.le_copyright.setText(self.pinfo['copyright'])
        self.le_copyright.setToolTip(self.pinfo['copyright'])
        self.le_unique_id.setText(str(self.pinfo['uniqueId']))
        self.le_unique_id.setToolTip(str(self.pinfo['uniqueId']))

        self.label_plugin.setText("\n%s\n" % (self.pinfo['name']))
        self.setWindowTitle(self.pinfo['name'])

        if self.ptype == PLUGIN_LADSPA:
            self.le_type.setText("LADSPA")
        elif self.ptype == PLUGIN_DSSI:
            self.le_type.setText("DSSI")
        elif self.ptype == PLUGIN_LV2:
            self.le_type.setText("LV2")
        elif self.ptype == PLUGIN_VST:
            self.le_type.setText("VST")
        elif self.ptype == PLUGIN_GIG:
            self.le_type.setText("GIG")
        elif self.ptype == PLUGIN_SF2:
            self.le_type.setText("SF2")
        elif self.ptype == PLUGIN_SFZ:
            self.le_type.setText("SFZ")
        else:
            self.le_type.setText(self.tr("Unknown"))

        audio_count = Carla.Host.get_audio_port_count_info(self.plugin_id)
        midi_count  = Carla.Host.get_midi_port_count_info(self.plugin_id)
        param_count = Carla.Host.get_parameter_count_info(self.plugin_id)

        self.le_ains.setText(str(audio_count['ins']))
        self.le_aouts.setText(str(audio_count['outs']))
        self.le_params.setText(str(param_count['ins']))
        self.le_couts.setText(str(param_count['outs']))
        self.le_is_synth.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_IS_SYNTH) else self.tr("No"))
        self.le_has_gui.setText(self.tr("Yes") if (self.pinfo['hints'] & PLUGIN_HAS_GUI) else self.tr("No"))

        self.scrollArea.setVisible((self.pinfo['hints'] & PLUGIN_IS_SYNTH) > 0 or (midi_count['ins'] > 0 < midi_count['outs']))
        self.parent().recheck_hints(self.pinfo['hints'])

    def do_reload_parameters(self):
        parameters_count = Carla.Host.get_parameter_count(self.plugin_id)

        self.parameter_list = []
        self.parameter_list_to_update = []

        self.tab_icon_count  = 0
        self.tab_icon_timers = []

        for i in range(self.tabWidget.count()):
            if i == 0: continue
            self.tabWidget.widget(1).deleteLater()
            self.tabWidget.removeTab(1)

        if parameters_count <= 0:
            pass

        elif parameters_count <= MAX_PARAMETERS:
            p_in = []
            p_in_tmp = []
            p_in_index = 0
            p_in_width = 0

            p_out = []
            p_out_tmp = []
            p_out_index = 0
            p_out_width = 0

            for i in range(parameters_count):
                param_info = Carla.Host.get_parameter_info(self.plugin_id, i)
                param_data = Carla.Host.get_parameter_data(self.plugin_id, i)
                param_ranges = Carla.Host.get_parameter_ranges(self.plugin_id, i)

                if param_data['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                    continue

                parameter = {
                    'type': param_data['type'],
                    'hints': param_data['hints'],
                    'name': cString(param_info['name']),
                    'unit': cString(param_info['unit']),
                    'scalepoints': [],

                    'index': param_data['index'],
                    'default': param_ranges['def'],
                    'minimum': param_ranges['min'],
                    'maximum': param_ranges['max'],
                    'step': param_ranges['step'],
                    'stepSmall': param_ranges['stepSmall'],
                    'stepLarge': param_ranges['stepLarge'],
                    'midiChannel': param_data['midiChannel'],
                    'midiCC': param_data['midiCC'],

                    'current': Carla.Host.get_current_parameter_value(self.plugin_id, i)
                }

                for j in range(param_info['scalePointCount']):
                    scalepoint = Carla.Host.get_parameter_scalepoint_info(self.plugin_id, i, j)
                    parameter['scalepoints'].append({
                          'value': scalepoint['value'],
                          'label': cString(scalepoint['label'])
                        })

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if parameter['type'] == PARAMETER_INPUT:
                    p_in_tmp.append(parameter)
                    p_in_width_tmp = QFontMetrics(self.font()).width(parameter['name'])

                    if p_in_width_tmp > p_in_width:
                        p_in_width = p_in_width_tmp

                    if len(p_in_tmp) == 10:
                        p_in.append((p_in_tmp, p_in_width))
                        p_in_tmp = []
                        p_in_index = 0
                        p_in_width = 0
                    else:
                        p_in_index += 1

                elif parameter['type'] == PARAMETER_OUTPUT:
                    p_out_tmp.append(parameter)
                    p_out_width_tmp = QFontMetrics(self.font()).width(parameter['name'])

                    if p_out_width_tmp > p_out_width:
                        p_out_width = p_out_width_tmp

                    if len(p_out_tmp) == 10:
                        p_out.append((p_out_tmp, p_out_width))
                        p_out_tmp = []
                        p_out_index = 0
                        p_out_width = 0
                    else:
                        p_out_index += 1

            else:
                # Final page width values
                if 0 < len(p_in_tmp) < 10:
                    p_in.append((p_in_tmp, p_in_width))

                if 0 < len(p_out_tmp) < 10:
                    p_out.append((p_out_tmp, p_out_width))

            # -----------------------------------------------------------------
            # Create parameter widgets

            if len(p_in) > 0:
                self.createParameterWidgets(p_in, self.tr("Parameters"), PARAMETER_INPUT)

            if len(p_out) > 0:
                self.createParameterWidgets(p_out, self.tr("Outputs"), PARAMETER_OUTPUT)

        else: # > MAX_PARAMETERS
            fake_name = "This plugin has too many parameters to display here!"

            p_fake = []
            p_fake_tmp = []
            p_fake_width = QFontMetrics(self.font()).width(fake_name)

            parameter = {
                'type': PARAMETER_UNKNOWN,
                'hints': 0,
                'name': fake_name,
                'unit': "",
                'scalepoints': [],

                'index': 0,
                'default': 0,
                'minimum': 0,
                'maximum': 0,
                'step': 0,
                'stepSmall': 0,
                'stepLarge': 0,
                'midiChannel': 0,
                'midiCC': -1,

                'current': 0.0
            }

            p_fake_tmp.append(parameter)
            p_fake.append((p_fake_tmp, p_fake_width))

            self.createParameterWidgets(p_fake, self.tr("Information"), PARAMETER_UNKNOWN)

    def do_reload_programs(self):
        # Programs
        self.cb_programs.blockSignals(True)
        self.cb_programs.clear()

        program_count = Carla.Host.get_program_count(self.plugin_id)

        if program_count > 0:
            self.cb_programs.setEnabled(True)

            for i in range(program_count):
                pname = cString(Carla.Host.get_program_name(self.plugin_id, i))
                self.cb_programs.addItem(pname)

            self.cur_program_index = Carla.Host.get_current_program_index(self.plugin_id)
            self.cb_programs.setCurrentIndex(self.cur_program_index)

        else:
            self.cb_programs.setEnabled(False)

        self.cb_programs.blockSignals(False)

        # MIDI Programs
        self.cb_midi_programs.blockSignals(True)
        self.cb_midi_programs.clear()

        midi_program_count = Carla.Host.get_midi_program_count(self.plugin_id)

        if midi_program_count > 0:
            self.cb_midi_programs.setEnabled(True)

            for i in range(midi_program_count):
                midip = Carla.Host.get_midi_program_data(self.plugin_id, i)

                bank = int(midip['bank'])
                prog = int(midip['program'])
                label = cString(midip['label'])

                self.cb_midi_programs.addItem("%03i:%03i - %s" % (bank, prog, label))

            self.cur_midi_program_index = Carla.Host.get_current_midi_program_index(self.plugin_id)
            self.cb_midi_programs.setCurrentIndex(self.cur_midi_program_index)

        else:
            self.cb_midi_programs.setEnabled(False)

        self.cb_midi_programs.blockSignals(False)

    def saveState_InternalFormat(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PRESET>\n"
                   "<CARLA-PRESET VERSION='%s'>\n") % VERSION

        content += self.parent().getSaveXMLContent()

        content += "</CARLA-PRESET>\n"

        try:
            fd = open(self.state_filename, "w")
            fd.write(content)
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save state file"))

    def saveState_Lv2Format(self):
        pass

    def saveState_VstFormat(self):
        pass

    def loadState_InternalFormat(self):
        try:
            fd = open(self.state_filename, "r")
            stateRead = fd.read()
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load state file"))
            return

        xml = QDomDocument()
        xml.setContent(stateRead)

        xml_node = xml.documentElement()

        if xml_node.tagName() != "CARLA-PRESET":
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla state file"))
            return

        x_save_state_dict = getStateDictFromXML(xml_node)

        self.parent().loadStateDict(x_save_state_dict)

    def createParameterWidgets(self, p_list_full, tab_name, ptype):
        for i in range(len(p_list_full)):
            p_list = p_list_full[i][0]
            width = p_list_full[i][1]

            if len(p_list) > 0:
                container = QWidget(self.tabWidget)
                layout = QVBoxLayout(container)
                container.setLayout(layout)

                for j in range(len(p_list)):
                    pwidget = PluginParameter(container, p_list[j], self.plugin_id)
                    pwidget.label.setMinimumWidth(width)
                    pwidget.label.setMaximumWidth(width)
                    pwidget.tab_index = self.tabWidget.count()
                    layout.addWidget(pwidget)

                    self.parameter_list.append((ptype, p_list[j]['index'], pwidget))

                    if (ptype == PARAMETER_INPUT):
                        self.connect(pwidget, SIGNAL("valueChanged(int, double)"), SLOT("slot_parameterValueChanged(int, double)"))

                    self.connect(pwidget, SIGNAL("midiChannelChanged(int, int)"), SLOT("slot_parameterMidiChannelChanged(int, int)"))
                    self.connect(pwidget, SIGNAL("midiCcChanged(int, int)"), SLOT("slot_parameterMidiCcChanged(int, int)"))

                # FIXME
                layout.addStretch()

                self.tabWidget.addTab(container, "%s (%i)" % (tab_name, i + 1))

                if (ptype == PARAMETER_INPUT):
                    self.tabWidget.setTabIcon(pwidget.tab_index, self.tab_icon_off)

                self.tab_icon_timers.append(ICON_STATE_NULL)

    def animateTab(self, index):
        if self.tab_icon_timers[index - 1] == ICON_STATE_NULL:
            self.tabWidget.setTabIcon(index, self.tab_icon_on)
        self.tab_icon_timers[index - 1] = ICON_STATE_ON

    def check_gui_stuff(self):
        # Check Tab icons
        for i in range(len(self.tab_icon_timers)):
            if self.tab_icon_timers[i] == ICON_STATE_ON:
                self.tab_icon_timers[i] = ICON_STATE_WAIT
            elif self.tab_icon_timers[i] == ICON_STATE_WAIT:
                self.tab_icon_timers[i] = ICON_STATE_OFF
            elif self.tab_icon_timers[i] == ICON_STATE_OFF:
                self.tabWidget.setTabIcon(i + 1, self.tab_icon_off)
                self.tab_icon_timers[i] = ICON_STATE_NULL

        # Check parameters needing update
        for parameter_id in self.parameter_list_to_update:
            value = Carla.Host.get_current_parameter_value(self.plugin_id, parameter_id)

            for ptype, pid, pwidget in self.parameter_list:
                if pid == parameter_id:
                    pwidget.set_parameter_value(value, False)

                    if ptype == PARAMETER_INPUT:
                        self.animateTab(pwidget.tab_index)

                    break

        # Clear all parameters
        self.parameter_list_to_update = []

        # Update output parameters
        QTimer.singleShot(0, self, SLOT("slot_checkOutputControlParameters()"))

    @pyqtSlot()
    def slot_saveState(self):
        # TODO - LV2 and VST native formats
        if (self.state_filename):
            ask_try = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if (ask_try == QMessageBox.Ok):
                self.saveState_InternalFormat()
            else:
                self.state_filename = None
                self.slot_saveState()

        else:
            file_filter = self.tr("Carla State File (*.carxs)")
            filename_try = QFileDialog.getSaveFileName(self, self.tr("Save Carla State File"), filter=file_filter)

            if (filename_try):
                self.state_filename = filename_try
                self.saveState_InternalFormat()

    @pyqtSlot()
    def slot_loadState(self):
        # TODO - LV2 and VST native formats
        file_filter = self.tr("Carla State File (*.carxs)")
        filename_try = QFileDialog.getOpenFileName(self, self.tr("Open Carla State File"), filter=file_filter)

        if (filename_try):
            self.state_filename = filename_try
            self.loadState_InternalFormat()

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameter_id, value):
        Carla.Host.set_parameter_value(self.plugin_id, parameter_id, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameter_id, channel):
        Carla.Host.set_parameter_midi_channel(self.plugin_id, parameter_id, channel - 1)

    @pyqtSlot(int, int)
    def slot_parameterMidiCcChanged(self, parameter_id, cc_index):
        Carla.Host.set_parameter_midi_cc(self.plugin_id, parameter_id, cc_index)

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        if self.cur_program_index != index:
            Carla.Host.set_program(self.plugin_id, index)
            QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        self.cur_program_index = index

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        if self.cur_midi_program_index != index:
            Carla.Host.set_midi_program(self.plugin_id, index)
            QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))
        self.cur_midi_program_index = index

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        Carla.Host.send_midi_note(self.plugin_id, 0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        Carla.Host.send_midi_note(self.plugin_id, 0, note, 0)

    @pyqtSlot()
    def slot_notesOn(self):
        self.parent().led_midi.setChecked(True)

    @pyqtSlot()
    def slot_notesOff(self):
        self.parent().led_midi.setChecked(False)

    @pyqtSlot()
    def slot_checkInputControlParameters(self):
        for ptype, pid, pwidget in self.parameter_list:
            if ptype == PARAMETER_INPUT:
                if self.pinfo["type"] not in (PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
                    pwidget.set_default_value(Carla.Host.get_default_parameter_value(self.plugin_id, pid))
                pwidget.set_parameter_value(Carla.Host.get_current_parameter_value(self.plugin_id, pid), False)

    @pyqtSlot()
    def slot_checkOutputControlParameters(self):
        for ptype, pid, pwidget in self.parameter_list:
            if (ptype == PARAMETER_OUTPUT):
                pwidget.set_parameter_value(Carla.Host.get_current_parameter_value(self.plugin_id, pid), False)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# (New) Plugin Widget
class PluginWidget(QFrame, ui_carla_plugin.Ui_PluginWidget):
    def __init__(self, parent, plugin_id):
        QFrame.__init__(self, parent)
        self.setupUi(self)

        self.plugin_id = plugin_id

        self.params_total = 0
        self.parameter_activity_timer = None

        self.last_led_ain_state = False
        self.last_led_aout_state = False

        # Fake effect
        self.color_1 = QColor(0, 0, 0, 220)
        self.color_2 = QColor(0, 0, 0, 170)
        self.color_3 = QColor(7, 7, 7, 250)
        self.color_4 = QColor(14, 14, 14, 255)

        self.led_enable.setColor(self.led_enable.BIG_RED)
        self.led_enable.setChecked(False)

        self.led_control.setColor(self.led_control.YELLOW)
        self.led_control.setEnabled(False)

        self.led_midi.setColor(self.led_midi.RED)
        self.led_midi.setEnabled(False)

        self.led_audio_in.setColor(self.led_audio_in.GREEN)
        self.led_audio_in.setEnabled(False)

        self.led_audio_out.setColor(self.led_audio_out.BLUE)
        self.led_audio_out.setEnabled(False)

        self.dial_drywet.setPixmap(3)
        self.dial_vol.setPixmap(3)
        self.dial_b_left.setPixmap(4)
        self.dial_b_right.setPixmap(4)

        #self.dial_drywet.setLabel("Wet")
        #self.dial_vol.setLabel("Vol")
        #self.dial_b_left.setLabel("L")
        #self.dial_b_right.setLabel("R")

        self.dial_drywet.setCustomPaint(self.dial_b_left.CUSTOM_PAINT_CARLA_WET)
        self.dial_vol.setCustomPaint(self.dial_b_right.CUSTOM_PAINT_CARLA_VOL)
        self.dial_b_left.setCustomPaint(self.dial_b_left.CUSTOM_PAINT_CARLA_L)
        self.dial_b_right.setCustomPaint(self.dial_b_right.CUSTOM_PAINT_CARLA_R)

        self.peak_in.setColor(self.peak_in.GREEN)
        self.peak_in.setOrientation(self.peak_in.HORIZONTAL)

        self.peak_out.setColor(self.peak_in.BLUE)
        self.peak_out.setOrientation(self.peak_out.HORIZONTAL)

        audio_count = Carla.Host.get_audio_port_count_info(self.plugin_id)
        midi_count  = Carla.Host.get_midi_port_count_info(self.plugin_id)

        self.peaks_in  = int(audio_count['ins'])
        self.peaks_out = int(audio_count['outs'])

        if self.peaks_in > 2 or PROCESS_MODE == PROCESS_MODE_CONTINUOUS_RACK:
            self.peaks_in = 2

        if self.peaks_out > 2 or PROCESS_MODE == PROCESS_MODE_CONTINUOUS_RACK:
            self.peaks_out = 2

        self.peak_in.setChannels(self.peaks_in)
        self.peak_out.setChannels(self.peaks_out)

        self.pinfo = Carla.Host.get_plugin_info(self.plugin_id)
        self.pinfo["binary"]    = cString(self.pinfo["binary"])
        self.pinfo["name"]      = cString(self.pinfo["name"])
        self.pinfo["label"]     = cString(self.pinfo["label"])
        self.pinfo["maker"]     = cString(self.pinfo["maker"])
        self.pinfo["copyright"] = cString(self.pinfo["copyright"])

        # Set widget page
        if self.pinfo['type'] == PLUGIN_NONE or audio_count['total'] == 0:
            self.stackedWidget.setCurrentIndex(1)
        else:
            self.stackedWidget.setCurrentIndex(0)

        self.label_name.setText(self.pinfo['name'])

        # Enable/disable features
        self.recheck_hints(self.pinfo['hints'])

        # Colorify
        if (self.pinfo['category'] == PLUGIN_CATEGORY_SYNTH):
            self.setWidgetColor(PALETTE_COLOR_WHITE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DELAY):
            self.setWidgetColor(PALETTE_COLOR_ORANGE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_EQ):
            self.setWidgetColor(PALETTE_COLOR_GREEN)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_FILTER):
            self.setWidgetColor(PALETTE_COLOR_BLUE)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_DYNAMICS):
            self.setWidgetColor(PALETTE_COLOR_PINK)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_MODULATOR):
            self.setWidgetColor(PALETTE_COLOR_RED)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_UTILITY):
            self.setWidgetColor(PALETTE_COLOR_YELLOW)
        elif (self.pinfo['category'] == PLUGIN_CATEGORY_OTHER):
            self.setWidgetColor(PALETTE_COLOR_BROWN)
        else:
            self.setWidgetColor(PALETTE_COLOR_NONE)

        if (self.pinfo['hints'] & PLUGIN_IS_SYNTH) > 0 or (midi_count['ins'] > 0 < midi_count['outs']):
            self.led_audio_in.setVisible(False)
        else:
            self.led_midi.setVisible(False)

        self.edit_dialog = PluginEdit(self, self.plugin_id)
        self.edit_dialog.hide()
        self.edit_dialog_geometry = None

        if self.pinfo['hints'] & PLUGIN_HAS_GUI:
            gui_info = Carla.Host.get_gui_info(self.plugin_id)
            self.gui_dialog_type = gui_info['type']

            if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_HWND, GUI_INTERNAL_X11)):
                self.gui_dialog = None
                self.gui_dialog = PluginGUI(self, self.pinfo['name'], gui_info['resizable'])
                self.gui_dialog.hide()
                self.gui_dialog_geometry = None
                self.connect(self.gui_dialog, SIGNAL("finished(int)"), SLOT("slot_guiClosed()"))

                # TODO - display
                Carla.Host.set_gui_data(self.plugin_id, 0, unwrapinstance(self.gui_dialog))

            elif self.gui_dialog_type in (GUI_EXTERNAL_LV2, GUI_EXTERNAL_SUIL, GUI_EXTERNAL_OSC):
                self.gui_dialog = None

            else:
                self.gui_dialog = None
                self.gui_dialog_type = GUI_NONE
                self.b_gui.setEnabled(False)

        else:
            self.gui_dialog = None
            self.gui_dialog_type = GUI_NONE

        self.connect(self.led_enable, SIGNAL("clicked(bool)"), SLOT("slot_setActive(bool)"))
        self.connect(self.dial_drywet, SIGNAL("sliderMoved(int)"), SLOT("slot_setDryWet(int)"))
        self.connect(self.dial_vol, SIGNAL("sliderMoved(int)"), SLOT("slot_setVolume(int)"))
        self.connect(self.dial_b_left, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceLeft(int)"))
        self.connect(self.dial_b_right, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceRight(int)"))
        self.connect(self.b_gui, SIGNAL("clicked(bool)"), SLOT("slot_guiClicked(bool)"))
        self.connect(self.b_edit, SIGNAL("clicked(bool)"), SLOT("slot_editClicked(bool)"))
        self.connect(self.b_remove, SIGNAL("clicked()"), SLOT("slot_removeClicked()"))

        self.connect(self.dial_drywet, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_vol, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_b_left, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        self.connect(self.dial_b_right, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))

        self.connect(self.edit_dialog, SIGNAL("finished(int)"), SLOT("slot_editClosed()"))

        self.check_gui_stuff()

    def set_active(self, active, gui_send=False, callback_send=True):
        if gui_send: self.led_enable.setChecked(active)
        if callback_send: Carla.Host.set_active(self.plugin_id, active)

    def set_drywet(self, value, gui_send=False, callback_send=True):
        if gui_send: self.dial_drywet.setValue(value)
        if callback_send: Carla.Host.set_drywet(self.plugin_id, float(value) / 1000)

        message = self.tr("Output dry/wet (%s%%)" % (value / 10))
        self.dial_drywet.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_volume(self, value, gui_send=False, callback_send=True):
        if gui_send: self.dial_vol.setValue(value)
        if callback_send: Carla.Host.set_volume(self.plugin_id, float(value) / 1000)

        message = self.tr("Output volume (%s%%)" % (value / 10))
        self.dial_vol.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_balance_left(self, value, gui_send=False, callback_send=True):
        if gui_send: self.dial_b_left.setValue(value)
        if callback_send: Carla.Host.set_balance_left(self.plugin_id, float(value) / 1000)

        if value == 0:
            message = self.tr("Left Panning (Center)")
        elif value < 0:
            message = self.tr("Left Panning (%s%% Left)" % (-value / 10))
        else:
            message = self.tr("Left Panning (%s%% Right)" % (value / 10))

        self.dial_b_left.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_balance_right(self, value, gui_send=False, callback_send=True):
        if gui_send: self.dial_b_right.setValue(value)
        if callback_send: Carla.Host.set_balance_right(self.plugin_id, float(value) / 1000)

        if value == 0:
            message = self.tr("Right Panning (Center)")
        elif value < 0:
            message = self.tr("Right Panning (%s%% Left)" % (-value / 10))
        else:
            message = self.tr("Right Panning (%s%% Right)" % (value / 10))

        self.dial_b_right.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def setWidgetColor(self, color):
        if color == PALETTE_COLOR_WHITE:
            r = 110
            g = 110
            b = 110
            border_r = 72
            border_g = 72
            border_b = 72
        elif color == PALETTE_COLOR_RED:
            r = 110
            g = 15
            b = 15
            border_r = 110
            border_g = 45
            border_b = 45
        elif color == PALETTE_COLOR_GREEN:
            r = 15
            g = 110
            b = 15
            border_r = 72
            border_g = 110
            border_b = 72
        elif color == PALETTE_COLOR_BLUE:
            r = 15
            g = 15
            b = 110
            border_r = 45
            border_g = 45
            border_b = 110
        elif color == PALETTE_COLOR_YELLOW:
            r = 110
            g = 110
            b = 15
            border_r = 110
            border_g = 110
            border_b = 110
        elif color == PALETTE_COLOR_ORANGE:
            r = 180
            g = 110
            b = 15
            border_r = 155
            border_g = 110
            border_b = 60
        elif color == PALETTE_COLOR_BROWN:
            r = 110
            g = 35
            b = 15
            border_r = 110
            border_g = 60
            border_b = 45
        elif color == PALETTE_COLOR_PINK:
            r = 110
            g = 15
            b = 110
            border_r = 110
            border_g = 45
            border_b = 110
        else:
            r = 35
            g = 35
            b = 35
            border_r = 60
            border_g = 60
            border_b = 60

        self.setStyleSheet("""
        QFrame#PluginWidget {
            background-image: url(:/bitmaps/textures/metal_9-512px.jpg);
            background-repeat: repeat-x;
            background-position: top left;
        }
        QLabel#label_name {
            color: white;
        }
        QWidget#widget_buttons {
            background-color: rgb(%i, %i, %i);
            border-left: 1px solid rgb(%i, %i, %i);
            border-right: 1px solid rgb(%i, %i, %i);
            border-bottom: 1px solid rgb(%i, %i, %i);
            border-bottom-left-radius: 3px;
            border-bottom-right-radius: 3px;
        }
        QPushButton#b_gui:hover, QPushButton#b_edit:hover, QPushButton#b_remove:hover {
            background-color: rgb(%i, %i, %i);
            border: 1px solid rgb(%i, %i, %i);
            border-radius: 3px;
        }
        QFrame#frame_name {
            background-color: rgb(%i, %i, %i);
            border: 1px solid rgb(%i, %i, %i);
            border-radius: 4px;
        }
        QFrame#frame_controls {
            background-image: url(:/bitmaps/carla_knobs1.png);
            background-color: rgb(35, 35, 35);
            border: 1px solid rgb(35, 35, 35);
            border-radius: 4px;
        }
        QFrame#frame_peaks {
            background-color: rgb(35, 35, 35);
            border: 1px solid rgb(35, 35, 35);
            border-radius: 4px;
        }
        """ % (
        # QWidget#widget_buttons
        border_r, border_g, border_b,
        border_r, border_g, border_b,
        border_r, border_g, border_b,
        border_r, border_g, border_b,
        # QPushButton#b_*
        r, g, b,
        border_r, border_g, border_b,
        # QFrame#frame_name
        r, g, b,
        border_r, border_g, border_b
        ))

    def recheck_hints(self, hints):
        self.pinfo['hints'] = hints
        self.dial_drywet.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_DRYWET)
        self.dial_vol.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_VOLUME)
        self.dial_b_left.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.dial_b_right.setEnabled(self.pinfo['hints'] & PLUGIN_CAN_BALANCE)
        self.b_gui.setEnabled(self.pinfo['hints'] & PLUGIN_HAS_GUI)

    def getSaveXMLContent(self):
        Carla.Host.prepare_for_save(self.plugin_id)

        if self.pinfo['type'] == PLUGIN_LADSPA:
            type_str = "LADSPA"
        elif self.pinfo['type'] == PLUGIN_DSSI:
            type_str = "DSSI"
        elif self.pinfo['type'] == PLUGIN_LV2:
            type_str = "LV2"
        elif self.pinfo['type'] == PLUGIN_VST:
            type_str = "VST"
        elif self.pinfo['type'] == PLUGIN_GIG:
            type_str = "GIG"
        elif self.pinfo['type'] == PLUGIN_SF2:
            type_str = "SF2"
        elif self.pinfo['type'] == PLUGIN_SFZ:
            type_str = "SFZ"
        else:
            type_str = "Unknown"

        x_save_state_dict = deepcopy(save_state_dict)

        # ----------------------------
        # Basic info

        x_save_state_dict['Type'] = type_str
        x_save_state_dict['Name'] = self.pinfo['name']
        x_save_state_dict['Label'] = self.pinfo['label']
        x_save_state_dict['Binary'] = self.pinfo['binary']
        x_save_state_dict['UniqueID'] = self.pinfo['uniqueId']

        # ----------------------------
        # Internals

        x_save_state_dict['Active'] = self.led_enable.isChecked()
        x_save_state_dict['DryWet'] = float(self.dial_drywet.value()) / 1000
        x_save_state_dict['Volume'] = float(self.dial_vol.value()) / 1000
        x_save_state_dict['Balance-Left'] = float(self.dial_b_left.value()) / 1000
        x_save_state_dict['Balance-Right'] = float(self.dial_b_right.value()) / 1000

        # ----------------------------
        # Current Program

        if (self.edit_dialog.cb_programs.currentIndex() >= 0):
            x_save_state_dict['CurrentProgramIndex'] = self.edit_dialog.cb_programs.currentIndex()
            x_save_state_dict['CurrentProgramName'] = self.edit_dialog.cb_programs.currentText()

        # ----------------------------
        # Current MIDI Program

        if self.edit_dialog.cb_midi_programs.currentIndex() >= 0:
            midi_program_data = Carla.Host.get_midi_program_data(self.plugin_id, self.edit_dialog.cb_midi_programs.currentIndex())
            x_save_state_dict['CurrentMidiBank'] = midi_program_data['bank']
            x_save_state_dict['CurrentMidiProgram'] = midi_program_data['program']

        # ----------------------------
        # Parameters

        parameter_count = Carla.Host.get_parameter_count(self.plugin_id)

        for i in range(parameter_count):
            parameter_info = Carla.Host.get_parameter_info(self.plugin_id, i)
            parameter_data = Carla.Host.get_parameter_data(self.plugin_id, i)

            if parameter_data['type'] != PARAMETER_INPUT:
                continue

            x_save_state_parameter = deepcopy(save_state_parameter)

            x_save_state_parameter['index']  = parameter_data['index']
            x_save_state_parameter['name']   = cString(parameter_info['name'])
            x_save_state_parameter['symbol'] = cString(parameter_info['symbol'])
            x_save_state_parameter['value']  = Carla.Host.get_current_parameter_value(self.plugin_id, parameter_data['index'])
            x_save_state_parameter['midiChannel'] = parameter_data['midiChannel'] + 1
            x_save_state_parameter['midiCC'] = parameter_data['midiCC']

            if (parameter_data['hints'] & PARAMETER_USES_SAMPLERATE):
                x_save_state_parameter['value'] /= Carla.Host.get_sample_rate()

            x_save_state_dict['Parameters'].append(x_save_state_parameter)

        # ----------------------------
        # Custom Data

        custom_data_count = Carla.Host.get_custom_data_count(self.plugin_id)

        for i in range(custom_data_count):
            custom_data = Carla.Host.get_custom_data(self.plugin_id, i)

            if custom_data['type'] == CUSTOM_DATA_INVALID:
                continue

            x_save_state_custom_data = deepcopy(save_state_custom_data)

            x_save_state_custom_data['type']  = CustomDataType2String(custom_data['type'])
            x_save_state_custom_data['key']   = cString(custom_data['key'])
            x_save_state_custom_data['value'] = cString(custom_data['value'])

            x_save_state_dict['CustomData'].append(x_save_state_custom_data)

        # ----------------------------
        # Chunk

        if self.pinfo['hints'] & PLUGIN_USES_CHUNKS:
            x_save_state_dict['Chunk'] = cString(Carla.Host.get_chunk_data(self.plugin_id))

        # ----------------------------
        # Generate XML for this plugin

        content = ""

        content += "  <Info>\n"
        content += "   <Type>%s</Type>\n" % x_save_state_dict['Type']
        content += "   <Name>%s</Name>\n" % xmlSafeString(x_save_state_dict['Name'], True)
        if self.pinfo['type'] == PLUGIN_LV2:
            content += "   <URI>%s</URI>\n" % xmlSafeString(x_save_state_dict['Label'], True)
        else:
            if x_save_state_dict['Label']:
                content += "   <Label>%s</Label>\n" % xmlSafeString(x_save_state_dict['Label'], True)
            content += "   <Binary>%s</Binary>\n" % xmlSafeString(x_save_state_dict['Binary'], True)
            if x_save_state_dict['UniqueID'] != 0:
                content += "   <UniqueID>%li</UniqueID>\n" % x_save_state_dict['UniqueID']
        content += "  </Info>\n"

        content += "\n"
        content += "  <Data>\n"
        content += "   <Active>%s</Active>\n" % "Yes" if x_save_state_dict['Active'] else "No"
        content += "   <DryWet>%f</DryWet>\n" % x_save_state_dict['DryWet']
        content += "   <Volume>%f</Volume>\n" % x_save_state_dict['Volume']
        content += "   <Balance-Left>%f</Balance-Left>\n" % x_save_state_dict['Balance-Left']
        content += "   <Balance-Right>%f</Balance-Right>\n" % x_save_state_dict['Balance-Right']

        for parameter in x_save_state_dict['Parameters']:
            content += "\n"
            content += "   <Parameter>\n"
            content += "    <index>%i</index>\n" % parameter['index']
            content += "    <name>%s</name>\n" % xmlSafeString(parameter['name'], True)
            if (parameter['symbol']):
                content += "    <symbol>%s</symbol>\n" % parameter['symbol']
            content += "    <value>%f</value>\n" % parameter['value']
            if (parameter['midiCC'] > 0):
                content += "    <midiChannel>%i</midiChannel>\n" % parameter['midiChannel']
                content += "    <midiCC>%i</midiCC>\n" % parameter['midiCC']
            content += "   </Parameter>\n"

        if (x_save_state_dict['CurrentProgramIndex'] >= 0):
            content += "\n"
            content += "   <CurrentProgramIndex>%i</CurrentProgramIndex>\n" % x_save_state_dict['CurrentProgramIndex']
            content += "   <CurrentProgramName>%s</CurrentProgramName>\n" % xmlSafeString(x_save_state_dict['CurrentProgramName'], True)

        if (x_save_state_dict['CurrentMidiBank'] >= 0 and x_save_state_dict['CurrentMidiProgram'] >= 0):
            content += "\n"
            content += "   <CurrentMidiBank>%i</CurrentMidiBank>\n" % x_save_state_dict['CurrentMidiBank']
            content += "   <CurrentMidiProgram>%i</CurrentMidiProgram>\n" % x_save_state_dict['CurrentMidiProgram']

        for custom_data in x_save_state_dict['CustomData']:
            content += "\n"
            content += "   <CustomData>\n"
            content += "    <type>%s</type>\n" % custom_data['type']
            content += "    <key>%s</key>\n" % xmlSafeString(custom_data['key'], True)
            if (custom_data['type'] in ("string", "chunk", "binary")):
                content += "    <value>\n"
                content += "%s\n" % xmlSafeString(custom_data['value'], True)
                content += "    </value>\n"
            else:
                content += "    <value>%s</value>\n" % xmlSafeString(custom_data['value'], True)
            content += "   </CustomData>\n"

        if (x_save_state_dict['Chunk']):
            content += "\n"
            content += "   <Chunk>\n"
            content += "%s\n" % x_save_state_dict['Chunk']
            content += "   </Chunk>\n"

        content += "  </Data>\n"

        return content

    def loadStateDict(self, content):
        # ---------------------------------------------------------------------
        # Part 1 - set custom data
        for custom_data in content['CustomData']:
            Carla.Host.set_custom_data(self.plugin_id, custom_data['type'], custom_data['key'], custom_data['value'])

        # ---------------------------------------------------------------------
        # Part 2 - set program
        program_id = -1
        program_count = Carla.Host.get_program_count(self.plugin_id)

        if (content['CurrentProgramName']):
            test_pname = cString(Carla.Host.get_program_name(self.plugin_id, content['CurrentProgramIndex']))

            # Program index and name matches
            if (content['CurrentProgramName'] == test_pname):
                program_id = content['CurrentProgramIndex']

            # index < count
            elif (content['CurrentProgramIndex'] < program_count):
                program_id = content['CurrentProgramIndex']

            # index not valid, try to find by name
            else:
                for i in range(program_count):
                    test_pname = cString(Carla.Host.get_program_name(self.plugin_id, i))

                    if (content['CurrentProgramName'] == test_pname):
                        program_id = i
                        break

            # set program now, if valid
            if (program_id >= 0):
                Carla.Host.set_program(self.plugin_id, program_id)
                self.edit_dialog.set_program(program_id)

        # ---------------------------------------------------------------------
        # Part 3 - set midi program
        if (content['CurrentMidiBank'] >= 0 and content['CurrentMidiProgram'] >= 0):
            midi_program_count = Carla.Host.get_midi_program_count(self.plugin_id)

            for i in range(midi_program_count):
                program_info = Carla.Host.get_midi_program_data(self.plugin_id, i)
                if program_info['bank'] == content['CurrentMidiBank'] and program_info['program'] == content['CurrentMidiProgram']:
                    Carla.Host.set_midi_program(self.plugin_id, i)
                    self.edit_dialog.set_midi_program(i)
                    break

        # ---------------------------------------------------------------------
        # Part 4a - get plugin parameter symbols
        param_symbols = [] # (index, symbol)

        for parameter in content['Parameters']:
            if parameter['symbol']:
                param_info = Carla.Host.get_parameter_info(self.plugin_id, parameter['index'])

                if param_info['symbol']:
                    param_symbols.append((parameter['index'], cString(param_info['symbol'])))

        # ---------------------------------------------------------------------
        # Part 4b - set parameter values (carefully)
        for parameter in content['Parameters']:
            index = -1

            if (content['Type'] == "LADSPA"):
                # Try to set by symbol, otherwise use index
                if (parameter['symbol']):
                    for param_symbol in param_symbols:
                        if (param_symbol[1] == parameter['symbol']):
                            index = param_symbol[0]
                            break
                    else:
                        index = parameter['index']
                else:
                    index = parameter['index']

            elif (content['Type'] == "LV2"):
                # Symbol only
                if (parameter['symbol']):
                    for param_symbol in param_symbols:
                        if (param_symbol[1] == parameter['symbol']):
                            index = param_symbol[0]
                            break
                    else:
                        print("Failed to find LV2 parameter symbol for %i -> %s" % (
                            parameter['index'], parameter['symbol']))
                else:
                    print("LV2 Plugin parameter #%i, '%s', has no symbol" % (parameter['index'], parameter['name']))

            else:
                # Index only
                index = parameter['index']

            # Now set parameter
            if (index >= 0):
                param_data = Carla.Host.get_parameter_data(self.plugin_id, parameter['index'])
                if (param_data['hints'] & PARAMETER_USES_SAMPLERATE):
                    parameter['value'] *= Carla.Host.get_sample_rate()

                Carla.Host.set_parameter_value(self.plugin_id, index, parameter['value'])
                Carla.Host.set_parameter_midi_channel(self.plugin_id, index, parameter['midiChannel'] - 1)
                Carla.Host.set_parameter_midi_cc(self.plugin_id, index, parameter['midiCC'])
            else:
                print("Could not set parameter data for %i -> %s" % (parameter['index'], parameter['name']))

        # ---------------------------------------------------------------------
        # Part 5 - set chunk data
        if (content['Chunk']):
            Carla.Host.set_chunk_data(self.plugin_id, content['Chunk'])

        # ---------------------------------------------------------------------
        # Part 6 - set internal stuff
        self.set_drywet(content['DryWet'] * 1000, True, True)
        self.set_volume(content['Volume'] * 1000, True, True)
        self.set_balance_left(content['Balance-Left'] * 1000, True, True)
        self.set_balance_right(content['Balance-Right'] * 1000, True, True)
        self.edit_dialog.do_reload_all()

        self.set_active(content['Active'], True, True)

    def check_gui_stuff(self):
        # Input peaks
        if (self.peaks_in > 0):
            if (self.peaks_in > 1):
                peak1 = Carla.Host.get_input_peak_value(self.plugin_id, 1)
                peak2 = Carla.Host.get_input_peak_value(self.plugin_id, 2)
                led_ain_state = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_in.displayMeter(1, peak1)
                self.peak_in.displayMeter(2, peak2)

            else:
                peak = Carla.Host.get_input_peak_value(self.plugin_id, 1)
                led_ain_state = bool(peak != 0.0)

                self.peak_in.displayMeter(1, peak)

            if (led_ain_state != self.last_led_ain_state):
                self.led_audio_in.setChecked(led_ain_state)

            self.last_led_ain_state = led_ain_state

        # Output peaks
        if (self.peaks_out > 0):
            if (self.peaks_out > 1):
                peak1 = Carla.Host.get_output_peak_value(self.plugin_id, 1)
                peak2 = Carla.Host.get_output_peak_value(self.plugin_id, 2)
                led_aout_state = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_out.displayMeter(1, peak1)
                self.peak_out.displayMeter(2, peak2)

            else:
                peak = Carla.Host.get_output_peak_value(self.plugin_id, 1)
                led_aout_state = bool(peak != 0.0)

                self.peak_out.displayMeter(1, peak)

            if (led_aout_state != self.last_led_aout_state):
                self.led_audio_out.setChecked(led_aout_state)

            self.last_led_aout_state = led_aout_state

    def check_gui_stuff2(self):
        # Parameter Activity LED
        if (self.parameter_activity_timer == ICON_STATE_ON):
            self.led_control.setChecked(True)
            self.parameter_activity_timer = ICON_STATE_WAIT
        elif (self.parameter_activity_timer == ICON_STATE_WAIT):
            self.parameter_activity_timer = ICON_STATE_OFF
        elif (self.parameter_activity_timer == ICON_STATE_OFF):
            self.led_control.setChecked(False)
            self.parameter_activity_timer = None

        # Update edit dialog
        self.edit_dialog.check_gui_stuff()

    @pyqtSlot(bool)
    def slot_setActive(self, yesno):
        self.set_active(yesno, False, True)

    @pyqtSlot(int)
    def slot_setDryWet(self, value):
        self.set_drywet(value, False, True)

    @pyqtSlot(int)
    def slot_setVolume(self, value):
        self.set_volume(value, False, True)

    @pyqtSlot(int)
    def slot_setBalanceLeft(self, value):
        self.set_balance_left(value, False, True)

    @pyqtSlot(int)
    def slot_setBalanceRight(self, value):
        self.set_balance_right(value, False, True)

    @pyqtSlot(bool)
    def slot_guiClicked(self, show):
        if (self.gui_dialog_type in (GUI_INTERNAL_QT4, GUI_INTERNAL_HWND, GUI_INTERNAL_X11)):
            if (show):
                if (self.gui_dialog_geometry):
                    self.gui_dialog.restoreGeometry(self.gui_dialog_geometry)
            else:
                self.gui_dialog_geometry = self.gui_dialog.saveGeometry()
            self.gui_dialog.setVisible(show)

        Carla.Host.show_gui(self.plugin_id, show)

    @pyqtSlot()
    def slot_guiClosed(self):
        self.b_gui.setChecked(False)

    @pyqtSlot(bool)
    def slot_editClicked(self, show):
        if (show):
            if (self.edit_dialog_geometry):
                self.edit_dialog.restoreGeometry(self.edit_dialog_geometry)
        else:
            self.edit_dialog_geometry = self.edit_dialog.saveGeometry()
        self.edit_dialog.setVisible(show)

    @pyqtSlot()
    def slot_editClosed(self):
        self.b_edit.setChecked(False)

    @pyqtSlot()
    def slot_removeClicked(self):
        Carla.gui.remove_plugin(self.plugin_id, True)

    @pyqtSlot()
    def slot_showCustomDialMenu(self):
        dial_name = self.sender().objectName()
        if dial_name == "dial_drywet":
            minimum = 0
            maximum = 100
            default = 100
            label = "Dry/Wet"
        elif dial_name == "dial_vol":
            minimum = 0
            maximum = 127
            default = 100
            label = "Volume"
        elif dial_name == "dial_b_left":
            minimum = -100
            maximum = 100
            default = -100
            label = "Balance-Left"
        elif dial_name == "dial_b_right":
            minimum = -100
            maximum = 100
            default = 100
            label = "Balance-Right"
        else:
            minimum = 0
            maximum = 100
            default = 100
            label = "Unknown"

        current = self.sender().value() / 10

        menu = QMenu(self)
        act_x_reset = menu.addAction(self.tr("Reset (%i%%)" % (default)))
        menu.addSeparator()
        act_x_min = menu.addAction(self.tr("Set to Minimum (%i%%)" % (minimum)))
        act_x_cen = menu.addAction(self.tr("Set to Center"))
        act_x_max = menu.addAction(self.tr("Set to Maximum (%i%%)" % (maximum)))
        menu.addSeparator()
        act_x_set = menu.addAction(self.tr("Set value..."))

        if (label not in ("Balance-Left", "Balance-Right")):
            menu.removeAction(act_x_cen)

        act_x_sel = menu.exec_(QCursor.pos())

        if act_x_sel == act_x_set:
            value_try = QInputDialog.getInteger(self, self.tr("Set value"), label, current, minimum, maximum, 1)
            if value_try[1]:
                value = value_try[0] * 10
            else:
                value = None

        elif act_x_sel == act_x_min:
            value = minimum * 10
        elif act_x_sel == act_x_max:
            value = maximum * 10
        elif act_x_sel == act_x_reset:
            value = default * 10
        elif act_x_sel == act_x_cen:
            value = 0
        else:
            value = None

        if value != None:
            if label == "Dry/Wet":
                self.set_drywet(value, True, True)
            elif label == "Volume":
                self.set_volume(value, True, True)
            elif label == "Balance-Left":
                self.set_balance_left(value, True, True)
            elif label == "Balance-Right":
                self.set_balance_right(value, True, True)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setPen(self.color_1)
        painter.drawLine(0, 3, self.width(), 3)
        painter.drawLine(0, self.height() - 4, self.width(), self.height() - 4)
        painter.setPen(self.color_2)
        painter.drawLine(0, 2, self.width(), 2)
        painter.drawLine(0, self.height() - 3, self.width(), self.height() - 3)
        painter.setPen(self.color_3)
        painter.drawLine(0, 1, self.width(), 1)
        painter.drawLine(0, self.height() - 2, self.width(), self.height() - 2)
        painter.setPen(self.color_4)
        painter.drawLine(0, 0, self.width(), 0)
        painter.drawLine(0, self.height() - 1, self.width(), self.height() - 1)
        QFrame.paintEvent(self, event)

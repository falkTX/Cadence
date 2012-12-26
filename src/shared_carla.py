#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

import platform
from copy import deepcopy
from decimal import Decimal
from sip import unwrapinstance
from PyQt4.QtCore import pyqtSlot, qFatal, Qt, QSettings, QTimer
from PyQt4.QtGui import QColor, QCursor, QDialog, QFontMetrics, QFrame, QGraphicsScene, QInputDialog, QLinearGradient, QMenu, QPainter, QPainterPath, QVBoxLayout, QWidget
from PyQt4.QtXml import QDomDocument

try:
    from PyQt4.QtGui import QX11EmbedContainer
    GuiContainer = QX11EmbedContainer
except:
    GuiContainer = QWidget

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_about, ui_carla_edit, ui_carla_parameter, ui_carla_plugin
from shared import *

is64bit = bool(platform.architecture()[0] == "64bit" and sys.maxsize > 2**32)

# ------------------------------------------------------------------------------------------------
# Backend defines

# static max values
MAX_PLUGINS    = 99
MAX_PARAMETERS = 200

# group plugin hints
PLUGIN_IS_BRIDGE          = 0x001
PLUGIN_IS_SYNTH           = 0x002
PLUGIN_HAS_GUI            = 0x004
PLUGIN_USES_CHUNKS        = 0x008
PLUGIN_USES_SINGLE_THREAD = 0x010
PLUGIN_CAN_DRYWET         = 0x020
PLUGIN_CAN_VOLUME         = 0x040
PLUGIN_CAN_BALANCE        = 0x080
PLUGIN_CAN_FORCE_STEREO   = 0x100

# group parameter hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# group custom data types
CUSTOM_DATA_INVALID = None
CUSTOM_DATA_CHUNK   = "http://kxstudio.sf.net/ns/carla/chunk"
CUSTOM_DATA_STRING  = "http://kxstudio.sf.net/ns/carla/string"

# enum BinaryType
BINARY_NONE    = 0
BINARY_POSIX32 = 1
BINARY_POSIX64 = 2
BINARY_WIN32   = 3
BINARY_WIN64   = 4
BINARY_OTHER   = 5

# enum PluginType
PLUGIN_NONE     = 0
PLUGIN_INTERNAL = 1
PLUGIN_LADSPA   = 2
PLUGIN_DSSI     = 3
PLUGIN_LV2      = 4
PLUGIN_VST      = 5
PLUGIN_GIG      = 6
PLUGIN_SF2      = 7
PLUGIN_SFZ      = 8

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
PARAMETER_UNKNOWN       = 0
PARAMETER_INPUT         = 1
PARAMETER_OUTPUT        = 2
PARAMETER_LATENCY       = 3
PARAMETER_SAMPLE_RATE   = 4
PARAMETER_LV2_FREEWHEEL = 5
PARAMETER_LV2_TIME      = 6

# enum InternalParametersIndex
PARAMETER_NULL   = -1
PARAMETER_ACTIVE = -2
PARAMETER_DRYWET = -3
PARAMETER_VOLUME = -4
PARAMETER_BALANCE_LEFT  = -5
PARAMETER_BALANCE_RIGHT = -6

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
OPTION_PROCESS_NAME           = 0
OPTION_PROCESS_MODE           = 1
OPTION_PROCESS_HIGH_PRECISION = 2
OPTION_MAX_PARAMETERS         = 3
OPTION_PREFERRED_BUFFER_SIZE  = 4
OPTION_PREFERRED_SAMPLE_RATE  = 5
OPTION_FORCE_STEREO           = 6
OPTION_USE_DSSI_VST_CHUNKS    = 7
OPTION_PREFER_PLUGIN_BRIDGES  = 8
OPTION_PREFER_UI_BRIDGES      = 9
OPTION_OSC_UI_TIMEOUT         = 10
OPTION_PATH_BRIDGE_POSIX32    = 11
OPTION_PATH_BRIDGE_POSIX64    = 12
OPTION_PATH_BRIDGE_WIN32      = 13
OPTION_PATH_BRIDGE_WIN64      = 14
OPTION_PATH_BRIDGE_LV2_GTK2    = 15
OPTION_PATH_BRIDGE_LV2_GTK3    = 16
OPTION_PATH_BRIDGE_LV2_QT4     = 17
OPTION_PATH_BRIDGE_LV2_QT5     = 18
OPTION_PATH_BRIDGE_LV2_COCOA   = 19
OPTION_PATH_BRIDGE_LV2_WINDOWS = 20
OPTION_PATH_BRIDGE_LV2_X11     = 21
OPTION_PATH_BRIDGE_VST_COCOA   = 22
OPTION_PATH_BRIDGE_VST_HWND    = 23
OPTION_PATH_BRIDGE_VST_X11     = 24

# enum CallbackType
CALLBACK_DEBUG                     = 0
CALLBACK_PARAMETER_VALUE_CHANGED   = 1
CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 2
CALLBACK_PARAMETER_MIDI_CC_CHANGED = 3
CALLBACK_PROGRAM_CHANGED           = 4
CALLBACK_MIDI_PROGRAM_CHANGED      = 5
CALLBACK_NOTE_ON           = 6
CALLBACK_NOTE_OFF          = 7
CALLBACK_SHOW_GUI          = 8
CALLBACK_RESIZE_GUI        = 9
CALLBACK_UPDATE            = 10
CALLBACK_RELOAD_INFO       = 11
CALLBACK_RELOAD_PARAMETERS = 12
CALLBACK_RELOAD_PROGRAMS   = 13
CALLBACK_RELOAD_ALL        = 14
CALLBACK_NSM_ANNOUNCE      = 15
CALLBACK_NSM_OPEN1         = 16
CALLBACK_NSM_OPEN2         = 17
CALLBACK_NSM_SAVE          = 18
CALLBACK_ERROR             = 19
CALLBACK_QUIT              = 20

# enum ProcessModeType
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2
PROCESS_MODE_PATCHBAY         = 3

# Set BINARY_NATIVE
if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if is64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if is64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Carla Host object

class CarlaHostObject(object):
    __slots__ = [
        'host',
        'gui',
        'isControl',
        'processMode',
        'maxParameters'
    ]

Carla = CarlaHostObject()
Carla.host = None
Carla.gui  = None
Carla.isControl = False
Carla.processMode   = PROCESS_MODE_CONTINUOUS_RACK
Carla.maxParameters = MAX_PARAMETERS

# ------------------------------------------------------------------------------------------------------------
# Carla GUI stuff

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

CarlaStateParameter = {
    'index': 0,
    'name': "",
    'symbol': "",
    'value': 0.0,
    'midiChannel': 1,
    'midiCC': -1
}

CarlaStateCustomData = {
    'type': CUSTOM_DATA_INVALID,
    'key': "",
    'value': ""
}

CarlaSaveState = {
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

# ------------------------------------------------------------------------------------------------------------
# Carla XML helpers

def getSaveStateDictFromXML(xmlNode):
    saveState = deepcopy(CarlaSaveState)

    node = xmlNode.firstChild()

    while not node.isNull():
        # ------------------------------------------------------
        # Info

        if node.toElement().tagName() == "Info":
            xmlInfo = node.toElement().firstChild()

            while not xmlInfo.isNull():
                tag  = xmlInfo.toElement().tagName()
                text = xmlInfo.toElement().text().strip()

                if tag == "Type":
                    saveState['Type'] = text
                elif tag == "Name":
                    saveState['Name'] = xmlSafeString(text, False)
                elif tag in ("Label", "URI"):
                    saveState['Label'] = xmlSafeString(text, False)
                elif tag == "Binary":
                    saveState['Binary'] = xmlSafeString(text, False)
                elif tag == "UniqueID":
                    if text.isdigit(): saveState['UniqueID'] = int(text)

                xmlInfo = xmlInfo.nextSibling()

        # ------------------------------------------------------
        # Data

        elif node.toElement().tagName() == "Data":
            xmlData = node.toElement().firstChild()

            while not xmlData.isNull():
                tag  = xmlData.toElement().tagName()
                text = xmlData.toElement().text().strip()

                # ----------------------------------------------
                # Internal Data

                if tag == "Active":
                    saveState['Active'] = bool(text == "Yes")
                elif tag == "DryWet":
                    if isNumber(text): saveState['DryWet'] = float(text)
                elif tag == "Volume":
                    if isNumber(text): saveState['Volume'] = float(text)
                elif tag == "Balance-Left":
                    if isNumber(text): saveState['Balance-Left'] = float(text)
                elif tag == "Balance-Right":
                    if isNumber(text): saveState['Balance-Right'] = float(text)

                # ----------------------------------------------
                # Program (current)

                elif tag == "CurrentProgramIndex":
                    if text.isdigit(): saveState['CurrentProgramIndex'] = int(text)
                elif tag == "CurrentProgramName":
                    saveState['CurrentProgramName'] = xmlSafeString(text, False)

                # ----------------------------------------------
                # Midi Program (current)

                elif tag == "CurrentMidiBank":
                    if text.isdigit(): saveState['CurrentMidiBank'] = int(text)
                elif tag == "CurrentMidiProgram":
                    if text.isdigit(): saveState['CurrentMidiProgram'] = int(text)

                # ----------------------------------------------
                # Parameters

                elif tag == "Parameter":
                    stateParameter = deepcopy(CarlaStateParameter)

                    xmlSubData = xmlData.toElement().firstChild()

                    while not xmlSubData.isNull():
                        pTag  = xmlSubData.toElement().tagName()
                        pText = xmlSubData.toElement().text().strip()

                        if pTag == "index":
                            if pText.isdigit(): stateParameter['index'] = int(pText)
                        elif pTag == "name":
                            stateParameter['name'] = xmlSafeString(pText, False)
                        elif pTag == "symbol":
                            stateParameter['symbol'] = xmlSafeString(pText, False)
                        elif pTag == "value":
                            if isNumber(pText): stateParameter['value'] = float(pText)
                        elif pTag == "midiChannel":
                            if pText.isdigit(): stateParameter['midiChannel'] = int(pText)
                        elif pTag == "midiCC":
                            if pText.isdigit(): stateParameter['midiCC'] = int(pText)

                        xmlSubData = xmlSubData.nextSibling()

                    saveState['Parameters'].append(stateParameter)

                # ----------------------------------------------
                # Custom Data

                elif tag == "CustomData":
                    stateCustomData = deepcopy(CarlaStateCustomData)

                    xmlSubData = xmlData.toElement().firstChild()

                    while not xmlSubData.isNull():
                        cTag  = xmlSubData.toElement().tagName()
                        cText = xmlSubData.toElement().text().strip()

                        if cTag == "type":
                            stateCustomData['type'] = xmlSafeString(cText, False)
                        elif cTag == "key":
                            stateCustomData['key'] = xmlSafeString(cText, False)
                        elif cTag == "value":
                            stateCustomData['value'] = xmlSafeString(cText, False)

                        xmlSubData = xmlSubData.nextSibling()

                    saveState['CustomData'].append(stateCustomData)

                # ----------------------------------------------
                # Chunk

                elif tag == "Chunk":
                    saveState['Chunk'] = xmlSafeString(text, False)

                # ----------------------------------------------

                xmlData = xmlData.nextSibling()

        # ------------------------------------------------------

        node = node.nextSibling()

    return saveState

def xmlSafeString(string, toXml):
    if toXml:
        return string.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;")
    else:
        return string.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"")

# ------------------------------------------------------------------------------------------------------------
# CarlaAbout.cpp

class CarlaAboutW(QDialog, ui_carla_about.Ui_CarlaAboutW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.l_about.setText(self.tr(""
                                     "<br>Version %s"
                                     "<br>Carla is a Multi-Plugin Host for JACK%s.<br>"
                                     "<br>Copyright (C) 2011-2012 falkTX<br>"
                                     "" % (VERSION, " - <b>OSC Bridge Version</b>" if Carla.isControl else "")))

        if Carla.isControl:
            self.l_extended.setVisible(False) # TODO - write about this special OSC version
            self.tabWidget.removeTab(1)
            self.tabWidget.removeTab(1)

        else:
            self.l_extended.setText(cString(Carla.host.get_extended_license_text()))
            self.le_osc_url.setText(cString(Carla.host.get_host_osc_url()) if Carla.host.is_engine_running() else self.tr("(Engine not running)"))

            self.l_osc_cmds.setText(
                                    " /set_active                 <i-value>\n"
                                    " /set_drywet                 <f-value>\n"
                                    " /set_volume                 <f-value>\n"
                                    " /set_balance_left           <f-value>\n"
                                    " /set_balance_right          <f-value>\n"
                                    " /set_parameter_value        <i-index> <f-value>\n"
                                    " /set_parameter_midi_cc      <i-index> <i-cc>\n"
                                    " /set_parameter_midi_channel <i-index> <i-channel>\n"
                                    " /set_program                <i-index>\n"
                                    " /set_midi_program           <i-index>\n"
                                    " /note_on                    <i-note> <i-velo>\n"
                                    " /note_off                   <i-note>\n"
                                  )

            self.l_example.setText("/Carla/2/set_parameter_value 5 1.0")
            self.l_example_help.setText("<i>(as in this example, \"2\" is the plugin number and \"5\" the parameter)</i>")

            self.l_ladspa.setText(self.tr("Everything! (Including LRDF)"))
            self.l_dssi.setText(self.tr("Everything! (Including CustomData/Chunks)"))
            self.l_lv2.setText(self.tr("About 95&#37; complete (using custom extensions).<br/>"
                                      "Implemented Feature/Extensions:"
                                      "<ul>"
                                      "<li>http://lv2plug.in/ns/ext/atom</li>"
                                      "<li>http://lv2plug.in/ns/ext/buf-size</li>"
                                      "<li>http://lv2plug.in/ns/ext/data-access</li>"
                                      #"<li>http://lv2plug.in/ns/ext/dynmanifest</li>"
                                      "<li>http://lv2plug.in/ns/ext/event</li>"
                                      "<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                      "<li>http://lv2plug.in/ns/ext/log</li>"
                                      "<li>http://lv2plug.in/ns/ext/midi</li>"
                                      "<li>http://lv2plug.in/ns/ext/options</li>"
                                      #"<li>http://lv2plug.in/ns/ext/parameters</li>"
                                      "<li>http://lv2plug.in/ns/ext/patch</li>"
                                      #"<li>http://lv2plug.in/ns/ext/port-groups</li>"
                                      "<li>http://lv2plug.in/ns/ext/port-props</li>"
                                      #"<li>http://lv2plug.in/ns/ext/presets</li>"
                                      "<li>http://lv2plug.in/ns/ext/state</li>"
                                      "<li>http://lv2plug.in/ns/ext/time</li>"
                                      "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                      "<li>http://lv2plug.in/ns/ext/urid</li>"
                                      "<li>http://lv2plug.in/ns/ext/worker</li>"
                                      "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                      "<li>http://lv2plug.in/ns/extensions/units</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/external-ui</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/rtmempool</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                      "</ul>"))
            self.l_vst.setText(self.tr("<p>About 85&#37; complete (missing vst bank/presets and some minor stuff)</p>"))

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# PluginParameter.cpp

# Plugin Parameter
class PluginParameter(QWidget, ui_carla_parameter.Ui_PluginParameter):
    def __init__(self, parent, pInfo, pluginId, tabIndex):
        QWidget.__init__(self, parent)
        self.setupUi(self)

        pType  = pInfo['type']
        pHints = pInfo['hints']

        self.m_midiCC      = -1
        self.m_midiChannel = 1
        self.m_pluginId    = pluginId
        self.m_parameterId = pInfo['index']
        self.m_tabIndex    = tabIndex

        self.label.setText(pInfo['name'])

        for MIDI_CC in MIDI_CC_LIST:
            self.combo.addItem(MIDI_CC)

        if pType == PARAMETER_INPUT:
            self.widget.set_minimum(pInfo['minimum'])
            self.widget.set_maximum(pInfo['maximum'])
            self.widget.set_default(pInfo['default'])
            self.widget.set_value(pInfo['current'], False)
            self.widget.set_label(pInfo['unit'])
            self.widget.set_step(pInfo['step'])
            self.widget.set_step_small(pInfo['stepSmall'])
            self.widget.set_step_large(pInfo['stepLarge'])
            self.widget.set_scalepoints(pInfo['scalepoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

            if not pHints & PARAMETER_IS_ENABLED:
                self.widget.set_read_only(True)
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

            elif not pHints & PARAMETER_IS_AUTOMABLE:
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        elif pType == PARAMETER_OUTPUT:
            self.widget.set_minimum(pInfo['minimum'])
            self.widget.set_maximum(pInfo['maximum'])
            self.widget.set_value(pInfo['current'], False)
            self.widget.set_label(pInfo['unit'])
            self.widget.set_read_only(True)

            if not pHints & PARAMETER_IS_AUTOMABLE:
                self.combo.setEnabled(False)
                self.sb_channel.setEnabled(False)

        else:
            self.widget.setVisible(False)
            self.combo.setVisible(False)
            self.sb_channel.setVisible(False)

        self.set_parameter_midi_cc(pInfo['midiCC'])
        self.set_parameter_midi_channel(pInfo['midiChannel'])

        self.connect(self.widget, SIGNAL("valueChanged(double)"), SLOT("slot_valueChanged(double)"))
        self.connect(self.sb_channel, SIGNAL("valueChanged(int)"), SLOT("slot_midiChannelChanged(int)"))
        self.connect(self.combo, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiCcChanged(int)"))

        #if force_parameters_style:
        #self.widget.force_plastique_style()

        if pHints & PARAMETER_USES_CUSTOM_TEXT:
            self.widget.set_text_call(self.textCallBack)

        self.widget.updateAll()

    def setDefaultValue(self, value):
        self.widget.set_default(value)

    def set_parameter_value(self, value, send=True):
        self.widget.set_value(value, send)

    def set_parameter_midi_cc(self, cc):
        self.m_midiCC = cc
        self.set_MIDI_CC_in_ComboBox(cc)

    def set_parameter_midi_channel(self, channel):
        self.m_midiChannel = channel+1
        self.sb_channel.setValue(channel+1)

    def set_MIDI_CC_in_ComboBox(self, cc):
        for i in range(len(MIDI_CC_LIST)):
            ccText = MIDI_CC_LIST[i].split(" ")[0]
            if int(ccText, 16) == cc:
                ccIndex = i
                break
        else:
            ccIndex = -1

        self.combo.setCurrentIndex(ccIndex+1)

    def tabIndex(self):
        return self.m_tabIndex

    def textCallBack(self):
        return cString(Carla.host.get_parameter_text(self.m_pluginId, self.m_parameterId))

    @pyqtSlot(float)
    def slot_valueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, double)"), self.m_parameterId, value)

    @pyqtSlot(int)
    def slot_midiCcChanged(self, ccIndex):
        if ccIndex <= 0:
            cc = -1
        else:
            ccText = MIDI_CC_LIST[ccIndex - 1].split(" ")[0]
            cc = int(ccText, 16)

        if self.m_midiCC != cc:
            self.emit(SIGNAL("midiCcChanged(int, int)"), self.m_parameterId, cc)

        self.m_midiCC = cc

    @pyqtSlot(int)
    def slot_midiChannelChanged(self, channel):
        if self.m_midiChannel != channel:
            self.emit(SIGNAL("midiChannelChanged(int, int)"), self.m_parameterId, channel)

        self.m_midiChannel = channel

# Plugin Editor (Built-in)
class PluginEdit(QDialog, ui_carla_edit.Ui_PluginEdit):
    def __init__(self, parent, pluginId):
        QDialog.__init__(self, Carla.gui)
        self.setupUi(self)

        self.m_firstShow  = True
        self.m_geometry   = None
        self.m_pluginId   = pluginId
        self.m_pluginInfo = None
        self.m_realParent = parent

        self.m_parameterCount = 0
        self.m_parameterList  = [] # (type, id, widget)
        self.m_parameterIdsToUpdate = [] # id

        self.m_currentProgram = -1
        self.m_currentMidiProgram = -1
        self.m_currentStateFilename = None

        self.m_tabIconOff = QIcon(":/bitmaps/led_off.png")
        self.m_tabIconOn  = QIcon(":/bitmaps/led_yellow.png")
        self.m_tabIconCount  = 0
        self.m_tabIconTimers = []

        self.keyboard.setMode(self.keyboard.HORIZONTAL)
        self.keyboard.setOctaves(6)
        self.scrollArea.ensureVisible(self.keyboard.width() * 1 / 5, 0)
        self.scrollArea.setVisible(False)

        # TODO - not implemented yet
        self.b_reload_program.setEnabled(False)
        self.b_reload_midi_program.setEnabled(False)

        # Not available for carla-control
        if Carla.isControl:
            self.b_load_state.setEnabled(False)
            self.b_save_state.setEnabled(False)
        else:
            self.connect(self.b_save_state, SIGNAL("clicked()"), SLOT("slot_saveState()"))
            self.connect(self.b_load_state, SIGNAL("clicked()"), SLOT("slot_loadState()"))

        self.connect(self.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))

        self.connect(self.cb_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_programIndexChanged(int)"))
        self.connect(self.cb_midi_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiProgramIndexChanged(int)"))

        self.connect(self, SIGNAL("finished(int)"), SLOT("slot_finished()"))

        self.do_reload_all()

    def do_reload_all(self):
        self.m_pluginInfo = Carla.host.get_plugin_info(self.m_pluginId)
        self.m_pluginInfo["binary"]    = cString(self.m_pluginInfo["binary"])
        self.m_pluginInfo["name"]      = cString(self.m_pluginInfo["name"])
        self.m_pluginInfo["label"]     = cString(self.m_pluginInfo["label"])
        self.m_pluginInfo["maker"]     = cString(self.m_pluginInfo["maker"])
        self.m_pluginInfo["copyright"] = cString(self.m_pluginInfo["copyright"])

        self.do_reload_info()
        self.do_reload_parameters()
        self.do_reload_programs()

    def do_reload_info(self):
        pluginName  = cString(Carla.host.get_real_plugin_name(self.m_pluginId))
        pluginType  = self.m_pluginInfo['type']
        pluginHints = self.m_pluginInfo['hints']

        # Automatically change to MidiProgram tab
        if pluginType != PLUGIN_VST and not self.le_name.text():
            self.tab_programs.setCurrentIndex(1)

        # Set Meta-Data
        if pluginType == PLUGIN_INTERNAL:
            self.le_type.setText(self.tr("Internal"))
        elif pluginType == PLUGIN_LADSPA:
            self.le_type.setText("LADSPA")
        elif pluginType == PLUGIN_DSSI:
            self.le_type.setText("DSSI")
        elif pluginType == PLUGIN_LV2:
            self.le_type.setText("LV2")
        elif pluginType == PLUGIN_VST:
            self.le_type.setText("VST")
        elif pluginType == PLUGIN_GIG:
            self.le_type.setText("GIG")
        elif pluginType == PLUGIN_SF2:
            self.le_type.setText("SF2")
        elif pluginType == PLUGIN_SFZ:
            self.le_type.setText("SFZ")
        else:
            self.le_type.setText(self.tr("Unknown"))

        self.le_name.setText(pluginName)
        self.le_name.setToolTip(pluginName)
        self.le_label.setText(self.m_pluginInfo['label'])
        self.le_label.setToolTip(self.m_pluginInfo['label'])
        self.le_maker.setText(self.m_pluginInfo['maker'])
        self.le_maker.setToolTip(self.m_pluginInfo['maker'])
        self.le_copyright.setText(self.m_pluginInfo['copyright'])
        self.le_copyright.setToolTip(self.m_pluginInfo['copyright'])
        self.le_unique_id.setText(str(self.m_pluginInfo['uniqueId']))
        self.le_unique_id.setToolTip(str(self.m_pluginInfo['uniqueId']))
        self.label_plugin.setText("\n%s\n" % self.m_pluginInfo['name'])
        self.setWindowTitle(self.m_pluginInfo['name'])

        # Set Processing Data
        audioCountInfo = Carla.host.get_audio_port_count_info(self.m_pluginId)
        midiCountInfo  = Carla.host.get_midi_port_count_info(self.m_pluginId)
        paramCountInfo = Carla.host.get_parameter_count_info(self.m_pluginId)

        self.le_ains.setText(str(audioCountInfo['ins']))
        self.le_aouts.setText(str(audioCountInfo['outs']))
        self.le_params.setText(str(paramCountInfo['ins']))
        self.le_couts.setText(str(paramCountInfo['outs']))

        self.le_is_synth.setText(self.tr("Yes") if bool(pluginHints & PLUGIN_IS_SYNTH) else self.tr("No"))
        self.le_has_gui.setText(self.tr("Yes") if bool(pluginHints & PLUGIN_HAS_GUI) else self.tr("No"))

        # Show/hide keyboard
        self.scrollArea.setVisible((pluginHints & PLUGIN_IS_SYNTH) > 0 or (midiCountInfo['ins'] > 0 < midiCountInfo['outs']))

        # Force-Update parent for new hints (knobs)
        self.m_realParent.recheck_hints(pluginHints)

    def do_reload_parameters(self):
        parameterCount = Carla.host.get_parameter_count(self.m_pluginId)

        self.m_parameterList = []
        self.m_parameterIdsToUpdate = []

        self.m_tabIconCount  = 0
        self.m_tabIconTimers = []

        # remove all parameter tabs
        for i in range(self.tabWidget.count()):
            if i == 0: continue
            self.tabWidget.widget(1).deleteLater()
            self.tabWidget.removeTab(1)

        if parameterCount <= 0:
            pass

        elif parameterCount <= Carla.maxParameters:
            paramInputListFull  = []
            paramOutputListFull = []

            paramInputList   = []
            paramInputWidth  = 0
            paramOutputList  = []
            paramOutputWidth = 0

            for i in range(parameterCount):
                paramInfo = Carla.host.get_parameter_info(self.m_pluginId, i)
                paramData = Carla.host.get_parameter_data(self.m_pluginId, i)
                paramRanges = Carla.host.get_parameter_ranges(self.m_pluginId, i)

                if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                    continue

                parameter = {
                    'type':  paramData['type'],
                    'hints': paramData['hints'],
                    'name':  cString(paramInfo['name']),
                    'unit':  cString(paramInfo['unit']),
                    'scalepoints': [],

                    'index':   paramData['index'],
                    'default': paramRanges['def'],
                    'minimum': paramRanges['min'],
                    'maximum': paramRanges['max'],
                    'step':    paramRanges['step'],
                    'stepSmall': paramRanges['stepSmall'],
                    'stepLarge': paramRanges['stepLarge'],
                    'midiCC':    paramData['midiCC'],
                    'midiChannel': paramData['midiChannel'],

                    'current': Carla.host.get_current_parameter_value(self.m_pluginId, i)
                }

                for j in range(paramInfo['scalePointCount']):
                    scalePointInfo = Carla.host.get_parameter_scalepoint_info(self.m_pluginId, i, j)

                    parameter['scalepoints'].append(
                        {
                          'value': scalePointInfo['value'],
                          'label': cString(scalePointInfo['label'])
                        })

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if parameter['type'] == PARAMETER_INPUT:
                    paramInputList.append(parameter)
                    paramInputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramInputWidthTMP > paramInputWidth:
                        paramInputWidth = paramInputWidthTMP

                    if len(paramInputList) == 10:
                        paramInputListFull.append((paramInputList, paramInputWidth))
                        paramInputList  = []
                        paramInputWidth = 0

                elif parameter['type'] == PARAMETER_OUTPUT:
                    paramOutputList.append(parameter)
                    paramOutputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramOutputWidthTMP > paramOutputWidth:
                        paramOutputWidth = paramOutputWidthTMP

                    if len(paramOutputList) == 10:
                        paramOutputListFull.append((paramOutputList, paramOutputWidth))
                        paramOutputList  = []
                        paramOutputWidth = 0

            else:
                # Final page width values
                if 0 < len(paramInputList) < 10:
                    paramInputListFull.append((paramInputList, paramInputWidth))

                if 0 < len(paramOutputList) < 10:
                    paramOutputListFull.append((paramOutputList, paramOutputWidth))

            # -----------------------------------------------------------------
            # Create parameter widgets

            self.createParameterWidgets(PARAMETER_INPUT, paramInputListFull, self.tr("Parameters"))
            self.createParameterWidgets(PARAMETER_OUTPUT, paramOutputListFull, self.tr("Outputs"))

        else: # > Carla.maxParameters
            fakeName = self.tr("This plugin has too many parameters to display here!")

            paramFakeListFull = []
            paramFakeList  = []
            paramFakeWidth = QFontMetrics(self.font()).width(fakeName)

            parameter = {
                'type':  PARAMETER_UNKNOWN,
                'hints': 0,
                'name':  fakeName,
                'unit':  "",
                'scalepoints': [],

                'index':   0,
                'default': 0,
                'minimum': 0,
                'maximum': 0,
                'step':     0,
                'stepSmall': 0,
                'stepLarge': 0,
                'midiCC':   -1,
                'midiChannel': 0,

                'current': 0.0
            }

            paramFakeList.append(parameter)
            paramFakeListFull.append((paramFakeList, paramFakeWidth))

            self.createParameterWidgets(PARAMETER_UNKNOWN, paramFakeListFull, self.tr("Information"))

    def createParameterWidgets(self, paramType, paramListFull, tabPageName):
        i = 1
        for paramList, width in paramListFull:
            if len(paramList) == 0:
                break

            tabPageContainer = QWidget(self.tabWidget)
            tabPageLayout    = QVBoxLayout(tabPageContainer)
            tabPageContainer.setLayout(tabPageLayout)

            for paramInfo in paramList:
                paramWidget = PluginParameter(tabPageContainer, paramInfo, self.m_pluginId, self.tabWidget.count())
                paramWidget.label.setMinimumWidth(width)
                paramWidget.label.setMaximumWidth(width)
                tabPageLayout.addWidget(paramWidget)

                self.m_parameterList.append((paramType, paramInfo['index'], paramWidget))

                if paramType == PARAMETER_INPUT:
                    self.connect(paramWidget, SIGNAL("valueChanged(int, double)"), SLOT("slot_parameterValueChanged(int, double)"))

                self.connect(paramWidget, SIGNAL("midiChannelChanged(int, int)"), SLOT("slot_parameterMidiChannelChanged(int, int)"))
                self.connect(paramWidget, SIGNAL("midiCcChanged(int, int)"), SLOT("slot_parameterMidiCcChanged(int, int)"))

            # FIXME
            tabPageLayout.addStretch()

            self.tabWidget.addTab(tabPageContainer, "%s (%i)" % (tabPageName, i))
            i += 1

            if paramType == PARAMETER_INPUT:
                self.tabWidget.setTabIcon(paramWidget.tabIndex(), self.m_tabIconOff)

            self.m_tabIconTimers.append(ICON_STATE_NULL)

    def do_reload_programs(self):
        # Programs
        self.cb_programs.blockSignals(True)
        self.cb_programs.clear()

        programCount = Carla.host.get_program_count(self.m_pluginId)

        if programCount > 0:
            self.cb_programs.setEnabled(True)

            for i in range(programCount):
                pName = cString(Carla.host.get_program_name(self.m_pluginId, i))
                self.cb_programs.addItem(pName)

            self.m_currentProgram = Carla.host.get_current_program_index(self.m_pluginId)
            self.cb_programs.setCurrentIndex(self.m_currentProgram)

        else:
            self.m_currentProgram = -1
            self.cb_programs.setEnabled(False)

        self.cb_programs.blockSignals(False)

        # MIDI Programs
        self.cb_midi_programs.blockSignals(True)
        self.cb_midi_programs.clear()

        midiProgramCount = Carla.host.get_midi_program_count(self.m_pluginId)

        if midiProgramCount > 0:
            self.cb_midi_programs.setEnabled(True)

            for i in range(midiProgramCount):
                mpData  = Carla.host.get_midi_program_data(self.m_pluginId, i)
                mpBank  = int(mpData['bank'])
                mpProg  = int(mpData['program'])
                mpLabel = cString(mpData['label'])
                self.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank, mpProg, mpLabel))

            self.m_currentMidiProgram = Carla.host.get_current_midi_program_index(self.m_pluginId)
            self.cb_midi_programs.setCurrentIndex(self.m_currentMidiProgram)

        else:
            self.m_currentMidiProgram = -1
            self.cb_midi_programs.setEnabled(False)

        self.cb_midi_programs.blockSignals(False)

    def do_update(self):
        # Update current program text
        if self.cb_programs.count() > 0:
            pIndex = self.cb_programs.currentIndex()
            pName  = cString(Carla.host.get_program_name(self.m_pluginId, pIndex))
            self.cb_programs.setItemText(pIndex, pName)

        # Update current midi program text
        if self.cb_midi_programs.count() > 0:
            mpIndex = self.cb_midi_programs.currentIndex()
            mpData  = Carla.host.get_midi_program_data(self.m_pluginId, mpIndex)
            mpBank  = int(mpData['bank'])
            mpProg  = int(mpData['program'])
            mpLabel = cString(mpData['label'])
            self.cb_midi_programs.setItemText(mpIndex, "%03i:%03i - %s" % (mpBank, mpProg, mpLabel))

        for paramType, paramId, paramWidget in self.m_parameterList:
            paramWidget.set_parameter_value(Carla.host.get_current_parameter_value(self.m_pluginId, paramId), False)
            paramWidget.update()

    def set_parameter_to_update(self, index):
        if index not in self.m_parameterIdsToUpdate:
            self.m_parameterIdsToUpdate.append(index)

    #def set_parameter_default_value(self, index, value):
        #self.m_parameterList[index].setDefaultValue(value)

    def set_parameter_midi_channel(self, index, midiChannel, blockSignals = False):
        for paramType, paramId, paramWidget in self.m_parameterList:
            if paramId == index:
                if (blockSignals): paramWidget.blockSignals(True)
                paramWidget.set_parameter_midi_channel(midiChannel)
                if (blockSignals): paramWidget.blockSignals(False)
                break

    def set_parameter_midi_cc(self, index, midiCC, blockSignals = False):
        for paramType, paramId, paramWidget in self.m_parameterList:
            if paramId == index:
                if (blockSignals): paramWidget.blockSignals(True)
                paramWidget.set_parameter_midi_cc(midiCC)
                if (blockSignals): paramWidget.blockSignals(False)
                break

    def set_program(self, index):
        self.m_currentProgram = index
        self.cb_programs.setCurrentIndex(index)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def set_midi_program(self, index):
        self.m_currentMidiProgram = index
        self.cb_midi_programs.setCurrentIndex(index)
        QTimer.singleShot(0, self, SLOT("slot_checkInputControlParameters()"))

    def saveState(self):
        content  = ""
        content += "<?xml version='1.0' encoding='UTF-8'?>\n"
        content += "<!DOCTYPE CARLA-PRESET>\n"
        content += "<CARLA-PRESET VERSION='%s'>\n" % VERSION
        content += self.m_realParent.getSaveXMLContent()
        content += "</CARLA-PRESET>\n"

        try:
            fd = uopen(self.m_currentStateFilename, "w")
            fd.write(content)
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save state file"))

    def saveStateLV2(self):
        pass

    def saveStateAsVstPreset(self):
        pass

    def loadState(self):
        try:
            fd = uopen(self.m_currentStateFilename, "r")
            stateRead = fd.read()
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load state file"))
            return

        xml = QDomDocument()
        xml.setContent(stateRead.encode("utf-8"))

        xmlNode = xml.documentElement()

        if xmlNode.tagName() != "CARLA-PRESET":
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla state file"))
            return

        saveState = getSaveStateDictFromXML(xmlNode)

        self.m_realParent.loadStateDict(saveState)

    def loadStateLV2(self):
        pass

    def loadStateFromVstPreset(self):
        pass

    def animateTab(self, index):
        if self.m_tabIconTimers[index-1] == ICON_STATE_NULL:
            self.tabWidget.setTabIcon(index, self.m_tabIconOn)

        self.m_tabIconTimers[index-1] = ICON_STATE_ON

    def setVisible(self, yesNo):
        if yesNo:
            if self.m_firstShow:
                self.m_firstShow = False
                self.restoreGeometry("")
            elif self.m_geometry and not self.m_geometry.isNull():
                self.restoreGeometry(self.m_geometry)
        else:
            self.m_geometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

    def updateParametersDefaultValues(self):
        for paramType, paramId, paramWidget in self.m_parameterList:
            if self.m_pluginInfo["type"] not in (PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
                paramWidget.setDefaultValue(Carla.host.get_default_parameter_value(self.m_pluginId, paramId))

    def updateParametersInputs(self):
        for paramType, paramId, paramWidget in self.m_parameterList:
            if paramType == PARAMETER_INPUT:
                paramWidget.set_parameter_value(Carla.host.get_current_parameter_value(self.m_pluginId, paramId), False)

    def updateParametersOutputs(self):
        for paramType, paramId, paramWidget in self.m_parameterList:
            if paramType == PARAMETER_OUTPUT:
                paramWidget.set_parameter_value(Carla.host.get_current_parameter_value(self.m_pluginId, paramId), False)

    def updatePlugin(self):
        # Check Tab icons
        for i in range(len(self.m_tabIconTimers)):
            if self.m_tabIconTimers[i] == ICON_STATE_ON:
                self.m_tabIconTimers[i] = ICON_STATE_WAIT
            elif self.m_tabIconTimers[i] == ICON_STATE_WAIT:
                self.m_tabIconTimers[i] = ICON_STATE_OFF
            elif self.m_tabIconTimers[i] == ICON_STATE_OFF:
                self.m_tabIconTimers[i] = ICON_STATE_NULL
                self.tabWidget.setTabIcon(i+1, self.m_tabIconOff)

        # Check parameters needing update
        for index in self.m_parameterIdsToUpdate:
            value = Carla.host.get_current_parameter_value(self.m_pluginId, index)

            for paramType, paramId, paramWidget in self.m_parameterList:
                if paramId == index:
                    paramWidget.set_parameter_value(value, False)

                    if paramType == PARAMETER_INPUT:
                        self.animateTab(paramWidget.tabIndex())

                    break

        # Clear all parameters
        self.m_parameterIdsToUpdate = []

        # Update output parameters
        self.updateParametersOutputs()

    @pyqtSlot()
    def slot_saveState(self):
        if self.m_pluginInfo['type'] == PLUGIN_LV2:
            # TODO
            QMessageBox.warning(self, self.tr("Warning"), self.tr("LV2 Presets is not implemented yet"))
            return self.saveStateLV2()

        # TODO - VST preset support

        if self.m_currentStateFilename:
            askTry = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if askTry == QMessageBox.Ok:
                return self.saveState()

            self.m_currentStateFilename = None
            self.slot_saveState()

        else:
            fileFilter  = self.tr("Carla State File (*.carxs)")
            filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Plugin State File"), filter=fileFilter)

            if filenameTry:
                if not filenameTry.endswith(".carxs"):
                    filenameTry += ".carxs"

                self.m_currentStateFilename = filenameTry
                self.saveState()

    @pyqtSlot()
    def slot_loadState(self):
        if self.m_pluginInfo['type'] == PLUGIN_LV2:
            # TODO
            QMessageBox.warning(self, self.tr("Warning"), self.tr("LV2 Presets is not implemented yet"))
            return self.loadStateLV2()

        # TODO - VST preset support

        fileFilter  = self.tr("Carla State File (*.carxs)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Plugin State File"), filter=fileFilter)

        if filenameTry:
            self.m_currentStateFilename = filenameTry
            self.loadState()

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameter_id, value):
        Carla.host.set_parameter_value(self.m_pluginId, parameter_id, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameter_id, channel):
        Carla.host.set_parameter_midi_channel(self.m_pluginId, parameter_id, channel-1)

    @pyqtSlot(int, int)
    def slot_parameterMidiCcChanged(self, parameter_id, cc_index):
        Carla.host.set_parameter_midi_cc(self.m_pluginId, parameter_id, cc_index)

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        if self.m_currentProgram != index:
            Carla.host.set_program(self.m_pluginId, index)
            self.updateParametersDefaultValues()
            self.updateParametersInputs()

        self.m_currentProgram = index

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        if self.m_currentMidiProgram != index:
            Carla.host.set_midi_program(self.m_pluginId, index)
            self.updateParametersDefaultValues()
            self.updateParametersInputs()

        self.m_currentMidiProgram = index

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        Carla.host.send_midi_note(self.m_pluginId, 0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        Carla.host.send_midi_note(self.m_pluginId, 0, note, 0)

    @pyqtSlot()
    def slot_notesOn(self):
        self.m_realParent.led_midi.setChecked(True)

    @pyqtSlot()
    def slot_notesOff(self):
        self.m_realParent.led_midi.setChecked(False)

    @pyqtSlot()
    def slot_checkInputControlParameters(self):
        self.updateParametersInputs()

    @pyqtSlot()
    def slot_finished(self):
        self.m_realParent.b_edit.setChecked(False)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# (New) Plugin Widget
class PluginWidget(QFrame, ui_carla_plugin.Ui_PluginWidget):
    def __init__(self, parent, pluginId):
        QFrame.__init__(self, parent)
        self.setupUi(self)

        #self.frame_controls.setVisible(False)

        self.m_pluginId   = pluginId
        self.m_pluginInfo = Carla.host.get_plugin_info(pluginId)
        self.m_pluginInfo["binary"]    = cString(self.m_pluginInfo["binary"])
        self.m_pluginInfo["name"]      = cString(self.m_pluginInfo["name"])
        self.m_pluginInfo["label"]     = cString(self.m_pluginInfo["label"])
        self.m_pluginInfo["maker"]     = cString(self.m_pluginInfo["maker"])
        self.m_pluginInfo["copyright"] = cString(self.m_pluginInfo["copyright"])

        self.m_parameterIconTimer = ICON_STATE_NULL

        self.m_lastGreenLedState = False
        self.m_lastBlueLedState  = False

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            self.m_peaksInputCount  = 2
            self.m_peaksOutputCount = 2
            #self.stackedWidget.setCurrentIndex(0)
        else:
            audioCountInfo = Carla.host.get_audio_port_count_info(self.m_pluginId)

            self.m_peaksInputCount  = int(audioCountInfo['ins'])
            self.m_peaksOutputCount = int(audioCountInfo['outs'])

            if self.m_peaksInputCount > 2:
                self.m_peaksInputCount = 2

            if self.m_peaksOutputCount > 2:
                self.m_peaksOutputCount = 2

            #if audioCountInfo['total'] == 0:
                #self.stackedWidget.setCurrentIndex(1)
            #else:
                #self.stackedWidget.setCurrentIndex(0)

        # Background
        self.m_colorTop    = QColor(60, 60, 60)
        self.m_colorBottom = QColor(47, 47, 47)

        # Colorify
        if self.m_pluginInfo['category'] == PLUGIN_CATEGORY_SYNTH:
            self.setWidgetColor(PALETTE_COLOR_WHITE)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_DELAY:
            self.setWidgetColor(PALETTE_COLOR_ORANGE)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_EQ:
            self.setWidgetColor(PALETTE_COLOR_GREEN)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_FILTER:
            self.setWidgetColor(PALETTE_COLOR_BLUE)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_DYNAMICS:
            self.setWidgetColor(PALETTE_COLOR_PINK)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_MODULATOR:
            self.setWidgetColor(PALETTE_COLOR_RED)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_UTILITY:
            self.setWidgetColor(PALETTE_COLOR_YELLOW)
        elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_OTHER:
            self.setWidgetColor(PALETTE_COLOR_BROWN)
        else:
            self.setWidgetColor(PALETTE_COLOR_NONE)

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

        self.dial_drywet.setCustomPaint(self.dial_drywet.CUSTOM_PAINT_CARLA_WET)
        self.dial_vol.setCustomPaint(self.dial_vol.CUSTOM_PAINT_CARLA_VOL)
        self.dial_b_left.setCustomPaint(self.dial_b_left.CUSTOM_PAINT_CARLA_L)
        self.dial_b_right.setCustomPaint(self.dial_b_right.CUSTOM_PAINT_CARLA_R)

        self.peak_in.setColor(self.peak_in.GREEN)
        self.peak_in.setOrientation(self.peak_in.HORIZONTAL)

        self.peak_out.setColor(self.peak_in.BLUE)
        self.peak_out.setOrientation(self.peak_out.HORIZONTAL)

        self.peak_in.setChannels(self.m_peaksInputCount)
        self.peak_out.setChannels(self.m_peaksOutputCount)

        self.label_name.setText(self.m_pluginInfo['name'])

        self.edit_dialog = PluginEdit(self, self.m_pluginId)
        self.edit_dialog.hide()

        self.gui_dialog = None

        if self.m_pluginInfo['hints'] & PLUGIN_HAS_GUI:
            guiInfo = Carla.host.get_gui_info(self.m_pluginId)
            guiType = guiInfo['type']

            if guiType in (GUI_INTERNAL_QT4, GUI_INTERNAL_COCOA, GUI_INTERNAL_HWND, GUI_INTERNAL_X11):
                self.gui_dialog = PluginGUI(self, self.m_pluginInfo['name'], guiInfo['resizable'])
                self.gui_dialog.hide()

                Carla.host.set_gui_container(self.m_pluginId, unwrapinstance(self.gui_dialog.getContainer()))

            elif guiType in (GUI_EXTERNAL_LV2, GUI_EXTERNAL_SUIL, GUI_EXTERNAL_OSC):
                pass

            else:
                self.b_gui.setEnabled(False)

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

        self.check_gui_stuff()

        # FIXME
        #self.setMaximumHeight(40)

    def set_active(self, active, sendGui=False, sendCallback=True):
        if sendGui:      self.led_enable.setChecked(active)
        if sendCallback: Carla.host.set_active(self.m_pluginId, active)

        if active:
            self.edit_dialog.keyboard.allNotesOff()

    def set_drywet(self, value, sendGui=False, sendCallback=True):
        if sendGui:      self.dial_drywet.setValue(value)
        if sendCallback: Carla.host.set_drywet(self.m_pluginId, float(value)/1000)

        message = self.tr("Output dry/wet (%i%%)" % int(value / 10))
        self.dial_drywet.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_volume(self, value, sendGui=False, sendCallback=True):
        if sendGui:      self.dial_vol.setValue(value)
        if sendCallback: Carla.host.set_volume(self.m_pluginId, float(value)/1000)

        message = self.tr("Output volume (%i%%)" % int(value / 10))
        self.dial_vol.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_balance_left(self, value, sendGui=False, sendCallback=True):
        if sendGui:      self.dial_b_left.setValue(value)
        if sendCallback: Carla.host.set_balance_left(self.m_pluginId, float(value)/1000)

        if value < 0:
            message = self.tr("Left Panning (%i%% Left)" % int(-value / 10))
        elif value > 0:
            message = self.tr("Left Panning (%i%% Right)" % int(value / 10))
        else:
            message = self.tr("Left Panning (Center)")

        self.dial_b_left.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def set_balance_right(self, value, sendGui=False, sendCallback=True):
        if sendGui:      self.dial_b_right.setValue(value)
        if sendCallback: Carla.host.set_balance_right(self.m_pluginId, float(value)/1000)

        if value < 0:
            message = self.tr("Right Panning (%i%% Left)" % int(-value / 10))
        elif value > 0:
            message = self.tr("Right Panning (%i%% Right)" % int(value / 10))
        else:
            message = self.tr("Right Panning (Center)")

        self.dial_b_right.setStatusTip(message)
        Carla.gui.statusBar().showMessage(message)

    def setId(self, idx):
        self.m_pluginId = idx
        self.edit_dialog.m_pluginId = idx

    def setWidgetColor(self, color):
        if color == PALETTE_COLOR_WHITE:
            r = 110
            g = 110
            b = 110
            borderR = 72
            borderG = 72
            borderB = 72
        elif color == PALETTE_COLOR_RED:
            r = 110
            g = 15
            b = 15
            borderR = 110
            borderG = 45
            borderB = 45
        elif color == PALETTE_COLOR_GREEN:
            r = 15
            g = 110
            b = 15
            borderR = 72
            borderG = 110
            borderB = 72
        elif color == PALETTE_COLOR_BLUE:
            r = 15
            g = 15
            b = 110
            borderR = 45
            borderG = 45
            borderB = 110
        elif color == PALETTE_COLOR_YELLOW:
            r = 110
            g = 110
            b = 15
            borderR = 110
            borderG = 110
            borderB = 110
        elif color == PALETTE_COLOR_ORANGE:
            r = 180
            g = 110
            b = 15
            borderR = 155
            borderG = 110
            borderB = 60
        elif color == PALETTE_COLOR_BROWN:
            r = 110
            g = 35
            b = 15
            borderR = 110
            borderG = 60
            borderB = 45
        elif color == PALETTE_COLOR_PINK:
            r = 110
            g = 15
            b = 110
            borderR = 110
            borderG = 45
            borderB = 110
        else:
            r = 35
            g = 35
            b = 35
            borderR = 60
            borderG = 60
            borderB = 60

        #QFrame#PluginWidget {
            #background-image: url(:/bitmaps/textures/metal_9-512px.jpg);
            #background-repeat: repeat-x;
            #background-position: top left;
        #}
        #QWidget#widget_buttons {
            #background-color: rgb(%i, %i, %i);
            #border-left: 1px solid rgb(%i, %i, %i);
            #border-right: 1px solid rgb(%i, %i, %i);
            #border-bottom: 1px solid rgb(%i, %i, %i);
            #border-bottom-left-radius: 3px;
            #border-bottom-right-radius: 3px;
        #}
        #QPushButton#b_gui:hover, QPushButton#b_edit:hover, QPushButton#b_remove:hover {
            #background-color: rgb(%i, %i, %i);
            #border: 1px solid rgb(%i, %i, %i);
            #border-radius: 3px;
        #}
        #QFrame#frame_name {
            #background-color: rgb(%i, %i, %i);
            #border: 1px solid rgb(%i, %i, %i);
            #border-radius: 4px;
        #}
        #QFrame#frame_peaks {
            #background-color: rgb(35, 35, 35);
            #border: 1px solid rgb(35, 35, 35);
            #border-radius: 4px;
        #}
        ## QWidget#widget_buttons
        #borderR, borderG, borderB,
        #borderR, borderG, borderB,
        #borderR, borderG, borderB,
        #borderR, borderG, borderB,
        ## QPushButton#b_*
        #r, g, b,
        #borderR, borderG, borderB,
        ## QFrame#frame_name
        #r, g, b,
        #borderR, borderG, borderB

        self.setStyleSheet("""
        QLabel#label_name {
            color: white;
        }
        QFrame#frame_controls {
            background-image: url(:/bitmaps/carla_knobs1.png);
            background-color: rgb(35, 35, 35);
            border: 1px solid rgb(35, 35, 35);
            border-radius: 4px;
        }
        """)

    def recheck_hints(self, hints):
        self.m_pluginInfo['hints'] = hints
        self.dial_drywet.setEnabled(hints & PLUGIN_CAN_DRYWET)
        self.dial_vol.setEnabled(hints & PLUGIN_CAN_VOLUME)
        self.dial_b_left.setEnabled(hints & PLUGIN_CAN_BALANCE)
        self.dial_b_right.setEnabled(hints & PLUGIN_CAN_BALANCE)
        self.b_gui.setEnabled(hints & PLUGIN_HAS_GUI)

    def getSaveXMLContent(self):
        Carla.host.prepare_for_save(self.m_pluginId)

        if self.m_pluginInfo['type'] == PLUGIN_INTERNAL:
            typeStr = "Internal"
        elif self.m_pluginInfo['type'] == PLUGIN_LADSPA:
            typeStr = "LADSPA"
        elif self.m_pluginInfo['type'] == PLUGIN_DSSI:
            typeStr = "DSSI"
        elif self.m_pluginInfo['type'] == PLUGIN_LV2:
            typeStr = "LV2"
        elif self.m_pluginInfo['type'] == PLUGIN_VST:
            typeStr = "VST"
        elif self.m_pluginInfo['type'] == PLUGIN_GIG:
            typeStr = "GIG"
        elif self.m_pluginInfo['type'] == PLUGIN_SF2:
            typeStr = "SF2"
        elif self.m_pluginInfo['type'] == PLUGIN_SFZ:
            typeStr = "SFZ"
        else:
            typeStr = "Unknown"

        saveState = deepcopy(CarlaSaveState)

        # ----------------------------
        # Basic info

        saveState['Type'] = typeStr
        saveState['Name'] = self.m_pluginInfo['name']
        saveState['Label'] = self.m_pluginInfo['label']
        saveState['Binary'] = self.m_pluginInfo['binary']
        saveState['UniqueID'] = self.m_pluginInfo['uniqueId']

        # ----------------------------
        # Internals

        saveState['Active'] = self.led_enable.isChecked()
        saveState['DryWet'] = float(self.dial_drywet.value()) / 1000
        saveState['Volume'] = float(self.dial_vol.value()) / 1000
        saveState['Balance-Left'] = float(self.dial_b_left.value()) / 1000
        saveState['Balance-Right'] = float(self.dial_b_right.value()) / 1000

        # ----------------------------
        # Current Program

        if self.edit_dialog.cb_programs.currentIndex() >= 0:
            saveState['CurrentProgramIndex'] = self.edit_dialog.cb_programs.currentIndex()
            saveState['CurrentProgramName']  = self.edit_dialog.cb_programs.currentText()

        # ----------------------------
        # Current MIDI Program

        if self.edit_dialog.cb_midi_programs.currentIndex() >= 0:
            midiProgramData = Carla.host.get_midi_program_data(self.m_pluginId, self.edit_dialog.cb_midi_programs.currentIndex())
            saveState['CurrentMidiBank']    = midiProgramData['bank']
            saveState['CurrentMidiProgram'] = midiProgramData['program']

        # ----------------------------
        # Parameters

        sampleRate = Carla.host.get_sample_rate()
        parameterCount = Carla.host.get_parameter_count(self.m_pluginId)

        for i in range(parameterCount):
            parameterInfo = Carla.host.get_parameter_info(self.m_pluginId, i)
            parameterData = Carla.host.get_parameter_data(self.m_pluginId, i)

            if parameterData['type'] != PARAMETER_INPUT:
                continue

            stateParameter = deepcopy(CarlaStateParameter)

            stateParameter['index']  = parameterData['index']
            stateParameter['name']   = cString(parameterInfo['name'])
            stateParameter['symbol'] = cString(parameterInfo['symbol'])
            stateParameter['value']  = Carla.host.get_current_parameter_value(self.m_pluginId, parameterData['index'])
            stateParameter['midiCC'] = parameterData['midiCC']
            stateParameter['midiChannel'] = parameterData['midiChannel'] + 1

            if parameterData['hints'] & PARAMETER_USES_SAMPLERATE:
                stateParameter['value'] /= sampleRate

            saveState['Parameters'].append(stateParameter)

        # ----------------------------
        # Custom Data

        customDataCount = Carla.host.get_custom_data_count(self.m_pluginId)

        for i in range(customDataCount):
            customData = Carla.host.get_custom_data(self.m_pluginId, i)

            if customData['type'] == CUSTOM_DATA_INVALID:
                continue

            stateCustomData = deepcopy(CarlaStateCustomData)

            stateCustomData['type']  = getCustomDataTypeString(customData['type'])
            stateCustomData['key']   = cString(customData['key'])
            stateCustomData['value'] = cString(customData['value'])

            saveState['CustomData'].append(stateCustomData)

        # ----------------------------
        # Chunk

        if self.m_pluginInfo['hints'] & PLUGIN_USES_CHUNKS:
            saveState['Chunk'] = cString(Carla.host.get_chunk_data(self.m_pluginId))

        # ----------------------------
        # Generate XML for this plugin

        content  = ""

        content += "  <Info>\n"
        content += "   <Type>%s</Type>\n" % saveState['Type']
        content += "   <Name>%s</Name>\n" % xmlSafeString(saveState['Name'], True)

        if self.m_pluginInfo['type'] == PLUGIN_LV2:
            content += "   <URI>%s</URI>\n" % xmlSafeString(saveState['Label'], True)

        else:
            if saveState['Label'] and self.m_pluginInfo['type'] not in (PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
                content += "   <Label>%s</Label>\n" % xmlSafeString(saveState['Label'], True)

            content += "   <Binary>%s</Binary>\n" % xmlSafeString(saveState['Binary'], True)

        if self.m_pluginInfo['type'] in (PLUGIN_LADSPA, PLUGIN_VST):
            content += "   <UniqueID>%li</UniqueID>\n" % saveState['UniqueID']

        content += "  </Info>\n"

        content += "\n"
        content += "  <Data>\n"
        content += "   <Active>%s</Active>\n" % "Yes" if saveState['Active'] else "No"
        content += "   <DryWet>%f</DryWet>\n" % saveState['DryWet']
        content += "   <Volume>%f</Volume>\n" % saveState['Volume']
        content += "   <Balance-Left>%f</Balance-Left>\n" % saveState['Balance-Left']
        content += "   <Balance-Right>%f</Balance-Right>\n" % saveState['Balance-Right']

        for parameter in saveState['Parameters']:
            content += "\n"
            content += "   <Parameter>\n"
            content += "    <index>%i</index>\n" % parameter['index']
            content += "    <name>%s</name>\n" % xmlSafeString(parameter['name'], True)

            if parameter['symbol']:
                content += "    <symbol>%s</symbol>\n" % xmlSafeString(parameter['symbol'], True)

            strValue = "{0:f}".format(Decimal("%g" % parameter['value']))
            content += "    <value>%s</value>\n" % strValue

            if parameter['midiCC'] > 0:
                content += "    <midiCC>%i</midiCC>\n" % parameter['midiCC']
                content += "    <midiChannel>%i</midiChannel>\n" % parameter['midiChannel']

            content += "   </Parameter>\n"

        if saveState['CurrentProgramIndex'] >= 0:
            content += "\n"
            content += "   <CurrentProgramIndex>%i</CurrentProgramIndex>\n" % saveState['CurrentProgramIndex']
            content += "   <CurrentProgramName>%s</CurrentProgramName>\n" % xmlSafeString(saveState['CurrentProgramName'], True)

        if saveState['CurrentMidiBank'] >= 0 and saveState['CurrentMidiProgram'] >= 0:
            content += "\n"
            content += "   <CurrentMidiBank>%i</CurrentMidiBank>\n" % saveState['CurrentMidiBank']
            content += "   <CurrentMidiProgram>%i</CurrentMidiProgram>\n" % saveState['CurrentMidiProgram']

        for customData in saveState['CustomData']:
            content += "\n"
            content += "   <CustomData>\n"
            content += "    <type>%s</type>\n" % customData['type']
            content += "    <key>%s</key>\n" % xmlSafeString(customData['key'], True)

            if customData['type'] in ("string", "chunk", "binary"):
                content += "    <value>\n"
                content += "%s\n" % xmlSafeString(customData['value'], True)
                content += "    </value>\n"
            else:
                content += "    <value>%s</value>\n" % xmlSafeString(customData['value'], True)

            content += "   </CustomData>\n"

        if saveState['Chunk']:
            content += "\n"
            content += "   <Chunk>\n"
            content += "%s\n" % saveState['Chunk']
            content += "   </Chunk>\n"

        content += "  </Data>\n"

        return content

    def loadStateDict(self, content):
        # ---------------------------------------------------------------------
        # Part 1 - set custom data (except binary/chunks)

        for customData in content['CustomData']:
            if customData['type'] != CUSTOM_DATA_CHUNK:
                Carla.host.set_custom_data(self.m_pluginId, customData['type'], customData['key'], customData['value'])

        # ---------------------------------------------------------------------
        # Part 2 - set program

        programId = -1

        if content['CurrentProgramName']:
            programCount = Carla.host.get_program_count(self.m_pluginId)
            testProgramName = cString(Carla.host.get_program_name(self.m_pluginId, content['CurrentProgramIndex']))

            # Program name matches
            if content['CurrentProgramName'] == testProgramName:
                programId = content['CurrentProgramIndex']

            # index < count
            elif content['CurrentProgramIndex'] < programCount:
                programId = content['CurrentProgramIndex']

            # index not valid, try to find by name
            else:
                for i in range(programCount):
                    testProgramName = cString(Carla.host.get_program_name(self.m_pluginId, i))

                    if content['CurrentProgramName'] == testProgramName:
                        programId = i
                        break

        # set program now, if valid
        if programId >= 0:
            Carla.host.set_program(self.m_pluginId, programId)
            self.edit_dialog.set_program(programId)

        # ---------------------------------------------------------------------
        # Part 3 - set midi program

        if content['CurrentMidiBank'] >= 0 and content['CurrentMidiProgram'] >= 0:
            midiProgramCount = Carla.host.get_midi_program_count(self.m_pluginId)

            for i in range(midiProgramCount):
                midiProgramData = Carla.host.get_midi_program_data(self.m_pluginId, i)
                if midiProgramData['bank'] == content['CurrentMidiBank'] and midiProgramData['program'] == content['CurrentMidiProgram']:
                    Carla.host.set_midi_program(self.m_pluginId, i)
                    self.edit_dialog.set_midi_program(i)
                    break

        # ---------------------------------------------------------------------
        # Part 4a - get plugin parameter symbols

        paramSymbols = [] # (index, symbol)

        for parameter in content['Parameters']:
            if parameter['symbol']:
                paramInfo = Carla.host.get_parameter_info(self.m_pluginId, parameter['index'])

                if paramInfo['symbol']:
                    paramSymbols.append((parameter['index'], cString(paramInfo['symbol'])))

        # ---------------------------------------------------------------------
        # Part 4b - set parameter values (carefully)

        sampleRate = Carla.host.get_sample_rate()

        for parameter in content['Parameters']:
            index = -1

            if content['Type'] == "LADSPA":
                # Try to set by symbol, otherwise use index
                if parameter['symbol']:
                    for paramIndex, paramSymbol in paramSymbols:
                        if parameter['symbol'] == paramSymbol:
                            index = paramIndex
                            break
                    else:
                        index = parameter['index']
                else:
                    index = parameter['index']

            elif content['Type'] == "LV2":
                # Symbol only
                if parameter['symbol']:
                    for paramIndex, paramSymbol in paramSymbols:
                        if parameter['symbol'] == paramSymbol:
                            index = paramIndex
                            break
                    else:
                        print("Failed to find LV2 parameter symbol for '%s')" % parameter['symbol'])
                else:
                    print("LV2 Plugin parameter '%s' has no symbol" % parameter['name'])

            else:
                # Index only
                index = parameter['index']

            # Now set parameter
            if index >= 0:
                paramData = Carla.host.get_parameter_data(self.m_pluginId, parameter['index'])
                if paramData['hints'] & PARAMETER_USES_SAMPLERATE:
                    parameter['value'] *= sampleRate

                Carla.host.set_parameter_value(self.m_pluginId, index, parameter['value'])
                Carla.host.set_parameter_midi_cc(self.m_pluginId, index, parameter['midiCC'])
                Carla.host.set_parameter_midi_channel(self.m_pluginId, index, parameter['midiChannel']-1)

            else:
                print("Could not set parameter data for '%s')" % parameter['name'])

        # ---------------------------------------------------------------------
        # Part 5 - set chunk data

        for customData in content['CustomData']:
            if customData['type'] == CUSTOM_DATA_CHUNK:
                Carla.host.set_custom_data(self.m_pluginId, customData['type'], customData['key'], customData['value'])

        if content['Chunk']:
            Carla.host.set_chunk_data(self.m_pluginId, content['Chunk'])

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
        if self.m_peaksInputCount > 0:
            if self.m_peaksInputCount > 1:
                peak1 = Carla.host.get_input_peak_value(self.m_pluginId, 1)
                peak2 = Carla.host.get_input_peak_value(self.m_pluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_in.displayMeter(1, peak1)
                self.peak_in.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_input_peak_value(self.m_pluginId, 1)
                ledState = bool(peak != 0.0)

                self.peak_in.displayMeter(1, peak)

            if self.m_lastGreenLedState != ledState:
                self.led_audio_in.setChecked(ledState)

            self.m_lastGreenLedState = ledState

        # Output peaks
        if self.m_peaksOutputCount > 0:
            if self.m_peaksOutputCount > 1:
                peak1 = Carla.host.get_output_peak_value(self.m_pluginId, 1)
                peak2 = Carla.host.get_output_peak_value(self.m_pluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.peak_out.displayMeter(1, peak1)
                self.peak_out.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_output_peak_value(self.m_pluginId, 1)
                ledState = bool(peak != 0.0)

                self.peak_out.displayMeter(1, peak)

            if self.m_lastBlueLedState != ledState:
                self.led_audio_out.setChecked(ledState)

            self.m_lastBlueLedState = ledState

    def check_gui_stuff2(self):
        # Parameter Activity LED
        if self.m_parameterIconTimer == ICON_STATE_ON:
            self.m_parameterIconTimer = ICON_STATE_WAIT
            self.led_control.setChecked(True)
        elif self.m_parameterIconTimer == ICON_STATE_WAIT:
            self.m_parameterIconTimer = ICON_STATE_OFF
        elif self.m_parameterIconTimer == ICON_STATE_OFF:
            self.m_parameterIconTimer = ICON_STATE_NULL
            self.led_control.setChecked(False)

        # Update edit dialog
        self.edit_dialog.updatePlugin()

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
        if self.gui_dialog:
            self.gui_dialog.setVisible(show)

        Carla.host.show_gui(self.m_pluginId, show)

    @pyqtSlot(bool)
    def slot_editClicked(self, show):
        self.edit_dialog.setVisible(show)

    @pyqtSlot()
    def slot_removeClicked(self):
        Carla.gui.remove_plugin(self.m_pluginId, True)

    @pyqtSlot()
    def slot_showCustomDialMenu(self):
        dialName = self.sender().objectName()
        if dialName == "dial_drywet":
            minimum = 0
            maximum = 100
            default = 100
            label   = "Dry/Wet"
        elif dialName == "dial_vol":
            minimum = 0
            maximum = 127
            default = 100
            label   = "Volume"
        elif dialName == "dial_b_left":
            minimum = -100
            maximum = 100
            default = -100
            label   = "Balance-Left"
        elif dialName == "dial_b_right":
            minimum = -100
            maximum = 100
            default = 100
            label   = "Balance-Right"
        else:
            minimum = 0
            maximum = 100
            default = 100
            label   = "Unknown"

        current = self.sender().value() / 10

        menu = QMenu(self)
        actReset = menu.addAction(self.tr("Reset (%i%%)" % default))
        menu.addSeparator()
        actMinimum = menu.addAction(self.tr("Set to Minimum (%i%%)" % minimum))
        actCenter  = menu.addAction(self.tr("Set to Center"))
        actMaximum = menu.addAction(self.tr("Set to Maximum (%i%%)" % maximum))
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        if label not in ("Balance-Left", "Balance-Right"):
            menu.removeAction(actCenter)

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            valueTry = QInputDialog.getInteger(self, self.tr("Set value"), label, current, minimum, maximum, 1)
            if valueTry[1]:
                value = valueTry[0] * 10
            else:
                value = None

        elif actSelected == actMinimum:
            value = minimum * 10
        elif actSelected == actMaximum:
            value = maximum * 10
        elif actSelected == actReset:
            value = default * 10
        elif actSelected == actCenter:
            value = 0
        else:
            return

        if label == "Dry/Wet":
            self.set_drywet(value, True, True)
        elif label == "Volume":
            self.set_volume(value, True, True)
        elif label == "Balance-Left":
            self.set_balance_left(value, True, True)
        elif label == "Balance-Right":
            self.set_balance_right(value, True, True)

    #def mousePressEvent(self, event):
        #Carla.gui.pluginWidgetClicked(self.edit_dialog)
        #QFrame.mousePressEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)

        areaX = self.area_right.x()

        # background
        #painter.setPen(self.m_colorTop)
        #painter.setBrush(self.m_colorTop)
        #painter.drawRect(0, 0, areaX+40, self.height())

        # bottom line
        painter.setPen(self.m_colorBottom)
        painter.setBrush(self.m_colorBottom)
        painter.drawRect(0, self.height()-5, areaX, 5)

        # top line
        painter.drawLine(0, 0, areaX+40, 0)

        # name -> leds arc
        path = QPainterPath()
        path.moveTo(areaX-80, self.height())
        path.cubicTo(areaX+40, self.height()-5, areaX-40, 30, areaX+20, 0)
        path.lineTo(areaX+20, self.height())
        painter.drawPath(path)

        # fill the rest
        painter.drawRect(areaX+20, 0, self.width(), self.height())

        #painter.drawLine(0, 3, self.width(), 3)
        #painter.drawLine(0, self.height() - 4, self.width(), self.height() - 4)
        #painter.setPen(self.m_color2)
        #painter.drawLine(0, 2, self.width(), 2)
        #painter.drawLine(0, self.height() - 3, self.width(), self.height() - 3)
        #painter.setPen(self.m_color3)
        #painter.drawLine(0, 1, self.width(), 1)
        #painter.drawLine(0, self.height() - 2, self.width(), self.height() - 2)
        #painter.setPen(self.m_color4)
        #painter.drawLine(0, 0, self.width(), 0)
        #painter.drawLine(0, self.height() - 1, self.width(), self.height() - 1)
        QFrame.paintEvent(self, event)

# Carla Scene
class CarlaScene(QGraphicsScene):
    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_view = view
        if not self.m_view:
            qFatal("CarlaScene() - invalid view")

        self.m_itemList = []
        for x in range(MAX_PLUGINS):
            self.m_itemList.append(None)

        bgGradient = QLinearGradient(0.0, 0.0, 0.2, 1.0)
        bgGradient.setColorAt(0, QColor(7, 7, 7))
        bgGradient.setColorAt(1, QColor(28, 28, 28))
        self.setBackgroundBrush(bgGradient)

    def addWidget(self, idx, widget):
        newItem = QGraphicsScene.addWidget(self, widget)
        newPosY = 0

        for item in self.m_itemList:
            if item:
                newPosY += item.widget().height()
                print(item.widget().height())

        print("fin", newPosY)
        newItem.setPos(0, newPosY)
        newItem.resize(widget.width(), widget.height())
        self.m_itemList[idx] = newItem

    def resize(self):
        width  = self.m_view.width()
        height = self.m_view.height()
        print("Resize", 0, 0, width, height)
        self.setSceneRect(0, 0, width, height)

        for item in self.m_itemList:
            if not item:
                continue

            item.resize(width, item.widget().height())

# Plugin GUI
class PluginGUI(QDialog):
    def __init__(self, parent, pluginName, resizable):
        QDialog.__init__(self, Carla.gui)

        self.m_firstShow  = True
        self.m_resizable  = resizable
        self.m_geometry   = None
        self.m_realParent = parent

        self.vbLayout = QVBoxLayout(self)
        self.vbLayout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.vbLayout)

        self.container = GuiContainer(self)
        self.vbLayout.addWidget(self.container)

        self.setNewSize(50, 50)
        self.setWindowTitle("%s (GUI)" % pluginName)

        if WINDOWS and not resizable:
            self.setWindowFlags(self.windowFlags() | Qt.MSWindowsFixedSizeDialogHint)

    def getContainer(self):
        return self.container

    def setNewSize(self, width, height):
        if width < 30:
            width = 30
        if height < 30:
            height = 30

        if self.m_resizable:
            self.resize(width, height)
        else:
            self.setFixedSize(width, height)
            self.container.setFixedSize(width, height)

    def setVisible(self, yesNo):
        if yesNo:
            if self.m_firstShow:
                self.m_firstShow = False
                self.restoreGeometry("")
            elif self.m_geometry and not self.m_geometry.isNull():
                self.restoreGeometry(self.m_geometry)
        else:
            self.m_geometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

    def hideEvent(self, event):
        event.accept()
        self.close()

    def closeEvent(self, event):
        if event.spontaneous():
            self.m_realParent.b_gui.setChecked(False)

        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

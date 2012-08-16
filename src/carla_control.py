#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
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
from PyQt4.QtGui import QApplication, QMainWindow
from liblo import make_method, Address, ServerError, ServerThread
from liblo import send as lo_send

# Imports (Custom)
import ui_carla_about, ui_carla_control, ui_carla_edit, ui_carla_parameter, ui_carla_plugin
from shared_carla import *

global carla_name, lo_target
carla_name = ""
lo_target  = None

Carla.isControl = True

# OSC Control server
class ControlServer(ServerThread):
    def __init__(self, parent):
        ServerThread.__init__(self, 8087)

        self.parent = parent

    def get_full_url(self):
        return "%scarla-control" % self.get_url()

    @make_method('/carla-control/add_plugin', 'is')
    def add_plugin_callback(self, path, args):
        pluginId, pluginName = args
        self.parent.emit(SIGNAL("AddPlugin(int, QString)"), pluginId, pluginName)

    @make_method('/carla-control/remove_plugin', 'i')
    def remove_plugin_callback(self, path, args):
        pluginId, = args
        self.parent.emit(SIGNAL("RemovePlugin(int)"), pluginId)

    @make_method('/carla-control/set_plugin_data', 'iiiissssh')
    def set_plugin_data_callback(self, path, args):
        pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId = args
        self.parent.emit(SIGNAL("SetPluginData(int, int, int, int, QString, QString, QString, QString, long)"), pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId)

    @make_method('/carla-control/set_plugin_ports', 'iiiiiiii')
    def set_plugin_ports_callback(self, path, args):
        pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals = args
        self.parent.emit(SIGNAL("SetPluginPorts(int, int, int, int, int, int, int, int)"), pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals)

    @make_method('/carla-control/set_parameter_data', 'iiiissd')
    def set_parameter_data_callback(self, path, args):
        pluginId, index, type_, hints, name, label, current = args
        self.parent.emit(SIGNAL("SetParameterData(int, int, int, int, QString, QString, double)"), pluginId, index, type_, hints, name, label, current)

    @make_method('/carla-control/set_parameter_ranges', 'iidddddd')
    def set_parameter_ranges_callback(self, path, args):
        pluginId, index, min_, max_, def_, step, stepSmall, stepLarge = args
        self.parent.emit(SIGNAL("SetParameterRanges(int, int, double, double, double, double, double, double, double)"), pluginId, index, min_, max_, def_, step, stepSmall, stepLarge)

    @make_method('/carla-control/set_parameter_midi_cc', 'iii')
    def set_parameter_midi_cc_callback(self, path, args):
        pluginId, index, cc = args
        self.parent.emit(SIGNAL("SetParameterMidiCC(int, int, int)"), pluginId, index, cc)

    @make_method('/carla-control/set_parameter_midi_channel', 'iii')
    def set_parameter_midi_channel_callback(self, path, args):
        pluginId, index, channel = args
        self.parent.emit(SIGNAL("SetParameterMidiChannel(int, int, int)"), pluginId, index, channel)

    @make_method('/carla-control/set_parameter_value', 'iid')
    def set_parameter_value_callback(self, path, args):
        pluginId, index, value = args
        self.parent.emit(SIGNAL("SetParameterValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_default_value', 'iid')
    def set_default_value_callback(self, path, args):
        pluginId, index, value = args
        self.parent.emit(SIGNAL("SetDefaultValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_program', 'ii')
    def set_program_callback(self, path, args):
        pluginId, index = args
        self.parent.emit(SIGNAL("SetProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_program_count', 'ii')
    def set_program_count_callback(self, path, args):
        pluginId, count = args
        self.parent.emit(SIGNAL("SetProgramCount(int, int)"), pluginId, count)

    @make_method('/carla-control/set_program_name', 'iis')
    def set_program_name_callback(self, path, args):
        pluginId, index, name = args
        self.parent.emit(SIGNAL("SetProgramName(int, int, QString)"), pluginId, index, name)

    @make_method('/carla-control/set_midi_program', 'ii')
    def set_midi_program_callback(self, path, args):
        pluginId, index = args
        self.parent.emit(SIGNAL("SetMidiProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_midi_program_count', 'ii')
    def set_midi_program_count_callback(self, path, args):
        pluginId, count = args
        self.parent.emit(SIGNAL("SetMidiProgram(int, int)"), pluginId, count)

    @make_method('/carla-control/set_midi_program_data', 'iiiiis')
    def set_midi_program_data_callback(self, path, args):
        pluginId, index, bank, program, name = args
        self.parent.emit(SIGNAL("SetMidiProgramName(int, int, int, int, int, QString)"), pluginId, index, bank, program, name)

    @make_method('/carla-control/set_input_peak_value', 'iid')
    def set_input_peak_value_callback(self, path, args):
        pluginId, portId, value = args
        self.parent.emit(SIGNAL("SetInputPeakValue(int, int, double)"), pluginId, portId, value)

    @make_method('/carla-control/set_output_peak_value', 'iid')
    def set_output_peak_value_callback(self, path, args):
        pluginId, portId, value = args
        self.parent.emit(SIGNAL("SetOutputPeakValue(int, int, double)"), pluginId, portId, value)

    @make_method('/carla-control/note_on', 'iiii')
    def note_on_callback(self, path, args):
        pluginId, channel, note, velo = args
        self.parent.emit(SIGNAL("NoteOn(int, int, int, int)"), pluginId, channel, note, velo)

    @make_method('/carla-control/note_off', 'iii')
    def note_off_callback(self, path, args):
        pluginId, channel, note = args
        self.parent.emit(SIGNAL("NoteOff(int, int, int)"), pluginId, channel, note)

    @make_method('/carla-control/exit', '')
    def exit_callback(self, path, args):
        self.parent.emit(SIGNAL("Exit()"))

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServer::fallback(\"%s\") - unknown message, args =" % path, args)

# Python Object class compatible to 'Host' on the Carla Backend code
class Host(object):
    def __init__(self):
        object.__init__(self)

    def set_active(self, plugin_id, onoff):
        global carla_name, to_target
        lo_path = "/%s/%i/set_active" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, 1 if onoff else 0)

    def set_drywet(self, plugin_id, value):
        global carla_name, to_target
        lo_path = "/%s/%i/set_drywet" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_volume(self, plugin_id, value):
        global carla_name, to_target
        lo_path = "/%s/%i/set_volume" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_balance_left(self, plugin_id, value):
        global carla_name, to_target
        lo_path = "/%s/%i/set_balance_left" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_balance_right(self, plugin_id, value):
        global carla_name, to_target
        lo_path = "/%s/%i/set_balance_right" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_parameter_value(self, plugin_id, parameter_id, value):
        global carla_name, to_target
        lo_path = "/%s/%i/set_parameter_value" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, value)

    def set_parameter_midi_cc(self, plugin_id, parameter_id, midi_cc):
        global carla_name, to_target
        lo_path = "/%s/%i/set_parameter_midi_cc" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, midi_cc)

    def set_parameter_midi_channel(self, plugin_id, parameter_id, channel):
        global carla_name, to_target
        lo_path = "/%s/%i/set_parameter_midi_channel" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, channel)

    def set_program(self, plugin_id, program_id):
        global carla_name, to_target
        lo_path = "/%s/%i/set_program" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, program_id)

    def set_midi_program(self, plugin_id, midi_program_id):
        global carla_name, to_target
        lo_path = "/%s/%i/set_midi_program" % (carla_name, plugin_id)
        lo_send(lo_target, lo_path, midi_program_id)

    def send_midi_note(self, plugin_id, channel, note, velocity):
      global carla_name, to_target
      if velocity:
          lo_path = "/%s/%i/note_on" % (carla_name, plugin_id)
          lo_send(lo_target, lo_path, channel, note, velocity)
      else:
          lo_path = "/%s/%i/note_off" % (carla_name, plugin_id)
          lo_send(lo_target, lo_path, channel, note)

# About Carla Dialog
class AboutW(QDialog, ui_carla_about.Ui_CarlaAboutW):
    def __init__(self, parent=None):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.l_about.setText(self.tr(""
            "<br>Version %1"
            "<br>Carla is a Multi-Plugin Host for JACK - <b>OSC Bridge Version</b>.<br>"
            "<br>Copyright (C) 2011 falkTX<br>"
            "<br><i>VST is a trademark of Steinberg Media Technologies GmbH.</i>"
            "").arg(VERSION))

        self.tabWidget.removeTab(1)
        self.tabWidget.removeTab(1)

# Main Window
class CarlaControlW(QMainWindow, ui_carla_control.Ui_CarlaControlW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Carla-Control")
        self.loadSettings()

        self.lo_address = ""
        self.lo_server  = None

        self.setStyleSheet("""
          QWidget#centralwidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                stop:0 rgb( 7,  7,  7),
                stop:1 rgb(28, 28, 28)
            );
          }
        """)

        self.plugin_list = []
        for x in range(MAX_PLUGINS):
          self.plugin_list.append(None)

        self.act_file_refresh.setEnabled(False)

        self.resize(self.width(), 0)

        #self.connect(self.act_file_connect, SIGNAL("triggered()"), self.do_connect)
        #self.connect(self.act_file_refresh, SIGNAL("triggered()"), self.do_refresh)

        #self.connect(self.act_help_about, SIGNAL("triggered()"), self.aboutCarla)
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        #self.connect(self, SIGNAL("AddPluginCallback(int, QString)"), self.handleAddPluginCallback)
        #self.connect(self, SIGNAL("RemovePluginCallback(int)"), self.handleRemovePluginCallback)
        #self.connect(self, SIGNAL("SetPluginDataCallback(int, int, int, int, QString, QString, QString, QString, long)"), self.handleSetPluginDataCallback)
        #self.connect(self, SIGNAL("SetPluginPortsCallback(int, int, int, int, int, int, int, int)"), self.handleSetPluginPortsCallback)
        #self.connect(self, SIGNAL("SetParameterValueCallback(int, int, double)"), self.handleSetParameterCallback)
        #self.connect(self, SIGNAL("SetParameterDataCallback(int, int, int, int, QString, QString, double, double, double, double, double, double, double)"), self.handleSetParameterDataCallback)
        #self.connect(self, SIGNAL("SetDefaultValueCallback(int, int, double)"), self.handleSetDefaultValueCallback)
        #self.connect(self, SIGNAL("SetInputPeakValueCallback(int, int, double)"), self.handleSetInputPeakValueCallback)
        #self.connect(self, SIGNAL("SetOutputPeakValueCallback(int, int, double)"), self.handleSetOutputPeakValueCallback)
        #self.connect(self, SIGNAL("SetProgramCallback(int, int)"), self.handleSetProgramCallback)
        #self.connect(self, SIGNAL("SetProgramCountCallback(int, int)"), self.handleSetProgramCountCallback)
        #self.connect(self, SIGNAL("SetProgramNameCallback(int, int, QString)"), self.handleSetProgramNameCallback)
        #self.connect(self, SIGNAL("SetMidiProgramCallback(int, int)"), self.handleSetMidiProgramCallback)
        #self.connect(self, SIGNAL("SetMidiProgramCountCallback(int, int)"), self.handleSetMidiProgramCountCallback)
        #self.connect(self, SIGNAL("SetMidiProgramDataCallback(int, int, QString)"), self.handleSetMidiProgramDataCallback)
        #self.connect(self, SIGNAL("NoteOnCallback(int, int, int)"), self.handleNoteOnCallback)
        #self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), self.handleNoteOffCallback)
        #self.connect(self, SIGNAL("ExitCallback()"), self.handleExitCallback)

    #def do_connect(self):
        #global carla_name, lo_target
        #if (lo_target and self.lo_server):
          #url_text = self.lo_address
        #else:
          #url_text = "osc.udp://127.0.0.1:19000/Carla"

        #ans_value = QInputDialog.getText(self, self.tr("Carla Control - Connect"), self.tr("Address"), text=url_text)

        #if (ans_value[1]):
          #if (lo_target and self.lo_server):
            #lo_send(lo_target, "unregister")

          #self.act_file_refresh.setEnabled(True)
          #self.lo_address = QStringStr(ans_value[0])
          #lo_target = Address(self.lo_address)

          #carla_name = self.lo_address.rsplit("/", 1)[1]
          #print "connected to", self.lo_address, "as", carla_name

          #try:
            #self.lo_server = ControlServer(self)
          #except ServerError, err:
            #print str(err)

          #if (self.lo_server):
            #self.lo_server.start()
            #self.do_refresh()
          #else:
            #pass

    #def do_refresh(self):
        #global lo_target
        #if (lo_target and self.lo_server):
          #self.func_remove_all()
          #lo_send(lo_target, "unregister")
          #lo_send(lo_target, "register", self.lo_server.get_full_url())

    #def func_remove_all(self):
        #for i in range(MAX_PLUGINS):
          #if (self.plugin_list[i] != None):
            #self.handleRemovePluginCallback(i)

    #def handleAddPluginCallback(self, plugin_id, plugin_name):
        #pwidget = PluginWidget(self, plugin_id, plugin_name)
        #self.w_plugins.layout().addWidget(pwidget)
        #self.plugin_list[plugin_id] = pwidget

    #def handleRemovePluginCallback(self, plugin_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.close()
          #pwidget.close()
          #pwidget.deleteLater()
          #self.w_plugins.layout().removeWidget(pwidget)
          #self.plugin_list[plugin_id] = None

    #def handleSetPluginDataCallback(self, plugin_id, ptype, category, hints, name, label, maker, copyright, unique_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.set_data(ptype, category, hints, name, label, maker, copyright, unique_id)

    #def handleSetPluginPortsCallback(self, plugin_id, ains, aouts, mins, mouts, cins, couts, ctotals):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.set_ports(ains, aouts, mins, mouts, cins, couts, ctotals)

    #def handleSetParameterCallback(self, plugin_id, parameter_id, value):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #if (parameter_id < 0):
            #pwidget.parameter_activity_timer = ICON_STATE_ON
          #else:
            #for param in pwidget.edit_dialog.parameter_list:
              #if (param[1] == parameter_id):
                #if (param[0] == PARAMETER_INPUT):
                  #pwidget.parameter_activity_timer = ICON_STATE_ON
                #break

          #if (parameter_id == PARAMETER_ACTIVE):
            #pwidget.set_active((value > 0.0), True, False)
          #elif (parameter_id == PARAMETER_DRYWET):
            #pwidget.set_drywet(value*1000, True, False)
          #elif (parameter_id == PARAMETER_VOLUME):
            #pwidget.set_vol(value*1000, True, False)
          #elif (parameter_id == PARAMETER_BALANCE_LEFT):
            #pwidget.set_balance_left(value*1000, True, False)
          #elif (parameter_id == PARAMETER_BALANCE_RIGHT):
            #pwidget.set_balance_right(value*1000, True, False)
          #elif (parameter_id >= 0):
            #pwidget.edit_dialog.set_parameter_value(parameter_id, value)

    #def handleSetParameterDataCallback(self, plugin_id, param_id, ptype, hints, name, label, current, x_min, x_max, x_def, x_step, x_step_small, x_step_large):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_parameter_data(param_id, ptype, hints, name, label, current, x_min, x_max, x_def, x_step, x_step_small, x_step_large)

    #def handleSetDefaultValueCallback(self, plugin_id, param_id, value):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_parameter_default_value(param_id, value)

    #def handleSetInputPeakValueCallback(self, plugin_id, port_id, value):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.set_input_peak_value(port_id, value)

    #def handleSetOutputPeakValueCallback(self, plugin_id, port_id, value):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.set_output_peak_value(port_id, value)

    #def handleSetProgramCallback(self, plugin_id, program_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_program(program_id)

    #def handleSetProgramCountCallback(self, plugin_id, program_count):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_program_count(program_count)

    #def handleSetProgramNameCallback(self, plugin_id, program_id, program_name):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_program_name(program_id, program_name)

    #def handleSetMidiProgramCallback(self, plugin_id, midi_program_id):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_midi_program(midi_program_id)

    #def handleSetMidiProgramCountCallback(self, plugin_id, midi_program_count):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_midi_program_count(midi_program_count)

    #def handleSetMidiProgramDataCallback(self, plugin_id, midi_program_id, bank_id, program_id, midi_program_name):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.set_midi_program_data(midi_program_id, bank_id, program_id, midi_program_name)

    #def handleNoteOnCallback(self, plugin_id, note, velo):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.keyboard.noteOn(note, False)

    #def handleNoteOffCallback(self, plugin_id, note, velo):
        #pwidget = self.plugin_list[plugin_id]
        #if (pwidget):
          #pwidget.edit_dialog.keyboard.noteOff(note, False)

    #def handleExitCallback(self):
        #self.func_remove_all()
        #self.act_file_refresh.setEnabled(False)

        #global carla_name, lo_target
        #carla_name = ""
        #lo_target = None
        #self.lo_address = ""

    #def aboutCarla(self):
        #AboutW(self).exec_()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))

    def closeEvent(self, event):
        self.saveSettings()

        global lo_target
        if lo_target and self.lo_server:
            lo_send(lo_target, "unregister")

        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla-Control")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("falkTX")
    app.setWindowIcon(QIcon(":/48x48/carla-control.png"))

    #style = app.style().metaObject().className()
    #force_parameters_style = (style in ["Bespin::Style"])

    CarlaHost = Host()

    # Show GUI
    Carla.gui = CarlaControlW()
    Carla.gui.show()

    # Set-up custom signal handling
    setUpSignals(Carla.gui)

    # App-Loop
    sys.exit(app.exec_())

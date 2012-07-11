#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK Patchbay
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
from PyQt4.QtCore import QSettings
from PyQt4.QtGui import QApplication, QMainWindow

# Imports (Custom Stuff)
import ui_catia
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

if LINUX:
    for iPATH in PATH:
        if os.path.exists(os.path.join(iPATH, "aconnect")):
            from subprocess import getoutput
            haveALSA = True
            break
    else:
        haveALSA = False
else:
    haveALSA = False

global a2j_client_name
a2j_client_name = None

GROUP_TYPE_NULL = 0
GROUP_TYPE_ALSA = 1
GROUP_TYPE_JACK = 2

iGroupId   = 0
iGroupName = 1
iGroupType = 2

iPortId    = 0
iPortName  = 1
iPortNameR = 2
iPortGroupName = 3

iConnId     = 0
iConnOutput = 1
iConnInput  = 2

# Main Window
class CatiaMainW(QMainWindow, ui_catia.Ui_CatiaMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        self.settings = QSettings("Cadence", "Catia")
        self.loadSettings(True)

        setIcons(self, ["canvas", "jack", "transport", "misc"])

        self.act_quit.setIcon(getIcon("application-exit"))
        self.act_configure.setIcon(getIcon("configure"))

        self.m_group_list = []
        self.m_group_split_list = []
        self.m_port_list = []
        self.m_connection_list = []
        self.m_last_group_id = 1
        self.m_last_port_id  = 1
        self.m_last_connection_id = 1

        self.m_xruns = 0
        self.m_buffer_size = 0
        self.m_sample_rate = 0
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        self.cb_buffer_size.clear()
        self.cb_sample_rate.clear()

        for buffer_size in buffer_sizes:
            self.cb_buffer_size.addItem(str(buffer_size))

        for sample_rate in sample_rates:
            self.cb_sample_rate.addItem(str(sample_rate))

        self.act_jack_bf_list = (self.act_jack_bf_16, self.act_jack_bf_32, self.act_jack_bf_64, self.act_jack_bf_128,
                                 self.act_jack_bf_256, self.act_jack_bf_512, self.act_jack_bf_1024, self.act_jack_bf_2048,
                                 self.act_jack_bf_4096, self.act_jack_bf_8192)

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
        p_features.group_rename = False
        p_features.port_info    = True
        p_features.port_rename  = bool(self.m_savedSettings["Main/JackPortAlias"] > 0)
        p_features.handle_group_pos = True

        patchcanvas.setOptions(p_options)
        patchcanvas.setFeatures(p_features)
        patchcanvas.init(self.scene, self.canvasCallback, DEBUG)

        # Try to connect to jack
        self.jackStarted()

        # DBus checks
        if haveDBus:
            if DBus.jack:
                pass
            else:
                self.act_tools_jack_start.setEnabled(False)
                self.act_tools_jack_stop.setEnabled(False)
                self.act_jack_configure.setEnabled(False)
                self.b_jack_configure.setEnabled(False)

            if DBus.a2j:
                if DBus.a2j.is_started():
                    self.a2jStarted()
                else:
                    self.a2jStopped()
            else:
                self.act_tools_a2j_start.setEnabled(False)
                self.act_tools_a2j_stop.setEnabled(False)
                self.act_tools_a2j_export_hw.setEnabled(False)
                self.menu_A2J_Bridge.setEnabled(False)

        else:
            # No DBus
            self.act_tools_jack_start.setEnabled(False)
            self.act_tools_jack_stop.setEnabled(False)
            self.act_jack_configure.setEnabled(False)
            self.b_jack_configure.setEnabled(False)
            self.act_tools_a2j_start.setEnabled(False)
            self.act_tools_a2j_stop.setEnabled(False)
            self.act_tools_a2j_export_hw.setEnabled(False)
            self.menu_A2J_Bridge.setEnabled(False)

        self.m_timer120 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])
        self.m_timer600 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 5)

        setCanvasConnections(self)
        setJackConnections(self, ["jack", "buffer-size", "transport", "misc"])

        self.connect(self.act_tools_jack_start, SIGNAL("triggered()"), SLOT("slot_JackServerStart()"))
        self.connect(self.act_tools_jack_stop, SIGNAL("triggered()"), SLOT("slot_JackServerStop()"))
        self.connect(self.act_tools_a2j_start, SIGNAL("triggered()"), SLOT("slot_A2JBridgeStart()"))
        self.connect(self.act_tools_a2j_stop, SIGNAL("triggered()"), SLOT("slot_A2JBridgeStop()"))
        self.connect(self.act_tools_a2j_export_hw, SIGNAL("triggered()"), SLOT("slot_A2JBridgeExportHW()"))

        self.connect(self.act_configure, SIGNAL("triggered()"), SLOT("slot_configureCatia()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCatia()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("XRunCallback()"), SLOT("slot_XRunCallback()"))
        self.connect(self, SIGNAL("BufferSizeCallback(int)"), SLOT("slot_BufferSizeCallback(int)"))
        self.connect(self, SIGNAL("SampleRateCallback(int)"), SLOT("slot_SampleRateCallback(int)"))
        self.connect(self, SIGNAL("PortRegistrationCallback(int, bool)"), SLOT("slot_PortRegistrationCallback(int, bool)"))
        self.connect(self, SIGNAL("PortConnectCallback(int, int, bool)"), SLOT("slot_PortConnectCallback(int, int, bool)"))
        self.connect(self, SIGNAL("PortRenameCallback(int, QString, QString)"), SLOT("slot_PortRenameCallback(int, QString, QString)"))
        self.connect(self, SIGNAL("ShutdownCallback()"), SLOT("slot_ShutdownCallback()"))

        if DBus.jack or DBus.a2j:
            DBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword='dest', path_keyword='path',
                member_keyword='member', interface_keyword='interface', sender_keyword='sender', )

    def canvasCallback(self, action, value1, value2, value_str):
        if action == patchcanvas.ACTION_GROUP_INFO:
            pass

        elif action == patchcanvas.ACTION_GROUP_RENAME:
            pass

        elif action == patchcanvas.ACTION_GROUP_SPLIT:
            group_id = value1
            patchcanvas.splitGroup(group_id)

        elif action == patchcanvas.ACTION_GROUP_JOIN:
            group_id = value1
            patchcanvas.joinGroup(group_id)

        elif action == patchcanvas.ACTION_PORT_INFO:
            port_id = value1

            for port in self.m_port_list:
                if port[iPortId] == port_id:
                    port_nameR = port[iPortNameR]
                    port_nameG = port[iPortGroupName]
                    break
            else:
                return

            if port_nameR.startswith("[ALSA-"):
                port_id, port_name = port_nameR.split("] ", 1)[1].split(" ", 1)

                flags = []
                if port_nameR.startswith("[ALSA-Input] "):
                    flags.append(self.tr("Input"))
                elif port_nameR.startswith("[ALSA-Output] "):
                    flags.append(self.tr("Output"))

                flags_text = " | ".join(flags)

                type_text = self.tr("ALSA MIDI")

                info = self.tr(""
                              "<table>"
                              "<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Id:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td colspan='2'>&nbsp;</td></tr>"
                              "<tr><td align='right'><b>Port Flags:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%s</td></tr>"
                              "</table>" % (port_nameG, port_id, port_name, flags_text, type_text))

            else:
                port_ptr   = jacklib.port_by_name(jack.client, port_nameR)
                port_flags = jacklib.port_flags(port_ptr)
                group_name = port_nameR.split(":", 1)[0]

                port_short_name = str(jacklib.port_short_name(port_ptr), encoding="utf-8")

                aliases = jacklib.port_get_aliases(port_ptr)
                if aliases[0] == 1:
                    alias1text = aliases[1]
                    alias2text = "(none)"
                elif aliases[0] == 2:
                    alias1text = aliases[1]
                    alias2text = aliases[2]
                else:
                    alias1text = "(none)"
                    alias2text = "(none)"

                flags = []
                if port_flags & jacklib.JackPortIsInput:
                    flags.append(self.tr("Input"))
                if port_flags & jacklib.JackPortIsOutput:
                    flags.append(self.tr("Output"))
                if port_flags & jacklib.JackPortIsPhysical:
                    flags.append(self.tr("Physical"))
                if port_flags & jacklib.JackPortCanMonitor:
                    flags.append(self.tr("Can Monitor"))
                if port_flags & jacklib.JackPortIsTerminal:
                    flags.append(self.tr("Terminal"))

                flags_text = " | ".join(flags)

                port_type_str = str(jacklib.port_type(port_ptr), encoding="utf-8")
                if port_type_str == jacklib.JACK_DEFAULT_AUDIO_TYPE:
                    type_text = self.tr("JACK Audio")
                elif port_type_str == jacklib.JACK_DEFAULT_MIDI_TYPE:
                    type_text = self.tr("JACK MIDI")
                else:
                    type_text = self.tr("Unknown")

                port_latency = jacklib.port_get_latency(port_ptr)
                port_total_latency = jacklib.port_get_total_latency(jack.client, port_ptr)

                latency_text = self.tr("%.1f ms (%i frames)" % (port_latency * 1000 / self.m_sample_rate, port_latency))
                latency_total_text = self.tr("%.1f ms (%i frames)" % (port_total_latency * 1000 / self.m_sample_rate, port_total_latency))

                info = self.tr(""
                              "<table>"
                              "<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Full Port Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Alias #1:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Alias #2:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td colspan='2'>&nbsp;</td></tr>"
                              "<tr><td align='right'><b>Port Flags:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td colspan='2'>&nbsp;</td></tr>"
                              "<tr><td align='right'><b>Port Latency:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Total Port Latency:</b></td><td>&nbsp;%s</td></tr>"
                              "</table>" % (group_name, port_short_name, port_nameR, alias1text, alias2text, flags_text, type_text, latency_text, latency_total_text))

            QMessageBox.information(self, self.tr("Port Information"), info)

        elif action == patchcanvas.ACTION_PORT_RENAME:
            global a2j_client_name

            port_id = value1
            port_short_name = unicode2ascii(value_str)

            for port in self.m_port_list:
                if port[iPortId] == port_id:
                    port_nameR = port[iPortNameR]

                    if port_nameR.startswith("[ALSA-"):
                        QMessageBox.warning(self, self.tr("Cannot continue"), self.tr("Rename functions rely on JACK aliases and cannot be done in ALSA ports"))
                        return

                    if port_nameR.split(":", 1)[0] == a2j_client_name:
                        a2j_split = port_nameR.split(":", 3)
                        port_name = "%s:%s: %s" % (a2j_split[0], a2j_split[1], port_short_name)
                    else:
                        port_name = "%s:%s" % (port[iPortGroupName], port_short_name)
                    break
            else:
                return

            port_ptr = jacklib.port_by_name(jack.client, port_nameR)
            aliases = jacklib.port_get_aliases(port_ptr)

            if aliases[0] == 2:
                # JACK only allows 2 aliases, remove 2nd
                jacklib.port_unset_alias(port_ptr, aliases[2])

                # If we're going for 1st alias, unset it too
                if self.m_savedSettings["Main/JackPortAlias"] == 1:
                    jacklib.port_unset_alias(port_ptr, aliases[1])

            elif aliases[0] == 1 and self.m_savedSettings["Main/JackPortAlias"] == 1:
                jacklib.port_unset_alias(port_ptr, aliases[1])

            if aliases[0] == 0 and self.m_savedSettings["Main/JackPortAlias"] == 2:
                # If 2nd alias is enabled and port had no previous aliases, set the 1st alias now
                jacklib.port_set_alias(port_ptr, port_name)

            if jacklib.port_set_alias(port_ptr, port_name) == 0:
                patchcanvas.renamePort(port_id, port_short_name)

        elif action == patchcanvas.ACTION_PORTS_CONNECT:
            port_a_id = value1
            port_b_id = value2
            port_a_nameR = ""
            port_b_nameR = ""

            for port in self.m_port_list:
                if port[iPortId] == port_a_id:
                    port_a_nameR = port[iPortNameR]
                if port[iPortId] == port_b_id:
                    port_b_nameR = port[iPortNameR]

            if port_a_nameR.startswith("[ALSA-"):
                port_a_alsa_id = port_a_nameR.split(" ", 2)[1]
                port_b_alsa_id = port_b_nameR.split(" ", 2)[1]

                if os.system("aconnect %s %s" % (port_a_alsa_id, port_b_alsa_id)) == 0:
                    self.canvas_connect_ports(port_a_id, port_b_id)

            elif port_a_nameR and port_b_nameR:
                jacklib.connect(jack.client, port_a_nameR, port_b_nameR)

        elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
            connection_id = value1

            for connection in self.m_connection_list:
                if connection[iConnId] == connection_id:
                    port_a_id = connection[iConnOutput]
                    port_b_id = connection[iConnInput]
                    break
            else:
                return

            port_a_nameR = ""
            port_b_nameR = ""

            for port in self.m_port_list:
                if port[iPortId] == port_a_id:
                    port_a_nameR = port[iPortNameR]
                if port[iPortId] == port_b_id:
                    port_b_nameR = port[iPortNameR]

            if port_a_nameR.startswith("[ALSA-"):
                port_a_alsa_id = port_a_nameR.split(" ", 2)[1]
                port_b_alsa_id = port_b_nameR.split(" ", 2)[1]

                if os.system("aconnect -d %s %s" % (port_a_alsa_id, port_b_alsa_id)) == 0:
                    self.canvas_disconnect_ports(port_a_id, port_b_id)

            elif port_a_nameR and port_b_nameR:
                jacklib.disconnect(jack.client, port_a_nameR, port_b_nameR)

    def init_jack(self):
        self.m_xruns = 0
        self.m_next_sample_rate = 0

        self.m_last_bpm = None
        self.m_last_transport_state = None

        buffer_size = int(jacklib.get_buffer_size(jack.client))
        sample_rate = int(jacklib.get_sample_rate(jack.client))
        realtime = bool(int(jacklib.is_realtime(jack.client)))

        setBufferSize(self, buffer_size)
        setSampleRate(self, sample_rate)
        setRealTime(self, realtime)
        setXruns(self, 0)

        refreshDSPLoad(self)
        refreshTransport(self)

        self.init_callbacks()
        self.init_ports()

        self.scene.zoom_fit()
        self.scene.zoom_reset()

        jacklib.activate(jack.client)

    def init_callbacks(self):
        jacklib.set_buffer_size_callback(jack.client, self.JackBufferSizeCallback, None)
        jacklib.set_sample_rate_callback(jack.client, self.JackSampleRateCallback, None)
        jacklib.set_xrun_callback(jack.client, self.JackXRunCallback, None)
        jacklib.set_port_registration_callback(jack.client, self.JackPortRegistrationCallback, None)
        jacklib.set_port_connect_callback(jack.client, self.JackPortConnectCallback, None)
        jacklib.set_session_callback(jack.client, self.JackSessionCallback, None)
        jacklib.on_shutdown(jack.client, self.JackShutdownCallback, None)

        if jacklib.JACK2:
            jacklib.set_port_rename_callback(jack.client, self.JackPortRenameCallback, None)

    def get_group_id(self, group_name):
        for group in self.m_group_list:
            if group[iGroupName] == group_name:
                return group[iGroupId]
        return -1

    def init_ports(self):
        if not jack.client:
            return

        self.m_group_list = []
        self.m_group_split_list = []
        self.m_port_list = []
        self.m_connection_list = []
        self.m_last_group_id = 1
        self.m_last_port_id = 1
        self.m_last_connection_id = 1

        # Get all jack ports, put a2j ones to the bottom of the list
        a2j_name_list = []
        port_name_list = c_char_p_p_to_list(jacklib.get_ports(jack.client, "", "", 0))

        global a2j_client_name

        h = 0
        for i in range(len(port_name_list)):
            if port_name_list[i - h].split(":")[0] == a2j_client_name:
                port_name = port_name_list.pop(i - h)
                a2j_name_list.append(port_name)
                h += 1

        for a2j_name in a2j_name_list:
            port_name_list.append(a2j_name)

        del a2j_name_list

        # Add jack ports
        for port_name in port_name_list:
            port_ptr = jacklib.port_by_name(jack.client, port_name)
            self.canvas_add_jack_port(port_ptr, port_name)

        # Add jack connections
        for port_name in port_name_list:
            port_ptr = jacklib.port_by_name(jack.client, port_name)

            # Only make connections from an output port
            if jacklib.port_flags(port_ptr) & jacklib.JackPortIsInput:
                continue

            port_connection_names = c_char_p_p_to_list(jacklib.port_get_all_connections(jack.client, port_ptr))

            for port_con_name in port_connection_names:
                self.canvas_connect_ports_by_name(port_name, port_con_name)

        if haveALSA:
            # Get ALSA MIDI ports (outputs)
            output = getoutput("env LANG=C aconnect -i").split("\n")
            last_group_id   = -1
            last_group_name = ""

            for line in output:
                # Make 'System' match JACK's 'system'
                if line == "client 0: 'System' [type=kernel]":
                    line = "client 0: 'system' [type=kernel]"

                if line.startswith("client "):
                    lineSplit  = line.split(": ", 1)
                    lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                    group_id   = int(lineSplit[0].replace("client ", ""))
                    group_name = lineSplit2[0]
                    group_type = lineSplit2[1].rsplit("]", 1)[0]

                    last_group_id = self.get_group_id(group_name)

                    if last_group_id == -1:
                        # Group doesn't exist yet
                        last_group_id = self.canvas_add_alsa_group(group_id, group_name, bool(group_type == "kernel"))

                    last_group_name = group_name

                elif line.startswith("    ") and last_group_id >= 0 and last_group_name:
                    lineSplit = line.split(" '", 1)
                    port_id   = int(lineSplit[0].strip())
                    port_name = lineSplit[1].rsplit("'", 1)[0].strip()

                    self.canvas_add_alsa_port(last_group_id, last_group_name, port_name, "%i:%i %s" % (group_id, port_id, port_name), False)

                else:
                    last_group_id   = -1
                    last_group_name = ""

            # Get ALSA MIDI ports (inputs)
            output = getoutput("env LANG=C aconnect -o").split("\n")
            last_group_id   = -1
            last_group_name = ""

            for line in output:
                # Make 'System' match JACK's 'system'
                if line == "client 0: 'System' [type=kernel]":
                    line = "client 0: 'system' [type=kernel]"

                if line.startswith("client "):
                    lineSplit  = line.split(": ", 1)
                    lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                    group_id   = int(lineSplit[0].replace("client ", ""))
                    group_name = lineSplit2[0]
                    group_type = lineSplit2[1].rsplit("]", 1)[0]

                    last_group_id = self.get_group_id(group_name)

                    if last_group_id == -1:
                        # Group doesn't exist yet
                        last_group_id = self.canvas_add_alsa_group(group_id, group_name, bool(group_type == "kernel"))

                    last_group_name = group_name

                elif line.startswith("    ") and last_group_id >= 0 and last_group_name:
                    lineSplit = line.split(" '", 1)
                    port_id   = int(lineSplit[0].strip())
                    port_name = lineSplit[1].rsplit("'", 1)[0].strip()

                    self.canvas_add_alsa_port(last_group_id, last_group_name, port_name, "%i:%i %s" % (group_id, port_id, port_name), True)

                else:
                    last_group_id   = -1
                    last_group_name = ""

            # Get ALSA MIDI connections
            output = getoutput("env LANG=C aconnect -ol").split("\n")
            last_group_id = -1
            last_port_id  = -1

            for line in output:
                # Make 'System' match JACK's 'system'
                if line == "client 0: 'System' [type=kernel]":
                    line = "client 0: 'system' [type=kernel]"

                if line.startswith("client "):
                    lineSplit  = line.split(": ", 1)
                    lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                    group_id   = int(lineSplit[0].replace("client ", ""))
                    group_name = lineSplit2[0]

                    last_group_id = self.get_group_id(group_name)

                elif line.startswith("    ") and last_group_id >= 0:
                    lineSplit = line.split(" '", 1)
                    port_id   = int(lineSplit[0].strip())
                    port_name = lineSplit[1].rsplit("'", 1)[0].strip()

                    for port in self.m_port_list:
                        if port[iPortNameR] == "[ALSA-Input] %i:%i %s" % (group_id, port_id, port_name):
                            last_port_id = port[iPortId]
                            break
                    else:
                        last_port_id = -1

                elif line.startswith("\tConnect") and last_group_id >= 0 and last_port_id >= 0:
                    if line.startswith("\tConnected From"):
                        lineSplit = line.split(": ", 1)[1]
                        lineConns = lineSplit.split(", ")

                        for lineConn in lineConns:
                            lineConnSplit = lineConn.replace("'","").split(":", 1)
                            alsa_group_id = int(lineConnSplit[0])
                            alsa_port_id  = int(lineConnSplit[1])

                            portNameRtest = "[ALSA-Output] %i:%i " % (alsa_group_id, alsa_port_id)

                            for port in self.m_port_list:
                                if port[iPortNameR].startswith(portNameRtest):
                                    self.canvas_connect_ports(port[iPortId], last_port_id)
                                    break

                else:
                    last_group_id = -1
                    last_port_id  = -1

    def canvas_add_alsa_group(self, alsa_group_id, group_name, hw_split):
        group_id = self.m_last_group_id

        if hw_split:
            patchcanvas.addGroup(group_id, group_name, patchcanvas.SPLIT_YES, patchcanvas.ICON_HARDWARE)
        else:
            patchcanvas.addGroup(group_id, group_name)

        group_obj = [None, None, None]
        group_obj[iGroupId]   = group_id
        group_obj[iGroupName] = group_name
        group_obj[iGroupType] = GROUP_TYPE_ALSA

        self.m_group_list.append(group_obj)
        self.m_last_group_id += 1

        return group_id

    def canvas_add_jack_group(self, group_name):
        group_id = self.m_last_group_id
        patchcanvas.addGroup(group_id, group_name)

        group_obj = [None, None, None]
        group_obj[iGroupId]   = group_id
        group_obj[iGroupName] = group_name
        group_obj[iGroupType] = GROUP_TYPE_JACK

        self.m_group_list.append(group_obj)
        self.m_last_group_id += 1

        return group_id

    def canvas_remove_group(self, group_name):
        group_id = -1
        for group in self.m_group_list:
            if group[iGroupName] == group_name:
                group_id = group[iGroupId]
                self.m_group_list.remove(group)
                break
        else:
            print("Catia - remove group failed")
            return

        patchcanvas.removeGroup(group_id)

    def canvas_add_alsa_port(self, group_id, group_name, port_name, port_nameR, is_port_input):
        port_id   = self.m_last_port_id
        port_mode = patchcanvas.PORT_MODE_INPUT if is_port_input else patchcanvas.PORT_MODE_OUTPUT
        port_type = patchcanvas.PORT_TYPE_MIDI_ALSA

        patchcanvas.addPort(group_id, port_id, port_name, port_mode, port_type)

        port_obj = [None, None, None, None]
        port_obj[iPortId]    = port_id
        port_obj[iPortName]  = port_name
        port_obj[iPortNameR] = "[ALSA-%s] %s" % ("Input" if is_port_input else "Output", port_nameR)
        port_obj[iPortGroupName] = group_name

        self.m_port_list.append(port_obj)
        self.m_last_port_id += 1

        return port_id

    def canvas_add_jack_port(self, port_ptr, port_name):
        global a2j_client_name

        port_id  = self.m_last_port_id
        group_id = -1

        port_nameR = port_name

        alias_n = self.m_savedSettings["Main/JackPortAlias"]
        if alias_n in (1, 2):
            aliases = jacklib.port_get_aliases(port_ptr)
            if aliases[0] == 2 and alias_n == 2:
                port_name = aliases[2]
            elif aliases[0] >= 1 and alias_n == 1:
                port_name = aliases[1]

        port_flags = jacklib.port_flags(port_ptr)
        group_name = port_name.split(":", 1)[0]

        if port_flags & jacklib.JackPortIsInput:
            port_mode = patchcanvas.PORT_MODE_INPUT
        elif port_flags & jacklib.JackPortIsOutput:
            port_mode = patchcanvas.PORT_MODE_OUTPUT
        else:
            port_mode = patchcanvas.PORT_MODE_NULL

        if group_name == a2j_client_name:
            port_type  = patchcanvas.PORT_TYPE_MIDI_A2J
            group_name = port_name.replace("%s:" % a2j_client_name, "", 1).split(" [", 1)[0]
            port_short_name = port_name.split("): ", 1)[1]

        else:
            port_short_name = port_name.replace("%s:" % group_name, "", 1)

            port_type_str = str(jacklib.port_type(port_ptr), encoding="utf-8")
            if port_type_str == jacklib.JACK_DEFAULT_AUDIO_TYPE:
                port_type = patchcanvas.PORT_TYPE_AUDIO_JACK
            elif port_type_str == jacklib.JACK_DEFAULT_MIDI_TYPE:
                port_type = patchcanvas.PORT_TYPE_MIDI_JACK
            else:
                port_type = patchcanvas.PORT_TYPE_NULL

        for group in self.m_group_list:
            if group[iGroupName] == group_name:
                group_id = group[iGroupId]
                break
        else:
            # For ports with no group
            group_id = self.canvas_add_jack_group(group_name)

        patchcanvas.addPort(group_id, port_id, port_short_name, port_mode, port_type)

        port_obj = [None, None, None, None]
        port_obj[iPortId]    = port_id
        port_obj[iPortName]  = port_name
        port_obj[iPortNameR] = port_nameR
        port_obj[iPortGroupName] = group_name

        self.m_port_list.append(port_obj)
        self.m_last_port_id += 1

        if group_id not in self.m_group_split_list and (port_flags & jacklib.JackPortIsPhysical) > 0:
            patchcanvas.splitGroup(group_id)
            patchcanvas.setGroupIcon(group_id, patchcanvas.ICON_HARDWARE)
            self.m_group_split_list.append(group_id)

        return port_id

    def canvas_remove_jack_port(self, port_id):
        patchcanvas.removePort(port_id)

        for port in self.m_port_list:
            if port[iPortId] == port_id:
                group_name = port[iPortGroupName]
                self.m_port_list.remove(port)
                break
        else:
            return

        # Check if group has no more ports; if yes remove it
        for port in self.m_port_list:
            if port[iPortGroupName] == group_name:
                break
        else:
            self.canvas_remove_group(group_name)

    def canvas_rename_port(self, port_id, port_short_name):
        patchcanvas.renamePort(port_id, port_short_name)

    def canvas_connect_ports(self, port_out_id, port_in_id):
        connection_id = self.m_last_connection_id
        patchcanvas.connectPorts(connection_id, port_out_id, port_in_id)

        conn_obj = [None, None, None]
        conn_obj[iConnId]     = connection_id
        conn_obj[iConnOutput] = port_out_id
        conn_obj[iConnInput]  = port_in_id

        self.m_connection_list.append(conn_obj)
        self.m_last_connection_id += 1

        return connection_id

    def canvas_connect_ports_by_name(self, port_out_name, port_in_name):
        port_out_id = -1
        port_in_id  = -1

        for port in self.m_port_list:
            if port[iPortNameR] == port_out_name:
                port_out_id = port[iPortId]
            elif port[iPortNameR] == port_in_name:
                port_in_id = port[iPortId]

        if port_out_id == -1 or port_in_id == -1:
            print("Catia - connect jack ports failed")
            return

        return self.canvas_connect_ports(port_out_id, port_in_id)

    def canvas_disconnect_ports(self, port_out_id, port_in_id):
        for connection in self.m_connection_list:
            if connection[iConnOutput] == port_out_id and connection[iConnInput] == port_in_id:
                patchcanvas.disconnectPorts(connection[iConnId])
                self.m_connection_list.remove(connection)
                break

    def canvas_disconnect_ports_by_name(self, port_out_name, port_in_name):
        port_out_id = -1
        port_in_id  = -1

        for port in self.m_port_list:
            if port[iPortNameR] == port_out_name:
                port_out_id = port[iPortId]
            elif port[iPortNameR] == port_in_name:
                port_in_id = port[iPortId]

        if port_out_id == -1 or port_in_id == -1:
            print("Catia - disconnect ports failed")
            return

        self.canvas_disconnect_ports(port_out_id, port_in_id)

    def jackStarted(self):
        if not jack.client:
            jack.client = jacklib.client_open("catia", jacklib.JackNoStartServer | jacklib.JackSessionID, None)
            if not jack.client:
                return self.jackStopped()

        self.act_jack_render.setEnabled(canRender)
        self.b_jack_render.setEnabled(canRender)
        self.menuJackServer(True)
        self.menuJackTransport(True)

        self.cb_buffer_size.setEnabled(True)
        self.cb_sample_rate.setEnabled(bool(DBus.jack)) # DBus.jack and jacksettings.getSampleRate() != -1
        self.menu_Jack_Buffer_Size.setEnabled(True)

        self.pb_dsp_load.setMaximum(100)
        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.update()

        self.init_jack()

    def jackStopped(self):
        if haveDBus:
            self.DBusReconnect()

        # client already closed
        jack.client = None

        if self.m_next_sample_rate:
            jack_sample_rate(self, self.m_next_sample_rate)

        if DBus.jack:
            buffer_size = jacksettings.getBufferSize()
            sample_rate = jacksettings.getSampleRate()
            buffer_size_test = bool(buffer_size != -1)
            sample_rate_test = bool(sample_rate != -1)

            if buffer_size_test:
                setBufferSize(self, buffer_size)

            if sample_rate_test:
                setSampleRate(self, sample_rate)

            setRealTime(self, jacksettings.isRealtime())

            self.cb_buffer_size.setEnabled(buffer_size_test)
            self.cb_sample_rate.setEnabled(sample_rate_test)
            self.menu_Jack_Buffer_Size.setEnabled(buffer_size_test)

        else:
            self.cb_buffer_size.setEnabled(False)
            self.cb_sample_rate.setEnabled(False)
            self.menu_Jack_Buffer_Size.setEnabled(False)

        self.act_jack_render.setEnabled(False)
        self.b_jack_render.setEnabled(False)
        self.menuJackServer(False)
        self.menuJackTransport(False)
        setXruns(self, -1)

        if self.m_selected_transport_view == TRANSPORT_VIEW_HMS:
            self.label_time.setText("00:00:00")
        elif self.m_selected_transport_view == TRANSPORT_VIEW_BBT:
            self.label_time.setText("000|0|0000")
        elif self.m_selected_transport_view == TRANSPORT_VIEW_FRAMES:
            self.label_time.setText("000'000'000")

        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.setMaximum(0)
        self.pb_dsp_load.update()

        patchcanvas.clear()

    def a2jStarted(self):
        self.menuA2JBridge(True)

    def a2jStopped(self):
        self.menuA2JBridge(False)

    def menuJackServer(self, started):
        if DBus.jack:
            self.act_tools_jack_start.setEnabled(not started)
            self.act_tools_jack_stop.setEnabled(started)
            self.menuA2JBridge(False)

    def menuJackTransport(self, enabled):
        self.act_transport_play.setEnabled(enabled)
        self.act_transport_stop.setEnabled(enabled)
        self.act_transport_backwards.setEnabled(enabled)
        self.act_transport_forwards.setEnabled(enabled)
        self.menu_Transport.setEnabled(enabled)
        self.group_transport.setEnabled(enabled)

    def menuA2JBridge(self, started):
        if DBus.jack and not DBus.jack.IsStarted():
            self.act_tools_a2j_start.setEnabled(False)
            self.act_tools_a2j_stop.setEnabled(False)
        else:
            self.act_tools_a2j_start.setEnabled(not started)
            self.act_tools_a2j_stop.setEnabled(started)
            self.act_tools_a2j_export_hw.setEnabled(not started)

    def DBusSignalReceiver(self, *args, **kwds):
        if kwds['interface'] == "org.freedesktop.DBus" and kwds['path'] == "/org/freedesktop/DBus" and kwds['member'] == "NameOwnerChanged":
            appInterface, appId, newId = args

            if not newId:
                # Something crashed
                if appInterface == "org.gna.home.a2jmidid":
                    QTimer.singleShot(0, self, SLOT("slot_handleCrash_a2j()"))
                elif appInterface == "org.jackaudio.service":
                    QTimer.singleShot(0, self, SLOT("slot_handleCrash_jack()"))

        elif kwds['interface'] == "org.jackaudio.JackControl":
            if kwds['member'] == "ServerStarted":
                self.jackStarted()
            elif kwds['member'] == "ServerStopped":
                self.jackStopped()
        elif kwds['interface'] == "org.gna.home.a2jmidid.control":
            if kwds['member'] == "bridge_started":
                self.a2jStarted()
            elif kwds['member'] == "bridge_stopped":
                self.a2jStopped()

    def DBusReconnect(self):
        global a2j_client_name

        try:
            DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
            jacksettings.initBus(DBus.bus)
        except:
            DBus.jack = None

        try:
            DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            a2j_client_name = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2j_client_name = None

    def JackXRunCallback(self, arg):
        if DEBUG: print("JackXRunCallback()")
        self.emit(SIGNAL("XRunCallback()"))
        return 0

    def JackBufferSizeCallback(self, buffer_size, arg):
        if DEBUG: print("JackBufferSizeCallback(%i)" % buffer_size)
        self.emit(SIGNAL("BufferSizeCallback(int)"), buffer_size)
        return 0

    def JackSampleRateCallback(self, sample_rate, arg):
        if DEBUG: print("JackSampleRateCallback(%i)" % sample_rate)
        self.emit(SIGNAL("SampleRateCallback(int)"), sample_rate)
        return 0

    def JackPortRegistrationCallback(self, port_id, register_yesno, arg):
        if DEBUG: print("JackPortRegistrationCallback(%i, %i)" % (port_id, register_yesno))
        self.emit(SIGNAL("PortRegistrationCallback(int, bool)"), port_id, bool(register_yesno))
        return 0

    def JackPortConnectCallback(self, port_a, port_b, connect_yesno, arg):
        if DEBUG: print("JackPortConnectCallback(%i, %i, %i)" % (port_a, port_b, connect_yesno))
        self.emit(SIGNAL("PortConnectCallback(int, int, bool)"), port_a, port_b, bool(connect_yesno))
        return 0

    def JackPortRenameCallback(self, port_id, old_name, new_name, arg):
        if DEBUG: print("JackPortRenameCallback(%i, %s, %s)" % (port_id, old_name, new_name))
        self.emit(SIGNAL("PortRenameCallback(int, QString, QString)"), port_id, str(old_name, encoding="utf-8"), str(new_name, encoding="utf-8"))
        return 0

    def JackSessionCallback(self, event, arg):
        if WINDOWS:
            filepath = os.path.join(sys.argv[0])
        else:
            if sys.argv[0].startswith("/"):
                filepath = "catia"
            else:
                filepath = os.path.join(sys.path[0], "catia.py")

        event.command_line = str(filepath).encode("utf-8")
        jacklib.session_reply(jack.client, event)

        if event.type == jacklib.JackSessionSaveAndQuit:
            app.quit()

            #jacklib.session_event_free(event)

    def JackShutdownCallback(self, arg):
        if DEBUG: print("JackShutdownCallback()")
        self.emit(SIGNAL("ShutdownCallback()"))
        return 0

    @pyqtSlot()
    def slot_JackServerStart(self):
        if DBus.jack:
            try:
                ret = bool(DBus.jack.StartServer())
                return ret
            except:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to start JACK, please check the logs for more information."))
                #self.jackStopped()
        return False

    @pyqtSlot()
    def slot_JackServerStop(self):
        if DBus.jack:
            return DBus.jack.StopServer()
        else:
            return False

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if jack.client:
            self.m_xruns = 0
            setXruns(self, 0)

    @pyqtSlot()
    def slot_A2JBridgeStart(self):
        if DBus.a2j:
            return DBus.a2j.start()
        else:
            return False

    @pyqtSlot()
    def slot_A2JBridgeStop(self):
        if DBus.a2j:
            return DBus.a2j.stop()
        else:
            return False

    @pyqtSlot()
    def slot_A2JBridgeExportHW(self):
        if DBus.a2j:
            ask = QMessageBox.question(self, self.tr("A2J Hardware Export"), self.tr("Enable Hardware Export on the A2J Bridge?"), QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel, QMessageBox.No)
            if ask == QMessageBox.Yes:
                DBus.a2j.set_hw_export(True)
            elif ask == QMessageBox.No:
                DBus.a2j.set_hw_export(False)

    @pyqtSlot()
    def slot_XRunCallback(self):
        self.m_xruns += 1
        setXruns(self, self.m_xruns)

    @pyqtSlot(int)
    def slot_BufferSizeCallback(self, buffer_size):
        setBufferSize(self, buffer_size)

    @pyqtSlot(int)
    def slot_SampleRateCallback(self, sample_rate):
        setSampleRate(self, sample_rate)

    @pyqtSlot(int, bool)
    def slot_PortRegistrationCallback(self, port_id_jack, register_yesno):
        port_ptr = jacklib.port_by_id(jack.client, port_id_jack)
        port_nameR = str(jacklib.port_name(port_ptr), encoding="utf-8")

        if register_yesno:
            self.canvas_add_jack_port(port_ptr, port_nameR)
        else:
            for port in self.m_port_list:
                if port[iPortNameR] == port_nameR:
                    port_id = port[iPortId]
                    break
            else:
                return

            self.canvas_remove_jack_port(port_id)

    @pyqtSlot(int, int, bool)
    def slot_PortConnectCallback(self, port_a_jack, port_b_jack, connect_yesno):
        port_a_ptr = jacklib.port_by_id(jack.client, port_a_jack)
        port_b_ptr = jacklib.port_by_id(jack.client, port_b_jack)
        port_a_nameR = str(jacklib.port_name(port_a_ptr), encoding="utf-8")
        port_b_nameR = str(jacklib.port_name(port_b_ptr), encoding="utf-8")

        if connect_yesno:
            self.canvas_connect_ports_by_name(port_a_nameR, port_b_nameR)
        else:
            self.canvas_disconnect_ports_by_name(port_a_nameR, port_b_nameR)

    @pyqtSlot(int, str, str)
    def slot_PortRenameCallback(self, port_id_jack, old_name, new_name):
        port_ptr = jacklib.port_by_id(jack.client, port_id_jack)
        port_short_name = str(jacklib.port_short_name(port_ptr), encoding="utf-8")

        for port in self.m_port_list:
            if port[iPortNameR] == old_name:
                port_id = port[iPortId]
                port[iPortNameR] = new_name
                break
        else:
            return

        # Only set new name in canvas if no alias is active for this port
        aliases = jacklib.port_get_aliases(port_ptr)
        if aliases[0] == 1 and self.m_savedSettings["Main/JackPortAlias"] == 1:
            pass
        elif aliases[0] == 2 and self.m_savedSettings["Main/JackPortAlias"] == 2:
            pass
        else:
            self.canvas_rename_port(port_id, port_short_name)

    @pyqtSlot()
    def slot_ShutdownCallback(self):
        self.jackStopped()

    @pyqtSlot()
    def slot_handleCrash_a2j(self):
        global a2j_client_name

        try:
            DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            a2j_client_name = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2j_client_name = None

        if DBus.a2j:
            if DBus.a2j.is_started():
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.act_tools_a2j_start.setEnabled(False)
            self.act_tools_a2j_stop.setEnabled(False)
            self.act_tools_a2j_export_hw.setEnabled(False)
            self.menu_A2J_Bridge.setEnabled(False)

    @pyqtSlot()
    def slot_handleCrash_jack(self):
        self.DBusReconnect()

        if DBus.jack:
            self.act_jack_configure.setEnabled(True)
            self.b_jack_configure.setEnabled(True)
        else:
            self.act_tools_jack_start.setEnabled(False)
            self.act_tools_jack_stop.setEnabled(False)
            self.act_jack_configure.setEnabled(False)
            self.b_jack_configure.setEnabled(False)

        if DBus.a2j:
            if DBus.a2j.is_started():
                self.a2jStarted()
            else:
                self.a2jStopped()
        else:
            self.act_tools_a2j_start.setEnabled(False)
            self.act_tools_a2j_stop.setEnabled(False)
            self.act_tools_a2j_export_hw.setEnabled(False)
            self.menu_A2J_Bridge.setEnabled(False)

        self.jackStopped()

    @pyqtSlot()
    def slot_configureCatia(self):
        dialog = SettingsW(self, "catia", hasGL)
        if dialog.exec_():
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
            p_features.group_rename = False
            p_features.port_info    = True
            p_features.port_rename  = bool(self.m_savedSettings["Main/JackPortAlias"] > 0)
            p_features.handle_group_pos = True

            patchcanvas.setOptions(p_options)
            patchcanvas.setFeatures(p_features)
            patchcanvas.init(self.scene, self.canvasCallback, DEBUG)

            self.init_ports()

    @pyqtSlot()
    def slot_aboutCatia(self):
        QMessageBox.about(self, self.tr("About Catia"), self.tr("<h3>Catia</h3>"
                                                                "<br>Version %s"
                                                                "<br>Catia is a nice JACK Patchbay with A2J Bridge integration.<br>"
                                                                "<br>Copyright (C) 2010-2012 falkTX" % VERSION))

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isVisible())
        self.settings.setValue("ShowStatusbar", self.frame_statusbar.isVisible())
        self.settings.setValue("TransportView", self.m_selected_transport_view)

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.frame_toolbar.setVisible(show_toolbar)

            show_statusbar = self.settings.value("ShowStatusbar", True, type=bool)
            self.act_settings_show_statusbar.setChecked(show_statusbar)
            self.frame_statusbar.setVisible(show_statusbar)

            setTransportView(self, self.settings.value("TransportView", TRANSPORT_VIEW_HMS, type=int))

        self.m_savedSettings = {
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int),
            "Main/JackPortAlias": self.settings.value("Main/JackPortAlias", 2, type=int),
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
        if event.timerId() == self.m_timer120:
            if jack.client:
                refreshTransport(self)
        elif event.timerId() == self.m_timer600:
            if jack.client:
                refreshDSPLoad(self)
            #else:
                #self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        patchcanvas.clear()
        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Catia")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/catia.svg"))

    if haveDBus:
        DBus.loop = DBusQtMainLoop(set_as_default=True)
        DBus.bus  = dbus.SessionBus(mainloop=DBus.loop)

        try:
            DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
            jacksettings.initBus(DBus.bus)
        except:
            DBus.jack = None

        try:
            DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            a2j_client_name = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2j_client_name = None

        if DBus.jack or DBus.a2j:
            string = "Using DBus for "
            if DBus.jack:
                string += "JACK"
                if DBus.a2j:
                    string += " and a2jmidid"
            elif DBus.a2j:
                string += "a2jmidid"
            print(string)

    else:
        DBus.jack = None
        DBus.a2j = None
        a2j_client_name = None

    # Show GUI
    gui = CatiaMainW()

    # Set-up custom signal handling
    set_up_signals(gui)

    gui.show()

    # App-Loop
    ret = app.exec_()

    # Close Jack
    if jack.client:
        jacklib.deactivate(jack.client)
        jacklib.client_close(jack.client)

    # Exit properly
    sys.exit(ret)

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK Patchbay
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

from PyQt4.QtGui import QApplication

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_catia
from shared_jack import *
from shared_canvas import *
from shared_settings import *

# ------------------------------------------------------------------------------------------------------------
# Try Import DBus

try:
    import dbus
    from dbus.mainloop.qt import DBusQtMainLoop
    haveDBus = True
except:
    haveDBus = False

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Check for ALSA-MIDI

if LINUX:
    for iPATH in PATH:
        if os.path.exists(os.path.join(iPATH, "aconnect")):
            from subprocess import getoutput
            haveALSA = True
            if DEBUG:
                print("Using experimental ALSA-MIDI support")
            break
    else:
        haveALSA = False
else:
    haveALSA = False

# ------------------------------------------------------------------------------------------------------------
# Global Variables

global a2jClientName
a2jClientName = None

# ------------------------------------------------------------------------------------------------------------
# Static Variables

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

# ------------------------------------------------------------------------------------------------------------
# Catia Main Window

class CatiaMainW(AbstractCanvasJackClass, ui_catia.Ui_CatiaMainW):
    def __init__(self, parent=None):
        AbstractCanvasJackClass.__init__(self, parent, "Catia")
        self.setupUi(self)

        self.m_groupList      = []
        self.m_groupSplitList = []
        self.m_portList       = []
        self.m_connectionList = []

        self.m_lastGroupId = 1
        self.m_lastPortId  = 1
        self.m_lastConnectionId = 1

        self.loadSettings(True)

        # -------------------------------------------------------------
        # Set-up GUI

        setIcons(self, ["canvas", "jack", "transport", "misc"])

        self.act_quit.setIcon(getIcon("application-exit"))
        self.act_configure.setIcon(getIcon("configure"))

        self.cb_buffer_size.clear()
        self.cb_sample_rate.clear()

        for buffer_size in BUFFER_SIZE_LIST:
            self.cb_buffer_size.addItem(str(buffer_size))

        for sample_rate in SAMPLE_RATE_LIST:
            self.cb_sample_rate.addItem(str(sample_rate))

        self.act_jack_bf_list = (self.act_jack_bf_16, self.act_jack_bf_32, self.act_jack_bf_64, self.act_jack_bf_128,
                                 self.act_jack_bf_256, self.act_jack_bf_512, self.act_jack_bf_1024, self.act_jack_bf_2048,
                                 self.act_jack_bf_4096, self.act_jack_bf_8192)

        if not haveALSA:
            self.act_settings_show_alsa.setEnabled(False)

        # -------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self.graphicsView)
        self.graphicsView.setScene(self.scene)
        self.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.m_savedSettings["Canvas/Antialiasing"] == patchcanvas.ANTIALIASING_FULL))
        if self.m_savedSettings["Canvas/UseOpenGL"] and hasGL:
            self.graphicsView.setViewport(QGLWidget(self.graphicsView))
            self.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.m_savedSettings["Canvas/HighQualityAntialiasing"])

        pOptions = patchcanvas.options_t()
        pOptions.theme_name       = self.m_savedSettings["Canvas/Theme"]
        pOptions.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
        pOptions.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
        pOptions.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
        pOptions.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = True
        pFeatures.port_rename  = bool(self.m_savedSettings["Main/JackPortAlias"] > 0)
        pFeatures.handle_group_pos = True

        patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Catia", self.scene, self.canvasCallback, DEBUG)

        # -------------------------------------------------------------
        # Try to connect to jack

        if self.jackStarted():
            self.init_alsaPorts()

        # -------------------------------------------------------------
        # Check DBus

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

        # -------------------------------------------------------------
        # Set-up Timers

        self.m_timer120 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])
        self.m_timer600 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 5)

        # -------------------------------------------------------------
        # Set-up Connections

        self.setCanvasConnections()
        self.setJackConnections(["jack", "buffer-size", "transport", "misc"])

        self.connect(self.act_tools_jack_start, SIGNAL("triggered()"), SLOT("slot_JackServerStart()"))
        self.connect(self.act_tools_jack_stop, SIGNAL("triggered()"), SLOT("slot_JackServerStop()"))
        self.connect(self.act_tools_a2j_start, SIGNAL("triggered()"), SLOT("slot_A2JBridgeStart()"))
        self.connect(self.act_tools_a2j_stop, SIGNAL("triggered()"), SLOT("slot_A2JBridgeStop()"))
        self.connect(self.act_tools_a2j_export_hw, SIGNAL("triggered()"), SLOT("slot_A2JBridgeExportHW()"))

        self.connect(self.act_settings_show_alsa, SIGNAL("triggered(bool)"), SLOT("slot_showAlsaMIDI(bool)"))
        self.connect(self.act_configure, SIGNAL("triggered()"), SLOT("slot_configureCatia()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCatia()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("XRunCallback()"), SLOT("slot_XRunCallback()"))
        self.connect(self, SIGNAL("BufferSizeCallback(int)"), SLOT("slot_BufferSizeCallback(int)"))
        self.connect(self, SIGNAL("SampleRateCallback(int)"), SLOT("slot_SampleRateCallback(int)"))
        self.connect(self, SIGNAL("ClientRegistrationCallback(QString, bool)"), SLOT("slot_ClientRegistrationCallback(QString, bool)"))
        self.connect(self, SIGNAL("PortRegistrationCallback(int, bool)"), SLOT("slot_PortRegistrationCallback(int, bool)"))
        self.connect(self, SIGNAL("PortConnectCallback(int, int, bool)"), SLOT("slot_PortConnectCallback(int, int, bool)"))
        self.connect(self, SIGNAL("PortRenameCallback(int, QString, QString)"), SLOT("slot_PortRenameCallback(int, QString, QString)"))
        self.connect(self, SIGNAL("ShutdownCallback()"), SLOT("slot_ShutdownCallback()"))

        # -------------------------------------------------------------
        # Set-up DBus

        if DBus.jack or DBus.a2j:
            DBus.bus.add_signal_receiver(self.DBusSignalReceiver, destination_keyword="dest", path_keyword="path",
                member_keyword="member", interface_keyword="interface", sender_keyword="sender")

        # -------------------------------------------------------------

    def canvasCallback(self, action, value1, value2, valueStr):
        if action == patchcanvas.ACTION_GROUP_INFO:
            pass

        elif action == patchcanvas.ACTION_GROUP_RENAME:
            pass

        elif action == patchcanvas.ACTION_GROUP_SPLIT:
            groupId = value1
            patchcanvas.splitGroup(groupId)

        elif action == patchcanvas.ACTION_GROUP_JOIN:
            groupId = value1
            patchcanvas.joinGroup(groupId)

        elif action == patchcanvas.ACTION_PORT_INFO:
            portId = value1

            for port in self.m_portList:
                if port[iPortId] == portId:
                    portNameR = port[iPortNameR]
                    portNameG = port[iPortGroupName]
                    break
            else:
                return

            if portNameR.startswith("[ALSA-"):
                portId, portName = portNameR.split("] ", 1)[1].split(" ", 1)

                flags = []
                if portNameR.startswith("[ALSA-Input] "):
                    flags.append(self.tr("Input"))
                elif portNameR.startswith("[ALSA-Output] "):
                    flags.append(self.tr("Output"))

                flagsText = " | ".join(flags)
                typeText  = self.tr("ALSA MIDI")

                info = self.tr(""
                              "<table>"
                              "<tr><td align='right'><b>Group Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Id:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Name:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td colspan='2'>&nbsp;</td></tr>"
                              "<tr><td align='right'><b>Port Flags:</b></td><td>&nbsp;%s</td></tr>"
                              "<tr><td align='right'><b>Port Type:</b></td><td>&nbsp;%s</td></tr>"
                              "</table>" % (portNameG, portId, portName, flagsText, typeText))

            else:
                portPtr   = jacklib.port_by_name(jack.client, portNameR)
                portFlags = jacklib.port_flags(portPtr)
                groupName = portNameR.split(":", 1)[0]
                portShortName = str(jacklib.port_short_name(portPtr), encoding="utf-8")

                aliases = jacklib.port_get_aliases(portPtr)
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
                if portFlags & jacklib.JackPortIsInput:
                    flags.append(self.tr("Input"))
                if portFlags & jacklib.JackPortIsOutput:
                    flags.append(self.tr("Output"))
                if portFlags & jacklib.JackPortIsPhysical:
                    flags.append(self.tr("Physical"))
                if portFlags & jacklib.JackPortCanMonitor:
                    flags.append(self.tr("Can Monitor"))
                if portFlags & jacklib.JackPortIsTerminal:
                    flags.append(self.tr("Terminal"))

                flagsText = " | ".join(flags)

                portTypeStr = str(jacklib.port_type(portPtr), encoding="utf-8")
                if portTypeStr == jacklib.JACK_DEFAULT_AUDIO_TYPE:
                    typeText = self.tr("JACK Audio")
                elif portTypeStr == jacklib.JACK_DEFAULT_MIDI_TYPE:
                    typeText = self.tr("JACK MIDI")
                else:
                    typeText = self.tr("Unknown")

                portLatency      = jacklib.port_get_latency(portPtr)
                portTotalLatency = jacklib.port_get_total_latency(jack.client, portPtr)
                latencyText      = self.tr("%.1f ms (%i frames)" % (portLatency * 1000 / self.m_sampleRate, portLatency))
                latencyTotalText = self.tr("%.1f ms (%i frames)" % (portTotalLatency * 1000 / self.m_sampleRate, portTotalLatency))

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
                               "</table>" % (groupName, portShortName, portNameR, alias1text, alias2text, flagsText, typeText, latencyText, latencyTotalText))

            QMessageBox.information(self, self.tr("Port Information"), info)

        elif action == patchcanvas.ACTION_PORT_RENAME:
            global a2jClientName

            portId = value1
            portShortName = asciiString(valueStr)

            for port in self.m_portList:
                if port[iPortId] == portId:
                    portNameR = port[iPortNameR]

                    if portNameR.startswith("[ALSA-"):
                        QMessageBox.warning(self, self.tr("Cannot continue"), self.tr(""
                            "Rename functions rely on JACK aliases and cannot be done in ALSA ports"))
                        return

                    if portNameR.split(":", 1)[0] == a2jClientName:
                        a2jSplit = portNameR.split(":", 3)
                        portName = "%s:%s: %s" % (a2jSplit[0], a2jSplit[1], portShortName)
                    else:
                        portName = "%s:%s" % (port[iPortGroupName], portShortName)
                    break
            else:
                return

            portPtr = jacklib.port_by_name(jack.client, portNameR)
            aliases = jacklib.port_get_aliases(portPtr)

            if aliases[0] == 2:
                # JACK only allows 2 aliases, remove 2nd
                jacklib.port_unset_alias(portPtr, aliases[2])

                # If we're going for 1st alias, unset it too
                if self.m_savedSettings["Main/JackPortAlias"] == 1:
                    jacklib.port_unset_alias(portPtr, aliases[1])

            elif aliases[0] == 1 and self.m_savedSettings["Main/JackPortAlias"] == 1:
                jacklib.port_unset_alias(portPtr, aliases[1])

            if aliases[0] == 0 and self.m_savedSettings["Main/JackPortAlias"] == 2:
                # If 2nd alias is enabled and port had no previous aliases, set the 1st alias now
                jacklib.port_set_alias(portPtr, portName)

            if jacklib.port_set_alias(portPtr, portName) == 0:
                patchcanvas.renamePort(portId, portShortName)

        elif action == patchcanvas.ACTION_PORTS_CONNECT:
            portIdA = value1
            portIdB = value2
            portRealNameA = ""
            portRealNameB = ""

            for port in self.m_portList:
                if port[iPortId] == portIdA:
                    portRealNameA = port[iPortNameR]
                if port[iPortId] == portIdB:
                    portRealNameB = port[iPortNameR]

            if portRealNameA.startswith("[ALSA-"):
                portIdAlsaA = portRealNameA.split(" ", 2)[1]
                portIdAlsaB = portRealNameB.split(" ", 2)[1]

                if os.system("aconnect %s %s" % (portIdAlsaA, portIdAlsaB)) == 0:
                    self.canvas_connectPorts(portIdA, portIdB)

            elif portRealNameA and portRealNameB:
                jacklib.connect(jack.client, portRealNameA, portRealNameB)

        elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
            connectionId = value1

            for connection in self.m_connectionList:
                if connection[iConnId] == connectionId:
                    portIdA = connection[iConnOutput]
                    portIdB = connection[iConnInput]
                    break
            else:
                return

            portRealNameA = ""
            portRealNameB = ""

            for port in self.m_portList:
                if port[iPortId] == portIdA:
                    portRealNameA = port[iPortNameR]
                if port[iPortId] == portIdB:
                    portRealNameB = port[iPortNameR]

            if portRealNameA.startswith("[ALSA-"):
                portIdAlsaA = portRealNameA.split(" ", 2)[1]
                portIdAlsaB = portRealNameB.split(" ", 2)[1]

                if os.system("aconnect -d %s %s" % (portIdAlsaA, portIdAlsaB)) == 0:
                    self.canvas_disconnectPorts(portIdA, portIdB)

            elif portRealNameA and portRealNameB:
                jacklib.disconnect(jack.client, portRealNameA, portRealNameB)

    def init_ports(self):
        self.m_groupList = []
        self.m_groupSplitList = []
        self.m_portList = []
        self.m_connectionList = []
        self.m_lastGroupId = 1
        self.m_lastPortId = 1
        self.m_lastConnectionId = 1

        self.init_jackPorts()
        self.init_alsaPorts()

    def init_jack(self):
        self.m_xruns = 0
        self.m_nextSampleRate = 0

        self.m_lastBPM = None
        self.m_lastTransportState = None

        bufferSize = int(jacklib.get_buffer_size(jack.client))
        sampleRate = int(jacklib.get_sample_rate(jack.client))
        realtime   = bool(int(jacklib.is_realtime(jack.client)))

        self.setBufferSize(bufferSize)
        self.setSampleRate(sampleRate)
        self.setRealTime(realtime)
        self.setXruns(0)

        self.refreshDSPLoad()
        self.refreshTransport()

        self.init_jackCallbacks()
        self.init_jackPorts()

        self.scene.zoom_fit()
        self.scene.zoom_reset()

        jacklib.activate(jack.client)

    def init_jackCallbacks(self):
        jacklib.set_buffer_size_callback(jack.client, self.JackBufferSizeCallback, None)
        jacklib.set_sample_rate_callback(jack.client, self.JackSampleRateCallback, None)
        jacklib.set_xrun_callback(jack.client, self.JackXRunCallback, None)
        jacklib.set_port_registration_callback(jack.client, self.JackPortRegistrationCallback, None)
        jacklib.set_port_connect_callback(jack.client, self.JackPortConnectCallback, None)
        jacklib.set_session_callback(jack.client, self.JackSessionCallback, None)
        jacklib.on_shutdown(jack.client, self.JackShutdownCallback, None)

        if jacklib.JACK2:
            jacklib.set_client_registration_callback(jack.client, self.JackClientRegistrationCallback, None)
            jacklib.set_port_rename_callback(jack.client, self.JackPortRenameCallback, None)

    def init_jackPorts(self):
        if not jack.client:
            return

        global a2jClientName

        # Get all jack ports, put a2j ones to the bottom of the list
        a2jNameList  = []
        portNameList = c_char_p_p_to_list(jacklib.get_ports(jack.client, "", "", 0))

        h = 0
        for i in range(len(portNameList)):
            if portNameList[i - h].split(":")[0] == a2jClientName:
                portName = portNameList.pop(i - h)
                a2jNameList.append(portName)
                h += 1

        for a2jName in a2jNameList:
            portNameList.append(a2jName)

        del a2jNameList

        # Add jack ports
        for portName in portNameList:
            portPtr = jacklib.port_by_name(jack.client, portName)
            self.canvas_addJackPort(portPtr, portName)

        # Add jack connections
        for portName in portNameList:
            portPtr = jacklib.port_by_name(jack.client, portName)

            # Only make connections from an output port
            if jacklib.port_flags(portPtr) & jacklib.JackPortIsInput:
                continue

            portConnectionNames = c_char_p_p_to_list(jacklib.port_get_all_connections(jack.client, portPtr))

            for portConName in portConnectionNames:
                self.canvas_connectPortsByName(portName, portConName)

    def init_alsaPorts(self):
        if not (haveALSA and self.act_settings_show_alsa.isChecked()):
            return

        # Get ALSA MIDI ports (outputs)
        output = getoutput("env LANG=C aconnect -i").split("\n")
        lastGroupId   = -1
        lastGroupName = ""

        for line in output:
            # Make 'System' match JACK's 'system'
            if line == "client 0: 'System' [type=kernel]":
                line = "client 0: 'system' [type=kernel]"

            if line.startswith("client "):
                lineSplit  = line.split(": ", 1)
                lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                groupId    = int(lineSplit[0].replace("client ", ""))
                groupName  = lineSplit2[0]
                groupType  = lineSplit2[1].rsplit("]", 1)[0]

                lastGroupId = self.canvas_getGroupId(groupName)

                if lastGroupId == -1:
                    # Group doesn't exist yet
                    lastGroupId = self.canvas_addAlsaGroup(groupId, groupName, bool(groupType == "kernel"))

                lastGroupName = groupName

            elif line.startswith("    ") and lastGroupId >= 0 and lastGroupName:
                lineSplit = line.split(" '", 1)
                portId    = int(lineSplit[0].strip())
                portName  = lineSplit[1].rsplit("'", 1)[0].strip()

                self.canvas_addAlsaPort(lastGroupId, lastGroupName, portName, "%i:%i %s" % (groupId, portId, portName), False)

            else:
                lastGroupId   = -1
                lastGroupName = ""

        # Get ALSA MIDI ports (inputs)
        output = getoutput("env LANG=C aconnect -o").split("\n")
        lastGroupId   = -1
        lastGroupName = ""

        for line in output:
            # Make 'System' match JACK's 'system'
            if line == "client 0: 'System' [type=kernel]":
                line = "client 0: 'system' [type=kernel]"

            if line.startswith("client "):
                lineSplit  = line.split(": ", 1)
                lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                groupId    = int(lineSplit[0].replace("client ", ""))
                groupName  = lineSplit2[0]
                groupType  = lineSplit2[1].rsplit("]", 1)[0]

                lastGroupId = self.canvas_getGroupId(groupName)

                if lastGroupId == -1:
                    # Group doesn't exist yet
                    lastGroupId = self.canvas_addAlsaGroup(groupId, groupName, bool(groupType == "kernel"))

                lastGroupName = groupName

            elif line.startswith("    ") and lastGroupId >= 0 and lastGroupName:
                lineSplit = line.split(" '", 1)
                portId    = int(lineSplit[0].strip())
                portName  = lineSplit[1].rsplit("'", 1)[0].strip()

                self.canvas_addAlsaPort(lastGroupId, lastGroupName, portName, "%i:%i %s" % (groupId, portId, portName), True)

            else:
                lastGroupId   = -1
                lastGroupName = ""

        # Get ALSA MIDI connections
        output = getoutput("env LANG=C aconnect -ol").split("\n")
        lastGroupId = -1
        lastPortId  = -1

        for line in output:
            # Make 'System' match JACK's 'system'
            if line == "client 0: 'System' [type=kernel]":
                line = "client 0: 'system' [type=kernel]"

            if line.startswith("client "):
                lineSplit  = line.split(": ", 1)
                lineSplit2 = lineSplit[1].replace("'", "", 1).split("' [type=", 1)
                groupId    = int(lineSplit[0].replace("client ", ""))
                groupName  = lineSplit2[0]

                lastGroupId = self.canvas_getGroupId(groupName)

            elif line.startswith("    ") and lastGroupId >= 0:
                lineSplit = line.split(" '", 1)
                portId    = int(lineSplit[0].strip())
                portName  = lineSplit[1].rsplit("'", 1)[0].strip()

                for port in self.m_portList:
                    if port[iPortNameR] == "[ALSA-Input] %i:%i %s" % (groupId, portId, portName):
                        lastPortId = port[iPortId]
                        break
                else:
                    lastPortId = -1

            elif line.startswith("\tConnect") and lastGroupId >= 0 and lastPortId >= 0:
                if line.startswith("\tConnected From"):
                    lineSplit = line.split(": ", 1)[1]
                    lineConns = lineSplit.split(", ")

                    for lineConn in lineConns:
                        lineConnSplit = lineConn.replace("'","").split(":", 1)
                        alsaGroupId   = int(lineConnSplit[0])
                        alsaPortId    = int(lineConnSplit[1])

                        portNameRtest = "[ALSA-Output] %i:%i " % (alsaGroupId, alsaPortId)

                        for port in self.m_portList:
                            if port[iPortNameR].startswith(portNameRtest):
                                self.canvas_connectPorts(port[iPortId], lastPortId)
                                break

            else:
                lastGroupId = -1
                lastPortId  = -1

    def canvas_getGroupId(self, groupName):
        for group in self.m_groupList:
            if group[iGroupName] == groupName:
                return group[iGroupId]
        return -1

    def canvas_addAlsaGroup(self, alsaGroupId, groupName, hwSplit):
        groupId = self.m_lastGroupId

        if hwSplit:
            patchcanvas.addGroup(groupId, groupName, patchcanvas.SPLIT_YES, patchcanvas.ICON_HARDWARE)
        else:
            patchcanvas.addGroup(groupId, groupName)

        groupObj = [None, None, None]
        groupObj[iGroupId]   = groupId
        groupObj[iGroupName] = groupName
        groupObj[iGroupType] = GROUP_TYPE_ALSA

        self.m_groupList.append(groupObj)
        self.m_lastGroupId += 1

        return groupId

    def canvas_addJackGroup(self, groupName):
        groupId = self.m_lastGroupId
        patchcanvas.addGroup(groupId, groupName)

        groupObj = [None, None, None]
        groupObj[iGroupId]   = groupId
        groupObj[iGroupName] = groupName
        groupObj[iGroupType] = GROUP_TYPE_JACK

        self.m_groupList.append(groupObj)
        self.m_lastGroupId += 1

        return groupId

    def canvas_removeGroup(self, groupName):
        groupId = -1
        for group in self.m_groupList:
            if group[iGroupName] == groupName:
                groupId = group[iGroupId]
                self.m_groupList.remove(group)
                break
        else:
            print("Catia - remove group failed")
            return

        patchcanvas.removeGroup(groupId)

    def canvas_addAlsaPort(self, groupId, groupName, portName, portNameR, isPortInput):
        portId   = self.m_lastPortId
        portMode = patchcanvas.PORT_MODE_INPUT if isPortInput else patchcanvas.PORT_MODE_OUTPUT
        portType = patchcanvas.PORT_TYPE_MIDI_ALSA

        patchcanvas.addPort(groupId, portId, portName, portMode, portType)

        portObj = [None, None, None, None]
        portObj[iPortId]    = portId
        portObj[iPortName]  = portName
        portObj[iPortNameR] = "[ALSA-%s] %s" % ("Input" if isPortInput else "Output", portNameR)
        portObj[iPortGroupName] = groupName

        self.m_portList.append(portObj)
        self.m_lastPortId += 1

        return portId

    def canvas_addJackPort(self, portPtr, portName):
        global a2jClientName

        portId  = self.m_lastPortId
        groupId = -1

        portNameR = portName

        aliasN = self.m_savedSettings["Main/JackPortAlias"]
        if aliasN in (1, 2):
            aliases = jacklib.port_get_aliases(portPtr)
            if aliases[0] == 2 and aliasN == 2:
                portName = aliases[2]
            elif aliases[0] >= 1 and aliasN == 1:
                portName = aliases[1]

        portFlags = jacklib.port_flags(portPtr)
        groupName = portName.split(":", 1)[0]

        if portFlags & jacklib.JackPortIsInput:
            portMode = patchcanvas.PORT_MODE_INPUT
        elif portFlags & jacklib.JackPortIsOutput:
            portMode = patchcanvas.PORT_MODE_OUTPUT
        else:
            portMode = patchcanvas.PORT_MODE_NULL

        if groupName == a2jClientName:
            portType  = patchcanvas.PORT_TYPE_MIDI_A2J
            groupName = portName.replace("%s:" % a2jClientName, "", 1).split(" [", 1)[0]
            portShortName = portName.split("): ", 1)[1]

        else:
            portShortName = portName.replace("%s:" % groupName, "", 1)

            portTypeStr = str(jacklib.port_type(portPtr), encoding="utf-8")
            if portTypeStr == jacklib.JACK_DEFAULT_AUDIO_TYPE:
                portType = patchcanvas.PORT_TYPE_AUDIO_JACK
            elif portTypeStr == jacklib.JACK_DEFAULT_MIDI_TYPE:
                portType = patchcanvas.PORT_TYPE_MIDI_JACK
            else:
                portType = patchcanvas.PORT_TYPE_NULL

        for group in self.m_groupList:
            if group[iGroupName] == groupName:
                groupId = group[iGroupId]
                break
        else:
            # For ports with no group
            groupId = self.canvas_addJackGroup(groupName)

        patchcanvas.addPort(groupId, portId, portShortName, portMode, portType)

        portObj = [None, None, None, None]
        portObj[iPortId]    = portId
        portObj[iPortName]  = portName
        portObj[iPortNameR] = portNameR
        portObj[iPortGroupName] = groupName

        self.m_portList.append(portObj)
        self.m_lastPortId += 1

        if groupId not in self.m_groupSplitList and (portFlags & jacklib.JackPortIsPhysical) > 0:
            patchcanvas.splitGroup(groupId)
            patchcanvas.setGroupIcon(groupId, patchcanvas.ICON_HARDWARE)
            self.m_groupSplitList.append(groupId)

        return portId

    def canvas_removeJackPort(self, portId):
        patchcanvas.removePort(portId)

        for port in self.m_portList:
            if port[iPortId] == portId:
                groupName = port[iPortGroupName]
                self.m_portList.remove(port)
                break
        else:
            return

        # Check if group has no more ports; if yes remove it
        for port in self.m_portList:
            if port[iPortGroupName] == groupName:
                break
        else:
            self.canvas_removeGroup(groupName)

    def canvas_renamePort(self, portId, portShortName):
        patchcanvas.renamePort(portId, portShortName)

    def canvas_connectPorts(self, portOutId, portInId):
        connectionId = self.m_lastConnectionId
        patchcanvas.connectPorts(connectionId, portOutId, portInId)

        connObj = [None, None, None]
        connObj[iConnId]     = connectionId
        connObj[iConnOutput] = portOutId
        connObj[iConnInput]  = portInId

        self.m_connectionList.append(connObj)
        self.m_lastConnectionId += 1

        return connectionId

    def canvas_connectPortsByName(self, portOutName, portInName):
        portOutId = -1
        portInId  = -1

        for port in self.m_portList:
            if port[iPortNameR] == portOutName:
                portOutId = port[iPortId]
            elif port[iPortNameR] == portInName:
                portInId = port[iPortId]

            if portOutId >= 0 and portInId >= 0:
                break

        else:
            print("Catia - connect jack ports failed")
            return -1

        return self.canvas_connectPorts(portOutId, portInId)

    def canvas_disconnectPorts(self, portOutId, portInId):
        for connection in self.m_connectionList:
            if connection[iConnOutput] == portOutId and connection[iConnInput] == portInId:
                patchcanvas.disconnectPorts(connection[iConnId])
                self.m_connectionList.remove(connection)
                break

    def canvas_disconnectPortsByName(self, portOutName, portInName):
        portOutId = -1
        portInId  = -1

        for port in self.m_portList:
            if port[iPortNameR] == portOutName:
                portOutId = port[iPortId]
            elif port[iPortNameR] == portInName:
                portInId = port[iPortId]

        if portOutId == -1 or portInId == -1:
            print("Catia - disconnect ports failed")
            return

        self.canvas_disconnectPorts(portOutId, portInId)

    def jackStarted(self):
        if not jack.client:
            jack.client = jacklib.client_open("catia", jacklib.JackNoStartServer | jacklib.JackSessionID, None)
            if not jack.client:
                self.jackStopped()
                return False

        canRender = render.canRender()

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

        return True

    def jackStopped(self):
        if haveDBus:
            self.DBusReconnect()

        # client already closed
        jack.client = None

        # refresh canvas (remove jack ports)
        patchcanvas.clear()
        self.init_ports()

        if self.m_nextSampleRate:
            self.jack_setSampleRate(self.m_nextSampleRate)

        if DBus.jack:
            bufferSize = jacksettings.getBufferSize()
            sampleRate = jacksettings.getSampleRate()
            bufferSizeTest = bool(bufferSize != -1)
            sampleRateTest = bool(sampleRate != -1)

            if bufferSizeTest:
                self.setBufferSize(bufferSize)

            if sampleRateTest:
                self.setSampleRate(sampleRate)

            self.setRealTime(jacksettings.isRealtime())

            self.cb_buffer_size.setEnabled(bufferSizeTest)
            self.cb_sample_rate.setEnabled(sampleRateTest)
            self.menu_Jack_Buffer_Size.setEnabled(bufferSizeTest)

        else:
            self.cb_buffer_size.setEnabled(False)
            self.cb_sample_rate.setEnabled(False)
            self.menu_Jack_Buffer_Size.setEnabled(False)

        self.act_jack_render.setEnabled(False)
        self.b_jack_render.setEnabled(False)
        self.menuJackServer(False)
        self.menuJackTransport(False)
        self.setXruns(-1)

        if self.m_curTransportView == TRANSPORT_VIEW_HMS:
            self.label_time.setText("00:00:00")
        elif self.m_curTransportView == TRANSPORT_VIEW_BBT:
            self.label_time.setText("000|0|0000")
        elif self.m_curTransportView == TRANSPORT_VIEW_FRAMES:
            self.label_time.setText("000'000'000")

        self.pb_dsp_load.setValue(0)
        self.pb_dsp_load.setMaximum(0)
        self.pb_dsp_load.update()

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
        if kwds["interface"] == "org.freedesktop.DBus" and kwds["path"] == "/org/freedesktop/DBus" and kwds["member"] == "NameOwnerChanged":
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
        global a2jClientName

        try:
            DBus.jack = DBus.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
            jacksettings.initBus(DBus.bus)
        except:
            DBus.jack = None

        try:
            DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            a2jClientName = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2jClientName = None

    def JackXRunCallback(self, arg):
        if DEBUG: print("JackXRunCallback()")
        self.emit(SIGNAL("XRunCallback()"))
        return 0

    def JackBufferSizeCallback(self, bufferSize, arg):
        if DEBUG: print("JackBufferSizeCallback(%i)" % bufferSize)
        self.emit(SIGNAL("BufferSizeCallback(int)"), bufferSize)
        return 0

    def JackSampleRateCallback(self, sampleRate, arg):
        if DEBUG: print("JackSampleRateCallback(%i)" % sampleRate)
        self.emit(SIGNAL("SampleRateCallback(int)"), sampleRate)
        return 0

    def JackClientRegistrationCallback(self, clientName, registerYesNo, arg):
        if DEBUG: print("JackClientRegistrationCallback(\"%s\", %i)" % (clientName, registerYesNo))
        self.emit(SIGNAL("ClientRegistrationCallback(QString, bool)"), str(clientName, encoding="utf-8"), bool(registerYesNo))
        return 0

    def JackPortRegistrationCallback(self, portId, registerYesNo, arg):
        if DEBUG: print("JackPortRegistrationCallback(%i, %i)" % (portId, registerYesNo))
        self.emit(SIGNAL("PortRegistrationCallback(int, bool)"), portId, bool(registerYesNo))
        return 0

    def JackPortConnectCallback(self, portA, portB, connectYesNo, arg):
        if DEBUG: print("JackPortConnectCallback(%i, %i, %i)" % (portA, portB, connectYesNo))
        self.emit(SIGNAL("PortConnectCallback(int, int, bool)"), portA, portB, bool(connectYesNo))
        return 0

    def JackPortRenameCallback(self, portId, oldName, newName, arg):
        if DEBUG: print("JackPortRenameCallback(%i, \"%s\", \"%s\")" % (portId, oldName, newName))
        self.emit(SIGNAL("PortRenameCallback(int, QString, QString)"), portId, str(oldName, encoding="utf-8"), str(newName, encoding="utf-8"))
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

    @pyqtSlot(bool)
    def slot_showAlsaMIDI(self, yesNo):
        # refresh canvas (remove jack ports)
        patchcanvas.clear()
        self.init_ports()

    @pyqtSlot()
    def slot_JackServerStart(self):
        ret = False
        if DBus.jack:
            try:
                ret = bool(DBus.jack.StartServer())
            except:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("Failed to start JACK, please check the logs for more information."))
                #self.jackStopped()
        return ret

    @pyqtSlot()
    def slot_JackServerStop(self):
        ret = False
        if DBus.jack:
            ret = bool(DBus.jack.StopServer())
        return ret

    @pyqtSlot()
    def slot_JackClearXruns(self):
        if jack.client:
            self.m_xruns = 0
            self.setXruns(0)

    @pyqtSlot()
    def slot_A2JBridgeStart(self):
        ret = False
        if DBus.a2j:
            ret = bool(DBus.a2j.start())
        return ret

    @pyqtSlot()
    def slot_A2JBridgeStop(self):
        ret = False
        if DBus.a2j:
            ret = bool(DBus.a2j.stop())
        return ret

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
        self.setXruns(self.m_xruns)

    @pyqtSlot(int)
    def slot_BufferSizeCallback(self, bufferSize):
        self.setBufferSize(bufferSize)

    @pyqtSlot(int)
    def slot_SampleRateCallback(self, sampleRate):
        self.setSampleRate(sampleRate)

    @pyqtSlot(str, bool)
    def slot_ClientRegistrationCallback(self, clientName, registerYesNo):
        if registerYesNo and clientName == "system" and jack.client:
            bufferSize = int(jacklib.get_buffer_size(jack.client))
            sampleRate = int(jacklib.get_sample_rate(jack.client))
            realtime   = bool(int(jacklib.is_realtime(jack.client)))

            self.setBufferSize(bufferSize)
            self.setSampleRate(sampleRate)
            self.setRealTime(realtime)
            self.setXruns(0)

    @pyqtSlot(int, bool)
    def slot_PortRegistrationCallback(self, portIdJack, registerYesNo):
        portPtr = jacklib.port_by_id(jack.client, portIdJack)
        portNameR = str(jacklib.port_name(portPtr), encoding="utf-8")

        if registerYesNo:
            self.canvas_addJackPort(portPtr, portNameR)
        else:
            for port in self.m_portList:
                if port[iPortNameR] == portNameR:
                    portIdCanvas = port[iPortId]
                    break
            else:
                return

            self.canvas_removeJackPort(portIdCanvas)

    @pyqtSlot(int, int, bool)
    def slot_PortConnectCallback(self, portIdJackA, portIdJackB, connectYesNo):
        portPtrA = jacklib.port_by_id(jack.client, portIdJackA)
        portPtrB = jacklib.port_by_id(jack.client, portIdJackB)
        portRealNameA = str(jacklib.port_name(portPtrA), encoding="utf-8")
        portRealNameB = str(jacklib.port_name(portPtrB), encoding="utf-8")

        if connectYesNo:
            self.canvas_connectPortsByName(portRealNameA, portRealNameB)
        else:
            self.canvas_disconnectPortsByName(portRealNameA, portRealNameB)

    @pyqtSlot(int, str, str)
    def slot_PortRenameCallback(self, portIdJack, oldName, newName):
        portPtr = jacklib.port_by_id(jack.client, portIdJack)
        portShortName = str(jacklib.port_short_name(portPtr), encoding="utf-8")

        for port in self.m_portList:
            if port[iPortNameR] == oldName:
                portIdCanvas = port[iPortId]
                port[iPortNameR] = newName
                break
        else:
            return

        # Only set new name in canvas if no alias is active for this port
        aliases = jacklib.port_get_aliases(portPtr)
        if aliases[0] == 1 and self.m_savedSettings["Main/JackPortAlias"] == 1:
            pass
        elif aliases[0] == 2 and self.m_savedSettings["Main/JackPortAlias"] == 2:
            pass
        else:
            self.canvas_renamePort(portIdCanvas, portShortName)

    @pyqtSlot()
    def slot_ShutdownCallback(self):
        self.jackStopped()

    @pyqtSlot()
    def slot_handleCrash_a2j(self):
        global a2jClientName

        try:
            DBus.a2j = dbus.Interface(DBus.bus.get_object("org.gna.home.a2jmidid", "/"), "org.gna.home.a2jmidid.control")
            a2jClientName = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2jClientName = None

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

            pOptions = patchcanvas.options_t()
            pOptions.theme_name       = self.m_savedSettings["Canvas/Theme"]
            pOptions.auto_hide_groups = self.m_savedSettings["Canvas/AutoHideGroups"]
            pOptions.use_bezier_lines = self.m_savedSettings["Canvas/UseBezierLines"]
            pOptions.antialiasing     = self.m_savedSettings["Canvas/Antialiasing"]
            pOptions.eyecandy         = self.m_savedSettings["Canvas/EyeCandy"]

            pFeatures = patchcanvas.features_t()
            pFeatures.group_info   = False
            pFeatures.group_rename = False
            pFeatures.port_info    = True
            pFeatures.port_rename  = bool(self.m_savedSettings["Main/JackPortAlias"] > 0)
            pFeatures.handle_group_pos = True

            patchcanvas.setOptions(pOptions)
            patchcanvas.setFeatures(pFeatures)
            patchcanvas.init("Catia", self.scene, self.canvasCallback, DEBUG)

            self.init_ports()

    @pyqtSlot()
    def slot_aboutCatia(self):
        QMessageBox.about(self, self.tr("About Catia"), self.tr("<h3>Catia</h3>"
                                                                "<br>Version %s"
                                                                "<br>Catia is a nice JACK Patchbay with A2J Bridge integration.<br>"
                                                                "<br>Copyright (C) 2010-2012 falkTX" % VERSION))

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowAlsaMIDI", self.act_settings_show_alsa.isChecked())
        self.settings.setValue("ShowToolbar", self.frame_toolbar.isVisible())
        self.settings.setValue("ShowStatusbar", self.frame_statusbar.isVisible())
        self.settings.setValue("TransportView", self.m_curTransportView)

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

            show_alsamidi = self.settings.value("ShowAlsaMIDI", False, type=bool)
            self.act_settings_show_alsa.setChecked(show_alsamidi)

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.frame_toolbar.setVisible(show_toolbar)

            show_statusbar = self.settings.value("ShowStatusbar", True, type=bool)
            self.act_settings_show_statusbar.setChecked(show_statusbar)
            self.frame_statusbar.setVisible(show_statusbar)

            self.setTransportView(self.settings.value("TransportView", TRANSPORT_VIEW_HMS, type=int))

        self.m_savedSettings = {
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int),
            "Main/JackPortAlias": self.settings.value("Main/JackPortAlias", 2, type=int),
            "Canvas/Theme": self.settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": self.settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": self.settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": self.settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": self.settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": self.settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/HighQualityAntialiasing": self.settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

    def timerEvent(self, event):
        if event.timerId() == self.m_timer120:
            if jack.client:
                self.refreshTransport()
        elif event.timerId() == self.m_timer600:
            if jack.client:
                self.refreshDSPLoad()
            #else:
                #self.update()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        patchcanvas.clear()
        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Catia")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/catia.svg"))

    if jacklib is None:
        QMessageBox.critical(None, app.translate("CatiaMainW", "Error"), app.translate("CatiaMainW",
            "JACK is not available in this system, cannot use this application."))
        sys.exit(1)

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
            a2jClientName = str(DBus.a2j.get_jack_client_name())
        except:
            DBus.a2j = None
            a2jClientName = None

        if DEBUG and (DBus.jack or DBus.a2j):
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
        DBus.a2j  = None
        a2jClientName = None
        if DEBUG:
            print("Not using DBus")

    # Init GUI
    gui = CatiaMainW()

    # Set-up custom signal handling
    setUpSignals(gui)

    # Show GUI
    gui.show()

    # App-Loop
    ret = app.exec_()

    # Close Jack
    if jack.client:
        jacklib.deactivate(jack.client)
        jacklib.client_close(jack.client)

    # Exit properly
    sys.exit(ret)

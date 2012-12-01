#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to JACK
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

from PyQt4.QtCore import pyqtSlot, QSettings, QTimer
from PyQt4.QtGui import QCursor, QFontMetrics, QMainWindow, QMenu

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import jacksettings
import logs, render
from shared import *
from jacklib_helpers import *

# ------------------------------------------------------------------------------------------------------------
# Have JACK2 ?

if DEBUG and jacklib and jacklib.JACK2:
    print("Using JACK2, version %s" % cString(jacklib.get_version_string()))

# ------------------------------------------------------------------------------------------------------------
# Static Variables

TRANSPORT_VIEW_HMS = 0
TRANSPORT_VIEW_BBT = 1
TRANSPORT_VIEW_FRAMES = 2

BUFFER_SIZE_LIST = (16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192)
SAMPLE_RATE_LIST = (22050, 32000, 44100, 48000, 88200, 96000, 192000)

# ------------------------------------------------------------------------------------------------------------
# Global DBus object

class DBusObject(object):
    __slots__ = [
        'loop',
        'bus',
        'a2j',
        'jack',
        'ladish_control',
        'ladish_studio',
        'ladish_room',
        'ladish_graph',
        'ladish_manager',
        'ladish_app_iface',
        'ladish_app_daemon',
        'patchbay'
    ]

DBus = DBusObject()
DBus.loop = None
DBus.bus  = None
DBus.a2j  = None
DBus.jack = None
DBus.ladish_control = None
DBus.ladish_studio  = None
DBus.ladish_room    = None
DBus.ladish_graph   = None
DBus.ladish_app_iface  = None
DBus.ladish_app_daemon = None
DBus.patchbay = None

# ------------------------------------------------------------------------------------------------------------
# Global JACK object

class JackObject(object):
    __slots__ = [
        'client'
    ]

jack = JackObject()
jack.client = None

# ------------------------------------------------------------------------------------------------------------
# Abstract JACK Window

class AbstractJackW(QMainWindow):
    def __init__(self, parent, appName):
        QMainWindow.__init__(self, parent)

        self.m_appName          = appName
        self.m_curTransportView = TRANSPORT_VIEW_HMS

        self.m_lastBPM = None
        self.m_lastTransportState = None

        self.m_xruns = -1
        self.m_bufferSize = 0
        self.m_sampleRate = 0
        self.m_nextSampleRate = 0

        self.m_logsW = None

        self.settings = QSettings("Cadence", appName)

    # -----------------------------------------------------------------
    # Property change calls

    def jack_buffer_size(self, bufferSize):
        if self.m_bufferSize != bufferSize:
            if jack.client:
                failed = bool(jacklib.set_buffer_size(jack.client, bufferSize) != 0)
            else:
                failed = bool(jacksettings.setBufferSize(bufferSize))

            if failed:
                print("Failed to change buffer-size as %i, reset to %i" % (bufferSize, self.m_bufferSize))
                self.setBufferSize(self.m_bufferSize, True)

    def jack_sample_rate(self, sampleRate):
        if jack.client:
            self.setSampleRate(sampleRate, True)
        else:
            if jacksettings.setSampleRate(sampleRate):
                self.setSampleRate(sampleRate)

    @pyqtSlot(int)
    def slot_jackBufferSize_Menu(self, bufferSize):
        self.jack_buffer_size(bufferSize)

    @pyqtSlot(str)
    def slot_jackBufferSize_ComboBox(self, text):
        if text and text.isdigit():
            self.jack_buffer_size(int(text))

    @pyqtSlot(str)
    def slot_jackSampleRate_ComboBox(self, text):
        if text and text.isdigit():
            self.jack_sample_rate(int(text))

    # -----------------------------------------------------------------
    # Transport calls

    def setTransportView(self, view):
        if view == TRANSPORT_VIEW_HMS:
            self.m_curTransportView = TRANSPORT_VIEW_HMS
            self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("00:00:00") + 3)
        elif view == TRANSPORT_VIEW_BBT:
            self.m_curTransportView = TRANSPORT_VIEW_BBT
            self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("000|00|0000") + 3)
        elif view == TRANSPORT_VIEW_FRAMES:
            self.m_curTransportView = TRANSPORT_VIEW_FRAMES
            self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("000'000'000") + 3)
        else:
            self.setTransportView(TRANSPORT_VIEW_HMS)

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, play):
        if not jack.client: return
        if play:
            jacklib.transport_start(jack.client)
        else:
            jacklib.transport_stop(jack.client)
        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if not jack.client: return
        jacklib.transport_stop(jack.client)
        jacklib.transport_locate(jack.client, 0)
        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if not jack.client: return
        newFrame = jacklib.get_current_transport_frame(jack.client) - 100000
        if newFrame < 0:
            newFrame = 0
        jacklib.transport_locate(jack.client, newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if not jack.client: return
        newFrame = jacklib.get_current_transport_frame(jack.client) + 100000
        jacklib.transport_locate(jack.client, newFrame)

    @pyqtSlot()
    def slot_transportViewMenu(self):
        menu = QMenu(self)
        act_t_hms = menu.addAction("Hours:Minutes:Seconds")
        act_t_bbt = menu.addAction("Beat:Bar:Tick")
        act_t_fr  = menu.addAction("Frames")

        act_t_hms.setCheckable(True)
        act_t_bbt.setCheckable(True)
        act_t_fr.setCheckable(True)

        if self.m_curTransportView == TRANSPORT_VIEW_HMS:
            act_t_hms.setChecked(True)
        elif self.m_curTransportView == TRANSPORT_VIEW_BBT:
            act_t_bbt.setChecked(True)
        elif self.m_curTransportView == TRANSPORT_VIEW_FRAMES:
            act_t_fr.setChecked(True)

        act_selected = menu.exec_(QCursor().pos())

        if act_selected == act_t_hms:
            self.setTransportView(TRANSPORT_VIEW_HMS)
        elif act_selected == act_t_bbt:
            self.setTransportView(TRANSPORT_VIEW_BBT)
        elif act_selected == act_t_fr:
            self.setTransportView(TRANSPORT_VIEW_FRAMES)

    # -----------------------------------------------------------------
    # Refresh GUI stuff

    def refreshDSPLoad(self):
        if not jack.client: return
        self.setDSPLoad(int(jacklib.cpu_load(jack.client)))

    def refreshTransport(self):
        if not jack.client: return
        pos = jacklib.jack_position_t()
        pos.valid = 0
        state = jacklib.transport_query(jack.client, jacklib.pointer(pos))

        if self.m_curTransportView == TRANSPORT_VIEW_HMS:
            frame = pos.frame
            time = frame / self.m_sampleRate
            secs = time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60
            self.label_time.setText("%02i:%02i:%02i" % (hrs, mins, secs))

        elif self.m_curTransportView == TRANSPORT_VIEW_BBT:
            if pos.valid & jacklib.JackPositionBBT:
                bar  = pos.bar
                beat = pos.beat
                tick = pos.tick
                if bar == 0:
                    beat = 0
                    tick = 0
                self.label_time.setText("%03i|%02i|%04i" % (bar, beat, tick))
            else:
                self.label_time.setText("%03i|%02i|%04i" % (0, 0, 0))

        elif self.m_curTransportView == TRANSPORT_VIEW_FRAMES:
            frame1 = pos.frame % 1000
            frame2 = (pos.frame / 1000) % 1000
            frame3 = (pos.frame / 1000000) % 1000
            self.label_time.setText("%03i'%03i'%03i" % (frame3, frame2, frame1))

        if pos.valid & jacklib.JackPositionBBT:
            if self.m_lastBPM != pos.beats_per_minute:
                self.sb_bpm.setValue(pos.beats_per_minute)
                self.sb_bpm.setStyleSheet("")
        else:
            pos.beats_per_minute = -1
            if self.m_lastBPM != pos.beats_per_minute:
                self.sb_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

        self.m_lastBPM = pos.beats_per_minute

        if state != self.m_lastTransportState:
            if state == jacklib.JackTransportStopped:
                icon = getIcon("media-playback-start")
                self.act_transport_play.setChecked(False)
                self.act_transport_play.setIcon(icon)
                self.act_transport_play.setText(self.tr("&Play"))
                self.b_transport_play.setChecked(False)
                self.b_transport_play.setIcon(icon)
            else:
                icon = getIcon("media-playback-pause")
                self.act_transport_play.setChecked(True)
                self.act_transport_play.setIcon(icon)
                self.act_transport_play.setText(self.tr("&Pause"))
                self.b_transport_play.setChecked(True)
                self.b_transport_play.setIcon(icon)

        self.m_lastTransportState = state

    # -----------------------------------------------------------------
    # Set GUI stuff

    def setBufferSize(self, bufferSize, forced=False):
        if self.m_bufferSize == bufferSize and not forced:
            return

        self.m_bufferSize = bufferSize

        if bufferSize:
            if bufferSize == 16:
                self.cb_buffer_size.setCurrentIndex(0)
            elif bufferSize == 32:
                self.cb_buffer_size.setCurrentIndex(1)
            elif bufferSize == 64:
                self.cb_buffer_size.setCurrentIndex(2)
            elif bufferSize == 128:
                self.cb_buffer_size.setCurrentIndex(3)
            elif bufferSize == 256:
                self.cb_buffer_size.setCurrentIndex(4)
            elif bufferSize == 512:
                self.cb_buffer_size.setCurrentIndex(5)
            elif bufferSize == 1024:
                self.cb_buffer_size.setCurrentIndex(6)
            elif bufferSize == 2048:
                self.cb_buffer_size.setCurrentIndex(7)
            elif bufferSize == 4096:
                self.cb_buffer_size.setCurrentIndex(8)
            elif bufferSize == 8192:
                self.cb_buffer_size.setCurrentIndex(9)
            else:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("Invalid JACK buffer-size requested: %i" % bufferSize))

        if self.m_appName == "Catia":
            if bufferSize:
                for act_bf in self.act_jack_bf_list:
                    act_bf.setEnabled(True)
                    if act_bf.text().replace("&", "") == str(bufferSize):
                        if not act_bf.isChecked():
                            act_bf.setChecked(True)
                    else:
                        if act_bf.isChecked():
                            act_bf.setChecked(False)
            #else:
                #for i in range(len(self.act_jack_bf_list)):
                    #self.act_jack_bf_list[i].setEnabled(False)
                    #if (self.act_jack_bf_list[i].isChecked()):
                        #self.act_jack_bf_list[i].setChecked(False)

    def setSampleRate(self, sampleRate, future=False):
        if self.m_sampleRate == sampleRate:
            return

        if future:
            if self.sender() == self.cb_sample_rate: # Changed using GUI
                ask = QMessageBox.question(self, self.tr("Change Sample Rate"),
                    self.tr("It's not possible to change Sample Rate while JACK is running.\n"
                            "Do you want to change as soon as JACK stops?"), QMessageBox.Ok | QMessageBox.Cancel)
                if ask == QMessageBox.Ok:
                    self.m_nextSampleRate = sampleRate
                else:
                    self.m_nextSampleRate = 0

        # not future
        else:
            self.m_sampleRate = sampleRate
            self.m_nextSampleRate = 0

        for i in range(len(SAMPLE_RATE_LIST)):
            sampleRate = SAMPLE_RATE_LIST[i]
            sampleRateStr = str(sampleRate)

            self.cb_sample_rate.setItemText(i, sampleRateStr)

            if self.m_sampleRate == sampleRate:
                self.cb_sample_rate.setCurrentIndex(i)

    def setRealTime(self, realtime):
        self.label_realtime.setText(" RT " if realtime else " <s>RT</s> ")
        self.label_realtime.setEnabled(realtime)

    def setDSPLoad(self, dsp_load):
        self.pb_dsp_load.setValue(dsp_load)

    def setXruns(self, xruns):
        txt1 = str(xruns) if (xruns >= 0) else "--"
        txt2 = "" if (xruns == 1) else "s"
        self.b_xruns.setText("%s Xrun%s" % (txt1, txt2))

    # -----------------------------------------------------------------
    # External Dialogs

    @pyqtSlot()
    def slot_showJackSettings(self):
        jacksettingsW = jacksettings.JackSettingsW(self)
        jacksettingsW.exec_()
        del jacksettingsW

        # Force update of gui widgets
        if not jack.client:
            self.jackStopped()

    @pyqtSlot()
    def slot_showLogs(self):
        if self.m_logsW is None:
            self.m_logsW = logs.LogsW(self)
        self.m_logsW.show()

    @pyqtSlot()
    def slot_showRender(self):
        renderW = render.RenderW(self)
        renderW.exec_()
        del renderW

# ------------------------------------------------------------------------------------------------------------
# Shared Connections

def setJackConnections(self_, modes):
    if "jack" in modes:
        self_.connect(self_.act_jack_clear_xruns, SIGNAL("triggered()"), SLOT("slot_JackClearXruns()"))
        self_.connect(self_.act_jack_render, SIGNAL("triggered()"), SLOT("slot_showRender()"))
        self_.connect(self_.act_jack_configure, SIGNAL("triggered()"), SLOT("slot_showJackSettings()"))
        self_.connect(self_.b_jack_clear_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))
        self_.connect(self_.b_jack_configure, SIGNAL("clicked()"), SLOT("slot_showJackSettings()"))
        self_.connect(self_.b_jack_render, SIGNAL("clicked()"), SLOT("slot_showRender()"))
        self_.connect(self_.cb_buffer_size, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_jackBufferSize_ComboBox(QString)"))
        self_.connect(self_.cb_sample_rate, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_jackSampleRate_ComboBox(QString)"))
        self_.connect(self_.b_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))

    #if "buffer-size" in modes:
        #self_.connect(self_.act_jack_bf_16, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 16))
        #self_.connect(self_.act_jack_bf_32, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 32))
        #self_.connect(self_.act_jack_bf_64, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 64))
        #self_.connect(self_.act_jack_bf_128, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 128))
        #self_.connect(self_.act_jack_bf_256, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 256))
        #self_.connect(self_.act_jack_bf_512, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 512))
        #self_.connect(self_.act_jack_bf_1024, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 1024))
        #self_.connect(self_.act_jack_bf_2048, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 2048))
        #self_.connect(self_.act_jack_bf_4096, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 4096))
        #self_.connect(self_.act_jack_bf_8192, SIGNAL("triggered(bool)"), lambda: slot_jackBufferSize_Menu(self_, 8192))

    if "transport" in modes:
        self_.connect(self_.act_transport_play, SIGNAL("triggered(bool)"), SLOT("slot_transportPlayPause(bool)"))
        self_.connect(self_.act_transport_stop, SIGNAL("triggered()"), SLOT("slot_transportStop()"))
        self_.connect(self_.act_transport_backwards, SIGNAL("triggered()"), SLOT("slot_transportBackwards()"))
        self_.connect(self_.act_transport_forwards, SIGNAL("triggered()"), SLOT("slot_transportForwards()"))
        self_.connect(self_.b_transport_play, SIGNAL("clicked(bool)"), SLOT("slot_transportPlayPause(bool)"))
        self_.connect(self_.b_transport_stop, SIGNAL("clicked()"), SLOT("slot_transportStop()"))
        self_.connect(self_.b_transport_backwards, SIGNAL("clicked()"), SLOT("slot_transportBackwards()"))
        self_.connect(self_.b_transport_forwards, SIGNAL("clicked()"), SLOT("slot_transportForwards()"))
        self_.connect(self_.label_time, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_transportViewMenu()"))

    if "misc" in modes:
        if LINUX:
            self_.connect(self_.act_show_logs, SIGNAL("triggered()"), SLOT("slot_showLogs()"))
        else:
            self_.act_show_logs.setEnabled(False)

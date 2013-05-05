#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to Canvas and JACK
# Copyright (C) 2010-2013 Filipe Coelho <falktx@falktx.com>
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

from PyQt4.QtCore import pyqtSlot, QTimer
from PyQt4.QtGui import QCursor, QFontMetrics, QImage, QMainWindow, QMenu, QPainter, QPrinter, QPrintDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas
import jacksettings
import logs
import render
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

gDBus = DBusObject()
gDBus.loop = None
gDBus.bus  = None
gDBus.a2j  = None
gDBus.jack = None
gDBus.ladish_control = None
gDBus.ladish_studio  = None
gDBus.ladish_room    = None
gDBus.ladish_graph   = None
gDBus.ladish_app_iface  = None
gDBus.ladish_app_daemon = None
gDBus.patchbay = None

# ------------------------------------------------------------------------------------------------------------
# Global JACK object

class JackObject(object):
    __slots__ = [
        'client'
    ]

gJack = JackObject()
gJack.client = None

# ------------------------------------------------------------------------------------------------------------
# Abstract Canvas and JACK Class

class AbstractCanvasJackClass(QMainWindow):
    def __init__(self, parent, appName):
        QMainWindow.__init__(self, parent)

        self.fAppName          = appName
        self.fCurTransportView = TRANSPORT_VIEW_HMS

        self.fLastBPM = None
        self.fLastTransportState = None

        self.fXruns = -1
        self.fBufferSize = 0
        self.fSampleRate = 0.0
        self.fNextSampleRate = 0.0

        self.fLogsW = None

    # -----------------------------------------------------------------
    # JACK Property change calls

    def jack_setBufferSize(self, bufferSize):
        if self.fBufferSize == bufferSize:
            return

        if gJack.client:
            failed = bool(jacklib.set_buffer_size(gJack.client, bufferSize) != 0)
        else:
            failed = bool(jacksettings.setBufferSize(bufferSize))

        if failed:
            print("Failed to change buffer-size as %i, reset to %i" % (bufferSize, self.fBufferSize))
            self.ui_setBufferSize(self.fBufferSize, True)

    def jack_setSampleRate(self, sampleRate):
        if gJack.client:
            # Show change-in-future dialog
            self.ui_setSampleRate(sampleRate, True)
        else:
            # Try to set sampleRate via dbus now
            if jacksettings.setSampleRate(sampleRate):
                self.ui_setSampleRate(sampleRate)

    @pyqtSlot()
    def slot_jackBufferSize_Menu(self):
        text = self.sender().text()
        if text and text.isdigit():
            self.jack_setBufferSize(int(text))

    @pyqtSlot(str)
    def slot_jackBufferSize_ComboBox(self, text):
        if text and text.isdigit():
            self.jack_setBufferSize(int(text))

    @pyqtSlot(str)
    def slot_jackSampleRate_ComboBox(self, text):
        if text and text.isdigit():
            self.jack_setSampleRate(int(text))

    # -----------------------------------------------------------------
    # JACK Transport calls

    def setTransportView(self, view):
        if view == TRANSPORT_VIEW_HMS:
            self.fCurTransportView = TRANSPORT_VIEW_HMS
            self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("00:00:00") + 3)
        elif view == TRANSPORT_VIEW_BBT:
            self.fCurTransportView = TRANSPORT_VIEW_BBT
            self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("000|00|0000") + 3)
        elif view == TRANSPORT_VIEW_FRAMES:
            self.fCurTransportView = TRANSPORT_VIEW_FRAMES
            self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("000'000'000") + 3)
        else:
            self.setTransportView(TRANSPORT_VIEW_HMS)

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, play):
        if not gJack.client:
            return

        if play:
            jacklib.transport_start(gJack.client)
        else:
            jacklib.transport_stop(gJack.client)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if not gJack.client:
            return

        jacklib.transport_stop(gJack.client)
        jacklib.transport_locate(gJack.client, 0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if not gJack.client:
            return

        newFrame = jacklib.get_current_transport_frame(gJack.client) - 100000

        if newFrame < 0:
            newFrame = 0

        jacklib.transport_locate(gJack.client, newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if not gJack.client:
            return

        newFrame = jacklib.get_current_transport_frame(gJack.client) + 100000
        jacklib.transport_locate(gJack.client, newFrame)

    @pyqtSlot()
    def slot_transportViewMenu(self):
        menu = QMenu(self)
        actHMS    = menu.addAction("Hours:Minutes:Seconds")
        actBBT    = menu.addAction("Beat:Bar:Tick")
        actFrames = menu.addAction("Frames")

        actHMS.setCheckable(True)
        actBBT.setCheckable(True)
        actFrames.setCheckable(True)

        if self.fCurTransportView == TRANSPORT_VIEW_HMS:
            actHMS.setChecked(True)
        elif self.fCurTransportView == TRANSPORT_VIEW_BBT:
            actBBT.setChecked(True)
        elif self.fCurTransportView == TRANSPORT_VIEW_FRAMES:
            actFrames.setChecked(True)

        actSelected = menu.exec_(QCursor().pos())

        if actSelected == actHMS:
            self.setTransportView(TRANSPORT_VIEW_HMS)
        elif actSelected == actBBT:
            self.setTransportView(TRANSPORT_VIEW_BBT)
        elif actSelected == actFrames:
            self.setTransportView(TRANSPORT_VIEW_FRAMES)

    # -----------------------------------------------------------------
    # Refresh JACK stuff

    def refreshDSPLoad(self):
        if not gJack.client:
            return

        self.ui_setDSPLoad(int(jacklib.cpu_load(gJack.client)))

    def refreshTransport(self):
        if not gJack.client:
            return

        pos = jacklib.jack_position_t()
        pos.valid = 0

        state = jacklib.transport_query(gJack.client, jacklib.pointer(pos))

        if self.fCurTransportView == TRANSPORT_VIEW_HMS:
            time = pos.frame / int(self.fSampleRate)
            secs = time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60
            self.ui.label_time.setText("%02i:%02i:%02i" % (hrs, mins, secs))

        elif self.fCurTransportView == TRANSPORT_VIEW_BBT:
            if pos.valid & jacklib.JackPositionBBT:
                bar  = pos.bar
                beat = pos.beat if bar != 0 else 0
                tick = pos.tick if bar != 0 else 0
                self.ui.label_time.setText("%03i|%02i|%04i" % (bar, beat, tick))
            else:
                self.ui.label_time.setText("%03i|%02i|%04i" % (0, 0, 0))

        elif self.fCurTransportView == TRANSPORT_VIEW_FRAMES:
            frame1 = pos.frame % 1000
            frame2 = (pos.frame / 1000) % 1000
            frame3 = (pos.frame / 1000000) % 1000
            self.ui.label_time.setText("%03i'%03i'%03i" % (frame3, frame2, frame1))

        if pos.valid & jacklib.JackPositionBBT:
            if self.fLastBPM != pos.beats_per_minute:
                self.ui.sb_bpm.setValue(pos.beats_per_minute)
                self.ui.sb_bpm.setStyleSheet("")
        else:
            pos.beats_per_minute = -1
            if self.fLastBPM != pos.beats_per_minute:
                self.ui.sb_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

        self.fLastBPM = pos.beats_per_minute

        if state != self.fLastTransportState:
            self.fLastTransportState = state

            if state == jacklib.JackTransportStopped:
                icon = getIcon("media-playback-start")
                self.ui.act_transport_play.setChecked(False)
                self.ui.act_transport_play.setIcon(icon)
                self.ui.act_transport_play.setText(self.tr("&Play"))
                self.ui.b_transport_play.setChecked(False)
                self.ui.b_transport_play.setIcon(icon)
            else:
                icon = getIcon("media-playback-pause")
                self.ui.act_transport_play.setChecked(True)
                self.ui.act_transport_play.setIcon(icon)
                self.ui.act_transport_play.setText(self.tr("&Pause"))
                self.ui.b_transport_play.setChecked(True)
                self.ui.b_transport_play.setIcon(icon)

    # -----------------------------------------------------------------
    # Set JACK stuff

    def ui_setBufferSize(self, bufferSize, forced=False):
        if self.fBufferSize == bufferSize and not forced:
            return

        self.fBufferSize = bufferSize

        if bufferSize:
            if bufferSize == 16:
                self.ui.cb_buffer_size.setCurrentIndex(0)
            elif bufferSize == 32:
                self.ui.cb_buffer_size.setCurrentIndex(1)
            elif bufferSize == 64:
                self.ui.cb_buffer_size.setCurrentIndex(2)
            elif bufferSize == 128:
                self.ui.cb_buffer_size.setCurrentIndex(3)
            elif bufferSize == 256:
                self.ui.cb_buffer_size.setCurrentIndex(4)
            elif bufferSize == 512:
                self.ui.cb_buffer_size.setCurrentIndex(5)
            elif bufferSize == 1024:
                self.ui.cb_buffer_size.setCurrentIndex(6)
            elif bufferSize == 2048:
                self.ui.cb_buffer_size.setCurrentIndex(7)
            elif bufferSize == 4096:
                self.ui.cb_buffer_size.setCurrentIndex(8)
            elif bufferSize == 8192:
                self.ui.cb_buffer_size.setCurrentIndex(9)
            else:
                self.ui.cb_buffer_size.setCurrentIndex(-1)

        if self.fAppName == "Catia" and bufferSize:
            for actBufSize in self.ui.act_jack_bf_list:
                actBufSize.setEnabled(True)

                if actBufSize.text().replace("&", "") == str(bufferSize):
                    if not actBufSize.isChecked():
                        actBufSize.setChecked(True)
                elif actBufSize.isChecked():
                    actBufSize.setChecked(False)

    def ui_setSampleRate(self, sampleRate, future=False):
        if self.fSampleRate == sampleRate:
            return

        if future:
            ask = QMessageBox.question(self, self.tr("Change Sample Rate"),
                        self.tr("It's not possible to change Sample Rate while JACK is running.\n"
                                "Do you want to change as soon as JACK stops?"), QMessageBox.Ok | QMessageBox.Cancel)

            if ask == QMessageBox.Ok:
                self.fNextSampleRate = sampleRate
            else:
                self.fNextSampleRate = 0.0

        # not future
        else:
            self.fSampleRate     = sampleRate
            self.fNextSampleRate = 0.0

        for i in range(len(SAMPLE_RATE_LIST)):
            sampleRate = SAMPLE_RATE_LIST[i]

            if self.fSampleRate == sampleRate:
                self.ui.cb_sample_rate.setCurrentIndex(i)

    def ui_setRealTime(self, isRealtime):
        self.ui.label_realtime.setText(" RT " if isRealtime else " <s>RT</s> ")
        self.ui.label_realtime.setEnabled(isRealtime)

    def ui_setDSPLoad(self, dspLoad):
        self.ui.pb_dsp_load.setValue(dspLoad)

    def ui_setXruns(self, xruns):
        txt1 = str(xruns) if (xruns >= 0) else "--"
        txt2 = "" if (xruns == 1) else "s"
        self.ui.b_xruns.setText("%s Xrun%s" % (txt1, txt2))

    # -----------------------------------------------------------------
    # External Dialogs

    @pyqtSlot()
    def slot_showJackSettings(self):
        jacksettingsW = jacksettings.JackSettingsW(self)
        jacksettingsW.exec_()
        del jacksettingsW

        # Force update of gui widgets
        if not gJack.client:
            self.jackStopped()

    @pyqtSlot()
    def slot_showLogs(self):
        if self.fLogsW is None:
            self.fLogsW = logs.LogsW(self)
        self.fLogsW.show()

    @pyqtSlot()
    def slot_showRender(self):
        renderW = render.RenderW(self)
        renderW.exec_()
        del renderW

    # -----------------------------------------------------------------
    # Shared Canvas code

    @pyqtSlot()
    def slot_canvasArrange(self):
        patchcanvas.arrange()

    @pyqtSlot()
    def slot_canvasRefresh(self):
        patchcanvas.clear()
        self.init_ports()

    @pyqtSlot()
    def slot_canvasZoomFit(self):
        self.scene.zoom_fit()

    @pyqtSlot()
    def slot_canvasZoomIn(self):
        self.scene.zoom_in()

    @pyqtSlot()
    def slot_canvasZoomOut(self):
        self.scene.zoom_out()

    @pyqtSlot()
    def slot_canvasZoomReset(self):
        self.scene.zoom_reset()

    @pyqtSlot()
    def slot_canvasPrint(self):
        self.scene.clearSelection()
        self.fExportPrinter = QPrinter()
        dialog = QPrintDialog(self.fExportPrinter, self)

        if dialog.exec_():
            painter = QPainter(self.fExportPrinter)
            painter.save()
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setRenderHint(QPainter.TextAntialiasing)
            self.scene.render(painter)
            painter.restore()

    @pyqtSlot()
    def slot_canvasSaveImage(self):
        newPath = QFileDialog.getSaveFileName(self, self.tr("Save Image"), filter=self.tr("PNG Image (*.png);;JPEG Image (*.jpg)"))

        if newPath:
            self.scene.clearSelection()

            # FIXME - must be a better way...
            if newPath.endswith((".jpg", ".jpG", ".jPG", ".JPG", ".JPg", ".Jpg")):
                imgFormat = "JPG"
            elif newPath.endswith((".png", ".pnG", ".pNG", ".PNG", ".PNg", ".Png")):
                imgFormat = "PNG"
            else:
                # File-dialog may not auto-add the extension
                imgFormat = "PNG"
                newPath  += ".png"

            self.fExportImage = QImage(self.scene.sceneRect().width(), self.scene.sceneRect().height(), QImage.Format_RGB32)
            painter = QPainter(self.fExportImage)
            painter.save()
            painter.setRenderHint(QPainter.Antialiasing) # TODO - set true, cleanup this
            painter.setRenderHint(QPainter.TextAntialiasing)
            self.scene.render(painter)
            self.fExportImage.save(newPath, imgFormat, 100)
            painter.restore()

    # -----------------------------------------------------------------
    # Shared Connections

    def setCanvasConnections(self):
        self.ui.act_canvas_arrange.setEnabled(False) # TODO, later
        self.connect(self.ui.act_canvas_arrange, SIGNAL("triggered()"), SLOT("slot_canvasArrange()"))
        self.connect(self.ui.act_canvas_refresh, SIGNAL("triggered()"), SLOT("slot_canvasRefresh()"))
        self.connect(self.ui.act_canvas_zoom_fit, SIGNAL("triggered()"), SLOT("slot_canvasZoomFit()"))
        self.connect(self.ui.act_canvas_zoom_in, SIGNAL("triggered()"), SLOT("slot_canvasZoomIn()"))
        self.connect(self.ui.act_canvas_zoom_out, SIGNAL("triggered()"), SLOT("slot_canvasZoomOut()"))
        self.connect(self.ui.act_canvas_zoom_100, SIGNAL("triggered()"), SLOT("slot_canvasZoomReset()"))
        self.connect(self.ui.act_canvas_print, SIGNAL("triggered()"), SLOT("slot_canvasPrint()"))
        self.connect(self.ui.act_canvas_save_image, SIGNAL("triggered()"), SLOT("slot_canvasSaveImage()"))
        self.connect(self.ui.b_canvas_zoom_fit, SIGNAL("clicked()"), SLOT("slot_canvasZoomFit()"))
        self.connect(self.ui.b_canvas_zoom_in, SIGNAL("clicked()"), SLOT("slot_canvasZoomIn()"))
        self.connect(self.ui.b_canvas_zoom_out, SIGNAL("clicked()"), SLOT("slot_canvasZoomOut()"))
        self.connect(self.ui.b_canvas_zoom_100, SIGNAL("clicked()"), SLOT("slot_canvasZoomReset()"))

    def setJackConnections(self, modes):
        if "jack" in modes:
            self.connect(self.ui.act_jack_clear_xruns, SIGNAL("triggered()"), SLOT("slot_JackClearXruns()"))
            self.connect(self.ui.act_jack_render, SIGNAL("triggered()"), SLOT("slot_showRender()"))
            self.connect(self.ui.act_jack_configure, SIGNAL("triggered()"), SLOT("slot_showJackSettings()"))
            self.connect(self.ui.b_jack_clear_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))
            self.connect(self.ui.b_jack_configure, SIGNAL("clicked()"), SLOT("slot_showJackSettings()"))
            self.connect(self.ui.b_jack_render, SIGNAL("clicked()"), SLOT("slot_showRender()"))
            self.connect(self.ui.cb_buffer_size, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_jackBufferSize_ComboBox(QString)"))
            self.connect(self.ui.cb_sample_rate, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_jackSampleRate_ComboBox(QString)"))
            self.connect(self.ui.b_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))

        if "buffer-size" in modes:
            self.connect(self.ui.act_jack_bf_16, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_32, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_64, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_128, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_256, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_512, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_1024, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_2048, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_4096, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))
            self.connect(self.ui.act_jack_bf_8192, SIGNAL("triggered(bool)"), SLOT("slot_jackBufferSize_Menu()"))

        if "transport" in modes:
            self.connect(self.ui.act_transport_play, SIGNAL("triggered(bool)"), SLOT("slot_transportPlayPause(bool)"))
            self.connect(self.ui.act_transport_stop, SIGNAL("triggered()"), SLOT("slot_transportStop()"))
            self.connect(self.ui.act_transport_backwards, SIGNAL("triggered()"), SLOT("slot_transportBackwards()"))
            self.connect(self.ui.act_transport_forwards, SIGNAL("triggered()"), SLOT("slot_transportForwards()"))
            self.connect(self.ui.b_transport_play, SIGNAL("clicked(bool)"), SLOT("slot_transportPlayPause(bool)"))
            self.connect(self.ui.b_transport_stop, SIGNAL("clicked()"), SLOT("slot_transportStop()"))
            self.connect(self.ui.b_transport_backwards, SIGNAL("clicked()"), SLOT("slot_transportBackwards()"))
            self.connect(self.ui.b_transport_forwards, SIGNAL("clicked()"), SLOT("slot_transportForwards()"))
            self.connect(self.ui.label_time, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_transportViewMenu()"))

        if "misc" in modes:
            if LINUX:
                self.connect(self.ui.act_show_logs, SIGNAL("triggered()"), SLOT("slot_showLogs()"))
            else:
                self.ui.act_show_logs.setEnabled(False)

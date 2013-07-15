#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK-Capture frontend, with freewheel and transport support
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

from PyQt4.QtCore import pyqtSlot, QProcess, QTime, QTimer, QSettings
from PyQt4.QtGui import QDialog
from time import sleep

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_render
from shared import *
from jacklib_helpers import *

# ------------------------------------------------------------------------------------------------------------
# Global variables

global gJackCapturePath, gJackClient
gJackCapturePath = ""
gJackClient      = None

# ------------------------------------------------------------------------------------------------------------
# Find 'jack_capture'

# Check for cxfreeze
if sys.path[0].rsplit(os.sep, 1)[-1] in ("catia", "claudia", "cadence-render", "render"):
    name = sys.path[0].rsplit(os.sep, 1)[-1]
    cwd  = sys.path[0].rsplit(name, 1)[0]
    if os.path.exists(os.path.join(cwd, "jack_capture")):
        gJackCapturePath = os.path.join(cwd, "jack_capture")
    del cwd, name

# Check in PATH
if not gJackCapturePath:
    for iPATH in PATH:
        if os.path.exists(os.path.join(iPATH, "jack_capture")):
            gJackCapturePath = os.path.join(iPATH, "jack_capture")
            break

def canRender():
    global gJackCapturePath
    return bool(gJackCapturePath)

# ------------------------------------------------------------------------------------------------------------
# Render Window

class RenderW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_render.Ui_RenderW()
        self.ui.setupUi(self)

        self.fFreewheel = False
        self.fLastTime  = -1
        self.fMaxTime   = 180

        self.fTimer   = QTimer(self)
        self.fProcess = QProcess(self)

        # -------------------------------------------------------------
        # Get JACK client and base information

        global gJackClient

        if gJackClient:
            self.fJackClient = gJackClient
        else:
            self.fJackClient = jacklib.client_open("Render-Dialog", jacklib.JackNoStartServer, None)

        self.fBufferSize = int(jacklib.get_buffer_size(self.fJackClient))
        self.fSampleRate = int(jacklib.get_sample_rate(self.fJackClient))

        for i in range(self.ui.cb_buffer_size.count()):
            if int(self.ui.cb_buffer_size.itemText(i)) == self.fBufferSize:
                self.ui.cb_buffer_size.setCurrentIndex(i)
                break
        else:
            self.ui.cb_buffer_size.addItem(str(self.fBufferSize))
            self.ui.cb_buffer_size.setCurrentIndex(self.ui.cb_buffer_size.count() - 1)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        # Get List of formats
        self.fProcess.start(gJackCapturePath, ["-pf"])
        self.fProcess.waitForFinished()

        formats     = str(self.fProcess.readAllStandardOutput(), encoding="utf-8").split(" ")
        formatsList = []

        for i in range(len(formats) - 1):
            iFormat = formats[i].strip()
            if iFormat:
                formatsList.append(iFormat)

        formatsList.sort()

        # Put all formats in combo-box, select 'wav' option
        for i in range(len(formatsList)):
            self.ui.cb_format.addItem(formatsList[i])
            if formatsList[i] == "wav":
                self.ui.cb_format.setCurrentIndex(i)

        self.ui.cb_depth.setCurrentIndex(4) #Float
        self.ui.rb_stereo.setChecked(True)

        self.ui.te_end.setTime(QTime(0, 3, 0))
        self.ui.progressBar.setFormat("")
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(1)
        self.ui.progressBar.setValue(0)

        self.ui.b_render.setIcon(getIcon("media-record"))
        self.ui.b_stop.setIcon(getIcon("media-playback-stop"))
        self.ui.b_close.setIcon(getIcon("window-close"))
        self.ui.b_open.setIcon(getIcon("document-open"))
        self.ui.b_stop.setVisible(False)
        self.ui.le_folder.setText(HOME)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self.ui.b_render, SIGNAL("clicked()"), SLOT("slot_renderStart()"))
        self.connect(self.ui.b_stop, SIGNAL("clicked()"), SLOT("slot_renderStop()"))
        self.connect(self.ui.b_open, SIGNAL("clicked()"), SLOT("slot_getAndSetPath()"))
        self.connect(self.ui.b_now_start, SIGNAL("clicked()"), SLOT("slot_setStartNow()"))
        self.connect(self.ui.b_now_end, SIGNAL("clicked()"), SLOT("slot_setEndNow()"))
        self.connect(self.ui.te_start, SIGNAL("timeChanged(const QTime)"), SLOT("slot_updateStartTime(const QTime)"))
        self.connect(self.ui.te_end, SIGNAL("timeChanged(const QTime)"), SLOT("slot_updateEndTime(const QTime)"))
        self.connect(self.fTimer, SIGNAL("timeout()"), SLOT("slot_updateProgressbar()"))

        # -------------------------------------------------------------

        self.loadSettings()

    @pyqtSlot()
    def slot_renderStart(self):
        if not os.path.exists(self.ui.le_folder.text()):
            QMessageBox.warning(self, self.tr("Warning"), self.tr("The selected directory does not exist. Please choose a valid one."))
            return

        timeStart = self.ui.te_start.time()
        timeEnd   = self.ui.te_end.time()
        minTime   = (timeStart.hour() * 3600) + (timeStart.minute() * 60) + (timeStart.second())
        maxTime   = (timeEnd.hour() * 3600) + (timeEnd.minute() * 60) + (timeEnd.second())

        newBufferSize = int(self.ui.cb_buffer_size.currentText())
        useTransport  = self.ui.group_time.isChecked()

        self.fFreewheel = bool(self.ui.cb_render_mode.currentIndex() == 1)
        self.fLastTime  = -1
        self.fMaxTime   = maxTime

        if self.fFreewheel:
            self.fTimer.setInterval(100)
        else:
            self.fTimer.setInterval(500)

        self.ui.group_render.setEnabled(False)
        self.ui.group_time.setEnabled(False)
        self.ui.group_encoding.setEnabled(False)
        self.ui.b_render.setVisible(False)
        self.ui.b_stop.setVisible(True)
        self.ui.b_close.setEnabled(False)

        if useTransport:
            self.ui.progressBar.setFormat("%p%")
            self.ui.progressBar.setMinimum(minTime)
            self.ui.progressBar.setMaximum(maxTime)
            self.ui.progressBar.setValue(minTime)
        else:
            self.ui.progressBar.setFormat("")
            self.ui.progressBar.setMinimum(0)
            self.ui.progressBar.setMaximum(0)
            self.ui.progressBar.setValue(0)

        self.ui.progressBar.update()

        arguments = []

        # Filename prefix
        arguments.append("-fp")
        arguments.append(self.ui.le_prefix.text())

        # Format
        arguments.append("-f")
        arguments.append(self.ui.cb_format.currentText())

        # Bit depth
        arguments.append("-b")
        arguments.append(self.ui.cb_depth.currentText())

        # Channels
        arguments.append("-c")
        if self.ui.rb_mono.isChecked():
            arguments.append("1")
        elif self.ui.rb_stereo.isChecked():
            arguments.append("2")
        else:
            arguments.append(str(self.ui.sb_channels.value()))

        # Controlled by transport
        if useTransport:
            arguments.append("-jt")

        # Silent mode
        arguments.append("-dc")
        arguments.append("-s")

        # Change current directory
        os.chdir(self.ui.le_folder.text())

        if newBufferSize != int(jacklib.get_buffer_size(self.fJackClient)):
            print("NOTICE: buffer size changed before render")
            jacklib.set_buffer_size(self.fJackClient, newBufferSize)

        if useTransport:
            if jacklib.transport_query(self.fJackClient, None) > jacklib.JackTransportStopped: # rolling or starting
                jacklib.transport_stop(self.fJackClient)

            jacklib.transport_locate(self.fJackClient, minTime * self.fSampleRate)

        self.fProcess.start(gJackCapturePath, arguments)
        self.fProcess.waitForStarted()

        if self.fFreewheel:
            print("NOTICE: rendering in freewheel mode")
            sleep(1)
            jacklib.set_freewheel(self.fJackClient, 1)

        if useTransport:
            self.fTimer.start()
            jacklib.transport_start(self.fJackClient)

    @pyqtSlot()
    def slot_renderStop(self):
        useTransport = self.ui.group_time.isChecked()

        if useTransport:
            jacklib.transport_stop(self.fJackClient)

        if self.fFreewheel:
            jacklib.set_freewheel(self.fJackClient, 0)
            sleep(1)

        self.fProcess.close()

        if useTransport:
            self.fTimer.stop()

        self.ui.group_render.setEnabled(True)
        self.ui.group_time.setEnabled(True)
        self.ui.group_encoding.setEnabled(True)
        self.ui.b_render.setVisible(True)
        self.ui.b_stop.setVisible(False)
        self.ui.b_close.setEnabled(True)

        self.ui.progressBar.setFormat("")
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(1)
        self.ui.progressBar.setValue(0)
        self.ui.progressBar.update()

        # Restore buffer size
        newBufferSize = int(jacklib.get_buffer_size(self.fJackClient))

        if newBufferSize != self.fBufferSize:
            jacklib.set_buffer_size(self.fJackClient, newBufferSize)

    @pyqtSlot()
    def slot_getAndSetPath(self):
        getAndSetPath(self, self.ui.le_folder.text(), self.ui.le_folder)

    @pyqtSlot()
    def slot_setStartNow(self):
        time = int(jacklib.get_current_transport_frame(self.fJackClient) / self.fSampleRate)
        secs = time % 60
        mins = int(time / 60) % 60
        hrs  = int(time / 3600) % 60
        self.ui.te_start.setTime(QTime(hrs, mins, secs))

    @pyqtSlot()
    def slot_setEndNow(self):
        time = int(jacklib.get_current_transport_frame(self.fJackClient) / self.fSampleRate)
        secs = time % 60
        mins = int(time / 60) % 60
        hrs  = int(time / 3600) % 60
        self.ui.te_end.setTime(QTime(hrs, mins, secs))

    @pyqtSlot(QTime)
    def slot_updateStartTime(self, time):
        if time >= self.ui.te_end.time():
            self.ui.te_end.setTime(time)
            self.ui.b_render.setEnabled(False)
        else:
            self.ui.b_render.setEnabled(True)

    @pyqtSlot(QTime)
    def slot_updateEndTime(self, time):
        if time <= self.ui.te_start.time():
            self.ui.te_start.setTime(time)
            self.ui.b_render.setEnabled(False)
        else:
            self.ui.b_render.setEnabled(True)

    @pyqtSlot()
    def slot_updateProgressbar(self):
        time = int(jacklib.get_current_transport_frame(self.fJackClient)) / self.fSampleRate
        self.ui.progressBar.setValue(time)

        if time > self.fMaxTime or (self.fLastTime > time and not self.fFreewheel):
            self.slot_renderStop()

        self.fLastTime = time

    def saveSettings(self):
        settings = QSettings("Cadence", "Cadence-Render")

        if self.ui.rb_mono.isChecked():
            channels = 1
        elif self.ui.rb_stereo.isChecked():
            channels = 2
        else:
            channels = self.ui.sb_channels.value()

        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("OutputFolder", self.ui.le_folder.text())
        settings.setValue("FilenamePrefix", self.ui.le_prefix.text())
        settings.setValue("EncodingFormat", self.ui.cb_format.currentText())
        settings.setValue("EncodingDepth", self.ui.cb_depth.currentText())
        settings.setValue("EncodingChannels", channels)
        settings.setValue("UseTransport", self.ui.group_time.isChecked())
        settings.setValue("StartTime", self.ui.te_start.time())
        settings.setValue("EndTime", self.ui.te_end.time())

    def loadSettings(self):
        settings = QSettings("Cadence", "Cadence-Render")

        self.restoreGeometry(settings.value("Geometry", ""))

        outputFolder = settings.value("OutputFolder", HOME)

        if os.path.exists(outputFolder):
            self.ui.le_folder.setText(outputFolder)

        self.ui.le_prefix.setText(settings.value("FilenamePrefix", "jack_capture_"))

        encFormat = settings.value("EncodingFormat", "Wav", type=str)

        for i in range(self.ui.cb_format.count()):
            if self.ui.cb_format.itemText(i) == encFormat:
                self.ui.cb_format.setCurrentIndex(i)
                break

        encDepth = settings.value("EncodingDepth", "Float", type=str)

        for i in range(self.ui.cb_depth.count()):
            if self.ui.cb_depth.itemText(i) == encDepth:
                self.ui.cb_depth.setCurrentIndex(i)
                break

        encChannels = settings.value("EncodingChannels", 2, type=int)

        if encChannels == 1:
            self.ui.rb_mono.setChecked(True)
        elif encChannels == 2:
            self.ui.rb_stereo.setChecked(True)
        else:
            self.ui.rb_outro.setChecked(True)
            self.ui.sb_channels.setValue(encChannels)

        self.ui.group_time.setChecked(settings.value("UseTransport", False, type=bool))
        self.ui.te_start.setTime(settings.value("StartTime", self.ui.te_start.time(), type=QTime))
        self.ui.te_end.setTime(settings.value("EndTime", self.ui.te_end.time(), type=QTime))

    def closeEvent(self, event):
        self.saveSettings()

        if self.fJackClient:
            jacklib.client_close(self.fJackClient)

        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Allow to use this as a standalone app

if __name__ == '__main__':
    # Additional imports
    from PyQt4.QtGui import QApplication

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Cadence-Render")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/cadence.svg"))

    if jacklib is None:
        QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW",
            "JACK is not available in this system, cannot use this application."))
        sys.exit(1)

    if not gJackCapturePath:
        QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW",
            "The 'jack_capture' application is not available.\n"
            "Is not possible to render without it!"))
        sys.exit(2)

    jackStatus  = jacklib.jack_status_t(0x0)
    gJackClient = jacklib.client_open("Render", jacklib.JackNoStartServer, jacklib.pointer(jackStatus))

    if not gJackClient:
        errorString = get_jack_status_error_string(jackStatus)
        QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW",
            "Could not connect to JACK, possible reasons:\n"
            "%s" % errorString))
        sys.exit(1)

    # Show GUI
    gui = RenderW(None)
    gui.setWindowIcon(getIcon("media-record", 48))
    gui.show()

    # App-Loop
    sys.exit(app.exec_())

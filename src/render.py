#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK-Capture frontend, with freewheel and transport support
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

from PyQt4.QtCore import pyqtSlot, QProcess, QTime, QTimer
from PyQt4.QtGui import QDialog
from time import sleep

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_render
from shared import *
from jacklib_helpers import *

# ------------------------------------------------------------------------------------------------------------
# Global JACK client (used in standalone mode)

global jackClient
jackClient = None

# ------------------------------------------------------------------------------------------------------------
# Render Window

class RenderW(QDialog, ui_render.Ui_RenderW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Get JACK client and base information

        global jackClient
        if jackClient:
            self.m_jackClient = jackClient
        else:
            self.m_jackClient = jacklib.client_open("Render-Dialog", jacklib.JackNoStartServer, None)

        self.m_bufferSize = int(jacklib.get_buffer_size(self.m_jackClient))
        self.m_sampleRate = int(jacklib.get_sample_rate(self.m_jackClient))

        for i in range(self.cb_buffer_size.count()):
            if int(self.cb_buffer_size.itemText(i)) == self.m_bufferSize:
                self.cb_buffer_size.setCurrentIndex(i)
                break
        else:
            self.cb_buffer_size.addItem(str(self.m_bufferSize))
            self.cb_buffer_size.setCurrentIndex(self.cb_buffer_size.count() - 1)

        # -------------------------------------------------------------
        # Internal stuff

        self.m_lastTime  = 0
        self.m_maxTime   = 180
        self.m_freewheel = False

        self.m_timer   = QTimer(self)
        self.m_process = QProcess(self)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        # Get List of formats
        self.m_process.start("jack_capture", ["-pf"])
        self.m_process.waitForFinished()

        formats     = str(self.m_process.readAllStandardOutput(), encoding="utf-8").split(" ")
        formatsList = []

        for i in range(len(formats) - 1):
            iFormat = formats[i].strip()
            if iFormat:
                formatsList.append(iFormat)

        formatsList.sort()

        # Put all formats in combo-box, select 'wav' option
        for i in range(len(formatsList)):
            self.cb_format.addItem(formatsList[i])
            if formatsList[i] == "wav":
                self.cb_format.setCurrentIndex(i)

        self.cb_depth.setCurrentIndex(4) #Float
        self.rb_stereo.setChecked(True)

        self.te_end.setTime(QTime(0, 3, 0))
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(0)
        self.progressBar.setValue(0)

        self.b_render.setIcon(getIcon("media-record"))
        self.b_stop.setIcon(getIcon("media-playback-stop"))
        self.b_close.setIcon(getIcon("window-close"))
        self.b_open.setIcon(getIcon("document-open"))
        self.b_stop.setVisible(False)
        self.le_folder.setText(HOME)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self.b_render, SIGNAL("clicked()"), SLOT("slot_renderStart()"))
        self.connect(self.b_stop, SIGNAL("clicked()"), SLOT("slot_renderStop()"))
        self.connect(self.b_open, SIGNAL("clicked()"), SLOT("slot_getAndSetPath()"))
        self.connect(self.b_now_start, SIGNAL("clicked()"), SLOT("slot_setStartNow()"))
        self.connect(self.b_now_end, SIGNAL("clicked()"), SLOT("slot_setEndNow()"))
        self.connect(self.te_start, SIGNAL("timeChanged(const QTime)"), SLOT("slot_updateStartTime(const QTime)"))
        self.connect(self.te_end, SIGNAL("timeChanged(const QTime)"), SLOT("slot_updateEndTime(const QTime)"))
        self.connect(self.m_timer, SIGNAL("timeout()"), SLOT("slot_updateProgressbar()"))

    @pyqtSlot()
    def slot_renderStart(self):
        if not os.path.exists(self.le_folder.text()):
            QMessageBox.warning(self, self.tr("Warning"), self.tr("The selected directory does not exist. Please choose a valid one."))
            return

        self.group_render.setEnabled(False)
        self.group_time.setEnabled(False)
        self.group_encoding.setEnabled(False)
        self.b_render.setVisible(False)
        self.b_stop.setVisible(True)
        self.b_close.setEnabled(False)

        timeStart = self.te_start.time()
        timeEnd   = self.te_end.time()
        minTime   = (timeStart.hour() * 3600) + (timeStart.minute() * 60) + (timeStart.second())
        maxTime   = (timeEnd.hour() * 3600) + (timeEnd.minute() * 60) + (timeEnd.second())

        newBufferSize = int(self.cb_buffer_size.currentText())

        self.m_maxTime   = maxTime
        self.m_freewheel = bool(self.cb_render_mode.currentIndex() == 1)

        self.progressBar.setMinimum(minTime)
        self.progressBar.setMaximum(maxTime)
        self.progressBar.setValue(minTime)
        self.progressBar.update()

        if self.m_freewheel:
            self.m_timer.setInterval(100)
        else:
            self.m_timer.setInterval(500)

        arguments = []

        # Bit depth
        arguments.append("-b")
        arguments.append(self.cb_depth.currentText())

        # Channels
        arguments.append("-c")
        if self.rb_mono.isChecked():
            arguments.append("1")
        elif self.rb_stereo.isChecked():
            arguments.append("2")
        else:
            arguments.append(str(self.sb_channels.value()))

        # Format
        arguments.append("-f")
        arguments.append(self.cb_format.currentText())

        # Controlled by transport
        arguments.append("-jt")

        # Silent mode
        arguments.append("-dc")
        arguments.append("-s")

        # Change current directory
        os.chdir(self.le_folder.text())

        if newBufferSize != int(jacklib.get_buffer_size(self.m_jackClient)):
            print("NOTICE: buffer size changed before render")
            jacklib.set_buffer_size(self.m_jackClient, newBufferSize)

        if jacklib.transport_query(self.m_jackClient, None) > jacklib.JackTransportStopped: # > JackTransportStopped is rolling|starting
            jacklib.transport_stop(self.m_jackClient)

        jacklib.transport_locate(self.m_jackClient, minTime * self.m_sampleRate)
        self.m_last_time = -1

        self.m_process.start("jack_capture", arguments)
        self.m_process.waitForStarted()

        if self.m_freewheel:
            print("NOTICE: rendering in freewheel mode")
            sleep(1)
            jacklib.set_freewheel(self.m_jackClient, 1)

        self.m_timer.start()
        jacklib.transport_start(self.m_jackClient)

    @pyqtSlot()
    def slot_renderStop(self):
        jacklib.transport_stop(self.m_jackClient)

        if self.m_freewheel:
            jacklib.set_freewheel(self.m_jackClient, 0)

        sleep(1)

        self.m_process.close()
        self.m_timer.stop()

        self.group_render.setEnabled(True)
        self.group_time.setEnabled(True)
        self.group_encoding.setEnabled(True)
        self.b_render.setVisible(True)
        self.b_stop.setVisible(False)
        self.b_close.setEnabled(True)

        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(0)
        self.progressBar.setValue(0)
        self.progressBar.update()

        # Restore buffer size
        newBufferSize = int(jacklib.get_buffer_size(self.m_jackClient))
        if newBufferSize != self.m_bufferSize:
            jacklib.set_buffer_size(self.m_jackClient, newBufferSize)

    @pyqtSlot()
    def slot_getAndSetPath(self):
        getAndSetPath(self, self.le_folder.text(), self.le_folder)

    @pyqtSlot()
    def slot_setStartNow(self):
        time = int(jacklib.get_current_transport_frame(self.m_jackClient) / self.m_sampleRate)
        secs = time % 60
        mins = int(time / 60) % 60
        hrs  = int(time / 3600) % 60
        self.te_start.setTime(QTime(hrs, mins, secs))

    @pyqtSlot()
    def slot_setEndNow(self):
        time = int(jacklib.get_current_transport_frame(self.m_jackClient) / self.m_sampleRate)
        secs = time % 60
        mins = int(time / 60) % 60
        hrs  = int(time / 3600) % 60
        self.te_end.setTime(QTime(hrs, mins, secs))

    @pyqtSlot(QTime)
    def slot_updateStartTime(self, time):
        if time >= self.te_end.time():
            self.te_end.setTime(time)
            self.b_render.setEnabled(False)
        else:
            self.b_render.setEnabled(True)

    @pyqtSlot(QTime)
    def slot_updateEndTime(self, time):
        if time <= self.te_start.time():
            self.te_start.setTime(time)
            self.b_render.setEnabled(False)
        else:
            self.b_render.setEnabled(True)

    @pyqtSlot()
    def slot_updateProgressbar(self):
        time = int(jacklib.get_current_transport_frame(self.m_jackClient)) / self.m_sampleRate
        self.progressBar.setValue(time)

        if time > self.m_maxTime or (self.m_lastTime > time and not self.m_freewheel):
            self.slot_renderStop()

        self.m_last_time = time

    def closeEvent(self, event):
        if self.m_jackClient:
            jacklib.client_close(self.m_jackClient)
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

    for iPATH in PATH:
        if os.path.exists(os.path.join(iPATH, "jack_capture")):
            break
    else:
        QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW",
            "The 'jack_capture' application is not available.\n"
            "Is not possible to render without it!"))
        sys.exit(1)

    jackStatus = jacklib.jack_status_t(0)
    jackClient = jacklib.client_open("Render", jacklib.JackNoStartServer, jacklib.pointer(jackStatus))

    if not jackClient:
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

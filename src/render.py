#!/usr/bin/env python
# -*- coding: utf-8 -*-

# JACK-Capture frontend, with freewheel and transport support
# Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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
from PyQt4.QtCore import pyqtSlot, Qt, QProcess, QTime, QTimer
from PyQt4.QtGui import QDialog
from time import sleep

# Imports (Custom Stuff)
import ui_render
from shared import *
from jacklib_helpers import *

global jack_client
jack_client = None

# Render Window
class RenderW(QDialog, ui_render.Ui_RenderW):
    def __init__(self, parent, flags):
        QDialog.__init__(self, parent, flags)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Get JACK client and base information

        global jack_client
        if (jack_client):
          self.m_jack_client = jack_client
          self.m_closeClient = False
        else:
          self.m_jack_client = jacklib.client_open("Render-Dialog", jacklib.JackNoStartServer, None)
          self.m_closeClient = True

        self.m_buffer_size = jacklib.get_buffer_size(self.m_jack_client)
        for i in range(self.cb_buffer_size.count()):
          if (int(self.cb_buffer_size.itemText(i)) == self.m_buffer_size):
            self.cb_buffer_size.setCurrentIndex(i)

        self.m_sample_rate = jacklib.get_sample_rate(self.m_jack_client)

        # -------------------------------------------------------------
        # Internal stuff

        self.m_max_time  = 180
        self.m_last_time = 0
        self.m_freewheel = False

        self.m_timer = QTimer(self)
        self.m_process = QProcess(self)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        # Get List of formats
        self.m_process.start("jack_capture", ["-pf"])
        self.m_process.waitForFinished()

        formats = str(self.m_process.readAllStandardOutput(), encoding="ascii").split(" ")
        for i in range(len(formats)-1):
          self.cb_format.addItem(formats[i])
          if (formats[i] == "wav"):
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
        if (os.path.exists(self.le_folder.text()) == False):
          QMessageBox.warning(self, self.tr("Warning"), self.tr("The selected directory does not exist. Please choose a valid one."))
          return

        self.group_render.setEnabled(False)
        self.group_time.setEnabled(False)
        self.group_encoding.setEnabled(False)
        self.b_render.setVisible(False)
        self.b_stop.setVisible(True)
        self.b_close.setEnabled(False)

        self.m_freewheel = (self.cb_render_mode.currentIndex() == 1)
        new_buffer_size = int(self.cb_buffer_size.currentText())

        time_start = self.te_start.time()
        time_end   = self.te_end.time()
        min_time = (time_start.hour()*3600)+(time_start.minute()*60)+(time_start.second())
        max_time = (time_end.hour()*3600)+(time_end.minute()*60)+(time_end.second())
        self.m_max_time = max_time

        self.progressBar.setMinimum(min_time)
        self.progressBar.setMaximum(max_time)
        self.progressBar.setValue(min_time)
        self.progressBar.update()

        if (self.m_freewheel):
          self.m_timer.setInterval(100)
        else:
          self.m_timer.setInterval(500)

        arguments = []

        # Bit depth
        arguments.append("-b")
        arguments.append(self.cb_depth.currentText())

        # Channels
        arguments.append("-c")
        if (self.rb_mono.isChecked()):
          arguments.append("1")
        elif (self.rb_stereo.isChecked()):
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

        if (new_buffer_size != jacklib.get_buffer_size(self.m_jack_client)):
          print("NOTICE: buffer size changed before render")
          jacklib.set_buffer_size(self.m_jack_client, new_buffer_size)

        if (jacklib.transport_query(self.m_jack_client, None) > jacklib.JackTransportStopped): # >TransportStopped is rolling/starting
          jacklib.transport_stop(self.m_jack_client)

        jacklib.transport_locate(self.m_jack_client, min_time*self.m_sample_rate)
        self.m_last_time = -1

        self.m_process.start("jack_capture", arguments)
        self.m_process.waitForStarted()

        if (self.m_freewheel):
          sleep(1)
          print("NOTICE: rendering in freewheel mode")
          jacklib.set_freewheel(jack_client, 1)

        self.m_timer.start()
        jacklib.transport_start(self.m_jack_client)

    @pyqtSlot()
    def slot_renderStop(self):
        jacklib.transport_stop(self.m_jack_client)

        if (self.m_freewheel):
          jacklib.set_freewheel(self.m_jack_client, 0)

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
        new_buffer_size = jacklib.get_buffer_size(self.m_jack_client)
        if (new_buffer_size != self.m_buffer_size):
          jacklib.set_buffer_size(self.m_jack_client, new_buffer_size)

    @pyqtSlot()
    def slot_getAndSetPath(self):
        getAndSetPath(self, self.le_folder.text(), self.le_folder)

    @pyqtSlot()
    def slot_setStartNow(self):
        time = jacklib.get_current_transport_frame(self.m_jack_client)/self.m_sample_rate
        secs = time % 60
        mins = (time / 60) % 60
        hrs  = (time / 3600) % 60
        self.te_start.setTime(QTime(hrs, mins, secs))

    @pyqtSlot()
    def slot_setEndNow(self):
        time = jacklib.get_current_transport_frame(self.m_jack_client)/self.m_sample_rate
        secs = time % 60
        mins = (time / 60) % 60
        hrs  = (time / 3600) % 60
        self.te_end.setTime(QTime(hrs, mins, secs))

    @pyqtSlot(QTime)
    def slot_updateStartTime(self, time):
        if (time >= self.te_end.time()):
          self.te_end.setTime(time)
          self.b_render.setEnabled(False)
        else:
          self.b_render.setEnabled(True)

    @pyqtSlot(QTime)
    def slot_updateEndTime(self, time):
        if (time <= self.te_start.time()):
          time = self.te_start.setTime(time)
          self.b_render.setEnabled(False)
        else:
          self.b_render.setEnabled(True)

    @pyqtSlot()
    def slot_updateProgressbar(self):
        time = jacklib.get_current_transport_frame(self.m_jack_client)/self.m_sample_rate
        self.progressBar.setValue(time)

        if (time > self.m_max_time or (self.m_last_time > time and self.m_freewheel == False)):
          self.slot_renderStop()

        self.m_last_time = time

    def closeEvent(self, event):
        if (self.m_closeClient):
          jacklib.client_close(self.m_jack_client)
        QDialog.closeEvent(self, event)

# -------------------------------------------------------------
# Allow to use this as a standalone app
if __name__ == '__main__':

    # Additional imports
    from PyQt4.QtGui import QApplication

    # App initialization
    app = QApplication(sys.argv)

    for iPATH in PATH:
      if os.path.exists(os.path.join(iPATH, "jack_capture")):
        break
    else:
      QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW", "The 'jack_capture' application is not available.\nIs not possible to render without it!"))
      sys.exit(1)

    jack_status = jacklib.jack_status_t(0)
    jack_client = jacklib.client_open("Render", jacklib.JackNoStartServer, jacklib.pointer(jack_status))

    if not jack_client:
      QMessageBox.critical(None, app.translate("RenderW", "Error"), app.translate("RenderW", "Could not connect to JACK, possible errors:\n%s" % (get_jack_status_error_string(jack_status))))
      sys.exit(1)

    # Show GUI
    gui = RenderW(None, Qt.WindowFlags())
    gui.setWindowIcon(getIcon("media-record", 48))
    gui.show()

    # App-Loop
    ret = app.exec_()

    if (jack_client):
      jacklib.client_close(jack_client)

    # Exit properly
    sys.exit(ret)

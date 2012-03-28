#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# XY Controller for JACK, using jacklib
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
from PyQt4.QtCore import pyqtSlot, Qt, QPointF, QRectF, QSettings, QTimer, QVariant
from PyQt4.QtGui import QApplication, QColor, QIcon, QPainter, QPen, QGraphicsItem, QGraphicsScene, QMainWindow
from queue import Queue, Empty as QuequeEmpty

# Imports (Custom)
import ui_xycontroller
from shared import *
from jacklib_helpers import *

# Globals
global jack_client, jack_midi_in_port, jack_midi_out_port, jack_midi_in_data, jack_midi_out_data
jack_client = None
jack_midi_in_port  = None
jack_midi_out_port = None
jack_midi_in_data  = Queue(512)
jack_midi_out_data = Queue(512)

# XY Controller Scene
class XYGraphicsScene(QGraphicsScene):
    def __init__(self, parent):
        QGraphicsScene.__init__(self, parent)

        self.cc_x = 1
        self.cc_y = 2
        self.m_channels = []
        self.m_mouseLock = False
        self.m_smooth = False
        self.m_smooth_x = 0
        self.m_smooth_y = 0

        self.setBackgroundBrush(Qt.black)

        cursorPen = QPen(QColor(255,255,255), 2)
        cursorBrush = QColor(255,255,255,50)
        self.m_cursor = self.addEllipse(QRectF(-10, -10, 20, 20), cursorPen, cursorBrush)

        linePen = QPen(QColor(200,200,200,100), 1, Qt.DashLine)
        self.m_lineH = self.addLine(-9999, 0, 9999, 0, linePen)
        self.m_lineV = self.addLine(0, -9999, 0, 9999, linePen)

        self.p_size = QRectF(-100, -100, 100, 100)

    def setControlX(self, x):
        self.cc_x = x

    def setControlY(self, y):
        self.cc_y = y

    def setChannels(self, channels):
        self.m_channels = channels

    def setPosX(self, x, forward=True):
        if (self.m_mouseLock == False):
          pos_x = x*(self.p_size.x()+self.p_size.width())
          self.m_cursor.setPos(pos_x, self.m_cursor.y())
          self.m_lineV.setX(pos_x)

          if (forward):
            self.sendMIDI(pos_x/(self.p_size.x()+self.p_size.width()), None)
          else:
            self.m_smooth_x = pos_x

    def setPosY(self, y, forward=True):
        if (self.m_mouseLock == False):
          pos_y = y*(self.p_size.y()+self.p_size.height())
          self.m_cursor.setPos(self.m_cursor.x(), pos_y)
          self.m_lineH.setY(pos_y)

          if (forward):
            self.sendMIDI(None, pos_y/(self.p_size.y()+self.p_size.height()))
          else:
            self.m_smooth_y = pos_y

    def setSmooth(self, smooth):
        self.m_smooth = smooth

    def setSmoothValues(self, x, y):
        self.m_smooth_x = x*(self.p_size.x()+self.p_size.width())
        self.m_smooth_y = y*(self.p_size.y()+self.p_size.height())

    def handleCC(self, param, value):
        sendUpdate = False

        if (param == self.cc_x):
          sendUpdate = True
          xp = (float(value)/63)-1.0
          yp = self.m_cursor.y()/(self.p_size.y()+self.p_size.height())

          if (xp < -1.0):
            xp = -1.0
          elif (xp > 1.0):
            xp = 1.0

          self.setPosX(xp, False)

        if (param == self.cc_y):
          sendUpdate = True
          xp = self.m_cursor.x()/(self.p_size.x()+self.p_size.width())
          yp = (float(value)/63)-1.0

          if (yp < -1.0):
            yp = -1.0
          elif (yp > 1.0):
            yp = 1.0

          self.setPosY(yp, False)

        if (sendUpdate):
          self.emit(SIGNAL("cursorMoved(double, double)"), xp, yp)

    def handleMousePos(self, pos):
        if (not self.p_size.contains(pos)):
          if (pos.x() < self.p_size.x()):
            pos.setX(self.p_size.x())
          elif (pos.x() > self.p_size.x()+self.p_size.width()):
            pos.setX(self.p_size.x()+self.p_size.width())

          if (pos.y() < self.p_size.y()):
            pos.setY(self.p_size.y())
          elif (pos.y() > self.p_size.y()+self.p_size.height()):
            pos.setY(self.p_size.y()+self.p_size.height())

        self.m_smooth_x = pos.x()
        self.m_smooth_y = pos.y()

        if (self.m_smooth == False):
          self.m_cursor.setPos(pos)
          self.m_lineH.setY(pos.y())
          self.m_lineV.setX(pos.x())

          xp = pos.x()/(self.p_size.x()+self.p_size.width())
          yp = pos.y()/(self.p_size.y()+self.p_size.height())

          self.sendMIDI(xp, yp)
          self.emit(SIGNAL("cursorMoved(double, double)"), xp, yp)

    def sendMIDI(self, xp=None, yp=None):
        global jack_midi_out_data
        rate = float(0xff)/4

        if (xp != None):
          value = int((xp*rate)+rate)
          for channel in self.m_channels:
            jack_midi_out_data.put_nowait((0xB0+channel-1, self.cc_x, value))

        if (yp != None):
          value = int((yp*rate)+rate)
          for channel in self.m_channels:
            jack_midi_out_data.put_nowait((0xB0+channel-1, self.cc_y, value))

    def updateSize(self, size):
        self.p_size.setRect(-(size.width()/2), -(size.height()/2), size.width(), size.height())

    def updateSmooth(self):
        if (self.m_smooth):
          if (self.m_cursor.x() != self.m_smooth_x or self.m_cursor.y() != self.m_smooth_y):
            new_x = (self.m_smooth_x+self.m_cursor.x()*3)/4
            new_y = (self.m_smooth_y+self.m_cursor.y()*3)/4
            pos = QPointF(new_x, new_y)

            self.m_cursor.setPos(pos)
            self.m_lineH.setY(pos.y())
            self.m_lineV.setX(pos.x())

            xp = pos.x()/(self.p_size.x()+self.p_size.width())
            yp = pos.y()/(self.p_size.y()+self.p_size.height())

            self.sendMIDI(xp, yp)
            self.emit(SIGNAL("cursorMoved(double, double)"), xp, yp)

    def keyPressEvent(self, event):
        event.accept()

    def wheelEvent(self, event):
        event.accept()

    def mousePressEvent(self, event):
        self.m_mouseLock = True
        self.handleMousePos(event.scenePos())
        QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        self.handleMousePos(event.scenePos())
        QGraphicsScene.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        self.m_mouseLock = False
        QGraphicsScene.mouseReleaseEvent(self, event)

# XY Controller Window
class XYControllerW(QMainWindow, ui_xycontroller.Ui_XYControllerW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.cc_x = 1
        self.cc_y = 2
        self.m_channels = []

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.dial_x.setPixmap(2)
        self.dial_y.setPixmap(2)
        self.dial_x.setLabel("X")
        self.dial_y.setLabel("Y")
        self.keyboard.setOctaves(6)

        self.scene = XYGraphicsScene(self)
        self.graphicsView.setScene(self.scene)
        self.graphicsView.setRenderHints(QPainter.Antialiasing)

        for MIDI_CC in MIDI_CC_LIST:
          self.cb_control_x.addItem(MIDI_CC)
          self.cb_control_y.addItem(MIDI_CC)

        # -------------------------------------------------------------
        # Load Settings

        self.settings = QSettings("Cadence", "XY-Controller")
        self.loadSettings()

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))

        self.connect(self.cb_smooth, SIGNAL("clicked(bool)"), SLOT("slot_setSmooth(bool)"))

        self.connect(self.dial_x, SIGNAL("valueChanged(int)"), SLOT("slot_updateSceneX(int)"))
        self.connect(self.dial_y, SIGNAL("valueChanged(int)"), SLOT("slot_updateSceneY(int)"))

        self.connect(self.cb_control_x, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_checkCC_X(QString)"))
        self.connect(self.cb_control_y, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_checkCC_Y(QString)"))

        self.connect(self.scene, SIGNAL("cursorMoved(double, double)"), SLOT("slot_sceneCursorMoved(double, double)"))

        self.connect(self.act_ch_01, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_02, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_03, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_04, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_05, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_06, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_07, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_08, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_09, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_10, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_11, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_12, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_13, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_14, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_15, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_16, SIGNAL("triggered(bool)"), SLOT("slot_checkChannel(bool)"))
        self.connect(self.act_ch_all, SIGNAL("triggered()"), SLOT("slot_checkChannel_all()"))
        self.connect(self.act_ch_none, SIGNAL("triggered()"), SLOT("slot_checkChannel_none()"))

        self.connect(self.act_show_keyboard, SIGNAL("triggered(bool)"), SLOT("slot_showKeyboard(bool)"))
        self.connect(self.act_about, SIGNAL("triggered()"), SLOT("slot_about()"))

        # -------------------------------------------------------------
        # Final stuff

        self.m_midiInTimerId = self.startTimer(50)
        QTimer.singleShot(0, self, SLOT("slot_updateScreen()"))

    def updateScreen(self):
        self.scene.updateSize(self.graphicsView.size())
        self.graphicsView.centerOn(0, 0)

        dial_x = self.dial_x.value()
        dial_y = self.dial_y.value()
        self.slot_updateSceneX(dial_x)
        self.slot_updateSceneY(dial_y)
        self.scene.setSmoothValues(float(dial_x)/100, float(dial_y)/100)

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        global jack_midi_out_data
        for channel in self.m_channels:
          jack_midi_out_data.put_nowait((0x90+channel-1, note, 100))

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        global jack_midi_out_data
        for channel in self.m_channels:
          jack_midi_out_data.put_nowait((0x80+channel-1, note, 0))

    @pyqtSlot(int)
    def slot_updateSceneX(self, x):
        self.scene.setPosX(float(x)/100, not self.dial_x.isSliderDown())

    @pyqtSlot(int)
    def slot_updateSceneY(self, y):
        self.scene.setPosY(float(y)/100, not self.dial_y.isSliderDown())

    @pyqtSlot(str)
    def slot_checkCC_X(self, text):
        if (text):
          self.cc_x = int(text.split(" ")[0], 16)
          self.scene.setControlX(self.cc_x)

    @pyqtSlot(str)
    def slot_checkCC_Y(self, text):
        if (text):
          self.cc_y = int(text.split(" ")[0], 16)
          self.scene.setControlY(self.cc_y)

    @pyqtSlot(bool)
    def slot_checkChannel(self, clicked):
        channel = int(self.sender().text())
        if (clicked and channel not in self.m_channels):
          self.m_channels.append(channel)
        elif (not clicked and channel in self.m_channels):
          self.m_channels.remove(channel)
        self.scene.setChannels(self.m_channels)

    @pyqtSlot()
    def slot_checkChannel_all(self):
        self.act_ch_01.setChecked(True)
        self.act_ch_02.setChecked(True)
        self.act_ch_03.setChecked(True)
        self.act_ch_04.setChecked(True)
        self.act_ch_05.setChecked(True)
        self.act_ch_06.setChecked(True)
        self.act_ch_07.setChecked(True)
        self.act_ch_08.setChecked(True)
        self.act_ch_09.setChecked(True)
        self.act_ch_10.setChecked(True)
        self.act_ch_11.setChecked(True)
        self.act_ch_12.setChecked(True)
        self.act_ch_13.setChecked(True)
        self.act_ch_14.setChecked(True)
        self.act_ch_15.setChecked(True)
        self.act_ch_16.setChecked(True)
        self.m_channels = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
        self.scene.setChannels(self.m_channels)

    @pyqtSlot()
    def slot_checkChannel_none(self):
        self.act_ch_01.setChecked(False)
        self.act_ch_02.setChecked(False)
        self.act_ch_03.setChecked(False)
        self.act_ch_04.setChecked(False)
        self.act_ch_05.setChecked(False)
        self.act_ch_06.setChecked(False)
        self.act_ch_07.setChecked(False)
        self.act_ch_08.setChecked(False)
        self.act_ch_09.setChecked(False)
        self.act_ch_10.setChecked(False)
        self.act_ch_11.setChecked(False)
        self.act_ch_12.setChecked(False)
        self.act_ch_13.setChecked(False)
        self.act_ch_14.setChecked(False)
        self.act_ch_15.setChecked(False)
        self.act_ch_16.setChecked(False)
        self.m_channels = []
        self.scene.setChannels(self.m_channels)

    @pyqtSlot(bool)
    def slot_setSmooth(self, yesno):
        self.scene.setSmooth(yesno)

    @pyqtSlot(float, float)
    def slot_sceneCursorMoved(self, xp, yp):
        self.dial_x.setValue(xp*100)
        self.dial_y.setValue(yp*100)

    @pyqtSlot(bool)
    def slot_showKeyboard(self, yesno):
        self.scrollArea.setVisible(yesno)
        QTimer.singleShot(0, self, SLOT("slot_updateScreen()"))

    @pyqtSlot()
    def slot_about(self):
        QMessageBox.about(self, self.tr("About XY Controller"), self.tr("<h3>XY Controller</h3>"
            "<br>Version %s"
            "<br>XY Controller is a simple XY widget that sends and receives data from Jack MIDI.<br>"
            "<br>Copyright (C) 2012 falkTX" % (VERSION)))

    @pyqtSlot()
    def slot_updateScreen(self):
        self.updateScreen()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowKeyboard", self.scrollArea.isVisible())
        self.settings.setValue("Smooth", self.cb_smooth.isChecked())
        self.settings.setValue("DialX", self.dial_x.value())
        self.settings.setValue("DialY", self.dial_y.value())
        self.settings.setValue("ControlX", self.cc_x)
        self.settings.setValue("ControlY", self.cc_y)
        self.settings.setValue("Channels", self.m_channels)

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))

        showKeyboard = self.settings.value("ShowKeyboard", False, type=bool)
        self.act_show_keyboard.setChecked(showKeyboard)
        self.scrollArea.setVisible(showKeyboard)

        smooth = self.settings.value("Smooth", False, type=bool)
        self.cb_smooth.setChecked(smooth)
        self.scene.setSmooth(smooth)

        self.dial_x.setValue(self.settings.value("DialX", 50, type=int))
        self.dial_y.setValue(self.settings.value("DialY", 50, type=int))

        self.cc_x = self.settings.value("ControlX", 1, type=int)
        self.cc_y = self.settings.value("ControlY", 2, type=int)
        self.scene.setControlX(self.cc_x)
        self.scene.setControlY(self.cc_y)

        self.m_channels = toList(self.settings.value("Channels", [1]))

        for i in range(len(self.m_channels)):
          self.m_channels[i] = int(self.m_channels[i])

        self.scene.setChannels(self.m_channels)

        for i in range(len(MIDI_CC_LIST)):
          cc = int(MIDI_CC_LIST[i].split(" ")[0], 16)
          if (self.cc_x == cc):
            self.cb_control_x.setCurrentIndex(i)
          if (self.cc_y == cc):
            self.cb_control_y.setCurrentIndex(i)

        if (1 in self.m_channels):
          self.act_ch_01.setChecked(True)
        if (2 in self.m_channels):
          self.act_ch_02.setChecked(True)
        if (3 in self.m_channels):
          self.act_ch_03.setChecked(True)
        if (4 in self.m_channels):
          self.act_ch_04.setChecked(True)
        if (5 in self.m_channels):
          self.act_ch_05.setChecked(True)
        if (6 in self.m_channels):
          self.act_ch_06.setChecked(True)
        if (7 in self.m_channels):
          self.act_ch_07.setChecked(True)
        if (8 in self.m_channels):
          self.act_ch_08.setChecked(True)
        if (9 in self.m_channels):
          self.act_ch_09.setChecked(True)
        if (10 in self.m_channels):
          self.act_ch_10.setChecked(True)
        if (11 in self.m_channels):
          self.act_ch_11.setChecked(True)
        if (12 in self.m_channels):
          self.act_ch_12.setChecked(True)
        if (13 in self.m_channels):
          self.act_ch_13.setChecked(True)
        if (14 in self.m_channels):
          self.act_ch_14.setChecked(True)
        if (15 in self.m_channels):
          self.act_ch_15.setChecked(True)
        if (16 in self.m_channels):
          self.act_ch_16.setChecked(True)

    def timerEvent(self, event):
        if (event.timerId() == self.m_midiInTimerId):
          global jack_midi_in_data
          if (jack_midi_in_data.empty() == False):
            while (True):
              try:
                data1, data2, data3 = jack_midi_in_data.get_nowait()
              except QuequeEmpty:
                break

              channel = (data1 & 0x0F)+1
              mode    =  data1 & 0xF0

              if (channel in self.m_channels):
                if (mode == 0x80):
                  self.keyboard.noteOff(data2, False)
                elif (mode == 0x90):
                  self.keyboard.noteOn(data2, False)
                elif (mode == 0xB0):
                  self.scene.handleCC(data2, data3)

              jack_midi_in_data.task_done()

          self.scene.updateSmooth()

        QMainWindow.timerEvent(self, event)

    def resizeEvent(self, event):
        self.updateScreen()
        QMainWindow.resizeEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()
        QMainWindow.closeEvent(self, event)

# -------------------------------------------------------------
# JACK Stuff

static_event = jacklib.jack_midi_event_t()
static_mtype = jacklib.c_ubyte*3

def jack_process_callback(nframes, arg):
    global jack_midi_in_port, jack_midi_out_port, jack_midi_in_data, jack_midi_out_data

    # MIDI In
    midi_in_buffer = jacklib.port_get_buffer(jack_midi_in_port, nframes)

    if (midi_in_buffer):
      event_count = jacklib.midi_get_event_count(midi_in_buffer)

      for i in range(event_count):
        if (jacklib.midi_event_get(jacklib.pointer(static_event), midi_in_buffer, i) == 0):
          if (static_event.size == 1):
            jack_midi_in_data.put_nowait((static_event.buffer[0], 0, 0))
          elif (static_event.size == 2):
            jack_midi_in_data.put_nowait((static_event.buffer[0], static_event.buffer[1], 0))
          elif (static_event.size >= 3):
            jack_midi_in_data.put_nowait((static_event.buffer[0], static_event.buffer[1], static_event.buffer[2]))

          if (jack_midi_in_data.full()):
            break

    # MIDI Out
    midi_out_buffer = jacklib.port_get_buffer(jack_midi_out_port, nframes)

    if (midi_out_buffer):
      jacklib.midi_clear_buffer(midi_out_buffer)

      if (jack_midi_out_data.empty() == False):
        while (True):
          try:
            mode, note, velo = jack_midi_out_data.get_nowait()
          except QuequeEmpty:
            break

          data = static_mtype(mode, note, velo)
          jacklib.midi_event_write(midi_out_buffer, 0, data, 3)

          jack_midi_out_data.task_done()

    return 0

def jack_session_callback(event, arg):
  if (WINDOWS):
    filepath = os.path.join(sys.argv[0])
  else:
    if (sys.argv[0].startswith("/")):
      filepath = "jack_xycontroller"
    else:
      filepath = os.path.join(sys.path[0], "xycontroller.py")

  event.command_line = str(filepath).encode("ascii")
  jacklib.session_reply(jack_client, event)

  if (event.type == jacklib.JackSessionSaveAndQuit):
    app.quit()

  #jacklib.session_event_free(event)

#--------------- main ------------------
if __name__ == '__main__':

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("XY-Controller")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("falkTX")
    #app.setWindowIcon(QIcon(":/48x48/xy-controller.png"))

    # Start jack
    jack_status = jacklib.jack_status_t(0)
    jack_client = jacklib.client_open("XY-Controller", jacklib.JackSessionID, jacklib.pointer(jack_status))

    if not jack_client:
      QMessageBox.critical(None, app.translate("XYControllerW", "Error"), app.translate("XYControllerW", "Could not connect to JACK, possible errors:\n%s" % (get_jack_status_error_string(jack_status))))
      sys.exit(1)

    jack_midi_in_port = jacklib.port_register(jack_client, "midi_in", jacklib.JACK_DEFAULT_MIDI_TYPE, jacklib.JackPortIsInput, 0)
    jack_midi_out_port = jacklib.port_register(jack_client, "midi_out", jacklib.JACK_DEFAULT_MIDI_TYPE, jacklib.JackPortIsOutput, 0)
    jacklib.set_session_callback(jack_client, jack_session_callback, None)
    jacklib.set_process_callback(jack_client, jack_process_callback, None)
    jacklib.activate(jack_client)

    # Show GUI
    gui = XYControllerW()
    gui.show()

    # Set-up custom signal handling
    set_up_signals(gui)

    # App-Loop
    ret = app.exec_()

    # Close Jack
    if (jack_client):
      jacklib.deactivate(jack_client)
      jacklib.client_close(jack_client)

    # Exit properly
    sys.exit(ret)

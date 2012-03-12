#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to JACK
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
from PyQt4.QtCore import pyqtSlot, QTimer
from PyQt4.QtGui import QCursor, QFontMetrics, QMenu

# Imports (Custom Stuff)
import jacksettings, logs, render
from shared import *
from jacklib_helpers import *

# Have JACK2 ?
try:
  jacklib.get_version_string()
  JACK2 = True
except:
  JACK2 = False

# Can Render ?
for iPATH in PATH:
  if (os.path.exists(os.path.join(iPATH, "jack_capture"))):
    canRender = True
    break
else:
  canRender = False

# Variables
TRANSPORT_VIEW_HMS = 0
TRANSPORT_VIEW_BBT = 1
TRANSPORT_VIEW_FRAMES = 2

buffer_sizes = (16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192)
sample_rates = (22050, 32000, 44100, 48000, 88200, 96000, 192000)

# DBus object
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
      'ladish_app_iface',
      'ladish_app_daemon',
      'patchbay'
    ]
DBus = DBusObject()

# Jack object
class JackObject(object):
    __slots__ = [
      'client'
    ]
jack = JackObject()

# Init objects
DBus.loop = None
DBus.bus = None
DBus.a2j = None
DBus.jack = None
DBus.ladish_control = None
DBus.ladish_studio = None
DBus.ladish_room = None
DBus.ladish_graph = None
DBus.ladish_app_iface = None
DBus.ladish_app_daemon = None
DBus.patchbay = None

jack.client = None

# -------------------------------------------------------------
# Property change calls

#def jack_buffer_size(self, buffer_size):
    #if (buffer_size != self.buffer_size):
      #if (jack.client):
        #jacklib.set_buffer_size(jack.client, buffer_size)
      #else:
        #jacksettings.setBufferSize(buffer_size)

    #else:
      ## Make GUIs show previous value
      #if ("setBufferSize" in dir(self)):
        #QTimer.singleShot(100, lambda bf=buffer_size: self.setBufferSize(bf))
      #else:
        #QTimer.singleShot(100, lambda parent=self, bf=buffer_size: setBufferSize(parent, bf))

#def jack_sample_rate(self, sample_rate):
    #if (jack.client):
      #setSampleRate(self, sample_rate, True)
    #else:
      #jacksettings.setSampleRate(sample_rate)
      #setSampleRate(self, sample_rate)

#def jack_buffer_size_cb(self, text):
    #if (text.isEmpty()): return
    #jack_buffer_size(self, int(QStringStr(text).replace("*","")))

#def jack_buffer_size_m(self, buffer_size):
    #jack_buffer_size(self, buffer_size)

#def jack_sample_rate_cb(self, text):
    #if (text.isEmpty()): return
    #jack_sample_rate(self, int(QStringStr(text).replace("*","")))

# -------------------------------------------------------------
# Transport calls

#def transport_playpause(self, play):
    #if (not jack.client): return
    #if (play):
      #jacklib.transport_start(jack.client)
    #else:
      #jacklib.transport_stop(jack.client)
    #refreshTransport(self)

#def transport_stop(self):
    #if (not jack.client): return
    #jacklib.transport_stop(jack.client)
    #jacklib.transport_locate(jack.client, 0)
    #refreshTransport(self)

#def transport_backwards(self):
    #if (not jack.client): return
    #new_frame = int(jacklib.get_current_transport_frame(jack.client))-100000
    #if (new_frame < 0): new_frame = 0
    #jacklib.transport_locate(jack.client, new_frame)

#def transport_forwards(self):
    #if (not jack.client): return
    #new_frame = int(jacklib.get_current_transport_frame(jack.client))+100000
    #jacklib.transport_locate(jack.client, new_frame)

#def transport_view_menu(self):
    #menu = QMenu(self)
    #act_t_hms = menu.addAction("Hours:Minutes:Seconds")
    #act_t_bbt = menu.addAction("Beat:Bar:Tick")
    #act_t_fr  = menu.addAction("Frames")

    #act_t_hms.setCheckable(True)
    #act_t_bbt.setCheckable(True)
    #act_t_fr.setCheckable(True)

    #if (self.selected_transport_view == TRANSPORT_VIEW_HMS):
      #act_t_hms.setChecked(True)
    #elif (self.selected_transport_view == TRANSPORT_VIEW_BBT):
      #act_t_bbt.setChecked(True)
    #elif (self.selected_transport_view == TRANSPORT_VIEW_FRAMES):
      #act_t_fr.setChecked(True)

    #act_selected = menu.exec_(QCursor().pos())

    #if (act_selected == act_t_hms):
      #transport_set_view(self, TRANSPORT_VIEW_HMS)
    #elif (act_selected == act_t_bbt):
      #transport_set_view(self, TRANSPORT_VIEW_BBT)
    #elif (act_selected == act_t_fr):
      #transport_set_view(self, TRANSPORT_VIEW_FRAMES)

def transport_set_view(self, view):
    if (view == TRANSPORT_VIEW_HMS):
      self.m_selected_transport_view = TRANSPORT_VIEW_HMS
      self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("00:00:00")+3)
    elif (view == TRANSPORT_VIEW_BBT):
      self.m_selected_transport_view = TRANSPORT_VIEW_BBT
      self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("000|00|0000")+3)
    elif (view == TRANSPORT_VIEW_FRAMES):
      self.m_selected_transport_view = TRANSPORT_VIEW_FRAMES
      self.label_time.setMinimumWidth(QFontMetrics(self.label_time.font()).width("000'000'000")+3)
    else:
      self.m_selected_transport_view = None

#def transport_bpm_set(self, bpm):
    #if (not jack.client): return
    #if (self.last_bpm != bpm):
      #pos = jacklib.jack_position_t
      #pos.valid = 0
      #state = jacklib.transport_query(jack.client, pos)

      #pos.beats_per_minute = bpm

      #if (state > jacklib.TransportStopped):
        #pos.frame += self.buffer_size

      #if (not pos.valid & jacklib.PositionBBT):
        #pos.bar = 1
        #pos.beat = 1
        #pos.tick = 0
        #pos.valid = jacklib.PositionBBT
        #QTimer.singleShot(self.buffer_size, transport_fix)

      #jacklib.transport_reposition(jack.client, pos)

      #self.last_bpm = bpm

#def transport_fix():
    #pos = jacklib.jack_position_t
    #pos.valid = 0
    #jacklib.transport_query(jack.client, pos)
    #pos.frame += jacklib.get_buffer_size(jack.client)
    #jacklib.transport_reposition(jack.client, pos)

# -------------------------------------------------------------
# Refresh GUI stuff

#def refreshBufferSize(self):
    #if (self.last_buffer_size != self.buffer_size):
      #setBufferSize(self, self.buffer_size)

    #self.last_buffer_size = self.buffer_size

#def refreshSampleRate(self):
    #if (self.last_sample_rate != self.sample_rate):
      #setSampleRate(self, self.sample_rate)

    #self.last_sample_rate = self.sample_rate

def refreshDSPLoad(self):
    if (not jack.client): return
    setDSPLoad(self, int(jacklib.cpu_load(jack.client)))

def refreshTransport(self):
    if (not jack.client): return
    pos = jacklib.jack_position_t()
    pos.valid = 0
    state = jacklib.transport_query(jack.client, jacklib.pointer(pos))

    if (self.m_selected_transport_view == TRANSPORT_VIEW_HMS):
      frame = pos.frame
      time  = frame / self.m_sample_rate
      secs  = time % 60
      mins  = (time / 60) % 60
      hrs   = (time / 3600) % 60
      secH  = minH = hrsH = ""
      if secs < 10: secH = "0"
      if mins < 10: minH = "0"
      if hrs  < 10: hrsH = "0"
      self.label_time.setText("%s%i:%s%i:%s%i" % (hrsH, hrs, minH, mins, secH, secs))

    #elif (self.m_selected_transport_view == TRANSPORT_VIEW_BBT):
      #if (pos.valid & jacklib.PositionBBT):
        #bar  = pos.bar
        #beat = pos.beat
        #tick = pos.tick
        #barH = beatH = tickH = ""
        #if (bar == 0):
          #beat = 0
          #tick = 0
          #barH = "00"
        #elif bar < 10: barH = "00"
        #elif bar < 100: barH = "0"
        #if tick < 10: tickH = "000"
        #elif tick < 100: tickH = "00"
        #elif tick < 1000: tickH = "0"
        #self.label_time.setText(barH+str(bar)+"|"+beatH+str(beat)+"|"+tickH+str(tick))
      #else:
        #self.label_time.setText("000|00|0000")

    #elif (self.m_selected_transport_view == TRANSPORT_VIEW_FRAMES):
      #frame  = pos.frame
      #frame1 = pos.frame % 1000
      #frame2 = (pos.frame / 1000) % 1000
      #frame3 = (pos.frame / 1000000) % 1000
      #frame1h = frame2h = frame3h = ""
      #if frame1 < 10: frame1h = "00"
      #elif frame1 < 100: frame1h = "0"
      #if frame2 < 10: frame2h = "00"
      #elif frame2 < 100: frame2h = "0"
      #if frame3 < 10: frame3h = "00"
      #elif frame3 < 100: frame3h = "0"
      #self.label_time.setText(frame3h+str(frame3)+"'"+frame2h+str(frame2)+"'"+frame1h+str(frame1))

    if (pos.valid & jacklib.JackPositionBBT):
      if (self.m_last_bpm != pos.beats_per_minute):
        self.sb_bpm.setValue(pos.beats_per_minute)
        self.sb_bpm.setStyleSheet("")
    else:
      pos.beats_per_minute = -1
      if (self.m_last_bpm != pos.beats_per_minute):
        self.sb_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

    self.m_last_bpm = pos.beats_per_minute

    if (state != self.m_last_transport_state):
      if (state == jacklib.JackTransportStopped):
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
    self.m_last_transport_state = state

# -------------------------------------------------------------
# Set GUI stuff

def setBufferSize(self, buffer_size):
    if (self.m_buffer_size == buffer_size):
      return

    self.m_buffer_size = buffer_size
    if (buffer_size):
      if (buffer_size == 16):
        self.cb_buffer_size.setCurrentIndex(0)
      elif (buffer_size == 32):
        self.cb_buffer_size.setCurrentIndex(1)
      elif (buffer_size == 64):
        self.cb_buffer_size.setCurrentIndex(2)
      elif (buffer_size == 128):
        self.cb_buffer_size.setCurrentIndex(3)
      elif (buffer_size == 256):
        self.cb_buffer_size.setCurrentIndex(4)
      elif (buffer_size == 512):
        self.cb_buffer_size.setCurrentIndex(5)
      elif (buffer_size == 1024):
        self.cb_buffer_size.setCurrentIndex(6)
      elif (buffer_size == 2048):
        self.cb_buffer_size.setCurrentIndex(7)
      elif (buffer_size == 4096):
        self.cb_buffer_size.setCurrentIndex(8)
      elif (buffer_size == 8192):
        self.cb_buffer_size.setCurrentIndex(9)
      else:
        QMessageBox.warning(self, self.tr("Warning"), self.tr("Invalid JACK buffer-size requested"))

    #if ("act_jack_bf_list" in dir(self)):
    if (buffer_size):
      for act_bf in self.act_jack_bf_list:
        act_bf.setEnabled(True)
        if (act_bf.text().replace("&","") == str(buffer_size)):
          #if (act_bf.isChecked() == False):
            act_bf.setChecked(True)
        else:
          if (act_bf.isChecked()):
            act_bf.setChecked(False)
      #else:
        #for i in range(len(self.act_jack_bf_list)):
          #self.act_jack_bf_list[i].setEnabled(False)
          #if (self.act_jack_bf_list[i].isChecked()):
            #self.act_jack_bf_list[i].setChecked(False)

def setSampleRate(self, sample_rate, future=False):
    if (self.m_sample_rate == sample_rate):
      return

    if (future):
      pass
      #if (self.sender() == self.cb_sample_rate): # Changed using GUI
        #ask = QMessageBox.question(self, self.tr("Change Sample Rate"), self.tr("It's not possible to change Sample Rate while JACK is running.\n"
                                      #"Do you want to change as soon as JACK stops?"), QMessageBox.Ok|QMessageBox.Cancel)
        #if (ask == QMessageBox.Ok):
          #self.next_sample_rate = sample_rate
        #else:
          #self.next_sample_rate = 0

    # not future
    else:
      self.m_sample_rate = sample_rate
      self.m_next_sample_rate = 0

    for i in range(len(sample_rates)):
      sample_rate = sample_rates[i]
      sample_rate_str = str(sample_rate)

      #if (self.m_next_sample_rate != 0 and self.m_next_sample_rate != self.m_sample_rate and self.m_sample_rate == sample_rate):
        #text += "*"

      self.cb_sample_rate.setItemText(i, sample_rate_str)

      if (self.m_sample_rate == sample_rate):
        self.cb_sample_rate.setCurrentIndex(i)

def setRealTime(self, realtime):
    self.label_realtime.setText(" RT " if realtime else " <s>RT</s> ")
    self.label_realtime.setEnabled(realtime)

def setDSPLoad(self, dsp_load):
    self.pb_dsp_load.setValue(dsp_load)

def setXruns(self, xruns):
    self.b_xruns.setText("%s Xrun%s" % ((str(xruns) if (xruns >= 0) else "--"), ("" if (xruns == 1) else "s")))

# -------------------------------------------------------------
# External Dialogs

@pyqtSlot()
def slot_showJackSettings(self):
    jacksettings.JackSettingsW(self).exec_()

@pyqtSlot()
def slot_showLogs(self):
    logs.LogsW(self).show()

@pyqtSlot()
def slot_showRender(self):
    render.RenderW(self).exec_()

# -------------------------------------------------------------
# Shared Connections

def setJackConnections(self, modes):
  pass
  if ("jack" in modes):
    self.connect(self.act_jack_clear_xruns, SIGNAL("triggered()"), SLOT("slot_JackClearXruns()"))
    self.connect(self.act_jack_render, SIGNAL("triggered()"), lambda: slot_showRender(self))
    self.connect(self.act_jack_configure, SIGNAL("triggered()"), lambda: slot_showJackSettings(self))
    self.connect(self.b_jack_clear_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))
    self.connect(self.b_jack_configure, SIGNAL("clicked()"), lambda: slot_showJackSettings(self))
    self.connect(self.b_jack_render, SIGNAL("clicked()"), lambda: slot_showRender(self))
    #self.connect(self.cb_buffer_size, SIGNAL("currentIndexChanged(QString)"), lambda: jack_buffer_size_cb(self, self.cb_buffer_size.currentText()))
    #self.connect(self.cb_sample_rate, SIGNAL("currentIndexChanged(QString)"), lambda: jack_sample_rate_cb(self, self.cb_sample_rate.currentText()))
    self.connect(self.b_xruns, SIGNAL("clicked()"), SLOT("slot_JackClearXruns()"))

  #if ("buffer-size" in modes):
    #self.connect(self.act_jack_bf_16, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 16))
    #self.connect(self.act_jack_bf_32, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 32))
    #self.connect(self.act_jack_bf_64, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 64))
    #self.connect(self.act_jack_bf_128, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 128))
    #self.connect(self.act_jack_bf_256, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 256))
    #self.connect(self.act_jack_bf_512, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 512))
    #self.connect(self.act_jack_bf_1024, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 1024))
    #self.connect(self.act_jack_bf_2048, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 2048))
    #self.connect(self.act_jack_bf_4096, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 4096))
    #self.connect(self.act_jack_bf_8192, SIGNAL("triggered(bool)"), lambda: jack_buffer_size_m(self, 8192))

  #if ("transport" in modes):
    #self.connect(self.act_transport_play, SIGNAL("triggered(bool)"), lambda: transport_playpause(self, self.act_transport_play.isChecked()))
    #self.connect(self.act_transport_stop, SIGNAL("triggered()"), lambda: transport_stop(self))
    #self.connect(self.act_transport_backwards, SIGNAL("triggered()"), lambda: transport_backwards(self))
    #self.connect(self.act_transport_forwards, SIGNAL("triggered()"), lambda: transport_forwards(self))
    #self.connect(self.b_transport_play, SIGNAL("clicked(bool)"), lambda: transport_playpause(self, self.b_transport_play.isChecked()))
    #self.connect(self.b_transport_stop, SIGNAL("clicked()"), lambda: transport_stop(self))
    #self.connect(self.b_transport_backwards, SIGNAL("clicked()"), lambda: transport_backwards(self))
    #self.connect(self.b_transport_forwards, SIGNAL("clicked()"), lambda: transport_forwards(self))
    #self.connect(self.sb_bpm, SIGNAL("valueChanged(double)"), lambda: transport_bpm_set(self, self.sb_bpm.value()))
    #self.connect(self.label_time, SIGNAL("customContextMenuRequested(QPoint)"), lambda: transport_view_menu(self))

  if ("misc" in modes):
    if (LINUX):
      self.connect(self.act_show_logs, SIGNAL("triggered()"), lambda: slot_showLogs(self))
    else:
      self.act_show_logs.setEnabled(False)

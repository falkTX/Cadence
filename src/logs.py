#!/usr/bin/env python
# -*- coding: utf-8 -*-

# JACK, A2J, LASH and LADISH Logs Viewer
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
from PyQt4.QtCore import pyqtSlot, Qt, QFile, QIODevice, QTextStream, QThread
from PyQt4.QtGui import QDialog, QPalette, QSyntaxHighlighter

# Imports (Custom Stuff)
import ui_logs
from shared import *

# Fix log text output (get rid of terminal colors stuff)
def fixLogText(text):
    return text.replace("[1m[31m","").replace("[1m[33m","").replace("[31m","").replace("[33m","").replace("[0m","")

# Syntax Highlighter for JACK
class SyntaxHighligher_JACK(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.m_palette = self.parent().palette()

    def highlightBlock(self, text):
      if (": ERROR: " in text):
        self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
      elif (": WARNING: " in text):
        self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
      elif (": ------------------" in text):
        self.setFormat(text.find(" ------------------"), len(text), self.m_palette.color(QPalette.Active, QPalette.Mid))
      elif (": Connecting " in text):
        self.setFormat(text.find(" Connecting "), len(text), self.m_palette.color(QPalette.Active, QPalette.Link))
      elif (": Disconnecting " in text):
        self.setFormat(text.find(" Disconnecting "), len(text), self.m_palette.color(QPalette.Active, QPalette.LinkVisited))
      #elif (": New client " in text):
        #self.setFormat(text.find(" New client "), len(text), self.m_palette.color(QPalette.Active, QPalette.Link))

# Syntax Highlighter for A2J
class SyntaxHighligher_A2J(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.m_palette = self.parent().palette()

    def highlightBlock(self, text):
      if (": error: " in text):
        self.setFormat(text.find(" error: "), len(text), Qt.red)
      elif (": WARNING: " in text):
        self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
      elif (": ----------------------------" in text):
        self.setFormat(text.find("----------------------------"), len(text), self.m_palette.color(QPalette.Active, QPalette.Mid))
      elif (": port created: " in text):
        self.setFormat(text.find(" port created: "), len(text), self.m_palette.color(QPalette.Active, QPalette.Link))
      elif (": port deleted: " in text):
        self.setFormat(text.find(" port deleted: "), len(text), self.m_palette.color(QPalette.Active, QPalette.LinkVisited))

# Syntax Highlighter for LASH
class SyntaxHighligher_LASH(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.m_palette = self.parent().palette()

    def highlightBlock(self, text):
      if (": ERROR: " in text):
        self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
      elif (": WARNING: " in text):
        self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
      elif (": ------------------" in text):
        self.setFormat(text.find(" ------------------"), len(text), self.m_palette.color(QPalette.Active, QPalette.Mid))

# Syntax Highlighter for LADISH
class SyntaxHighligher_LADISH(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.m_palette = self.parent().palette()

    def highlightBlock(self, text):
      if (": ERROR: " in text):
        self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
      elif (": WARNING: " in text):
        self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
      elif (": -------" in text):
        self.setFormat(text.find(" -------"), len(text), self.m_palette.color(QPalette.Active, QPalette.Mid))

# Lockless file read thread
class LogsReadThread(QThread):
    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.m_purgeLogs = False

        # -------------------------------------------------------------
        # Take some values from parent

        self.LOG_FILE_JACK   = self.parent().LOG_FILE_JACK
        self.LOG_FILE_A2J    = self.parent().LOG_FILE_A2J
        self.LOG_FILE_LASH   = self.parent().LOG_FILE_LASH
        self.LOG_FILE_LADISH = self.parent().LOG_FILE_LADISH

        # -------------------------------------------------------------
        # Init logs

        if (self.LOG_FILE_JACK):
          self.log_jack_file = QFile(self.LOG_FILE_JACK)
          self.log_jack_file.open(QIODevice.ReadOnly)
          self.log_jack_stream = QTextStream(self.log_jack_file)
          self.log_jack_stream.setCodec("UTF-8")

        if (self.LOG_FILE_A2J):
          self.log_a2j_file = QFile(self.LOG_FILE_A2J)
          self.log_a2j_file.open(QIODevice.ReadOnly)
          self.log_a2j_stream = QTextStream(self.log_a2j_file)
          self.log_a2j_stream.setCodec("UTF-8")

        if (self.LOG_FILE_LASH):
          self.log_lash_file = QFile(self.LOG_FILE_LASH)
          self.log_lash_file.open(QIODevice.ReadOnly)
          self.log_lash_stream = QTextStream(self.log_lash_file)
          self.log_lash_stream.setCodec("UTF-8")

        if (self.LOG_FILE_LADISH):
          self.log_ladish_file = QFile(self.LOG_FILE_LADISH)
          self.log_ladish_file.open(QIODevice.ReadOnly)
          self.log_ladish_stream = QTextStream(self.log_ladish_file)
          self.log_ladish_stream.setCodec("UTF-8")

    def purgeLogs(self):
        self.m_purgeLogs = True

    def run(self):
        # -------------------------------------------------------------
        # Read logs and set text in main thread

        while (self.isRunning()):
          if (self.m_purgeLogs):
            if (self.LOG_FILE_JACK):
              self.log_jack_stream.flush()
              self.log_jack_file.close()
              self.log_jack_file.open(QIODevice.WriteOnly)
              self.log_jack_file.close()
              self.log_jack_file.open(QIODevice.ReadOnly)

            if (self.LOG_FILE_A2J):
              self.log_a2j_stream.flush()
              self.log_a2j_file.close()
              self.log_a2j_file.open(QIODevice.WriteOnly)
              self.log_a2j_file.close()
              self.log_a2j_file.open(QIODevice.ReadOnly)

            if (self.LOG_FILE_LASH):
              self.log_lash_stream.flush()
              self.log_lash_file.close()
              self.log_lash_file.open(QIODevice.WriteOnly)
              self.log_lash_file.close()
              self.log_lash_file.open(QIODevice.ReadOnly)

            if (self.LOG_FILE_LADISH):
              self.log_ladish_stream.flush()
              self.log_ladish_file.close()
              self.log_ladish_file.open(QIODevice.WriteOnly)
              self.log_ladish_file.close()
              self.log_ladish_file.open(QIODevice.ReadOnly)

          else:
            text_jack   = ""
            text_a2j    = ""
            text_lash   = ""
            text_ladish = ""

            if (self.LOG_FILE_JACK):
              text_jack = fixLogText(self.log_jack_stream.readAll()).strip()

            if (self.LOG_FILE_A2J):
              text_a2j = fixLogText(self.log_a2j_stream.readAll()).strip()

            if (self.LOG_FILE_LASH):
              text_lash = fixLogText(self.log_lash_stream.readAll()).strip()

            if (self.LOG_FILE_LADISH):
              text_ladish = fixLogText(self.log_ladish_stream.readAll()).strip()

            self.parent().setLogsText(text_jack, text_a2j, text_lash, text_ladish)
            self.emit(SIGNAL("updateLogs()"))

          self.sleep(1)

        # -------------------------------------------------------------
        # Close logs before closing thread

        if (self.LOG_FILE_JACK):
          self.log_jack_file.close()

        if (self.LOG_FILE_A2J):
          self.log_a2j_file.close()

        if (self.LOG_FILE_LASH):
          self.log_lash_file.close()

        if (self.LOG_FILE_LADISH):
          self.log_ladish_file.close()

# Logs Window
class LogsW(QDialog, ui_logs.Ui_LogsW):

    LOG_PATH = os.path.join(HOME, ".log")

    LOG_FILE_JACK   = os.path.join(LOG_PATH, "jack", "jackdbus.log")
    LOG_FILE_A2J    = os.path.join(LOG_PATH, "a2j", "a2j.log")
    LOG_FILE_LASH   = os.path.join(LOG_PATH, "lash", "lash.log")
    LOG_FILE_LADISH = os.path.join(LOG_PATH, "ladish", "ladish.log")

    def __init__(self, parent, flags):
        QDialog.__init__(self, parent, flags)
        self.setupUi(self)

        self.b_close.setIcon(getIcon("dialog-close"))
        self.b_purge.setIcon(getIcon("user-trash"))

        self.m_firstRun = True
        self.m_text_jack   = ""
        self.m_text_a2j    = ""
        self.m_text_lash   = ""
        self.m_text_ladish = ""

        # -------------------------------------------------------------
        # Check for unexisting logs and remove tabs for those

        tab_index = 0

        if (os.path.exists(self.LOG_FILE_JACK) == False):
          self.LOG_FILE_JACK = None
          self.tabWidget.removeTab(0-tab_index)
          tab_index += 1

        if (os.path.exists(self.LOG_FILE_A2J) == False):
          self.LOG_FILE_A2J = None
          self.tabWidget.removeTab(1-tab_index)
          tab_index += 1

        if (os.path.exists(self.LOG_FILE_LASH) == False):
          self.LOG_FILE_LASH = None
          self.tabWidget.removeTab(2-tab_index)
          tab_index += 1

        if (os.path.exists(self.LOG_FILE_LADISH) == False):
          self.LOG_FILE_LADISH = None
          self.tabWidget.removeTab(3-tab_index)
          tab_index += 1

        # -------------------------------------------------------------
        # Init logs viewers

        if (self.LOG_FILE_JACK):
          syntax_jack = SyntaxHighligher_JACK(self.pte_jack)
          syntax_jack.setDocument(self.pte_jack.document())

        if (self.LOG_FILE_A2J):
          syntax_a2j = SyntaxHighligher_A2J(self.pte_a2j)
          syntax_a2j.setDocument(self.pte_a2j.document())

        if (self.LOG_FILE_LASH):
          syntax_lash = SyntaxHighligher_LASH(self.pte_lash)
          syntax_lash.setDocument(self.pte_lash.document())

        if (self.LOG_FILE_LADISH):
          syntax_ladish = SyntaxHighligher_LADISH(self.pte_ladish)
          syntax_ladish.setDocument(self.pte_ladish.document())

        # -------------------------------------------------------------
        # Init file read thread

        self.m_readThread = LogsReadThread(self)
        self.m_readThread.start(QThread.IdlePriority)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self.b_purge, SIGNAL("clicked()"), SLOT("slot_purgeLogs()"))
        self.connect(self.m_readThread, SIGNAL("updateLogs()"), SLOT("slot_updateLogs()"))

    def setLogsText(self, text_jack, text_a2j, text_lash, text_ladish):
        self.m_text_jack   = text_jack
        self.m_text_a2j    = text_a2j
        self.m_text_lash   = text_lash
        self.m_text_ladish = text_ladish

    @pyqtSlot()
    def slot_updateLogs(self):
        if (self.m_firstRun):
          self.pte_jack.clear()
          self.pte_a2j.clear()
          self.pte_lash.clear()
          self.pte_ladish.clear()

        if (self.LOG_FILE_JACK):
          if (self.m_text_jack):
            self.pte_jack.appendPlainText(self.m_text_jack)

        if (self.LOG_FILE_A2J):
          if (self.m_text_a2j):
            self.pte_a2j.appendPlainText(self.m_text_a2j)

        if (self.LOG_FILE_LASH):
          if (self.m_text_lash):
            self.pte_lash.appendPlainText(self.m_text_lash)

        if (self.LOG_FILE_LADISH):
          if (self.m_text_ladish):
            self.pte_ladish.appendPlainText(self.m_text_ladish)

        if (self.m_firstRun):
          self.pte_jack.horizontalScrollBar().setValue(0)
          self.pte_jack.verticalScrollBar().setValue(self.pte_jack.verticalScrollBar().maximum())
          self.pte_a2j.horizontalScrollBar().setValue(0)
          self.pte_a2j.verticalScrollBar().setValue(self.pte_a2j.verticalScrollBar().maximum())
          self.pte_lash.horizontalScrollBar().setValue(0)
          self.pte_lash.verticalScrollBar().setValue(self.pte_lash.verticalScrollBar().maximum())
          self.pte_ladish.horizontalScrollBar().setValue(0)
          self.pte_ladish.verticalScrollBar().setValue(self.pte_ladish.verticalScrollBar().maximum())
          self.m_firstRun = False

    @pyqtSlot()
    def slot_purgeLogs(self):
        self.m_readThread.purgeLogs()
        self.pte_jack.clear()
        self.pte_a2j.clear()
        self.pte_lash.clear()
        self.pte_ladish.clear()

    def closeEvent(self, event):
        self.m_readThread.quit()
        return QDialog.closeEvent(self, event)

# -------------------------------------------------------------
# Allow to use this as a standalone app
if __name__ == '__main__':

    # Additional imports
    from PyQt4.QtGui import QApplication

    # App initialization
    app = QApplication(sys.argv)

    # Show GUI
    gui = LogsW(None, Qt.WindowFlags())
    gui.show()

    set_up_signals(gui)

    # App-Loop
    sys.exit(app.exec_())

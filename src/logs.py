#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# JACK, A2J, LASH and LADISH Logs Viewer
# Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

if True:
    from PyQt5.QtCore import pyqtSlot, Qt, QFile, QIODevice, QMutex, QMutexLocker, QTextStream, QThread, QSettings
    from PyQt5.QtGui import QPalette, QSyntaxHighlighter
    from PyQt5.QtWidgets import QDialog
else:
    from PyQt4.QtCore import pyqtSlot, Qt, QFile, QIODevice, QMutex, QMutexLocker, QTextStream, QThread, QSettings
    from PyQt4.QtGui import QPalette, QSyntaxHighlighter
    from PyQt4.QtGui import QDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_logs
from shared import *
from shared_i18n import *

# ------------------------------------------------------------------------------------------------------------
# Fix log text output (get rid of terminal colors stuff)

def fixLogText(text):
    return text.replace("[1m[31m", "").replace("[1m[33m", "").replace("[31m", "").replace("[33m", "").replace("[0m", "")

# ------------------------------------------------------------------------------------------------------------
# Syntax Highlighter for JACK

class SyntaxHighlighter_JACK(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.fPalette = parent.palette()

    def highlightBlock(self, text):
        if ": ERROR: " in text:
            self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
        elif ": WARNING: " in text:
            self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
        elif ": ------------------" in text:
            self.setFormat(text.find(" ------------------"), len(text), self.fPalette.color(QPalette.Active, QPalette.Mid))
        elif ": Connecting " in text:
            self.setFormat(text.find(" Connecting "), len(text), self.fPalette.color(QPalette.Active, QPalette.Link))
        elif ": Disconnecting " in text:
            self.setFormat(text.find(" Disconnecting "), len(text), self.fPalette.color(QPalette.Active, QPalette.LinkVisited))
        #elif (": New client " in text):
            #self.setFormat(text.find(" New client "), len(text), self.fPalette.color(QPalette.Active, QPalette.Link))

# ------------------------------------------------------------------------------------------------------------
# Syntax Highlighter for A2J

class SyntaxHighlighter_A2J(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.fPalette = parent.palette()

    def highlightBlock(self, text):
        if ": error: " in text:
            self.setFormat(text.find(" error: "), len(text), Qt.red)
        elif ": WARNING: " in text:
            self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
        elif ": ----------------------------" in text:
            self.setFormat(text.find("----------------------------"), len(text), self.fPalette.color(QPalette.Active, QPalette.Mid))
        elif ": port created: " in text:
            self.setFormat(text.find(" port created: "), len(text), self.fPalette.color(QPalette.Active, QPalette.Link))
        elif ": port deleted: " in text:
            self.setFormat(text.find(" port deleted: "), len(text), self.fPalette.color(QPalette.Active, QPalette.LinkVisited))

# ------------------------------------------------------------------------------------------------------------
# Syntax Highlighter for LASH

class SyntaxHighlighter_LASH(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.fPalette = parent.palette()

    def highlightBlock(self, text):
        if ": ERROR: " in text:
            self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
        elif ": WARNING: " in text:
            self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
        elif ": ------------------" in text:
            self.setFormat(text.find(" ------------------"), len(text), self.fPalette.color(QPalette.Active, QPalette.Mid))

# ------------------------------------------------------------------------------------------------------------
# Syntax Highlighter for LADISH

class SyntaxHighlighter_LADISH(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        self.fPalette = parent.palette()

    def highlightBlock(self, text):
        if ": ERROR: " in text:
            self.setFormat(text.find(" ERROR: "), len(text), Qt.red)
        elif ": WARNING: " in text:
            self.setFormat(text.find(" WARNING: "), len(text), Qt.darkRed)
        elif ": -------" in text:
            self.setFormat(text.find(" -------"), len(text), self.fPalette.color(QPalette.Active, QPalette.Mid))

# ------------------------------------------------------------------------------------------------------------
# Lock-less file read thread

class LogsReadThread(QThread):
    MAX_INITIAL_SIZE = 2*1024*1024 # 2Mb

    updateLogs = pyqtSignal()

    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.fCloseNow   = False
        self.fPurgeLogs  = False
        self.fRealParent = parent

        # -------------------------------------------------------------
        # Take some values from Logs Window

        self.LOG_FILE_JACK   = LogsW.LOG_FILE_JACK
        self.LOG_FILE_A2J    = LogsW.LOG_FILE_A2J
        self.LOG_FILE_LASH   = LogsW.LOG_FILE_LASH
        self.LOG_FILE_LADISH = LogsW.LOG_FILE_LADISH

        # -------------------------------------------------------------
        # Init logs

        if self.LOG_FILE_JACK is not None:
            self.fLogFileJACK = QFile(self.LOG_FILE_JACK)
            self.fLogFileJACK.open(QIODevice.ReadOnly)
            self.fLogStreamJACK = QTextStream(self.fLogFileJACK)
            self.fLogStreamJACK.setCodec("UTF-8")

            if self.fLogFileJACK.size() > self.MAX_INITIAL_SIZE:
                self.fLogStreamJACK.seek(self.fLogFileJACK.size() - self.MAX_INITIAL_SIZE)

        if self.LOG_FILE_A2J is not None:
            self.fLogFileA2J = QFile(self.LOG_FILE_A2J)
            self.fLogFileA2J.open(QIODevice.ReadOnly)
            self.fLogStreamA2J = QTextStream(self.fLogFileA2J)
            self.fLogStreamA2J.setCodec("UTF-8")

            if self.fLogFileA2J.size() > self.MAX_INITIAL_SIZE:
                self.fLogStreamA2J.seek(self.fLogFileA2J.size() - self.MAX_INITIAL_SIZE)

        if self.LOG_FILE_LASH is not None:
            self.fLogFileLASH = QFile(self.LOG_FILE_LASH)
            self.fLogFileLASH.open(QIODevice.ReadOnly)
            self.fLogStreamLASH = QTextStream(self.fLogFileLASH)
            self.fLogStreamLASH.setCodec("UTF-8")

            if self.fLogFileLASH.size() > self.MAX_INITIAL_SIZE:
                self.fLogStreamLASH.seek(self.fLogFileLASH.size() - self.MAX_INITIAL_SIZE)

        if self.LOG_FILE_LADISH is not None:
            self.fLogFileLADISH = QFile(self.LOG_FILE_LADISH)
            self.fLogFileLADISH.open(QIODevice.ReadOnly)
            self.fLogStreamLADISH = QTextStream(self.fLogFileLADISH)
            self.fLogStreamLADISH.setCodec("UTF-8")

            if self.fLogFileLADISH.size() > self.MAX_INITIAL_SIZE:
                self.fLogStreamLADISH.seek(self.fLogFileLADISH.size() - self.MAX_INITIAL_SIZE)

    def closeNow(self):
        self.fCloseNow = True

    def purgeLogs(self):
        self.fPurgeLogs = True

    def run(self):
        # -------------------------------------------------------------
        # Read logs and set text in main thread

        while not self.fCloseNow:
            if self.fPurgeLogs:
                if self.LOG_FILE_JACK:
                    self.fLogStreamJACK.flush()
                    self.fLogFileJACK.close()
                    self.fLogFileJACK.open(QIODevice.WriteOnly)
                    self.fLogFileJACK.close()
                    self.fLogFileJACK.open(QIODevice.ReadOnly)

                if self.LOG_FILE_A2J:
                    self.fLogStreamA2J.flush()
                    self.fLogFileA2J.close()
                    self.fLogFileA2J.open(QIODevice.WriteOnly)
                    self.fLogFileA2J.close()
                    self.fLogFileA2J.open(QIODevice.ReadOnly)

                if self.LOG_FILE_LASH:
                    self.fLogStreamLASH.flush()
                    self.fLogFileLASH.close()
                    self.fLogFileLASH.open(QIODevice.WriteOnly)
                    self.fLogFileLASH.close()
                    self.fLogFileLASH.open(QIODevice.ReadOnly)

                if self.LOG_FILE_LADISH:
                    self.fLogStreamLADISH.flush()
                    self.fLogFileLADISH.close()
                    self.fLogFileLADISH.open(QIODevice.WriteOnly)
                    self.fLogFileLADISH.close()
                    self.fLogFileLADISH.open(QIODevice.ReadOnly)

                self.fPurgeLogs = False

            else:
                if self.LOG_FILE_JACK:
                    textJACK = fixLogText(self.fLogStreamJACK.readAll()).strip()
                else:
                    textJACK = ""

                if self.LOG_FILE_A2J:
                    textA2J = fixLogText(self.fLogStreamA2J.readAll()).strip()
                else:
                    textA2J = ""

                if self.LOG_FILE_LASH:
                    textLASH = fixLogText(self.fLogStreamLASH.readAll()).strip()
                else:
                    textLASH = ""

                if self.LOG_FILE_LADISH:
                    textLADISH = fixLogText(self.fLogStreamLADISH.readAll()).strip()
                else:
                    textLADISH = ""

                self.fRealParent.setLogsText(textJACK, textA2J, textLASH, textLADISH)
                self.updateLogs.emit()

            if not self.fCloseNow:
                self.sleep(1)

        # -------------------------------------------------------------
        # Close logs before closing thread

        if self.LOG_FILE_JACK:
            self.fLogFileJACK.close()

        if self.LOG_FILE_A2J:
            self.fLogFileA2J.close()

        if self.LOG_FILE_LASH:
            self.fLogFileLASH.close()

        if self.LOG_FILE_LADISH:
            self.fLogFileLADISH.close()

# ------------------------------------------------------------------------------------------------------------
# Logs Window

class LogsW(QDialog):
    LOG_PATH = os.path.join(HOME, ".log")

    LOG_FILE_JACK   = os.path.join(LOG_PATH, "jack", "jackdbus.log")
    LOG_FILE_A2J    = os.path.join(LOG_PATH, "a2j", "a2j.log")
    LOG_FILE_LASH   = os.path.join(LOG_PATH, "lash", "lash.log")
    LOG_FILE_LADISH = os.path.join(LOG_PATH, "ladish", "ladish.log")

    if not os.path.exists(LOG_FILE_JACK):
        LOG_FILE_JACK = None

    if not os.path.exists(LOG_FILE_A2J):
        LOG_FILE_A2J = None

    if not os.path.exists(LOG_FILE_LASH):
        LOG_FILE_LASH = None

    if not os.path.exists(LOG_FILE_LADISH):
        LOG_FILE_LADISH = None

    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()
    SIGUSR2 = pyqtSignal()

    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_logs.Ui_LogsW()
        self.ui.setupUi(self)

        self.loadSettings()

        self.fFirstRun = True
        self.fTextLock = QMutex()

        self.fTextJACK   = ""
        self.fTextA2J    = ""
        self.fTextLASH   = ""
        self.fTextLADISH = ""

        # -------------------------------------------------------------
        # Set-up GUI

        self.ui.b_close.setIcon(getIcon("window-close"))
        self.ui.b_purge.setIcon(getIcon("user-trash"))

        # -------------------------------------------------------------
        # Check for non-existing logs and remove tabs for those

        tabIndex = 0

        if self.LOG_FILE_JACK is None:
            self.ui.tabWidget.removeTab(0 - tabIndex)
            tabIndex += 1

        if self.LOG_FILE_A2J is None:
            self.ui.tabWidget.removeTab(1 - tabIndex)
            tabIndex += 1

        if self.LOG_FILE_LASH is None:
            self.ui.tabWidget.removeTab(2 - tabIndex)
            tabIndex += 1

        if self.LOG_FILE_LADISH is None:
            self.ui.tabWidget.removeTab(3 - tabIndex)
            tabIndex += 1

        # -------------------------------------------------------------
        # Init logs viewers

        if self.LOG_FILE_JACK:
            self.fSyntaxJACK = SyntaxHighlighter_JACK(self.ui.pte_jack)
            self.fSyntaxJACK.setDocument(self.ui.pte_jack.document())

        if self.LOG_FILE_A2J:
            self.fSyntaxA2J = SyntaxHighlighter_A2J(self.ui.pte_a2j)
            self.fSyntaxA2J.setDocument(self.ui.pte_a2j.document())

        if self.LOG_FILE_LASH:
            self.fSyntaxLASH = SyntaxHighlighter_LASH(self.ui.pte_lash)
            self.fSyntaxLASH.setDocument(self.ui.pte_lash.document())

        if self.LOG_FILE_LADISH:
            self.SyntaxLADISH = SyntaxHighlighter_LADISH(self.ui.pte_ladish)
            self.SyntaxLADISH.setDocument(self.ui.pte_ladish.document())

        # -------------------------------------------------------------
        # Init file read thread

        self.fReadThread = LogsReadThread(self)
        self.fReadThread.start(QThread.IdlePriority)

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.b_purge.clicked.connect(self.slot_purgeLogs)
        self.fReadThread.updateLogs.connect(self.slot_updateLogs)

        # -------------------------------------------------------------

    def setLogsText(self, textJACK, textA2J, textLASH, textLADISH):
        QMutexLocker(self.fTextLock)

        self.fTextJACK   = textJACK
        self.fTextA2J    = textA2J
        self.fTextLASH   = textLASH
        self.fTextLADISH = textLADISH

    @pyqtSlot()
    def slot_updateLogs(self):
        QMutexLocker(self.fTextLock)

        if self.fFirstRun:
            self.ui.pte_jack.clear()
            self.ui.pte_a2j.clear()
            self.ui.pte_lash.clear()
            self.ui.pte_ladish.clear()

        if self.LOG_FILE_JACK and self.fTextJACK:
            self.ui.pte_jack.appendPlainText(self.fTextJACK)

        if self.LOG_FILE_A2J and self.fTextA2J:
            self.ui.pte_a2j.appendPlainText(self.fTextA2J)

        if self.LOG_FILE_LASH and self.fTextLASH:
            self.ui.pte_lash.appendPlainText(self.fTextLASH)

        if self.LOG_FILE_LADISH and self.fTextLADISH:
            self.ui.pte_ladish.appendPlainText(self.fTextLADISH)

        if self.fFirstRun:
            self.ui.pte_jack.horizontalScrollBar().setValue(0)
            self.ui.pte_jack.verticalScrollBar().setValue(self.ui.pte_jack.verticalScrollBar().maximum())
            self.ui.pte_a2j.horizontalScrollBar().setValue(0)
            self.ui.pte_a2j.verticalScrollBar().setValue(self.ui.pte_a2j.verticalScrollBar().maximum())
            self.ui.pte_lash.horizontalScrollBar().setValue(0)
            self.ui.pte_lash.verticalScrollBar().setValue(self.ui.pte_lash.verticalScrollBar().maximum())
            self.ui.pte_ladish.horizontalScrollBar().setValue(0)
            self.ui.pte_ladish.verticalScrollBar().setValue(self.ui.pte_ladish.verticalScrollBar().maximum())
            self.fFirstRun = False

    @pyqtSlot()
    def slot_purgeLogs(self):
        self.fReadThread.purgeLogs()
        self.ui.pte_jack.clear()
        self.ui.pte_a2j.clear()
        self.ui.pte_lash.clear()
        self.ui.pte_ladish.clear()

    def loadSettings(self):
        settings = QSettings("Cadence", "Cadence-Logs")
        self.restoreGeometry(settings.value("Geometry", b""))

    def saveSettings(self):
        settings = QSettings("Cadence", "Cadence-Logs")
        settings.setValue("Geometry", self.saveGeometry())

    def closeEvent(self, event):
        self.saveSettings()

        if self.fReadThread.isRunning():
            self.fReadThread.closeNow()

            if not self.fReadThread.wait(2000):
                self.fReadThread.terminate()

        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Allow to use this as a standalone app

if __name__ == '__main__':
    # Additional imports
    from PyQt5.QtWidgets import QApplication

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Cadence-Logs")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/cadence.svg"))
    setup_i18n()

    # Show GUI
    gui = LogsW(None)
    gui.show()

    setUpSignals(gui)

    # App-Loop
    sys.exit(app.exec_())

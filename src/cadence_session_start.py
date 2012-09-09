#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Imports (Global)
import dbus, os, sys
from PyQt4.QtCore import QCoreApplication, QProcess, QSettings
from time import sleep

# Imports (Custom Stuff)
from shared import *

# Cadence Global Settings
GlobalSettings = QSettings("Cadence", "GlobalSettings")

DEFAULT_LADSPA_PATH = "/usr/lib/ladspa:/usr/local/lib/ladspa:/usr/lib32/ladspa:"+HOME+"/.ladspa"
DEFAULT_DSSI_PATH = "/usr/lib/dssi:/usr/local/lib/dssi:/usr/lib32/dssi:"+HOME+"/.dssi"
DEFAULT_LV2_PATH  = "/usr/lib/lv2:/usr/local/lib/lv2:"+HOME+"/.lv2"
DEFAULT_VST_PATH  = "/usr/lib/vst:/usr/local/lib/vst:/usr/lib32/vst:"+HOME+"/.vst"

# DBus
class DBus(object):
    __slots__ = [
      'bus',
      'a2j',
      'jack'
    ]
DBus = DBus()

# Start JACK, A2J and Pulse, according to user settings
def startSession():
    pass

def printLADSPA_PATH():
    EXTRA_LADSPA_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_LADSPA_PATH", "", type=str).split(":")
    LADSPA_PATH_str = DEFAULT_LADSPA_PATH

    for i in range(len(EXTRA_LADSPA_DIRS)):
        if (EXTRA_LADSPA_DIRS[i]):
            LADSPA_PATH_str += ":"+EXTRA_LADSPA_DIRS[i]

    print(LADSPA_PATH_str)

def printDSSI_PATH():
    EXTRA_DSSI_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_DSSI_PATH", "", type=str).split(":")
    DSSI_PATH_str = DEFAULT_DSSI_PATH

    for i in range(len(EXTRA_DSSI_DIRS)):
        if (EXTRA_DSSI_DIRS[i]):
            DSSI_PATH_str += ":"+EXTRA_DSSI_DIRS[i]

    print(DSSI_PATH_str)

def printLV2_PATH():
    EXTRA_LV2_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_LV2_PATH", "", type=str).split(":")
    LV2_PATH_str = DEFAULT_LV2_PATH

    for i in range(len(EXTRA_LV2_DIRS)):
        if (EXTRA_LV2_DIRS[i]):
            LV2_PATH_str += ":"+EXTRA_LV2_DIRS[i]

    print(LV2_PATH_str)

def printVST_PATH():
    EXTRA_VST_DIRS = GlobalSettings.value("AudioPlugins/EXTRA_VST_PATH", "", type=str).split(":")
    VST_PATH_str = DEFAULT_VST_PATH

    for i in range(len(EXTRA_VST_DIRS)):
      if (EXTRA_VST_DIRS[i]):
        VST_PATH_str += ":"+EXTRA_VST_DIRS[i]

    print(VST_PATH_str)

def printArguments():
    print("\t-s|--start  \tStart session")
    print("")
    print("\t-h|--help   \tShow this help message")
    print("\t-v|--version\tShow version")

def printError(cmd):
    print("Invalid arguments")
    print("Run '%s -h' for help" % (cmd))

def printHelp(cmd):
    print("Usage: %s [cmd]" % (cmd))
    printArguments()

def printVersion():
    print("Cadence version %s" % (VERSION))
    print("Developed by falkTX and the rest of the KXStudio Team")

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QCoreApplication(sys.argv)
    app.setApplicationName("Cadence")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")

    # Check arguments
    cmd = sys.argv[0]

    if len(app.arguments()) == 1:
        printHelp(cmd)
    elif len(app.arguments()) == 2:
        arg = app.arguments()[1]
        if arg == "--printLADSPA_PATH":
            printLADSPA_PATH()
        elif arg == "--printDSSI_PATH":
            printDSSI_PATH()
        elif arg == "--printLV2_PATH":
            printLV2_PATH()
        elif arg == "--printVST_PATH":
            printVST_PATH()
        elif arg in ["-s", "--s", "-start", "--start"]:
            sys.exit(startSession())
        elif arg in ["-h", "--h", "-help", "--help"]:
            printHelp(cmd)
        elif arg in ["-v", "--v", "-version", "--version"]:
            printVersion()
        else:
            printError(cmd)
    else:
        printError(cmd)

    # Exit
    sys.exit(0)


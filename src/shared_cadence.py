#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Imports (Global)
import os
from PyQt4.QtCore import QProcess, QSettings
from time import sleep

# Imports (Custom Stuff)
from shared import *

# Default Plugin PATHs
DEFAULT_LADSPA_PATH = [
    os.path.join(HOME, ".ladspa"),
    os.path.join("/", "usr", "lib", "ladspa"),
    os.path.join("/", "usr", "local", "lib", "ladspa")
]

DEFAULT_DSSI_PATH = [
    os.path.join(HOME, ".dssi"),
    os.path.join("/", "usr", "lib", "dssi"),
    os.path.join("/", "usr", "local", "lib", "dssi")
]

DEFAULT_LV2_PATH = [
    os.path.join(HOME, ".lv2"),
    os.path.join("/", "usr", "lib", "lv2"),
    os.path.join("/", "usr", "local", "lib", "lv2")
]

DEFAULT_VST_PATH = [
    os.path.join(HOME, ".vst"),
    os.path.join("/", "usr", "lib", "vst"),
    os.path.join("/", "usr", "local", "lib", "vst")
]

# Global Settings
GlobalSettings = QSettings("Cadence", "GlobalSettings")

# Get Process list
# TODO - Windows support, others
def getProcList():
    retProcs = []

    process = QProcess()
    process.start("ps", ["-e"])
    process.waitForFinished()

    processDump = process.readAllStandardOutput().split("\n")

    for i in range(len(processDump)):
        if (i == 0): continue
        dumpTest = str(processDump[i], encoding="utf-8").rsplit(":", 1)[-1].split(" ")
        if len(dumpTest) > 1 and dumpTest[1]:
            retProcs.append(dumpTest[1])

    return retProcs

def stopAllAudioProcesses():
    process = QProcess()

    procsTerm = ["a2j", "a2jmidid", "artsd", "jackd", "jackdmp", "knotify4", "lash", "ladishd", "ladiappd", "ladiconfd", "jmcore"]
    procsKill = ["jackdbus", "pulseaudio"]
    tries = 20

    process.start("killall", procsTerm)
    process.waitForFinished()

    for x in range(tries):
        procsList = getProcList()
        for term in range(len(procsTerm)):
            if term in procsList:
                break
            else:
                sleep(0.1)
        else:
            break

    process.start("killall", ["-KILL"] + procsKill)
    process.waitForFinished()

    for x in range(tries):
        procsList = getProcList()
        for kill in range(len(procsKill)):
            if kill in procsList:
                break
            else:
                sleep(0.1)
        else:
            break

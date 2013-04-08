#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Cadence ALSA-Loop daemon
# Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

import os
import sys
from signal import signal, SIGINT, SIGTERM
from time import sleep
from PyQt4.QtCore import QProcess

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import jacklib

# --------------------------------------------------
# Auto re-activate if on good kernel

global reactivateCounter
reactivateCounter = -1
isKernelGood      = os.uname()[2] >= "3.8"

# --------------------------------------------------
# Global loop check

global doLoop, doRunNow, useZita, procIn, procOut
doLoop    = True
doRunNow  = True
useZita   = False
procIn    = QProcess()
procOut   = QProcess()
checkFile = "/tmp/.cadence-aloop-daemon.x"

# --------------------------------------------------
# Global JACK variables

global bufferSize, sampleRate, channels
bufferSize = 1024
sampleRate = 44100
channels   = 2

# --------------------------------------------------
# quit on SIGINT or SIGTERM

def signal_handler(sig, frame=0):
    global doLoop
    doLoop = False

# --------------------------------------------------
# listen to jack buffer-size and sample-rate changes

def buffer_size_callback(newBufferSize, arg):
    global doRunNow, bufferSize
    bufferSize = newBufferSize
    doRunNow = True
    return 0

def sample_rate_callback(newSampleRate, arg):
    global doRunNow, sampleRate
    sampleRate = newSampleRate
    doRunNow = True
    return 0

# --------------------------------------------------
# listen to jack2 master switch

def client_registration_callback(clientName, register, arg):
    if clientName == b"system" and register:
        print("NOTICE: Possible JACK2 master switch")
        global bufferSize, sampleRate, doRunNow
        bufferSize = jacklib.get_buffer_size(client)
        sampleRate = jacklib.get_sample_rate(client)
        doRunNow   = True

    elif clientName in (b"alsa2jack", b"jack2alsa") and not register:
        global doLoop, reactivateCounter, useZita
        if doRunNow or not doLoop:
            return

        if isKernelGood:
            if reactivateCounter == -1:
                reactivateCounter = 0
                print("NOTICE: %s has been stopped, waiting 5 secs to reactivate" % ("zita-a2j/j2a" if useZita else "alsa_in/out"))
        elif doLoop:
            doLoop = False
            print("NOTICE: %s has been stopped, quitting now..." % ("zita-a2j/j2a" if useZita else "alsa_in/out"))

# --------------------------------------------------
# listen to jack shutdown

def shutdown_callback(arg):
    global doLoop
    doLoop = False

# --------------------------------------------------
# run alsa_in and alsa_out
def run_alsa_bridge():
    global bufferSize, sampleRate, channels
    global procIn, procOut
    global useZita

    if procIn.state() != QProcess.NotRunning:
        procIn.terminate()
        procIn.waitForFinished(1000)
    if procOut.state() != QProcess.NotRunning:
        procOut.terminate()
        procOut.waitForFinished(1000)

    if useZita:
        procIn.start("env",  ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "zita-a2j", "-d", "hw:Loopback,1,0", "-r", "%i" % sampleRate, "-p", "%i" % bufferSize, "-j", "alsa2jack", "-c", "%i" % channels])
        procOut.start("env", ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "zita-j2a", "-d", "hw:Loopback,1,1", "-r", "%i" % sampleRate, "-p", "%i" % bufferSize, "-j", "jack2alsa", "-c", "%i" % channels])
    else:
        procIn.start("env",  ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_in",  "-d", "cloop", "%i" % sampleRate, "-p", "%i" % bufferSize, "-j", "alsa2jack", "-q", "1", "-c", "%i" % channels])
        procOut.start("env", ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_out", "-d", "ploop", "%i" % sampleRate, "-p", "%i" % bufferSize, "-j", "jack2alsa", "-q", "1", "-c", "%i" % channels])

    if procIn.waitForStarted():
        sleep(1)
        jacklib.connect(client, "alsa2jack:capture_1", "system:playback_1")
        jacklib.connect(client, "alsa2jack:capture_2", "system:playback_2")

    if procOut.waitForStarted():
        sleep(1)
        jacklib.connect(client, "system:capture_1", "jack2alsa:playback_1")
        jacklib.connect(client, "system:capture_2", "jack2alsa:playback_2")

#--------------- main ------------------
if __name__ == '__main__':

    for i in range(len(sys.argv)):
        if i == 0: continue

        argv = sys.argv[i]

        if argv == "--zita":
            useZita = True
        elif argv.startswith("--channels="):
            chStr = argv.replace("--channels=", "")

            if chStr.isdigit():
                channels = int(chStr)

    # Init JACK client
    client = jacklib.client_open("cadence-aloop-daemon", jacklib.JackUseExactName, None)

    if not client:
        print("cadence-aloop-daemon is already running, delete \"/tmp/.cadence-aloop-daemon.x\" to close it")
        quit()

    if jacklib.JACK2:
        jacklib.set_client_registration_callback(client, client_registration_callback, None)

    jacklib.set_buffer_size_callback(client, buffer_size_callback, None)
    jacklib.set_sample_rate_callback(client, sample_rate_callback, None)
    jacklib.on_shutdown(client, shutdown_callback, None)
    jacklib.activate(client)

    # Quit when requested
    signal(SIGINT, signal_handler)
    signal(SIGTERM, signal_handler)

    # Get initial values
    sampleRate = jacklib.get_sample_rate(client)
    bufferSize = jacklib.get_buffer_size(client)

    # Create check file
    if not os.path.exists(checkFile):
        os.mknod(checkFile)

    # Keep running until told otherwise
    firstStart = True
    while doLoop and os.path.exists(checkFile):
        if doRunNow:
            if firstStart:
                firstStart = False
                print("cadence-aloop-daemon started, using %s and %i channels" % ("zita-a2j/j2a" if useZita else "alsa_in/out", channels))

            run_alsa_bridge()
            doRunNow = False

        elif isKernelGood and reactivateCounter >= 0:
            if reactivateCounter == 5:
                reactivateCounter = -1
                doRunNow = True
            else:
                reactivateCounter += 1

        sleep(1)

    # Close JACK client
    jacklib.deactivate(client)
    jacklib.client_close(client)

    if os.path.exists(checkFile):
        os.remove(checkFile)

    if procIn.state() != QProcess.NotRunning:
        procIn.terminate()
        procIn.waitForFinished(1000)
    if procOut.state() != QProcess.NotRunning:
        procOut.terminate()
        procOut.waitForFinished(1000)

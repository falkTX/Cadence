#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Imports (Global)
import os
from signal import signal, SIGINT, SIGTERM
from time import sleep
from PyQt4.QtCore import QProcess

# Imports (Custom Stuff)
import jacklib

# --------------------------------------------------
# Global loop check

global doLoop, doRunNow, procIn, procOut
doLoop    = True
doRunNow  = True
procIn    = QProcess()
procOut   = QProcess()
checkFile = "/tmp/.cadence-aloop-daemon.x"

# --------------------------------------------------
# Global JACK variables

global bufferSize, sampleRate
bufferSize = 1024
sampleRate = 44100

# --------------------------------------------------
# quit on SIGINT or SIGTERM

def signal_handler(sig, frame=0):
    global doLoop
    doLoop = False

# --------------------------------------------------
# listen to jack buffer size changes

def buffer_size_callback(newBufferSize, arg):
    global doRunNow, bufferSize
    bufferSize = newBufferSize
    doRunNow = True
    return 0

# --------------------------------------------------
# listen to jack shutdown

def shutdown_callback(arg):
    global doLoop
    doLoop = False

# --------------------------------------------------
# run alsa_in and alsa_out
def run_alsa_bridge():
    global bufferSize, sampleRate
    global procIn, procOut

    if procIn.state() != QProcess.NotRunning:
        procIn.terminate()
        procIn.waitForFinished(1000)
    if procOut.state() != QProcess.NotRunning:
        procOut.terminate()
        procOut.waitForFinished(1000)

    procIn.start("env",  ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_in",  "-j", "alsa2jack", "-d", "cloop", "-q", "1"])
    procOut.start("env", ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_out", "-j", "jack2alsa", "-d", "ploop", "-q", "1"])

    procIn.waitForStarted()
    procOut.waitForStarted()

    # Pause it for a bit, and connect to the system ports
    sleep(1)
    jacklib.connect(client, "alsa2jack:capture_1", "system:playback_1")
    jacklib.connect(client, "alsa2jack:capture_2", "system:playback_2")
    jacklib.connect(client, "system:capture_1", "jack2alsa:playback_1")
    jacklib.connect(client, "system:capture_2", "jack2alsa:playback_2")

#--------------- main ------------------
if __name__ == '__main__':

    # Init JACK client
    client = jacklib.client_open("cadence-aloop-daemon", jacklib.JackUseExactName, None)

    if not client:
        quit()

    jacklib.set_buffer_size_callback(client, buffer_size_callback, None)
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
    while doLoop and os.path.exists(checkFile):
        if doRunNow:
            doRunNow = False
            run_alsa_bridge()
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

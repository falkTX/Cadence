#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Imports (Global)
import os, sys
from signal import signal, SIGINT, SIGTERM
from time import sleep
from PyQt4.QtCore import QProcess

# Imports (Custom Stuff)
import jacklib

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
    global doRunNow
    if clientName == b"system" and register:
        print("NOTICE: Possible JACK2 master switch")
        global bufferSize, sampleRate
        doRunNow   = True
        sampleRate = jacklib.get_sample_rate(client)
        bufferSize = jacklib.get_buffer_size(client)
    elif clientName in (b"alsa2jack", b"jack2alsa") and not register:
        global doLoop, useZita
        if doLoop and not doRunNow:
            doLoop = False
            print("NOTICE: %s have been stopped, quitting now..." % ("zita-a2j/j2a" if useZita else "alsa_in/out"))

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
        procIn.start("env",  ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "zita-a2j", "-L", "-j", "alsa2jack", "-d", "hw:Loopback,1,0"])
        procOut.start("env", ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "zita-j2a", "-L", "-j", "jack2alsa", "-d", "hw:Loopback,1,1"])
    else:
        procIn.start("env",  ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_in",  "-j", "alsa2jack", "-d", "cloop", "-q", "1", "-c", "%i" % channels])
        procOut.start("env", ["JACK_SAMPLE_RATE=%i" % sampleRate, "JACK_PERIOD_SIZE=%i" % bufferSize, "alsa_out", "-j", "jack2alsa", "-d", "ploop", "-q", "1", "-c", "%i" % channels])

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

    useZita = bool(len(sys.argv) == 2 and sys.argv[1] in ("-zita", "--zita"))

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
            run_alsa_bridge()
            doRunNow = False

            if firstStart:
                firstStart = False
                print("cadence-aloop-daemon started, using %s" % ("zita-a2j/j2a" if useZita else "alsa_in/out"))

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

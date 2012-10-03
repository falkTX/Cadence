#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Imports (Global)
from ctypes import *
from os import path, system
from sys import version_info
from signal import signal, SIGINT, SIGTERM
from time import sleep

# Imports (Custom Stuff)
import jacklib

# --------------------------------------------------
# Global loop check

global doLoop, doRunNow
doLoop   = True
doRunNow = True

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
    global sample_rate, buffer_size
    killList = "alsa_in alsa_out zita-a2j zita-j2a"

    # On KXStudio, pulseaudio is nicely bridged & configured to work with JACK
    if not path.exists("/usr/share/kxstudio/config/pulse/daemon.conf"):
        killList += " pulseaudio"

    system("killall %s" % killList)

    #if (False):
    system("env JACK_SAMPLE_RATE=%i JACK_PERIOD_SIZE=%i alsa_in  -j alsa2jack -d cloop -q 1 2>&1 1> /dev/null &" % (sampleRate, bufferSize))
    system("env JACK_SAMPLE_RATE=%i JACK_PERIOD_SIZE=%i alsa_out -j jack2alsa -d ploop -q 1 2>&1 1> /dev/null &" % (sampleRate, bufferSize))
    #else:
      #system("env JACK_SAMPLE_RATE=%i JACK_PERIOD_SIZE=%i zita-a2j -j alsa2jack -d hw:Loopback,1,0 -r 44100 &" % (sample_rate, buffer_size))
      #system("env JACK_SAMPLE_RATE=%i JACK_PERIOD_SIZE=%i zita-j2a -j jack2alsa -d hw:Loopback,1,1 -r 44100 &" % (sample_rate, buffer_size))

    # Pause it for a bit, and connect to the system ports
    sleep(2)
    jacklib.connect(client, "alsa2jack:capture_1", "system:playback_1")
    jacklib.connect(client, "alsa2jack:capture_2", "system:playback_2")
    jacklib.connect(client, "system:capture_1", "jack2alsa:playback_1")
    jacklib.connect(client, "system:capture_2", "jack2alsa:playback_2")

#--------------- main ------------------
if __name__ == '__main__':

    # Init JACK client
    client = jacklib.client_open("cadence-aloop-daemon", 0, None)

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

    # Keep running until told otherwise
    while doLoop:
        if doRunNow:
            doRunNow = False
            run_alsa_bridge()
        sleep(1)

    # Close JACK client
    jacklib.deactivate(client)
    jacklib.client_close(client)

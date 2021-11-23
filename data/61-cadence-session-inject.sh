#!/bin/sh

# Cadence Session Startup Injection
# Set plugin paths and start JACK (or not) according to user settings

INSTALL_PREFIX="X-PREFIX-X"

if [ -f $INSTALL_PREFIX/bin/cadence-session-start ]; then

export CADENCE_AUTO_STARTED="true"

export LADSPA_PATH="`$INSTALL_PREFIX/bin/cadence-session-start --printLADSPA_PATH`"
export DSSI_PATH="`$INSTALL_PREFIX/bin/cadence-session-start --printDSSI_PATH`"
export LV2_PATH="`$INSTALL_PREFIX/bin/cadence-session-start --printLV2_PATH`"
export VST_PATH="`$INSTALL_PREFIX/bin/cadence-session-start --printVST_PATH`"

STARTUP="$INSTALL_PREFIX/bin/cadence-session-start --system-start-by-x11-startup $STARTUP"

fi

unset INSTALL_PREFIX

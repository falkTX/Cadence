#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

# Special rule for AVLinux
AVLINUX_PY2BUILD = False

if AVLINUX_PY2BUILD:
    from sip import setapi
    setapi("QString", 2)
    setapi("QVariant", 1)

# Imports (Global)
import os, sys
from ctypes import *
from copy import deepcopy

if AVLINUX_PY2BUILD:
    from commands import getoutput
else:
    from subprocess import Popen, PIPE

# Imports (Custom)
from shared import *

try:
    import ladspa_rdf
    haveLRDF = True
except:
    print("LRDF Support not available (LADSPA-RDF will be disabled)")
    haveLRDF = False

if AVLINUX_PY2BUILD:
    is64bit = True # TODO ------------------------- TODO ---------- TODO False
    c_uintptr = c_uint32
elif sys.int_info[1] == 4:
    is64bit = True
    c_uintptr = c_uint64
else:
    is64bit = False
    c_uintptr = c_uint32

# Convert a ctypes struct into a dict
def struct_to_dict(struct):
    return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------
# Default Plugin Folders

if WINDOWS:
    splitter = ";"
    APPDATA = os.getenv("APPDATA")
    PROGRAMFILES = os.getenv("PROGRAMFILES")
    COMMONPROGRAMFILES = os.getenv("COMMONPROGRAMFILES")

    # Small integrity tests
    if not APPDATA:
        print("APPDATA variable not set, cannot continue")
        sys.exit(1)

    if not PROGRAMFILES:
        print("PROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    if not COMMONPROGRAMFILES:
        print("COMMONPROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    DEFAULT_LADSPA_PATH = [
        os.path.join(APPDATA, "LADSPA"),
        os.path.join(PROGRAMFILES, "LADSPA")
    ]

    DEFAULT_DSSI_PATH = [
        os.path.join(APPDATA, "DSSI"),
        os.path.join(PROGRAMFILES, "DSSI")
    ]

    DEFAULT_LV2_PATH = [
        os.path.join(APPDATA, "LV2"),
        os.path.join(COMMONPROGRAMFILES, "LV2")
    ]

    DEFAULT_VST_PATH = [
        os.path.join(PROGRAMFILES, "VstPlugins"),
        os.path.join(PROGRAMFILES, "Steinberg", "VstPlugins")
    ]

    DEFAULT_GIG_PATH = [
        os.path.join(APPDATA, "GIG"),
    ]

    DEFAULT_SF2_PATH = [
        os.path.join(APPDATA, "SF2"),
    ]

    DEFAULT_SFZ_PATH = [
        os.path.join(APPDATA, "SFZ"),
    ]

    #if (is64bit):
    # TODO

elif MACOS:
    splitter = ":"

    DEFAULT_LADSPA_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LADSPA"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "LADSPA")
    ]

    DEFAULT_DSSI_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "DSSI"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "DSSI")
    ]

    DEFAULT_LV2_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LV2"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "LV2")
    ]

    DEFAULT_VST_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "VST"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "VST")
    ]

    DEFAULT_GIG_PATH = [
        # TODO
    ]

    DEFAULT_SF2_PATH = [
        # TODO
    ]

    DEFAULT_SFZ_PATH = [
        # TODO
    ]

else:
    splitter = ":"

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

    DEFAULT_GIG_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "gig")
    ]

    DEFAULT_SF2_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "sf2")
    ]

    DEFAULT_SFZ_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "sfz")
    ]

# ------------------------------------------------------------------------------------------------
# Search for Carla library and tools

global carla_library_path
carla_library_path = ""

carla_discovery_unix32 = ""
carla_discovery_unix64 = ""
carla_discovery_win32  = ""
carla_discovery_win64  = ""

carla_bridge_unix32 = ""
carla_bridge_unix64 = ""
carla_bridge_win32  = ""
carla_bridge_win64  = ""

carla_bridge_lv2_gtk2 = ""
carla_bridge_lv2_qt4  = ""
carla_bridge_lv2_x11  = ""
carla_bridge_vst_x11  = ""

if WINDOWS:
    carla_libname = "carla_backend.dll"
elif MACOS:
    carla_libname = "carla_backend.dylib"
else:
    carla_libname = "carla_backend.so"

CWD = sys.path[0]

# make it work with cxfreeze
if CWD.endswith("/carla"):
    CWD = CWD.rsplit("/carla", 1)[0]

# find carla_library_path
if os.path.exists(os.path.join(CWD, "carla-backend", carla_libname)):
    carla_library_path = os.path.join(CWD, "carla-backend", carla_libname)
else:
    if WINDOWS:
        CARLA_PATH = (os.path.join(PROGRAMFILES, "Cadence", "carla"),)
    elif MACOS:
        # TODO
        CARLA_PATH = ("/usr/lib", "/usr/local/lib/")
    else:
        CARLA_PATH = ("/usr/lib", "/usr/local/lib/")

    for p in CARLA_PATH:
        if os.path.exists(os.path.join(p, "carla", carla_libname)):
            carla_library_path = os.path.join(p, "carla", carla_libname)
            break

# find carla_discovery_unix32
if os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-unix32")):
    carla_discovery_unix32 = os.path.join(CWD, "carla-discovery", "carla-discovery-unix32")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-discovery-unix32")):
            carla_discovery_unix32 = os.path.join(p, "carla-discovery-unix32")
            break

# find carla_discovery_unix64
if os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-unix64")):
    carla_discovery_unix64 = os.path.join(CWD, "carla-discovery", "carla-discovery-unix64")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-discovery-unix64")):
            carla_discovery_unix64 = os.path.join(p, "carla-discovery-unix64")
            break

# find carla_discovery_win32
if os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-win32.exe")):
    carla_discovery_win32 = os.path.join(CWD, "carla-discovery", "carla-discovery-win32.exe")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-discovery-wine32.exe")):
            carla_discovery_win32 = os.path.join(p, "carla-discovery-win32.exe")
            break

# find carla_discovery_win64
if os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-win64.exe")):
    carla_discovery_win64 = os.path.join(CWD, "carla-discovery", "carla-discovery-win64.exe")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-discovery-win64.exe")):
            carla_discovery_win64 = os.path.join(p, "carla-discovery-win64.exe")
            break

# find carla_bridge_unix32
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-unix32")):
    carla_bridge_unix32 = os.path.join(CWD, "carla-bridge", "carla-bridge-unix32")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-unix32")):
            carla_bridge_unix32 = os.path.join(p, "carla-bridge-unix32")
            break

# find carla_bridge_unix64
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-unix64")):
    carla_bridge_unix64 = os.path.join(CWD, "carla-bridge", "carla-bridge-unix64")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-unix64")):
            carla_bridge_unix64 = os.path.join(p, "carla-bridge-unix64")
            break

# find carla_bridge_win32
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-win32.exe")):
    carla_bridge_win32 = os.path.join(CWD, "carla-bridge", "carla-bridge-win32.exe")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-wine32.exe")):
            carla_bridge_win32 = os.path.join(p, "carla-bridge-win32.exe")
            break

# find carla_bridge_win64
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-win64.exe")):
    carla_bridge_win64 = os.path.join(CWD, "carla-bridge", "carla-bridge-win64.exe")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-win64.exe")):
            carla_bridge_win64 = os.path.join(p, "carla-bridge-win64.exe")
            break

# find carla_bridge_lv2_gtk2
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-gtk2")):
    carla_bridge_lv2_gtk2 = os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-gtk2")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-lv2-gtk2")):
            carla_bridge_lv2_gtk2 = os.path.join(p, "carla-bridge-lv2-gtk2")
            break

# find carla_bridge_lv2_qt4
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-qt4")):
    carla_bridge_lv2_qt4 = os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-qt4")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-lv2-qt4")):
            carla_bridge_lv2_qt4 = os.path.join(p, "carla-bridge-lv2-qt4")
            break

# find carla_bridge_lv2_x11
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-x11")):
    carla_bridge_lv2_x11 = os.path.join(CWD, "carla-bridge", "carla-bridge-lv2-x11")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-lv2-x11")):
            carla_bridge_lv2_x11 = os.path.join(p, "carla-bridge-lv2-x11")
            break

# find carla_bridge_vst_x11
if os.path.exists(os.path.join(CWD, "carla-bridge", "carla-bridge-vst-x11")):
    carla_bridge_vst_x11 = os.path.join(CWD, "carla-bridge", "carla-bridge-vst-x11")
else:
    for p in PATH:
        if os.path.exists(os.path.join(p, "carla-bridge-vst-x11")):
            carla_bridge_vst_x11 = os.path.join(p, "carla-bridge-vst-x11")
            break

# ------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(bPATH, OS):
    binaries = []

    if OS == "WINDOWS":
        extensions = (".dll",)
    elif OS == "MACOS":
        extensions = (".dylib", ".so")
    else:
        extensions = (".so", ".sO", ".SO", ".So")

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            binaries.append(os.path.join(root, name))

    return binaries

def findLV2Bundles(bPATH):
    bundles = []
    extensions = (".lv2", ".lV2", ".LV2", ".Lv2")

    for root, dirs, files in os.walk(bPATH):
        for dir_ in [dir_ for dir_ in dirs if dir_.endswith(extensions)]:
            bundles.append(os.path.join(root, dir_))

    return bundles

def findSoundKits(bPATH, stype):
    soundfonts = []

    if stype == "gig":
        extensions = (".gig", ".giG", ".gIG", ".GIG", ".GIg", ".Gig")
    elif stype == "sf2":
        extensions = (".sf2", ".sF2", ".SF2", ".Sf2")
    elif stype == "sfz":
        extensions = (".sfz", ".sfZ", ".sFZ", ".SFZ", ".SFz", ".Sfz")
    else:
        extensions = ()

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            soundfonts.append(os.path.join(root, name))

    return soundfonts

def findDSSIGUI(filename, name, label):
    plugin_dir = filename.rsplit(".", 1)[0]
    short_name = os.path.basename(plugin_dir)
    gui_filename = ""

    check_name  = name.replace(" ", "_")
    check_label = label
    check_sname = short_name

    if check_name[-1]  != "_": check_name += "_"
    if check_label[-1] != "_": check_label += "_"
    if check_sname[-1] != "_": check_sname += "_"

    for root, dirs, files in os.walk(plugin_dir):
        gui_files = files
        break
    else:
        gui_files = []

    for gui in gui_files:
        if gui.startswith(check_name) or gui.startswith(check_label) or gui.startswith(check_sname):
            gui_filename = os.path.join(plugin_dir, gui)
            break

    return gui_filename

# ------------------------------------------------------------------------------------------------
# Plugin Query

PyPluginInfo = {
    'build': 0, # BINARY_NONE
    'type': 0, # PLUGIN_NONE,
    'hints': 0x0,
    'binary': "",
    'name': "",
    'label': "",
    'maker': "",
    'copyright': "",
    'unique_id': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'audio.totals': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'midi.totals': 0,
    'parameters.ins': 0,
    'parameters.outs': 0,
    'parameters.total': 0,
    'programs.total': 0
}

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
    fake_label = os.path.basename(filename).rsplit(".", 1)[0]
    plugins = []
    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        if isWine:
            command.append("WINEDEBUG=-all")

    command.append(tool)
    command.append(stype)
    if AVLINUX_PY2BUILD:
        command.append('"%s"' % filename)
    else:
        command.append(filename)

    if AVLINUX_PY2BUILD:
        try:
            output = getoutput(" ".join(command)).split("\n")
        except:
            output = []
    else:
        Ps = Popen(command, stdout=PIPE)
        Ps.wait()
        output = Ps.stdout.read().decode("utf-8", errors="ignore").split("\n")

    pinfo = None

    for line in output:
        if line == "carla-discovery::init::-----------":
            pinfo = deepcopy(PyPluginInfo)
            pinfo['type'] = itype
            pinfo['binary'] = filename

        elif line == "carla-discovery::end::------------":
            if pinfo != None:
                plugins.append(pinfo)
                pinfo = None

        elif line == "Segmentation fault":
            print("carla-discovery::crash::%s crashed during discovery" % filename)

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::error::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::"):
            if pinfo == None:
                continue

            prop, value = line.replace("carla-discovery::", "").split("::", 1)

            if prop == "name":
                pinfo['name'] = value if value else fake_label
            elif prop == "label":
                pinfo['label'] = value if value else fake_label
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "copyright":
                pinfo['copyright'] = value
            elif prop == "unique_id":
                if value.isdigit(): pinfo['unique_id'] = int(value)
            elif prop == "hints":
                if value.isdigit(): pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit(): pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit(): pinfo['audio.outs'] = int(value)
            elif prop == "audio.total":
                if value.isdigit(): pinfo['audio.total'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit(): pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit(): pinfo['midi.outs'] = int(value)
            elif prop == "midi.total":
                if value.isdigit(): pinfo['midi.total'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit(): pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit(): pinfo['parameters.outs'] = int(value)
            elif prop == "parameters.total":
                if value.isdigit(): pinfo['parameters.total'] = int(value)
            elif prop == "programs.total":
                if value.isdigit(): pinfo['programs.total'] = int(value)
            elif prop == "build":
                if value.isdigit(): pinfo['build'] = int(value)

    # Additional checks
    for pinfo in plugins:
        if itype == PLUGIN_DSSI:
            if findDSSIGUI(pinfo['binary'], pinfo['name'], pinfo['label']):
                pinfo['hints'] |= PLUGIN_HAS_GUI

    return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginLV2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, isWine)

def checkPluginVST(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST, "VST", filename, tool, isWine)

def checkPluginGIG(filename, tool):
    return runCarlaDiscovery(PLUGIN_GIG, "GIG", filename, tool)

def checkPluginSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkPluginSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

# ------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int
c_nullptr = None

# static max values
MAX_PLUGINS     = 99
MAX_PARAMETERS  = 200

# plugin hints
PLUGIN_IS_BRIDGE   = 0x01
PLUGIN_IS_SYNTH    = 0x02
PLUGIN_HAS_GUI     = 0x04
PLUGIN_USES_CHUNKS = 0x08
PLUGIN_CAN_DRYWET  = 0x10
PLUGIN_CAN_VOLUME  = 0x20
PLUGIN_CAN_BALANCE = 0x40

# parameter hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# enum BinaryType
BINARY_NONE   = 0
BINARY_UNIX32 = 1
BINARY_UNIX64 = 2
BINARY_WIN32  = 3
BINARY_WIN64  = 4

# enum PluginType
PLUGIN_NONE   = 0
PLUGIN_LADSPA = 1
PLUGIN_DSSI   = 2
PLUGIN_LV2    = 3
PLUGIN_VST    = 4
PLUGIN_GIG    = 5
PLUGIN_SF2    = 6
PLUGIN_SFZ    = 7

# enum PluginCategory
PLUGIN_CATEGORY_NONE      = 0
PLUGIN_CATEGORY_SYNTH     = 1
PLUGIN_CATEGORY_DELAY     = 2 # also Reverb
PLUGIN_CATEGORY_EQ        = 3
PLUGIN_CATEGORY_FILTER    = 4
PLUGIN_CATEGORY_DYNAMICS  = 5 # Amplifier, Compressor, Gate
PLUGIN_CATEGORY_MODULATOR = 6 # Chorus, Flanger, Phaser
PLUGIN_CATEGORY_UTILITY   = 7 # Analyzer, Converter, Mixer
PLUGIN_CATEGORY_OTHER     = 8 # used to check if a plugin has a category

# enum ParameterType
PARAMETER_UNKNOWN = 0
PARAMETER_INPUT   = 1
PARAMETER_OUTPUT  = 2
PARAMETER_LATENCY = 3

# enum InternalParametersIndex
PARAMETER_ACTIVE = -1
PARAMETER_DRYWET = -2
PARAMETER_VOLUME = -3
PARAMETER_BALANCE_LEFT  = -4
PARAMETER_BALANCE_RIGHT = -5

# enum CustomDataType
CUSTOM_DATA_INVALID = 0
CUSTOM_DATA_STRING  = 1
CUSTOM_DATA_PATH    = 2
CUSTOM_DATA_CHUNK   = 3
CUSTOM_DATA_BINARY  = 4

# enum GuiType
GUI_NONE = 0
GUI_INTERNAL_QT4  = 1
GUI_INTERNAL_X11  = 2
GUI_INTERNAL_HWND = 3
GUI_EXTERNAL_LV2  = 4
GUI_EXTERNAL_OSC  = 5

# enum OptionsType
OPTION_MAX_PARAMETERS       = 1
OPTION_GLOBAL_JACK_CLIENT   = 2
OPTION_PREFER_UI_BRIDGES    = 3
OPTION_PROCESS_HQ           = 4
OPTION_OSC_GUI_TIMEOUT      = 5
OPTION_USE_DSSI_CHUNKS      = 6
OPTION_PATH_LADSPA          = 7
OPTION_PATH_DSSI            = 8
OPTION_PATH_LV2             = 9
OPTION_PATH_VST             = 10
OPTION_PATH_GIG             = 11
OPTION_PATH_SF2             = 12
OPTION_PATH_SFZ             = 13
OPTION_PATH_BRIDGE_UNIX32   = 14
OPTION_PATH_BRIDGE_UNIX64   = 15
OPTION_PATH_BRIDGE_WIN32    = 16
OPTION_PATH_BRIDGE_WIN64    = 17
OPTION_PATH_BRIDGE_LV2_GTK2 = 18
OPTION_PATH_BRIDGE_LV2_QT4  = 19
OPTION_PATH_BRIDGE_LV2_X11  = 20
OPTION_PATH_BRIDGE_VST_X11  = 21

# enum CallbackType
CALLBACK_DEBUG                = 0
CALLBACK_PARAMETER_CHANGED    = 1 # parameter_id, 0, value
CALLBACK_PROGRAM_CHANGED      = 2 # program_id, 0, 0
CALLBACK_MIDI_PROGRAM_CHANGED = 3 # midi_program_id, 0, 0
CALLBACK_NOTE_ON              = 4 # key, velocity, 0
CALLBACK_NOTE_OFF             = 5 # key, velocity, 0
CALLBACK_SHOW_GUI             = 6 # show? (0|1, -1=quit), 0, 0
CALLBACK_RESIZE_GUI           = 7 # width, height, 0
CALLBACK_UPDATE               = 8
CALLBACK_RELOAD_INFO          = 9
CALLBACK_RELOAD_PARAMETERS    = 10
CALLBACK_RELOAD_PROGRAMS      = 11
CALLBACK_RELOAD_ALL           = 12
CALLBACK_QUIT                 = 13

class ParameterData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("index", c_int32),
        ("rindex", c_int32),
        ("hints", c_int32),
        ("midi_channel", c_uint8),
        ("midi_cc", c_int16)
    ]

class ParameterRanges(Structure):
    _fields_ = [
        ("def", c_double),
        ("min", c_double),
        ("max", c_double),
        ("step", c_double),
        ("step_small", c_double),
        ("step_large", c_double)
    ]

class CustomData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("key", c_char_p),
        ("value", c_char_p)
    ]

class PluginInfo(Structure):
    _fields_ = [
        ("valid", c_bool),
        ("type", c_enum),
        ("category", c_enum),
        ("hints", c_uint),
        ("binary", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p),
        ("unique_id", c_long)
    ]

class PortCountInfo(Structure):
    _fields_ = [
        ("valid", c_bool),
        ("ins", c_uint32),
        ("outs", c_uint32),
        ("total", c_uint32)
    ]

class ParameterInfo(Structure):
    _fields_ = [
        ("valid", c_bool),
        ("name", c_char_p),
        ("symbol", c_char_p),
        ("unit", c_char_p),
        ("scalepoint_count", c_uint32)
    ]

class ScalePointInfo(Structure):
    _fields_ = [
        ("valid", c_bool),
        ("value", c_double),
        ("label", c_char_p)
    ]

class MidiProgramInfo(Structure):
    _fields_ = [
        ("valid", c_bool),
        ("bank", c_uint32),
        ("program", c_uint32),
        ("label", c_char_p)
    ]

class GuiInfo(Structure):
    _fields_ = [
        ("type", c_enum),
        ("resizable", c_bool),
    ]

CallbackFunc = CFUNCTYPE(None, c_enum, c_ushort, c_int, c_int, c_double)

if LINUX or MACOS:
    BINARY_NATIVE = BINARY_UNIX64 if is64bit else BINARY_UNIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if is64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_NONE

# ------------------------------------------------------------------------------------------------
# Backend C++ -> Python object

global Callback
Callback = None

class Host(object):
    def __init__(self, lib_prefix_arg):
        object.__init__(self)

        global carla_library_path

        if lib_prefix_arg:
            carla_library_path = os.path.join(lib_prefix_arg, "lib", "carla", carla_libname)

        self.lib = cdll.LoadLibrary(carla_library_path)

        self.lib.engine_init.argtypes = [c_char_p]
        self.lib.engine_init.restype = c_bool

        self.lib.engine_close.argtypes = None
        self.lib.engine_close.restype = c_bool

        self.lib.is_engine_running.argtypes = None
        self.lib.is_engine_running.restype = c_bool

        self.lib.add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_void_p]
        self.lib.add_plugin.restype = c_short

        self.lib.remove_plugin.argtypes = [c_ushort]
        self.lib.remove_plugin.restype = c_bool

        self.lib.get_plugin_info.argtypes = [c_ushort]
        self.lib.get_plugin_info.restype = POINTER(PluginInfo)

        self.lib.get_audio_port_count_info.argtypes = [c_ushort]
        self.lib.get_audio_port_count_info.restype = POINTER(PortCountInfo)

        self.lib.get_midi_port_count_info.argtypes = [c_ushort]
        self.lib.get_midi_port_count_info.restype = POINTER(PortCountInfo)

        self.lib.get_parameter_count_info.argtypes = [c_ushort]
        self.lib.get_parameter_count_info.restype = POINTER(PortCountInfo)

        self.lib.get_parameter_info.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_info.restype = POINTER(ParameterInfo)

        self.lib.get_scalepoint_info.argtypes = [c_ushort, c_uint32, c_uint32]
        self.lib.get_scalepoint_info.restype = POINTER(ScalePointInfo)

        self.lib.get_midi_program_info.argtypes = [c_ushort, c_uint32]
        self.lib.get_midi_program_info.restype = POINTER(MidiProgramInfo)

        self.lib.get_gui_info.argtypes = [c_ushort]
        self.lib.get_gui_info.restype = POINTER(GuiInfo)

        self.lib.get_parameter_data.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_data.restype = POINTER(ParameterData)

        self.lib.get_parameter_ranges.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_ranges.restype = POINTER(ParameterRanges)

        self.lib.get_custom_data.argtypes = [c_ushort, c_uint32]
        self.lib.get_custom_data.restype = POINTER(CustomData)

        self.lib.get_chunk_data.argtypes = [c_ushort]
        self.lib.get_chunk_data.restype = c_char_p

        self.lib.get_parameter_count.argtypes = [c_ushort]
        self.lib.get_parameter_count.restype = c_uint32

        self.lib.get_program_count.argtypes = [c_ushort]
        self.lib.get_program_count.restype = c_uint32

        self.lib.get_midi_program_count.argtypes = [c_ushort]
        self.lib.get_midi_program_count.restype = c_uint32

        self.lib.get_custom_data_count.argtypes = [c_ushort]
        self.lib.get_custom_data_count.restype = c_uint32

        self.lib.get_parameter_text.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_text.restype = c_char_p

        self.lib.get_program_name.argtypes = [c_ushort, c_uint32]
        self.lib.get_program_name.restype = c_char_p

        self.lib.get_midi_program_name.argtypes = [c_ushort, c_uint32]
        self.lib.get_midi_program_name.restype = c_char_p

        self.lib.get_real_plugin_name.argtypes = [c_ushort]
        self.lib.get_real_plugin_name.restype = c_char_p

        self.lib.get_current_program_index.argtypes = [c_ushort]
        self.lib.get_current_program_index.restype = c_int32

        self.lib.get_current_midi_program_index.argtypes = [c_ushort]
        self.lib.get_current_midi_program_index.restype = c_int32

        self.lib.get_default_parameter_value.argtypes = [c_ushort, c_uint32]
        self.lib.get_default_parameter_value.restype = c_double

        self.lib.get_current_parameter_value.argtypes = [c_ushort, c_uint32]
        self.lib.get_current_parameter_value.restype = c_double

        self.lib.get_input_peak_value.argtypes = [c_ushort, c_ushort]
        self.lib.get_input_peak_value.restype = c_double

        self.lib.get_output_peak_value.argtypes = [c_ushort, c_ushort]
        self.lib.get_output_peak_value.restype = c_double

        self.lib.set_active.argtypes = [c_ushort, c_bool]
        self.lib.set_active.restype = None

        self.lib.set_drywet.argtypes = [c_ushort, c_double]
        self.lib.set_drywet.restype = None

        self.lib.set_volume.argtypes = [c_ushort, c_double]
        self.lib.set_volume.restype = None

        self.lib.set_balance_left.argtypes = [c_ushort, c_double]
        self.lib.set_balance_left.restype = None

        self.lib.set_balance_right.argtypes = [c_ushort, c_double]
        self.lib.set_balance_right.restype = None

        self.lib.set_parameter_value.argtypes = [c_ushort, c_uint32, c_double]
        self.lib.set_parameter_value.restype = None

        self.lib.set_parameter_midi_channel.argtypes = [c_ushort, c_uint32, c_uint8]
        self.lib.set_parameter_midi_channel.restype = None

        self.lib.set_parameter_midi_cc.argtypes = [c_ushort, c_uint32, c_int16]
        self.lib.set_parameter_midi_cc.restype = None

        self.lib.set_program.argtypes = [c_ushort, c_uint32]
        self.lib.set_program.restype = None

        self.lib.set_midi_program.argtypes = [c_ushort, c_uint32]
        self.lib.set_midi_program.restype = None

        self.lib.set_custom_data.argtypes = [c_ushort, c_enum, c_char_p, c_char_p]
        self.lib.set_custom_data.restype = None

        self.lib.set_chunk_data.argtypes = [c_ushort, c_char_p]
        self.lib.set_chunk_data.restype = None

        self.lib.set_gui_data.argtypes = [c_ushort, c_int, c_uintptr]
        self.lib.set_gui_data.restype = None

        self.lib.show_gui.argtypes = [c_ushort, c_bool]
        self.lib.show_gui.restype = None

        self.lib.idle_guis.argtypes = None
        self.lib.idle_guis.restype = None

        self.lib.send_midi_note.argtypes = [c_ushort, c_uint8, c_uint8]
        self.lib.send_midi_note.restype = None

        self.lib.prepare_for_save.argtypes = [c_ushort]
        self.lib.prepare_for_save.restype = None

        self.lib.set_callback_function.argtypes = [CallbackFunc]
        self.lib.set_callback_function.restype = None

        self.lib.set_option.argtypes = [c_enum, c_int, c_char_p]
        self.lib.set_option.restype = None

        self.lib.get_last_error.argtypes = None
        self.lib.get_last_error.restype = c_char_p

        self.lib.get_host_client_name.argtypes = None
        self.lib.get_host_client_name.restype = c_char_p

        self.lib.get_host_osc_url.argtypes = None
        self.lib.get_host_osc_url.restype = c_char_p

        self.lib.get_buffer_size.argtypes = None
        self.lib.get_buffer_size.restype = c_uint32

        self.lib.get_sample_rate.argtypes = None
        self.lib.get_sample_rate.restype = c_double

        self.lib.get_latency.argtypes = None
        self.lib.get_latency.restype = c_double

    def engine_init(self, client_name):
        if AVLINUX_PY2BUILD:
            return self.lib.engine_init(client_name)
        return self.lib.engine_init(client_name.encode("utf-8"))

    def engine_close(self):
        return self.lib.engine_close()

    def is_engine_running(self):
        return self.lib.is_engine_running()

    def add_plugin(self, btype, ptype, filename, label, extra_stuff):
        if AVLINUX_PY2BUILD:
            return self.lib.add_plugin(btype, ptype, filename, label, cast(extra_stuff, c_void_p))
        return self.lib.add_plugin(btype, ptype, filename.encode("utf-8"), label.encode("utf-8"), cast(extra_stuff, c_void_p))

    def remove_plugin(self, plugin_id):
        return self.lib.remove_plugin(plugin_id)

    def get_plugin_info(self, plugin_id):
        return struct_to_dict(self.lib.get_plugin_info(plugin_id).contents)

    def get_audio_port_count_info(self, plugin_id):
        return struct_to_dict(self.lib.get_audio_port_count_info(plugin_id).contents)

    def get_midi_port_count_info(self, plugin_id):
        return struct_to_dict(self.lib.get_midi_port_count_info(plugin_id).contents)

    def get_parameter_count_info(self, plugin_id):
        return struct_to_dict(self.lib.get_parameter_count_info(plugin_id).contents)

    def get_parameter_info(self, plugin_id, parameter_id):
        return struct_to_dict(self.lib.get_parameter_info(plugin_id, parameter_id).contents)

    def get_scalepoint_info(self, plugin_id, parameter_id, scalepoint_id):
        return struct_to_dict(self.lib.get_scalepoint_info(plugin_id, parameter_id, scalepoint_id).contents)

    def get_midi_program_info(self, plugin_id, midi_program_id):
        return struct_to_dict(self.lib.get_midi_program_info(plugin_id, midi_program_id).contents)

    def get_parameter_data(self, plugin_id, parameter_id):
        return struct_to_dict(self.lib.get_parameter_data(plugin_id, parameter_id).contents)

    def get_parameter_ranges(self, plugin_id, parameter_id):
        return struct_to_dict(self.lib.get_parameter_ranges(plugin_id, parameter_id).contents)

    def get_custom_data(self, plugin_id, custom_data_id):
        return struct_to_dict(self.lib.get_custom_data(plugin_id, custom_data_id).contents)

    def get_chunk_data(self, plugin_id):
        return self.lib.get_chunk_data(plugin_id)

    def get_gui_info(self, plugin_id):
        return struct_to_dict(self.lib.get_gui_info(plugin_id).contents)

    def get_parameter_count(self, plugin_id):
        return self.lib.get_parameter_count(plugin_id)

    def get_program_count(self, plugin_id):
        return self.lib.get_program_count(plugin_id)

    def get_midi_program_count(self, plugin_id):
        return self.lib.get_midi_program_count(plugin_id)

    def get_custom_data_count(self, plugin_id):
        return self.lib.get_custom_data_count(plugin_id)

    def get_parameter_text(self, plugin_id, program_id):
        return self.lib.get_parameter_text(plugin_id, program_id)

    def get_program_name(self, plugin_id, program_id):
        return self.lib.get_program_name(plugin_id, program_id)

    def get_midi_program_name(self, plugin_id, midi_program_id):
        return self.lib.get_midi_program_name(plugin_id, midi_program_id)

    def get_real_plugin_name(self, plugin_id):
        return self.lib.get_real_plugin_name(plugin_id)

    def get_current_program_index(self, plugin_id):
        return self.lib.get_current_program_index(plugin_id)

    def get_current_midi_program_index(self, plugin_id):
        return self.lib.get_current_midi_program_index(plugin_id)

    def get_default_parameter_value(self, plugin_id, parameter_id):
        return self.lib.get_default_parameter_value(plugin_id, parameter_id)

    def get_current_parameter_value(self, plugin_id, parameter_id):
        return self.lib.get_current_parameter_value(plugin_id, parameter_id)

    def get_input_peak_value(self, plugin_id, port_id):
        return self.lib.get_input_peak_value(plugin_id, port_id)

    def get_output_peak_value(self, plugin_id, port_id):
        return self.lib.get_output_peak_value(plugin_id, port_id)

    def set_active(self, plugin_id, onoff):
        self.lib.set_active(plugin_id, onoff)

    def set_drywet(self, plugin_id, value):
        self.lib.set_drywet(plugin_id, value)

    def set_volume(self, plugin_id, value):
        self.lib.set_volume(plugin_id, value)

    def set_balance_left(self, plugin_id, value):
        self.lib.set_balance_left(plugin_id, value)

    def set_balance_right(self, plugin_id, value):
        self.lib.set_balance_right(plugin_id, value)

    def set_parameter_value(self, plugin_id, parameter_id, value):
        self.lib.set_parameter_value(plugin_id, parameter_id, value)

    def set_parameter_midi_channel(self, plugin_id, parameter_id, channel):
        self.lib.set_parameter_midi_channel(plugin_id, parameter_id, channel)

    def set_parameter_midi_cc(self, plugin_id, parameter_id, midi_cc):
        self.lib.set_parameter_midi_cc(plugin_id, parameter_id, midi_cc)

    def set_program(self, plugin_id, program_id):
        self.lib.set_program(plugin_id, program_id)

    def set_midi_program(self, plugin_id, midi_program_id):
        self.lib.set_midi_program(plugin_id, midi_program_id)

    def set_custom_data(self, plugin_id, dtype, key, value):
        if AVLINUX_PY2BUILD:
            self.lib.set_custom_data(plugin_id, dtype, key, value)
        self.lib.set_custom_data(plugin_id, dtype, key.encode("utf-8"), value.encode("utf-8"))

    def set_chunk_data(self, plugin_id, chunk_data):
        if AVLINUX_PY2BUILD:
            self.lib.set_chunk_data(plugin_id, chunk_data)
        self.lib.set_chunk_data(plugin_id, chunk_data.encode("utf-8"))

    def set_gui_data(self, plugin_id, data, gui_addr):
        self.lib.set_gui_data(plugin_id, data, gui_addr)

    def show_gui(self, plugin_id, yesno):
        self.lib.show_gui(plugin_id, yesno)

    def idle_guis(self):
        self.lib.idle_guis()

    def send_midi_note(self, plugin_id, note, velocity):
        self.lib.send_midi_note(plugin_id, note, velocity)

    def prepare_for_save(self, plugin_id):
        self.lib.prepare_for_save(plugin_id)

    def set_callback_function(self, func):
        global Callback
        Callback = CallbackFunc(func)
        self.lib.set_callback_function(Callback)

    def set_option(self, option, value, value_str):
        if AVLINUX_PY2BUILD:
            self.lib.set_option(option, value, value_str)
        self.lib.set_option(option, value, value_str.encode("utf-8"))

    def get_last_error(self):
        return self.lib.get_last_error()

    def get_host_client_name(self):
        return self.lib.get_host_client_name()

    def get_host_osc_url(self):
        return self.lib.get_host_osc_url()

    def get_buffer_size(self):
        return self.lib.get_buffer_size()

    def get_sample_rate(self):
        return self.lib.get_sample_rate()

    def get_latency(self):
        return self.lib.get_latency()

# ------------------------------------------------------------------------------------------------
# Default Plugin Folders (set)

global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, SF2_PATH

LADSPA_PATH_env = os.getenv("LADSPA_PATH")
DSSI_PATH_env = os.getenv("DSSI_PATH")
LV2_PATH_env = os.getenv("LV2_PATH")
VST_PATH_env = os.getenv("VST_PATH")
GIG_PATH_env = os.getenv("GIG_PATH")
SF2_PATH_env = os.getenv("SF2_PATH")
SFZ_PATH_env = os.getenv("SFZ_PATH")

if LADSPA_PATH_env:
    LADSPA_PATH = LADSPA_PATH_env.split(splitter)
else:
    LADSPA_PATH = DEFAULT_LADSPA_PATH

if DSSI_PATH_env:
    DSSI_PATH = DSSI_PATH_env.split(splitter)
else:
    DSSI_PATH = DEFAULT_DSSI_PATH

if LV2_PATH_env:
    LV2_PATH = LV2_PATH_env.split(splitter)
else:
    LV2_PATH = DEFAULT_LV2_PATH

if VST_PATH_env:
    VST_PATH = VST_PATH_env.split(splitter)
else:
    VST_PATH = DEFAULT_VST_PATH

if GIG_PATH_env:
    GIG_PATH = GIG_PATH_env.split(splitter)
else:
    GIG_PATH = DEFAULT_GIG_PATH

if SF2_PATH_env:
    SF2_PATH = SF2_PATH_env.split(splitter)
else:
    SF2_PATH = DEFAULT_SF2_PATH

if SFZ_PATH_env:
    SFZ_PATH = SFZ_PATH_env.split(splitter)
else:
    SFZ_PATH = DEFAULT_SFZ_PATH

if haveLRDF:
    LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
    if LADSPA_RDF_PATH_env:
        ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))

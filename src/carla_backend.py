#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com> FIXME
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
import os, sys
from ctypes import *
from copy import deepcopy
from subprocess import getoutput

# Imports (Custom)
try:
  import ladspa_rdf
  haveRDF = True
except:
  print("RDF Support not available (LADSPA-RDF and LV2 will be disabled)")
  haveRDF = False

# Set Platform and Architecture
is64bit = False
LINUX   = False
MACOS   = False
WINDOWS = False

if (sys.platform == "darwin"):
  MACOS = True
elif (sys.platform.startswith("linux")):
  LINUX = True
  if (len(os.uname()) > 4 and os.uname()[4] in ('x86_64',)): #FIXME - need more checks
    is64bit = True
elif (sys.platform.startswith("win")):
  WINDOWS = True
  if (sys.platform == "win64"):
    is64bit = True

if (is64bit):
  c_intptr = c_int64
else:
  c_intptr = c_int32

# Get short filename from full filename (/a/b.c -> b.c)
def getShortFileName(filename):
  short = filename
  if (os.sep in filename):
    short = filename.rsplit(os.sep, 1)[1]
  return short

# Convert a ctypes struct into a dict
def struct_to_dict(struct):
  return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------
# Default Plugin Folders

if (WINDOWS):
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

  DEFAULT_SF2_PATH = [
    os.path.join(APPDATA, "SF2")
  ]

  #if (is64bit):
    # TODO

elif (MACOS):
  splitter = ":"
  HOME = os.getenv("HOME")

  # Small integrity tests
  if not HOME:
    print("HOME variable not set, cannot continue")
    sys.exit(1)

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

  DEFAULT_SF2_PATH = [
    # TODO
  ]

else:
  splitter = ":"
  HOME = os.getenv("HOME")

  # Small integrity tests
  if not HOME:
    print("HOME variable not set, cannot continue")
    sys.exit(1)

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

  DEFAULT_SF2_PATH = [
    os.path.join(HOME, ".sf2"),
    os.path.join("/", "usr", "share", "sounds", "sf2")
  ]

# ------------------------------------------------------------------------------------------------
# Carla command-line tools

carla_discovery_unix32 = ""
carla_discovery_unix64 = ""
carla_discovery_win32  = ""
carla_discovery_win64  = ""

#carla_bridge_lv2_gtk2  = ""
#carla_bridge_lv2_qt4   = ""
#carla_bridge_lv2_x11   = ""
#carla_bridge_vst_qt4   = ""
#carla_bridge_winvst    = ""

if (WINDOWS):
  PATH = (os.path.join(PROGRAMFILES, "Cadence", "carla"),)

else:
  PATH_env = os.getenv("PATH")

  if (PATH_env != None):
    PATH = PATH_env.split(":")
  else:
    PATH = ("/usr/bin", "/usr/local/bin")

CWD = sys.path[0]

# discovery-unix32
if (os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-unix32"))):
  carla_discovery_unix32 = os.path.join(CWD, "carla-discovery", "carla-discovery-unix32")
else:
  for p in PATH:
    if (os.path.exists(os.path.join(p, "carla-discovery-unix32"))):
      carla_discovery_unix32 = os.path.join(p, "carla-discovery-unix32")
      break

# discovery-unix64
if (os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-unix64"))):
  carla_discovery_unix64 = os.path.join(CWD, "carla-discovery", "carla-discovery-unix64")
else:
  for p in PATH:
    if (os.path.exists(os.path.join(p, "carla-discovery-unix64"))):
      carla_discovery_unix64 = os.path.join(p, "carla-discovery-unix64")
      break

# discovery-win32
if (os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-win32.exe"))):
  carla_discovery_win32 = os.path.join(CWD, "carla-discovery", "carla-discovery-win32.exe")
else:
  for p in PATH:
    if (os.path.exists(os.path.join(p, "carla-discovery-wine32.exe"))):
      carla_discovery_win32 = os.path.join(p, "carla-discovery-win32.exe")
      break

# discovery-win64
if (os.path.exists(os.path.join(CWD, "carla-discovery", "carla-discovery-win64.exe"))):
  carla_discovery_win64 = os.path.join(CWD, "carla-discovery", "carla-discovery-win64.exe")
else:
  for p in PATH:
    if (os.path.exists(os.path.join(p, "carla-discovery-win64.exe"))):
      carla_discovery_win64 = os.path.join(p, "carla-discovery-win64.exe")
      break

  ## lv2-gtk2
  #if (os.path.exists(os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-gtk2"))):
    #carla_bridge_lv2_gtk2 = os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-gtk2")
  #else:
    #for p in PATH:
      #if (os.path.exists(os.path.join(p, "carla-bridge-lv2-gtk2"))):
        #carla_bridge_lv2_gtk2 = os.path.join(p, "carla-bridge-lv2-gtk2")
        #break

  ## lv2-qt4
  #if (os.path.exists(os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-qt4"))):
    #carla_bridge_lv2_qt4 = os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-qt4")
  #else:
    #for p in PATH:
      #if (os.path.exists(os.path.join(p, "carla-bridge-lv2-qt4"))):
        #carla_bridge_lv2_qt4 = os.path.join(p, "carla-bridge-lv2-qt4")
        #break

  ## lv2-x11
  #if (os.path.exists(os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-x11"))):
    #carla_bridge_lv2_x11 = os.path.join(CWD, "carla-bridges", "carla-bridge-lv2-x11")
  #else:
    #for p in PATH:
      #if (os.path.exists(os.path.join(p, "carla-bridge-lv2-x11"))):
        #carla_bridge_lv2_x11 = os.path.join(p, "carla-bridge-lv2-x11")
        #break

  ## vst-qt4
  #if (os.path.exists(os.path.join(CWD, "carla-bridges", "carla-bridge-vst-qt4"))):
    #carla_bridge_vst_qt4 = os.path.join(CWD, "carla-bridges", "carla-bridge-vst-qt4")
  #else:
    #for p in PATH:
      #if (os.path.exists(os.path.join(p, "carla-bridge-vst-qt4"))):
        #carla_bridge_vst_qt4 = os.path.join(p, "carla-bridge-vst-qt4")
        #break

  ## winvst
  #if (os.path.exists(os.path.join(CWD, "carla-bridges", "carla-bridge-winvst.exe"))):
    #carla_bridge_winvst = os.path.join(CWD, "carla-bridges", "carla-bridge-winvst.exe")
  #else:
    #for p in PATH:
      #if (os.path.exists(os.path.join(p, "carla-bridge-winvst.exe"))):
        #carla_bridge_winvst = os.path.join(p, "carla-bridge-winvst.exe")
        #break

print("carla_discovery_unix32 ->", carla_discovery_unix32)
print("carla_discovery_unix64 ->", carla_discovery_unix64)
print("carla_discovery_win32  ->", carla_discovery_win32)
print("carla_discovery_win64  ->", carla_discovery_win64)

print("LADSPA ->", DEFAULT_LADSPA_PATH)
print("DSSI   ->", DEFAULT_DSSI_PATH)
print("LV2    ->", DEFAULT_LV2_PATH)
print("VST    ->", DEFAULT_VST_PATH)
print("SF2    ->", DEFAULT_SF2_PATH)

# ------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(PATH, OS):
  binaries = []

  if (OS == "WINDOWS"):
    extensions = (".dll", ".dlL", ".dLL", ".DLL", "DLl", "Dll")
  elif (OS == "MACOS"):
    extensions = (".dylib", ".so")
  else:
    extensions = (".so", ".sO", ".SO", ".So")

  for root, dirs, files in os.walk(PATH):
    for name in [name for name in files if name.endswith(extensions)]:
      binaries.append(os.path.join(root, name))

  return binaries

def findSoundFonts(PATH):
  soundfonts = []

  extensions = (".sf2", ".sF2", ".SF2", ".Sf2")

  for root, dirs, files in os.walk(PATH):
    for name in [name for name in files if name.endswith(extensions)]:
      soundfonts.append(os.path.join(root, name))

  return soundfonts

#def findLV2Bundles(PATH):
  #bundles = []
  #extensions = (".lv2", ".lV2", ".LV2", ".Lv2")

  #for root, dirs, files in os.walk(PATH):
    #for dir_ in [dir_ for dir_ in dirs if dir_.endswith(extensions)]:
      #bundles.append(os.path.join(root, dir_))

  #return bundles

def findDSSIGUI(filename, name, label):
  plugin_dir = filename.rsplit(".", 1)[0]
  short_name = getShortFileName(plugin_dir)
  gui_filename = ""

  check_name  = name.replace(" ","_")
  check_label = label
  check_sname = short_name

  if (check_name[-1]  != "_"): check_name  += "_"
  if (check_label[-1] != "_"): check_label += "_"
  if (check_sname[-1] != "_"): check_sname += "_"

  for root, dirs, files in os.walk(plugin_dir):
    gui_files = files
    break
  else:
    gui_files = []

  for gui in gui_files:
    if (gui.startswith(check_name) or gui.startswith(check_label) or gui.startswith(check_sname)):
      gui_filename = os.path.join(plugin_dir, gui)
      break

  return gui_filename

# ------------------------------------------------------------------------------------------------
# Plugin Query

PyPluginInfo = {
  'build':     0, # BINARY_NONE
  'type':      0, # PLUGIN_NONE,
  'category':  0, # PLUGIN_CATEGORY_NONE
  'hints':     0x0,
  'binary':    "",
  'name':      "",
  'label':     "",
  'maker':     "",
  'copyright': "",
  'unique_id': 0,
  'audio.ins':    0,
  'audio.outs':   0,
  'audio.totals': 0,
  'midi.ins':     0,
  'midi.outs':    0,
  'midi.totals':  0,
  'parameters.ins':   0,
  'parameters.outs':  0,
  'parameters.total': 0,
  'programs.total': 0
}

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
  short_name = getShortFileName(filename).rsplit(".", 1)[0]
  plugins = []
  pinfo = None

  command = ""

  if (LINUX or MACOS):
    command += "env LANG=C "
    if (isWine):
      command += "WINEDEBUG=-all "

  command += "%s %s \"%s\"" % (tool, stype, filename)

  try:
    output = getoutput(command).split("\n")
  except:
    output = []

  for line in output:
    if (line == "carla-discovery::init::-----------"):
      pinfo = deepcopy(PyPluginInfo)
      pinfo['type']   = itype
      pinfo['binary'] = filename

    elif (line == "carla-discovery::end::------------"):
      if (pinfo != None):
        plugins.append(pinfo)
        pinfo = None

    elif (line == "Segmentation fault"):
      print("carla-discovery::crash::%s crashed during discovery" % (filename))

    elif line.startswith("err:module:import_dll Library"):
      print(line)

    elif line.startswith("carla-discovery::error::"):
      print("%s - %s" % (line, filename))

    elif line.startswith("carla-discovery::"):
      if (pinfo == None):
        continue

      prop, value = line.replace("carla-discovery::","").split("::", 1)

      if (prop == "name"):
        pinfo['name'] = value if (value) else short_name
      elif (prop == "label"):
        pinfo['label'] = value if (value) else short_name
      elif (prop == "maker"):
        pinfo['maker'] = value
      elif (prop == "copyright"):
        pinfo['copyright'] = value
      elif (prop == "unique_id"):
        if value.isdigit(): pinfo['unique_id'] = int(value)
      elif (prop == "hints"):
        if value.isdigit(): pinfo['hints'] = int(value)
      elif (prop == "category"):
        if value.isdigit(): pinfo['category'] = int(value)
      elif (prop == "audio.ins"):
        if value.isdigit(): pinfo['audio.ins'] = int(value)
      elif (prop == "audio.outs"):
        if value.isdigit(): pinfo['audio.outs'] = int(value)
      elif (prop == "audio.total"):
        if value.isdigit(): pinfo['audio.total'] = int(value)
      elif (prop == "midi.ins"):
        if value.isdigit(): pinfo['midi.ins'] = int(value)
      elif (prop == "midi.outs"):
        if value.isdigit(): pinfo['midi.outs'] = int(value)
      elif (prop == "midi.total"):
        if value.isdigit(): pinfo['midi.total'] = int(value)
      elif (prop == "parameters.ins"):
        if value.isdigit(): pinfo['parameters.ins'] = int(value)
      elif (prop == "parameters.outs"):
        if value.isdigit(): pinfo['parameters.outs'] = int(value)
      elif (prop == "parameters.total"):
        if value.isdigit(): pinfo['parameters.total'] = int(value)
      elif (prop == "programs.total"):
        if value.isdigit(): pinfo['programs.total'] = int(value)
      elif (prop == "build"):
        if value.isdigit(): pinfo['build'] = int(value)

  # Additional checks
  for pinfo in plugins:
    if (itype == PLUGIN_DSSI):
      if (findDSSIGUI(pinfo['binary'], pinfo['name'], pinfo['label'])):
        pinfo['hints'] |= PLUGIN_HAS_GUI

  return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
  return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
  return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginVST(filename, tool, isWine=False):
  return runCarlaDiscovery(PLUGIN_VST, "VST", filename, tool, isWine)

def checkPluginSF2(filename, tool):
  return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

#def checkPluginLV2(rdf_info):
  #plugins = []

  #pinfo = deepcopy(PyPluginInfo)
  #pinfo['type'] = PLUGIN_LV2
  #pinfo['category'] = PLUGIN_CATEGORY_NONE # TODO
  #pinfo['hints'] = 0
  #pinfo['binary'] = rdf_info['Binary']
  #pinfo['name'] = rdf_info['Name']
  #pinfo['label'] = rdf_info['URI']
  #pinfo['maker'] = rdf_info['Author']
  #pinfo['copyright'] = rdf_info['License']
  #pinfo['id'] = str(rdf_info['UniqueID'])
  #pinfo['audio.ins'] = 0
  #pinfo['audio.outs'] = 0
  #pinfo['audio.total'] = 0
  #pinfo['midi.ins'] = 0
  #pinfo['midi.outs'] = 0
  #pinfo['midi.total'] = 0
  #pinfo['parameters.ins'] = 0
  #pinfo['parameters.outs'] = 0
  #pinfo['parameters.total'] = 0
  #pinfo['programs.total'] = rdf_info['PresetCount']

  #if (not rdf_info['Bundle'] or pinfo['binary'] == "" or pinfo['name'] == ""):
    #return None

  #for i in range(rdf_info['PortCount']):
    #PortType = rdf_info['Ports'][i]['Type']
    #PortProps = rdf_info['Ports'][i]['Properties']
    #if (PortType & lv2_rdf.LV2_PORT_AUDIO):
      #pinfo['audio.total'] += 1
      #if (PortType & lv2_rdf.LV2_PORT_INPUT):
        #pinfo['audio.ins'] += 1
      #elif (PortType & lv2_rdf.LV2_PORT_OUTPUT):
        #pinfo['audio.outs'] += 1
    #elif (PortType & lv2_rdf.LV2_PORT_EVENT_MIDI):
      #pinfo['midi.total'] += 1
      #if (PortType & lv2_rdf.LV2_PORT_INPUT):
        #pinfo['midi.ins'] += 1
      #elif (PortType & lv2_rdf.LV2_PORT_OUTPUT):
        #pinfo['midi.outs'] += 1
    #elif (PortType & lv2_rdf.LV2_PORT_CONTROL):
      #pinfo['parameters.total'] += 1
      #if (PortType & lv2_rdf.LV2_PORT_INPUT):
        #pinfo['parameters.ins'] += 1
      #elif (PortType & lv2_rdf.LV2_PORT_OUTPUT):
        #if (not PortProps & lv2_rdf.LV2_PORT_LATENCY):
          #pinfo['parameters.outs'] += 1

  #if (rdf_info['Type'] & lv2_rdf.LV2_GROUP_GENERATOR):
    #pinfo['hints'] |= PLUGIN_IS_SYNTH

  #if (rdf_info['UICount'] > 0):
    #pinfo['hints'] |= PLUGIN_HAS_GUI

  #plugins.append(pinfo)

  #return plugins

# ------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int
c_nullptr = None

# static max values
MAX_PLUGINS     = 99
MAX_PARAMETERS  = 200
MAX_MIDI_EVENTS = 512

# plugin hints
PLUGIN_HAS_GUI     = 0x01
PLUGIN_IS_BRIDGE   = 0x02
PLUGIN_IS_SYNTH    = 0x04
PLUGIN_USES_CHUNKS = 0x08
PLUGIN_CAN_DRYWET  = 0x10
PLUGIN_CAN_VOL     = 0x20
PLUGIN_CAN_BALANCE = 0x40

# parameter hints
PARAMETER_IS_ENABLED        = 0x01
PARAMETER_IS_AUTOMABLE      = 0x02
PARAMETER_HAS_STRICT_BOUNDS = 0x04
PARAMETER_USES_SCALEPOINTS  = 0x08
PARAMETER_USES_SAMPLERATE   = 0x10

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
PLUGIN_SF2    = 5

# enum PluginCategory
PLUGIN_CATEGORY_NONE      = 0
PLUGIN_CATEGORY_SYNTH     = 1
PLUGIN_CATEGORY_DELAY     = 2 # also Reverb
PLUGIN_CATEGORY_EQ        = 3
PLUGIN_CATEGORY_FILTER    = 4
PLUGIN_CATEGORY_DYNAMICS  = 5 # Amplifier, Compressor, Gate
PLUGIN_CATEGORY_MODULATOR = 6 # Chorus, Flanger, Phaser
PLUGIN_CATEGORY_UTILITY   = 7 # Analyzer, Converter, Mixer
PLUGIN_CATEGORY_OUTRO     = 8 # used to check if a plugin has a category

# enum ParameterType
PARAMETER_UNKNOWN = 0
PARAMETER_INPUT   = 1
PARAMETER_OUTPUT  = 2

# enum InternalParametersIndex
PARAMETER_ACTIVE = -1
PARAMETER_DRYWET = -2
PARAMETER_VOLUME = -3
PARAMETER_BALANCE_LEFT  = -4
PARAMETER_BALANCE_RIGHT = -5

# enum GuiType
GUI_NONE         = 0
GUI_INTERNAL_QT4 = 1
GUI_INTERNAL_X11 = 2
GUI_EXTERNAL_OSC = 3
GUI_EXTERNAL_LV2 = 4

# enum OptionsType
OPTION_GLOBAL_JACK_CLIENT = 1

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
    ("type", c_char_p),
    ("key", c_char_p),
    ("value", c_char_p)
  ]

class GuiData(Structure):
  _fields_ = [
    ("type", c_enum),
    ("visible", c_bool),
    ("resizable", c_bool),
    ("width", c_uint),
    ("height", c_uint),
    ("name", c_char_p), # DSSI Filename; LV2 Window Title
    ("show_now", c_bool)
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
    ("label", c_char_p),
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

class PluginBridgeInfo(Structure):
  _fields_ = [
    ("category", c_enum),
    ("hints", c_uint),
    ("name", c_char_p),
    ("maker", c_char_p),
    ("unique_id", c_long),
    ("ains", c_uint32),
    ("aouts", c_uint32)
  ]

CallbackFunc = CFUNCTYPE(None, c_enum, c_ushort, c_int, c_int, c_double)

if (LINUX or MACOS):
  BINARY_NATIVE = BINARY_UNIX64 if (is64bit) else BINARY_UNIX32
elif (WINDOWS):
  BINARY_NATIVE = BINARY_WIN64 if (is64bit) else BINARY_WIN32
else:
  BINARY_NATIVE = BINARY_NONE

# ------------------------------------------------------------------------------------------------
# Backend C++ -> Python object

class Host(object):
    def __init__(self):
        object.__init__(self)

        if (WINDOWS):
          libname = "carla_backend.dll"
        elif (MACOS):
          libname = "carla_backend.dylib"
        else:
          libname = "carla_backend.so"

        self.lib = cdll.LoadLibrary(os.path.join("carla", libname))

        self.lib.carla_init.argtypes = [c_char_p]
        self.lib.carla_init.restype = c_bool

        self.lib.carla_close.argtypes = None
        self.lib.carla_close.restype = c_bool

        self.lib.carla_is_engine_running.argtypes = None
        self.lib.carla_is_engine_running.restype = c_bool

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

        self.lib.get_parameter_data.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_data.restype = POINTER(ParameterData)

        self.lib.get_parameter_ranges.argtypes = [c_ushort, c_uint32]
        self.lib.get_parameter_ranges.restype = POINTER(ParameterRanges)

        self.lib.get_custom_data.argtypes = [c_ushort, c_uint32]
        self.lib.get_custom_data.restype = POINTER(CustomData)

        self.lib.get_chunk_data.argtypes = [c_ushort]
        self.lib.get_chunk_data.restype = c_char_p

        self.lib.get_gui_data.argtypes = [c_ushort]
        self.lib.get_gui_data.restype = POINTER(GuiData)

        self.lib.get_parameter_count.argtypes = [c_ushort]
        self.lib.get_parameter_count.restype = c_uint32

        self.lib.get_program_count.argtypes = [c_ushort]
        self.lib.get_program_count.restype = c_uint32

        self.lib.get_midi_program_count.argtypes = [c_ushort]
        self.lib.get_midi_program_count.restype = c_uint32

        self.lib.get_custom_data_count.argtypes = [c_ushort]
        self.lib.get_custom_data_count.restype = c_uint32

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

        self.lib.set_custom_data.argtypes = [c_ushort, c_char_p, c_char_p, c_char_p]
        self.lib.set_custom_data.restype = None

        self.lib.set_chunk_data.argtypes = [c_ushort, c_char_p]
        self.lib.set_chunk_data.restype = None

        self.lib.set_gui_data.argtypes = [c_ushort, c_int, c_intptr]
        self.lib.set_gui_data.restype = None

        self.lib.show_gui.argtypes = [c_ushort, c_bool]
        self.lib.show_gui.restype = None

        self.lib.idle_gui.argtypes = [c_ushort]
        self.lib.idle_gui.restype = None

        self.lib.send_midi_note.argtypes = [c_ushort, c_bool, c_uint8, c_uint8]
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

    def carla_init(self, client_name):
        return self.lib.carla_init(client_name.encode("utf-8"))

    def carla_close(self):
        return self.lib.carla_close()

    def carla_is_engine_running(self):
        return self.lib.carla_is_engine_running()

    def add_plugin(self, btype, ptype, filename, label, extra_stuff):
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

    def get_gui_data(self, plugin_id):
        return struct_to_dict(self.lib.get_gui_data(plugin_id).contents)

    def get_parameter_count(self, plugin_id):
        return self.lib.get_parameter_count(plugin_id)

    def get_program_count(self, plugin_id):
        return self.lib.get_program_count(plugin_id)

    def get_midi_program_count(self, plugin_id):
        return self.lib.get_midi_program_count(plugin_id)

    def get_custom_data_count(self, plugin_id):
        return self.lib.get_custom_data_count(plugin_id)

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
        return self.lib.set_custom_data(plugin_id, dtype, key, value)

    def set_chunk_data(self, plugin_id, chunk_data):
        self.lib.set_chunk_data(plugin_id, chunk_data.encode("utf-8"))

    def set_gui_data(self, plugin_id, data, gui_addr):
        self.lib.set_gui_data(plugin_id, data, gui_addr)

    def show_gui(self, plugin_id, yesno):
        self.lib.show_gui(plugin_id, yesno)

    def idle_gui(self, plugin_id):
        self.lib.idle_gui(plugin_id)

    def send_midi_note(self, plugin_id, onoff, note, velocity):
        self.lib.send_midi_note(plugin_id, onoff, note, velocity)

    def prepare_for_save(self, plugin_id):
        self.lib.prepare_for_save(plugin_id)

    def set_callback_function(self, func):
        global Callback
        Callback = CallbackFunc(func)
        self.lib.set_callback_function(Callback)

    def set_option(self, option, value, value_str):
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

LADSPA_PATH_env = os.getenv("LADSPA_PATH")
DSSI_PATH_env   = os.getenv("DSSI_PATH")
LV2_PATH_env    = os.getenv("LV2_PATH")
VST_PATH_env    = os.getenv("VST_PATH")
SF2_PATH_env    = os.getenv("SF2_PATH")

if (LADSPA_PATH_env):
  LADSPA_PATH = LADSPA_PATH_env.split(splitter)
else:
  LADSPA_PATH = DEFAULT_LADSPA_PATH

if (DSSI_PATH_env):
  DSSI_PATH = DSSI_PATH_env.split(splitter)
else:
  DSSI_PATH = DEFAULT_DSSI_PATH

if (LV2_PATH_env):
  LV2_PATH = LV2_PATH_env.split(splitter)
else:
  LV2_PATH = DEFAULT_LV2_PATH

if (VST_PATH_env):
  VST_PATH = VST_PATH_env.split(splitter)
else:
  VST_PATH = DEFAULT_VST_PATH

if (SF2_PATH_env):
  SF2_PATH = SF2_PATH_env.split(splitter)
else:
  SF2_PATH = DEFAULT_SF2_PATH

if (haveRDF):
  LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
  if (LADSPA_RDF_PATH_env):
    ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))

  #lv2_rdf.set_rdf_path(LV2_PATH)

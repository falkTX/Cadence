#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LADSPA RDF python support
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

# -------------------------------------------------------------------------------
#  C types

# Imports (Global)
from ctypes import *
from copy import deepcopy

# Base Types
LADSPA_Data = c_float
LADSPA_Property = c_int
LADSPA_PluginType = c_ulonglong

# Unit Types
LADSPA_UNIT_DB                = 0x01
LADSPA_UNIT_COEF              = 0x02
LADSPA_UNIT_HZ                = 0x04
LADSPA_UNIT_S                 = 0x08
LADSPA_UNIT_MS                = 0x10
LADSPA_UNIT_MIN               = 0x20

LADSPA_UNIT_CLASS_AMPLITUDE   = LADSPA_UNIT_DB|LADSPA_UNIT_COEF
LADSPA_UNIT_CLASS_FREQUENCY   = LADSPA_UNIT_HZ
LADSPA_UNIT_CLASS_TIME        = LADSPA_UNIT_S|LADSPA_UNIT_MS|LADSPA_UNIT_MIN

# Port Types (Official API)
LADSPA_PORT_INPUT             = 0x1
LADSPA_PORT_OUTPUT            = 0x2
LADSPA_PORT_CONTROL           = 0x4
LADSPA_PORT_AUDIO             = 0x8

# Port Hints
LADSPA_PORT_UNIT              = 0x1
LADSPA_PORT_DEFAULT           = 0x2
LADSPA_PORT_LABEL             = 0x4

# Plugin Types
LADSPA_CLASS_UTILITY          = 0x000000001
LADSPA_CLASS_GENERATOR        = 0x000000002
LADSPA_CLASS_SIMULATOR        = 0x000000004
LADSPA_CLASS_OSCILLATOR       = 0x000000008
LADSPA_CLASS_TIME             = 0x000000010
LADSPA_CLASS_DELAY            = 0x000000020
LADSPA_CLASS_PHASER           = 0x000000040
LADSPA_CLASS_FLANGER          = 0x000000080
LADSPA_CLASS_CHORUS           = 0x000000100
LADSPA_CLASS_REVERB           = 0x000000200
LADSPA_CLASS_FREQUENCY        = 0x000000400
LADSPA_CLASS_FREQUENCY_METER  = 0x000000800
LADSPA_CLASS_FILTER           = 0x000001000
LADSPA_CLASS_LOWPASS          = 0x000002000
LADSPA_CLASS_HIGHPASS         = 0x000004000
LADSPA_CLASS_BANDPASS         = 0x000008000
LADSPA_CLASS_COMB             = 0x000010000
LADSPA_CLASS_ALLPASS          = 0x000020000
LADSPA_CLASS_EQ               = 0x000040000
LADSPA_CLASS_PARAEQ           = 0x000080000
LADSPA_CLASS_MULTIEQ          = 0x000100000
LADSPA_CLASS_AMPLITUDE        = 0x000200000
LADSPA_CLASS_PITCH            = 0x000400000
LADSPA_CLASS_AMPLIFIER        = 0x000800000
LADSPA_CLASS_WAVESHAPER       = 0x001000000
LADSPA_CLASS_MODULATOR        = 0x002000000
LADSPA_CLASS_DISTORTION       = 0x004000000
LADSPA_CLASS_DYNAMICS         = 0x008000000
LADSPA_CLASS_COMPRESSOR       = 0x010000000
LADSPA_CLASS_EXPANDER         = 0x020000000
LADSPA_CLASS_LIMITER          = 0x040000000
LADSPA_CLASS_GATE             = 0x080000000
LADSPA_CLASS_SPECTRAL         = 0x100000000
LADSPA_CLASS_NOTCH            = 0x200000000

LADSPA_GROUP_DYNAMICS         = LADSPA_CLASS_DYNAMICS|LADSPA_CLASS_COMPRESSOR|LADSPA_CLASS_EXPANDER|LADSPA_CLASS_LIMITER|LADSPA_CLASS_GATE
LADSPA_GROUP_AMPLITUDE        = LADSPA_CLASS_AMPLITUDE|LADSPA_CLASS_AMPLIFIER|LADSPA_CLASS_WAVESHAPER|LADSPA_CLASS_MODULATOR|LADSPA_CLASS_DISTORTION|LADSPA_GROUP_DYNAMICS
LADSPA_GROUP_EQ               = LADSPA_CLASS_EQ|LADSPA_CLASS_PARAEQ|LADSPA_CLASS_MULTIEQ
LADSPA_GROUP_FILTER           = LADSPA_CLASS_FILTER|LADSPA_CLASS_LOWPASS|LADSPA_CLASS_HIGHPASS|LADSPA_CLASS_BANDPASS|LADSPA_CLASS_COMB|LADSPA_CLASS_ALLPASS|LADSPA_CLASS_NOTCH
LADSPA_GROUP_FREQUENCY        = LADSPA_CLASS_FREQUENCY|LADSPA_CLASS_FREQUENCY_METER|LADSPA_GROUP_FILTER|LADSPA_GROUP_EQ|LADSPA_CLASS_PITCH
LADSPA_GROUP_SIMULATOR        = LADSPA_CLASS_SIMULATOR|LADSPA_CLASS_REVERB
LADSPA_GROUP_TIME             = LADSPA_CLASS_TIME|LADSPA_CLASS_DELAY|LADSPA_CLASS_PHASER|LADSPA_CLASS_FLANGER|LADSPA_CLASS_CHORUS|LADSPA_CLASS_REVERB
LADSPA_GROUP_GENERATOR        = LADSPA_CLASS_GENERATOR|LADSPA_CLASS_OSCILLATOR

# A Scale Point
class LADSPA_RDF_ScalePoint(Structure):
  _fields_ = [
    ("Value", LADSPA_Data),
    ("Label", c_char_p)
  ]

# A Port
class LADSPA_RDF_Port(Structure):
  _fields_ = [
    ("Type", LADSPA_Property),
    ("Hints", LADSPA_Property),
    ("Label", c_char_p),
    ("Default", LADSPA_Data),
    ("Unit", LADSPA_Property),

    ("ScalePointCount", c_ulong),
    ("ScalePoints", POINTER(LADSPA_RDF_ScalePoint))
  ]

# The actual plugin descriptor
class LADSPA_RDF_Descriptor(Structure):
  _fields_ = [
    ("Type", LADSPA_PluginType),
    ("UniqueID", c_ulong),
    ("Title", c_char_p),
    ("Creator", c_char_p),

    ("PortCount", c_ulong),
    ("Ports", POINTER(LADSPA_RDF_Port))
  ]

# -------------------------------------------------------------------------------
#  Python compatible C types

PyLADSPA_RDF_ScalePoint = {
  'Value': 0.0,
  'Label': ""
}

PyLADSPA_RDF_Port = {
  'Type': 0x0,
  'Hints': 0x0,
  'Label': "",
  'Default': 0.0,
  'Unit': 0x0,

  'ScalePointCount': 0,
  'ScalePoints': [],

  # Only here to help, NOT in the API:
  'index': 0
}

PyLADSPA_RDF_Descriptor = {
  'Type': 0x0,
  'UniqueID': 0,
  'Title': "",
  'Creator': "",

  'PortCount': 0,
  'Ports': []
}

# -------------------------------------------------------------------------------
#  RDF data and conversions

# Namespaces
NS_dc   = "http://purl.org/dc/elements/1.1/"
NS_rdf  = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
NS_rdfs = "http://www.w3.org/2000/01/rdf-schema#"

NS_ladspa = "http://ladspa.org/ontology#"

# Prefixes (sorted alphabetically and by type)
rdf_prefix = {
  # Base types
  'dc:creator': NS_dc+"creator",
  'dc:rights':  NS_dc+"rights",
  'dc:title':   NS_dc+"title",
  'rdf:value':  NS_rdf+"value",
  'rdf:type':   NS_rdf+"type",

  # LADSPA Stuff
  'ladspa:forPort':      NS_ladspa+"forPort",
  'ladspa:hasLabel':     NS_ladspa+"hasLabel",
  'ladspa:hasPoint':     NS_ladspa+"hasPoint",
  'ladspa:hasPort':      NS_ladspa+"hasPort",
  'ladspa:hasPortValue': NS_ladspa+"hasPortValue",
  'ladspa:hasScale':     NS_ladspa+"hasScale",
  'ladspa:hasSetting':   NS_ladspa+"hasSetting",
  'ladspa:hasUnit':      NS_ladspa+"hasUnit",

  # LADSPA Extensions
  'ladspa:NotchPlugin':    NS_ladspa+"NotchPlugin",
  'ladspa:SpectralPlugin': NS_ladspa+"SpectralPlugin"
}

def get_c_plugin_class(value):
  value_str = value.replace(NS_ladspa,"")

  if (value_str == "Plugin"):
    return 0
  elif (value_str == "UtilityPlugin"):
    return LADSPA_CLASS_UTILITY
  elif (value_str == "GeneratorPlugin"):
    return LADSPA_CLASS_GENERATOR
  elif (value_str == "SimulatorPlugin"):
    return LADSPA_CLASS_SIMULATOR
  elif (value_str == "OscillatorPlugin"):
    return LADSPA_CLASS_OSCILLATOR
  elif (value_str == "TimePlugin"):
    return LADSPA_CLASS_TIME
  elif (value_str == "DelayPlugin"):
    return LADSPA_CLASS_DELAY
  elif (value_str == "PhaserPlugin"):
    return LADSPA_CLASS_PHASER
  elif (value_str == "FlangerPlugin"):
    return LADSPA_CLASS_FLANGER
  elif (value_str == "ChorusPlugin"):
    return LADSPA_CLASS_CHORUS
  elif (value_str == "ReverbPlugin"):
    return LADSPA_CLASS_REVERB
  elif (value_str == "FrequencyPlugin"):
    return LADSPA_CLASS_FREQUENCY
  elif (value_str == "FrequencyMeterPlugin"):
    return LADSPA_CLASS_FREQUENCY_METER
  elif (value_str == "FilterPlugin"):
    return LADSPA_CLASS_FILTER
  elif (value_str == "LowpassPlugin"):
    return LADSPA_CLASS_LOWPASS
  elif (value_str == "HighpassPlugin"):
    return LADSPA_CLASS_HIGHPASS
  elif (value_str == "BandpassPlugin"):
    return LADSPA_CLASS_BANDPASS
  elif (value_str == "CombPlugin"):
    return LADSPA_CLASS_COMB
  elif (value_str == "AllpassPlugin"):
    return LADSPA_CLASS_ALLPASS
  elif (value_str == "EQPlugin"):
    return LADSPA_CLASS_EQ
  elif (value_str == "ParaEQPlugin"):
    return LADSPA_CLASS_PARAEQ
  elif (value_str == "MultiEQPlugin"):
    return LADSPA_CLASS_MULTIEQ
  elif (value_str == "AmplitudePlugin"):
    return LADSPA_CLASS_AMPLITUDE
  elif (value_str == "PitchPlugin"):
    return LADSPA_CLASS_PITCH
  elif (value_str == "AmplifierPlugin"):
    return LADSPA_CLASS_AMPLIFIER
  elif (value_str == "WaveshaperPlugin"):
    return LADSPA_CLASS_WAVESHAPER
  elif (value_str == "ModulatorPlugin"):
    return LADSPA_CLASS_MODULATOR
  elif (value_str == "DistortionPlugin"):
    return LADSPA_CLASS_DISTORTION
  elif (value_str == "DynamicsPlugin"):
    return LADSPA_CLASS_DYNAMICS
  elif (value_str == "CompressorPlugin"):
    return LADSPA_CLASS_COMPRESSOR
  elif (value_str == "ExpanderPlugin"):
    return LADSPA_CLASS_EXPANDER
  elif (value_str == "LimiterPlugin"):
    return LADSPA_CLASS_LIMITER
  elif (value_str == "GatePlugin"):
    return LADSPA_CLASS_GATE
  elif (value_str == "SpectralPlugin"):
    return LADSPA_CLASS_SPECTRAL
  elif (value_str == "NotchPlugin"):
    return LADSPA_CLASS_NOTCH
  elif (value_str == "MixerPlugin"):
    return LADSPA_CLASS_EQ
  else:
    print("LADSPA_RDF - Got an unknown plugin type '%s'" % (value_str))
    return 0

def get_c_port_type(value):
  value_str = value.replace(NS_ladspa,"")

  if (value_str == "Port"):
    return 0
  elif (value_str == "ControlPort"):
    return LADSPA_PORT_CONTROL
  elif (value_str == "AudioPort"):
    return LADSPA_PORT_AUDIO
  elif (value_str == "InputPort"):
    return LADSPA_PORT_INPUT
  elif (value_str == "OutputPort"):
    return LADSPA_PORT_OUTPUT
  elif (value_str in ("ControlInputPort", "InputControlPort")):
    return LADSPA_PORT_CONTROL|LADSPA_PORT_INPUT
  elif (value_str in ("ControlOutputPort", "OutputControlPort")):
    return LADSPA_PORT_CONTROL|LADSPA_PORT_OUTPUT
  elif (value_str in ("AudioInputPort", "InputAudioPort")):
    return LADSPA_PORT_AUDIO|LADSPA_PORT_INPUT
  elif (value_str in ("AudioOutputPort", "OutputAudioPort")):
    return LADSPA_PORT_AUDIO|LADSPA_PORT_OUTPUT
  else:
    print("LADSPA_RDF - Got an unknown port type '%s'" % (value_str))
    return 0

def get_c_unit_type(value):
  value_str = value.replace(NS_ladspa,"")

  if (value_str in ("Unit", "Units", "AmplitudeUnits", "FrequencyUnits", "TimeUnits")):
    return 0
  elif (value_str == "dB"):
    return LADSPA_UNIT_DB
  elif (value_str == "coef"):
    return LADSPA_UNIT_COEF
  elif (value_str == "Hz"):
    return LADSPA_UNIT_HZ
  elif (value_str == "seconds"):
    return LADSPA_UNIT_S
  elif (value_str == "milliseconds"):
    return LADSPA_UNIT_MS
  elif (value_str == "minutes"):
    return LADSPA_UNIT_MIN
  else:
    print("LADSPA_RDF - Got an unknown unit type '%s'" % (value_str))
    return 0

# -------------------------------------------------------------------------------
#  Global objects

global LADSPA_RDF_PATH, LADSPA_Plugins
LADSPA_RDF_PATH = ("/usr/share/ladspa/rdf", "/usr/local/share/ladspa/rdf")
LADSPA_Plugins  = []

# Set LADSPA_RDF_PATH variable
def set_rdf_path(PATH):
  global LADSPA_RDF_PATH
  LADSPA_RDF_PATH = PATH

# -------------------------------------------------------------------------------
#  Helper methods

LADSPA_RDF_TYPE_PLUGIN = 1
LADSPA_RDF_TYPE_PORT   = 2

# Check RDF Type
def rdf_is_type(_subject, compare):
  if (type(_subject) == URIRef and NS_ladspa in _subject):
    if (compare == LADSPA_RDF_TYPE_PLUGIN):
      return bool(to_plugin_number(_subject).isdigit())
    elif (compare == LADSPA_RDF_TYPE_PORT):
      return bool("." in to_plugin_number(_subject))
  else:
    return False

def to_float(rdf_item):
  return float(str(rdf_item).replace("f",""))

# Convert RDF LADSPA subject into a number
def to_plugin_number(_subject):
  return str(_subject).replace(NS_ladspa,"")

# Convert RDF LADSPA subject into a plugin and port number
def to_plugin_and_port_number(_subject):
  numbers = str(_subject).replace(NS_ladspa,"").split(".")
  return (numbers[0], numbers[1])

# Convert RDF LADSPA subject into a port number
def to_plugin_port(_subject):
  return to_plugin_and_port_number(_subject)[1]

# -------------------------------------------------------------------------------
#  RDF store/retrieve data methods

def check_and_add_plugin(plugin_id):
  global LADSPA_Plugins
  for i in range(len(LADSPA_Plugins)):
    if (LADSPA_Plugins[i]['UniqueID'] == plugin_id):
      return i
  else:
    plugin = deepcopy(PyLADSPA_RDF_Descriptor)
    plugin['UniqueID'] = plugin_id
    LADSPA_Plugins.append(plugin)
    return len(LADSPA_Plugins)-1

def set_plugin_value(plugin_id, key, value):
  global LADSPA_Plugins
  index = check_and_add_plugin(plugin_id)
  LADSPA_Plugins[index][key] = value

def add_plugin_value(plugin_id, key, value):
  global LADSPA_Plugins
  index = check_and_add_plugin(plugin_id)
  LADSPA_Plugins[index][key] += value

def or_plugin_value(plugin_id, key, value):
  global LADSPA_Plugins
  index = check_and_add_plugin(plugin_id)
  LADSPA_Plugins[index][key] |= value

def append_plugin_value(plugin_id, key, value):
  global LADSPA_Plugins
  index = check_and_add_plugin(plugin_id)
  LADSPA_Plugins[index][key].append(value)

def check_and_add_port(plugin_id, port_id):
  global LADSPA_Plugins
  index = check_and_add_plugin(plugin_id)
  Ports = LADSPA_Plugins[index]['Ports']
  for i in range(len(Ports)):
    if (Ports[i]['index'] == port_id):
      return (index, i)
  else:
    pcount = LADSPA_Plugins[index]['PortCount']
    port = deepcopy(PyLADSPA_RDF_Port)
    port['index'] = port_id
    Ports.append(port)
    LADSPA_Plugins[index]['PortCount'] += 1
    return (index, pcount)

def set_port_value(plugin_id, port_id, key, value):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  LADSPA_Plugins[i]['Ports'][j][key] = value

def add_port_value(plugin_id, port_id, key, value):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  LADSPA_Plugins[i]['Ports'][j][key] += value

def or_port_value(plugin_id, port_id, key, value):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  LADSPA_Plugins[i]['Ports'][j][key] |= value

def append_port_value(plugin_id, port_id, key, value):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  LADSPA_Plugins[i]['Ports'][j][key].append(value)

def add_scalepoint(plugin_id, port_id, value, label):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  Port = LADSPA_Plugins[i]['Ports'][j]
  scalepoint = deepcopy(PyLADSPA_RDF_ScalePoint)
  scalepoint['Value'] = value
  scalepoint['Label'] = label
  Port['ScalePoints'].append(scalepoint)
  Port['ScalePointCount'] += 1

def set_port_default(plugin_id, port_id, value):
  global LADSPA_Plugins
  i, j = check_and_add_port(plugin_id, port_id)
  Port = LADSPA_Plugins[i]['Ports'][j]
  Port['Default'] = value
  Port['Hints'] |= LADSPA_PORT_DEFAULT

def get_node_objects(value_nodes, n_subject):
  ret_nodes = []
  for _subject, _predicate, _object in value_nodes:
    if (_subject == n_subject):
      ret_nodes.append((_predicate, _object))
  return ret_nodes

def append_and_sort(value, vlist):
  if (len(vlist) == 0):
    vlist.append(value)
  elif (value < vlist[0]):
    vlist.insert(0, value)
  elif (value > vlist[len(vlist)-1]):
    vlist.append(value)
  else:
    for i in range(len(vlist)):
      if (value < vlist[i]):
        vlist.insert(i, value)
        break
    else:
      print("LADSPA_RDF - CRITICAL ERROR #001")

  return vlist

def get_value_index(value, vlist):
  for i in range(len(vlist)):
    if (vlist[i] == value):
      return i
  else:
    print("LADSPA_RDF - CRITICAL ERROR #002")
    return 0

# -------------------------------------------------------------------------------
#  RDF sort data methods

# Sort the plugin's port's ScalePoints by value
def SORT_PyLADSPA_RDF_ScalePoints(old_dict_list):
  new_dict_list = []
  indexes_list = []

  for i in range(len(old_dict_list)):
    new_dict_list.append(deepcopy(PyLADSPA_RDF_ScalePoint))
    append_and_sort(old_dict_list[i]['Value'], indexes_list)

  for i in range(len(old_dict_list)):
    index = get_value_index(old_dict_list[i]['Value'], indexes_list)
    new_dict_list[index]['Value'] = old_dict_list[i]['Value']
    new_dict_list[index]['Label'] = old_dict_list[i]['Label']

  return new_dict_list

# Sort the plugin's port by index
def SORT_PyLADSPA_RDF_Ports(old_dict_list):
  new_dict_list = []
  max_index = -1

  for i in range(len(old_dict_list)):
    if (old_dict_list[i]['index'] > max_index):
      max_index = old_dict_list[i]['index']

  for i in range(max_index+1):
    new_dict_list.append(deepcopy(PyLADSPA_RDF_Port))

  for i in range(len(old_dict_list)):
    index = old_dict_list[i]['index']
    new_dict_list[index]['index']   = old_dict_list[i]['index']
    new_dict_list[index]['Type']    = old_dict_list[i]['Type']
    new_dict_list[index]['Hints']   = old_dict_list[i]['Hints']
    new_dict_list[index]['Unit']    = old_dict_list[i]['Unit']
    new_dict_list[index]['Default'] = old_dict_list[i]['Default']
    new_dict_list[index]['Label']   = old_dict_list[i]['Label']
    new_dict_list[index]['ScalePointCount'] = old_dict_list[i]['ScalePointCount']
    new_dict_list[index]['ScalePoints'] = SORT_PyLADSPA_RDF_ScalePoints(old_dict_list[i]['ScalePoints'])

  return new_dict_list

# -------------------------------------------------------------------------------
#  RDF data parsing

from rdflib import ConjunctiveGraph, URIRef, Literal, BNode

# Fully parse rdf file
def parse_rdf_file(filename):
  primer = ConjunctiveGraph()

  try:
    primer.parse(filename, format='xml')
    rdf_list = [(x, y, z) for x, y, z in primer]
  except:
    rdf_list = []

  # For BNodes
  index_nodes = [] # Subject [index], Predicate, Plugin, Port
  value_nodes = [] # Subject [index], Predicate, Object

  # Parse RDF list
  for _subject, _predicate, _object in rdf_list:

    # Fix broken or old plugins
    if (_predicate == URIRef("http://ladspa.org/ontology#hasUnits")):
      _predicate = URIRef(rdf_prefix['ladspa:hasUnit'])

    # Plugin information
    if (rdf_is_type(_subject, LADSPA_RDF_TYPE_PLUGIN)):
      plugin_id = int(to_plugin_number(_subject))

      if (_predicate == URIRef(rdf_prefix['dc:creator'])):
        set_plugin_value(plugin_id, 'Creator', str(_object))

      elif (_predicate == URIRef(rdf_prefix['dc:rights'])):
        # No useful information here
        pass

      elif (_predicate == URIRef(rdf_prefix['dc:title'])):
        set_plugin_value(plugin_id, 'Title', str(_object))

      elif (_predicate == URIRef(rdf_prefix['rdf:type'])):
        c_class = get_c_plugin_class(str(_object))
        or_plugin_value(plugin_id, 'Type', c_class)

      elif (_predicate == URIRef(rdf_prefix['ladspa:hasPort'])):
        # No useful information here
        pass

      elif (_predicate == URIRef(rdf_prefix['ladspa:hasSetting'])):
        index_nodes.append((_object, _predicate, plugin_id, None))

      else:
        print("LADSPA_RDF - Plugin predicate '%s' not handled" % (_predicate))

    # Port information
    elif (rdf_is_type(_subject, LADSPA_RDF_TYPE_PORT)):
      plugin_port = to_plugin_and_port_number(_subject)
      plugin_id = int(plugin_port[0])
      port_id   = int(plugin_port[1])

      if (_predicate == URIRef(rdf_prefix['rdf:type'])):
        c_class = get_c_port_type(str(_object))
        or_port_value(plugin_id, port_id, 'Type', c_class)

      elif (_predicate == URIRef(rdf_prefix['ladspa:hasLabel'])):
        set_port_value(plugin_id, port_id, 'Label', str(_object))
        or_port_value(plugin_id, port_id, 'Hints', LADSPA_PORT_LABEL)

      elif (_predicate == URIRef(rdf_prefix['ladspa:hasScale'])):
        index_nodes.append((_object, _predicate, plugin_id, port_id))

      elif (_predicate == URIRef(rdf_prefix['ladspa:hasUnit'])):
        c_unit = get_c_unit_type(str(_object))
        set_port_value(plugin_id, port_id, 'Unit', c_unit)
        or_port_value(plugin_id, port_id, 'Hints', LADSPA_PORT_UNIT)

      else:
        print("LADSPA_RDF - Port predicate '%s' not handled" % (_predicate))

    # These "extensions" are already implemented
    elif (_subject in (URIRef(rdf_prefix['ladspa:NotchPlugin']), URIRef(rdf_prefix['ladspa:SpectralPlugin']))):
      pass

    elif (type(_subject) == BNode):
      value_nodes.append((_subject, _predicate, _object))

    else:
      print("LADSPA_RDF - Unknown subject type '%s'" % (_subject))

  # Parse BNodes, indexes
  bnodes_data_dump = []

  for n_subject, n_predicate, plugin_id, port_id in index_nodes:
    n_objects = get_node_objects(value_nodes, n_subject)

    for subn_predicate, subn_subject in n_objects:
      subn_objects = get_node_objects(value_nodes, subn_subject)

      for real_predicate, real_object in subn_objects:
        if (n_predicate == URIRef(rdf_prefix['ladspa:hasScale']) and subn_predicate == URIRef(rdf_prefix['ladspa:hasPoint'])):
          bnodes_data_dump.append(("scalepoint", subn_subject, plugin_id, port_id, real_predicate, real_object))
        elif (n_predicate == URIRef(rdf_prefix['ladspa:hasSetting']) and subn_predicate == URIRef(rdf_prefix['ladspa:hasPortValue'])):
          bnodes_data_dump.append(("port_default", subn_subject, plugin_id, port_id, real_predicate, real_object))
        else:
          print("LADSPA_RDF - Unknown BNode combo - '%s' + '%s'" % (n_predicate, subn_predicate))

  # Process BNodes, values
  scalepoints   = [] # subject, plugin, port, value, label
  port_defaults = [] # subject, plugin, port, def-value

  for n_type, n_subject, n_plugin, n_port, n_predicate, n_object in bnodes_data_dump:
    if (n_type == "scalepoint"):
      for i in range(len(scalepoints)):
        if (scalepoints[i][0] == n_subject):
          index = i
          break
      else:
        scalepoints.append([n_subject, n_plugin, n_port, None, None])
        index = len(scalepoints)-1

      if (n_predicate == URIRef(rdf_prefix['rdf:value'])):
        scalepoints[index][3] = to_float(n_object)
      elif (n_predicate == URIRef(rdf_prefix['ladspa:hasLabel'])):
        scalepoints[index][4] = str(n_object)

    elif (n_type == "port_default"):
      for i in range(len(port_defaults)):
        if (port_defaults[i][0] == n_subject):
          index = i
          break
      else:
        port_defaults.append([n_subject, n_plugin, None, None])
        index = len(port_defaults)-1

      if (n_predicate == URIRef(rdf_prefix['ladspa:forPort'])):
        port_defaults[index][2] = int(to_plugin_port(n_object))
      elif (n_predicate == URIRef(rdf_prefix['rdf:value'])):
        port_defaults[index][3] = to_float(n_object)

  # Now add the last information
  for scalepoint in scalepoints:
    index, plugin_id, port_id, value, label = scalepoint
    add_scalepoint(plugin_id, port_id, value, label)

  for port_default in port_defaults:
    index, plugin_id, port_id, value = port_default
    set_port_default(plugin_id, port_id, value)

# -------------------------------------------------------------------------------
#  LADSPA_RDF main methods

import os

# Main function - check all rdfs for information about ladspa plugins
def recheck_all_plugins(qobject, start_value, percent_value, m_value):
  global LADSPA_RDF_PATH, LADSPA_Plugins

  LADSPA_Plugins = []
  rdf_files      = []
  rdf_extensions = (".rdf", ".rdF", ".rDF", ".RDF", ".RDf", "Rdf")

  # Get all RDF files
  for PATH in LADSPA_RDF_PATH:
    for root, dirs, files in os.walk(PATH):
      for filename in [filename for filename in files if filename.endswith(rdf_extensions)]:
        rdf_files.append(os.path.join(root, filename))

  # Parse all RDF files
  for i in range(len(rdf_files)):
    rdf_file = rdf_files[i]

    # Tell GUI we're parsing this bundle
    if (qobject):
      percent = (float(i) / len(rdf_files) ) * percent_value
      qobject.pluginLook(start_value + (percent * m_value), rdf_file)

    # Parse RDF
    parse_rdf_file(rdf_file)

  return LADSPA_Plugins

# Convert PyLADSPA_Plugins into ctype structs
def get_c_ladspa_rdfs(PyPluginList):
  C_LADSPA_Plugins = []
  c_unicode_error_str = "(unicode error)".encode("ascii")

  for plugin in PyPluginList:
    # Sort the ports by index
    ladspa_ports = SORT_PyLADSPA_RDF_Ports(plugin['Ports'])

    # Initial data
    desc = LADSPA_RDF_Descriptor()
    desc.Type      = plugin['Type']
    desc.UniqueID  = plugin['UniqueID']

    try:
      desc.Title   = plugin['Title'].encode("ascii")
    except:
      desc.Title   = c_unicode_error_str

    try:
      desc.Creator = plugin['Creator'].encode("ascii")
    except:
      desc.Creator = c_unicode_error_str

    desc.PortCount = plugin['PortCount']

    # Ports
    _PortType  = LADSPA_RDF_Port*desc.PortCount
    desc.Ports = _PortType()

    for i in range(desc.PortCount):
      port    = LADSPA_RDF_Port()
      py_port = ladspa_ports[i]

      port.Type    = py_port['Type']
      port.Hints   = py_port['Hints']

      try:
        port.Label = py_port['Label'].encode("ascii")
      except:
        port.Label = c_unicode_error_str

      port.Default = py_port['Default']
      port.Unit    = py_port['Unit']

      # ScalePoints
      port.ScalePointCount = py_port['ScalePointCount']

      _ScalePointType  = LADSPA_RDF_ScalePoint*port.ScalePointCount
      port.ScalePoints = _ScalePointType()

      for j in range(port.ScalePointCount):
        scalepoint    = LADSPA_RDF_ScalePoint()
        py_scalepoint = py_port['ScalePoints'][j]

        try:
          scalepoint.Label = py_scalepoint['Label'].encode("ascii")
        except:
          scalepoint.Label = c_unicode_error_str

        scalepoint.Value   = py_scalepoint['Value']

        port.ScalePoints[j] = scalepoint

      desc.Ports[i] = port

    C_LADSPA_Plugins.append(desc)

  return C_LADSPA_Plugins

# -------------------------------------------------------------------------------
#  Implementation test

#if __name__ == '__main__':
    #set_rdf_path(["/home/falktx/Personal/FOSS/GIT/Cadence/lrdf/"])
    #plugins = recheck_all_plugins(None, 0)
    #for plugin in LADSPA_Plugins:
      #print("----------------------")
      #print("Type:     0x%X" % (plugin["Type"]))
      #print("UniqueID: %i" % (plugin["UniqueID"]))
      #print("Title:    %s" % (plugin["Title"]))
      #print("Creator:  %s" % (plugin["Creator"]))
      #print("Ports:    (%i)" % (plugin["PortCount"]))
      #for i in range(plugin["PortCount"]):
        #port = plugin["Ports"][i]
        #print(" --> Port #%i" % (i+1))
        #print("  Type:        0x%X" % (port["Type"]))
        #print("  Hints:       0x%X" % (port["Hints"]))
        #print("  Label:       %s" % (port["Label"]))
        #print("  Default:     %f" % (port["Default"]))
        #print("  Unit:        0x%i" % (port["Unit"]))
        #print("  ScalePoints: (%i)" % (port["ScalePointCount"]))
        #for j in range(port["ScalePointCount"]):
          #scalepoint = port["ScalePoints"][j]
          #print("   --> ScalePoint #%i" % (j+1))
          #print("    Value: %f" % (scalepoint["Value"]))
          #print("    Label: %s" % (scalepoint["Label"]))
    #print(len(LADSPA_Plugins))
    #get_c_ladspa_rdfs(LADSPA_Plugins)

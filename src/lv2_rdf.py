#!/usr/bin/env python
# -*- coding: utf-8 -*-

# -------------------------------------------------------------------------------
#  LV2 URIs

LV2_ATOM_URI                   = "http://lv2plug.in/ns/ext/atom"
LV2_CV_PORT_URI                = "http://lv2plug.in/ns/ext/cv-port"
LV2_DATA_ACCESS_URI            = "http://lv2plug.in/ns/ext/data-access"
LV2_DYN_MANIFEST_URI           = "http://lv2plug.in/ns/ext/dynmanifest"
LV2_EVENT_URI                  = "http://lv2plug.in/ns/ext/event"
LV2_HOST_INFO_URI              = "http://lv2plug.in/ns/ext/host-info"
LV2_INSTANCE_ACCESS_URI        = "http://lv2plug.in/ns/ext/instance-access"
LV2_LOG_URI                    = "http://lv2plug.in/ns/ext/log"
LV2_MIDI_URI                   = "http://lv2plug.in/ns/ext/midi"
LV2_PATCH_URI                  = "http://lv2plug.in/ns/ext/patch"
LV2_PORT_GROUPS_URI            = "http://lv2plug.in/ns/ext/port-groups"
LV2_PORT_PROPS_URI             = "http://lv2plug.in/ns/ext/port-props"
LV2_PRESETS_URI                = "http://lv2plug.in/ns/ext/presets"
LV2_PUI_URI                    = "http://lv2plug.in/ns/ext/pui"
LV2_REFERENCE_URI              = "http://lv2plug.in/ns/ext/reference"
LV2_RESIZE_PORT_URI            = "http://lv2plug.in/ns/ext/resize-port"
LV2_STATE_URI                  = "http://lv2plug.in/ns/ext/state"
LV2_TIME_URI                   = "http://lv2plug.in/ns/ext/time"
LV2_UI_RESIZE_URI              = "http://lv2plug.in/ns/ext/ui-resize"
LV2_URI_MAP_URI                = "http://lv2plug.in/ns/ext/uri-map"
LV2_URID_URI                   = "http://lv2plug.in/ns/ext/urid"
LV2_WORKER_URI                 = "http://lv2plug.in/ns/ext/worker"

LV2_UI_URI                     = "http://lv2plug.in/ns/extensions/ui"
LV2_UNITS_URI                  = "http://lv2plug.in/ns/extensions/units"

LV2_MIDI_MAP_URI               = "http://ll-plugins.nongnu.org/lv2/ext/midimap"

LV2_DYNPARAM_URI               = "http://home.gna.org/lv2dynparam/v1"
LV2_RTSAFE_MEMORY_POOL_URI     = "http://home.gna.org/lv2dynparam/rtmempool/v1"

LV2_EXTERNAL_UI_URI            = "http://nedko.arnaudov.name/lv2/external_ui/"

LV2_RDF_Supported_Features_URI = (
  LV2_ATOM_URI,
  LV2_CV_PORT_URI,
  LV2_DATA_ACCESS_URI,
  LV2_EVENT_URI,
  LV2_HOST_INFO_URI,
  LV2_INSTANCE_ACCESS_URI,
  LV2_MIDI_URI,
  LV2_PORT_PROPS_URI,
  LV2_PRESETS_URI,
  LV2_STATE_URI,
  LV2_TIME_URI,
  LV2_UI_RESIZE_URI,
  LV2_URI_MAP_URI,
  LV2_URID_URI,
  LV2_UI_URI,
  LV2_UNITS_URI,
  LV2_MIDI_MAP_URI,
  LV2_RTSAFE_MEMORY_POOL_URI,
  LV2_EXTERNAL_UI_URI
)

# -------------------------------------------------------------------------------
#  C types

# Imports (Global)
from ctypes import *
from copy import deepcopy

# Base Types
LV2_Data = c_float
LV2_URI  = c_char_p
LV2_Property   = c_uint32
LV2_PluginType = c_ulonglong

# Port MIDI Map Types
LV2_PORT_MIDI_MAP_CC           = 0x1
LV2_PORT_MIDI_MAP_NRPN         = 0x2

# A Port Midi Map
class LV2_RDF_PortMidiMap(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("Number", c_uint32)
  ]

# Port Point Hints
LV2_PORT_POINT_DEFAULT         = 0x1
LV2_PORT_POINT_MINIMUM         = 0x2
LV2_PORT_POINT_MAXIMUM         = 0x4

# Port Points
class LV2_RDF_PortPoints(Structure):
  _fields_ = [
    ("Hints", LV2_Property),
    ("Default", LV2_Data),
    ("Minimum", LV2_Data),
    ("Maximum", LV2_Data)
  ]

# Port Unit Types
LV2_UNIT_BAR                   = 0x01
LV2_UNIT_BEAT                  = 0x02
LV2_UNIT_BPM                   = 0x03
LV2_UNIT_CENT                  = 0x04
LV2_UNIT_CM                    = 0x05
LV2_UNIT_COEF                  = 0x06
LV2_UNIT_DB                    = 0x07
LV2_UNIT_DEGREE                = 0x08
LV2_UNIT_HZ                    = 0x09
LV2_UNIT_INCH                  = 0x0A
LV2_UNIT_KHZ                   = 0x0B
LV2_UNIT_KM                    = 0x0C
LV2_UNIT_M                     = 0x0D
LV2_UNIT_MHZ                   = 0x0E
LV2_UNIT_MIDINOTE              = 0x0F
LV2_UNIT_MILE                  = 0x10
LV2_UNIT_MIN                   = 0x11
LV2_UNIT_MM                    = 0x12
LV2_UNIT_MS                    = 0x13
LV2_UNIT_OCT                   = 0x14
LV2_UNIT_PC                    = 0x15
LV2_UNIT_S                     = 0x16
LV2_UNIT_SEMITONE              = 0x17

# Port Unit Hints
LV2_PORT_UNIT                  = 0x1
LV2_PORT_UNIT_NAME             = 0x2
LV2_PORT_UNIT_RENDER           = 0x4
LV2_PORT_UNIT_SYMBOL           = 0x8

# A Port Unit
class LV2_RDF_PortUnit(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("Hints", LV2_Property),
    ("Name", c_char_p),
    ("Render", c_char_p),
    ("Symbol", c_char_p)
  ]

# A Port Scale Point
class LV2_RDF_PortScalePoint(Structure):
  _fields_ = [
    ("Label", c_char_p),
    ("Value", LV2_Data)
  ]

# Port Types
LV2_PORT_INPUT                 = 0x01
LV2_PORT_OUTPUT                = 0x02
LV2_PORT_CONTROL               = 0x04
LV2_PORT_AUDIO                 = 0x08
LV2_PORT_ATOM_MESSAGE          = 0x10
LV2_PORT_ATOM_VALUE            = 0x20
LV2_PORT_CV                    = 0x40
LV2_PORT_EVENT                 = 0x80
LV2_PORT_MIDI_LL               = 0x100

# TODO - Port Atom types

# Port Event types
LV2_PORT_EVENT_MIDI            = 0x1000
LV2_PORT_EVENT_TIME            = 0x2000

# Port Properties
LV2_PORT_OPTIONAL              = 0x00001
LV2_PORT_LATENCY               = 0x00002
LV2_PORT_TOGGLED               = 0x00004
LV2_PORT_SAMPLE_RATE           = 0x00008
LV2_PORT_INTEGER               = 0x00010
LV2_PORT_ENUMERATION           = 0x00020
LV2_PORT_CAUSES_ARTIFACTS      = 0x00040
LV2_PORT_CONTINUOUS_CV         = 0x00080
LV2_PORT_DISCRETE_CV           = 0x00100
LV2_PORT_EXPENSIVE             = 0x00200
LV2_PORT_HAS_STRICT_BOUNDS     = 0x00400
LV2_PORT_LOGARITHMIC           = 0x00800
LV2_PORT_NOT_AUTOMATIC         = 0x01000
LV2_PORT_NOT_ON_GUI            = 0x02000
LV2_PORT_REPORTS_BEATS_PER_BAR = 0x04000
LV2_PORT_REPORTS_BEAT_UNIT     = 0x08000
LV2_PORT_REPORTS_BPM           = 0x10000
LV2_PORT_TRIGGER               = 0x20000

# A Port
class LV2_RDF_Port(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("Properties", LV2_Property),
    ("Name", c_char_p),
    ("Symbol", c_char_p),

    ("MidiMap", LV2_RDF_PortMidiMap),
    ("Points", LV2_RDF_PortPoints),
    ("Unit", LV2_RDF_PortUnit),

    ("ScalePointCount", c_uint32),
    ("ScalePoints", POINTER(LV2_RDF_PortScalePoint))
  ]

# A Preset Port
class LV2_RDF_PresetPort(Structure):
  _fields_ = [
    ("Symbol", c_char_p),
    ("Value", LV2_Data)
  ]

# Preset State Types
LV2_PRESET_STATE_NULL          = 0x0
LV2_PRESET_STATE_BOOL          = 0x1
LV2_PRESET_STATE_INT           = 0x2
LV2_PRESET_STATE_LONG          = 0x3
LV2_PRESET_STATE_STRING        = 0x4
LV2_PRESET_STATE_BINARY        = 0x5

# A Preset State Value
class LV2_RDF_PresetStateValue(Union):
  _fields_ = [
    ("b", c_bool),
    ("i", c_int),
    ("li", c_long),
    ("s", c_char_p)
  ]

# A Preset State
class LV2_RDF_PresetState(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("Key", c_char_p),
    ("Value", LV2_RDF_PresetStateValue)
  ]

# A Preset
class LV2_RDF_Preset(Structure):
  _fields_ = [
    ("URI", LV2_URI),
    ("Label", c_char_p),

    ("PortCount", c_uint32),
    ("Ports", POINTER(LV2_RDF_PresetPort)),

    ("StateCount", c_uint32),
    ("States", POINTER(LV2_RDF_PresetState))
  ]

# Feature Types
LV2_FEATURE_OPTIONAL           = 0x1
LV2_FEATURE_REQUIRED           = 0x2

# A Feature
class LV2_RDF_Feature(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("URI", LV2_URI)
  ]

# UI Types
LV2_UI_X11                     = 0x1
LV2_UI_GTK2                    = 0x2
LV2_UI_QT4                     = 0x3
LV2_UI_EXTERNAL                = 0x4
LV2_UI_OLD_EXTERNAL            = 0x5

# An UI
class LV2_RDF_UI(Structure):
  _fields_ = [
    ("Type", LV2_Property),
    ("URI", LV2_URI),
    ("Binary", c_char_p),
    ("Bundle", c_char_p),

    ("FeatureCount", c_uint32),
    ("Features", POINTER(LV2_RDF_Feature)),

    ("ExtensionCount", c_uint32),
    ("Extensions", POINTER(LV2_URI))
  ]

# Plugin Types
LV2_CLASS_GENERATOR            = 0x000000001
LV2_CLASS_INSTRUMENT           = 0x000000002
LV2_CLASS_OSCILLATOR           = 0x000000004
LV2_CLASS_UTILITY              = 0x000000008
LV2_CLASS_CONVERTER            = 0x000000010
LV2_CLASS_ANALYSER             = 0x000000020
LV2_CLASS_MIXER                = 0x000000040
LV2_CLASS_SIMULATOR            = 0x000000080
LV2_CLASS_DELAY                = 0x000000100
LV2_CLASS_MODULATOR            = 0x000000200
LV2_CLASS_REVERB               = 0x000000400
LV2_CLASS_PHASER               = 0x000000800
LV2_CLASS_FLANGER              = 0x000001000
LV2_CLASS_CHORUS               = 0x000002000
LV2_CLASS_FILTER               = 0x000004000
LV2_CLASS_LOWPASS              = 0x000008000
LV2_CLASS_BANDPASS             = 0x000010000
LV2_CLASS_HIGHPASS             = 0x000020000
LV2_CLASS_COMB                 = 0x000040000
LV2_CLASS_ALLPASS              = 0x000080000
LV2_CLASS_EQUALISER            = 0x000100000
LV2_CLASS_PARAMETRIC           = 0x000200000
LV2_CLASS_MULTIBAND            = 0x000400000
LV2_CLASS_SPACIAL              = 0x000800000
LV2_CLASS_SPECTRAL             = 0x001000000
LV2_CLASS_PITCH_SHIFTER        = 0x002000000
LV2_CLASS_AMPLIFIER            = 0x004000000
LV2_CLASS_DISTORTION           = 0x008000000
LV2_CLASS_WAVESHAPER           = 0x010000000
LV2_CLASS_DYNAMICS             = 0x020000000
LV2_CLASS_COMPRESSOR           = 0x040000000
LV2_CLASS_EXPANDER             = 0x080000000
LV2_CLASS_LIMITER              = 0x100000000
LV2_CLASS_GATE                 = 0x200000000
LV2_CLASS_FUNCTION             = 0x400000000
LV2_CLASS_CONSTANT             = 0x800000000

LV2_GROUP_GENERATOR            = LV2_CLASS_GENERATOR|LV2_CLASS_INSTRUMENT|LV2_CLASS_OSCILLATOR
LV2_GROUP_UTILITY              = LV2_CLASS_UTILITY|LV2_CLASS_CONVERTER|LV2_CLASS_ANALYSER|LV2_CLASS_MIXER|LV2_CLASS_FUNCTION|LV2_CLASS_CONSTANT
LV2_GROUP_SIMULATOR            = LV2_CLASS_SIMULATOR|LV2_CLASS_REVERB
LV2_GROUP_DELAY                = LV2_CLASS_DELAY|LV2_CLASS_REVERB
LV2_GROUP_MODULATOR            = LV2_CLASS_MODULATOR|LV2_CLASS_PHASER|LV2_CLASS_FLANGER|LV2_CLASS_CHORUS
LV2_GROUP_FILTER               = LV2_CLASS_FILTER|LV2_CLASS_LOWPASS|LV2_CLASS_BANDPASS|LV2_CLASS_HIGHPASS|LV2_CLASS_COMB|LV2_CLASS_ALLPASS|LV2_CLASS_EQUALISER|LV2_CLASS_PARAMETRIC|LV2_CLASS_MULTIBAND
LV2_GROUP_EQULISER             = LV2_CLASS_EQUALISER|LV2_CLASS_PARAMETRIC|LV2_CLASS_MULTIBAND
LV2_GROUP_SPECTRAL             = LV2_CLASS_SPECTRAL|LV2_CLASS_PITCH_SHIFTER
LV2_GROUP_DISTORTION           = LV2_CLASS_DISTORTION|LV2_CLASS_WAVESHAPER
LV2_GROUP_DYNAMICS             = LV2_CLASS_DYNAMICS|LV2_CLASS_COMPRESSOR|LV2_CLASS_EXPANDER|LV2_CLASS_LIMITER|LV2_CLASS_GATE

# The actual plugin descriptor
class LV2_RDF_Descriptor(Structure):
  _fields_ = [
    ("Type", LV2_PluginType),
    ("URI", LV2_URI),
    ("Name", c_char_p),
    ("Author", c_char_p),
    ("License", c_char_p),
    ("Binary", c_char_p),
    ("Bundle", c_char_p),
    ("UniqueID", c_ulong),

    ("PortCount", c_uint32),
    ("Ports", POINTER(LV2_RDF_Port)),

    ("PresetCount", c_uint32),
    ("Presets", POINTER(LV2_RDF_Preset)),

    ("FeatureCount", c_uint32),
    ("Features", POINTER(LV2_RDF_Feature)),

    ("ExtensionCount", c_uint32),
    ("Extensions", POINTER(c_char_p)),

    ("UICount", c_uint32),
    ("UIs", POINTER(LV2_RDF_UI))
  ]

# -------------------------------------------------------------------------------
#  Python compatible C types

PyLV2_RDF_PortMidiMap = {
  'Type':   0x0,
  'Number': 0
}

PyLV2_RDF_PortPoints = {
  'Hints':   0x0,
  'Default': 0.0,
  'Minimum': 0.0,
  'Maximum': 0.0
}

PyLV2_RDF_PortUnit = {
  'Type':   0x0,
  'Hints':  0x0,
  'Name':   "",
  'Render': "",
  'Symbol': ""
}

PyLV2_RDF_PortScalePoint = {
  'Label': "",
  'Value': 0.0
}

PyLV2_RDF_Port = {
  'Type':       0x0,
  'Properties': 0x0,
  'Name':       "",
  'Symbol':     "",
  'MidiMap':    deepcopy(PyLV2_RDF_PortMidiMap),
  'Points':     deepcopy(PyLV2_RDF_PortPoints),
  'Unit':       deepcopy(PyLV2_RDF_PortUnit),
  'ScalePointCount': 0,
  'ScalePoints':     [],

  # Extra Data (Not in API)
  'index':      -1
}

PyLV2_RDF_PresetPort = {
  'Symbol': "",
  'Value':  0.0
}

PyLV2_RDF_PresetState = {
  'Type':   0,
  'Key':    "",
  'Value':  None
}

PyLV2_RDF_Preset = {
  'URI':         "",
  'Label':       "",
  'PortCount':   0,
  'Ports':       [],
  'StateCount':  0,
  'States':      [],

  # Extra Data (Not in API)
  'appliesTo': []
}

PyLV2_RDF_Feature = {
  'Type': 0x0,
  'URI':  ""
}

PyLV2_RDF_UI = {
  'Type':           0x0,
  'URI':            "",
  'Binary':         "",
  'Bundle':         "",
  'FeatureCount':   0,
  'Features':       [],
  'ExtensionCount': 0,
  'Extensions':     [],

  # Extra Data (Not in API)
  'appliesTo':      []
}

PyLV2_RDF_Descriptor = {
  'Type':           0x0,
  'URI':            "",
  'Name':           "",
  'Author':         "",
  'License':        "",
  'Binary':         "",
  'Bundle':         "",
  'UniqueID':       0,
  'PortCount':      0,
  'Ports':          [],
  'PresetCount':    0,
  'Presets':        [],
  'FeatureCount':   0,
  'Features':       [],
  'ExtensionCount': 0,
  'Extensions':     [],
  'UICount':        0,
  'UIs':            []
}

# -------------------------------------------------------------------------------
#  RDF data and conversions

# Namespaces
NS_dc   = "http://purl.org/dc/terms/"
NS_doap = "http://usefulinc.com/ns/doap#"
NS_foaf = "http://xmlns.com/foaf/0.1/"
NS_rdf  = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
NS_rdfs = "http://www.w3.org/2000/01/rdf-schema#"

NS_lv2       = "http://lv2plug.in/ns/lv2core#"
NS_lv2atom   = "http://lv2plug.in/ns/ext/atom#"
NS_lv2ctx    = "http://lv2plug.in/ns/ext/contexts#"
NS_lv2cv     = "http://lv2plug.in/ns/ext/cv-port#"
NS_lv2dyn    = "http://lv2plug.in/ns/ext/dynmanifest#"
NS_lv2ev     = "http://lv2plug.in/ns/ext/event#"
NS_lv2hi     = "http://lv2plug.in/ns/ext/host-info#"
NS_lv2pg     = "http://lv2plug.in/ns/ext/port-groups#"
NS_lv2pprops = "http://lv2plug.in/ns/ext/port-props#"
NS_lv2pset   = "http://lv2plug.in/ns/ext/presets#"
NS_lv2state  = "http://lv2plug.in/ns/ext/state#"
NS_lv2ui     = "http://lv2plug.in/ns/extensions/ui#"
NS_lv2units  = "http://lv2plug.in/ns/extensions/units#"
#NS_llext     = "http://ll-plugins.nongnu.org/lv2/ext/"
NS_lv2mm     = "http://ll-plugins.nongnu.org/lv2/ext/midimap#"
NS_llplug    = "http://ll-plugins.nongnu.org/lv2/namespace#"

# Prefixes (sorted alphabetically and by type)
rdf_prefix = {
  # Base types
  'dc:replaces':      NS_dc+"replaces",
  'doap:creator':     NS_doap+"creator",
  'doap:description': NS_doap+"description",
  'doap:developer':   NS_doap+"developer",
  'doap:homepage':    NS_doap+"homepage",
  'doap:license':     NS_doap+"license",
  'doap:maintainer':  NS_doap+"maintainer",
  'doap:name':        NS_doap+"name",
  #'foaf:homepage':   "http://xmlns.com/foaf/0.1/homepage",
  #'foaf:mbox':       "http://xmlns.com/foaf/0.1/mbox",
  'foaf:name':        NS_foaf+"name",
  'rdf:type':         NS_rdf+"type",
  'rdf:value':        NS_rdf+"value",
  'rdfs:Class':       NS_rdfs+"Class",
  'rdfs:comment':     NS_rdfs+"comment",
  'rdfs:label':       NS_rdfs+"label",
  'rdfs:seeAlso':     NS_rdfs+"seeAlso",
  'rdfs:subClassOf':  NS_rdfs+"subClassOf",

  # LV2 Stuff
  'lv2:appliesTo':           NS_lv2+"appliesTo",
  'lv2:binary':              NS_lv2+"binary",
  'lv2:default':             NS_lv2+"default",
  'lv2:documentation':       NS_lv2+"documentation",
  'lv2:extensionData':       NS_lv2+"extensionData",
  'lv2:index':               NS_lv2+"index",
  'lv2:maximum':             NS_lv2+"maximum",
  'lv2:minimum':             NS_lv2+"minimum",
  'lv2:name':                NS_lv2+"name",
  'lv2:port':                NS_lv2+"port",
  'lv2:portProperty':        NS_lv2+"portProperty",
  'lv2:property':            NS_lv2+"property",
  'lv2:scalePoint':          NS_lv2+"scalePoint",
  'lv2:symbol':              NS_lv2+"symbol",

  'lv2:optionalFeature':     NS_lv2+"optionalFeature",
  'lv2:requiredFeature':     NS_lv2+"requiredFeature",

  # LV2 Atom
  'lv2atom:String':          NS_lv2atom+"String",

  # LV2 CV
  'lv2cv:CVPort':            NS_lv2cv+"CVPort",

  # LV2 Event
  'lv2ev:EventPort':         NS_lv2ev+"EventPort",
  'lv2ev:supportsEvent':     NS_lv2ev+"supportsEvent",

  # LV2 Presets
  'lv2pset:Preset':          NS_lv2pset+"Preset",
  'lv2pset:value':           NS_lv2pset+"value",

  # LV2 State
  'lv2state:state':          NS_lv2state+"state",

  # LV2 UI
  'lv2ui:ui':                NS_lv2ui+"ui",
  'lv2ui:binary':            NS_lv2ui+"binary",
  'lv2ui:events':            NS_lv2ui+"events",
  'lv2ui:portIndex':         NS_lv2ui+"portIndex",
  'lv2ui:portNotification':  NS_lv2ui+"portNotification",
  'lv2ui:residentSONames':   NS_lv2ui+"residentSONames",

  'lv2ui:optionalFeature':   NS_lv2ui+"optionalFeature",
  'lv2ui:requiredFeature':   NS_lv2ui+"requiredFeature",

  # LV2 Units
  'lv2units:unit':           NS_lv2units+"unit",
  'lv2units:name':           NS_lv2units+"name",
  'lv2units:render':         NS_lv2units+"render",
  'lv2units:symbol':         NS_lv2units+"symbol",

  # LV2 Midi Map
  'lv2mm:defaultMidiController': NS_lv2mm+"defaultMidiController",
  'lv2mm:controllerType':        NS_lv2mm+"controllerType",
  'lv2mm:controllerNumber':      NS_lv2mm+"controllerNumber",

  # ll-plugins
  'llplug:MathConstantPlugin':   NS_llplug+"MathConstantPlugin",
  'llplug:MathFunctionPlugin':   NS_llplug+"MathFunctionPlugin",
  'llplug:pegName':              NS_llplug+"pegName",
  'llplug:svgIcon':              NS_llplug+"svgIcon"
}

def get_c_plugin_class(value):
  value_str = value.replace(NS_lv2, "", 1)

  if (value_str == "Plugin"):
    return 0
  elif (value_str == "GeneratorPlugin"):
    return LV2_CLASS_GENERATOR
  elif (value_str == "InstrumentPlugin"):
    return LV2_CLASS_INSTRUMENT
  elif (value_str == "OscillatorPlugin"):
    return LV2_CLASS_OSCILLATOR
  elif (value_str == "UtilityPlugin"):
    return LV2_CLASS_UTILITY
  elif (value_str == "ConverterPlugin"):
    return LV2_CLASS_CONVERTER
  elif (value_str == "AnalyserPlugin"):
    return LV2_CLASS_ANALYSER
  elif (value_str == "MixerPlugin"):
    return LV2_CLASS_MIXER
  elif (value_str == "SimulatorPlugin"):
    return LV2_CLASS_SIMULATOR
  elif (value_str == "DelayPlugin"):
    return LV2_CLASS_DELAY
  elif (value_str == "ModulatorPlugin"):
    return LV2_CLASS_MODULATOR
  elif (value_str == "ReverbPlugin"):
    return LV2_CLASS_REVERB
  elif (value_str == "PhaserPlugin"):
    return LV2_CLASS_PHASER
  elif (value_str == "FlangerPlugin"):
    return LV2_CLASS_FLANGER
  elif (value_str == "ChorusPlugin"):
    return LV2_CLASS_CHORUS
  elif (value_str == "FilterPlugin"):
    return LV2_CLASS_FILTER
  elif (value_str == "LowpassPlugin"):
    return LV2_CLASS_LOWPASS
  elif (value_str == "BandpassPlugin"):
    return LV2_CLASS_BANDPASS
  elif (value_str == "HighpassPlugin"):
    return LV2_CLASS_HIGHPASS
  elif (value_str == "CombPlugin"):
    return LV2_CLASS_COMB
  elif (value_str == "AllpassPlugin"):
    return LV2_CLASS_ALLPASS
  elif (value_str == "EQPlugin"):
    return LV2_CLASS_EQUALISER
  elif (value_str == "ParaEQPlugin"):
    return LV2_CLASS_PARAMETRIC
  elif (value_str == "MultiEQPlugin"):
    return LV2_CLASS_MULTIBAND
  elif (value_str == "SpatialPlugin"):
    return LV2_CLASS_SPACIAL
  elif (value_str == "SpectralPlugin"):
    return LV2_CLASS_SPECTRAL
  elif (value_str == "PitchPlugin"):
    return LV2_CLASS_PITCH_SHIFTER
  elif (value_str == "AmplifierPlugin"):
    return LV2_CLASS_AMPLIFIER
  elif (value_str == "DistortionPlugin"):
    return LV2_CLASS_DISTORTION
  elif (value_str == "WaveshaperPlugin"):
    return LV2_CLASS_WAVESHAPER
  elif (value_str == "DynamicsPlugin"):
    return LV2_CLASS_DYNAMICS
  elif (value_str == "CompressorPlugin"):
    return LV2_CLASS_COMPRESSOR
  elif (value_str == "ExpanderPlugin"):
    return LV2_CLASS_EXPANDER
  elif (value_str == "LimiterPlugin"):
    return LV2_CLASS_LIMITER
  elif (value_str == "GatePlugin"):
    return LV2_CLASS_GATE
  elif (value_str == "FunctionPlugin"):
    return LV2_CLASS_FUNCTION
  elif (value_str == "ConstantPlugin"):
    return LV2_CLASS_CONSTANT
  elif (value_str == "ADVENV"):
    return LV2_CLASS_UTILITY
  elif (value_str == "AMP"):
    return LV2_CLASS_AMPLIFIER
  elif (value_str == "ANALOGDRIVER"):
    return LV2_CLASS_DYNAMICS
  elif (value_str == "CVS"):
    return 0 # FIXME?
  elif (value_str == "DELAY"):
    return LV2_CLASS_DELAY
  elif (value_str == "DYNAMICWAVES"):
    return LV2_CLASS_DYNAMICS
  elif (value_str == "ENV"):
    return LV2_CLASS_UTILITY
  elif (value_str == "HZTOVC"):
    return LV2_CLASS_CONVERTER
  elif (value_str == "LFO"):
    return LV2_CLASS_OSCILLATOR
  elif (value_str == "Noise2"):
    return LV2_CLASS_GENERATOR
  elif (value_str == "MIXER"):
    return LV2_CLASS_MIXER
  elif (value_str == "SLEW"):
    return LV2_CLASS_UTILITY
  elif (value_str == "VCO2"):
    return LV2_CLASS_OSCILLATOR
  elif (value_str == "VCF"):
    return LV2_CLASS_UTILITY
  elif (value_str == "VCAEXP"):
    return LV2_CLASS_UTILITY
  elif (value_str == "VCALIN"):
    return LV2_CLASS_UTILITY
  elif (value_str == "VCTOHZ"):
    return LV2_CLASS_CONVERTER
  else:
    print("LV2_RDF - Got an unknown plugin type '%s'" % value_str)
    return 0

def get_c_port_type(value):
  value_str = value.replace(NS_lv2, "", 1)

  if (value_str == "Port"):
    return 0
  elif (value_str == "InputPort"):
    return LV2_PORT_INPUT
  elif (value_str == "OutputPort"):
    return LV2_PORT_OUTPUT
  elif (value_str == "ControlPort"):
    return LV2_PORT_CONTROL
  elif (value_str == "AudioPort"):
    return LV2_PORT_AUDIO
  elif (value == rdf_prefix['lv2cv:CVPort']):
    return LV2_PORT_CV
  elif (value == rdf_prefix['lv2ev:EventPort']):
    return LV2_PORT_EVENT
  else:
    print("LV2_RDF - Got an unknown port type '%s'" % value_str)
    return 0

def get_c_port_event_type(value):
  if (value == "http://lv2plug.in/ns/ext/midi#MidiEvent"):
    return LV2_PORT_EVENT_MIDI
  elif (value == "http://lv2plug.in/ns/ext/time#Position"):
    return LV2_PORT_EVENT_TIME
  else:
    print("LV2_RDF - Got an unknown port event type '%s'" % value)
    return 0

def get_c_port_midi_map_type(value):
  value_str = value.replace(NS_llplug, "", 1)

  if (value_str == "CC"):
    return LV2_PORT_MIDI_MAP_CC
  elif (value_str == "NRPN"):
    return LV2_PORT_MIDI_MAP_NRPN
  else:
    print("LV2_RDF - Got an unknown midi map type '%s'" % value_str)
    return 0

def get_c_port_property(value):
  # Fix old plugins
  if (value.startswith("http://lv2plug.in/ns/dev/extportinfo#")):
    value = value.replace("http://lv2plug.in/ns/dev/extportinfo#", NS_lv2, 1)

  value_str = value.replace(NS_lv2, "", 1).replace(NS_lv2pprops, "", 1)

  if (value_str == "connectionOptional"):
    return LV2_PORT_OPTIONAL
  elif (value_str == "reportsLatency"):
    return LV2_PORT_LATENCY
  elif (value_str == "toggled"):
    return LV2_PORT_TOGGLED
  elif (value_str == "sampleRate"):
    return LV2_PORT_SAMPLE_RATE
  elif (value_str == "integer"):
    return LV2_PORT_INTEGER
  elif (value_str == "enumeration"):
    return LV2_PORT_ENUMERATION
  elif (value_str == "causesArtifacts"):
    return LV2_PORT_CAUSES_ARTIFACTS
  elif (value_str == "continuousCV"):
    return LV2_PORT_CONTINUOUS_CV
  elif (value_str == "discreteCV"):
    return LV2_PORT_DISCRETE_CV
  elif (value_str == "expensive"):
    return LV2_PORT_EXPENSIVE
  elif (value_str == "hasStrictBounds"):
    return LV2_PORT_HAS_STRICT_BOUNDS
  elif (value_str == "logarithmic"):
    return LV2_PORT_LOGARITHMIC
  elif (value_str == "notAutomatic"):
    return LV2_PORT_NOT_AUTOMATIC
  elif (value_str == "notOnGUI"):
    return LV2_PORT_NOT_ON_GUI
  elif (value_str == "reportsBeatsPerBar"):
    return LV2_PORT_REPORTS_BEATS_PER_BAR
  elif (value_str == "reportsBeatUnit"):
    return LV2_PORT_REPORTS_BEAT_UNIT
  elif (value_str == "reportsBpm"):
    return LV2_PORT_REPORTS_BPM
  elif (value_str == "trigger"):
    return LV2_PORT_TRIGGER
  elif (value_str == "outputGain"):
    return 0
  else:
    print("LV2_RDF - Got an unknown port property '%s'" % value_str)
    return 0

def get_c_port_unit_type(value):
  value_str = value.replace(NS_lv2units, "", 1)

  if (value_str == "bar"):
    return LV2_UNIT_BAR
  elif (value_str == "beat"):
    return LV2_UNIT_BEAT
  elif (value_str == "bpm"):
    return LV2_UNIT_BPM
  elif (value_str == "cent"):
    return LV2_UNIT_CENT
  elif (value_str == "cm"):
    return LV2_UNIT_CM
  elif (value_str == "coef"):
    return LV2_UNIT_COEF
  elif (value_str == "db"):
    return LV2_UNIT_DB
  elif (value_str == "degree"):
    return LV2_UNIT_DEGREE
  elif (value_str == "hz"):
    return LV2_UNIT_HZ
  elif (value_str == "inch"):
    return LV2_UNIT_INCH
  elif (value_str == "khz"):
    return LV2_UNIT_KHZ
  elif (value_str == "km"):
    return LV2_UNIT_KM
  elif (value_str == "m"):
    return LV2_UNIT_M
  elif (value_str == "mhz"):
    return LV2_UNIT_MHZ
  elif (value_str == "midiNote"):
    return LV2_UNIT_MIDINOTE
  elif (value_str == "mile"):
    return LV2_UNIT_MILE
  elif (value_str == "min"):
    return LV2_UNIT_MIN
  elif (value_str == "mm"):
    return LV2_UNIT_MM
  elif (value_str == "ms"):
    return LV2_UNIT_MS
  elif (value_str == "oct"):
    return LV2_UNIT_OCT
  elif (value_str == "pc"):
    return LV2_UNIT_PC
  elif (value_str == "s"):
    return LV2_UNIT_S
  elif (value_str == "semitone12TET"):
    return LV2_UNIT_SEMITONE
  else:
    print("LV2_RDF - Got an unknown Unit type '%s'" % value_str)
    return 0

def get_c_ui_type(value):
  value_str = value.replace(NS_lv2ui, "", 1)

  if (value_str == "X11UI"):
    return LV2_UI_X11
  elif (value_str == "GtkUI"):
    return LV2_UI_GTK2
  elif (value_str == "Qt4UI"):
    return LV2_UI_QT4
  elif (value_str == "external"):
    return LV2_UI_OLD_EXTERNAL
  elif (value == LV2_EXTERNAL_UI_URI):
    return LV2_UI_EXTERNAL
  else:
    print("LV2_RDF - Got an unknown UI type '%s'" % value_str)
    return 0

# -------------------------------------------------------------------------------
#  Global objects

import os

global LV2_PATH, LV2_Plugins, LV2_Presets, LV2_UIs, seeAlso_list
LV2_Plugins  = []
LV2_Presets  = []
LV2_UIs      = []
seeAlso_list = []

CWD = os.path.realpath(".")
LV2_PATH = ["/usr/lib/lv2", "/usr/local/lib/lv2"]

# Set LV2_PATH variable
def set_rdf_path(PATH):
  global LV2_PATH
  LV2_PATH = PATH

# -------------------------------------------------------------------------------
#  Helper methods

def to_string(string):
  try:
    return str(unicode(string).encode('utf-8'))
  except:
    return "(unicode error)"

def to_local_name(uri):
  # TODO - Windows support?
  return uri.replace("file://", "")

def to_short_name(uri, bundle_path):
  if (bundle_path.endswith((".ttl", ".ttL", ".tTL", ".TTL", ".TTl", ".Ttl")) and os.sep in bundle_path):
    bundle_path = bundle_path.rsplit(os.sep, 1)[0]
  if (bundle_path.endswith((".lv2", ".lV2", ".LV2", ".Lv2")) and os.sep in bundle_path):
    bundle_path = bundle_path.rsplit(os.sep, 1)[0]
  return to_local_name(uri).replace(CWD, bundle_path)

def get_real_bundle_path(bundle_path):
  if (bundle_path[-1] == os.sep):
    # Remove trailing separator, for endswith comparison
    real_bundle_path = bundle_path[0:-1]
  else:
    real_bundle_path = bundle_path

  if (real_bundle_path.endswith((".lv2", ".lV2", ".LV2", ".Lv2"))):
    return real_bundle_path+os.sep
  elif (real_bundle_path.endswith((".ttl", ".ttL", ".tTL", ".TTL", ".TTl", ".Ttl"))):
    return real_bundle_path.rsplit(os.sep, 1)[0]+os.sep
  else:
    return real_bundle_path+os.sep

def is_number(value):
  string = to_string(value)
  if (string != "" and string != "(unicode error)"):
    if (string.startswith("-")):
      string = string.replace("-", "", 1)
    sstring = string.split(".")
    if (len(sstring) == 1 and sstring[0].isdigit()):
      return True
    elif (len(sstring) == 2 and sstring[0].isdigit() and sstring[1].isdigit()):
      return True
    else:
      return False
  else:
    return False

def append_and_sort(vlist, value):
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

  return vlist

# -------------------------------------------------------------------------------
#  RDF store/retrieve data methods

# Nodes
iNodeURI   = 0
iNodeProp  = 1
iNodeValue = 2

def get_node_property(uri, nodes, prop, default):
  for node in nodes:
    if (node[iNodeURI] == uri and node[iNodeProp] == prop):
      return node[iNodeValue]
  else:
    return default

def get_node_properties(uri, nodes, prop):
  props = []
  for node in nodes:
    if (node[iNodeURI] == uri and node[iNodeProp] == prop):
      props.append(node[iNodeValue])
  return props

def get_all_node_properties(uri, nodes):
  props = []
  for node in nodes:
    if (node[iNodeURI] == uri):
      props.append((node[iNodeProp], node[iNodeValue]))
  return props

def append_seeAlso_value(seeAlso):
  global seeAlso_list
  if (not seeAlso in seeAlso_list):
    seeAlso_list.append(seeAlso)

# LV2_Plugins
def is_plugin(uri):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      return True
  return False

def maybe_add_plugin_uri(uri):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      break
  else:
    plugin_dict = deepcopy(PyLV2_RDF_Descriptor)
    plugin_dict['URI'] = uri
    LV2_Plugins.append(plugin_dict)

def set_plugin_value(uri, key, value):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      plugin[key] = value
      break

def add_plugin_value(uri, key, value):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      plugin[key] += value
      break

def or_plugin_value(uri, key, value):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      plugin[key] |= value
      break

def append_plugin_value(uri, key, value):
  global LV2_Plugins
  for plugin in LV2_Plugins:
    if (plugin['URI'] == uri):
      plugin[key].append(value)
      break

# LV2_Presets
def is_preset(uri):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      return True
  return False

def maybe_add_preset_uri(uri):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      break
  else:
    preset_dict = deepcopy(PyLV2_RDF_Preset)
    preset_dict['URI'] = uri
    LV2_Presets.append(preset_dict)

def set_preset_value(uri, key, value):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      preset[key] = value
      break

def add_preset_value(uri, key, value):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      preset[key] += value
      break

def or_preset_value(uri, key, value):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      preset[key] |= value
      break

def append_preset_value(uri, key, value):
  global LV2_Presets
  for preset in LV2_Presets:
    if (preset['URI'] == uri):
      preset[key].append(value)
      break

# LV2_UIs
def is_ui(uri):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      return True
  return False

def maybe_add_ui_uri(uri):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      break
  else:
    ui_dict = deepcopy(PyLV2_RDF_UI)
    ui_dict['URI'] = uri
    LV2_UIs.append(ui_dict)

def set_ui_value(uri, key, value):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      ui[key] = value
      break

def add_ui_value(uri, key, value):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      ui[key] += value
      break

def or_ui_value(uri, key, value):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      ui[key] |= value
      break

def append_ui_value(uri, key, value):
  global LV2_UIs
  for ui in LV2_UIs:
    if (ui['URI'] == uri):
      ui[key].append(value)
      break

# -------------------------------------------------------------------------------
#  RDF sort data methods

def SORT_PyLV2_RDF_Ports(old_dict_list):
  new_dict_list = []

  for i in old_dict_list:
    new_dict_list.append(deepcopy(PyLV2_RDF_Port))

  for old_dict in old_dict_list:
    index = old_dict['index']
    new_dict_list[index]['index']           = index
    new_dict_list[index]['Type']            = old_dict['Type']
    new_dict_list[index]['Properties']      = old_dict['Properties']
    new_dict_list[index]['Name']            = old_dict['Name']
    new_dict_list[index]['Symbol']          = old_dict['Symbol']
    new_dict_list[index]['MidiMap']         = old_dict['MidiMap']
    new_dict_list[index]['Points']          = old_dict['Points']
    new_dict_list[index]['Unit']            = old_dict['Unit']
    new_dict_list[index]['ScalePointCount'] = old_dict['ScalePointCount']
    new_dict_list[index]['ScalePoints']     = old_dict['ScalePoints']

  return new_dict_list

def SORT_PyLV2_RDF_PortScalePoints(old_dict_list):
  new_dict_list = []
  indexes_list  = []

  for old_dict in old_dict_list:
    new_dict_list.append(deepcopy(PyLV2_RDF_PortScalePoint))
    append_and_sort(indexes_list, old_dict['Value'])

  for old_dict in old_dict_list:
    index = indexes_list.index(old_dict['Value'])
    new_dict_list[index]['Value'] = old_dict['Value']
    new_dict_list[index]['Label'] = old_dict['Label']

  return new_dict_list

# -------------------------------------------------------------------------------
#  RDF data parsing

from rdflib import ConjunctiveGraph, URIRef, Literal, BNode

# Fill the actual information
def fill_information(parse, bundle_path):

  # Initial parse to get all Plugin, Preset and UI URIs on this bundle
  for i in range(len(parse)):
    uri, prop, value = parse[i]

    if (type(prop) == URIRef and type(uri) == URIRef and type(value) in (URIRef, Literal)):
      uri   = to_string(uri)
      prop  = to_string(prop)
      value = to_string(value)

      if (prop == rdf_prefix['rdf:type']):
        if (value.startswith(NS_lv2)):
          maybe_add_plugin_uri(uri)
        elif (value == rdf_prefix['lv2pset:Preset']):
          maybe_add_preset_uri(uri)
        elif (value.startswith(NS_lv2ui) or value == LV2_EXTERNAL_UI_URI):
          maybe_add_ui_uri(uri)

  # Save for later use
  nodes_info = [] # just indexes
  nodes_list = [] # the actual data

  # Real parse, for each parameter
  for i in range(len(parse)):
    uri, prop, value = parse[i]

    if (type(prop) == URIRef):
      prop = to_string(prop)

      if (type(uri) == URIRef):
        if (type(value) in (URIRef, Literal)):
          uri   = to_string(uri)
          value = to_string(value)

          # Fix broken or old plugins
          if (value.startswith("http://lv2plug.in/ns/dev/")):
            value = value.replace("http://lv2plug.in/ns/dev/", "http://lv2plug.in/ns/ext/", 1)
          if (prop.startswith("http://lv2plug.in/ns/dev/")):
            prop = prop.replace("http://lv2plug.in/ns/dev/", "http://lv2plug.in/ns/ext/", 1)

          if (prop == "http://lv2plug.in/ns/ext/presets#hasPreset"):
            prop = rdf_prefix['rdfs:seeAlso']
          elif (prop == "http://dublincore.org/documents/dcmi-namespace/replaces"):
            prop = rdf_prefix['dc:replaces']
          elif (prop in (NS_lv2+"property", NS_lv2+"pluginProperty")):
            prop = rdf_prefix['lv2:optionalFeature']

          # Skip specification
          if (uri == "http://lv2plug.in/ns/lv2core" or uri.startswith("http://lv2plug.in/ns/ext") or value == rdf_prefix['rdfs:Class'] or prop == rdf_prefix['rdfs:subClassOf']):
            continue

          # Type --start
          if (prop == rdf_prefix['rdf:type']):

            # Plugin
            if (value.startswith(NS_lv2)):
              c_class = get_c_plugin_class(value)
              or_plugin_value(uri, 'Type', c_class)

            # DynManifest, ignored
            elif (value.startswith(NS_lv2dyn)):
              pass

            # Host Info, ignored
            elif (value.startswith(NS_lv2hi)):
              pass

            # Port Groups, ignored
            elif (value.startswith(NS_lv2pg) or value.startswith("http://ll-plugins.nongnu.org/lv2/ext/portgroups#")):
              pass

            # Preset
            elif (value.startswith(NS_lv2pset)):
              # Already handled before
              pass

            # UI
            elif (value.startswith(NS_lv2ui) or value == LV2_EXTERNAL_UI_URI):
              c_ui_type = get_c_ui_type(value)
              set_ui_value(uri, 'Type', c_ui_type)

            # Special Types
            elif (value == rdf_prefix['llplug:MathConstantPlugin']):
              or_plugin_value(uri, 'Type', LV2_CLASS_CONSTANT)

            elif (value == rdf_prefix['llplug:MathFunctionPlugin']):
              or_plugin_value(uri, 'Type', LV2_CLASS_FUNCTION)

            elif (value == "http://foltman.com/ns/BooleanPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_FUNCTION)

            elif (value == "http://foltman.com/ns/IntegerPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_FUNCTION)

            elif (value == "http://foltman.com/ns/MathOperatorPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_FUNCTION)

            elif (value == "http://foltman.com/ns/MIDIPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_GENERATOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspChEQMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspChEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspChEQBMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspChEQBStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspChorusPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_CHORUS)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspDistPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_DISTORTION)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspGrEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMkiiGraEQMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMkiiGraEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMBCStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMBCBStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMxEQMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMxEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMxDYNMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_DYNAMICS)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspMxDYNStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_DYNAMICS)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspPEQMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspPEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspPhaserPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_PHASER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProDynMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_DYNAMICS)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProDynStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_DYNAMICS)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProEQMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProEQStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_EQUALISER)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProGateMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_GATE)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspProGateStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_GATE)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspSRStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_REVERB)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspSRBStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_REVERB)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspVCMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspVCStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspVCBMonoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspVCBStereoPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_COMPRESSOR)

            elif (value == "http://linuxdsp.co.uk/lv2/ns/linuxdspWahPlugin"):
              or_plugin_value(uri, 'Type', LV2_CLASS_SIMULATOR)

            else:
              print("LV2_RDF - URI Type '%s' not handled (uri: %s)" % (value, uri))

          # Type --end

          # Preset start
          elif (prop.startswith(NS_lv2pset)):
            print("LV2_RDF - Preset URI Type '%s' not handled (uri: %s)" % (prop, uri))

          # UI ----start
          elif (prop.startswith(NS_lv2ui)):

            if (prop == rdf_prefix['lv2ui:ui']):
              append_ui_value(value, 'appliesTo', uri)

            elif (prop == rdf_prefix['lv2ui:binary']):
              ui_binary = os.path.join(bundle_path, to_short_name(value, bundle_path))
              set_ui_value(uri, 'Binary', ui_binary)

              ui_bundle_path = get_real_bundle_path(ui_binary.rsplit(os.sep, 1)[0])
              set_ui_value(uri, 'Bundle', ui_bundle_path)

            elif (prop == rdf_prefix['lv2ui:events']):
              pass

            elif (prop == rdf_prefix['lv2ui:portIndex']):
              pass

            elif (prop == rdf_prefix['lv2ui:portNotification']):
              pass

            elif (prop == rdf_prefix['lv2ui:residentSONames']):
              pass

            elif (prop == rdf_prefix['lv2ui:optionalFeature']):
              py_feature = deepcopy(PyLV2_RDF_Feature)
              py_feature['Type'] = LV2_FEATURE_OPTIONAL
              py_feature['URI']  = value

              append_ui_value(uri, 'Features', py_feature)
              add_ui_value(uri, 'FeatureCount', 1)

            elif (prop == rdf_prefix['lv2ui:requiredFeature']):
              py_feature = deepcopy(PyLV2_RDF_Feature)
              py_feature['Type'] = LV2_FEATURE_REQUIRED
              py_feature['URI']  = value

              append_ui_value(uri, 'Features', py_feature)
              add_ui_value(uri, 'FeatureCount', 1)

            else:
              print("LV2_RDF - UI URI Type '%s' not handled (uri: %s)" % (prop, uri))

          # UI ----end

          # DynManifest, ignored
          elif (value.startswith(NS_lv2dyn)):
            pass

          # Contexts, ignored
          elif (value.startswith(NS_lv2ctx)):
            pass

          # Host Info, ignored
          elif (value.startswith(NS_lv2hi)):
            pass

          # Port Groups, ignored
          elif (prop.startswith(NS_lv2pg)):
            pass

          elif (prop == rdf_prefix['lv2:appliesTo']):
            if (is_preset(uri)):
              append_preset_value(uri, 'appliesTo', value)

          elif (prop == rdf_prefix['lv2:binary']):
            binary = os.path.join(bundle_path, to_short_name(value, bundle_path))
            set_plugin_value(uri, 'Binary', binary)

            real_bundle_path = get_real_bundle_path(bundle_path)
            set_plugin_value(uri, 'Bundle', real_bundle_path)

          elif (prop == rdf_prefix['lv2:documentation']):
            pass

          elif (prop == rdf_prefix['lv2:extensionData']):
            if (is_plugin(uri)):
              append_plugin_value(uri, 'Extensions', value)
              add_plugin_value(uri, 'ExtensionCount', 1)
            elif (is_ui(uri)):
              append_ui_value(uri, 'Extensions', value)
              add_ui_value(uri, 'ExtensionCount', 1)
            else:
              print("LV2_RDF - Invalid extensionData, not for Plugin or UI")

          elif (prop == rdf_prefix['lv2:symbol']):
            pass

          elif (prop == rdf_prefix['lv2:optionalFeature']):
            py_feature = deepcopy(PyLV2_RDF_Feature)
            py_feature['Type'] = LV2_FEATURE_OPTIONAL
            py_feature['URI']  = value

            if (is_plugin(uri)):
              append_plugin_value(uri, 'Features', py_feature)
              add_plugin_value(uri, 'FeatureCount', 1)
            elif (is_ui(uri)):
              append_ui_value(uri, 'Features', py_feature)
              add_ui_value(uri, 'FeatureCount', 1)
            else:
              print("LV2_RDF - Invalid feature, not for Plugin or UI")

          elif (prop == rdf_prefix['lv2:requiredFeature']):
            py_feature = deepcopy(PyLV2_RDF_Feature)
            py_feature['Type'] = LV2_FEATURE_REQUIRED
            py_feature['URI']  = value

            if (is_plugin(uri)):
              append_plugin_value(uri, 'Features', py_feature)
              add_plugin_value(uri, 'FeatureCount', 1)
            elif (is_ui(uri)):
              append_ui_value(uri, 'Features', py_feature)
              add_ui_value(uri, 'FeatureCount', 1)
            else:
              print("LV2_RDF - Invalid feature, not for Plugin or UI")

          elif (prop == rdf_prefix['dc:replaces']):
            value_id_str = value.rsplit(":", 1)[-1]

            if (value_id_str.isdigit()):
              value_id = int(value_id_str)
            else:
              value_id = 0

            set_plugin_value(uri, 'UniqueID', value_id)

          elif (prop == rdf_prefix['doap:creator']):
            set_plugin_value(uri, 'Author', value)

          elif (prop == rdf_prefix['doap:description']):
            pass

          elif (prop == rdf_prefix['doap:developer']):
            set_plugin_value(uri, 'Author', value)

          elif (prop == rdf_prefix['doap:homepage']):
            pass

          elif (prop == rdf_prefix['doap:license']):
            set_plugin_value(uri, 'License', value)

          elif (prop == rdf_prefix['doap:maintainer']):
            set_plugin_value(uri, 'Author', value)

          elif (prop == rdf_prefix['doap:name']):
            set_plugin_value(uri, 'Name', value)

          elif (prop == rdf_prefix['rdfs:comment']):
            pass

          elif (prop == rdf_prefix['rdfs:label']):
            if (is_preset(uri)):
              set_preset_value(uri, 'Label', value)

          elif (prop == rdf_prefix['rdfs:seeAlso']):
            seeAlso = os.path.join(bundle_path, to_short_name(value, bundle_path))
            append_seeAlso_value(seeAlso)

          # Special
          elif (prop == rdf_prefix['llplug:pegName']):
            pass

          elif (prop == rdf_prefix['llplug:svgIcon']):
            pass

          elif (prop == "http://ll-plugins.nongnu.org/lv2/presets#presetFile"):
            pass

          elif (prop == "http://plugin.org.uk/extensions#code"):
            pass

          elif (prop == "http://plugin.org.uk/extensions#createdBy"):
            set_plugin_value(uri, 'Author', value)

          else:
            print("LV2_RDF - URI Property '%s' not handled (uri: %s)" % (prop, uri))

        elif (type(value) == BNode):
          # Handle it later
          nodes_info.append((to_string(uri), prop, to_string(value)))

        else:
          print("LV2_RDF - Don't know how to handle value '%s'" % value)

      elif (type(uri) == BNode):
        # Handle it later
        if (type(value) == Literal and is_number(value)):
          nodes_list.append((to_string(uri), prop, value))
        else:
          nodes_list.append((to_string(uri), prop, to_string(value)))

      else:
        print("LV2_RDF - Don't know how to handle URI", uri)

    else:
      print("LV2_RDF - Don't know how to handle property", prop)

  if (len(nodes_info) > 0 and len(nodes_list) == 0):
    print("LV2_RDF - Got a broken plugin! (has misleading information)")
    return

  # Process nodes

  for i in range(len(nodes_info)):
    uri, prop, value = nodes_info[i]

    # Plugin -----start
    if (is_plugin(uri)):

      # Port -----start
      if (prop == rdf_prefix['lv2:port']):

        py_port = deepcopy(PyLV2_RDF_Port)

        # Midi Map

        midi_map_node_try = get_node_property(value, nodes_list, rdf_prefix['lv2mm:defaultMidiController'], None)

        if (midi_map_node_try != None):
          midi_map_type_try   = get_node_property(midi_map_node_try, nodes_list, rdf_prefix['lv2mm:controllerType'], None)
          midi_map_number_try = get_node_property(midi_map_node_try, nodes_list, rdf_prefix['lv2mm:controllerNumber'], None)

          if (midi_map_type_try != None and midi_map_number_try != None and midi_map_number_try.isdigit()):
            py_port['MidiMap']['Type']   = get_c_port_midi_map_type(midi_map_type_try)
            py_port['MidiMap']['Number'] = int(midi_map_number_try)

        # Points

        default_try = get_node_property(value, nodes_list, rdf_prefix['lv2:default'], None)
        minimum_try = get_node_property(value, nodes_list, rdf_prefix['lv2:minimum'], None)
        maximum_try = get_node_property(value, nodes_list, rdf_prefix['lv2:maximum'], None)

        if (default_try != None and is_number(default_try)):
          py_port['Points']['Hints']  |= LV2_PORT_POINT_DEFAULT
          py_port['Points']['Default'] = float(default_try)

        if (minimum_try != None and is_number(minimum_try)):
          py_port['Points']['Hints']  |= LV2_PORT_POINT_MINIMUM
          py_port['Points']['Minimum'] = float(minimum_try)

        if (maximum_try != None and is_number(maximum_try)):
          py_port['Points']['Hints']  |= LV2_PORT_POINT_MAXIMUM
          py_port['Points']['Maximum'] = float(maximum_try)

        # Unit

        unit_try = get_node_property(value, nodes_list, rdf_prefix['lv2units:unit'], None)

        if (unit_try != None):
          py_port['Unit']['Hints'] |= LV2_PORT_UNIT
          py_port['Unit']['Type']   = get_c_port_unit_type(unit_try)

          unit_name_try   = get_node_property(value, nodes_list, rdf_prefix['lv2units:name'], None)
          unit_render_try = get_node_property(value, nodes_list, rdf_prefix['lv2units:render'], None)
          unit_symbol_try = get_node_property(value, nodes_list, rdf_prefix['lv2units:symbol'], None)

          if (unit_name_try != None):
            py_port['Unit']['Hints'] |= LV2_PORT_UNIT_NAME
            py_port['Unit']['Name']   = unit_name_try

          if (unit_render_try != None):
            py_port['Unit']['Hints'] |= LV2_PORT_UNIT_RENDER
            py_port['Unit']['Render'] = unit_render_try

          if (unit_symbol_try != None):
            py_port['Unit']['Hints'] |= LV2_PORT_UNIT_SYMBOL
            py_port['Unit']['Symbol'] = unit_symbol_try

        # Scale Points

        scalepoints = get_node_properties(value, nodes_list, rdf_prefix['lv2:scalePoint'])

        for scalepoint in scalepoints:
          label_try = get_node_property(scalepoint, nodes_list, rdf_prefix['rdfs:label'], None)
          value_try = get_node_property(scalepoint, nodes_list, rdf_prefix['rdf:value'], None)

          if (label_try != None and value_try != None and is_number(value_try)):
            py_scalepoint = deepcopy(PyLV2_RDF_PortScalePoint)
            py_scalepoint['Label'] = label_try
            py_scalepoint['Value'] = float(value_try)

            py_port['ScalePoints'].append(py_scalepoint)
            py_port['ScalePointCount'] += 1

          else:
            print("LV2_RDF - Internal error, incorrect Scale Points info")

        # The Port

        port_types  = get_node_properties(value, nodes_list, rdf_prefix['rdf:type'])
        port_props  = get_node_properties(value, nodes_list, rdf_prefix['lv2:portProperty'])
        port_index  = get_node_property(value, nodes_list, rdf_prefix['lv2:index'], None)
        port_name   = get_node_property(value, nodes_list, rdf_prefix['lv2:name'], None)
        port_symbol = get_node_property(value, nodes_list, rdf_prefix['lv2:symbol'], None)

        if (port_index == None or port_name == None or port_symbol == None):
          print("LV2_RDF - Internal error, incomplete Port data")
          continue

        for port_type in port_types:
          c_port_type = get_c_port_type(port_type)
          py_port['Type'] |= c_port_type

          if (c_port_type == LV2_PORT_EVENT):
            event_type_try = get_node_property(value, nodes_list, rdf_prefix['lv2ev:supportsEvent'], None)

            if (event_type_try != None):
              py_port['Type'] |= get_c_port_event_type(event_type_try)
            else:
              print("LV2_RDF - Internal error, Event Port without supported Event")

        for port_prop in port_props:
          py_port['Properties'] |= get_c_port_property(port_prop)

        py_port['index']  = int(port_index)
        py_port['Name']   = port_name
        py_port['Symbol'] = port_symbol

        append_plugin_value(uri, 'Ports', py_port)
        add_plugin_value(uri, 'PortCount', 1)

      # Port -----end

      elif (prop == rdf_prefix['doap:creator']):
        author = to_string(get_node_property(value, nodes_list, rdf_prefix['foaf:name'], ""))
        if (author and author != "(unicode error)"):
          set_plugin_value(uri, 'Author', author)

      elif (prop == rdf_prefix['doap:developer']):
        author = to_string(get_node_property(value, nodes_list, rdf_prefix['foaf:name'], ""))
        if (author and author != "(unicode error)"):
          set_plugin_value(uri, 'Author', author)

      elif (prop == rdf_prefix['doap:maintainer']):
        author = to_string(get_node_property(value, nodes_list, rdf_prefix['foaf:name'], ""))
        if (author and author != "(unicode error)"):
          set_plugin_value(uri, 'Author', author)

      # Special
      elif (prop == "http://plugin.org.uk/extensions#callback"):
        pass

      else:
        print("LV2_RDF - Plugin URI Property '%s' not handled (uri: %s)" % (prop, uri))

    # Plugin -----end

    # Preset -----start
    elif (is_preset(uri)):

      if (prop == rdf_prefix['lv2:port']):
        symbol_try = get_node_property(value, nodes_list, rdf_prefix['lv2:symbol'], None)
        value_try  = get_node_property(value, nodes_list, rdf_prefix['lv2pset:value'], None)

        if (symbol_try != None and value_try != None and is_number(value_try)):
          py_port = deepcopy(PyLV2_RDF_PresetPort)
          py_port['Symbol'] = symbol_try
          py_port['Value']  = float(value_try)

          append_preset_value(uri, 'Ports', py_port)
          add_preset_value(uri, 'PortCount', 1)

        else:
          print("LV2_RDF - Internal error, incomplete Preset Port data")

      elif (prop == rdf_prefix['lv2state:state']):
        node_props_try = get_all_node_properties(value, nodes_list)

        if (len(node_props_try) == 1):
          key, value = node_props_try[0]

          py_state = deepcopy(PyLV2_RDF_PresetState)
          py_state['Key']   = key
          py_state['Value'] = value

          append_preset_value(uri, 'States', py_state)
          add_preset_value(uri, 'StateCount', 1)

      else:
        print("LV2_RDF - Preset URI Property '%s' not handled (uri: %s)" % (prop, uri))

    # Preset -----end

    # UI ---------start
    elif (is_ui(uri)):

      if (prop == rdf_prefix['lv2ui:events']):
        pass

      elif (prop == rdf_prefix['lv2ui:portIndex']):
        pass

      elif (prop == rdf_prefix['lv2ui:portNotification']):
        pass

      elif (prop == rdf_prefix['lv2ui:residentSONames']):
        pass

      else:
        print("LV2_RDF - UI URI Property '%s' not handled (uri: %s)" % (prop, uri))

    # UI ---------end

    elif (prop.startswith(NS_lv2hi)):
      pass

    else:
      print("LV2_RDF - Don't know how to handle Node", prop)

# Parse ttl file into 'lists'
def parse_ttl_file(filename):
  primer = ConjunctiveGraph()
  try:
    primer.parse(filename, format='n3')
  except:
    #os.system("sed -i s/\\\r\\\n/\\\n/ %s" % (filename))
    #try:
      #primer.parse(filename, format='n3')
    #except:
    #print "LV2_RDF - Failed to parse", filename
    return []

  return [(x, y, z) for x, y, z in primer]

# Fully parse bundle
def parse_bundle(filename, bundle_path):
  parse = parse_ttl_file(filename)
  fill_information(parse, bundle_path)

# -------------------------------------------------------------------------------
#  LV2_RDF main methods

# Main function - check all bundles for information about lv2 plugins
def recheck_all_plugins(qobject=None):
  global LV2_PATH, LV2_Plugins, LV2_Presets, LV2_UIs, seeAlso_list

  LV2_Plugins   = []
  lv2_bundles   = []

  lv2_extensions   = (".lv2", ".lV2", ".LV2", ".Lv2")
  turle_extensions = (".ttl", ".ttL", ".tTL", ".TTL", ".TTl", ".Ttl")

  # Get all LV2 bundles
  for PATH in LV2_PATH:
    for root, dirs, files in os.walk(PATH):
      for bundle in [bundle for bundle in dirs if bundle.endswith(lv2_extensions)]:
        lv2_bundles.append(os.path.join(root, bundle))

  # Parse all manifest files
  for lv2_bundle in lv2_bundles:
    for ext in turle_extensions:
      if (os.path.exists(os.path.join(lv2_bundle, "manifest"+ext))):
        parse_bundle(os.path.join(lv2_bundle, "manifest"+ext), lv2_bundle)
        break
      elif (os.path.exists(os.path.join(lv2_bundle, "Manifest"+ext))):
        parse_bundle(os.path.join(lv2_bundle, "Manifest"+ext), lv2_bundle)
        break
      elif (os.path.exists(os.path.join(lv2_bundle, "MANIFEST"+ext))):
        parse_bundle(os.path.join(lv2_bundle, "MANIFEST"+ext), lv2_bundle)
        break

  # Parse all seeAlso files
  for i in range(len(seeAlso_list)):
    seealso_file = seeAlso_list[i]
    if (seealso_file and os.path.exists(seealso_file) and os.path.isfile(seealso_file)):
      if (os.path.getsize(seealso_file) > 1204000):
        print("LV2_RDF - File %s is too big, will not be parsed" % (seealso_file))
      else:
        if (qobject):
          # Tell GUI we're parsing this bundle
          percent = qobject.last_value + ( (float(i) / len(seeAlso_list) ) * qobject.percent_value )
          qobject.pluginLook(percent, seealso_file)

        # Fill the rest of the plugin information (from seeAlso file)
        bundle_path   = seealso_file.rsplit(os.sep, 1)[0]
        seealso_parse = parse_ttl_file(seealso_file)

        fill_information(seealso_parse, bundle_path)

  # Link Presets and UIs to their plugins
  for preset in LV2_Presets:
    uris = preset['appliesTo']
    for plugin in LV2_Plugins:
      if (plugin['URI'] in uris):
        plugin['Presets'].append(preset)
        plugin['PresetCount'] += 1
        break

  for ui in LV2_UIs:
    uris = ui['appliesTo']
    for plugin in LV2_Plugins:
      if (plugin['URI'] in uris):
        plugin['UIs'].append(ui)
        plugin['UICount'] += 1

  return LV2_Plugins

# Convert PyLV2_Plugins into ctype structs
def get_c_lv2_rdfs(PyPluginList):
  C_LV2_Plugins = []

  for plugin in PyPluginList:
    c_plugin = LV2_RDF_Descriptor()

    c_plugin.Type           = plugin['Type']
    c_plugin.URI            = plugin['URI']

    try:
      c_plugin.Name         = plugin['Name']
    except:
      c_plugin.Name         = "(unicode error)"

    try:
      c_plugin.Author       = plugin['Author']
    except:
      c_plugin.Author       = "(unicode error)"

    c_plugin.License        = plugin['License']
    c_plugin.Binary         = plugin['Binary']
    c_plugin.Bundle         = plugin['Bundle']

    c_plugin.UniqueID       = plugin['UniqueID']
    c_plugin.PortCount      = plugin['PortCount']
    c_plugin.PresetCount    = plugin['PresetCount']
    c_plugin.FeatureCount   = plugin['FeatureCount']
    c_plugin.ExtensionCount = plugin['ExtensionCount']
    c_plugin.UICount        = plugin['UICount']

    # Allocate data
    c_PortType          = LV2_RDF_Port*c_plugin.PortCount
    c_plugin.Ports      = c_PortType()

    c_PresetType        = LV2_RDF_Preset*c_plugin.PresetCount
    c_plugin.Presets    = c_PresetType()

    c_FeatureType       = LV2_RDF_Feature*c_plugin.FeatureCount
    c_plugin.Features   = c_FeatureType()

    c_ExtensionType     = LV2_URI*c_plugin.ExtensionCount
    c_plugin.Extensions = c_ExtensionType()

    c_UIType            = LV2_RDF_UI*c_plugin.UICount
    c_plugin.UIs        = c_UIType()

    # ------ Ports ------

    # Sort the ports by index
    py_ports = SORT_PyLV2_RDF_Ports(plugin['Ports'])

    i = 0
    for py_port in py_ports:
      c_port = LV2_RDF_Port()

      c_port.Type       = py_port['Type']
      c_port.Properties = py_port['Properties']

      try:
        c_port.Name     = py_port['Name']
      except:
        c_port.Name     = "(unicode error)"

      c_port.Symbol     = py_port['Symbol']

      c_port.MidiMap = LV2_RDF_PortMidiMap()
      c_port.MidiMap.Type   = py_port['MidiMap']['Type']
      c_port.MidiMap.Number = py_port['MidiMap']['Number']

      c_port.Points = LV2_RDF_PortPoints()
      c_port.Points.Hints    = py_port['Points']['Hints']
      c_port.Points.Default  = py_port['Points']['Default']
      c_port.Points.Minimum  = py_port['Points']['Minimum']
      c_port.Points.Maximum  = py_port['Points']['Maximum']

      c_port.Unit = LV2_RDF_PortUnit()
      c_port.Unit.Type   = py_port['Unit']['Type']
      c_port.Unit.Hints  = py_port['Unit']['Hints']
      c_port.Unit.Name   = py_port['Unit']['Name']
      c_port.Unit.Render = py_port['Unit']['Render']
      c_port.Unit.Symbol = py_port['Unit']['Symbol']

      c_port.ScalePointCount = py_port['ScalePointCount']

      c_PortScalePointType = LV2_RDF_PortScalePoint*c_port.ScalePointCount
      c_port.ScalePoints   = c_PortScalePointType()

      # Sort the port scale points by value
      py_port_scale_points = SORT_PyLV2_RDF_PortScalePoints(py_port['ScalePoints'])

      j = 0
      for py_port_scale_point in py_port_scale_points:
        c_scale_point = LV2_RDF_PortScalePoint()

        try:
          c_scale_point.Label = py_port_scale_point['Label']
        except:
          c_scale_point.Label = "(unicode error)"

        c_scale_point.Value = py_port_scale_point['Value']

        c_port.ScalePoints[j] = c_scale_point
        j += 1

      c_plugin.Ports[i] = c_port
      i += 1

    # ----- Presets -----

    i = 0
    for py_preset in plugin['Presets']:
      c_preset = LV2_RDF_Preset()

      c_preset.URI        = py_preset['URI']
      c_preset.Label      = py_preset['Label']

      c_preset.PortCount  = py_preset['PortCount']
      c_preset.StateCount = py_preset['StateCount']

      c_PresetPortType    = LV2_RDF_PresetPort*c_preset.PortCount
      c_preset.Ports      = c_PresetPortType()

      c_PresetStateType   = LV2_RDF_PresetState*c_preset.StateCount
      c_preset.States     = c_PresetStateType()

      j = 0
      for py_preset_port in py_preset['Ports']:
        c_preset_port = LV2_RDF_PresetPort()
        c_preset_port.Symbol = py_preset_port['Symbol']
        c_preset_port.Value  = py_preset_port['Value']

        c_preset.Ports[j] = c_preset_port
        j += 1

      j = 0
      for py_preset_state in py_preset['States']:
        c_preset_state = LV2_RDF_PresetState()
        c_preset_state.Key   = py_preset_state['Key']
        c_preset_state.Value = py_preset_state['Value']

        c_preset.States[j] = c_preset_state
        j += 1

      c_plugin.Presets[i] = c_preset
      i += 1

    # ---- Features -----

    i = 0
    for py_feature in plugin['Features']:
      c_feature = LV2_RDF_Feature()

      c_feature.Type = py_feature['Type']
      c_feature.URI  = py_feature['URI']

      c_plugin.Features[i] = c_feature
      i += 1

    # --- Extensions ----

    i = 0
    for py_extension in plugin['Extensions']:
      c_extension = py_extension

      c_plugin.Extensions[i] = c_extension
      i += 1

    # ------- UIs -------

    i = 0
    for py_ui in plugin['UIs']:
      c_ui = LV2_RDF_UI()

      c_ui.Type           = py_ui['Type']
      c_ui.URI            = py_ui['URI']
      c_ui.Binary         = py_ui['Binary']
      c_ui.Bundle         = py_ui['Bundle']
      c_ui.FeatureCount   = py_ui['FeatureCount']
      c_ui.ExtensionCount = py_ui['ExtensionCount']

      c_FeatureType = LV2_RDF_Feature*c_ui.FeatureCount
      c_ui.Features = c_FeatureType()

      c_ExtensionType = LV2_URI*c_ui.ExtensionCount
      c_ui.Extensions = c_ExtensionType()

      j = 0
      for py_feature in py_ui['Features']:
        c_feature = LV2_RDF_Feature()

        c_feature.Type = py_feature['Type']
        c_feature.URI  = py_feature['URI']

        c_ui.Features[j] = c_feature
        j += 1

      j = 0
      for py_extension in py_ui['Extensions']:
        c_extension = py_extension

        c_ui.Extensions[j] = c_extension
        j += 1

      c_plugin.UIs[i] = c_ui
      i += 1

    C_LV2_Plugins.append(c_plugin)

  return C_LV2_Plugins

# -------------------------------------------------------------------------------
#  Implementation test

#if __name__ == '__main__':
    #set_rdf_path(["/home/falktx/Personal/FOSS/GIT/distrho/bin/"])
    #set_rdf_path(["/home/falktx/.lv2/"])
    #plugins = recheck_all_plugins()
    #for plugin in LV2_Plugins:
      #print plugin['PresetCount'], plugin['URI']
      #for preset in plugin['Presets']:
        #print "    ", preset['Label'], preset['PortCount']
        #for port in preset['Ports']:
          #print "       ", port['Symbol'], "->", port['Value']
    #for preset in LV2_Presets:
      #print preset['URI'], preset['Label']
    #for ui in LV2_UIs:
      #print ui['URI']
    #get_c_lv2_rdfs(plugins)
    #print_extensions()

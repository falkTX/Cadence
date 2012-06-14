/*
 * Custom types to store LV2 information
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef LV2_RDF_INCLUDED
#define LV2_RDF_INCLUDED

#include <cstdint>

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/atom-forge.h"
#include "lv2/atom-util.h"
#include "lv2/data-access.h"
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/log.h"
#include "lv2/midi.h"
#include "lv2/patch.h"
#include "lv2/port-props.h"
#include "lv2/presets.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"
#include "lv2/worker.h"

#include "lv2/lv2dynparam.h"
#include "lv2/lv2-miditype.h"
#include "lv2/lv2-midifunctions.h"
#include "lv2/lv2_external_ui.h"
#include "lv2/lv2_programs.h"
#include "lv2/lv2_rtmempool.h"

// Base Types
typedef float LV2_Data;
typedef const char* LV2_URI;
typedef uint32_t LV2_Property;
typedef unsigned long long LV2_PluginType;

// Port Midi Map Types
#define LV2_PORT_MIDI_MAP_CC             0x1
#define LV2_PORT_MIDI_MAP_NRPN           0x2

#define LV2_IS_PORT_MIDI_MAP_CC(x)       ((x) == LV2_PORT_MIDI_MAP_CC)
#define LV2_IS_PORT_MIDI_MAP_NRPN(x)     ((x) == LV2_PORT_MIDI_MAP_NRPN)

// Port Midi Map
struct LV2_RDF_PortMidiMap {
    LV2_Property Type;
    uint32_t Number;
};

// Port Point Hints
#define LV2_PORT_POINT_DEFAULT           0x1
#define LV2_PORT_POINT_MINIMUM           0x2
#define LV2_PORT_POINT_MAXIMUM           0x4

#define LV2_HAVE_DEFAULT_PORT_POINT(x)   ((x) & LV2_PORT_POINT_DEFAULT)
#define LV2_HAVE_MINIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MINIMUM)
#define LV2_HAVE_MAXIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MAXIMUM)

// Port Points
struct LV2_RDF_PortPoints {
    LV2_Property Hints;
    LV2_Data Default;
    LV2_Data Minimum;
    LV2_Data Maximum;
};

// Port Unit Types
#define LV2_UNIT_BAR                     0x01
#define LV2_UNIT_BEAT                    0x02
#define LV2_UNIT_BPM                     0x03
#define LV2_UNIT_CENT                    0x04
#define LV2_UNIT_CM                      0x05
#define LV2_UNIT_COEF                    0x06
#define LV2_UNIT_DB                      0x07
#define LV2_UNIT_DEGREE                  0x08
#define LV2_UNIT_FRAME                   0x09
#define LV2_UNIT_HZ                      0x0A
#define LV2_UNIT_INCH                    0x0B
#define LV2_UNIT_KHZ                     0x0C
#define LV2_UNIT_KM                      0x0D
#define LV2_UNIT_M                       0x0E
#define LV2_UNIT_MHZ                     0x0F
#define LV2_UNIT_MIDINOTE                0x10
#define LV2_UNIT_MILE                    0x11
#define LV2_UNIT_MIN                     0x12
#define LV2_UNIT_MM                      0x13
#define LV2_UNIT_MS                      0x14
#define LV2_UNIT_OCT                     0x15
#define LV2_UNIT_PC                      0x16
#define LV2_UNIT_S                       0x17
#define LV2_UNIT_SEMITONE                0x18

#define LV2_IS_UNIT_BAR(x)               ((x) == LV2_UNIT_BAR)
#define LV2_IS_UNIT_BEAT(x)              ((x) == LV2_UNIT_BEAT)
#define LV2_IS_UNIT_BPM(x)               ((x) == LV2_UNIT_BPM)
#define LV2_IS_UNIT_CENT(x)              ((x) == LV2_UNIT_CENT)
#define LV2_IS_UNIT_CM(x)                ((x) == LV2_UNIT_CM)
#define LV2_IS_UNIT_COEF(x)              ((x) == LV2_UNIT_COEF)
#define LV2_IS_UNIT_DB(x)                ((x) == LV2_UNIT_DB)
#define LV2_IS_UNIT_DEGREE(x)            ((x) == LV2_UNIT_DEGREE)
#define LV2_IS_UNIT_FRAME(x)             ((x) == LV2_UNIT_FRAME)
#define LV2_IS_UNIT_HZ(x)                ((x) == LV2_UNIT_HZ)
#define LV2_IS_UNIT_INCH(x)              ((x) == LV2_UNIT_INCH)
#define LV2_IS_UNIT_KHZ(x)               ((x) == LV2_UNIT_KHZ)
#define LV2_IS_UNIT_KM(x)                ((x) == LV2_UNIT_KM)
#define LV2_IS_UNIT_M(x)                 ((x) == LV2_UNIT_M)
#define LV2_IS_UNIT_MHZ(x)               ((x) == LV2_UNIT_MHZ)
#define LV2_IS_UNIT_MIDINOTE(x)          ((x) == LV2_UNIT_MIDINOTE)
#define LV2_IS_UNIT_MILE(x)              ((x) == LV2_UNIT_MILE)
#define LV2_IS_UNIT_MIN(x)               ((x) == LV2_UNIT_MIN)
#define LV2_IS_UNIT_MM(x)                ((x) == LV2_UNIT_MM)
#define LV2_IS_UNIT_MS(x)                ((x) == LV2_UNIT_MS)
#define LV2_IS_UNIT_OCT(x)               ((x) == LV2_UNIT_OCT)
#define LV2_IS_UNIT_PC(x)                ((x) == LV2_UNIT_PC)
#define LV2_IS_UNIT_S(x)                 ((x) == LV2_UNIT_S)
#define LV2_IS_UNIT_SEMITONE(x)          ((x) == LV2_UNIT_SEMITONE)

// Port Unit Hints
#define LV2_PORT_UNIT                    0x1
#define LV2_PORT_UNIT_NAME               0x2
#define LV2_PORT_UNIT_RENDER             0x4
#define LV2_PORT_UNIT_SYMBOL             0x8

#define LV2_HAVE_UNIT(x)                 ((x) & LV2_PORT_UNIT)
#define LV2_HAVE_UNIT_NAME(x)            ((x) & LV2_PORT_UNIT_NAME)
#define LV2_HAVE_UNIT_RENDER(x)          ((x) & LV2_PORT_UNIT_RENDER)
#define LV2_HAVE_UNIT_SYMBOL(x)          ((x) & LV2_PORT_UNIT_SYMBOL)

// Port Unit
struct LV2_RDF_PortUnit {
    LV2_Property Type;
    LV2_Property Hints;
    const char* Name;
    const char* Render;
    const char* Symbol;
};

// Port Scale Point
struct LV2_RDF_PortScalePoint {
    const char* Label;
    LV2_Data Value;
};

// Port Types
#define LV2_PORT_INPUT                   0x01
#define LV2_PORT_OUTPUT                  0x02
#define LV2_PORT_CONTROL                 0x04
#define LV2_PORT_AUDIO                   0x08
#define LV2_PORT_CV                      0x10
#define LV2_PORT_ATOM                    0x20
#define LV2_PORT_ATOM_SEQUENCE          (0x40 | LV2_PORT_ATOM)
#define LV2_PORT_EVENT                   0x80
#define LV2_PORT_MIDI_LL                 0x100

// Port Support Types
#define LV2_PORT_SUPPORTS_MIDI_EVENT     0x1000
#define LV2_PORT_SUPPORTS_PATCH_MESSAGE  0x2000

#define LV2_IS_PORT_INPUT(x)             ((x) & LV2_PORT_INPUT)
#define LV2_IS_PORT_OUTPUT(x)            ((x) & LV2_PORT_OUTPUT)
#define LV2_IS_PORT_CONTROL(x)           ((x) & LV2_PORT_CONTROL)
#define LV2_IS_PORT_AUDIO(x)             ((x) & LV2_PORT_AUDIO)
#define LV2_IS_PORT_ATOM_SEQUENCE(x)     ((x) & LV2_PORT_ATOM_SEQUENCE)
#define LV2_IS_PORT_CV(x)                ((x) & LV2_PORT_CV)
#define LV2_IS_PORT_EVENT(x)             ((x) & LV2_PORT_EVENT)
#define LV2_IS_PORT_MIDI_LL(x)           ((x) & LV2_PORT_MIDI_LL)

// Port Properties
#define LV2_PORT_OPTIONAL                0x0001
#define LV2_PORT_ENUMERATION             0x0002
#define LV2_PORT_INTEGER                 0x0004
#define LV2_PORT_SAMPLE_RATE             0x0008
#define LV2_PORT_TOGGLED                 0x0010
#define LV2_PORT_CAUSES_ARTIFACTS        0x0020
#define LV2_PORT_CONTINUOUS_CV           0x0040
#define LV2_PORT_DISCRETE_CV             0x0080
#define LV2_PORT_EXPENSIVE               0x0100
#define LV2_PORT_STRICT_BOUNDS           0x0200
#define LV2_PORT_LOGARITHMIC             0x0400
#define LV2_PORT_NOT_AUTOMATIC           0x0800
#define LV2_PORT_NOT_ON_GUI              0x1000
#define LV2_PORT_TRIGGER                 0x2000

#define LV2_IS_PORT_OPTIONAL(x)          ((x) & LV2_PORT_OPTIONAL)
#define LV2_IS_PORT_ENUMERATION(x)       ((x) & LV2_PORT_ENUMERATION)
#define LV2_IS_PORT_INTEGER(x)           ((x) & LV2_PORT_INTEGER)
#define LV2_IS_PORT_SAMPLE_RATE(x)       ((x) & LV2_PORT_SAMPLE_RATE)
#define LV2_IS_PORT_TOGGLED(x)           ((x) & LV2_PORT_TOGGLED)
#define LV2_IS_PORT_CAUSES_ARTIFACTS(x)  ((x) & LV2_PORT_CAUSES_ARTIFACTS)
#define LV2_IS_PORT_CONTINUOUS_CV(x)     ((x) & LV2_PORT_CONTINUOUS_CV)
#define LV2_IS_PORT_DISCRETE_CV(x)       ((x) & LV2_PORT_DISCRETE_CV)
#define LV2_IS_PORT_EXPENSIVE(x)         ((x) & LV2_PORT_EXPENSIVE)
#define LV2_IS_PORT_STRICT_BOUNDS(x)     ((x) & LV2_PORT_STRICT_BOUNDS)
#define LV2_IS_PORT_LOGARITHMIC(x)       ((x) & LV2_PORT_LOGARITHMIC)
#define LV2_IS_PORT_NOT_AUTOMATIC(x)     ((x) & LV2_PORT_NOT_AUTOMATIC)
#define LV2_IS_PORT_NOT_ON_GUI(x)        ((x) & LV2_PORT_NOT_ON_GUI)
#define LV2_IS_PORT_TRIGGER(x)           ((x) & LV2_PORT_TRIGGER)

// Port Designation
#define LV2_PORT_LATENCY                 0x1
#define LV2_PORT_TIME_BAR                0x2
#define LV2_PORT_TIME_BAR_BEAT           0x3
#define LV2_PORT_TIME_BEAT               0x4
#define LV2_PORT_TIME_BEAT_UNIT          0x5
#define LV2_PORT_TIME_BEATS_PER_BAR      0x6
#define LV2_PORT_TIME_BEATS_PER_MINUTE   0x7
#define LV2_PORT_TIME_FRAME              0x8
#define LV2_PORT_TIME_FRAMES_PER_SECOND  0x9
#define LV2_PORT_TIME_POSITION           0xA
#define LV2_PORT_TIME_SPEED              0xB

#define LV2_IS_PORT_LATENCY(x)           ((x) == LV2_PORT_LATENCY)
#define LV2_IS_PORT_TIME_BAR(x)          ((x) == LV2_PORT_TIME_BAR)
#define LV2_IS_PORT_TIME_BAR_BEAT(x)     ((x) == LV2_PORT_TIME_BAR_BEAT)
#define LV2_IS_PORT_TIME_BEAT(x)         ((x) == LV2_PORT_TIME_BEAT)
#define LV2_IS_PORT_TIME_BEAT_UNIT(x)    ((x) == LV2_PORT_TIME_BEAT_UNIT)
#define LV2_IS_PORT_TIME_BEATS_PER_BAR(x)     ((x) == LV2_PORT_TIME_BEATS_PER_BAR)
#define LV2_IS_PORT_TIME_BEATS_PER_MINUTE(x)  ((x) == LV2_PORT_TIME_BEATS_PER_MINUTE)
#define LV2_IS_PORT_TIME_FRAME(x)             ((x) == LV2_PORT_TIME_FRAME)
#define LV2_IS_PORT_TIME_FRAMES_PER_SECOND(x) ((x) == LV2_PORT_TIME_FRAMES_PER_SECOND)
#define LV2_IS_PORT_TIME_POSITION(x)          ((x) == LV2_PORT_TIME_POSITION)
#define LV2_IS_PORT_TIME_SPEED(x)             ((x) == LV2_PORT_TIME_SPEED)

// Port
struct LV2_RDF_Port {
    LV2_Property Type;
    LV2_Property Properties;
    LV2_Property Designation;
    const char* Name;
    const char* Symbol;

    LV2_RDF_PortMidiMap MidiMap;
    LV2_RDF_PortPoints Points;
    LV2_RDF_PortUnit Unit;

    uint32_t ScalePointCount;
    LV2_RDF_PortScalePoint* ScalePoints;
};

// Preset Port
struct LV2_RDF_PresetPort {
    const char* Symbol;
    LV2_Data Value;
};

// Preset State Types
#define LV2_PRESET_STATE_NULL            0x0
#define LV2_PRESET_STATE_BOOL            0x1
#define LV2_PRESET_STATE_INT             0x2
#define LV2_PRESET_STATE_LONG            0x3
#define LV2_PRESET_STATE_FLOAT           0x4
#define LV2_PRESET_STATE_STRING          0x5
#define LV2_PRESET_STATE_BINARY          0x6

#define LV2_IS_PRESET_STATE_NULL(x)      ((x) == LV2_PRESET_STATE_NULL)
#define LV2_IS_PRESET_STATE_BOOL(x)      ((x) == LV2_PRESET_STATE_BOOL)
#define LV2_IS_PRESET_STATE_INT(x)       ((x) == LV2_PRESET_STATE_INT)
#define LV2_IS_PRESET_STATE_LONG(x)      ((x) == LV2_PRESET_STATE_LONG)
#define LV2_IS_PRESET_STATE_FLOAT(x)     ((x) == LV2_PRESET_STATE_FLOAT)
#define LV2_IS_PRESET_STATE_STRING(x)    ((x) == LV2_PRESET_STATE_STRING)
#define LV2_IS_PRESET_STATE_BINARY(x)    ((x) == LV2_PRESET_STATE_BINARY)

// Preset State
struct LV2_RDF_PresetState {
    LV2_Property Type;
    const char* Key;
    union {
        bool b;
        int i;
        long li;
        float f;
        const char* s;
    } Value;
};

// A Preset
struct LV2_RDF_Preset {
    LV2_URI URI;
    const char* Label;

    uint32_t PortCount;
    LV2_RDF_PresetPort* Ports;

    uint32_t StateCount;
    LV2_RDF_PresetState* States;
};

// Feature Types
#define LV2_FEATURE_OPTIONAL             0x1
#define LV2_FEATURE_REQUIRED             0x2

#define LV2_IS_FEATURE_OPTIONAL(x)       ((x) == LV2_FEATURE_OPTIONAL)
#define LV2_IS_FEATURE_REQUIRED(x)       ((x) == LV2_FEATURE_REQUIRED)

// Feature
struct LV2_RDF_Feature {
    LV2_Property Type;
    LV2_URI URI;
};

// UI Types
#define LV2_UI_GTK2                      0x1
#define LV2_UI_QT4                       0x2
#define LV2_UI_X11                       0x3
#define LV2_UI_EXTERNAL                  0x4
#define LV2_UI_OLD_EXTERNAL              0x5

#define LV2_IS_UI_GTK2(x)                ((x) == LV2_UI_GTK2)
#define LV2_IS_UI_QT4(x)                 ((x) == LV2_UI_QT4)
#define LV2_IS_UI_X11(x)                 ((x) == LV2_UI_X11)
#define LV2_IS_UI_EXTERNAL(x)            ((x) == LV2_UI_EXTERNAL)
#define LV2_IS_UI_OLD_EXTERNAL(x)        ((x) == LV2_UI_OLD_EXTERNAL)

// UI
struct LV2_RDF_UI {
    LV2_Property Type;
    LV2_URI URI;
    const char* Binary;
    const char* Bundle;

    uint32_t FeatureCount;
    LV2_RDF_Feature* Features;

    uint32_t ExtensionCount;
    LV2_URI* Extensions;
};

// Plugin Types
#define LV2_CLASS_ALLPASS                0x000000001LL
#define LV2_CLASS_AMPLIFIER              0x000000002LL
#define LV2_CLASS_ANALYSER               0x000000004LL
#define LV2_CLASS_BANDPASS               0x000000008LL
#define LV2_CLASS_CHORUS                 0x000000010LL
#define LV2_CLASS_COMB                   0x000000020LL
#define LV2_CLASS_COMPRESSOR             0x000000040LL
#define LV2_CLASS_CONSTANT               0x000000080LL
#define LV2_CLASS_CONVERTER              0x000000100LL
#define LV2_CLASS_DELAY                  0x000000200LL
#define LV2_CLASS_DISTORTION             0x000000400LL
#define LV2_CLASS_DYNAMICS               0x000000800LL
#define LV2_CLASS_EQ                     0x000001000LL
#define LV2_CLASS_EXPANDER               0x000002000LL
#define LV2_CLASS_FILTER                 0x000004000LL
#define LV2_CLASS_FLANGER                0x000008000LL
#define LV2_CLASS_FUNCTION               0x000010000LL
#define LV2_CLASS_GATE                   0x000020000LL
#define LV2_CLASS_GENERATOR              0x000040000LL
#define LV2_CLASS_HIGHPASS               0x000080000LL
#define LV2_CLASS_INSTRUMENT             0x000100000LL
#define LV2_CLASS_LIMITER                0x000200000LL
#define LV2_CLASS_LOWPASS                0x000400000LL
#define LV2_CLASS_MIXER                  0x000800000LL
#define LV2_CLASS_MODULATOR              0x001000000LL
#define LV2_CLASS_MULTI_EQ               0x002000000LL
#define LV2_CLASS_OSCILLATOR             0x004000000LL
#define LV2_CLASS_PARA_EQ                0x008000000LL
#define LV2_CLASS_PHASER                 0x010000000LL
#define LV2_CLASS_PITCH                  0x020000000LL
#define LV2_CLASS_REVERB                 0x040000000LL
#define LV2_CLASS_SIMULATOR              0x080000000LL
#define LV2_CLASS_SPATIAL                0x100000000LL
#define LV2_CLASS_SPECTRAL               0x200000000LL
#define LV2_CLASS_UTILITY                0x400000000LL
#define LV2_CLASS_WAVESHAPER             0x800000000LL

#define LV2_GROUP_DELAY                  (LV2_CLASS_DELAY|LV2_CLASS_REVERB)
#define LV2_GROUP_DISTORTION             (LV2_CLASS_DISTORTION|LV2_CLASS_WAVESHAPER)
#define LV2_GROUP_DYNAMICS               (LV2_CLASS_DYNAMICS|LV2_CLASS_AMPLIFIER|LV2_CLASS_COMPRESSOR|LV2_CLASS_EXPANDER|LV2_CLASS_GATE|LV2_CLASS_LIMITER)
#define LV2_GROUP_EQ                     (LV2_CLASS_EQ|LV2_CLASS_PARA_EQ|LV2_CLASS_MULTI_EQ)
#define LV2_GROUP_FILTER                 (LV2_CLASS_FILTER|LV2_CLASS_ALLPASS|LV2_CLASS_BANDPASS|LV2_CLASS_COMB|LV2_GROUP_EQ|LV2_CLASS_HIGHPASS|LV2_CLASS_LOWPASS)
#define LV2_GROUP_GENERATOR              (LV2_CLASS_GENERATOR|LV2_CLASS_CONSTANT|LV2_CLASS_INSTRUMENT|LV2_CLASS_OSCILLATOR)
#define LV2_GROUP_MODULATOR              (LV2_CLASS_MODULATOR|LV2_CLASS_CHORUS|LV2_CLASS_FLANGER|LV2_CLASS_PHASER)
#define LV2_GROUP_REVERB                 (LV2_CLASS_REVERB)
#define LV2_GROUP_SIMULATOR              (LV2_CLASS_SIMULATOR|LV2_CLASS_REVERB)
#define LV2_GROUP_SPATIAL                (LV2_CLASS_SPATIAL)
#define LV2_GROUP_SPECTRAL               (LV2_CLASS_SPECTRAL|LV2_CLASS_PITCH)
#define LV2_GROUP_UTILITY                (LV2_CLASS_UTILITY|LV2_CLASS_ANALYSER|LV2_CLASS_CONVERTER|LV2_CLASS_FUNCTION|LV2_CLASS_MIXER)

#define LV2_IS_DELAY(x)                  ((x) & LV2_GROUP_DELAY)
#define LV2_IS_DISTORTION(x)             ((x) & LV2_GROUP_DISTORTION)
#define LV2_IS_DYNAMICS(x)               ((x) & LV2_GROUP_DYNAMICS)
#define LV2_IS_EQ(x)                     ((x) & LV2_GROUP_EQ)
#define LV2_IS_FILTER(x)                 ((x) & LV2_GROUP_FILTER)
#define LV2_IS_GENERATOR(x)              ((x) & LV2_GROUP_GENERATOR)
#define LV2_IS_MODULATOR(x)              ((x) & LV2_GROUP_MODULATOR)
#define LV2_IS_REVERB(x)                 ((x) & LV2_GROUP_REVERB)
#define LV2_IS_SIMULATOR(x)              ((x) & LV2_GROUP_SIMULATOR)
#define LV2_IS_SPATIAL(x)                ((x) & LV2_GROUP_SPATIAL)
#define LV2_IS_SPECTRAL(x)               ((x) & LV2_GROUP_SPECTRAL)
#define LV2_IS_UTILITY(x)                ((x) & LV2_GROUP_UTILITY)

// Plugin
struct LV2_RDF_Descriptor {
    LV2_PluginType Type;
    LV2_URI URI;
    const char* Name;
    const char* Author;
    const char* License;
    const char* Binary;
    const char* Bundle;
    unsigned long UniqueID;

    uint32_t PortCount;
    LV2_RDF_Port* Ports;

    uint32_t PresetCount;
    LV2_RDF_Preset* Presets;

    uint32_t FeatureCount;
    LV2_RDF_Feature* Features;

    uint32_t ExtensionCount;
    LV2_URI* Extensions;

    uint32_t UICount;
    LV2_RDF_UI* UIs;
};

// ------------------------------------------------------------------------------------------------

#include "lilv/lilvmm.hpp"

#include <QtCore/QString>
#include <QtCore/QStringList>

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

class Lv2WorldClass : public Lilv::World
{
public:
    Lv2WorldClass() : Lilv::World(),
        port                (new_uri(LV2_CORE__port)),
        symbol              (new_uri(LV2_CORE__symbol)),
        class_allpass       (new_uri(LV2_CORE__AllpassPlugin)),
        class_amplifier     (new_uri(LV2_CORE__AmplifierPlugin)),
        class_analyzer      (new_uri(LV2_CORE__AnalyserPlugin)),
        class_bandpass      (new_uri(LV2_CORE__BandpassPlugin)),
        class_chorus        (new_uri(LV2_CORE__ChorusPlugin)),
        class_comb          (new_uri(LV2_CORE__CombPlugin)),
        class_compressor    (new_uri(LV2_CORE__CompressorPlugin)),
        class_constant      (new_uri(LV2_CORE__ConstantPlugin)),
        class_converter     (new_uri(LV2_CORE__ConverterPlugin)),
        class_delay         (new_uri(LV2_CORE__DelayPlugin)),
        class_distortion    (new_uri(LV2_CORE__DistortionPlugin)),
        class_dynamics      (new_uri(LV2_CORE__DynamicsPlugin)),
        class_eq            (new_uri(LV2_CORE__EQPlugin)),
        class_expander      (new_uri(LV2_CORE__ExpanderPlugin)),
        class_filter        (new_uri(LV2_CORE__FilterPlugin)),
        class_flanger       (new_uri(LV2_CORE__FlangerPlugin)),
        class_function      (new_uri(LV2_CORE__FunctionPlugin)),
        class_gate          (new_uri(LV2_CORE__GatePlugin)),
        class_generator     (new_uri(LV2_CORE__GeneratorPlugin)),
        class_highpass      (new_uri(LV2_CORE__HighpassPlugin)),
        class_instrument    (new_uri(LV2_CORE__InstrumentPlugin)),
        class_limiter       (new_uri(LV2_CORE__LimiterPlugin)),
        class_lowpass       (new_uri(LV2_CORE__LowpassPlugin)),
        class_mixer         (new_uri(LV2_CORE__MixerPlugin)),
        class_modulator     (new_uri(LV2_CORE__ModulatorPlugin)),
        class_multi_eq      (new_uri(LV2_CORE__MultiEQPlugin)),
        class_oscillator    (new_uri(LV2_CORE__OscillatorPlugin)),
        class_para_eq       (new_uri(LV2_CORE__ParaEQPlugin)),
        class_phaser        (new_uri(LV2_CORE__PhaserPlugin)),
        class_pitch         (new_uri(LV2_CORE__PitchPlugin)),
        class_reverb        (new_uri(LV2_CORE__ReverbPlugin)),
        class_simulator     (new_uri(LV2_CORE__SimulatorPlugin)),
        class_spatial       (new_uri(LV2_CORE__SpatialPlugin)),
        class_spectral      (new_uri(LV2_CORE__SpectralPlugin)),
        class_utility       (new_uri(LV2_CORE__UtilityPlugin)),
        class_waveshaper    (new_uri(LV2_CORE__WaveshaperPlugin)),

        port_input          (new_uri(LV2_CORE__InputPort)),
        port_output         (new_uri(LV2_CORE__OutputPort)),
        port_control        (new_uri(LV2_CORE__ControlPort)),
        port_audio          (new_uri(LV2_CORE__AudioPort)),
        port_cv             (new_uri(LV2_CORE__CVPort)),
        port_atom           (new_uri(LV2_ATOM__AtomPort)),
        port_event          (new_uri(LV2_EVENT__EventPort)),
        port_midi_ll        (new_uri(LV2_MIDI_LL__MidiPort)),

        pprop_optional      (new_uri(LV2_CORE__connectionOptional)),
        pprop_enumeration   (new_uri(LV2_CORE__enumeration)),
        pprop_integer       (new_uri(LV2_CORE__integer)),
        pprop_sample_rate   (new_uri(LV2_CORE__sampleRate)),
        pprop_toggled       (new_uri(LV2_CORE__toggled)),
        pprop_artifacts     (new_uri(LV2_PORT_PROPS__causesArtifacts)),
        pprop_continuous_cv (new_uri(LV2_PORT_PROPS__continuousCV)),
        pprop_discrete_cv   (new_uri(LV2_PORT_PROPS__discreteCV)),
        pprop_expensive     (new_uri(LV2_PORT_PROPS__expensive)),
        pprop_strict_bounds (new_uri(LV2_PORT_PROPS__hasStrictBounds)),
        pprop_logarithmic   (new_uri(LV2_PORT_PROPS__logarithmic)),
        pprop_not_automatic (new_uri(LV2_PORT_PROPS__notAutomatic)),
        pprop_not_on_gui    (new_uri(LV2_PORT_PROPS__notOnGUI)),
        pprop_trigger       (new_uri(LV2_PORT_PROPS__trigger)),

        unit_unit           (new_uri(LV2_UNITS__unit)),
        unit_name           (new_uri(LV2_UNITS__name)),
        unit_render         (new_uri(LV2_UNITS__render)),
        unit_symbol         (new_uri(LV2_UNITS__symbol)),

        unit_bar            (new_uri(LV2_UNITS__bar)),
        unit_beat           (new_uri(LV2_UNITS__beat)),
        unit_bpm            (new_uri(LV2_UNITS__bpm)),
        unit_cent           (new_uri(LV2_UNITS__cent)),
        unit_cm             (new_uri(LV2_UNITS__cm)),
        unit_coef           (new_uri(LV2_UNITS__coef)),
        unit_db             (new_uri(LV2_UNITS__db)),
        unit_degree         (new_uri(LV2_UNITS__degree)),
        unit_frame          (new_uri(LV2_UNITS__frame)),
        unit_hz             (new_uri(LV2_UNITS__hz)),
        unit_inch           (new_uri(LV2_UNITS__inch)),
        unit_khz            (new_uri(LV2_UNITS__khz)),
        unit_km             (new_uri(LV2_UNITS__km)),
        unit_m              (new_uri(LV2_UNITS__m)),
        unit_mhz            (new_uri(LV2_UNITS__mhz)),
        unit_midi_note      (new_uri(LV2_UNITS__midiNote)),
        unit_mile           (new_uri(LV2_UNITS__mile)),
        unit_min            (new_uri(LV2_UNITS__min)),
        unit_mm             (new_uri(LV2_UNITS__mm)),
        unit_ms             (new_uri(LV2_UNITS__ms)),
        unit_oct            (new_uri(LV2_UNITS__oct)),
        unit_pc             (new_uri(LV2_UNITS__pc)),
        unit_s              (new_uri(LV2_UNITS__s)),
        unit_semitone       (new_uri(LV2_UNITS__semitone12TET)),

        ui_gtk2             (new_uri(LV2_UI__GtkUI)),
        ui_qt4              (new_uri(LV2_UI__Qt4UI)),
        ui_x11              (new_uri(LV2_UI__X11UI)),
        ui_external         (new_uri(LV2_EXTERNAL_UI_URI)),
        ui_external_old     (new_uri(LV2_EXTERNAL_UI_DEPRECATED_URI)),

        preset_preset       (new_uri(LV2_PRESETS__Preset)),
        preset_value        (new_uri(LV2_PRESETS__value)),

        time_bar            (new_uri(LV2_TIME__bar)),
        time_barBeat        (new_uri(LV2_TIME__barBeat)),
        time_beat           (new_uri(LV2_TIME__beat)),
        time_beatUnit       (new_uri(LV2_TIME__beatUnit)),
        time_beatsPerBar    (new_uri(LV2_TIME__beatsPerBar)),
        time_beatsPerMinute (new_uri(LV2_TIME__beatsPerMinute)),
        time_frame          (new_uri(LV2_TIME__frame)),
        time_framesPerSecond (new_uri(LV2_TIME__framesPerSecond)),
        time_position       (new_uri(LV2_TIME__position)),
        time_speed          (new_uri(LV2_TIME__speed)),

        value_default       (new_uri(LV2_CORE__default)),
        value_minimum       (new_uri(LV2_CORE__minimum)),
        value_maximum       (new_uri(LV2_CORE__maximum)),

        atom_sequence       (new_uri(LV2_ATOM__Sequence)),
        atom_buffer_type    (new_uri(LV2_ATOM__bufferType)),
        atom_supports       (new_uri(LV2_ATOM__supports)),

        midi_event          (new_uri(LV2_MIDI__MidiEvent)),
        patch_message       (new_uri(LV2_PATCH__Message)),

        mm_default_control  (new_uri(NS_llmm "defaultMidiController")),
        mm_control_type     (new_uri(NS_llmm "controllerType")),
        mm_control_number   (new_uri(NS_llmm "controllerNumber")),

        dct_replaces        (new_uri(NS_dct "replaces")),
        doap_license        (new_uri(NS_doap "license")),
        rdf_type            (new_uri(NS_rdf "type")),
        rdfs_label          (new_uri(NS_rdfs "label"))
    {}

    // Base Types
    Lilv::Node port;
    Lilv::Node symbol;

    // Plugin Types
    Lilv::Node class_allpass;
    Lilv::Node class_amplifier;
    Lilv::Node class_analyzer;
    Lilv::Node class_bandpass;
    Lilv::Node class_chorus;
    Lilv::Node class_comb;
    Lilv::Node class_compressor;
    Lilv::Node class_constant;
    Lilv::Node class_converter;
    Lilv::Node class_delay;
    Lilv::Node class_distortion;
    Lilv::Node class_dynamics;
    Lilv::Node class_eq;
    Lilv::Node class_expander;
    Lilv::Node class_filter;
    Lilv::Node class_flanger;
    Lilv::Node class_function;
    Lilv::Node class_gate;
    Lilv::Node class_generator;
    Lilv::Node class_highpass;
    Lilv::Node class_instrument;
    Lilv::Node class_limiter;
    Lilv::Node class_lowpass;
    Lilv::Node class_mixer;
    Lilv::Node class_modulator;
    Lilv::Node class_multi_eq;
    Lilv::Node class_oscillator;
    Lilv::Node class_para_eq;
    Lilv::Node class_phaser;
    Lilv::Node class_pitch;
    Lilv::Node class_reverb;
    Lilv::Node class_simulator;
    Lilv::Node class_spatial;
    Lilv::Node class_spectral;
    Lilv::Node class_utility;
    Lilv::Node class_waveshaper;

    // Port Types
    Lilv::Node port_input;
    Lilv::Node port_output;
    Lilv::Node port_control;
    Lilv::Node port_audio;
    Lilv::Node port_cv;
    Lilv::Node port_atom;
    Lilv::Node port_event;
    Lilv::Node port_midi_ll;

    // Port Properties
    Lilv::Node pprop_optional;
    Lilv::Node pprop_enumeration;
    Lilv::Node pprop_integer;
    Lilv::Node pprop_sample_rate;
    Lilv::Node pprop_toggled;
    Lilv::Node pprop_artifacts;
    Lilv::Node pprop_continuous_cv;
    Lilv::Node pprop_discrete_cv;
    Lilv::Node pprop_expensive;
    Lilv::Node pprop_strict_bounds;
    Lilv::Node pprop_logarithmic;
    Lilv::Node pprop_not_automatic;
    Lilv::Node pprop_not_on_gui;
    Lilv::Node pprop_trigger;

    // Unit Hints
    Lilv::Node unit_unit;
    Lilv::Node unit_name;
    Lilv::Node unit_render;
    Lilv::Node unit_symbol;

    // Unit Types
    Lilv::Node unit_bar;
    Lilv::Node unit_beat;
    Lilv::Node unit_bpm;
    Lilv::Node unit_cent;
    Lilv::Node unit_cm;
    Lilv::Node unit_coef;
    Lilv::Node unit_db;
    Lilv::Node unit_degree;
    Lilv::Node unit_frame;
    Lilv::Node unit_hz;
    Lilv::Node unit_inch;
    Lilv::Node unit_khz;
    Lilv::Node unit_km;
    Lilv::Node unit_m;
    Lilv::Node unit_mhz;
    Lilv::Node unit_midi_note;
    Lilv::Node unit_mile;
    Lilv::Node unit_min;
    Lilv::Node unit_mm;
    Lilv::Node unit_ms;
    Lilv::Node unit_oct;
    Lilv::Node unit_pc;
    Lilv::Node unit_s;
    Lilv::Node unit_semitone;

    // UI Types
    Lilv::Node ui_gtk2;
    Lilv::Node ui_qt4;
    Lilv::Node ui_x11;
    Lilv::Node ui_external;
    Lilv::Node ui_external_old;

    Lilv::Node preset_preset;
    Lilv::Node preset_value;

    // LV2 stuff
    Lilv::Node time_bar;
    Lilv::Node time_barBeat;
    Lilv::Node time_beat;
    Lilv::Node time_beatUnit;
    Lilv::Node time_beatsPerBar;
    Lilv::Node time_beatsPerMinute;
    Lilv::Node time_frame;
    Lilv::Node time_framesPerSecond;
    Lilv::Node time_position;
    Lilv::Node time_speed;

    Lilv::Node value_default;
    Lilv::Node value_minimum;
    Lilv::Node value_maximum;

    Lilv::Node atom_sequence;
    Lilv::Node atom_buffer_type;
    Lilv::Node atom_supports;

    // Event Data/Types
    Lilv::Node midi_event;
    Lilv::Node patch_message;

    // MIDI CC
    Lilv::Node mm_default_control;
    Lilv::Node mm_control_type;
    Lilv::Node mm_control_number;

    // Other
    Lilv::Node dct_replaces;
    Lilv::Node doap_license;
    Lilv::Node rdf_type;
    Lilv::Node rdfs_label;

    void init()
    {
        static bool need_init = true;
        if (need_init)
        {
            need_init = false;
            load_all();
        }
    }
};

static Lv2WorldClass Lv2World;

// ------------------------------------------------------------------------------------------------

// Create new RDF object
inline const LV2_RDF_Descriptor* lv2_rdf_new(const char* URI)
{
    const Lilv::Plugins Plugins = Lv2World.get_all_plugins();

    LILV_FOREACH(plugins, i, Plugins)
    {
        Lilv::Plugin Plugin(lilv_plugins_get(Plugins, i));

        if (strcmp(Plugin.get_uri().as_string(), URI) == 0)
        {
            LV2_RDF_Descriptor* rdf_descriptor = new LV2_RDF_Descriptor;

            // --------------------------------------------------
            // Set Plugin Type

            rdf_descriptor->Type = 0x0;

            Lilv::Nodes types(Plugin.get_value(Lv2World.rdf_type));

            if (types.contains(Lv2World.class_allpass))
                rdf_descriptor->Type |= LV2_CLASS_ALLPASS;
            if (types.contains(Lv2World.class_amplifier))
                rdf_descriptor->Type |= LV2_CLASS_AMPLIFIER;
            if (types.contains(Lv2World.class_analyzer))
                rdf_descriptor->Type |= LV2_CLASS_ANALYSER;
            if (types.contains(Lv2World.class_bandpass))
                rdf_descriptor->Type |= LV2_CLASS_BANDPASS;
            if (types.contains(Lv2World.class_chorus))
                rdf_descriptor->Type |= LV2_CLASS_CHORUS;
            if (types.contains(Lv2World.class_comb))
                rdf_descriptor->Type |= LV2_CLASS_COMB;
            if (types.contains(Lv2World.class_compressor))
                rdf_descriptor->Type |= LV2_CLASS_COMPRESSOR;
            if (types.contains(Lv2World.class_constant))
                rdf_descriptor->Type |= LV2_CLASS_CONSTANT;
            if (types.contains(Lv2World.class_converter))
                rdf_descriptor->Type |= LV2_CLASS_CONVERTER;
            if (types.contains(Lv2World.class_delay))
                rdf_descriptor->Type |= LV2_CLASS_DELAY;
            if (types.contains(Lv2World.class_distortion))
                rdf_descriptor->Type |= LV2_CLASS_DISTORTION;
            if (types.contains(Lv2World.class_dynamics))
                rdf_descriptor->Type |= LV2_CLASS_DYNAMICS;
            if (types.contains(Lv2World.class_eq))
                rdf_descriptor->Type |= LV2_CLASS_EQ;
            if (types.contains(Lv2World.class_expander))
                rdf_descriptor->Type |= LV2_CLASS_EXPANDER;
            if (types.contains(Lv2World.class_filter))
                rdf_descriptor->Type |= LV2_CLASS_FILTER;
            if (types.contains(Lv2World.class_flanger))
                rdf_descriptor->Type |= LV2_CLASS_FLANGER;
            if (types.contains(Lv2World.class_function))
                rdf_descriptor->Type |= LV2_CLASS_FUNCTION;
            if (types.contains(Lv2World.class_gate))
                rdf_descriptor->Type |= LV2_CLASS_GATE;
            if (types.contains(Lv2World.class_generator))
                rdf_descriptor->Type |= LV2_CLASS_GENERATOR;
            if (types.contains(Lv2World.class_highpass))
                rdf_descriptor->Type |= LV2_CLASS_HIGHPASS;
            if (types.contains(Lv2World.class_instrument))
                rdf_descriptor->Type |= LV2_CLASS_INSTRUMENT;
            if (types.contains(Lv2World.class_limiter))
                rdf_descriptor->Type |= LV2_CLASS_LIMITER;
            if (types.contains(Lv2World.class_lowpass))
                rdf_descriptor->Type |= LV2_CLASS_LOWPASS;
            if (types.contains(Lv2World.class_mixer))
                rdf_descriptor->Type |= LV2_CLASS_MIXER;
            if (types.contains(Lv2World.class_modulator))
                rdf_descriptor->Type |= LV2_CLASS_MODULATOR;
            if (types.contains(Lv2World.class_multi_eq))
                rdf_descriptor->Type |= LV2_CLASS_MULTI_EQ;
            if (types.contains(Lv2World.class_oscillator))
                rdf_descriptor->Type |= LV2_CLASS_OSCILLATOR;
            if (types.contains(Lv2World.class_para_eq))
                rdf_descriptor->Type |= LV2_CLASS_PARA_EQ;
            if (types.contains(Lv2World.class_phaser))
                rdf_descriptor->Type |= LV2_CLASS_PHASER;
            if (types.contains(Lv2World.class_pitch))
                rdf_descriptor->Type |= LV2_CLASS_PITCH;
            if (types.contains(Lv2World.class_reverb))
                rdf_descriptor->Type |= LV2_CLASS_REVERB;
            if (types.contains(Lv2World.class_simulator))
                rdf_descriptor->Type |= LV2_CLASS_SIMULATOR;
            if (types.contains(Lv2World.class_spatial))
                rdf_descriptor->Type |= LV2_CLASS_SPATIAL;
            if (types.contains(Lv2World.class_spectral))
                rdf_descriptor->Type |= LV2_CLASS_SPECTRAL;
            if (types.contains(Lv2World.class_utility))
                rdf_descriptor->Type |= LV2_CLASS_UTILITY;
            if (types.contains(Lv2World.class_waveshaper))
                rdf_descriptor->Type |= LV2_CLASS_WAVESHAPER;

            // --------------------------------------------------
            // Set Plugin Information

            rdf_descriptor->URI         = strdup(URI);
            rdf_descriptor->Binary      = strdup(lilv_uri_to_path(Plugin.get_library_uri().as_string()));
            rdf_descriptor->Bundle      = strdup(lilv_uri_to_path(Plugin.get_bundle_uri().as_string()));

            if (Plugin.get_name())
                rdf_descriptor->Name    = strdup(Plugin.get_name().as_string());
            else
                rdf_descriptor->Name    = nullptr;

            if (Plugin.get_author_name())
                rdf_descriptor->Author  = strdup(Plugin.get_author_name().as_string());
            else
                rdf_descriptor->Author  = nullptr;

            Lilv::Nodes license = Plugin.get_value(Lv2World.doap_license);

            if (license.size() > 0)
                rdf_descriptor->License = strdup(Lilv::Node(lilv_nodes_get(license, license.begin())).as_string());
            else
                rdf_descriptor->License = nullptr;

            // --------------------------------------------------
            // Set Plugin UniqueID

            rdf_descriptor->UniqueID = 0;

            Lilv::Nodes replaces = Plugin.get_value(Lv2World.dct_replaces);

            if (replaces.size() > 0)
            {
                Lilv::Node replace_value(lilv_nodes_get(replaces, replaces.begin()));

                if (replace_value.is_uri())
                {
                    QString replace_uri(replace_value.as_uri());

                    if (replace_uri.startsWith("urn:"))
                    {
                        QString replace_id = replace_uri.split(":").last();

                        bool ok;
                        int uniqueId = replace_id.toInt(&ok);

                        if (ok && uniqueId > 0)
                            rdf_descriptor->UniqueID = uniqueId;
                    }
                }
            }

            // --------------------------------------------------
            // Set Plugin Ports

            rdf_descriptor->PortCount = Plugin.get_num_ports();

            if (rdf_descriptor->PortCount > 0)
            {
                rdf_descriptor->Ports = new LV2_RDF_Port [rdf_descriptor->PortCount];

                for (uint32_t j = 0; j < rdf_descriptor->PortCount; j++)
                {
                    Lilv::Port Port = Plugin.get_port_by_index(j);
                    LV2_RDF_Port* RDF_Port = &rdf_descriptor->Ports[j];

                    // ------------------------------------------
                    // Set Port Type

                    RDF_Port->Type = 0x0;

                    if (Port.is_a(Lv2World.port_input))
                        RDF_Port->Type |= LV2_PORT_INPUT;

                    if (Port.is_a(Lv2World.port_output))
                        RDF_Port->Type |= LV2_PORT_OUTPUT;

                    if (Port.is_a(Lv2World.port_control))
                        RDF_Port->Type |= LV2_PORT_CONTROL;

                    if (Port.is_a(Lv2World.port_audio))
                        RDF_Port->Type |= LV2_PORT_AUDIO;

                    if (Port.is_a(Lv2World.port_cv))
                        RDF_Port->Type |= LV2_PORT_CV;

                    if (Port.is_a(Lv2World.port_atom))
                    {
                        RDF_Port->Type |= LV2_PORT_ATOM;

                        Lilv::Nodes bufferTypes(Port.get_value(Lv2World.atom_buffer_type));
                        if (bufferTypes.contains(Lv2World.atom_sequence))
                            RDF_Port->Type |= LV2_PORT_ATOM_SEQUENCE;

                        Lilv::Nodes supports(Port.get_value(Lv2World.atom_supports));
                        if (supports.contains(Lv2World.midi_event))
                            RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                        if (supports.contains(Lv2World.patch_message))
                            RDF_Port->Type |= LV2_PORT_SUPPORTS_PATCH_MESSAGE;
                    }

                    if (Port.is_a(Lv2World.port_event))
                    {
                        RDF_Port->Type |= LV2_PORT_EVENT;

                        if (Port.supports_event(Lv2World.midi_event))
                            RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }

                    if (Port.is_a(Lv2World.port_midi_ll))
                    {
                        RDF_Port->Type |= LV2_PORT_MIDI_LL;
                        RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }

                    // ------------------------------------------
                    // Set Port Properties

                    RDF_Port->Properties = 0x0;

                    if (Port.has_property(Lv2World.pprop_optional))
                        RDF_Port->Properties = LV2_PORT_OPTIONAL;
                    if (Port.has_property(Lv2World.pprop_enumeration))
                        RDF_Port->Properties = LV2_PORT_ENUMERATION;
                    if (Port.has_property(Lv2World.pprop_integer))
                        RDF_Port->Properties = LV2_PORT_INTEGER;
                    if (Port.has_property(Lv2World.pprop_sample_rate))
                        RDF_Port->Properties = LV2_PORT_SAMPLE_RATE;
                    if (Port.has_property(Lv2World.pprop_toggled))
                        RDF_Port->Properties = LV2_PORT_TOGGLED;

                    if (Port.has_property(Lv2World.pprop_artifacts))
                        RDF_Port->Properties = LV2_PORT_CAUSES_ARTIFACTS;
                    if (Port.has_property(Lv2World.pprop_continuous_cv))
                        RDF_Port->Properties = LV2_PORT_CONTINUOUS_CV;
                    if (Port.has_property(Lv2World.pprop_discrete_cv))
                        RDF_Port->Properties = LV2_PORT_DISCRETE_CV;
                    if (Port.has_property(Lv2World.pprop_expensive))
                        RDF_Port->Properties = LV2_PORT_EXPENSIVE;
                    if (Port.has_property(Lv2World.pprop_strict_bounds))
                        RDF_Port->Properties = LV2_PORT_STRICT_BOUNDS;
                    if (Port.has_property(Lv2World.pprop_logarithmic))
                        RDF_Port->Properties = LV2_PORT_LOGARITHMIC;
                    if (Port.has_property(Lv2World.pprop_not_automatic))
                        RDF_Port->Properties = LV2_PORT_NOT_AUTOMATIC;
                    if (Port.has_property(Lv2World.pprop_not_on_gui))
                        RDF_Port->Properties = LV2_PORT_NOT_ON_GUI;
                    if (Port.has_property(Lv2World.pprop_trigger))
                        RDF_Port->Properties = LV2_PORT_TRIGGER;

                    // ------------------------------------------
                    // Set Port Designation

                    if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_bar) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BAR;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_barBeat) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BAR_BEAT;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_beat) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BEAT;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_beatUnit) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BEAT_UNIT;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_beatsPerBar) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BEATS_PER_BAR;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_beatsPerMinute) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_BEATS_PER_MINUTE;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_frame) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_FRAME;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_framesPerSecond) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_FRAMES_PER_SECOND;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_position) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_POSITION;
                    else if (lilv_plugin_get_port_by_designation(Plugin.me, nullptr, Lv2World.time_speed) == Port.me)
                        RDF_Port->Designation = LV2_PORT_TIME_SPEED;
                    else
                        RDF_Port->Designation = 0;

                    // ------------------------------------------
                    // Set Port Information

                    RDF_Port->Name   = strdup(Lilv::Node(Port.get_name()).as_string());
                    RDF_Port->Symbol = strdup(Lilv::Node(Port.get_symbol()).as_string());

                    // ------------------------------------------
                    // Set Port MIDI Map

                    RDF_Port->MidiMap.Type   = 0x0;
                    RDF_Port->MidiMap.Number = 0;

#if 0
                    Lilv::Nodes midi_maps = Port.get_value(Lv2World.mm_default_controller);

                    if (midi_maps.size() > 0)
                    {
                        qDebug("-------------------- has midi map");

                        Lilv::Node midi_map_node(lilv_nodes_get(midi_maps, midi_maps.begin()));

                        midi_maps = Port.get_value(midi_map_node);

                        //                            LILV_FOREACH(nodes, j, midi_maps)
                        //                            {
                        //                                Lilv::Node Node = Lilv::Node(lilv_nodes_get(midi_maps, j));

                        //                                if (Node.is_string())
                        //                                    qDebug("-------------------- has midi map -> S %s", Node.as_string());
                        //                                else if (Node.is_int())
                        //                                    qDebug("-------------------- has midi map -> I %i", Node.as_int());
                        //                                else if (Node.is_literal())
                        //                                    qDebug("-------------------- has midi map -> L");
                        //                                else
                        //                                    qDebug("-------------------- has midi map (Unknown)");
                        //                            }

                        //Lilv::Node midi_map_node(lilv_nodes_get(midi_maps, midi_maps.begin()));

                        //Lilv::Nodes midi_map_nodes = Port.get_value(midi_map_node);
                        //if (midi_map_nodes.size() > 0)
                        //   qDebug("-------------------- has midi map +  control type");

                        //if (Lilv::Nodes(Port.get_value(Lv2World.mm_controller_type)).size() > 0)
                        //{

                        //}
                    }
#endif

                    // ------------------------------------------
                    // Set Port Points

                    RDF_Port->Points.Hints   = 0x0;
                    RDF_Port->Points.Default = 0.0f;
                    RDF_Port->Points.Minimum = 0.0f;
                    RDF_Port->Points.Maximum = 1.0f;

                    Lilv::Nodes value = Port.get_value(Lv2World.value_default);

                    if (value.size() > 0)
                    {
                        RDF_Port->Points.Hints  |= LV2_PORT_POINT_DEFAULT;
                        RDF_Port->Points.Default = Lilv::Node(lilv_nodes_get(value, value.begin())).as_float();
                    }

                    value = Port.get_value(Lv2World.value_minimum);

                    if (value.size() > 0)
                    {
                        RDF_Port->Points.Hints  |= LV2_PORT_POINT_MINIMUM;
                        RDF_Port->Points.Minimum = Lilv::Node(lilv_nodes_get(value, value.begin())).as_float();
                    }

                    value = Port.get_value(Lv2World.value_maximum);

                    if (value.size() > 0)
                    {
                        RDF_Port->Points.Hints  |= LV2_PORT_POINT_MAXIMUM;
                        RDF_Port->Points.Maximum = Lilv::Node(lilv_nodes_get(value, value.begin())).as_float();
                    }

                    // ------------------------------------------
                    // Set Port Unit

                    RDF_Port->Unit.Type  = 0x0;
                    RDF_Port->Unit.Hints = 0x0;

                    Lilv::Nodes unit_units = Port.get_value(Lv2World.unit_unit);

                    if (unit_units.size() > 0)
                    {
                        RDF_Port->Unit.Hints |= LV2_PORT_UNIT;

                        if (unit_units.contains(Lv2World.unit_bar))
                            RDF_Port->Unit.Type = LV2_UNIT_BAR;
                        else if (unit_units.contains(Lv2World.unit_beat))
                            RDF_Port->Unit.Type = LV2_UNIT_BEAT;
                        else if (unit_units.contains(Lv2World.unit_bpm))
                            RDF_Port->Unit.Type = LV2_UNIT_BPM;
                        else if (unit_units.contains(Lv2World.unit_cent))
                            RDF_Port->Unit.Type = LV2_UNIT_CENT;
                        else if (unit_units.contains(Lv2World.unit_cm))
                            RDF_Port->Unit.Type = LV2_UNIT_CM;
                        else if (unit_units.contains(Lv2World.unit_coef))
                            RDF_Port->Unit.Type = LV2_UNIT_COEF;
                        else if (unit_units.contains(Lv2World.unit_db))
                            RDF_Port->Unit.Type = LV2_UNIT_DB;
                        else if (unit_units.contains(Lv2World.unit_degree))
                            RDF_Port->Unit.Type = LV2_UNIT_DEGREE;
                        else if (unit_units.contains(Lv2World.unit_frame))
                            RDF_Port->Unit.Type = LV2_UNIT_FRAME;
                        else if (unit_units.contains(Lv2World.unit_hz))
                            RDF_Port->Unit.Type = LV2_UNIT_HZ;
                        else if (unit_units.contains(Lv2World.unit_inch))
                            RDF_Port->Unit.Type = LV2_UNIT_INCH;
                        else if (unit_units.contains(Lv2World.unit_khz))
                            RDF_Port->Unit.Type = LV2_UNIT_KHZ;
                        else if (unit_units.contains(Lv2World.unit_km))
                            RDF_Port->Unit.Type = LV2_UNIT_KM;
                        else if (unit_units.contains(Lv2World.unit_m))
                            RDF_Port->Unit.Type = LV2_UNIT_M;
                        else if (unit_units.contains(Lv2World.unit_mhz))
                            RDF_Port->Unit.Type = LV2_UNIT_MHZ;
                        else if (unit_units.contains(Lv2World.unit_midi_note))
                            RDF_Port->Unit.Type = LV2_UNIT_MIDINOTE;
                        else if (unit_units.contains(Lv2World.unit_mile))
                            RDF_Port->Unit.Type = LV2_UNIT_MILE;
                        else if (unit_units.contains(Lv2World.unit_min))
                            RDF_Port->Unit.Type = LV2_UNIT_MIN;
                        else if (unit_units.contains(Lv2World.unit_mm))
                            RDF_Port->Unit.Type = LV2_UNIT_MM;
                        else if (unit_units.contains(Lv2World.unit_ms))
                            RDF_Port->Unit.Type = LV2_UNIT_MS;
                        else if (unit_units.contains(Lv2World.unit_oct))
                            RDF_Port->Unit.Type = LV2_UNIT_OCT;
                        else if (unit_units.contains(Lv2World.unit_pc))
                            RDF_Port->Unit.Type = LV2_UNIT_PC;
                        else if (unit_units.contains(Lv2World.unit_s))
                            RDF_Port->Unit.Type = LV2_UNIT_S;
                        else if (unit_units.contains(Lv2World.unit_semitone))
                            RDF_Port->Unit.Type = LV2_UNIT_SEMITONE;
                    }

                    Lilv::Nodes unit_name   = Port.get_value(Lv2World.unit_name);
                    Lilv::Nodes unit_render = Port.get_value(Lv2World.unit_render);
                    Lilv::Nodes unit_symbol = Port.get_value(Lv2World.unit_symbol);

                    if (unit_name.size() > 0)
                    {
                        RDF_Port->Unit.Hints |= LV2_PORT_UNIT_NAME;
                        RDF_Port->Unit.Name = strdup(Lilv::Node(lilv_nodes_get(unit_name, unit_name.begin())).as_string());
                    }
                    else
                        RDF_Port->Unit.Name = nullptr;

                    if (unit_render.size() > 0)
                    {
                        RDF_Port->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                        RDF_Port->Unit.Render = strdup(Lilv::Node(lilv_nodes_get(unit_render, unit_render.begin())).as_string());
                    }
                    else
                        RDF_Port->Unit.Render = nullptr;

                    if (unit_symbol.size() > 0)
                    {
                        RDF_Port->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                        RDF_Port->Unit.Symbol = strdup(Lilv::Node(lilv_nodes_get(unit_symbol, unit_symbol.begin())).as_string());
                    }
                    else
                        RDF_Port->Unit.Symbol = nullptr;

                    // ------------------------------------------
                    // Set Port Scale Points

                    Lilv::ScalePoints scalepoints = Port.get_scale_points();

                    RDF_Port->ScalePointCount = scalepoints.size();

                    if (RDF_Port->ScalePointCount > 0)
                    {
                        RDF_Port->ScalePoints = new LV2_RDF_PortScalePoint [RDF_Port->ScalePointCount];

                        uint32_t h = 0;
                        LILV_FOREACH(scale_points, j, scalepoints)
                        {
                            Lilv::ScalePoint ScalePoint = lilv_scale_points_get(scalepoints, j);

                            LV2_RDF_PortScalePoint* RDF_ScalePoint = &RDF_Port->ScalePoints[h++];
                            RDF_ScalePoint->Label = strdup(Lilv::Node(ScalePoint.get_label()).as_string());
                            RDF_ScalePoint->Value = Lilv::Node(ScalePoint.get_value()).as_float();
                        }
                    }
                    else
                        RDF_Port->ScalePoints = nullptr;
                }

                // Set Latency port
                if (Plugin.has_latency())
                {
                    unsigned int index = Plugin.get_latency_port_index();
                    if (index < rdf_descriptor->PortCount)
                        rdf_descriptor->Ports[index].Designation = LV2_PORT_LATENCY;
                }
            }
            else
                rdf_descriptor->Ports = nullptr;

            // --------------------------------------------------
            // Set Plugin Presets

            Lilv::Nodes Presets = Plugin.get_related(Lv2World.preset_preset);

            rdf_descriptor->PresetCount = Presets.size();

            if (rdf_descriptor->PresetCount > 0)
            {
                rdf_descriptor->Presets = new LV2_RDF_Preset [rdf_descriptor->PresetCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, Presets)
                {
                    Lilv::Node Node = Lilv::Node(lilv_nodes_get(Presets, j));
                    Lv2World.load_resource(Node);

                    LV2_RDF_Preset* RDF_Preset = &rdf_descriptor->Presets[h++];

                    // ------------------------------------------
                    // Set Preset Information

                    Lilv::Nodes Label = Lv2World.find_nodes(Node, Lv2World.rdfs_label, nullptr);

                    if (Node.is_uri())
                        RDF_Preset->URI   = strdup(Node.as_uri());
                    else
                        RDF_Preset->URI   = nullptr;

                    if (Label.size() > 0)
                        RDF_Preset->Label = strdup(Lilv::Node(lilv_nodes_get(Label, Label.begin())).as_string());
                    else
                        RDF_Preset->Label = nullptr;

                    // ------------------------------------------
                    // Set Preset Ports

                    Lilv::Nodes PresetPorts = Lv2World.find_nodes(Node, Lv2World.port, nullptr);

                    RDF_Preset->PortCount = PresetPorts.size();

                    if (RDF_Preset->PortCount > 0)
                    {
                        RDF_Preset->Ports = new LV2_RDF_PresetPort[RDF_Preset->PortCount];

                        uint32_t g = 0;
                        LILV_FOREACH(nodes, k, PresetPorts)
                        {
                            Lilv::Node PresetPort = Lilv::Node(lilv_nodes_get(PresetPorts, k));

                            Lilv::Nodes PresetPortSymbol = Lv2World.find_nodes(PresetPort, Lv2World.symbol, nullptr);
                            Lilv::Nodes PresetPortValue  = Lv2World.find_nodes(PresetPort, Lv2World.preset_value, nullptr);

                            LV2_RDF_PresetPort* RDF_PresetPort = &RDF_Preset->Ports[g++];

                            RDF_PresetPort->Symbol = strdup(Lilv::Node(lilv_nodes_get(PresetPortSymbol, PresetPortSymbol.begin())).as_string());
                            RDF_PresetPort->Value  = Lilv::Node(lilv_nodes_get(PresetPortValue, PresetPortValue.begin())).as_float();
                        }
                    }
                    else
                        RDF_Preset->Ports = nullptr;

                    // ------------------------------------------
                    // Set Preset States, TODO

                    RDF_Preset->StateCount = 0;
                }
            }
            else
                rdf_descriptor->Presets = nullptr;

            // --------------------------------------------------
            // Set Plugin Features

            Lilv::Nodes features  = Plugin.get_supported_features();
            Lilv::Nodes featuresR = Plugin.get_required_features();

            rdf_descriptor->FeatureCount = features.size();

            if (rdf_descriptor->FeatureCount > 0)
            {
                rdf_descriptor->Features = new LV2_RDF_Feature [rdf_descriptor->FeatureCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, features)
                {
                    Lilv::Node Node = Lilv::Node(lilv_nodes_get(features, j));

                    LV2_RDF_Feature* RDF_Feature = &rdf_descriptor->Features[h++];
                    RDF_Feature->Type = featuresR.contains(Node) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                    RDF_Feature->URI  = strdup(Node.as_uri());
                }
            }
            else
                rdf_descriptor->Features = nullptr;

            // --------------------------------------------------
            // Set Plugin Extensions

            Lilv::Nodes extensions = Plugin.get_extension_data();

            rdf_descriptor->ExtensionCount = extensions.size();

            if (rdf_descriptor->ExtensionCount > 0)
            {
                rdf_descriptor->Extensions = new LV2_URI [rdf_descriptor->ExtensionCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, extensions)
                {
                    Lilv::Node Node = Lilv::Node(lilv_nodes_get(extensions, j));

                    rdf_descriptor->Extensions[h++] = strdup(Node.as_uri());
                }
            }
            else
                rdf_descriptor->Extensions = nullptr;

            // --------------------------------------------------
            // Set Plugin UIs

            Lilv::UIs uis = Plugin.get_uis();

            rdf_descriptor->UICount = uis.size();

            if (rdf_descriptor->UICount > 0)
            {
                rdf_descriptor->UIs = new LV2_RDF_UI [rdf_descriptor->UICount];

                uint32_t h = 0;
                LILV_FOREACH(uis, j, uis)
                {
                    Lilv::UI UI = lilv_uis_get(uis, j);

                    LV2_RDF_UI* RDF_UI = &rdf_descriptor->UIs[h++];

                    // ------------------------------------------
                    // Set UI Type

                    if (UI.is_a(Lv2World.ui_gtk2))
                        RDF_UI->Type = LV2_UI_GTK2;
                    else if (UI.is_a(Lv2World.ui_qt4))
                        RDF_UI->Type = LV2_UI_QT4;
                    else if (UI.is_a(Lv2World.ui_x11))
                        RDF_UI->Type = LV2_UI_X11;
                    else if (UI.is_a(Lv2World.ui_external))
                        RDF_UI->Type = LV2_UI_EXTERNAL;
                    else if (UI.is_a(Lv2World.ui_external_old))
                        RDF_UI->Type = LV2_UI_OLD_EXTERNAL;
                    else
                        RDF_UI->Type = 0;

                    // ------------------------------------------
                    // Set UI Information

                    RDF_UI->URI    = strdup(UI.get_uri().as_uri());
                    RDF_UI->Binary = strdup(lilv_uri_to_path(UI.get_binary_uri().as_string()));
                    RDF_UI->Bundle = strdup(lilv_uri_to_path(UI.get_bundle_uri().as_string()));

                    // ------------------------------------------
                    // Set UI Features

                    Lilv::Nodes features  = UI.get_supported_features();
                    Lilv::Nodes featuresR = UI.get_required_features();

                    RDF_UI->FeatureCount = features.size();

                    if (RDF_UI->FeatureCount > 0)
                    {
                        RDF_UI->Features = new LV2_RDF_Feature [RDF_UI->FeatureCount];

                        uint32_t h = 0;
                        LILV_FOREACH(nodes, k, features)
                        {
                            Lilv::Node Node = Lilv::Node(lilv_nodes_get(features, k));

                            LV2_RDF_Feature* RDF_Feature = &RDF_UI->Features[h++];
                            RDF_Feature->Type = featuresR.contains(Node) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                            RDF_Feature->URI  = strdup(Node.as_uri());
                        }
                    }
                    else
                        RDF_UI->Features = nullptr;

                    // ------------------------------------------
                    // Set UI Extensions

                    Lilv::Nodes extensions = UI.get_extension_data();

                    RDF_UI->ExtensionCount = extensions.size();

                    if (RDF_UI->ExtensionCount > 0)
                    {
                        RDF_UI->Extensions = new LV2_URI [RDF_UI->ExtensionCount];

                        uint32_t h = 0;
                        LILV_FOREACH(nodes, k, extensions)
                        {
                            Lilv::Node Node = Lilv::Node(lilv_nodes_get(extensions, k));

                            RDF_UI->Extensions[h++] = strdup(Node.as_uri());
                        }
                    }
                    else
                        RDF_UI->Extensions = nullptr;
                }
            }
            else
                rdf_descriptor->UIs = nullptr;

            return rdf_descriptor;
        }
    }

    return nullptr;
}

// Copy RDF object
inline const LV2_RDF_Descriptor* lv2_rdf_dup(LV2_RDF_Descriptor* rdf_descriptor)
{
    uint32_t i, j;
    LV2_RDF_Descriptor* new_descriptor = new LV2_RDF_Descriptor;

    new_descriptor->Type           = rdf_descriptor->Type;
    new_descriptor->UniqueID       = rdf_descriptor->UniqueID;

    new_descriptor->PortCount      = rdf_descriptor->PortCount;
    new_descriptor->PresetCount    = rdf_descriptor->PresetCount;
    new_descriptor->FeatureCount   = rdf_descriptor->FeatureCount;
    new_descriptor->ExtensionCount = rdf_descriptor->ExtensionCount;
    new_descriptor->UICount        = rdf_descriptor->UICount;

    new_descriptor->URI            = strdup(rdf_descriptor->URI);
    new_descriptor->Name           = strdup(rdf_descriptor->Name);
    new_descriptor->Author         = strdup(rdf_descriptor->Author);
    new_descriptor->License        = strdup(rdf_descriptor->License);
    new_descriptor->Binary         = strdup(rdf_descriptor->Binary);
    new_descriptor->Bundle         = strdup(rdf_descriptor->Bundle);

    // Ports
    if (new_descriptor->PortCount > 0)
    {
        new_descriptor->Ports = new LV2_RDF_Port[new_descriptor->PortCount];

        for (i=0; i < new_descriptor->PortCount; i++)
        {
            LV2_RDF_Port* Port = &new_descriptor->Ports[i];

            Port->Type            = rdf_descriptor->Ports[i].Type;
            Port->Properties      = rdf_descriptor->Ports[i].Properties;

            Port->MidiMap.Type    = rdf_descriptor->Ports[i].MidiMap.Type;
            Port->MidiMap.Number  = rdf_descriptor->Ports[i].MidiMap.Number;

            Port->Points.Hints    = rdf_descriptor->Ports[i].Points.Hints;
            Port->Points.Default  = rdf_descriptor->Ports[i].Points.Default;
            Port->Points.Minimum  = rdf_descriptor->Ports[i].Points.Minimum;
            Port->Points.Maximum  = rdf_descriptor->Ports[i].Points.Maximum;

            Port->Unit.Type       = rdf_descriptor->Ports[i].Unit.Type;
            Port->Unit.Hints      = rdf_descriptor->Ports[i].Unit.Hints;

            Port->ScalePointCount = rdf_descriptor->Ports[i].ScalePointCount;

            Port->Name            = strdup(rdf_descriptor->Ports[i].Name);
            Port->Symbol          = strdup(rdf_descriptor->Ports[i].Symbol);

            if (rdf_descriptor->Ports[i].Unit.Name)
                Port->Unit.Name   = strdup(rdf_descriptor->Ports[i].Unit.Name);
            else
                Port->Unit.Name   = nullptr;

            if (rdf_descriptor->Ports[i].Unit.Render)
                Port->Unit.Render = strdup(rdf_descriptor->Ports[i].Unit.Render);
            else
                Port->Unit.Render = nullptr;

            if (rdf_descriptor->Ports[i].Unit.Symbol)
                Port->Unit.Symbol = strdup(rdf_descriptor->Ports[i].Unit.Symbol);
            else
                Port->Unit.Symbol = nullptr;

            if (Port->ScalePointCount > 0)
            {
                Port->ScalePoints = new LV2_RDF_PortScalePoint[Port->ScalePointCount];

                for (j=0; j < Port->ScalePointCount; j++)
                {
                    Port->ScalePoints[j].Value = rdf_descriptor->Ports[i].ScalePoints[j].Value;
                    Port->ScalePoints[j].Label = strdup(rdf_descriptor->Ports[i].ScalePoints[j].Label);
                }
            }
            else
                Port->ScalePoints = nullptr;
        }
    }
    else
        new_descriptor->Ports = nullptr;

    // Presets
    if (new_descriptor->PresetCount > 0)
    {
        new_descriptor->Presets = new LV2_RDF_Preset[new_descriptor->PresetCount];

        for (i=0; i < new_descriptor->PresetCount; i++)
        {
            LV2_RDF_Preset* Preset = &new_descriptor->Presets[i];

            Preset->PortCount  = rdf_descriptor->Presets[i].PortCount;
            Preset->StateCount = rdf_descriptor->Presets[i].StateCount;

            Preset->URI        = strdup(rdf_descriptor->Presets[i].URI);
            Preset->Label      = strdup(rdf_descriptor->Presets[i].Label);

            // Ports
            if (Preset->PortCount > 0)
            {
                Preset->Ports = new LV2_RDF_PresetPort[Preset->PortCount];

                for (j=0; j < Preset->PortCount; j++)
                {
                    Preset->Ports[j].Value  = rdf_descriptor->Presets[i].Ports[j].Value;
                    Preset->Ports[j].Symbol = strdup(rdf_descriptor->Presets[i].Ports[j].Symbol);
                }
            }
            else
                Preset->Ports = nullptr;

            // States
            if (Preset->StateCount > 0)
            {
                Preset->States = new LV2_RDF_PresetState[Preset->StateCount];

                for (j=0; j < Preset->StateCount; j++)
                {
                    Preset->States[j].Type  = rdf_descriptor->Presets[i].States[j].Type;
                    Preset->States[j].Key   = strdup(rdf_descriptor->Presets[i].States[j].Key);

                    switch (Preset->States[j].Type)
                    {
                    case LV2_PRESET_STATE_BOOL:
                        Preset->States[j].Value.b = rdf_descriptor->Presets[i].States[j].Value.b;
                        break;
                    case LV2_PRESET_STATE_INT:
                        Preset->States[j].Value.i = rdf_descriptor->Presets[i].States[j].Value.i;
                        break;
                    case LV2_PRESET_STATE_LONG:
                        Preset->States[j].Value.li = rdf_descriptor->Presets[i].States[j].Value.li;
                        break;
                    case LV2_PRESET_STATE_FLOAT:
                        Preset->States[j].Value.f = rdf_descriptor->Presets[i].States[j].Value.f;
                        break;
                    case LV2_PRESET_STATE_STRING:
                    case LV2_PRESET_STATE_BINARY:
                        Preset->States[j].Value.s = strdup(rdf_descriptor->Presets[i].States[j].Value.s);
                        break;
                    default:
                        // Invalid type
                        Preset->States[j].Type = LV2_PRESET_STATE_NULL;
                        break;
                    }
                }
            }
            else
                Preset->States = nullptr;
        }
    }
    else
        new_descriptor->Presets = nullptr;

    // Features
    if (new_descriptor->FeatureCount > 0)
    {
        new_descriptor->Features = new LV2_RDF_Feature[new_descriptor->FeatureCount];

        for (i=0; i < new_descriptor->FeatureCount; i++)
        {
            new_descriptor->Features[i].Type = rdf_descriptor->Features[i].Type;
            new_descriptor->Features[i].URI  = strdup(rdf_descriptor->Features[i].URI);
        }
    }
    else
        new_descriptor->Features = nullptr;

    // Extensions
    if (new_descriptor->ExtensionCount > 0)
    {
        new_descriptor->Extensions = new LV2_URI[new_descriptor->ExtensionCount];

        for (i=0; i < new_descriptor->ExtensionCount; i++)
        {
            new_descriptor->Extensions[i] = strdup(rdf_descriptor->Extensions[i]);
        }
    }
    else
        new_descriptor->Extensions = nullptr;

    // UIs
    if (new_descriptor->UICount > 0)
    {
        new_descriptor->UIs = new LV2_RDF_UI[new_descriptor->UICount];

        for (i=0; i < new_descriptor->UICount; i++)
        {
            LV2_RDF_UI* UI = &new_descriptor->UIs[i];

            UI->Type           = rdf_descriptor->UIs[i].Type;

            UI->FeatureCount   = rdf_descriptor->UIs[i].FeatureCount;
            UI->ExtensionCount = rdf_descriptor->UIs[i].ExtensionCount;

            UI->URI            = strdup(rdf_descriptor->UIs[i].URI);
            UI->Binary         = strdup(rdf_descriptor->UIs[i].Binary);
            UI->Bundle         = strdup(rdf_descriptor->UIs[i].Bundle);

            // UI Features
            if (UI->FeatureCount > 0)
            {
                UI->Features = new LV2_RDF_Feature[UI->FeatureCount];

                for (j=0; j < UI->FeatureCount; j++)
                {
                    UI->Features[j].Type = rdf_descriptor->UIs[i].Features[j].Type;
                    UI->Features[j].URI  = strdup(rdf_descriptor->UIs[i].Features[j].URI);
                }
            }
            else
                UI->Features = nullptr;

            // UI Extensions
            if (UI->ExtensionCount > 0)
            {
                UI->Extensions = new LV2_URI[UI->ExtensionCount];

                for (j=0; j < UI->ExtensionCount; j++)
                {
                    UI->Extensions[j] = strdup(rdf_descriptor->UIs[i].Extensions[j]);
                }
            }
            else
                UI->Extensions = nullptr;
        }
    }
    else
        new_descriptor->UIs = 0;

    return new_descriptor;
}

// Delete object
inline void lv2_rdf_free(const LV2_RDF_Descriptor* rdf_descriptor)
{
    uint32_t i, j;

    free((void*)rdf_descriptor->URI);
    free((void*)rdf_descriptor->Name);
    free((void*)rdf_descriptor->Author);
    free((void*)rdf_descriptor->License);
    free((void*)rdf_descriptor->Binary);
    free((void*)rdf_descriptor->Bundle);

    if (rdf_descriptor->PortCount > 0)
    {
        for (i=0; i < rdf_descriptor->PortCount; i++)
        {
            LV2_RDF_Port* Port = &rdf_descriptor->Ports[i];

            free((void*)Port->Name);
            free((void*)Port->Symbol);

            if (Port->Unit.Name)
                free((void*)Port->Unit.Name);

            if (Port->Unit.Render)
                free((void*)Port->Unit.Render);

            if (Port->Unit.Symbol)
                free((void*)Port->Unit.Symbol);

            if (Port->ScalePointCount > 0)
            {
                for (j=0; j < Port->ScalePointCount; j++)
                    free((void*)Port->ScalePoints[j].Label);

                delete[] Port->ScalePoints;
            }
        }
        delete[] rdf_descriptor->Ports;
    }

    if (rdf_descriptor->PresetCount > 0)
    {
        for (i=0; i < rdf_descriptor->PresetCount; i++)
        {
            LV2_RDF_Preset* Preset = &rdf_descriptor->Presets[i];

            free((void*)Preset->URI);
            free((void*)Preset->Label);

            for (j=0; j < Preset->PortCount; j++)
            {
                if (Preset->Ports[j].Symbol)
                    free((void*)Preset->Ports[j].Symbol);
            }

            for (j=0; j < Preset->StateCount; j++)
            {
                free((void*)Preset->States[j].Key);

                if (Preset->States[j].Type == LV2_PRESET_STATE_STRING || Preset->States[j].Type == LV2_PRESET_STATE_BINARY)
                    free((void*)Preset->States[j].Value.s);
            }
        }
        delete[] rdf_descriptor->Presets;
    }

    if (rdf_descriptor->FeatureCount > 0)
    {
        for (i=0; i < rdf_descriptor->FeatureCount; i++)
            free((void*)rdf_descriptor->Features[i].URI);

        delete[] rdf_descriptor->Features;
    }

    if (rdf_descriptor->ExtensionCount > 0)
    {
        for (i=0; i < rdf_descriptor->ExtensionCount; i++)
            free((void*)rdf_descriptor->Extensions[i]);

        delete[] rdf_descriptor->Extensions;
    }

    if (rdf_descriptor->UICount > 0)
    {
        for (i=0; i < rdf_descriptor->UICount; i++)
        {
            LV2_RDF_UI* UI = &rdf_descriptor->UIs[i];

            free((void*)UI->URI);
            free((void*)UI->Binary);
            free((void*)UI->Bundle);

            if (UI->FeatureCount > 0)
            {
                for (j=0; j < UI->FeatureCount; j++)
                    free((void*)UI->Features[j].URI);

                delete[] UI->Features;
            }

            if (UI->ExtensionCount > 0)
            {
                for (j=0; j < UI->ExtensionCount; j++)
                    free((void*)UI->Extensions[j]);

                delete[] UI->Extensions;
            }
        }
        delete[] rdf_descriptor->UIs;
    }

    delete rdf_descriptor;
}

// ------------------------------------------------------------------------------------------------

inline bool is_lv2_feature_supported(const char* uri)
{
    if (strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    if (strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    if (strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    if (strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    if (strcmp(uri, LV2_LOG__log) == 0)
        return true;
    if (strcmp(uri, LV2_PROGRAMS__Host) == 0)
        return true;
    if (strcmp(uri, LV2_RTSAFE_MEMORY_POOL_URI) == 0)
        return true;
    if (strcmp(uri, LV2_STATE__makePath) == 0)
        return true;
    if (strcmp(uri, LV2_STATE__mapPath) == 0)
        return true;
    if (strcmp(uri, LV2_PORT_PROPS__supportsStrictBounds) == 0)
        return true;
    if (strcmp(uri, LV2_URI_MAP_URI) == 0)
        return true;
    if (strcmp(uri, LV2_URID__map) == 0)
        return true;
    if (strcmp(uri, LV2_URID__unmap) == 0)
        return true;
    if (strcmp(uri, LV2_WORKER__schedule) == 0)
        return true;
    return false;
}

inline bool is_lv2_ui_feature_supported(const char* uri)
{
    if (is_lv2_feature_supported(uri))
        return true;
    if (strcmp(uri, LV2_DATA_ACCESS_URI) == 0)
        return true;
    if (strcmp(uri, LV2_INSTANCE_ACCESS_URI) == 0)
        return true;
    if (strcmp(uri, LV2_UI__noUserResize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__fixedSize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__parent) == 0)
        return true;
    if (strcmp(uri, LV2_UI__portMap) == 0)
        return true;
    if (strcmp(uri, LV2_UI__portSubscribe) == 0)
        return false; // TODO
    if (strcmp(uri, LV2_UI__resize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__touch) == 0)
        return false; // TODO
    if (strcmp(uri, LV2_UI_PREFIX "makeResident") == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_URI) == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    return false;
}

inline const char* lv2_get_ui_uri(int UiType)
{
    switch (UiType)
    {
    case LV2_UI_GTK2:
        return LV2_UI__GtkUI;
    case LV2_UI_QT4:
        return LV2_UI__Qt4UI;
    case LV2_UI_X11:
        return LV2_UI__X11UI;
    case LV2_UI_EXTERNAL:
        return LV2_EXTERNAL_UI_URI;
    case LV2_UI_OLD_EXTERNAL:
        return LV2_EXTERNAL_UI_DEPRECATED_URI;
    default:
        return "UI URI Type Not Supported in LV2_RDF";
    }
}

#endif /* LV2_RDF_INCLUDED */

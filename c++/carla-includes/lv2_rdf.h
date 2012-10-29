/*
 * Custom types to store LV2 information
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

// TODO - presets

#include <cstdint>

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

// Port Point Hints
#define LV2_PORT_POINT_DEFAULT           0x1
#define LV2_PORT_POINT_MINIMUM           0x2
#define LV2_PORT_POINT_MAXIMUM           0x4

#define LV2_HAVE_DEFAULT_PORT_POINT(x)   ((x) & LV2_PORT_POINT_DEFAULT)
#define LV2_HAVE_MINIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MINIMUM)
#define LV2_HAVE_MAXIMUM_PORT_POINT(x)   ((x) & LV2_PORT_POINT_MAXIMUM)

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
#define LV2_PORT_DESIGNATION_FREEWHEELING            0x1
#define LV2_PORT_DESIGNATION_LATENCY                 0x2
#define LV2_PORT_DESIGNATION_SAMPLE_RATE             0x3
#define LV2_PORT_DESIGNATION_TIME_BAR                0x4
#define LV2_PORT_DESIGNATION_TIME_BAR_BEAT           0x5
#define LV2_PORT_DESIGNATION_TIME_BEAT               0x6
#define LV2_PORT_DESIGNATION_TIME_BEAT_UNIT          0x7
#define LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR      0x8
#define LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE   0x9
#define LV2_PORT_DESIGNATION_TIME_FRAME              0xA
#define LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND  0xB
#define LV2_PORT_DESIGNATION_TIME_POSITION           0xC
#define LV2_PORT_DESIGNATION_TIME_SPEED              0xD

#define LV2_IS_PORT_DESIGNATION_FREEWHEELING(x)      ((x) == LV2_PORT_DESIGNATION_FREEWHEELING)
#define LV2_IS_PORT_DESIGNATION_LATENCY(x)           ((x) == LV2_PORT_DESIGNATION_LATENCY)
#define LV2_IS_PORT_DESIGNATION_SAMPLE_RATE(x)       ((x) == LV2_PORT_DESIGNATION_SAMPLE_RATE)
#define LV2_IS_PORT_DESIGNATION_TIME_BAR(x)          ((x) == LV2_PORT_DESIGNATION_TIME_BAR)
#define LV2_IS_PORT_DESIGNATION_TIME_BAR_BEAT(x)     ((x) == LV2_PORT_DESIGNATION_TIME_BAR_BEAT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEAT(x)         ((x) == LV2_PORT_DESIGNATION_TIME_BEAT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEAT_UNIT(x)    ((x) == LV2_PORT_DESIGNATION_TIME_BEAT_UNIT)
#define LV2_IS_PORT_DESIGNATION_TIME_BEATS_PER_BAR(x)     ((x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR)
#define LV2_IS_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE(x)  ((x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE)
#define LV2_IS_PORT_DESIGNATION_TIME_FRAME(x)             ((x) == LV2_PORT_DESIGNATION_TIME_FRAME)
#define LV2_IS_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND(x) ((x) == LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND)
#define LV2_IS_PORT_DESIGNATION_TIME_POSITION(x)          ((x) == LV2_PORT_DESIGNATION_TIME_POSITION)
#define LV2_IS_PORT_DESIGNATION_TIME_SPEED(x)             ((x) == LV2_PORT_DESIGNATION_TIME_SPEED)
#define LV2_IS_PORT_DESIGNATION_TIME(x)                   ((x) == LV2_PORT_DESIGNATION_TIME_BAR || (x) == LV2_PORT_DESIGNATION_TIME_BAR_BEAT ||                   \
                                                           (x) == LV2_PORT_DESIGNATION_TIME_BEAT || (x) == LV2_PORT_DESIGNATION_TIME_BEAT_UNIT ||                 \
                                                           (x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_BAR || (x) == LV2_PORT_DESIGNATION_TIME_BEATS_PER_MINUTE || \
                                                           (x) == LV2_PORT_DESIGNATION_TIME_FRAME || (x) == LV2_PORT_DESIGNATION_TIME_FRAMES_PER_SECOND ||        \
                                                           (x) == LV2_PORT_DESIGNATION_TIME_POSITION || (x) == LV2_PORT_DESIGNATION_TIME_SPEED)

// Preset State Types (TODO: Null is not a type, this is just a placeholder)
#define LV2_PRESET_STATE_NULL            0x0

#define LV2_IS_PRESET_STATE_NULL(x)      ((x) == LV2_PRESET_STATE_NULL)

// Feature Types
#define LV2_FEATURE_OPTIONAL             0x1
#define LV2_FEATURE_REQUIRED             0x2

#define LV2_IS_FEATURE_OPTIONAL(x)       ((x) == LV2_FEATURE_OPTIONAL)
#define LV2_IS_FEATURE_REQUIRED(x)       ((x) == LV2_FEATURE_REQUIRED)

// UI Types
#define LV2_UI_GTK2                      0x1
#define LV2_UI_GTK3                      0x2
#define LV2_UI_QT4                       0x3
#define LV2_UI_COCOA                     0x4
#define LV2_UI_WINDOWS                   0x5
#define LV2_UI_X11                       0x6
#define LV2_UI_EXTERNAL                  0x7
#define LV2_UI_OLD_EXTERNAL              0x8

#define LV2_IS_UI_GTK2(x)                ((x) == LV2_UI_GTK2)
#define LV2_IS_UI_GTK3(x)                ((x) == LV2_UI_GTK3)
#define LV2_IS_UI_QT4(x)                 ((x) == LV2_UI_QT4)
#define LV2_IS_UI_COCOA(x)               ((x) == LV2_UI_COCOA)
#define LV2_IS_UI_WINDOWS(x)             ((x) == LV2_UI_WINDOWS)
#define LV2_IS_UI_X11(x)                 ((x) == LV2_UI_X11)
#define LV2_IS_UI_EXTERNAL(x)            ((x) == LV2_UI_EXTERNAL)
#define LV2_IS_UI_OLD_EXTERNAL(x)        ((x) == LV2_UI_OLD_EXTERNAL)

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

// Port Midi Map
struct LV2_RDF_PortMidiMap {
    LV2_Property Type;
    uint32_t Number;

    LV2_RDF_PortMidiMap()
        : Type(0),
          Number(0) {}
};

// Port Points
struct LV2_RDF_PortPoints {
    LV2_Property Hints;
    LV2_Data Default;
    LV2_Data Minimum;
    LV2_Data Maximum;

    LV2_RDF_PortPoints()
        : Hints(0x0),
          Default(0.0f),
          Minimum(0.0f),
          Maximum(1.0f) {}
};

// Port Unit
struct LV2_RDF_PortUnit {
    LV2_Property Type;
    LV2_Property Hints;
    const char* Name;
    const char* Render;
    const char* Symbol;

    LV2_RDF_PortUnit()
        : Type(0),
          Hints(0x0),
          Name(nullptr),
          Render(nullptr),
          Symbol(nullptr) {}

    ~LV2_RDF_PortUnit()
    {
        if (Name)
            free((void*)Name);

        if (Render)
            free((void*)Render);

        if (Symbol)
            free((void*)Symbol);
    }
};

// Port Scale Point
struct LV2_RDF_PortScalePoint {
    const char* Label;
    LV2_Data Value;

    LV2_RDF_PortScalePoint()
        : Label(nullptr),
          Value(0.0f) {}

    ~LV2_RDF_PortScalePoint()
    {
        if (Label)
            free((void*)Label);
    }
};

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

    LV2_RDF_Port()
        : Type(0x0),
          Properties(0x0),
          Designation(0),
          Name(nullptr),
          Symbol(nullptr),
          ScalePointCount(0),
          ScalePoints(nullptr) {}

    ~LV2_RDF_Port()
    {
        if (Name)
            free((void*)Name);

        if (Symbol)
            free((void*)Symbol);

        if (ScalePoints)
            delete[] ScalePoints;
    }
};

// Preset Port
struct LV2_RDF_PresetPort {
    const char* Symbol;
    LV2_Data Value;

    LV2_RDF_PresetPort()
        : Symbol(nullptr),
          Value(0.0f) {}

    ~LV2_RDF_PresetPort()
    {
        if (Symbol)
            free((void*)Symbol);
    }
};

// Preset State
struct LV2_RDF_PresetState {
    LV2_Property Type;
    const char* Key;
    union {
        // TODO
    } Value;

    LV2_RDF_PresetState()
        : Type(0),
          Key(nullptr) {}

    ~LV2_RDF_PresetState()
    {
        if (Key)
            free((void*)Key);
    }
};

// Preset
struct LV2_RDF_Preset {
    LV2_URI URI;
    const char* Label;

    uint32_t PortCount;
    LV2_RDF_PresetPort* Ports;

    uint32_t StateCount;
    LV2_RDF_PresetState* States;

    LV2_RDF_Preset()
        : URI(nullptr),
          Label(nullptr),
          PortCount(0),
          Ports(nullptr),
          StateCount(0),
          States(nullptr) {}

    ~LV2_RDF_Preset()
    {
        if (URI)
            free((void*)URI);

        if (Label)
            free((void*)Label);

        if (Ports)
            delete[] Ports;

        if (States)
            delete[] States;
    }
};

// Feature
struct LV2_RDF_Feature {
    LV2_Property Type;
    LV2_URI URI;

    LV2_RDF_Feature()
        : Type(0),
          URI(nullptr) {}

    ~LV2_RDF_Feature()
    {
        if (URI)
            free((void*)URI);
    }
};

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

    LV2_RDF_UI()
        : Type(0),
          URI(nullptr),
          Binary(nullptr),
          Bundle(nullptr),
          FeatureCount(0),
          Features(nullptr),
          ExtensionCount(0),
          Extensions(nullptr) {}

    ~LV2_RDF_UI()
    {
        if (URI)
            free((void*)URI);

        if (Binary)
            free((void*)Binary);

        if (Bundle)
            free((void*)Bundle);

        if (Features)
            delete[] Features;

        if (Extensions)
            delete[] Extensions;
    }
};

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

    LV2_RDF_Descriptor()
        : Type(0x0),
          URI(nullptr),
          Name(nullptr),
          Author(nullptr),
          License(nullptr),
          Binary(nullptr),
          Bundle(nullptr),
          UniqueID(0),
          PortCount(0),
          Ports(nullptr),
          PresetCount(0),
          Presets(nullptr),
          FeatureCount(0),
          Features(nullptr),
          ExtensionCount(0),
          Extensions(nullptr),
          UICount(0),
          UIs(nullptr) {}

    ~LV2_RDF_Descriptor()
    {
        if (URI)
            free((void*)URI);

        if (Name)
            free((void*)Name);

        if (Author)
            free((void*)Author);

        if (License)
            free((void*)License);

        if (Binary)
            free((void*)Binary);

        if (Bundle)
            free((void*)Bundle);

        if (Ports)
            delete[] Ports;

        if (Presets)
            delete[] Presets;

        if (Features)
            delete[] Features;

        if (Extensions)
            delete[] Extensions;

        if (UIs)
            delete[] UIs;
    }
};

#endif /* LV2_RDF_INCLUDED */

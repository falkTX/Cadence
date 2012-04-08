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

#include <stdint.h>

// Base Types
typedef float LV2_Data;
typedef const char* LV2_URI;
typedef uint32_t LV2_Property;
typedef unsigned long long LV2_PluginType;

// Port MIDI Map Types
#define LV2_PORT_MIDI_MAP_CC             0x1
#define LV2_PORT_MIDI_MAP_NRPN           0x2

#define LV2_IS_PORT_MIDI_MAP_CC(x)       ((x) == LV2_PORT_MIDI_MAP_CC)
#define LV2_IS_PORT_MIDI_MAP_NRPN(x)     ((x) == LV2_PORT_MIDI_MAP_NRPN)

// A Port Midi Map
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
#define LV2_UNIT_HZ                      0x09
#define LV2_UNIT_INCH                    0x0A
#define LV2_UNIT_KHZ                     0x0B
#define LV2_UNIT_KM                      0x0C
#define LV2_UNIT_M                       0x0D
#define LV2_UNIT_MHZ                     0x0E
#define LV2_UNIT_MIDINOTE                0x0F
#define LV2_UNIT_MILE                    0x10
#define LV2_UNIT_MIN                     0x11
#define LV2_UNIT_MM                      0x12
#define LV2_UNIT_MS                      0x13
#define LV2_UNIT_OCT                     0x14
#define LV2_UNIT_PC                      0x15
#define LV2_UNIT_S                       0x16
#define LV2_UNIT_SEMITONE                0x17

#define LV2_IS_UNIT_BAR(x)               ((x) == LV2_UNIT_BAR)
#define LV2_IS_UNIT_BEAT(x)              ((x) == LV2_UNIT_BEAT)
#define LV2_IS_UNIT_BPM(x)               ((x) == LV2_UNIT_BPM)
#define LV2_IS_UNIT_CENT(x)              ((x) == LV2_UNIT_CENT)
#define LV2_IS_UNIT_CM(x)                ((x) == LV2_UNIT_CM)
#define LV2_IS_UNIT_COEF(x)              ((x) == LV2_UNIT_COEF)
#define LV2_IS_UNIT_DB(x)                ((x) == LV2_UNIT_DB)
#define LV2_IS_UNIT_DEGREE(x)            ((x) == LV2_UNIT_DEGREE)
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

// A Port Unit
struct LV2_RDF_PortUnit {
    LV2_Property Type;
    LV2_Property Hints;
    const char* Name;
    const char* Render;
    const char* Symbol;
};

// A Port Scale Point
struct LV2_RDF_PortScalePoint {
    const char* Label;
    LV2_Data Value;
};

// Port Types
#define LV2_PORT_INPUT                   0x01
#define LV2_PORT_OUTPUT                  0x02
#define LV2_PORT_CONTROL                 0x04
#define LV2_PORT_AUDIO                   0x08
#define LV2_PORT_ATOM_MESSAGE            0x10
#define LV2_PORT_ATOM_VALUE              0x20
#define LV2_PORT_CV                      0x40
#define LV2_PORT_EVENT                   0x80
#define LV2_PORT_MIDI_LL                 0x100

// TODO - Port Atom types

// Port Event types
#define LV2_PORT_EVENT_MIDI              0x1000
#define LV2_PORT_EVENT_TIME              0x2000

#define LV2_IS_PORT_INPUT(x)             ((x) & LV2_PORT_INPUT)
#define LV2_IS_PORT_OUTPUT(x)            ((x) & LV2_PORT_OUTPUT)
#define LV2_IS_PORT_CONTROL(x)           ((x) & LV2_PORT_CONTROL)
#define LV2_IS_PORT_AUDIO(x)             ((x) & LV2_PORT_AUDIO)
#define LV2_IS_PORT_ATOM_MESSAGE(x)      ((x) & LV2_PORT_ATOM_MESSAGE)
#define LV2_IS_PORT_ATOM_VALUE(x)        ((x) & LV2_PORT_ATOM_VALUE)
#define LV2_IS_PORT_CV(x)                ((x) & LV2_PORT_CV)
#define LV2_IS_PORT_EVENT(x)             ((x) & LV2_PORT_EVENT)
#define LV2_IS_PORT_MIDI_LL(x)           ((x) & LV2_PORT_MIDI_LL)
#define LV2_IS_PORT_EVENT_MIDI(x)        ((x) & LV2_PORT_EVENT_MIDI)
#define LV2_IS_PORT_EVENT_TIME(x)        ((x) & LV2_PORT_EVENT_TIME)

// Port Properties
#define LV2_PORT_OPTIONAL                0x00001
#define LV2_PORT_LATENCY                 0x00002
#define LV2_PORT_TOGGLED                 0x00004
#define LV2_PORT_SAMPLE_RATE             0x00008
#define LV2_PORT_INTEGER                 0x00010
#define LV2_PORT_ENUMERATION             0x00020
#define LV2_PORT_CAUSES_ARTIFACTS        0x00040
#define LV2_PORT_CONTINUOUS_CV           0x00080
#define LV2_PORT_DISCRETE_CV             0x00100
#define LV2_PORT_EXPENSIVE               0x00200
#define LV2_PORT_HAS_STRICT_BOUNDS       0x00400
#define LV2_PORT_LOGARITHMIC             0x00800
#define LV2_PORT_NOT_AUTOMATIC           0x01000
#define LV2_PORT_NOT_ON_GUI              0x02000
#define LV2_PORT_REPORTS_BEATS_PER_BAR   0x04000
#define LV2_PORT_REPORTS_BEAT_UNIT       0x08000
#define LV2_PORT_REPORTS_BPM             0x10000
#define LV2_PORT_TRIGGER                 0x20000

#define LV2_IS_PORT_OPTIONAL(x)          ((x) & LV2_PORT_OPTIONAL)
#define LV2_IS_PORT_LATENCY(x)           ((x) & LV2_PORT_LATENCY)
#define LV2_IS_PORT_TOGGLED(x)           ((x) & LV2_PORT_TOGGLED)
#define LV2_IS_PORT_SAMPLE_RATE(x)       ((x) & LV2_PORT_SAMPLE_RATE)
#define LV2_IS_PORT_INTEGER(x)           ((x) & LV2_PORT_INTEGER)
#define LV2_IS_PORT_ENUMERATION(x)       ((x) & LV2_PORT_ENUMERATION)
#define LV2_IS_PORT_CAUSES_ARTIFACTS(x)  ((x) & LV2_PORT_CAUSES_ARTIFACTS)
#define LV2_IS_PORT_CONTINUOUS_CV(x)     ((x) & LV2_PORT_CONTINUOUS_CV)
#define LV2_IS_PORT_DISCRETE_CV(x)       ((x) & LV2_PORT_DISCRETE_CV)
#define LV2_IS_PORT_EXPENSIVE(x)         ((x) & LV2_PORT_EXPENSIVE)
#define LV2_IS_PORT_HAS_STRICT_BOUNDS(x) ((x) & LV2_PORT_HAS_STRICT_BOUNDS)
#define LV2_IS_PORT_LOGARITHMIC(x)       ((x) & LV2_PORT_LOGARITHMIC)
#define LV2_IS_PORT_NOT_AUTOMATIC(x)     ((x) & LV2_PORT_NOT_AUTOMATIC)
#define LV2_IS_PORT_NOT_ON_GUI(x)        ((x) & LV2_PORT_NOT_ON_GUI)
#define LV2_IS_PORT_REPORTS_BEATS_PER_BAR(x) ((x) & LV2_PORT_REPORTS_BEATS_PER_BAR)
#define LV2_IS_PORT_REPORTS_BEAT_UNIT(x) ((x) & LV2_PORT_REPORTS_BEAT_UNIT)
#define LV2_IS_PORT_REPORTS_BPM(x)       ((x) & LV2_PORT_REPORTS_BPM)
#define LV2_IS_PORT_TRIGGER(x)           ((x) & LV2_PORT_TRIGGER)

// A Port
struct LV2_RDF_Port {
    LV2_Property Type;
    LV2_Property Properties;
    const char* Name;
    const char* Symbol;

    LV2_RDF_PortMidiMap MidiMap;
    LV2_RDF_PortPoints Points;
    LV2_RDF_PortUnit Unit;

    uint32_t ScalePointCount;
    LV2_RDF_PortScalePoint* ScalePoints;
};

// A Preset Port
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

// A Preset State Value
union LV2_RDF_PresetStateValue {
    bool b;
    int i;
    long li;
    float f;
    const char* s;
};

// A Preset State
struct LV2_RDF_PresetState {
    LV2_Property Type;
    const char* Key;
    LV2_RDF_PresetStateValue Value;
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

// A Feature
struct LV2_RDF_Feature {
    LV2_Property Type;
    LV2_URI URI;
};

// UI Types
#define LV2_UI_X11                       0x1
#define LV2_UI_GTK2                      0x2
#define LV2_UI_QT4                       0x3
#define LV2_UI_EXTERNAL                  0x4
#define LV2_UI_OLD_EXTERNAL              0x5

#define LV2_IS_UI_X11(x)                 ((x) == LV2_UI_X11)
#define LV2_IS_UI_GTK2(x)                ((x) == LV2_UI_GTK2)
#define LV2_IS_UI_QT4(x)                 ((x) == LV2_UI_QT4)
#define LV2_IS_UI_EXTERNAL(x)            ((x) == LV2_UI_EXTERNAL)
#define LV2_IS_UI_OLD_EXTERNAL(x)        ((x) == LV2_UI_OLD_EXTERNAL)

// An UI
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
#define LV2_CLASS_GENERATOR              0x000000001
#define LV2_CLASS_INSTRUMENT             0x000000002
#define LV2_CLASS_OSCILLATOR             0x000000004
#define LV2_CLASS_UTILITY                0x000000008
#define LV2_CLASS_CONVERTER              0x000000010
#define LV2_CLASS_ANALYSER               0x000000020
#define LV2_CLASS_MIXER                  0x000000040
#define LV2_CLASS_SIMULATOR              0x000000080
#define LV2_CLASS_DELAY                  0x000000100
#define LV2_CLASS_MODULATOR              0x000000200
#define LV2_CLASS_REVERB                 0x000000400
#define LV2_CLASS_PHASER                 0x000000800
#define LV2_CLASS_FLANGER                0x000001000
#define LV2_CLASS_CHORUS                 0x000002000
#define LV2_CLASS_FILTER                 0x000004000
#define LV2_CLASS_LOWPASS                0x000008000
#define LV2_CLASS_BANDPASS               0x000010000
#define LV2_CLASS_HIGHPASS               0x000020000
#define LV2_CLASS_COMB                   0x000040000
#define LV2_CLASS_ALLPASS                0x000080000
#define LV2_CLASS_EQUALISER              0x000100000
#define LV2_CLASS_PARAMETRIC             0x000200000
#define LV2_CLASS_MULTIBAND              0x000400000
#define LV2_CLASS_SPACIAL                0x000800000
#define LV2_CLASS_SPECTRAL               0x001000000
#define LV2_CLASS_PITCH_SHIFTER          0x002000000
#define LV2_CLASS_AMPLIFIER              0x004000000
#define LV2_CLASS_DISTORTION             0x008000000
#define LV2_CLASS_WAVESHAPER             0x010000000
#define LV2_CLASS_DYNAMICS               0x020000000
#define LV2_CLASS_COMPRESSOR             0x040000000
#define LV2_CLASS_EXPANDER               0x080000000
#define LV2_CLASS_LIMITER                0x100000000LL
#define LV2_CLASS_GATE                   0x200000000LL
#define LV2_CLASS_FUNCTION               0x400000000LL
#define LV2_CLASS_CONSTANT               0x800000000LL

#define LV2_GROUP_GENERATOR              (LV2_CLASS_GENERATOR|LV2_CLASS_INSTRUMENT|LV2_CLASS_OSCILLATOR)
#define LV2_GROUP_UTILITY                (LV2_CLASS_UTILITY|LV2_CLASS_CONVERTER|LV2_CLASS_ANALYSER|LV2_CLASS_MIXER|LV2_CLASS_FUNCTION|LV2_CLASS_CONSTANT)
#define LV2_GROUP_SIMULATOR              (LV2_CLASS_SIMULATOR|LV2_CLASS_REVERB)
#define LV2_GROUP_DELAY                  (LV2_CLASS_DELAY|LV2_CLASS_REVERB)
#define LV2_GROUP_MODULATOR              (LV2_CLASS_MODULATOR|LV2_CLASS_PHASER|LV2_CLASS_FLANGER|LV2_CLASS_CHORUS)
#define LV2_GROUP_FILTER                 (LV2_CLASS_FILTER|LV2_CLASS_LOWPASS|LV2_CLASS_BANDPASS|LV2_CLASS_HIGHPASS|LV2_CLASS_COMB|LV2_CLASS_ALLPASS|LV2_CLASS_EQUALISER|LV2_CLASS_PARAMETRIC|LV2_CLASS_MULTIBAND)
#define LV2_GROUP_EQUALISER              (LV2_CLASS_EQUALISER|LV2_CLASS_PARAMETRIC|LV2_CLASS_MULTIBAND)
#define LV2_GROUP_SPECTRAL               (LV2_CLASS_SPECTRAL|LV2_CLASS_PITCH_SHIFTER)
#define LV2_GROUP_DISTORTION             (LV2_CLASS_DISTORTION|LV2_CLASS_WAVESHAPER)
#define LV2_GROUP_DYNAMICS               (LV2_CLASS_DYNAMICS|LV2_CLASS_AMPLIFIER|LV2_CLASS_COMPRESSOR|LV2_CLASS_EXPANDER|LV2_CLASS_LIMITER|LV2_CLASS_GATE)

#define LV2_IS_GENERATOR(x)              ((x) & LV2_GROUP_GENERATOR)
#define LV2_IS_UTILITY(x)                ((x) & LV2_GROUP_UTILITY)
#define LV2_IS_SIMULATOR(x)              ((x) & LV2_GROUP_SIMULATOR)
#define LV2_IS_DELAY(x)                  ((x) & LV2_GROUP_DELAY)
#define LV2_IS_MODULATOR(x)              ((x) & LV2_GROUP_MODULATOR)
#define LV2_IS_FILTER(x)                 ((x) & LV2_GROUP_FILTER)
#define LV2_IS_EQUALISER(x)              ((x) & LV2_GROUP_EQUALISER)
#define LV2_IS_SPECTRAL(x)               ((x) & LV2_GROUP_SPECTRAL)
#define LV2_IS_DISTORTION(x)             ((x) & LV2_GROUP_DISTORTION)
#define LV2_IS_DYNAMICS(x)               ((x) & LV2_GROUP_DYNAMICS)

// A Plugin
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

// Delete copied object
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

inline bool is_lv2_feature_supported(const char* uri)
{
    if (strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    else if (strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    else if (strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    else if (strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_STATE__makePath) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_STATE__mapPath) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_URI_MAP_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_URID__map) == 0)
        return true;
    else if (strcmp(uri, LV2_URID__unmap) == 0)
        return true;
    else if (strcmp(uri, LV2_RTSAFE_MEMORY_POOL_URI) == 0)
        return true;
    else
        return false;
}

inline bool is_lv2_ui_feature_supported(const char* uri)
{
    if (strcmp(uri, LV2_CORE__hardRTCapable) == 0)
        return true;
    else if (strcmp(uri, LV2_CORE__inPlaceBroken) == 0)
        return true;
    else if (strcmp(uri, LV2_CORE__isLive) == 0)
        return true;
    else if (strcmp(uri, LV2_EVENT_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_STATE__makePath) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_STATE__mapPath) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_URI_MAP_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_URID__map) == 0)
        return true;
    else if (strcmp(uri, LV2_URID__unmap) == 0)
        return true;
    else if (strcmp(uri, LV2_RTSAFE_MEMORY_POOL_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_DATA_ACCESS_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_INSTANCE_ACCESS_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_UI__noUserResize) == 0)
        return true;
    else if (strcmp(uri, LV2_UI__fixedSize) == 0)
        return true;
    else if (strcmp(uri, LV2_UI__parent) == 0)
        return true;
    else if (strcmp(uri, LV2_UI__portMap) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_UI__portSubscribe) == 0)
        return false; // TODO
    else if (strcmp(uri, LV2_UI__touch) == 0)
        return false; // TODO
    //else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#Events") == 0)
        //return true;
    else if (strcmp(uri, LV2_UI_PREFIX "makeResident") == 0)
        return true;
    //else if (strcmp(uri, "http://lv2plug.in/ns/extensions/ui#makeSONameResident") == 0)
        //return true;
    else if (strcmp(uri, LV2_EXTERNAL_UI_URI) == 0)
        return true;
    else if (strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    else
        return false;
}

inline const char* lv2_get_ui_uri(int UiType)
{
    switch(UiType)
    {
    case LV2_UI_X11:
        return LV2_UI__X11UI;
    case LV2_UI_GTK2:
        return LV2_UI__GtkUI;
    case LV2_UI_QT4:
        return LV2_UI__Qt4UI;
    case LV2_UI_EXTERNAL:
        return LV2_EXTERNAL_UI_URI;
    case LV2_UI_OLD_EXTERNAL:
        return LV2_EXTERNAL_UI_DEPRECATED_URI;
    default:
        return "UI URI Type Not Supported in LV2_RDF";
    }
}

#endif /* #ifndef LV2_RDF_INCLUDED */

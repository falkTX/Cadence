/*
 * Carla common LV2 code
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

#ifndef CARLA_LV2_INCLUDES_H
#define CARLA_LV2_INCLUDES_H

// TODO - presets
// FIXME - use strings for unit checks

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

#include "lv2_rdf.h"
#include "lilv/lilvmm.hpp"

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

// ------------------------------------------------------------------------------------------------

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"

#define LV2_MIDI_Map__CC   "http://ll-plugins.nongnu.org/lv2/namespace#CC"
#define LV2_MIDI_Map__NRPN "http://ll-plugins.nongnu.org/lv2/namespace#NRPN"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

class Lv2WorldClass : public Lilv::World
{
public:
    Lv2WorldClass() : Lilv::World(),
        port                (new_uri(LV2_CORE__port)),
        symbol              (new_uri(LV2_CORE__symbol)),
        designation         (new_uri(LV2_CORE__designation)),
        freewheeling        (new_uri(LV2_CORE__freeWheeling)),
        reportsLatency      (new_uri(LV2_CORE__reportsLatency)),

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
        ui_cocoa            (new_uri(LV2_UI__CocoaUI)),
        ui_windows          (new_uri(LV2_UI__WindowsUI)),
        ui_x11              (new_uri(LV2_UI__X11UI)),
        ui_external         (new_uri(LV2_EXTERNAL_UI_URI)),
        ui_external_old     (new_uri(LV2_EXTERNAL_UI_DEPRECATED_URI)),

        preset_preset       (new_uri(LV2_PRESETS__Preset)),
        preset_value        (new_uri(LV2_PRESETS__value)),

        state_state         (new_uri(LV2_STATE__state)),

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
    Lilv::Node designation;
    Lilv::Node freewheeling;
    Lilv::Node reportsLatency;

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
    Lilv::Node ui_cocoa;
    Lilv::Node ui_windows;
    Lilv::Node ui_x11;
    Lilv::Node ui_external;
    Lilv::Node ui_external_old;

    Lilv::Node preset_preset;
    Lilv::Node preset_value;

    Lilv::Node state_state;

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
        static bool needInit = true;
        if (needInit)
        {
            needInit = false;
            load_all();
        }
    }
};

static Lv2WorldClass Lv2World;

// ------------------------------------------------------------------------------------------------

// Create new RDF object
static inline
const LV2_RDF_Descriptor* lv2_rdf_new(const LV2_URI URI)
{
    const Lilv::Plugins Plugins = Lv2World.get_all_plugins();

    LILV_FOREACH(plugins, i, Plugins)
    {
        Lilv::Plugin Plugin(lilv_plugins_get(Plugins, i));

        if (strcmp(Plugin.get_uri().as_string(), URI) != 0)
            continue;

        LV2_RDF_Descriptor* const rdf_descriptor = new LV2_RDF_Descriptor;

        // --------------------------------------------------
        // Set Plugin Type
        {
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
        }

        // --------------------------------------------------
        // Set Plugin Information
        {
            rdf_descriptor->URI         = strdup(URI);
            rdf_descriptor->Binary      = strdup(lilv_uri_to_path(Plugin.get_library_uri().as_string()));
            rdf_descriptor->Bundle      = strdup(lilv_uri_to_path(Plugin.get_bundle_uri().as_string()));

            if (Plugin.get_name())
                rdf_descriptor->Name    = strdup(Plugin.get_name().as_string());

            if (Plugin.get_author_name())
                rdf_descriptor->Author  = strdup(Plugin.get_author_name().as_string());

            Lilv::Nodes license(Plugin.get_value(Lv2World.doap_license));

            if (license.size() > 0)
                rdf_descriptor->License = strdup(Lilv::Node(lilv_nodes_get(license, license.begin())).as_string());
        }

        // --------------------------------------------------
        // Set Plugin UniqueID
        {
            Lilv::Nodes replaces(Plugin.get_value(Lv2World.dct_replaces));

            if (replaces.size() > 0)
            {
                Lilv::Node replaceValue(lilv_nodes_get(replaces, replaces.begin()));

                if (replaceValue.is_uri())
                {
                    const QString replaceURI(replaceValue.as_uri());

                    if (replaceURI.startsWith("urn:"))
                    {
                        const QString replaceId(replaceURI.split(":").last());

                        bool ok;
                        long uniqueId = replaceId.toLong(&ok);

                        if (ok && uniqueId > 0)
                            rdf_descriptor->UniqueID = uniqueId;
                    }
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Ports
        {
            rdf_descriptor->PortCount = Plugin.get_num_ports();

            if (rdf_descriptor->PortCount > 0)
            {
                rdf_descriptor->Ports = new LV2_RDF_Port[rdf_descriptor->PortCount];

                for (uint32_t j = 0; j < rdf_descriptor->PortCount; j++)
                {
                    Lilv::Port Port(Plugin.get_port_by_index(j));

                    LV2_RDF_Port* const RDF_Port = &rdf_descriptor->Ports[j];

                    // --------------------------------------
                    // Set Port Type
                    {
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
                    }

                    // --------------------------------------
                    // Set Port Properties
                    {
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

                        if (Port.has_property(Lv2World.reportsLatency))
                            RDF_Port->Designation = LV2_PORT_LATENCY;
                    }

                    // --------------------------------------
                    // Set Port Designation (FIXME)
                    {
                        Lilv::Nodes DesignationNodes(Port.get_value(Lv2World.designation));

                        if (DesignationNodes.size() > 0)
                        {
                            const char* const designation = Lilv::Node(lilv_nodes_get(DesignationNodes, DesignationNodes.begin())).as_string();

                            if (strcmp(designation, LV2_TIME__bar) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BAR;
                            else if (strcmp(designation, LV2_TIME__barBeat) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BAR_BEAT;
                            else if (strcmp(designation, LV2_TIME__beat) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BEAT;
                            else if (strcmp(designation, LV2_TIME__beatUnit) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BEAT_UNIT;
                            else if (strcmp(designation, LV2_TIME__beatsPerBar) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BEATS_PER_BAR;
                            else if (strcmp(designation, LV2_TIME__beatsPerMinute) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_BEATS_PER_MINUTE;
                            else if (strcmp(designation, LV2_TIME__frame) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_FRAME;
                            else if (strcmp(designation, LV2_TIME__framesPerSecond) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_FRAMES_PER_SECOND;
                            else if (strcmp(designation, LV2_TIME__position) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_POSITION;
                            else if (strcmp(designation, LV2_TIME__speed) == 0)
                                RDF_Port->Designation = LV2_PORT_TIME_SPEED;
                            else
                                qWarning("lv2_rdf_new(%s) - got unknown Port Designation '%s'", URI, designation);
                        }
                    }

                    // --------------------------------------
                    // Set Port Information
                    {
                        RDF_Port->Name   = strdup(Lilv::Node(Port.get_name()).as_string());
                        RDF_Port->Symbol = strdup(Lilv::Node(Port.get_symbol()).as_string());
                    }

                    // --------------------------------------
                    // Set Port MIDI Map
                    {
                        Lilv::Nodes MidiMapNodes(Port.get_value(Lv2World.mm_default_control));

                        if (MidiMapNodes.size() > 0)
                        {
                            Lilv::Node MidiMapNode(lilv_nodes_get(MidiMapNodes, MidiMapNodes.begin()));

                            if (MidiMapNode.is_blank())
                            {
                                Lilv::Nodes MidiMapTypeNodes(Lv2World.find_nodes(MidiMapNode, Lv2World.mm_control_type, nullptr));
                                Lilv::Nodes MidiMapNumberNodes(Lv2World.find_nodes(MidiMapNode, Lv2World.mm_control_number, nullptr));

                                if (MidiMapTypeNodes.size() == 1 && MidiMapNumberNodes.size() == 1)
                                {
                                    const char* const type = Lilv::Node(lilv_nodes_get(MidiMapTypeNodes, MidiMapTypeNodes.begin())).as_string();

                                    if (strcmp(type, LV2_MIDI_Map__CC) == 0)
                                        RDF_Port->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                    else if (strcmp(type, LV2_MIDI_Map__NRPN) == 0)
                                        RDF_Port->MidiMap.Type = LV2_PORT_MIDI_MAP_NRPN;
                                    else
                                        qWarning("lv2_rdf_new(%s) - got unknown Port Midi Map type '%s'", URI, type);

                                    RDF_Port->MidiMap.Number = Lilv::Node(lilv_nodes_get(MidiMapNumberNodes, MidiMapNumberNodes.begin())).as_int();
                                }
                            }
                        }
                    }

                    // --------------------------------------
                    // Set Port Points
                    {
                        Lilv::Nodes value(Port.get_value(Lv2World.value_default));

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
                    }

                    // --------------------------------------
                    // Set Port Unit
                    {
                        Lilv::Nodes unit_units(Port.get_value(Lv2World.unit_unit));

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

                        Lilv::Nodes unit_name(Port.get_value(Lv2World.unit_name));

                        if (unit_name.size() > 0)
                        {
                            RDF_Port->Unit.Hints |= LV2_PORT_UNIT_NAME;
                            RDF_Port->Unit.Name = strdup(Lilv::Node(lilv_nodes_get(unit_name, unit_name.begin())).as_string());
                        }

                        Lilv::Nodes unit_render(Port.get_value(Lv2World.unit_render));

                        if (unit_render.size() > 0)
                        {
                            RDF_Port->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                            RDF_Port->Unit.Render = strdup(Lilv::Node(lilv_nodes_get(unit_render, unit_render.begin())).as_string());
                        }

                        Lilv::Nodes unit_symbol(Port.get_value(Lv2World.unit_symbol));

                        if (unit_symbol.size() > 0)
                        {
                            RDF_Port->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                            RDF_Port->Unit.Symbol = strdup(Lilv::Node(lilv_nodes_get(unit_symbol, unit_symbol.begin())).as_string());
                        }
                    }

                    // --------------------------------------
                    // Set Port Scale Points

                    Lilv::ScalePoints ScalePoints(Port.get_scale_points());

                    RDF_Port->ScalePointCount = ScalePoints.size();

                    if (RDF_Port->ScalePointCount > 0)
                    {
                        RDF_Port->ScalePoints = new LV2_RDF_PortScalePoint[RDF_Port->ScalePointCount];

                        uint32_t h = 0;
                        LILV_FOREACH(scale_points, j, ScalePoints)
                        {
                            Lilv::ScalePoint ScalePoint(lilv_scale_points_get(ScalePoints, j));

                            LV2_RDF_PortScalePoint* const RDF_ScalePoint = &RDF_Port->ScalePoints[h++];
                            RDF_ScalePoint->Label = strdup(Lilv::Node(ScalePoint.get_label()).as_string());
                            RDF_ScalePoint->Value = Lilv::Node(ScalePoint.get_value()).as_float();
                        }
                    }
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Presets (TODO)
        {
            Lilv::Nodes Presets(Plugin.get_related(Lv2World.preset_preset));

            rdf_descriptor->PresetCount = Presets.size();

            if (rdf_descriptor->PresetCount > 0)
            {
                QStringList PresetListURIs;

                LILV_FOREACH(nodes, j, Presets)
                        // FIXME - check appliesTo()
                        PresetListURIs.append(QString(Lilv::Node(lilv_nodes_get(Presets, j)).as_uri()));

                PresetListURIs.sort();

                rdf_descriptor->Presets = new LV2_RDF_Preset[rdf_descriptor->PresetCount];

                LILV_FOREACH(nodes, j, Presets)
                {
                    Lilv::Node PresetNode(lilv_nodes_get(Presets, j));
                    Lv2World.load_resource(PresetNode);

                    LV2_URI PresetURI = PresetNode.as_uri();
                    uint32_t index = PresetListURIs.indexOf(QString(PresetURI));

                    LV2_RDF_Preset* const RDF_Preset = &rdf_descriptor->Presets[index];

                    // --------------------------------------
                    // Set Preset Information
                    {
                        RDF_Preset->URI = strdup(PresetURI);

                        Lilv::Nodes PresetLabel(Lv2World.find_nodes(PresetNode, Lv2World.rdfs_label, nullptr));

                        if (PresetLabel.size() > 0)
                            RDF_Preset->Label = strdup(Lilv::Node(lilv_nodes_get(PresetLabel, PresetLabel.begin())).as_string());
                    }

                    // --------------------------------------
                    // Set Preset Ports
                    {
                        Lilv::Nodes PresetPorts(Lv2World.find_nodes(PresetNode, Lv2World.port, nullptr));

                        RDF_Preset->PortCount = PresetPorts.size();

                        if (RDF_Preset->PortCount > 0)
                        {
                            RDF_Preset->Ports = new LV2_RDF_PresetPort[RDF_Preset->PortCount];

                            uint32_t g = 0;
                            LILV_FOREACH(nodes, k, PresetPorts)
                            {
                                Lilv::Node PresetPort(lilv_nodes_get(PresetPorts, k));

                                Lilv::Nodes PresetPortSymbol(Lv2World.find_nodes(PresetPort, Lv2World.symbol, nullptr));
                                Lilv::Nodes PresetPortValue(Lv2World.find_nodes(PresetPort, Lv2World.preset_value, nullptr));

                                LV2_RDF_PresetPort* const RDF_PresetPort = &RDF_Preset->Ports[g++];
                                RDF_PresetPort->Symbol = strdup(Lilv::Node(lilv_nodes_get(PresetPortSymbol, PresetPortSymbol.begin())).as_string());
                                RDF_PresetPort->Value  = Lilv::Node(lilv_nodes_get(PresetPortValue, PresetPortValue.begin())).as_float();
                            }
                        }
                    }

                    // --------------------------------------
                    // Set Preset States
                    {
                        Lilv::Nodes PresetStates(Lv2World.find_nodes(PresetNode, Lv2World.state_state, nullptr));

                        RDF_Preset->StateCount = PresetStates.size();

                        if (RDF_Preset->StateCount > 0)
                        {
                            RDF_Preset->States = new LV2_RDF_PresetState[RDF_Preset->StateCount];

                            uint32_t g = 0;
                            LILV_FOREACH(nodes, k, PresetStates)
                            {
                                Lilv::Node PresetState(PresetStates.get(k));

                                if (PresetState.is_blank())
                                {
                                    // TODO

                                    LV2_RDF_PresetState* const RDF_PresetState = &RDF_Preset->States[g++];

                                    RDF_PresetState->Type = LV2_PRESET_STATE_NULL;
                                    RDF_PresetState->Key  = nullptr;
                                }
                            }
                        }
                    }
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Features
        {
            Lilv::Nodes Features(Plugin.get_supported_features());
            Lilv::Nodes FeaturesR(Plugin.get_required_features());

            rdf_descriptor->FeatureCount = Features.size();

            if (rdf_descriptor->FeatureCount > 0)
            {
                rdf_descriptor->Features = new LV2_RDF_Feature[rdf_descriptor->FeatureCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, Features)
                {
                    Lilv::Node FeatureNode(lilv_nodes_get(Features, j));

                    LV2_RDF_Feature* const RDF_Feature = &rdf_descriptor->Features[h++];
                    RDF_Feature->Type = FeaturesR.contains(FeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                    RDF_Feature->URI  = strdup(FeatureNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Extensions
        {
            Lilv::Nodes Extensions(Plugin.get_extension_data());

            rdf_descriptor->ExtensionCount = Extensions.size();

            if (rdf_descriptor->ExtensionCount > 0)
            {
                rdf_descriptor->Extensions = new LV2_URI[rdf_descriptor->ExtensionCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, Extensions)
                {
                    Lilv::Node ExtensionNode(Lilv::Node(lilv_nodes_get(Extensions, j)));

                    rdf_descriptor->Extensions[h++] = strdup(ExtensionNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin UIs
        {
            Lilv::UIs UIs(Plugin.get_uis());

            rdf_descriptor->UICount = UIs.size();

            if (rdf_descriptor->UICount > 0)
            {
                rdf_descriptor->UIs = new LV2_RDF_UI[rdf_descriptor->UICount];

                uint32_t h = 0;
                LILV_FOREACH(uis, j, UIs)
                {
                    Lilv::UI UI(lilv_uis_get(UIs, j));

                    LV2_RDF_UI* const RDF_UI = &rdf_descriptor->UIs[h++];

                    // --------------------------------------
                    // Set UI Type
                    {
                        if (UI.is_a(Lv2World.ui_gtk2))
                            RDF_UI->Type = LV2_UI_GTK2;
                        else if (UI.is_a(Lv2World.ui_qt4))
                            RDF_UI->Type = LV2_UI_QT4;
                        else if (UI.is_a(Lv2World.ui_cocoa))
                            RDF_UI->Type = LV2_UI_COCOA;
                        else if (UI.is_a(Lv2World.ui_windows))
                            RDF_UI->Type = LV2_UI_WINDOWS;
                        else if (UI.is_a(Lv2World.ui_x11))
                            RDF_UI->Type = LV2_UI_X11;
                        else if (UI.is_a(Lv2World.ui_external))
                            RDF_UI->Type = LV2_UI_EXTERNAL;
                        else if (UI.is_a(Lv2World.ui_external_old))
                            RDF_UI->Type = LV2_UI_OLD_EXTERNAL;
                        else
                            qWarning("lv2_rdf_new(%s) - got unknown UI type '%s'", URI, UI.get_uri().as_uri());
                    }

                    // --------------------------------------
                    // Set UI Information
                    {
                        RDF_UI->URI    = strdup(UI.get_uri().as_uri());
                        RDF_UI->Binary = strdup(lilv_uri_to_path(UI.get_binary_uri().as_string()));
                        RDF_UI->Bundle = strdup(lilv_uri_to_path(UI.get_bundle_uri().as_string()));
                    }

                    // --------------------------------------
                    // Set UI Features
                    {
                        Lilv::Nodes Features(UI.get_supported_features());
                        Lilv::Nodes FeaturesR(UI.get_required_features());

                        RDF_UI->FeatureCount = Features.size();

                        if (RDF_UI->FeatureCount > 0)
                        {
                            RDF_UI->Features = new LV2_RDF_Feature [RDF_UI->FeatureCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, Features)
                            {
                                Lilv::Node FeatureNode(lilv_nodes_get(Features, k));

                                LV2_RDF_Feature* const RDF_Feature = &RDF_UI->Features[h++];
                                RDF_Feature->Type = FeaturesR.contains(FeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                                RDF_Feature->URI  = strdup(FeatureNode.as_uri());
                            }
                        }
                    }

                    // --------------------------------------
                    // Set UI Extensions
                    {
                        Lilv::Nodes Extensions(UI.get_extension_data());

                        RDF_UI->ExtensionCount = Extensions.size();

                        if (RDF_UI->ExtensionCount > 0)
                        {
                            RDF_UI->Extensions = new LV2_URI[RDF_UI->ExtensionCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, Extensions)
                            {
                                Lilv::Node ExtensionNode(lilv_nodes_get(Extensions, k));

                                RDF_UI->Extensions[h++] = strdup(ExtensionNode.as_uri());
                            }
                        }
                    }
                }
            }
        }

        return rdf_descriptor;
    }

    return nullptr;
}

// Copy RDF object
static inline
const LV2_RDF_Descriptor* lv2_rdf_dup(const LV2_RDF_Descriptor* const rdf_descriptor)
{
    LV2_RDF_Descriptor* const new_descriptor = new LV2_RDF_Descriptor;

    new_descriptor->Type           = rdf_descriptor->Type;
    new_descriptor->UniqueID       = rdf_descriptor->UniqueID;

    new_descriptor->PortCount      = rdf_descriptor->PortCount;
    new_descriptor->PresetCount    = rdf_descriptor->PresetCount;
    new_descriptor->FeatureCount   = rdf_descriptor->FeatureCount;
    new_descriptor->ExtensionCount = rdf_descriptor->ExtensionCount;
    new_descriptor->UICount        = rdf_descriptor->UICount;

    if (rdf_descriptor->URI)
        new_descriptor->URI        = strdup(rdf_descriptor->URI);

    if (rdf_descriptor->Name)
        new_descriptor->Name       = strdup(rdf_descriptor->Name);

    if (rdf_descriptor->Author)
        new_descriptor->Author     = strdup(rdf_descriptor->Author);

    if (rdf_descriptor->License)
        new_descriptor->License    = strdup(rdf_descriptor->License);

    if (rdf_descriptor->Binary)
        new_descriptor->Binary     = strdup(rdf_descriptor->Binary);

    if (rdf_descriptor->Bundle)
        new_descriptor->Bundle     = strdup(rdf_descriptor->Bundle);

    // Ports
    if (new_descriptor->PortCount > 0)
    {
        new_descriptor->Ports = new LV2_RDF_Port[new_descriptor->PortCount];

        for (uint32_t i=0; i < new_descriptor->PortCount; i++)
        {
            LV2_RDF_Port* const Port = &new_descriptor->Ports[i];

            Port->Type            = rdf_descriptor->Ports[i].Type;
            Port->Properties      = rdf_descriptor->Ports[i].Properties;
            Port->Designation     = rdf_descriptor->Ports[i].Designation;

            Port->MidiMap.Type    = rdf_descriptor->Ports[i].MidiMap.Type;
            Port->MidiMap.Number  = rdf_descriptor->Ports[i].MidiMap.Number;

            Port->Points.Hints    = rdf_descriptor->Ports[i].Points.Hints;
            Port->Points.Default  = rdf_descriptor->Ports[i].Points.Default;
            Port->Points.Minimum  = rdf_descriptor->Ports[i].Points.Minimum;
            Port->Points.Maximum  = rdf_descriptor->Ports[i].Points.Maximum;

            Port->Unit.Type       = rdf_descriptor->Ports[i].Unit.Type;
            Port->Unit.Hints      = rdf_descriptor->Ports[i].Unit.Hints;

            Port->ScalePointCount = rdf_descriptor->Ports[i].ScalePointCount;

            if (rdf_descriptor->Ports[i].Name)
                Port->Name        = strdup(rdf_descriptor->Ports[i].Name);

            if (rdf_descriptor->Ports[i].Symbol)
                Port->Symbol      = strdup(rdf_descriptor->Ports[i].Symbol);

            if (rdf_descriptor->Ports[i].Unit.Name)
                Port->Unit.Name   = strdup(rdf_descriptor->Ports[i].Unit.Name);

            if (rdf_descriptor->Ports[i].Unit.Render)
                Port->Unit.Render = strdup(rdf_descriptor->Ports[i].Unit.Render);

            if (rdf_descriptor->Ports[i].Unit.Symbol)
                Port->Unit.Symbol = strdup(rdf_descriptor->Ports[i].Unit.Symbol);

            if (Port->ScalePointCount > 0)
            {
                Port->ScalePoints = new LV2_RDF_PortScalePoint[Port->ScalePointCount];

                for (uint32_t j=0; j < Port->ScalePointCount; j++)
                {
                    Port->ScalePoints[j].Value = rdf_descriptor->Ports[i].ScalePoints[j].Value;

                    if (rdf_descriptor->Ports[i].ScalePoints[j].Label)
                        Port->ScalePoints[j].Label = strdup(rdf_descriptor->Ports[i].ScalePoints[j].Label);
                }
            }
        }
    }

    // Presets
    if (new_descriptor->PresetCount > 0)
    {
        new_descriptor->Presets = new LV2_RDF_Preset[new_descriptor->PresetCount];

        for (uint32_t i=0; i < new_descriptor->PresetCount; i++)
        {
            LV2_RDF_Preset* const Preset = &new_descriptor->Presets[i];

            Preset->PortCount  = rdf_descriptor->Presets[i].PortCount;
            Preset->StateCount = rdf_descriptor->Presets[i].StateCount;

            if (rdf_descriptor->Presets[i].URI)
                Preset->URI    = strdup(rdf_descriptor->Presets[i].URI);

            if (rdf_descriptor->Presets[i].Label)
                Preset->Label  = strdup(rdf_descriptor->Presets[i].Label);

            // Ports
            if (Preset->PortCount > 0)
            {
                Preset->Ports = new LV2_RDF_PresetPort[Preset->PortCount];

                for (uint32_t j=0; j < Preset->PortCount; j++)
                {
                    Preset->Ports[j].Value = rdf_descriptor->Presets[i].Ports[j].Value;

                    if (rdf_descriptor->Presets[i].Ports[j].Symbol)
                        Preset->Ports[j].Symbol = strdup(rdf_descriptor->Presets[i].Ports[j].Symbol);
                }
            }

            // States
            if (Preset->StateCount > 0)
            {
                Preset->States = new LV2_RDF_PresetState[Preset->StateCount];

                for (uint32_t j=0; j < Preset->StateCount; j++)
                {
                    Preset->States[j].Type = rdf_descriptor->Presets[i].States[j].Type;

                    if (rdf_descriptor->Presets[i].States[j].Key)
                        Preset->States[j].Key = strdup(rdf_descriptor->Presets[i].States[j].Key);

                    // TODO - copy value
                }
            }
        }
    }

    // Features
    if (new_descriptor->FeatureCount > 0)
    {
        new_descriptor->Features = new LV2_RDF_Feature[new_descriptor->FeatureCount];

        for (uint32_t i=0; i < new_descriptor->FeatureCount; i++)
        {
            new_descriptor->Features[i].Type = rdf_descriptor->Features[i].Type;

            if (rdf_descriptor->Features[i].URI)
                new_descriptor->Features[i].URI = strdup(rdf_descriptor->Features[i].URI);
        }
    }

    // Extensions
    if (new_descriptor->ExtensionCount > 0)
    {
        new_descriptor->Extensions = new LV2_URI[new_descriptor->ExtensionCount];

        for (uint32_t i=0; i < new_descriptor->ExtensionCount; i++)
        {
            if (rdf_descriptor->Extensions[i])
                new_descriptor->Extensions[i] = strdup(rdf_descriptor->Extensions[i]);
        }
    }

    // UIs
    if (new_descriptor->UICount > 0)
    {
        new_descriptor->UIs = new LV2_RDF_UI[new_descriptor->UICount];

        for (uint32_t i=0; i < new_descriptor->UICount; i++)
        {
            LV2_RDF_UI* const UI = &new_descriptor->UIs[i];

            UI->Type           = rdf_descriptor->UIs[i].Type;

            UI->FeatureCount   = rdf_descriptor->UIs[i].FeatureCount;
            UI->ExtensionCount = rdf_descriptor->UIs[i].ExtensionCount;

            if (rdf_descriptor->UIs[i].URI)
                UI->URI        = strdup(rdf_descriptor->UIs[i].URI);

            if (rdf_descriptor->UIs[i].Binary)
                UI->Binary     = strdup(rdf_descriptor->UIs[i].Binary);

            if (rdf_descriptor->UIs[i].Bundle)
                UI->Bundle     = strdup(rdf_descriptor->UIs[i].Bundle);

            // UI Features
            if (UI->FeatureCount > 0)
            {
                UI->Features = new LV2_RDF_Feature[UI->FeatureCount];

                for (uint32_t j=0; j < UI->FeatureCount; j++)
                {
                    UI->Features[j].Type = rdf_descriptor->UIs[i].Features[j].Type;

                    if (rdf_descriptor->UIs[i].Features[j].URI)
                        UI->Features[j].URI = strdup(rdf_descriptor->UIs[i].Features[j].URI);
                }
            }

            // UI Extensions
            if (UI->ExtensionCount > 0)
            {
                UI->Extensions = new LV2_URI[UI->ExtensionCount];

                for (uint32_t j=0; j < UI->ExtensionCount; j++)
                {
                    if (rdf_descriptor->UIs[i].Extensions[j])
                        UI->Extensions[j] = strdup(rdf_descriptor->UIs[i].Extensions[j]);
                }
            }
        }
    }

    return new_descriptor;
}

// Delete object
static inline
void lv2_rdf_free(const LV2_RDF_Descriptor* const rdf_descriptor)
{
    if (rdf_descriptor->URI)
        free((void*)rdf_descriptor->URI);

    if (rdf_descriptor->Name)
        free((void*)rdf_descriptor->Name);

    if (rdf_descriptor->Author)
        free((void*)rdf_descriptor->Author);

    if (rdf_descriptor->License)
        free((void*)rdf_descriptor->License);

    if (rdf_descriptor->Binary)
        free((void*)rdf_descriptor->Binary);

    if (rdf_descriptor->Bundle)
        free((void*)rdf_descriptor->Bundle);

    if (rdf_descriptor->PortCount > 0)
    {
        for (uint32_t i=0; i < rdf_descriptor->PortCount; i++)
        {
            const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[i];

            if (Port->Name)
                free((void*)Port->Name);

            if (Port->Symbol)
                free((void*)Port->Symbol);

            if (Port->Unit.Name)
                free((void*)Port->Unit.Name);

            if (Port->Unit.Render)
                free((void*)Port->Unit.Render);

            if (Port->Unit.Symbol)
                free((void*)Port->Unit.Symbol);

            if (Port->ScalePointCount > 0)
            {
                for (uint32_t j=0; j < Port->ScalePointCount; j++)
                {
                    const LV2_RDF_PortScalePoint* const PortScalePoint = &Port->ScalePoints[j];

                    if (PortScalePoint->Label)
                        free((void*)PortScalePoint->Label);
                }
                delete[] Port->ScalePoints;
            }
        }
        delete[] rdf_descriptor->Ports;
    }

    if (rdf_descriptor->PresetCount > 0)
    {
        for (uint32_t i=0; i < rdf_descriptor->PresetCount; i++)
        {
            const LV2_RDF_Preset* const Preset = &rdf_descriptor->Presets[i];

            if (Preset->URI)
                free((void*)Preset->URI);

            if (Preset->Label)
                free((void*)Preset->Label);

            if (Preset->PortCount > 0)
            {
                for (uint32_t j=0; j < Preset->PortCount; j++)
                {
                    const LV2_RDF_PresetPort* const PresetPort = &Preset->Ports[j];

                    if (PresetPort->Symbol)
                        free((void*)PresetPort->Symbol);
                }
                delete[] Preset->Ports;
            }

            if (Preset->StateCount > 0)
            {
                for (uint32_t j=0; j < Preset->StateCount; j++)
                {
                    const LV2_RDF_PresetState* const PresetState = &Preset->States[j];

                    if (PresetState->Key)
                        free((void*)PresetState->Key);

                    // TODO - delete value
                }
                delete[] Preset->States;
            }
        }
        delete[] rdf_descriptor->Presets;
    }

    if (rdf_descriptor->FeatureCount > 0)
    {
        for (uint32_t i=0; i < rdf_descriptor->FeatureCount; i++)
        {
            const LV2_RDF_Feature* const Feature = &rdf_descriptor->Features[i];

            if (Feature->URI)
                free((void*)Feature->URI);
        }
        delete[] rdf_descriptor->Features;
    }

    if (rdf_descriptor->ExtensionCount > 0)
    {
        for (uint32_t i=0; i < rdf_descriptor->ExtensionCount; i++)
        {
            const LV2_URI Extension = rdf_descriptor->Extensions[i];

            if (Extension)
                free((void*)Extension);
        }
        delete[] rdf_descriptor->Extensions;
    }

    if (rdf_descriptor->UICount > 0)
    {
        for (uint32_t i=0; i < rdf_descriptor->UICount; i++)
        {
            const LV2_RDF_UI* const UI = &rdf_descriptor->UIs[i];

            if (UI->URI)
                free((void*)UI->URI);

            if (UI->Binary)
                free((void*)UI->Binary);

            if (UI->Bundle)
                free((void*)UI->Bundle);

            if (UI->FeatureCount > 0)
            {
                for (uint32_t j=0; j < UI->FeatureCount; j++)
                {
                    const LV2_RDF_Feature* const Feature = &UI->Features[j];

                    if (Feature->URI)
                        free((void*)Feature->URI);
                }
                delete[] UI->Features;
            }

            if (UI->ExtensionCount > 0)
            {
                for (uint32_t j=0; j < UI->ExtensionCount; j++)
                {
                    const LV2_URI Extension = UI->Extensions[j];

                    if (Extension)
                        free((void*)Extension);
                }
                delete[] UI->Extensions;
            }
        }
        delete[] rdf_descriptor->UIs;
    }

    delete rdf_descriptor;
}

// ------------------------------------------------------------------------------------------------

static inline
bool is_lv2_feature_supported(const LV2_URI uri)
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

static inline
bool is_lv2_ui_feature_supported(const LV2_URI uri)
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
        return false; // TODO: uninplemented
    if (strcmp(uri, LV2_UI__resize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__touch) == 0)
        return false; // TODO: uninplemented
    if (strcmp(uri, LV2_UI_PREFIX "makeResident") == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_URI) == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    return false;
}

static inline
LV2_URI lv2_get_ui_uri(const int UiType)
{
    switch (UiType)
    {
    case LV2_UI_GTK2:
        return LV2_UI__GtkUI;
    case LV2_UI_QT4:
        return LV2_UI__Qt4UI;
    case LV2_UI_COCOA:
        return LV2_UI__CocoaUI;
    case LV2_UI_WINDOWS:
        return LV2_UI__WindowsUI;
    case LV2_UI_X11:
        return LV2_UI__X11UI;
    case LV2_UI_EXTERNAL:
        return LV2_EXTERNAL_UI_URI;
    case LV2_UI_OLD_EXTERNAL:
        return LV2_EXTERNAL_UI_DEPRECATED_URI;
    default:
        return "UI URI Type Not Supported";
    }
}

#endif // CARLA_LV2_INCLUDES_H

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

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/atom-forge.h"
#include "lv2/atom-util.h"
#include "lv2/data-access.h"
// dynmanifest
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/log.h"
#include "lv2/midi.h"
#include "lv2/patch.h"
#include "lv2/port-groups.h"
#include "lv2/port-props.h"
#include "lv2/presets.h"
// resize-port
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
#include "sratom/sratom.h"

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

// ------------------------------------------------------------------------------------------------

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_doap "http://usefulinc.com/ns/doap#"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_rdfs "http://www.w3.org/2000/01/rdf-schema#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"

#define LV2_PARAMETERS_URI    "http://lv2plug.in/ns/ext/parameters"
#define LV2_PARAMETERS_PREFIX LV2_PARAMETERS_URI "#"

#define LV2_MIDI_Map__CC      "http://ll-plugins.nongnu.org/lv2/namespace#CC"
#define LV2_MIDI_Map__NRPN    "http://ll-plugins.nongnu.org/lv2/namespace#NRPN"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

#define LV2_UI__makeResident  LV2_UI_PREFIX "makeResident"

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
    {
        needInit = true;
    }

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

    // UI Types
    Lilv::Node ui_gtk2;
    Lilv::Node ui_qt4;
    Lilv::Node ui_cocoa;
    Lilv::Node ui_windows;
    Lilv::Node ui_x11;
    Lilv::Node ui_external;
    Lilv::Node ui_external_old;

    // Misc
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
        if (needInit)
        {
            needInit = false;
            load_all();
        }
    }

private:
    bool needInit;
};

static Lv2WorldClass Lv2World;

// ------------------------------------------------------------------------------------------------

// Create new RDF object
static inline
const LV2_RDF_Descriptor* lv2_rdf_new(const LV2_URI URI)
{
    Q_ASSERT(URI);
    Lv2World.init();

    Lilv::Plugins lilvPlugins = Lv2World.get_all_plugins();

    LILV_FOREACH(plugins, i, lilvPlugins)
    {
        Lilv::Plugin lilvPlugin(lilvPlugins.get(i));

        if (strcmp(lilvPlugin.get_uri().as_string(), URI) != 0)
            continue;

        LV2_RDF_Descriptor* const rdf_descriptor = new LV2_RDF_Descriptor;

        // --------------------------------------------------
        // Set Plugin Type
        {
            Lilv::Nodes typeNodes(lilvPlugin.get_value(Lv2World.rdf_type));

            if (typeNodes.size() > 0)
            {
                if (typeNodes.contains(Lv2World.class_allpass))
                    rdf_descriptor->Type |= LV2_CLASS_ALLPASS;
                if (typeNodes.contains(Lv2World.class_amplifier))
                    rdf_descriptor->Type |= LV2_CLASS_AMPLIFIER;
                if (typeNodes.contains(Lv2World.class_analyzer))
                    rdf_descriptor->Type |= LV2_CLASS_ANALYSER;
                if (typeNodes.contains(Lv2World.class_bandpass))
                    rdf_descriptor->Type |= LV2_CLASS_BANDPASS;
                if (typeNodes.contains(Lv2World.class_chorus))
                    rdf_descriptor->Type |= LV2_CLASS_CHORUS;
                if (typeNodes.contains(Lv2World.class_comb))
                    rdf_descriptor->Type |= LV2_CLASS_COMB;
                if (typeNodes.contains(Lv2World.class_compressor))
                    rdf_descriptor->Type |= LV2_CLASS_COMPRESSOR;
                if (typeNodes.contains(Lv2World.class_constant))
                    rdf_descriptor->Type |= LV2_CLASS_CONSTANT;
                if (typeNodes.contains(Lv2World.class_converter))
                    rdf_descriptor->Type |= LV2_CLASS_CONVERTER;
                if (typeNodes.contains(Lv2World.class_delay))
                    rdf_descriptor->Type |= LV2_CLASS_DELAY;
                if (typeNodes.contains(Lv2World.class_distortion))
                    rdf_descriptor->Type |= LV2_CLASS_DISTORTION;
                if (typeNodes.contains(Lv2World.class_dynamics))
                    rdf_descriptor->Type |= LV2_CLASS_DYNAMICS;
                if (typeNodes.contains(Lv2World.class_eq))
                    rdf_descriptor->Type |= LV2_CLASS_EQ;
                if (typeNodes.contains(Lv2World.class_expander))
                    rdf_descriptor->Type |= LV2_CLASS_EXPANDER;
                if (typeNodes.contains(Lv2World.class_filter))
                    rdf_descriptor->Type |= LV2_CLASS_FILTER;
                if (typeNodes.contains(Lv2World.class_flanger))
                    rdf_descriptor->Type |= LV2_CLASS_FLANGER;
                if (typeNodes.contains(Lv2World.class_function))
                    rdf_descriptor->Type |= LV2_CLASS_FUNCTION;
                if (typeNodes.contains(Lv2World.class_gate))
                    rdf_descriptor->Type |= LV2_CLASS_GATE;
                if (typeNodes.contains(Lv2World.class_generator))
                    rdf_descriptor->Type |= LV2_CLASS_GENERATOR;
                if (typeNodes.contains(Lv2World.class_highpass))
                    rdf_descriptor->Type |= LV2_CLASS_HIGHPASS;
                if (typeNodes.contains(Lv2World.class_instrument))
                    rdf_descriptor->Type |= LV2_CLASS_INSTRUMENT;
                if (typeNodes.contains(Lv2World.class_limiter))
                    rdf_descriptor->Type |= LV2_CLASS_LIMITER;
                if (typeNodes.contains(Lv2World.class_lowpass))
                    rdf_descriptor->Type |= LV2_CLASS_LOWPASS;
                if (typeNodes.contains(Lv2World.class_mixer))
                    rdf_descriptor->Type |= LV2_CLASS_MIXER;
                if (typeNodes.contains(Lv2World.class_modulator))
                    rdf_descriptor->Type |= LV2_CLASS_MODULATOR;
                if (typeNodes.contains(Lv2World.class_multi_eq))
                    rdf_descriptor->Type |= LV2_CLASS_MULTI_EQ;
                if (typeNodes.contains(Lv2World.class_oscillator))
                    rdf_descriptor->Type |= LV2_CLASS_OSCILLATOR;
                if (typeNodes.contains(Lv2World.class_para_eq))
                    rdf_descriptor->Type |= LV2_CLASS_PARA_EQ;
                if (typeNodes.contains(Lv2World.class_phaser))
                    rdf_descriptor->Type |= LV2_CLASS_PHASER;
                if (typeNodes.contains(Lv2World.class_pitch))
                    rdf_descriptor->Type |= LV2_CLASS_PITCH;
                if (typeNodes.contains(Lv2World.class_reverb))
                    rdf_descriptor->Type |= LV2_CLASS_REVERB;
                if (typeNodes.contains(Lv2World.class_simulator))
                    rdf_descriptor->Type |= LV2_CLASS_SIMULATOR;
                if (typeNodes.contains(Lv2World.class_spatial))
                    rdf_descriptor->Type |= LV2_CLASS_SPATIAL;
                if (typeNodes.contains(Lv2World.class_spectral))
                    rdf_descriptor->Type |= LV2_CLASS_SPECTRAL;
                if (typeNodes.contains(Lv2World.class_utility))
                    rdf_descriptor->Type |= LV2_CLASS_UTILITY;
                if (typeNodes.contains(Lv2World.class_waveshaper))
                    rdf_descriptor->Type |= LV2_CLASS_WAVESHAPER;
            }
        }

        // --------------------------------------------------
        // Set Plugin Information
        {
            rdf_descriptor->URI    = strdup(URI);
            rdf_descriptor->Binary = strdup(lilv_uri_to_path(lilvPlugin.get_library_uri().as_string()));
            rdf_descriptor->Bundle = strdup(lilv_uri_to_path(lilvPlugin.get_bundle_uri().as_string()));

            if (lilvPlugin.get_name())
                rdf_descriptor->Name = strdup(lilvPlugin.get_name().as_string());

            if (lilvPlugin.get_author_name())
                rdf_descriptor->Author  = strdup(lilvPlugin.get_author_name().as_string());

            Lilv::Nodes licenseNodes(lilvPlugin.get_value(Lv2World.doap_license));

            if (licenseNodes.size() > 0)
                rdf_descriptor->License = strdup(licenseNodes.get_first().as_string());
        }

        // --------------------------------------------------
        // Set Plugin UniqueID
        {
            Lilv::Nodes replaceNodes(lilvPlugin.get_value(Lv2World.dct_replaces));

            if (replaceNodes.size() > 0)
            {
                Lilv::Node replaceNode(replaceNodes.get_first());

                if (replaceNode.is_uri())
                {
                    const QString replaceURI(replaceNode.as_uri());

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

        if (lilvPlugin.get_num_ports() > 0)
        {
            rdf_descriptor->PortCount = lilvPlugin.get_num_ports();
            rdf_descriptor->Ports = new LV2_RDF_Port[rdf_descriptor->PortCount];

            for (uint32_t j = 0; j < rdf_descriptor->PortCount; j++)
            {
                Lilv::Port lilvPort(lilvPlugin.get_port_by_index(j));
                LV2_RDF_Port* const rdf_port = &rdf_descriptor->Ports[j];

                // --------------------------------------
                // Set Port Type
                {
                    if (lilvPort.is_a(Lv2World.port_input))
                        rdf_port->Type |= LV2_PORT_INPUT;

                    if (lilvPort.is_a(Lv2World.port_output))
                        rdf_port->Type |= LV2_PORT_OUTPUT;

                    if (lilvPort.is_a(Lv2World.port_control))
                        rdf_port->Type |= LV2_PORT_CONTROL;

                    if (lilvPort.is_a(Lv2World.port_audio))
                        rdf_port->Type |= LV2_PORT_AUDIO;

                    if (lilvPort.is_a(Lv2World.port_cv))
                        rdf_port->Type |= LV2_PORT_CV;

                    if (lilvPort.is_a(Lv2World.port_atom))
                    {
                        rdf_port->Type |= LV2_PORT_ATOM;

                        Lilv::Nodes bufferTypeNodes(lilvPort.get_value(Lv2World.atom_buffer_type));

                        if (bufferTypeNodes.contains(Lv2World.atom_sequence))
                            rdf_port->Type |= LV2_PORT_ATOM_SEQUENCE;

                        Lilv::Nodes supportNodes(lilvPort.get_value(Lv2World.atom_supports));

                        if (supportNodes.contains(Lv2World.midi_event))
                            rdf_port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                        if (supportNodes.contains(Lv2World.patch_message))
                            rdf_port->Type |= LV2_PORT_SUPPORTS_PATCH_MESSAGE;
                    }

                    if (lilvPort.is_a(Lv2World.port_event))
                    {
                        rdf_port->Type |= LV2_PORT_EVENT;

                        if (lilvPort.supports_event(Lv2World.midi_event))
                            rdf_port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }

                    if (lilvPort.is_a(Lv2World.port_midi_ll))
                    {
                        rdf_port->Type |= LV2_PORT_MIDI_LL;
                        rdf_port->Type |= LV2_PORT_SUPPORTS_MIDI_EVENT;
                    }
                }

                // --------------------------------------
                // Set Port Properties
                {
                    if (lilvPort.has_property(Lv2World.pprop_optional))
                        rdf_port->Properties = LV2_PORT_OPTIONAL;
                    if (lilvPort.has_property(Lv2World.pprop_enumeration))
                        rdf_port->Properties = LV2_PORT_ENUMERATION;
                    if (lilvPort.has_property(Lv2World.pprop_integer))
                        rdf_port->Properties = LV2_PORT_INTEGER;
                    if (lilvPort.has_property(Lv2World.pprop_sample_rate))
                        rdf_port->Properties = LV2_PORT_SAMPLE_RATE;
                    if (lilvPort.has_property(Lv2World.pprop_toggled))
                        rdf_port->Properties = LV2_PORT_TOGGLED;

                    if (lilvPort.has_property(Lv2World.pprop_artifacts))
                        rdf_port->Properties = LV2_PORT_CAUSES_ARTIFACTS;
                    if (lilvPort.has_property(Lv2World.pprop_continuous_cv))
                        rdf_port->Properties = LV2_PORT_CONTINUOUS_CV;
                    if (lilvPort.has_property(Lv2World.pprop_discrete_cv))
                        rdf_port->Properties = LV2_PORT_DISCRETE_CV;
                    if (lilvPort.has_property(Lv2World.pprop_expensive))
                        rdf_port->Properties = LV2_PORT_EXPENSIVE;
                    if (lilvPort.has_property(Lv2World.pprop_strict_bounds))
                        rdf_port->Properties = LV2_PORT_STRICT_BOUNDS;
                    if (lilvPort.has_property(Lv2World.pprop_logarithmic))
                        rdf_port->Properties = LV2_PORT_LOGARITHMIC;
                    if (lilvPort.has_property(Lv2World.pprop_not_automatic))
                        rdf_port->Properties = LV2_PORT_NOT_AUTOMATIC;
                    if (lilvPort.has_property(Lv2World.pprop_not_on_gui))
                        rdf_port->Properties = LV2_PORT_NOT_ON_GUI;
                    if (lilvPort.has_property(Lv2World.pprop_trigger))
                        rdf_port->Properties = LV2_PORT_TRIGGER;

                    if (lilvPort.has_property(Lv2World.reportsLatency))
                        rdf_port->Designation = LV2_PORT_LATENCY;
                }

                // --------------------------------------
                // Set Port Designation
                {
                    Lilv::Nodes designationNodes(lilvPort.get_value(Lv2World.designation));

                    if (designationNodes.size() > 0)
                    {
                        const char* const designation = designationNodes.get_first().as_string();

                        if (strcmp(designation, LV2_CORE__latency) == 0)
                            rdf_port->Designation = LV2_PORT_LATENCY;
                        else if (strcmp(designation, LV2_TIME__bar) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BAR;
                        else if (strcmp(designation, LV2_TIME__barBeat) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BAR_BEAT;
                        else if (strcmp(designation, LV2_TIME__beat) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BEAT;
                        else if (strcmp(designation, LV2_TIME__beatUnit) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BEAT_UNIT;
                        else if (strcmp(designation, LV2_TIME__beatsPerBar) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BEATS_PER_BAR;
                        else if (strcmp(designation, LV2_TIME__beatsPerMinute) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_BEATS_PER_MINUTE;
                        else if (strcmp(designation, LV2_TIME__frame) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_FRAME;
                        else if (strcmp(designation, LV2_TIME__framesPerSecond) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_FRAMES_PER_SECOND;
                        else if (strcmp(designation, LV2_TIME__position) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_POSITION;
                        else if (strcmp(designation, LV2_TIME__speed) == 0)
                            rdf_port->Designation = LV2_PORT_TIME_SPEED;
                        else if (strncmp(designation, LV2_PARAMETERS_PREFIX, strlen(LV2_PARAMETERS_PREFIX)) == 0)
                            pass();
                        else if (strncmp(designation, LV2_PORT_GROUPS_PREFIX, strlen(LV2_PORT_GROUPS_PREFIX)) == 0)
                            pass();
                        else
                            qWarning("lv2_rdf_new(%s) - got unknown Port Designation '%s'", URI, designation);
                    }
                }

                // --------------------------------------
                // Set Port Information
                {
                    rdf_port->Name   = strdup(Lilv::Node(lilvPort.get_name()).as_string());
                    rdf_port->Symbol = strdup(Lilv::Node(lilvPort.get_symbol()).as_string());
                }

                // --------------------------------------
                // Set Port MIDI Map
                {
                    Lilv::Nodes midiMapNodes(lilvPort.get_value(Lv2World.mm_default_control));

                    if (midiMapNodes.size() > 0)
                    {
                        Lilv::Node midiMapNode(midiMapNodes.get_first());

                        if (midiMapNode.is_blank())
                        {
                            Lilv::Nodes midiMapTypeNodes(Lv2World.find_nodes(midiMapNode, Lv2World.mm_control_type, nullptr));
                            Lilv::Nodes midiMapNumberNodes(Lv2World.find_nodes(midiMapNode, Lv2World.mm_control_number, nullptr));

                            if (midiMapTypeNodes.size() == 1 && midiMapNumberNodes.size() == 1)
                            {
                                const char* const midiMapType = midiMapTypeNodes.get_first().as_string();

                                if (strcmp(midiMapType, LV2_MIDI_Map__CC) == 0)
                                    rdf_port->MidiMap.Type = LV2_PORT_MIDI_MAP_CC;
                                else if (strcmp(midiMapType, LV2_MIDI_Map__NRPN) == 0)
                                    rdf_port->MidiMap.Type = LV2_PORT_MIDI_MAP_NRPN;
                                else
                                    qWarning("lv2_rdf_new(%s) - got unknown Port Midi Map type '%s'", URI, midiMapType);

                                rdf_port->MidiMap.Number = midiMapNumberNodes.get_first().as_int();
                            }
                        }
                    }
                }

                // --------------------------------------
                // Set Port Points
                {
                    Lilv::Nodes valueNodes(lilvPort.get_value(Lv2World.value_default));

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_DEFAULT;
                        rdf_port->Points.Default = valueNodes.get_first().as_float();
                    }

                    valueNodes = lilvPort.get_value(Lv2World.value_minimum);

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_MINIMUM;
                        rdf_port->Points.Minimum = valueNodes.get_first().as_float();
                    }

                    valueNodes = lilvPort.get_value(Lv2World.value_maximum);

                    if (valueNodes.size() > 0)
                    {
                        rdf_port->Points.Hints  |= LV2_PORT_POINT_MAXIMUM;
                        rdf_port->Points.Maximum = valueNodes.get_first().as_float();
                    }
                }

                // --------------------------------------
                // Set Port Unit
                {
                    Lilv::Nodes unitTypeNodes(lilvPort.get_value(Lv2World.unit_unit));

                    if (unitTypeNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT;

                        const char* const unitType = unitTypeNodes.get_first().as_uri();

                        if (strcmp(unitType, LV2_UNITS__bar) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_BAR;
                        else if (strcmp(unitType, LV2_UNITS__beat) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_BEAT;
                        else if (strcmp(unitType, LV2_UNITS__bpm) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_BPM;
                        else if (strcmp(unitType, LV2_UNITS__cent) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_CENT;
                        else if (strcmp(unitType, LV2_UNITS__cm) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_CM;
                        else if (strcmp(unitType, LV2_UNITS__coef) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_COEF;
                        else if (strcmp(unitType, LV2_UNITS__db) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_DB;
                        else if (strcmp(unitType, LV2_UNITS__degree) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_DEGREE;
                        else if (strcmp(unitType, LV2_UNITS__frame) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_FRAME;
                        else if (strcmp(unitType, LV2_UNITS__hz) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_HZ;
                        else if (strcmp(unitType, LV2_UNITS__inch) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_INCH;
                        else if (strcmp(unitType, LV2_UNITS__khz) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_KHZ;
                        else if (strcmp(unitType, LV2_UNITS__km) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_KM;
                        else if (strcmp(unitType, LV2_UNITS__m) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_M;
                        else if (strcmp(unitType, LV2_UNITS__mhz) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MHZ;
                        else if (strcmp(unitType, LV2_UNITS__midiNote) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MIDINOTE;
                        else if (strcmp(unitType, LV2_UNITS__mile) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MILE;
                        else if (strcmp(unitType, LV2_UNITS__min) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MIN;
                        else if (strcmp(unitType, LV2_UNITS__mm) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MM;
                        else if (strcmp(unitType, LV2_UNITS__ms) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_MS;
                        else if (strcmp(unitType, LV2_UNITS__oct) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_OCT;
                        else if (strcmp(unitType, LV2_UNITS__pc) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_PC;
                        else if (strcmp(unitType, LV2_UNITS__s) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_S;
                        else if (strcmp(unitType, LV2_UNITS__semitone12TET) == 0)
                            rdf_port->Unit.Type = LV2_UNIT_SEMITONE;
                        else
                            qWarning("lv2_rdf_new(%s) - got unknown Unit type '%s'", URI, unitType);
                    }

                    Lilv::Nodes unitNameNodes(lilvPort.get_value(Lv2World.unit_name));

                    if (unitNameNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_NAME;
                        rdf_port->Unit.Name   = strdup(unitNameNodes.get_first().as_string());
                    }

                    Lilv::Nodes unitRenderNodes(lilvPort.get_value(Lv2World.unit_render));

                    if (unitRenderNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_RENDER;
                        rdf_port->Unit.Render = strdup(unitRenderNodes.get_first().as_string());
                    }

                    Lilv::Nodes unitSymbolNodes(lilvPort.get_value(Lv2World.unit_symbol));

                    if (unitSymbolNodes.size() > 0)
                    {
                        rdf_port->Unit.Hints |= LV2_PORT_UNIT_SYMBOL;
                        rdf_port->Unit.Symbol = strdup(unitSymbolNodes.get_first().as_string());
                    }
                }

                // --------------------------------------
                // Set Port Scale Points

                Lilv::ScalePoints lilvScalePoints(lilvPort.get_scale_points());

                if (lilvScalePoints.size() > 0)
                {
                    rdf_port->ScalePointCount = lilvScalePoints.size();
                    rdf_port->ScalePoints = new LV2_RDF_PortScalePoint[rdf_port->ScalePointCount];

                    uint32_t h = 0;
                    LILV_FOREACH(scale_points, j, lilvScalePoints)
                    {
                        Lilv::ScalePoint lilvScalePoint(lilvScalePoints.get(j));
                        LV2_RDF_PortScalePoint* const rdf_scalePoint = &rdf_port->ScalePoints[h++];

                        rdf_scalePoint->Label = strdup(Lilv::Node(lilvScalePoint.get_label()).as_string());
                        rdf_scalePoint->Value = Lilv::Node(lilvScalePoint.get_value()).as_float();
                    }
                }
            }
        }

#if 0
        // --------------------------------------------------
        // Set Plugin Presets (TODO)
        {
            Lilv::Nodes presetNodes(lilvPlugin.get_related(Lv2World.preset_preset));

            if (presetNodes.size() > 0)
            {
                // create a list of preset URIs (for checking appliesTo, sorting and uniqueness)
                QStringList presetListURIs;

                LILV_FOREACH(nodes, j, presetNodes)
                {
                    Lilv::Node presetNode(presetNodes.get(j));
                    // FIXME - check appliesTo

                    QString presetURI(presetNode.as_uri());

                    if (! presetListURIs.contains(presetURI))
                        presetListURIs.append(presetURI);
                }

                presetListURIs.sort();

                // create presets with unique URIs
                rdf_descriptor->PresetCount = presetListURIs.count();
                rdf_descriptor->Presets = new LV2_RDF_Preset[rdf_descriptor->PresetCount];

                // set preset data
                LILV_FOREACH(nodes, j, presetNodes)
                {
                    Lilv::Node presetNode(presetNodes.get(j));
                    Lv2World.load_resource(presetNode);

                    LV2_URI presetURI = presetNode.as_uri();
                    int32_t index = presetListURIs.indexOf(QString(presetURI));

                    if (index < 0)
                        continue;

                    LV2_RDF_Preset* const rdf_preset = &rdf_descriptor->Presets[index];

                    // --------------------------------------
                    // Set Preset Information
                    {
                        rdf_preset->URI = strdup(presetURI);

                        Lilv::Nodes presetLabelNodes(Lv2World.find_nodes(presetNode, Lv2World.rdfs_label, nullptr));

                        if (presetLabelNodes.size() > 0)
                            rdf_preset->Label = strdup(presetLabelNodes.get_first().as_string());
                    }

                    // --------------------------------------
                    // Set Preset Ports
                    {
                        Lilv::Nodes presetPortNodes(Lv2World.find_nodes(presetNode, Lv2World.port, nullptr));

                        if (presetPortNodes.size() > 0)
                        {
                            rdf_preset->PortCount = presetPortNodes.size();
                            rdf_preset->Ports = new LV2_RDF_PresetPort[rdf_preset->PortCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, presetPortNodes)
                            {
                                Lilv::Node presetPortNode(presetPortNodes.get(k));

                                Lilv::Nodes presetPortSymbolNodes(Lv2World.find_nodes(presetPortNode, Lv2World.symbol, nullptr));
                                Lilv::Nodes presetPortValueNodes(Lv2World.find_nodes(presetPortNode, Lv2World.preset_value, nullptr));

                                if (presetPortSymbolNodes.size() == 1 && presetPortValueNodes.size() == 1)
                                {
                                    LV2_RDF_PresetPort* const rdf_presetPort = &rdf_preset->Ports[h++];
                                    rdf_presetPort->Symbol = strdup(presetPortSymbolNodes.get_first().as_string());
                                    rdf_presetPort->Value  = presetPortValueNodes.get_first().as_float();
                                }
                            }
                        }
                    }

                    // --------------------------------------
                    // Set Preset States
                    {
                        Lilv::Nodes presetStateNodes(Lv2World.find_nodes(presetNode, Lv2World.state_state, nullptr));

                        if (presetStateNodes.size() > 0)
                        {
                            rdf_preset->StateCount = presetStateNodes.size();
                            rdf_preset->States = new LV2_RDF_PresetState[rdf_preset->StateCount];

                            uint32_t h = 0;
                            LILV_FOREACH(nodes, k, presetStateNodes)
                            {
                                Lilv::Node presetStateNode(presetStateNodes.get(k));

                                if (presetStateNode.is_blank())
                                {
                                    // TODO
                                    LV2_RDF_PresetState* const rdf_presetState = &rdf_preset->States[h++];
                                    rdf_presetState->Type = LV2_PRESET_STATE_NULL;
                                    rdf_presetState->Key  = nullptr;
                                }
                            }
                        }
                    }
                }
            }
        }
#endif

        // --------------------------------------------------
        // Set Plugin Features
        {
            Lilv::Nodes lilvFeatureNodes(lilvPlugin.get_supported_features());

            if (lilvFeatureNodes.size() > 0)
            {
                Lilv::Nodes lilvFeatureNodesR(lilvPlugin.get_required_features());

                rdf_descriptor->FeatureCount = lilvFeatureNodes.size();
                rdf_descriptor->Features = new LV2_RDF_Feature[rdf_descriptor->FeatureCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, lilvFeatureNodes)
                {
                    Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(j));

                    LV2_RDF_Feature* const rdf_feature = &rdf_descriptor->Features[h++];
                    rdf_feature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                    rdf_feature->URI  = strdup(lilvFeatureNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin Extensions
        {
            Lilv::Nodes lilvExtensionDataNodes(lilvPlugin.get_extension_data());

            if (lilvExtensionDataNodes.size() > 0)
            {
                rdf_descriptor->ExtensionCount = lilvExtensionDataNodes.size();
                rdf_descriptor->Extensions = new LV2_URI[rdf_descriptor->ExtensionCount];

                uint32_t h = 0;
                LILV_FOREACH(nodes, j, lilvExtensionDataNodes)
                {
                    Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(j));

                    LV2_URI* const rdf_extension = &rdf_descriptor->Extensions[h++];
                    *rdf_extension = strdup(lilvExtensionDataNode.as_uri());
                }
            }
        }

        // --------------------------------------------------
        // Set Plugin UIs
        {
            Lilv::UIs lilvUIs(lilvPlugin.get_uis());

            if (lilvUIs.size() > 0)
            {
                rdf_descriptor->UICount = lilvUIs.size();
                rdf_descriptor->UIs = new LV2_RDF_UI[rdf_descriptor->UICount];

                uint32_t h = 0;
                LILV_FOREACH(uis, j, lilvUIs)
                {
                    Lilv::UI lilvUI(lilvUIs.get(j));
                    LV2_RDF_UI* const rdf_ui = &rdf_descriptor->UIs[h++];

                    // --------------------------------------
                    // Set UI Type
                    {
                        if (lilvUI.is_a(Lv2World.ui_gtk2))
                            rdf_ui->Type = LV2_UI_GTK2;
                        else if (lilvUI.is_a(Lv2World.ui_qt4))
                            rdf_ui->Type = LV2_UI_QT4;
                        else if (lilvUI.is_a(Lv2World.ui_cocoa))
                            rdf_ui->Type = LV2_UI_COCOA;
                        else if (lilvUI.is_a(Lv2World.ui_windows))
                            rdf_ui->Type = LV2_UI_WINDOWS;
                        else if (lilvUI.is_a(Lv2World.ui_x11))
                            rdf_ui->Type = LV2_UI_X11;
                        else if (lilvUI.is_a(Lv2World.ui_external))
                            rdf_ui->Type = LV2_UI_EXTERNAL;
                        else if (lilvUI.is_a(Lv2World.ui_external_old))
                            rdf_ui->Type = LV2_UI_OLD_EXTERNAL;
                        else
                            qWarning("lv2_rdf_new(%s) - got unknown UI type '%s'", URI, lilvUI.get_uri().as_uri());
                    }

                    // --------------------------------------
                    // Set UI Information
                    {
                        rdf_ui->URI    = strdup(lilvUI.get_uri().as_uri());
                        rdf_ui->Binary = strdup(lilv_uri_to_path(lilvUI.get_binary_uri().as_string()));
                        rdf_ui->Bundle = strdup(lilv_uri_to_path(lilvUI.get_bundle_uri().as_string()));
                    }

                    // --------------------------------------
                    // Set UI Features
                    {
                        Lilv::Nodes lilvFeatureNodes(lilvUI.get_supported_features());

                        if (lilvFeatureNodes.size() > 0)
                        {
                            Lilv::Nodes lilvFeatureNodesR(lilvUI.get_required_features());

                            rdf_ui->FeatureCount = lilvFeatureNodes.size();
                            rdf_ui->Features = new LV2_RDF_Feature[rdf_ui->FeatureCount];

                            uint32_t x = 0;
                            LILV_FOREACH(nodes, k, lilvFeatureNodes)
                            {
                                Lilv::Node lilvFeatureNode(lilvFeatureNodes.get(k));

                                LV2_RDF_Feature* const rdf_feature = &rdf_ui->Features[x++];
                                rdf_feature->Type = lilvFeatureNodesR.contains(lilvFeatureNode) ? LV2_FEATURE_REQUIRED : LV2_FEATURE_OPTIONAL;
                                rdf_feature->URI  = strdup(lilvFeatureNode.as_uri());
                            }
                        }
                    }

                    // --------------------------------------
                    // Set UI Extensions
                    {
                        Lilv::Nodes lilvExtensionDataNodes(lilvUI.get_extension_data());

                        if (lilvExtensionDataNodes.size() > 0)
                        {
                            rdf_ui->ExtensionCount = lilvExtensionDataNodes.size();
                            rdf_ui->Extensions = new LV2_URI[rdf_ui->ExtensionCount];

                            uint32_t x = 0;
                            LILV_FOREACH(nodes, k, lilvExtensionDataNodes)
                            {
                                Lilv::Node lilvExtensionDataNode(lilvExtensionDataNodes.get(k));

                                LV2_URI* const rdf_extension = &rdf_ui->Extensions[x++];
                                *rdf_extension = strdup(lilvExtensionDataNode.as_uri());
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
    if (strcmp(uri, LV2_UI__fixedSize) == 0)
        return true;
    if (strcmp(uri, LV2_UI__noUserResize) == 0)
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
    if (strcmp(uri, LV2_UI__makeResident) == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_URI) == 0)
        return true;
    if (strcmp(uri, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
        return true;
    return false;
}

static inline
LV2_URI get_lv2_ui_uri(const LV2_Property type)
{
    switch (type)
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
        return "UI URI type not supported";
    }
}

#endif // CARLA_LV2_INCLUDES_H

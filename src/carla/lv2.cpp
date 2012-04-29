/*
 * JACK Backend code for Carla
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

#ifdef BUILD_BRIDGE
#error Cannot use bridge for lv2 plugins
#endif

#include "carla_plugin.h"

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/atom-util.h"
#include "lv2/data-access.h"
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/port-props.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"

#include "lv2/lv2-miditype.h"
#include "lv2/lv2-midifunctions.h"
#include "lv2/lv2_rtmempool.h"
#include "lv2/lv2_external_ui.h"

#include "lilv/lilvmm.hpp"
#include "lv2_rdf.h"

extern "C" {
#include "lv2-rtmempool/rtmempool.h"
}

#define NS_dct  "http://purl.org/dc/terms/"
#define NS_rdf  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_llmm "http://ll-plugins.nongnu.org/lv2/ext/midimap#"

#define LV2_MIDI_LL__MidiPort "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"

#if 1
int main() { return 0; }
#endif

#include <QtGui/QDialog>
#include <QtGui/QLayout>

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x7FFF; // 32767

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x1000;
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x2000;

// extra parameter hints
const unsigned int PARAMETER_IS_TRIGGER          = 0x100;
const unsigned int PARAMETER_HAS_STRICT_BOUNDS   = 0x200;

// feature ids
const uint32_t lv2_feature_id_uri_map         = 0;
const uint32_t lv2_feature_id_urid_map        = 1;
const uint32_t lv2_feature_id_urid_unmap      = 2;
const uint32_t lv2_feature_id_event           = 3;
const uint32_t lv2_feature_id_rtmempool       = 4;
const uint32_t lv2_feature_id_data_access     = 5;
const uint32_t lv2_feature_id_instance_access = 6;
const uint32_t lv2_feature_id_ui_parent       = 7;
const uint32_t lv2_feature_id_ui_resize       = 8;
const uint32_t lv2_feature_id_external_ui     = 9;
const uint32_t lv2_feature_id_external_ui_old = 10;
const uint32_t lv2_feature_count              = 11;

// event data/types
const unsigned int CARLA_EVENT_DATA_ATOM    = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT   = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL = 0x04;
const unsigned int CARLA_EVENT_TYPE_MIDI    = 0x10;
const unsigned int CARLA_EVENT_TYPE_TIME    = 0x20;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL           = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK     = 1;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE  = 2;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING    = 3;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT     = 4;
const uint32_t CARLA_URI_MAP_ID_TIME_POSITION  = 5;
const uint32_t CARLA_URI_MAP_ID_COUNT          = 6;

enum Lv2ParameterDataType {
    LV2_PARAMETER_TYPE_CONTROL,
    LV2_PARAMETER_TYPE_SOMETHING_ELSE_HERE
};

struct EventData {
    unsigned int types;
    jack_port_t* port;
    union {
        LV2_Atom_Sequence* a;
        LV2_Event_Buffer* e;
        LV2_MIDI* m;
    } buffer;
};

struct PluginEventData {
    uint32_t count;
    EventData* data;
};

struct Lv2ParameterData {
    Lv2ParameterDataType type;
    union {
        float control;
    };
};

const char* lv2bridge2str(LV2_Property type)
{
    switch (type)
    {
    case LV2_UI_GTK2:
        return carla_options.bridge_lv2gtk2;
    case LV2_UI_QT4:
        return carla_options.bridge_lv2qt4;
    case LV2_UI_X11:
        return carla_options.bridge_lv2x11;
    default:
        return nullptr;
    }
}

class Lv2WorldClass : public Lilv::World
{
public:
    Lv2WorldClass() : Lilv::World(),
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

        extension_data      (new_uri(LV2_CORE_PREFIX "extensionData")), // FIXME - typo in lv2 ?

        value_default       (new_uri(LV2_CORE__default)),
        value_minimum       (new_uri(LV2_CORE__minimum)),
        value_maximum       (new_uri(LV2_CORE__maximum)),

        atom_sequence       (new_uri(LV2_ATOM__Sequence)),
        atom_buffer_type    (new_uri(LV2_ATOM__bufferType)),
        atom_supports       (new_uri(LV2_ATOM__supports)),

        midi_event          (new_uri(LV2_MIDI__MidiEvent)),

        time_position       (new_uri(LV2_TIME__Position)),

        mm_default_controller (new_uri(NS_llmm "defaultMidiController")),
        mm_controller_type    (new_uri(NS_llmm "controllerType")),
        mm_controller_number  (new_uri(NS_llmm "controllerNumber")),

        dct_replaces        (new_uri(NS_dct "replaces")),
        rdf_type            (new_uri(NS_rdf "type"))
    {
        initiated = false;
    }

    void Init()
    {
        if (initiated == false)
        {
            qDebug("Lv2World::Init()");
            initiated = true;
            load_all();
        }
    }

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

    // Port Unit
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

    // LV2 stuff
    Lilv::Node extension_data;

    Lilv::Node value_default;
    Lilv::Node value_minimum;
    Lilv::Node value_maximum;

    Lilv::Node atom_sequence;
    Lilv::Node atom_buffer_type;
    Lilv::Node atom_supports;

    Lilv::Node midi_event;

    Lilv::Node time_position;

    Lilv::Node mm_default_controller;
    Lilv::Node mm_controller_type;
    Lilv::Node mm_controller_number;

    // Other
    Lilv::Node dct_replaces;
    Lilv::Node rdf_type;

private:
    bool initiated;
};

static Lv2WorldClass Lv2World;

class Lv2Plugin : public CarlaPlugin
{
public:
    Lv2Plugin() :
        CarlaPlugin()
    {
        qDebug("Lv2Plugin::Lv2Plugin()");
        m_type = PLUGIN_LV2;

        ain_rindexes  = nullptr;
        aout_rindexes = nullptr;
        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        ui.lib = nullptr;
        ui.handle = nullptr;
        ui.descriptor = nullptr;
        ui.rdf_descriptor = nullptr;

        gui.type = GUI_NONE;
        gui.visible = false;
        gui.resizable = false;
        gui.width = 0;
        gui.height = 0;

        // Fill pre-set URI keys
        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            custom_uri_ids.append(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;

        Lv2World.Init();
    }

    virtual ~Lv2Plugin()
    {
        qDebug("Lv2Plugin::~Lv2Plugin()");

        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            switch(gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_X11:
                break;

            case GUI_EXTERNAL_OSC:
                if (osc.data.target)
                {
                    osc_send_hide(&osc.data);
                    osc_send_quit(&osc.data);
                }

                if (osc.thread)
                {
                    // Wait a bit first, try safe quit else force kill
                    if (osc.thread->isRunning())
                    {
                        if (osc.thread->wait(2000) == false)
                            osc.thread->quit();

                        if (osc.thread->isRunning() && osc.thread->wait(1000) == false)
                        {
                            qWarning("Failed to properly stop LV2 OSC-GUI thread");
                            osc.thread->terminate();
                        }
                    }

                    delete osc.thread;
                }

                osc_clear_data(&osc.data);

                break;

            case GUI_EXTERNAL_LV2:
                if (gui.visible && ui.widget)
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);

                break;

            default:
                break;
            }

            if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
                ui.descriptor->cleanup(ui.handle);

            ui.lib = nullptr;
            ui.handle = nullptr;
            ui.descriptor = nullptr;
            ui.rdf_descriptor = nullptr;

            if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
                delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

            if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
                delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

            if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
            {
                free((void*)((lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data)->plugin_human_id);
                delete (lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data;
            }

            ui_lib_close();
        }

        if (handle && descriptor->deactivate && m_active_before)
            descriptor->deactivate(handle);

        if (handle && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
            delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

        if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
            delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

        if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
            delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

        if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
            delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

        if (features[lv2_feature_id_rtmempool] && features[lv2_feature_id_rtmempool]->data)
            delete (lv2_rtsafe_memory_pool_provider*)features[lv2_feature_id_rtmempool]->data;

        for (uint32_t i=0; i < lv2_feature_count; i++)
        {
            if (features[i])
                delete features[i];
        }

        for (int i=0; i < custom_uri_ids.count(); i++)
        {
            if (custom_uri_ids[i])
                free((void*)custom_uri_ids[i]);
        }

        custom_uri_ids.clear();
    }

    virtual PluginCategory category()
    {
        LV2_Property Category = rdf_descriptor->Type;

        if (LV2_IS_DELAY(Category))
            return PLUGIN_CATEGORY_DELAY;
        else if (LV2_IS_DISTORTION(Category))
            return PLUGIN_CATEGORY_OUTRO;
        else if (LV2_IS_DYNAMICS(Category))
            return PLUGIN_CATEGORY_DYNAMICS;
        else if (LV2_IS_EQ(Category))
            return PLUGIN_CATEGORY_EQ;
        else if (LV2_IS_FILTER(Category))
            return PLUGIN_CATEGORY_FILTER;
        else if (LV2_IS_GENERATOR(Category))
            return PLUGIN_CATEGORY_SYNTH;
        else if (LV2_IS_MODULATOR(Category))
            return PLUGIN_CATEGORY_MODULATOR;
        else if (LV2_IS_REVERB(Category))
            return PLUGIN_CATEGORY_DELAY;
        else if (LV2_IS_SIMULATOR(Category))
            return PLUGIN_CATEGORY_OUTRO;
        else if (LV2_IS_SPATIAL(Category))
            return PLUGIN_CATEGORY_OUTRO;
        else if (LV2_IS_SPECTRAL(Category))
            return PLUGIN_CATEGORY_UTILITY;
        else if (LV2_IS_UTILITY(Category))
            return PLUGIN_CATEGORY_UTILITY;

        return get_category_from_name(m_name);
    }

    virtual long unique_id()
    {
        return rdf_descriptor->UniqueID;
    }

    virtual uint32_t min_count()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i < evin.count; i++)
        {
            if (evin.data[i].types & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    virtual uint32_t mout_count()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].types & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    virtual uint32_t param_scalepoint_count(uint32_t param_id)
    {
        int32_t rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[rindex].ScalePointCount;
    }

    virtual double get_parameter_value(uint32_t param_id)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
        {
            double value = lv2param[param_id].control;
            if (1) // FIXME - only if output and strict bounds
                fix_parameter_value(value, param.ranges[param_id]);
            return value;
        }
        default:
            return 0.0;
        }
    }

    virtual double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Value;
    }

    virtual void get_label(char* buf_str)
    {
        if (rdf_descriptor->URI)
            strncpy(buf_str, rdf_descriptor->URI, STR_MAX);
        else
            *buf_str = 0;
    }

    virtual void get_maker(char* buf_str)
    {
        if (rdf_descriptor->Author)
            strncpy(buf_str, rdf_descriptor->Author, STR_MAX);
        else
            *buf_str = 0;
    }

    virtual void get_copyright(char* buf_str)
    {
        if (rdf_descriptor->License)
            strncpy(buf_str, rdf_descriptor->License, STR_MAX);
        else
            *buf_str = 0;
    }

    virtual void get_real_name(char* buf_str)
    {
        if (rdf_descriptor->Name)
            strncpy(buf_str, rdf_descriptor->Name, STR_MAX);
        else
            *buf_str = 0;
    }

    virtual void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Name, STR_MAX);
    }

    virtual void get_parameter_symbol(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Symbol, STR_MAX);
    }

    virtual void get_parameter_label(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;

        LV2_RDF_Port* Port = &rdf_descriptor->Ports[rindex];

        if (LV2_HAVE_UNIT_SYMBOL(Port->Unit.Hints))
            strncpy(buf_str, Port->Unit.Symbol, STR_MAX);

        else if (LV2_HAVE_UNIT(Port->Unit.Hints))
        {
            switch (Port->Unit.Type)
            {
            case LV2_UNIT_BAR:
                strncpy(buf_str, "bars", STR_MAX);
                return;
            case LV2_UNIT_BEAT:
                strncpy(buf_str, "beats", STR_MAX);
                return;
            case LV2_UNIT_BPM:
                strncpy(buf_str, "BPM", STR_MAX);
                return;
            case LV2_UNIT_CENT:
                strncpy(buf_str, "ct", STR_MAX);
                return;
            case LV2_UNIT_CM:
                strncpy(buf_str, "cm", STR_MAX);
                return;
            case LV2_UNIT_COEF:
                strncpy(buf_str, "(coef)", STR_MAX);
                return;
            case LV2_UNIT_DB:
                strncpy(buf_str, "dB", STR_MAX);
                return;
            case LV2_UNIT_DEGREE:
                strncpy(buf_str, "deg", STR_MAX);
                return;
            case LV2_UNIT_FRAME:
                strncpy(buf_str, "frames", STR_MAX);
                return;
            case LV2_UNIT_HZ:
                strncpy(buf_str, "Hz", STR_MAX);
                return;
            case LV2_UNIT_INCH:
                strncpy(buf_str, "in", STR_MAX);
                return;
            case LV2_UNIT_KHZ:
                strncpy(buf_str, "kHz", STR_MAX);
                return;
            case LV2_UNIT_KM:
                strncpy(buf_str, "km", STR_MAX);
                return;
            case LV2_UNIT_M:
                strncpy(buf_str, "m", STR_MAX);
                return;
            case LV2_UNIT_MHZ:
                strncpy(buf_str, "MHz", STR_MAX);
                return;
            case LV2_UNIT_MIDINOTE:
                strncpy(buf_str, "note", STR_MAX);
                return;
            case LV2_UNIT_MILE:
                strncpy(buf_str, "mi", STR_MAX);
                return;
            case LV2_UNIT_MIN:
                strncpy(buf_str, "min", STR_MAX);
                return;
            case LV2_UNIT_MM:
                strncpy(buf_str, "mm", STR_MAX);
                return;
            case LV2_UNIT_MS:
                strncpy(buf_str, "ms", STR_MAX);
                return;
            case LV2_UNIT_OCT:
                strncpy(buf_str, "oct", STR_MAX);
                return;
            case LV2_UNIT_PC:
                strncpy(buf_str, "%", STR_MAX);
                return;
            case LV2_UNIT_S:
                strncpy(buf_str, "s", STR_MAX);
                return;
            case LV2_UNIT_SEMITONE:
                strncpy(buf_str, "semi", STR_MAX);
                return;
            }
        }
        *buf_str = 0;
    }

    virtual void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Label, STR_MAX);
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        info->type      = gui.type;
        info->resizable = gui.resizable;
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            fix_parameter_value(value, param.ranges[param_id]);
            lv2param[param_id].control = value;
            break;
        default:
            break;
        }

        if (gui_send /*&& gui.visible*/)
        {
            switch (gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_X11:
            case GUI_EXTERNAL_LV2:
                if (ui.handle && ui.descriptor && ui.descriptor->port_event)
                {
                    float fvalue = value;
                    ui.descriptor->port_event(ui.handle, param.data[param_id].rindex, sizeof(float), 0, &fvalue);
                }
                break;

            case GUI_EXTERNAL_OSC:
                osc_send_control(&osc.data, param.data[param_id].rindex, value);
                break;

            default:
                break;
            }
        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    virtual void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send)
    {
        CarlaPlugin::set_custom_data(dtype, key, value, gui_send);

        if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
        {
            LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (state)
                state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
        }
    }

    virtual void set_gui_data(int, void* ptr)
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            if (ui.widget)
            {
                QDialog* qtPtr  = (QDialog*)ptr;
                QWidget* widget = (QWidget*)ui.widget;

                qtPtr->layout()->addWidget(widget);
                widget->adjustSize();
                widget->setParent(qtPtr);
                widget->show();
            }
            break;

        case GUI_INTERNAL_X11:
            if (ui.descriptor)
            {
                QDialog* qtPtr  = (QDialog*)ptr;
                features[lv2_feature_id_ui_parent]->data = (void*)qtPtr->winId();

                ui.handle = ui.descriptor->instantiate(ui.descriptor,
                                                       descriptor->URI,
                                                       ui.rdf_descriptor->Bundle,
                                                       carla_lv2_ui_write_function,
                                                       this,
                                                       &ui.widget,
                                                       features);
                update_ui_ports();
            }
            break;

        default:
            break;
        }
    }

    virtual void show_gui(bool yesno)
    {
        // FIXME - is gui.visible needed at all?
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            gui.visible = yesno;
            break;

        case GUI_INTERNAL_X11:
            gui.visible = yesno;

            if (gui.visible && gui.width > 0 && gui.height > 0)
                callback_action(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);

            break;

        case GUI_EXTERNAL_OSC:
            if (yesno)
            {
                osc.thread->start();
            }
            else
            {
                osc_send_hide(&osc.data);
                osc_send_quit(&osc.data);
                osc_clear_data(&osc.data);
            }
            break;

        case GUI_EXTERNAL_LV2:
            if (ui.handle == nullptr)
                reinit_external_ui();

            if (ui.handle && ui.widget)
            {
                if (yesno)
                {
                    LV2_EXTERNAL_UI_SHOW((lv2_external_ui*)ui.widget);
                    gui.visible = true;
                }
                else
                {
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);
                    gui.visible = false;

                    //if (ui.descriptor->cleanup)
                    //    ui.descriptor->cleanup(ui.handle);

                    //ui.handle = nullptr;
                }
            }
            else
            {
                // failed to init UI
                gui.visible = false;
                callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
            }
            break;

        default:
            break;
        }
    }

    virtual void idle_gui()
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
        case GUI_INTERNAL_X11:
        case GUI_EXTERNAL_LV2:
            if (ui.handle && ui.descriptor)
            {
                if (ui.descriptor->port_event)
                {
                    for (uint32_t i=0; i < param.count; i++)
                    {
                        if (param.data[i].type == PARAMETER_OUTPUT && (param.data[i].hints & PARAMETER_IS_AUTOMABLE) > 0)
                        {
                            float value = get_parameter_value(i);
                            ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
                        }
                    }
                }

                if (gui.type == GUI_EXTERNAL_LV2)
                    LV2_EXTERNAL_UI_RUN((lv2_external_ui*)ui.widget);
            }
            break;

        default:
            break;
        }
    }

    virtual void reload()
    {
        qDebug("Lv2Plugin::reload()");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        if (_id >= 0)
            remove_from_jack();

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, cv_ins, cv_outs, ev_ins, ev_outs, params, j;
        ains = aouts = cv_ins = cv_outs = ev_ins = ev_outs = params = 0;

        const uint32_t PortCount = rdf_descriptor->PortCount;
        unsigned int event_data_type = 0;

        for (uint32_t i=0; i<PortCount; i++)
        {
            const LV2_Property PortType = rdf_descriptor->Ports[i].Type;
            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ains += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    aouts += 1;
            }
            else if (LV2_IS_PORT_CV(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    cv_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    cv_outs += 1;
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(PortType))
            {
                qDebug("Atom Sequence port found, index: %i, name: %s", i, rdf_descriptor->Ports[i].Name);
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_ATOM;
            }
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_EVENT;
            }
            else if (LV2_IS_PORT_MIDI_LL(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_MIDI_LL;
            }
            else if (LV2_IS_PORT_CONTROL(PortType))
                params += 1;
            else
                qDebug("Unknown port type found, index: %i, name: %s", i, rdf_descriptor->Ports[i].Name);
        }

        qDebug("Lv2Plugin::reload() - %i | %i | %i", ains, aouts, params);

        //        if (params == 0 && (hints & PLUGIN_HAS_EXTENSION_DYNPARAM) > 0)
        //        {
        //            dynparam_plugin = (lv2dynparam_plugin_callbacks*)descriptor->extension_data(LV2DYNPARAM_URI);
        //            if (dynparam_plugin)
        //                dynparam_plugin->host_attach(handle, &dynparam_host, this);
        //        }

        if (ains > 0)
        {
            ain.ports    = new jack_port_t*[ains];
            ain_rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new jack_port_t*[aouts];
            aout_rindexes = new uint32_t[aouts];
        }

        if (ev_ins > 0)
        {
            evin.data = new EventData[ev_ins];

            for (j=0; j < ev_ins; j++)
            {
                evin.data[j].port = nullptr;

                if (event_data_type == CARLA_EVENT_DATA_ATOM)
                {
                    evin.data[j].types    = CARLA_EVENT_DATA_ATOM;
                    evin.data[j].buffer.a = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evin.data[j].buffer.a->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evin.data[j].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evin.data[j].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                    evin.data[j].buffer.a->body.pad  = 0;
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evin.data[j].types    = CARLA_EVENT_DATA_EVENT;
                    evin.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evin.data[j].types    = CARLA_EVENT_DATA_MIDI_LL;
                    evin.data[j].buffer.m = new LV2_MIDI;
                    evin.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evin.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evin.data[j].types  = 0;
            }
        }

        if (ev_outs > 0)
        {
            evout.data = new EventData[ev_outs];

            for (j=0; j < ev_outs; j++)
            {
                evout.data[j].port = nullptr;

                if (event_data_type == CARLA_EVENT_DATA_ATOM)
                {
                    evout.data[j].types    = CARLA_EVENT_DATA_ATOM;
                    evout.data[j].buffer.a = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evout.data[j].buffer.a->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evout.data[j].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evout.data[j].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                    evout.data[j].buffer.a->body.pad  = 0;
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evout.data[j].types    = CARLA_EVENT_DATA_EVENT;
                    evout.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evout.data[j].types    = CARLA_EVENT_DATA_MIDI_LL;
                    evout.data[j].buffer.m = new LV2_MIDI;
                    evout.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evout.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evout.data[j].types  = 0;
            }
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            lv2param     = new Lv2ParameterData[params];
        }

        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];
        bool needs_cin  = false;
        bool needs_cout = false;

        for (uint32_t i=0; i < PortCount; i++)
        {
            const LV2_Property PortType  = rdf_descriptor->Ports[i].Type;

            if (LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_ATOM_SEQUENCE(PortType) || LV2_IS_PORT_CV(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carla_options.global_jack_client)
                {
                    strcpy(port_name, m_name);
                    strcat(port_name, ":");
                    strncat(port_name, rdf_descriptor->Ports[i].Name, port_name_size/2);
                }
                else
#endif
                    strncpy(port_name, rdf_descriptor->Ports[i].Name, port_name_size/2);
            }

            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                    ain_rindexes[j] = i;
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    aout_rindexes[j] = i;
                    needs_cin = true;
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else
                    qWarning("WARNING - Got a broken Port (CV, but not input or output)");

                descriptor->connect_port(handle, i, nullptr);
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.a);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI)
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_TIME)
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.a);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI)
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_TIME)
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Atom Sequence, but not input or output)");
            }
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.e);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI)
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_TIME)
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.e);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI)
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_TIME)
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Event, but not input or output)");
            }
            else if (LV2_IS_PORT_MIDI_LL(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.m);

                    evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                    evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.m);

                    evout.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                    evout.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                }
                else
                    qWarning("WARNING - Got a broken Port (Midi, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(PortType))
            {
                const LV2_Property PortProps = rdf_descriptor->Ports[i].Properties;
                const LV2_RDF_PortPoints PortPoints = rdf_descriptor->Ports[i].Points;

                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midi_channel = 0;
                param.data[j].midi_cc = -1;

                double min, max, def, step, step_small, step_large;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(PortPoints.Hints))
                    min = PortPoints.Minimum;
                else
                    min = 0.0;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(PortPoints.Hints))
                    max = PortPoints.Maximum;
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(PortPoints.Hints))
                    def = PortPoints.Default;
                else
                {
                    // no default value
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (max - min <= 0.0)
                {
                    qWarning("Broken plugin parameter -> max - min <= 0");
                    max = min + 0.1;
                }

                if (LV2_IS_PORT_SAMPLE_RATE(PortProps))
                {
                    double sample_rate = get_sample_rate();
                    min *= sample_rate;
                    max *= sample_rate;
                    def *= sample_rate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_INTEGER(PortProps))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                }
                else if (LV2_IS_PORT_TOGGLED(PortProps))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }

                if (LV2_IS_PORT_INPUT(PortType))
                {
                    param.data[j].type = PARAMETER_INPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needs_cin = true;

                    // MIDI CC value
                    LV2_RDF_PortMidiMap* PortMidiMap = &rdf_descriptor->Ports[i].MidiMap;
                    if (LV2_IS_PORT_MIDI_MAP_CC(PortMidiMap->Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(PortMidiMap->Number))
                            param.data[j].midi_cc = PortMidiMap->Number;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    param.data[j].type = PARAMETER_OUTPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;

                    if (LV2_IS_PORT_LATENCY(PortProps) == false)
                    {
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needs_cout = true;
                    }
                    else
                    {
                        // latency parameter
                        min = 0;
                        max = get_sample_rate();
                        def = 0;
                        step = 1;
                        step_small = 1;
                        step_large = 1;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(PortProps))
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_HAS_STRICT_BOUNDS(PortProps) /*|| (force_strict_bounds && LV2_IS_PORT_OUTPUT(PortType))*/)
                    param.data[j].hints |= PARAMETER_HAS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_AUTOMATIC(PortProps) || LV2_IS_PORT_NOT_ON_GUI(PortProps))
                    param.data[j].hints &= ~PARAMETER_IS_ENABLED;

                if (LV2_IS_PORT_CAUSES_ARTIFACTS(PortProps) || LV2_IS_PORT_EXPENSIVE(PortProps))
                    param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].step_small = step_small;
                param.ranges[j].step_large = step_large;

                // Set LV2 params as needed
                lv2param[j].type = LV2_PARAMETER_TYPE_CONTROL;
                lv2param[j].control = def;

                descriptor->connect_port(handle, i, &lv2param[j].control);
            }
            else
                // Port Type not supported, but it's optional anyway
                descriptor->connect_port(handle, i, nullptr);
        }

        if (needs_cin)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-in");
            }
            else
#endif
                strcpy(port_name, "control-in");

            param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        if (needs_cout)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-out");
            }
            else
#endif
                strcpy(port_name, "control-out");

            param.port_cout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }

        ain.count   = ains;
        aout.count  = aouts;
        evin.count  = ev_ins;
        evout.count = ev_outs;
        param.count = params;

        reload_programs(true);

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (LV2_IS_GENERATOR(rdf_descriptor->Type))
            m_hints |= PLUGIN_IS_SYNTH;

        if (aouts > 0 && (ains == aouts || ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aouts >= 2 && aouts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client == false)
#endif
            jack_activate(jack_client);
    }

    virtual void prepare_for_save()
    {
        if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
        {
            LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (state)
                state->save(handle, carla_lv2_state_store, this, 0, features);
        }
    }

    virtual void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        uint32_t midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* ains_buffer[ain.count];
        jack_default_audio_sample_t* aouts_buffer[aout.count];
        void* evins_buffer[evin.count];
        void* evouts_buffer[evout.count];

        // different midi APIs
        uint32_t atomSequenceOffsets[evin.count];
        LV2_Event_Iterator evin_iters[evin.count];
        LV2_MIDIState evin_states[evin.count];


        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
                atomSequenceOffsets[i] = 0;
                evin.data[i].buffer.a->atom.size = 0;
                evin.data[i].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evin.data[i].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                evin.data[i].buffer.a->body.pad  = 0;
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evin.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evin.data[i].buffer.e + 1));
                lv2_event_begin(&evin_iters[i], evin.data[i].buffer.e);
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
            {
                evin_states[i].midi = evin.data[i].buffer.m;
                evin_states[i].frame_count = nframes;
                evin_states[i].position = 0;

                evin_states[i].midi->event_count = 0;
                evin_states[i].midi->size = 0;
            }

            if (evin.data[i].port)
                evins_buffer[i] = jack_port_get_buffer(evin.data[i].port, nframes);
            else
                evins_buffer[i] = nullptr;
        }

        for (i=0; i < evout.count; i++)
        {
            if (evout.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
                evout.data[i].buffer.a->atom.size = 0;
                evout.data[i].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evout.data[i].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                evout.data[i].buffer.a->body.pad  = 0;
            }
            else if (evout.data[i].types & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evout.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evout.data[i].buffer.e + 1));
            }
            else if (evout.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }

            if (evout.data[i].port)
                evouts_buffer[i] = jack_port_get_buffer(evout.data[i].port, nframes);
            else
                evouts_buffer[i] = nullptr;
        }

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            short j2 = (ain.count == 1) ? 0 : 1;

            for (k=0; k<nframes; k++)
            {
                if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                    ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                if (abs_d(ains_buffer[j2][k]) > ains_peak_tmp[1])
                    ains_peak_tmp[1] = abs_d(ains_buffer[j2][k]);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin)
        {
            void* pin_buffer = jack_port_get_buffer(param.port_cin, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            unsigned char next_bank_id = 0;
            if (midiprog.current > 0 && midiprog.count > 0)
                next_bank_id = midiprog.data[midiprog.current].bank;

            for (i=0; i < n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                jack_midi_data_t status = pin_event.buffer[0];
                unsigned char channel   = status & 0x0F;

                // Control change
                if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    jack_midi_data_t control = pin_event.buffer[1];
                    jack_midi_data_t c_value = pin_event.buffer[2];

                    // Bank Select
                    if (MIDI_IS_CONTROL_BANK_SELECT(control))
                    {
                        next_bank_id = c_value;
                        continue;
                    }

                    double value;

                    // Control GUI stuff (channel 0 only)
                    if (channel == 0)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(control) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = double(c_value)/127;
                            set_drywet(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(control) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = double(c_value)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_BALANCE(control) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = (double(c_value)-63.5)/63.5;

                            if (value < 0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0)
                            {
                                left  = (value*2)-1.0;
                                right = 1.0;
                            }
                            else
                            {
                                left  = -1.0;
                                right = 1.0;
                            }

                            set_balance_left(left, false, false);
                            set_balance_right(right, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();

                            if (m_active && m_active_before)
                            {
                                if (descriptor->deactivate)
                                    descriptor->deactivate(handle);

                                m_active_before = false;
                            }
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0 && param.data[k].midi_channel == channel && param.data[k].midi_cc == control)
                        {
                            value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;
                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    uint32_t mbank_id = next_bank_id;
                    uint32_t mprog_id = pin_event.buffer[1]; // & 0x7F;

                    // TODO ...

                    for (k=0; k < midiprog.count; k++)
                    {
                        if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                        {
                            set_midi_program(k, false, false, false, false);
                            postpone_event(PostEventMidiProgramChange, k, 0.0);
                            break;
                        }
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (evin.count > 0)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (ext_midi_notes[i].valid)
                {
                    uint8_t midi_event[4] = { 0 };
                    midi_event[0] = ext_midi_notes[i].onoff ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midi_event[1] = ext_midi_notes[i].note;
                    midi_event[2] = ext_midi_notes[i].velo;

                    // send to all midi inputs
                    for (k=0; k < evin.count; k++)
                    {
                        if (evin.data[k].types & CARLA_EVENT_TYPE_MIDI)
                        {
                            if (evin.data[k].types & CARLA_EVENT_DATA_ATOM)
                            {
                                LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evin.data[k].buffer.a) + atomSequenceOffsets[k]);
                                aev->time.frames = 0;
                                aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                                aev->body.size   = 3;
                                memcpy(LV2_ATOM_BODY(&aev->body), midi_event, 3);

                                uint32_t size            = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 3);
                                atomSequenceOffsets[k]  += size;
                                evin.data[k].buffer.a->atom.size += size;
                            }
                            else if (evin.data[k].types & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evin_iters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midi_event);

                            else if (evin.data[k].types & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evin_states[k], 0, 3, midi_event);
                        }
                    }

                    ext_midi_notes[i].valid = false;
                    midi_event_count += 1;
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (JACK)

        for (i=0; i < evin.count; i++)
        {
            if (evins_buffer[i] == nullptr)
                continue;

            jack_midi_event_t min_event;
            uint32_t n_min_events = jack_midi_get_event_count(evins_buffer[i]);

            for (k=0; k < n_min_events && midi_event_count < MAX_MIDI_EVENTS; k++)
            {
                if (jack_midi_event_get(&min_event, evins_buffer[i], k) != 0)
                    break;

                jack_midi_data_t status = min_event.buffer[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && min_event.buffer[2] == 0)
                {
                    min_event.buffer[0] -= 0x10;
                    status = min_event.buffer[0];
                }

                // write supported status types
                if (MIDI_IS_STATUS_NOTE_OFF(status) || MIDI_IS_STATUS_NOTE_ON(status) || MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) || MIDI_IS_STATUS_AFTERTOUCH(status) || MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
                        continue; // TODO
                    else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&evin_iters[i], min_event.time, 0, CARLA_URI_MAP_ID_MIDI_EVENT, min_event.size, min_event.buffer);
                    else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
                        lv2midi_put_event(&evin_states[i], min_event.time, min_event.size, min_event.buffer);

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                        postpone_event(PostEventNoteOff, min_event.buffer[1], 0.0);
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                        postpone_event(PostEventNoteOn, min_event.buffer[1], min_event.buffer[2]);
                }

                midi_event_count += 1;
            }
        } // End of MIDI Input (JACK)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                descriptor->connect_port(handle, ain_rindexes[i], ains_buffer[i]);

            for (i=0; i < aout.count; i++)
                descriptor->connect_port(handle, aout_rindexes[i], aouts_buffer[i]);

            if (descriptor->run)
                descriptor->run(handle, nframes);
        }
        else
        {
            if (m_active_before)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            double bal_rangeL, bal_rangeR;
            jack_default_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                for (k=0; k<nframes; k++)
                {
                    if ((m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0)
                    {
                        if (aout.count == 1)
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                        else
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                    }

                    if (m_hints & PLUGIN_CAN_VOLUME)
                        aouts_buffer[i][k] *= x_vol;
                }

                // Balance
                if (m_hints & PLUGIN_CAN_BALANCE)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_default_audio_sample_t)*nframes);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k<nframes; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            aouts_buffer[i][k]  = old_bal_left[k]*(1.0-bal_rangeL);
                            aouts_buffer[i][k] += aouts_buffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            aouts_buffer[i][k]  = aouts_buffer[i][k]*bal_rangeR;
                            aouts_buffer[i][k] += old_bal_left[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                if (i < 2)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                            aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                    }
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_default_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.port_cout)
        {
            void* cout_buffer = jack_port_get_buffer(param.port_cout, nframes);
            jack_midi_clear_buffer(cout_buffer);

            double value, rvalue;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT && param.data[k].midi_cc > 0)
                {
                    switch (lv2param[k].type)
                    {
                    case LV2_PARAMETER_TYPE_CONTROL:
                        value = lv2param[k].control;
                        break;
                    default:
                        value = param.ranges[k].min;
                        break;
                    }

                    rvalue = (value - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min) * 127;

                    jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                    event_buffer[0] = 0xB0 + param.data[k].midi_channel;
                    event_buffer[1] = param.data[k].midi_cc;
                    event_buffer[2] = rvalue;
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        for (i=0; i < evout.count; i++)
        {
            if (evouts_buffer[i] == nullptr)
                continue;

            jack_midi_clear_buffer(evouts_buffer[i]);

            if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
            {
                LV2_Event* ev;
                LV2_Event_Iterator iter;

                uint8_t* data;
                lv2_event_begin(&iter, evout.data[i].buffer.e);

                for (k=0; k < iter.buf->event_count; k++)
                {
                    ev = lv2_event_get(&iter, &data);
                    if (ev && data)
                        jack_midi_event_write(evouts_buffer[i], ev->frames, data, ev->size);

                    lv2_event_increment(&iter);
                }
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
            {
                LV2_MIDIState state = { evout.data[i].buffer.m, nframes, 0 };

                uint32_t event_size;
                double event_timestamp;
                unsigned char* event_data;

                while (lv2midi_get_event(&state, &event_timestamp, &event_size, &event_data) < nframes)
                {
                    jack_midi_event_write(evouts_buffer[i], event_timestamp, event_data, event_size);
                    lv2midi_step(&state);
                }
            }
        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        ains_peak[(plugin_id*2)+0]  = ains_peak_tmp[0];
        ains_peak[(plugin_id*2)+1]  = ains_peak_tmp[1];
        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    virtual void delete_buffers()
    {
        qDebug("Lv2Plugin::delete_buffers() - start");

        if (ain.count > 0)
            delete[] ain_rindexes;

        if (aout.count > 0)
            delete[] aout_rindexes;

        if (param.count > 0)
            delete[] lv2param;

        if (evin.count > 0)
        {
            for (uint32_t i=0; i < evin.count; i++)
            {
                if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
                {
                    free(evin.data[i].buffer.a);
                }
                else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
                {
                    free(evin.data[i].buffer.e);
                }
                else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evin.data[i].buffer.m->data;
                    delete evin.data[i].buffer.m;
                }
            }

            delete[] evin.data;
        }

        if (evout.count > 0)
        {
            for (uint32_t i=0; i < evout.count; i++)
            {
                if (evout.data[i].types & CARLA_EVENT_DATA_ATOM)
                {
                    free(evout.data[i].buffer.a);
                }
                else if (evout.data[i].types & CARLA_EVENT_DATA_EVENT)
                {
                    free(evout.data[i].buffer.e);
                }
                else if (evout.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evout.data[i].buffer.m->data;
                    delete evout.data[i].buffer.m;
                }
            }

            delete[] evout.data;
        }

        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        qDebug("Lv2Plugin::delete_buffers() - end");
    }

    uint32_t get_custom_uri_id(const char* uri)
    {
        qDebug("Lv2Plugin::get_custom_uri_id(%s)", uri);

        for (int i=0; i < custom_uri_ids.count(); i++)
        {
            if (custom_uri_ids[i] && strcmp(custom_uri_ids[i], uri) == 0)
                return i;
        }

        custom_uri_ids.append(strdup(uri));
        return custom_uri_ids.count()-1;
    }

    const char* get_custom_uri_string(int uri_id)
    {
        qDebug("Lv2Plugin::get_custom_uri_string(%i)", uri_id);

        if (uri_id < custom_uri_ids.count())
            return custom_uri_ids.at(uri_id);
        else
            return nullptr;
    }

    bool is_ui_resizable()
    {
        if (ui.rdf_descriptor)
        {
            for (uint32_t i=0; i < ui.rdf_descriptor->FeatureCount; i++)
            {
                if (strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                    return false;
            }
            return true;
        }
        return false;
    }

    bool is_ui_bridgeable(uint32_t ui_id)
    {
        const LV2_RDF_UI* const rdf_ui = &rdf_descriptor->UIs[ui_id];

        for (uint32_t i=0; i < rdf_ui->FeatureCount; i++)
        {
            if (strcmp(rdf_ui->Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0 || strcmp(rdf_ui->Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
    }

    void reinit_external_ui()
    {
        qDebug("Lv2Plugin::reinit_external_ui()");

        ui.widget = nullptr;
        ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle && ui.widget)
        {
            update_ui_ports();
        }
        else
        {
            qDebug("Lv2Plugin::reinit_external_ui() - Failed to re-initiate external UI");

            ui.handle = nullptr;
            ui.widget = nullptr;
            callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
        }
    }

    void update_ui_ports()
    {
        qDebug("Lv2Plugin::update_ui_ports()");

        if (ui.handle && ui.descriptor && ui.descriptor->port_event)
        {
            float value;
            for (uint32_t i=0; i < param.count; i++)
            {
                value = get_parameter_value(i);
                ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
            }
        }
    }

#if 0
    // FIXME - resolve jack deactivate
    void lv2_remove_from_jack()
    {
        qDebug("Lv2Plugin::lv2_remove_from_jack() - start");

        for (uint32_t i=0; i < evin.count; i++)
        {
            if (evin.data[i].port)
                jack_port_unregister(jack_client, evin.data[i].port);
        }

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].port)
                jack_port_unregister(jack_client, evout.data[i].port);
        }

        qDebug("Lv2Plugin::lv2_remove_from_jack() - end");
    }
#endif

    bool init(const char* bundle, const char* URI)
    {
        const Lilv::Plugins Plugins = Lv2World.get_all_plugins();

        LILV_FOREACH(plugins, i, Plugins)
        {
            Lilv::Plugin Plugin = Lilv::Plugin(lilv_plugins_get(Plugins, i));

            if (strcmp(Plugin.get_uri().as_string(), URI) == 0)
            {
                rdf_descriptor = new LV2_RDF_Descriptor;

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

                // FIXME - get more values

                rdf_descriptor->URI        = strdup(URI);
                rdf_descriptor->Binary     = strdup(lilv_uri_to_path(Plugin.get_library_uri().as_string()));
                rdf_descriptor->Bundle     = strdup(lilv_uri_to_path(Plugin.get_bundle_uri().as_string()));

                if (Plugin.get_name())
                    rdf_descriptor->Name   = strdup(Plugin.get_name().as_string());
                else
                    rdf_descriptor->Name   = nullptr;

                if (Plugin.get_author_name())
                    rdf_descriptor->Author = strdup(Plugin.get_author_name().as_string());
                else
                    rdf_descriptor->Author = nullptr;

                rdf_descriptor->License    = nullptr;

                // --------------------------------------------------
                // Set Plugin UniqueID

                rdf_descriptor->UniqueID = 0;

                Lilv::Nodes replaces(Plugin.get_value(Lv2World.dct_replaces));

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
                                RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI;
                            if (supports.contains(Lv2World.time_position))
                                RDF_Port->Type |= LV2_PORT_SUPPORTS_TIME;
                        }

                        if (Port.is_a(Lv2World.port_event))
                        {
                            RDF_Port->Type |= LV2_PORT_EVENT;

                            if (Port.supports_event(Lv2World.midi_event))
                                RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI;
                            if (Port.supports_event(Lv2World.time_position))
                                RDF_Port->Type |= LV2_PORT_SUPPORTS_TIME;
                        }

                        if (Port.is_a(Lv2World.port_midi_ll))
                        {
                            RDF_Port->Type |= LV2_PORT_MIDI_LL;
                            RDF_Port->Type |= LV2_PORT_SUPPORTS_MIDI;
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
                            RDF_Port->Properties = LV2_PORT_HAS_STRICT_BOUNDS;
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

                rdf_descriptor->PresetCount = 0;                

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

                Lilv::Nodes extensions = Plugin.get_value(Lv2World.extension_data);

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

                        // TODO
                        RDF_UI->FeatureCount   = 0;
                        RDF_UI->ExtensionCount = 0;
                    }
                }
                else
                    rdf_descriptor->UIs = nullptr;

                break;
            }
        }

        if (rdf_descriptor)
        {
            if (lib_open(rdf_descriptor->Binary))
            {
                LV2_Descriptor_Function descfn = (LV2_Descriptor_Function)lib_symbol("lv2_descriptor");

                if (descfn)
                {
                    uint32_t i = 0;
                    while ((descriptor = descfn(i++)))
                    {
                        if (strcmp(descriptor->URI, URI) == 0)
                            break;
                    }

                    if (descriptor)
                    {
                        bool can_continue = true;

                        // Check supported ports
                        for (i=0; i < rdf_descriptor->PortCount; i++)
                        {
                            LV2_Property PortType = rdf_descriptor->Ports[i].Type;
                            if (bool(LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_CONTROL(PortType) || LV2_IS_PORT_ATOM_SEQUENCE(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType)) == false)
                            {
                                qCritical("Got unsupported port -> %i", PortType);
                                if (! LV2_IS_PORT_OPTIONAL(rdf_descriptor->Ports[i].Properties))
                                {
                                    set_last_error("Plugin requires a port that is not currently supported");
                                    can_continue = false;
                                    break;
                                }
                            }
                        }

                        // Check supported features
                        for (i=0; i < rdf_descriptor->FeatureCount; i++)
                        {
                            if (LV2_IS_FEATURE_REQUIRED(rdf_descriptor->Features[i].Type) && is_lv2_feature_supported(rdf_descriptor->Features[i].URI) == false)
                            {
                                QString msg = QString("Plugin requires a feature that is not supported:\n%1").arg(rdf_descriptor->Features[i].URI);
                                set_last_error(msg.toUtf8().constData());
                                can_continue = false;
                                break;
                            }
                        }

                        // Check extensions (...)
                        for (i=0; i < rdf_descriptor->ExtensionCount; i++)
                        {
                            if (strcmp(rdf_descriptor->Extensions[i], LV2_STATE__interface) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_STATE;
                            //else if (strcmp(rdf_descriptor->Extensions[i], LV2DYNPARAM_URI) == 0)
                            //    plugin->hints |= PLUGIN_HAS_EXTENSION_DYNPARAM;
                            else
                                qDebug("Plugin has non-supported extension: '%s'", rdf_descriptor->Extensions[i]);
                        }

                        if (can_continue)
                        {
                            // Initialize features
                            LV2_URI_Map_Feature* URI_Map_Feature = new LV2_URI_Map_Feature;
                            URI_Map_Feature->callback_data       = this;
                            URI_Map_Feature->uri_to_id           = carla_lv2_uri_to_id;

                            LV2_URID_Map* URID_Map_Feature       = new LV2_URID_Map;
                            URID_Map_Feature->handle             = this;
                            URID_Map_Feature->map                = carla_lv2_urid_map;

                            LV2_URID_Unmap* URID_Unmap_Feature   = new LV2_URID_Unmap;
                            URID_Unmap_Feature->handle           = this;
                            URID_Unmap_Feature->unmap            = carla_lv2_urid_unmap;

                            LV2_Event_Feature* Event_Feature     = new LV2_Event_Feature;
                            Event_Feature->callback_data         = this;
                            Event_Feature->lv2_event_ref         = nullptr;
                            Event_Feature->lv2_event_unref       = nullptr;

                            lv2_rtsafe_memory_pool_provider* RT_MemPool_Feature = new lv2_rtsafe_memory_pool_provider;
                            rtmempool_allocator_init(RT_MemPool_Feature);

                            features[lv2_feature_id_uri_map]          = new LV2_Feature;
                            features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
                            features[lv2_feature_id_uri_map]->data    = URI_Map_Feature;

                            features[lv2_feature_id_urid_map]         = new LV2_Feature;
                            features[lv2_feature_id_urid_map]->URI    = LV2_URID_MAP_URI;
                            features[lv2_feature_id_urid_map]->data   = URID_Map_Feature;

                            features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
                            features[lv2_feature_id_urid_unmap]->URI  = LV2_URID_UNMAP_URI;
                            features[lv2_feature_id_urid_unmap]->data = URID_Unmap_Feature;

                            features[lv2_feature_id_event]            = new LV2_Feature;
                            features[lv2_feature_id_event]->URI       = LV2_EVENT_URI;
                            features[lv2_feature_id_event]->data      = Event_Feature;

                            features[lv2_feature_id_rtmempool]        = new LV2_Feature;
                            features[lv2_feature_id_rtmempool]->URI   = LV2_RTSAFE_MEMORY_POOL_URI;
                            features[lv2_feature_id_rtmempool]->data  = RT_MemPool_Feature;

                            handle = descriptor->instantiate(descriptor, get_sample_rate(), rdf_descriptor->Bundle, features);

                            if (handle)
                            {
                                m_filename = strdup(bundle);
                                m_name = get_unique_name(rdf_descriptor->Name);

                                if (carla_jack_register_plugin(this, &jack_client))
                                {
                                    // ----------------- GUI Stuff -------------------------------------------------------

#if 1
                                    uint32_t UICount = rdf_descriptor->UICount;

                                    if (UICount > 0)
                                    {
                                        // Find more appropriate UI (Qt4 -> X11 -> Gtk2 -> External, use bridges whenever possible)
                                        int eQt4, eX11, eGtk2, iX11, iQt4, iExt, iFinal;
                                        eQt4 = eX11 = eGtk2 = iQt4 = iX11 = iExt = iFinal = -1;

                                        for (i=0; i < UICount; i++)
                                        {
                                            switch (rdf_descriptor->UIs[i].Type)
                                            {
                                            case LV2_UI_QT4:
                                                if (is_ui_bridgeable(i))
                                                    eQt4 = i;
                                                else
                                                    iQt4 = i;
                                                break;

                                            case LV2_UI_X11:
                                                if (is_ui_bridgeable(i))
                                                    eX11 = i;
                                                else
                                                    iX11 = i;
                                                break;

                                            case LV2_UI_GTK2:
                                                eGtk2 = i;
                                                break;

                                            case LV2_UI_EXTERNAL:
                                            case LV2_UI_OLD_EXTERNAL:
                                                iExt = i;
                                                break;

                                            default:
                                                break;
                                            }
                                        }

                                        if (eQt4 >= 0)
                                            iFinal = eQt4;
                                        else if (eX11 >= 0)
                                            iFinal = eX11;
                                        else if (eGtk2 >= 0)
                                            iFinal = eGtk2;
                                        else if (iQt4 >= 0)
                                            iFinal = iQt4;
                                        else if (iX11 >= 0)
                                            iFinal = iX11;
                                        else if (iExt >= 0)
                                            iFinal = iExt;

                                        bool is_bridged = (iFinal == eQt4 || iFinal == eX11 || iFinal == eGtk2);

                                        // Use proper UI now
                                        if (iFinal >= 0)
                                        {
                                            ui.rdf_descriptor = &rdf_descriptor->UIs[iFinal];

                                            // Check supported UI features
                                            can_continue = true;

                                            for (i=0; i < ui.rdf_descriptor->FeatureCount; i++)
                                            {
                                                if (LV2_IS_FEATURE_REQUIRED(ui.rdf_descriptor->Features[i].Type) && is_lv2_ui_feature_supported(ui.rdf_descriptor->Features[i].URI) == false)
                                                {
                                                    qCritical("Plugin UI requires a feature that is not supported:\n%s", ui.rdf_descriptor->Features[i].URI);
                                                    can_continue = false;
                                                    break;
                                                }
                                            }

                                            if (can_continue)
                                            {
                                                if (ui_lib_open(ui.rdf_descriptor->Binary))
                                                {
                                                    LV2UI_DescriptorFunction ui_descfn = (LV2UI_DescriptorFunction)ui_lib_symbol("lv2ui_descriptor");

                                                    if (ui_descfn)
                                                    {
                                                        i = 0;
                                                        while ((ui.descriptor = ui_descfn(i++)))
                                                        {
                                                            if (strcmp(ui.descriptor->URI, ui.rdf_descriptor->URI) == 0)
                                                                break;
                                                        }

                                                        if (ui.descriptor)
                                                        {
                                                            // UI Window Title
                                                            QString gui_title = QString("%1 (GUI)").arg(m_name);
                                                            LV2_Property UiType = ui.rdf_descriptor->Type;

                                                            if (is_bridged)
                                                            {
                                                                const char* osc_binary = lv2bridge2str(UiType);

                                                                if (osc_binary)
                                                                {
                                                                    gui.type = GUI_EXTERNAL_OSC;
                                                                    osc.thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
                                                                    osc.thread->setOscData(osc_binary, descriptor->URI, ui.descriptor->URI, ui.rdf_descriptor->Binary, ui.rdf_descriptor->Bundle);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                // Initialize UI features
                                                                LV2_Extension_Data_Feature* UI_Data_Feature = new LV2_Extension_Data_Feature;
                                                                UI_Data_Feature->data_access                = descriptor->extension_data;

                                                                lv2_external_ui_host* External_UI_Feature   = new lv2_external_ui_host;
                                                                External_UI_Feature->ui_closed              = carla_lv2_external_ui_closed;
                                                                External_UI_Feature->plugin_human_id        = strdup(gui_title.toUtf8().constData()); // NOTE

                                                                LV2UI_Resize* UI_Resize_Feature             = new LV2UI_Resize;
                                                                UI_Resize_Feature->handle                   = this;
                                                                UI_Resize_Feature->ui_resize                = carla_lv2_ui_resize;

                                                                features[lv2_feature_id_data_access]           = new LV2_Feature;
                                                                features[lv2_feature_id_data_access]->URI      = LV2_DATA_ACCESS_URI;
                                                                features[lv2_feature_id_data_access]->data     = UI_Data_Feature;

                                                                features[lv2_feature_id_instance_access]       = new LV2_Feature;
                                                                features[lv2_feature_id_instance_access]->URI  = LV2_INSTANCE_ACCESS_URI;
                                                                features[lv2_feature_id_instance_access]->data = handle;

                                                                features[lv2_feature_id_ui_parent]             = new LV2_Feature;
                                                                features[lv2_feature_id_ui_parent]->URI        = LV2_UI__parent;
                                                                features[lv2_feature_id_ui_parent]->data       = nullptr;

                                                                features[lv2_feature_id_ui_resize]             = new LV2_Feature;
                                                                features[lv2_feature_id_ui_resize]->URI        = LV2_UI__resize;
                                                                features[lv2_feature_id_ui_resize]->data       = UI_Resize_Feature;

                                                                features[lv2_feature_id_external_ui]           = new LV2_Feature;
                                                                features[lv2_feature_id_external_ui]->URI      = LV2_EXTERNAL_UI_URI;
                                                                features[lv2_feature_id_external_ui]->data     = External_UI_Feature;

                                                                features[lv2_feature_id_external_ui_old]       = new LV2_Feature;
                                                                features[lv2_feature_id_external_ui_old]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
                                                                features[lv2_feature_id_external_ui_old]->data = External_UI_Feature;

                                                                switch (UiType)
                                                                {
                                                                case LV2_UI_QT4:
                                                                    qDebug("Will use LV2 Qt4 UI");
                                                                    gui.type      = GUI_INTERNAL_QT4;
                                                                    gui.resizable = is_ui_resizable();

                                                                    ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);
                                                                    update_ui_ports();

                                                                    break;

                                                                case LV2_UI_X11:
                                                                    qDebug("Will use LV2 X11 UI");
                                                                    gui.type      = GUI_INTERNAL_X11;
                                                                    gui.resizable = is_ui_resizable();
                                                                    break;

                                                                case LV2_UI_GTK2:
                                                                    qDebug("Will use LV2 Gtk2 UI, NOT!");
                                                                    break;

                                                                case LV2_UI_EXTERNAL:
                                                                case LV2_UI_OLD_EXTERNAL:
                                                                    qDebug("Will use LV2 External UI");
                                                                    gui.type = GUI_EXTERNAL_LV2;
                                                                    break;

                                                                default:
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            qCritical("Could not find the requested GUI in the plugin UI library");
                                                            ui_lib_close();
                                                            ui.lib = nullptr;
                                                            ui.rdf_descriptor = nullptr;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        qCritical("Could not find the LV2UI Descriptor in the UI library");
                                                        ui_lib_close();
                                                        ui.lib = nullptr;
                                                        ui.rdf_descriptor = nullptr;
                                                    }
                                                }
                                                else
                                                    qCritical("Could not load UI library, error was:\n%s", ui_lib_error());
                                            }
                                            else
                                                // cannot continue, UI Feature not supported
                                                ui.rdf_descriptor = nullptr;
                                        }
                                        else
                                            qWarning("Failed to find an appropriate LV2 UI for this plugin");

                                    } // End of GUI Stuff

                                    if (gui.type != GUI_NONE)
                                        m_hints |= PLUGIN_HAS_GUI;
#endif

                                    return true;
                                }
                                else
                                    set_last_error("Failed to register plugin in JACK");
                            }
                            else
                                set_last_error("Plugin failed to initialize");
                        }
                        // error already set
                    }
                    else
                        set_last_error("Could not find the requested plugin URI in the plugin library");
                }
                else
                    set_last_error("Could not find the LV2 Descriptor in the plugin library");
            }
            else
                set_last_error(lib_error());
        }
        else
            set_last_error("Failed to find the requested plugin in the LV2 Bundle");

        return false;
    }

    bool ui_lib_open(const char* filename)
    {
#ifdef Q_OS_WIN
        ui.lib = LoadLibraryA(filename);
#else
        ui.lib = dlopen(filename, RTLD_LAZY);
#endif
        return bool(ui.lib);
    }

    bool ui_lib_close()
    {
        if (ui.lib)
#ifdef Q_OS_WIN
            return FreeLibrary((HMODULE)ui.lib) != 0;
#else
            return dlclose(ui.lib) != 0;
#endif
        else
            return false;
    }

    void* ui_lib_symbol(const char* symbol)
    {
        if (ui.lib)
#ifdef Q_OS_WIN
            return (void*)GetProcAddress((HMODULE)ui.lib, symbol);
#else
            return dlsym(ui.lib, symbol);
#endif
        else
            return nullptr;
    }

    const char* ui_lib_error()
    {
#ifdef Q_OS_WIN
        static char libError[2048];
        memset(libError, 0, sizeof(char)*2048);

        LPVOID winErrorString;
        DWORD  winErrorCode = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

        snprintf(libError, 2048, "%s: error code %i: %s", m_filename, winErrorCode, (const char*)winErrorString);
        LocalFree(winErrorString);

        return libError;
#else
        return dlerror();
#endif
    }

    // ----------------- URI-Map Feature -------------------------------------------------
    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

        if (map && strcmp(map, LV2_EVENT_URI) == 0)
        {
            // Event types
            if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
                return CARLA_URI_MAP_ID_MIDI_EVENT;
            else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
                return CARLA_URI_MAP_ID_TIME_POSITION;
        }
        else if (strcmp(uri, LV2_ATOM__Chunk) == 0)
        {
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        }
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
        {
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        }
        else if (strcmp(uri, LV2_ATOM__String) == 0)
        {
            return CARLA_URI_MAP_ID_ATOM_STRING;
        }

        // Custom types
        if (data)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)data;
            return plugin->get_custom_uri_id(uri);
        }

        return CARLA_URI_MAP_ID_NULL;
    }

    // ----------------- URID Feature ----------------------------------------------------
    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_urid_map(%p, %s)", handle, uri);

        if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;
        else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
            return CARLA_URI_MAP_ID_TIME_POSITION;
        else if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_id(uri);
        }

        return CARLA_URI_MAP_ID_NULL;
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_urid_unmap(%p, %i)", handle, urid);

        if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return "http://lv2plug.in/ns/ext/midi#MidiEvent";
        else if (urid == CARLA_URI_MAP_ID_TIME_POSITION)
            return "http://lv2plug.in/ns/ext/time#Position";
        else if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        else if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        else if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_string(urid);
        }

        return nullptr;
    }

    // ----------------- State Feature ---------------------------------------------------
    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            const char* uri_key = plugin->get_custom_uri_string(key);

            if (uri_key > 0 && (flags & LV2_STATE_IS_POD) > 0)
            {
                qDebug("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Got uri_key and flags", handle, key, value, size, type, flags);

                CustomDataType dtype;

                if (type == CARLA_URI_MAP_ID_ATOM_STRING)
                    dtype = CUSTOM_DATA_STRING;
                else if (type >= CARLA_URI_MAP_ID_COUNT)
                    dtype = CUSTOM_DATA_BINARY;
                else
                    dtype = CUSTOM_DATA_INVALID;

                if (value && dtype != CUSTOM_DATA_INVALID)
                {
                    // Check if we already have this key
                    for (int i=0; i < plugin->custom.count(); i++)
                    {
                        if (strcmp(plugin->custom[i].key, uri_key) == 0)
                        {
                            free((void*)plugin->custom[i].value);

                            if (dtype == CUSTOM_DATA_STRING)
                                plugin->custom[i].value = strdup((const char*)value);
                            else
                            {
                                QByteArray chunk((const char*)value, size);
                                plugin->custom[i].value = strdup(chunk.toBase64().constData());
                            }

                            return LV2_STATE_SUCCESS;
                        }
                    }

                    // Add a new one then
                    CustomData new_data;
                    new_data.type = dtype;
                    new_data.key  = strdup(uri_key);

                    if (dtype == CUSTOM_DATA_STRING)
                        new_data.value = strdup((const char*)value);
                    else
                    {
                        QByteArray chunk((const char*)value, size);
                        new_data.value = strdup(chunk.toBase64().constData());
                    }

                    plugin->custom.append(new_data);

                    return LV2_STATE_SUCCESS;
                }
                else
                {
                    qCritical("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid type", handle, key, value, size, type, flags);
                    return LV2_STATE_ERR_BAD_TYPE;
                }
            }
            else
            {
                qWarning("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid attributes", handle, key, value, size, type, flags);
                return LV2_STATE_ERR_BAD_FLAGS;
            }
        }
        else
        {
            qCritical("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid handle", handle, key, value, size, type, flags);
            return LV2_STATE_ERR_UNKNOWN;
        }
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            const char* uri_key = plugin->get_custom_uri_string(key);

            if (uri_key)
            {
                const char* string_data = nullptr;
                CustomDataType dtype = CUSTOM_DATA_INVALID;

                for (int i=0; i < plugin->custom.count(); i++)
                {
                    if (strcmp(plugin->custom[i].key, uri_key) == 0)
                    {
                        dtype = plugin->custom[i].type;
                        string_data = plugin->custom[i].value;
                        break;
                    }
                }

                if (string_data)
                {
                    *size  = 0;
                    *type  = 0;
                    *flags = 0;

                    if (dtype == CUSTOM_DATA_STRING)
                    {
                        *type = CARLA_URI_MAP_ID_ATOM_STRING;
                        return string_data;
                    }
                    else if (dtype == CUSTOM_DATA_BINARY)
                    {
                        static QByteArray chunk;
                        chunk = QByteArray::fromBase64(string_data);

                        *size = chunk.size();
                        *type = key;
                        return chunk.constData();
                    }
                    else
                        qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key type", handle, key, size, type, flags);
                }
                else
                    qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key", handle, key, size, type, flags);
            }
            else
                qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Failed to find key", handle, key, size, type, flags);
        }
        else
            qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid handle", handle, key, size, type, flags);

        return nullptr;
    }

    // ----------------- External UI Feature ---------------------------------------------
    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_external_ui_closed(%p)", controller);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;
            plugin->gui.visible = false;

            if (plugin->ui.descriptor->cleanup)
                plugin->ui.descriptor->cleanup(plugin->ui.handle);

            plugin->ui.handle = nullptr;
            callback_action(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
        }
    }

    // ----------------- UI Resize Feature -----------------------------------------------
    static int carla_lv2_ui_resize(LV2UI_Feature_Handle data, int width, int height)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_ui_resized(%p, %i, %i)", data, width, height);

        if (data)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)data;
            plugin->gui.width  = width;
            plugin->gui.height = height;
            callback_action(CALLBACK_RESIZE_GUI, plugin->id(), width, height, 0.0);
            return 0;
        }

        return 1;
    }

    // ----------------- UI Extension ----------------------------------------------------
    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;

            if (format == 0 && buffer_size == sizeof(float))
            {
                int32_t param_id = -1;
                float value = *(float*)buffer;

                for (uint32_t i=0; i < plugin->param.count; i++)
                {
                    if (plugin->param.data[i].rindex == (int32_t)port_index)
                    {
                        param_id = i; //plugin->param.data[i].index;
                        break;
                    }
                }

                if (param_id > -1) // && plugin->ctrl.data[param_id].type == PARAMETER_INPUT
                    plugin->set_parameter_value(param_id, value, false, true, true);
            }
        }
    }

private:
    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    LV2_RDF_Descriptor* rdf_descriptor;
    LV2_Feature* features[lv2_feature_count+1];

    struct {
        void* lib;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI* rdf_descriptor;
    } ui;

    struct {
        GuiType type;
        bool visible;
        bool resizable;
        int width;
        int height;
    } gui;

    uint32_t* ain_rindexes;
    uint32_t* aout_rindexes;
    Lv2ParameterData* lv2param;
    PluginEventData evin;
    PluginEventData evout;
    QList<const char*> custom_uri_ids;
};

short add_plugin_lv2(const char* filename, const char* label)
{
    qDebug("add_plugin_lv2(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        Lv2Plugin* plugin = new Lv2Plugin;

        if (plugin->init(filename, label))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
            plugin->osc_global_register_new();
#endif
        }
        else
        {
            delete plugin;
            id = -1;
        }
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

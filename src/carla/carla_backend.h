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

#ifndef CARLA_BACKEND_H
#define CARLA_BACKEND_H

#include "carla_includes.h"

#define STR_MAX 255

// static max values
#ifdef BUILD_BRIDGE
const unsigned short MAX_PLUGINS   = 1;
#else
const unsigned short MAX_PLUGINS   = 99;
#endif
const unsigned int MAX_PARAMETERS  = 200;
const unsigned int MAX_MIDI_EVENTS = 512;

// plugin hints
const unsigned int PLUGIN_HAS_GUI     = 0x01;
const unsigned int PLUGIN_IS_BRIDGE   = 0x02;
const unsigned int PLUGIN_IS_SYNTH    = 0x04;
const unsigned int PLUGIN_USES_CHUNKS = 0x08;
const unsigned int PLUGIN_CAN_DRYWET  = 0x10;
const unsigned int PLUGIN_CAN_VOLUME  = 0x20;
const unsigned int PLUGIN_CAN_BALANCE = 0x40;

// parameter hints
const unsigned int PARAMETER_IS_ENABLED        = 0x01;
const unsigned int PARAMETER_IS_AUTOMABLE      = 0x02;
const unsigned int PARAMETER_HAS_STRICT_BOUNDS = 0x04;
const unsigned int PARAMETER_USES_SCALEPOINTS  = 0x08;
const unsigned int PARAMETER_USES_SAMPLERATE   = 0x10;

enum BinaryType {
    BINARY_NONE   = 0,
    BINARY_UNIX32 = 1,
    BINARY_UNIX64 = 2,
    BINARY_WIN32  = 3,
    BINARY_WIN64  = 4
};

enum PluginType {
    PLUGIN_NONE   = 0,
    PLUGIN_LADSPA = 1,
    PLUGIN_DSSI   = 2,
    PLUGIN_LV2    = 3,
    PLUGIN_VST    = 4,
    PLUGIN_SF2    = 5
};

enum PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0,
    PLUGIN_CATEGORY_SYNTH     = 1,
    PLUGIN_CATEGORY_DELAY     = 2, // also Reverb
    PLUGIN_CATEGORY_EQ        = 3,
    PLUGIN_CATEGORY_FILTER    = 4,
    PLUGIN_CATEGORY_DYNAMICS  = 5, // Amplifier, Compressor, Gate
    PLUGIN_CATEGORY_MODULATOR = 6, // Chorus, Flanger, Phaser
    PLUGIN_CATEGORY_UTILITY   = 7, // Analyzer, Converter, Mixer
    PLUGIN_CATEGORY_OUTRO     = 8  // used to check if a plugin has a category
};

enum ParameterType {
    PARAMETER_UNKNOWN = 0,
    PARAMETER_INPUT   = 1,
    PARAMETER_OUTPUT  = 2
};

enum InternalParametersIndex {
    PARAMETER_ACTIVE = -1,
    PARAMETER_DRYWET = -2,
    PARAMETER_VOLUME = -3,
    PARAMETER_BALANCE_LEFT  = -4,
    PARAMETER_BALANCE_RIGHT = -5
};

enum CustomDataType {
    CUSTOM_DATA_INVALID = 0,
    CUSTOM_DATA_BOOL    = 1,
    CUSTOM_DATA_INT     = 2,
    CUSTOM_DATA_LONG    = 3,
    CUSTOM_DATA_FLOAT   = 4,
    CUSTOM_DATA_STRING  = 5,
    CUSTOM_DATA_BINARY  = 6
};

enum GuiType {
    GUI_NONE         = 0,
    GUI_INTERNAL_QT4 = 1,
    GUI_INTERNAL_X11 = 2,
    GUI_EXTERNAL_OSC = 3,
    GUI_EXTERNAL_LV2 = 4
};

enum OptionsType {
    OPTION_GLOBAL_JACK_CLIENT = 1,
    OPTION_USE_DSSI_CHUNKS    = 2,
    OPTION_PREFER_UI_BRIDGES  = 3,
    OPTION_PATH_BRIDGE_UNIX32 = 4,
    OPTION_PATH_BRIDGE_UNIX64 = 5,
    OPTION_PATH_BRIDGE_WIN32  = 6,
    OPTION_PATH_BRIDGE_WIN64  = 7
};

enum CallbackType {
    CALLBACK_DEBUG                = 0,
    CALLBACK_PARAMETER_CHANGED    = 1, // parameter_id, 0, value
    CALLBACK_PROGRAM_CHANGED      = 2, // program_id, 0, 0
    CALLBACK_MIDI_PROGRAM_CHANGED = 3, // bank_id, program_id, 0
    CALLBACK_NOTE_ON              = 4, // key, velocity, 0
    CALLBACK_NOTE_OFF             = 5, // key, 0, 0
    CALLBACK_SHOW_GUI             = 6, // show? (0|1, -1=quit), 0, 0
    CALLBACK_RESIZE_GUI           = 7, // width, height, 0
    CALLBACK_UPDATE               = 8,
    CALLBACK_RELOAD_INFO          = 9,
    CALLBACK_RELOAD_PARAMETERS    = 10,
    CALLBACK_RELOAD_PROGRAMS      = 11,
    CALLBACK_RELOAD_ALL           = 12,
    CALLBACK_QUIT                 = 13
};

struct ParameterData {
    ParameterType type;
    int32_t index;
    int32_t rindex;
    int32_t hints;
    uint8_t midi_channel;
    int16_t midi_cc;
};

struct ParameterRanges {
    double def;
    double min;
    double max;
    double step;
    double step_small;
    double step_large;
};

struct CustomData {
    CustomDataType type;
    const char* key;
    const char* value;
};

struct PluginInfo {
    bool valid;
    PluginType type;
    PluginCategory category;
    unsigned int hints;
    const char* binary;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;
    long unique_id;
};

struct PortCountInfo {
    bool valid;
    uint32_t ins;
    uint32_t outs;
    uint32_t total;
};

struct ParameterInfo {
    bool valid;
    const char* name;
    const char* symbol;
    const char* label;
    uint32_t scalepoint_count;
};

struct ScalePointInfo {
    bool valid;
    double value;
    const char* label;
};

struct MidiProgramInfo {
    bool valid;
    uint32_t bank;
    uint32_t program;
    const char* label;
};

struct GuiInfo {
    GuiType type;
};

struct PluginBridgeInfo {
    PluginCategory category;
    unsigned int hints;
    const char* name;
    const char* maker;
    long unique_id;
    uint32_t ains;
    uint32_t aouts;
    uint32_t mins;
    uint32_t mouts;
};

struct carla_options_t {
    bool initiated;
    bool global_jack_client;
    bool use_dssi_chunks;
    bool prefer_ui_bridges;
    const char* bridge_unix32;
    const char* bridge_unix64;
    const char* bridge_win32;
    const char* bridge_win64;
};

typedef void (*CallbackFunc)(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3);

// -----------------------------------------------------
// Exported symbols (API)

CARLA_EXPORT bool carla_init(const char* client_name);
CARLA_EXPORT bool carla_close();
CARLA_EXPORT bool carla_is_engine_running();

CARLA_EXPORT short add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff);
CARLA_EXPORT bool remove_plugin(unsigned short plugin_id);

CARLA_EXPORT PluginInfo* get_plugin_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_audio_port_count_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_midi_port_count_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_parameter_count_info(unsigned short plugin_id);
CARLA_EXPORT ParameterInfo* get_parameter_info(unsigned short plugin_id, uint32_t parameter_id);
CARLA_EXPORT ScalePointInfo* get_scalepoint_info(unsigned short plugin_id, uint32_t parameter_id, uint32_t scalepoint_id);
CARLA_EXPORT MidiProgramInfo* get_midi_program_info(unsigned short plugin_id, uint32_t midi_program_id);
CARLA_EXPORT GuiInfo* get_gui_info(unsigned short plugin_id);

CARLA_EXPORT ParameterData* get_parameter_data(unsigned short plugin_id, uint32_t parameter_id);
CARLA_EXPORT ParameterRanges* get_parameter_ranges(unsigned short plugin_id, uint32_t parameter_id);
CARLA_EXPORT CustomData* get_custom_data(unsigned short plugin_id, uint32_t custom_data_id);
CARLA_EXPORT const char* get_chunk_data(unsigned short plugin_id);

CARLA_EXPORT uint32_t get_parameter_count(unsigned short plugin_id);
CARLA_EXPORT uint32_t get_program_count(unsigned short plugin_id);
CARLA_EXPORT uint32_t get_midi_program_count(unsigned short plugin_id);
CARLA_EXPORT uint32_t get_custom_data_count(unsigned short plugin_id);

CARLA_EXPORT const char* get_program_name(unsigned short plugin_id, uint32_t program_id);
CARLA_EXPORT const char* get_midi_program_name(unsigned short plugin_id, uint32_t midi_program_id);
CARLA_EXPORT const char* get_real_plugin_name(unsigned short plugin_id);

CARLA_EXPORT int32_t get_current_program_index(unsigned short plugin_id);
CARLA_EXPORT int32_t get_current_midi_program_index(unsigned short plugin_id);

CARLA_EXPORT double get_default_parameter_value(unsigned short plugin_id, uint32_t parameter_id);
CARLA_EXPORT double get_current_parameter_value(unsigned short plugin_id, uint32_t parameter_id);

CARLA_EXPORT double get_input_peak_value(unsigned short plugin_id, unsigned short port_id);
CARLA_EXPORT double get_output_peak_value(unsigned short plugin_id, unsigned short port_id);

CARLA_EXPORT void set_active(unsigned short plugin_id, bool onoff);
CARLA_EXPORT void set_drywet(unsigned short plugin_id, double value);
CARLA_EXPORT void set_volume(unsigned short plugin_id, double value);
CARLA_EXPORT void set_balance_left(unsigned short plugin_id, double value);
CARLA_EXPORT void set_balance_right(unsigned short plugin_id, double value);

CARLA_EXPORT void set_parameter_value(unsigned short plugin_id, uint32_t parameter_id, double value);
CARLA_EXPORT void set_parameter_midi_channel(unsigned short plugin_id, uint32_t parameter_id, uint8_t channel);
CARLA_EXPORT void set_parameter_midi_cc(unsigned short plugin_id, uint32_t parameter_id, int16_t midi_cc);
CARLA_EXPORT void set_program(unsigned short plugin_id, uint32_t program_id);
CARLA_EXPORT void set_midi_program(unsigned short plugin_id, uint32_t midi_program_id);

CARLA_EXPORT void set_custom_data(unsigned short plugin_id, CustomDataType dtype, const char* key, const char* value);
CARLA_EXPORT void set_chunk_data(unsigned short plugin_id, const char* chunk_data);
CARLA_EXPORT void set_gui_data(unsigned short plugin_id, int data, intptr_t gui_addr);

CARLA_EXPORT void show_gui(unsigned short plugin_id, bool yesno);
CARLA_EXPORT void idle_gui(unsigned short plugin_id);

CARLA_EXPORT void send_midi_note(unsigned short plugin_id, bool onoff, uint8_t note, uint8_t velocity);
CARLA_EXPORT void prepare_for_save(unsigned short plugin_id);

CARLA_EXPORT void set_callback_function(CallbackFunc func);
CARLA_EXPORT void set_option(OptionsType option, int value, const char* value_str);

CARLA_EXPORT const char* get_last_error();
CARLA_EXPORT const char* get_host_client_name();
CARLA_EXPORT const char* get_host_osc_url();

CARLA_EXPORT uint32_t get_buffer_size();
CARLA_EXPORT double get_sample_rate();
CARLA_EXPORT double get_latency();

// End of exported symbols
// -----------------------------------------------------

#endif // CARLA_BACKEND_H

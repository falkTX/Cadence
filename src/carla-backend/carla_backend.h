/*
 * Carla Backend
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

#ifdef CARLA_BACKEND_NO_NAMESPACE
#define CARLA_BACKEND_START_NAMESPACE
#define CARLA_BACKEND_END_NAMESPACE
#else
#define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
#define CARLA_BACKEND_END_NAMESPACE }
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

#define STR_MAX 256

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API
 * @{
 */

#ifdef BUILD_BRIDGE
const unsigned short MAX_PLUGINS = 1;
#else
const unsigned short MAX_PLUGINS = 99; //!< Maximum number of plugins
#endif
const unsigned int MAX_PARAMETERS = 200; //!< Default value for maximum number of parameters the callback can handle.\see OPTION_MAX_PARAMETERS

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * \see CarlaPlugin::hints
 * @{
 */
const unsigned int PLUGIN_IS_BRIDGE   = 0x01; //!< Plugin is a bridge (ie, BridgePlugin). This hint is required because "bridge" itself is not a PluginType.
const unsigned int PLUGIN_IS_SYNTH    = 0x02; //!< Plugin is a synthesizer (produces sound).
const unsigned int PLUGIN_HAS_GUI     = 0x04; //!< Plugin has its own custom GUI.
const unsigned int PLUGIN_USES_CHUNKS = 0x08; //!< Plugin uses chunks to save internal data.\see CarlaPlugin::chunkData()
const unsigned int PLUGIN_CAN_DRYWET  = 0x10; //!< Plugin can make use of Dry/Wet controls.
const unsigned int PLUGIN_CAN_VOLUME  = 0x20; //!< Plugin can make use of Volume controls.
const unsigned int PLUGIN_CAN_BALANCE = 0x40; //!< Plugin can make use of Left & Right Balance controls.
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * Various parameter hints.
 * \see CarlaPlugin::paramData()
 * @{
 */
const unsigned int PARAMETER_IS_BOOLEAN       = 0x01; //!< Parameter value is of boolean type (always at minimum or maximum).
const unsigned int PARAMETER_IS_INTEGER       = 0x02; //!< Parameter values are always integer.
const unsigned int PARAMETER_IS_LOGARITHMIC   = 0x04; //!< Parameter is logarithmic (informative only, not really implemented).
const unsigned int PARAMETER_IS_ENABLED       = 0x08; //!< Parameter is enabled and will be shown in the host built-in editor.
const unsigned int PARAMETER_IS_AUTOMABLE     = 0x10; //!< Parameter is automable (realtime safe)
const unsigned int PARAMETER_USES_SAMPLERATE  = 0x20; //!< Parameter needs sample rate to work (value and ranges are multiplied by SR, and divided by SR on save).
const unsigned int PARAMETER_USES_SCALEPOINTS = 0x40; //!< Parameter uses scalepoints to define internal values in a meaninful way.
const unsigned int PARAMETER_USES_CUSTOM_TEXT = 0x80; //!< Parameter uses custom text for displaying its value.\see CarlaPlugin::getParameterText()
/**@}*/

/*!
 * The binary type of a plugin.\n
 * If a plugin binary type is loaded that doesn't match the system's native type, a bridge will be used (see BridgePlugin).
 */
enum BinaryType {
    BINARY_NONE   = 0, //!< Null binary type.
    BINARY_UNIX32 = 1, //!< Unix 32bit.
    BINARY_UNIX64 = 2, //!< Unix 64bit.
    BINARY_WIN32  = 3, //!< Windows 32bit.
    BINARY_WIN64  = 4  //!< Windows 64bit.
};

/*!
 * All the available plugin types, provided by subclasses of CarlaPlugin.\n
 * \note Some plugin classes might provide more than 1 plugin type.
 */
enum PluginType {
    PLUGIN_NONE   = 0, //!< Null plugin type.
    PLUGIN_LADSPA = 1, //!< LADSPA plugin.\see LadspaPlugin
    PLUGIN_DSSI   = 2, //!< DSSI plugin.\see DssiPlugin
    PLUGIN_LV2    = 3, //!< LV2 plugin.\see Lv2Plugin
    PLUGIN_VST    = 4, //!< VST plugin.\see VstPlugin
    PLUGIN_GIG    = 5, //!< GIG sound kit, provided by LinuxSampler.\see LinuxSamplerPlugin
    PLUGIN_SF2    = 6, //!< SF2 sound kit (aka SoundFont), provided by FluidSynth.\see FluidSynthPlugin
    PLUGIN_SFZ    = 7  //!< SFZ sound kit, provided by LinuxSampler.\see LinuxSamplerPlugin
};

enum PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0, //!< Unknown or undefined plugin category
    PLUGIN_CATEGORY_SYNTH     = 1, //!< A synthesizer or generator
    PLUGIN_CATEGORY_DELAY     = 2, //!< A delay or reverberator
    PLUGIN_CATEGORY_EQ        = 3, //!< An equalizer
    PLUGIN_CATEGORY_FILTER    = 4, //!< A filter
    PLUGIN_CATEGORY_DYNAMICS  = 5, //!< A "dynamic" plugin (amplifier, compressor, gate, etc)
    PLUGIN_CATEGORY_MODULATOR = 6, //!< A "modulator" plugin (chorus, flanger, phaser, etc)
    PLUGIN_CATEGORY_UTILITY   = 7, //!< An utility plugin (analyzer, converter, mixer, etc)
    PLUGIN_CATEGORY_OTHER     = 8  //!< Misc plugin (used to check if plugin has a category)
};

enum ParameterType {
    PARAMETER_UNKNOWN = 0,
    PARAMETER_INPUT   = 1,
    PARAMETER_OUTPUT  = 2,
    PARAMETER_LATENCY = 3
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
    CUSTOM_DATA_STRING  = 1,
    CUSTOM_DATA_PATH    = 2,
    CUSTOM_DATA_CHUNK   = 3,
    CUSTOM_DATA_BINARY  = 4
};

enum GuiType {
    GUI_NONE = 0,
    GUI_INTERNAL_QT4  = 1,
    GUI_INTERNAL_HWND = 2,
    GUI_INTERNAL_X11  = 3,
    GUI_EXTERNAL_LV2  = 4,
    GUI_EXTERNAL_OSC  = 5
};

// TODO - fill desc
enum OptionsType {
    OPTION_PROCESS_MODE         = 1, //!< ...\see ProcessModeType
    OPTION_MAX_PARAMETERS       = 2, //!< Maximum number of parameters the callback can handle, defaults to MAX_PARAMETERS
    OPTION_PREFER_UI_BRIDGES    = 3,
    OPTION_PROCESS_HQ           = 4,
    OPTION_OSC_GUI_TIMEOUT      = 5,
    OPTION_USE_DSSI_CHUNKS      = 6,
    OPTION_PATH_LADSPA          = 7,
    OPTION_PATH_DSSI            = 8,
    OPTION_PATH_LV2             = 9,
    OPTION_PATH_VST             = 10,
    OPTION_PATH_GIG             = 11,
    OPTION_PATH_SF2             = 12,
    OPTION_PATH_SFZ             = 13,
    OPTION_PATH_BRIDGE_UNIX32   = 14,
    OPTION_PATH_BRIDGE_UNIX64   = 15,
    OPTION_PATH_BRIDGE_WIN32    = 16,
    OPTION_PATH_BRIDGE_WIN64    = 17,
    OPTION_PATH_BRIDGE_LV2_GTK2 = 18,
    OPTION_PATH_BRIDGE_LV2_QT4  = 19,
    OPTION_PATH_BRIDGE_LV2_X11  = 20,
    OPTION_PATH_BRIDGE_VST_X11  = 21
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

enum ProcessModeType {
    PROCESS_MODE_SINGLE_CLIENT    = 0, //!< Single client mode (dynamic input/outputs as needed by plugins)
    PROCESS_MODE_MULTIPLE_CLIENTS = 1, //!< Multiple client mode
    PROCESS_MODE_CONTINUOUS_RACK  = 2  //!< Single client "rack" mode. Processes plugins in order of it's id, with forced stereo input/output.
};

struct ParameterData {
    ParameterType type;
    qint32 index;
    qint32 rindex;
    qint32 hints;
    quint8 midiChannel;
    qint16 midiCC;
};

struct ParameterRanges {
    double def;
    double min;
    double max;
    double step;
    double stepSmall;
    double stepLarge;
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
    long uniqueId;
};

struct PortCountInfo {
    bool valid;
    quint32 ins;
    quint32 outs;
    quint32 total;
};

struct ParameterInfo {
    bool valid;
    const char* name;
    const char* symbol;
    const char* unit;
    quint32 scalePointCount;
};

struct ScalePointInfo {
    bool valid;
    double value;
    const char* label;
};

struct MidiProgramInfo {
    bool valid;
    quint32 bank;
    quint32 program;
    const char* label;
};

struct GuiInfo {
    GuiType type;
    bool resizable;
};

class CarlaPlugin;

typedef void (*CallbackFunc)(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3);

// -----------------------------------------------------
// Exported symbols (API)

#ifndef CARLA_BACKEND_NO_EXPORTS
CARLA_EXPORT bool engine_init(const char* client_name);
CARLA_EXPORT bool engine_close();
CARLA_EXPORT bool is_engine_running();

CARLA_EXPORT short add_plugin(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff);
CARLA_EXPORT bool remove_plugin(unsigned short plugin_id);

CARLA_EXPORT PluginInfo* get_plugin_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_audio_port_count_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_midi_port_count_info(unsigned short plugin_id);
CARLA_EXPORT PortCountInfo* get_parameter_count_info(unsigned short plugin_id);
CARLA_EXPORT ParameterInfo* get_parameter_info(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT ScalePointInfo* get_scalepoint_info(unsigned short plugin_id, quint32 parameter_id, quint32 scalepoint_id);
CARLA_EXPORT MidiProgramInfo* get_midi_program_info(unsigned short plugin_id, quint32 midi_program_id);
CARLA_EXPORT GuiInfo* get_gui_info(unsigned short plugin_id);

CARLA_EXPORT const ParameterData* get_parameter_data(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const ParameterRanges* get_parameter_ranges(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const CustomData* get_custom_data(unsigned short plugin_id, quint32 custom_data_id);
CARLA_EXPORT const char* get_chunk_data(unsigned short plugin_id);

CARLA_EXPORT quint32 get_parameter_count(unsigned short plugin_id);
CARLA_EXPORT quint32 get_program_count(unsigned short plugin_id);
CARLA_EXPORT quint32 get_midi_program_count(unsigned short plugin_id);
CARLA_EXPORT quint32 get_custom_data_count(unsigned short plugin_id);

CARLA_EXPORT const char* get_parameter_text(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const char* get_program_name(unsigned short plugin_id, quint32 program_id);
CARLA_EXPORT const char* get_midi_program_name(unsigned short plugin_id, quint32 midi_program_id);
CARLA_EXPORT const char* get_real_plugin_name(unsigned short plugin_id);

CARLA_EXPORT qint32 get_current_program_index(unsigned short plugin_id);
CARLA_EXPORT qint32 get_current_midi_program_index(unsigned short plugin_id);

CARLA_EXPORT double get_default_parameter_value(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT double get_current_parameter_value(unsigned short plugin_id, quint32 parameter_id);

CARLA_EXPORT double get_input_peak_value(unsigned short plugin_id, unsigned short port_id);
CARLA_EXPORT double get_output_peak_value(unsigned short plugin_id, unsigned short port_id);

CARLA_EXPORT void set_active(unsigned short plugin_id, bool onoff);
CARLA_EXPORT void set_drywet(unsigned short plugin_id, double value);
CARLA_EXPORT void set_volume(unsigned short plugin_id, double value);
CARLA_EXPORT void set_balance_left(unsigned short plugin_id, double value);
CARLA_EXPORT void set_balance_right(unsigned short plugin_id, double value);

CARLA_EXPORT void set_parameter_value(unsigned short plugin_id, quint32 parameter_id, double value);
CARLA_EXPORT void set_parameter_midi_channel(unsigned short plugin_id, quint32 parameter_id, quint8 channel);
CARLA_EXPORT void set_parameter_midi_cc(unsigned short plugin_id, quint32 parameter_id, qint16 midi_cc);
CARLA_EXPORT void set_program(unsigned short plugin_id, quint32 program_id);
CARLA_EXPORT void set_midi_program(unsigned short plugin_id, quint32 midi_program_id);

CARLA_EXPORT void set_custom_data(unsigned short plugin_id, CustomDataType dtype, const char* key, const char* value);
CARLA_EXPORT void set_chunk_data(unsigned short plugin_id, const char* chunk_data);
CARLA_EXPORT void set_gui_data(unsigned short plugin_id, int data, quintptr gui_addr);

CARLA_EXPORT void show_gui(unsigned short plugin_id, bool yesno);
CARLA_EXPORT void idle_guis();

CARLA_EXPORT void send_midi_note(unsigned short plugin_id, quint8 note, quint8 velocity);
CARLA_EXPORT void prepare_for_save(unsigned short plugin_id);

CARLA_EXPORT void set_option(OptionsType option, int value, const char* value_str);
#endif

CARLA_EXPORT void set_callback_function(CallbackFunc func);

CARLA_EXPORT const char* get_last_error();
CARLA_EXPORT const char* get_host_client_name();
CARLA_EXPORT const char* get_host_osc_url();

CARLA_EXPORT quint32 get_buffer_size();
CARLA_EXPORT double get_sample_rate();
CARLA_EXPORT double get_latency();

// End of exported symbols
// -----------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#ifndef CARLA_BACKEND_NO_NAMESPACE
typedef CarlaBackend::CarlaPlugin CarlaPlugin;
#endif

/**@}*/

#endif // CARLA_BACKEND_H

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

#define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
#define CARLA_BACKEND_END_NAMESPACE }

CARLA_BACKEND_START_NAMESPACE

#define STR_MAX 0xff

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API
 * @{
 */

#ifdef BUILD_BRIDGE
const unsigned short MAX_PLUGINS  = 1;
#else
const unsigned short MAX_PLUGINS  = 99;  //!< Maximum number of loadable plugins
#endif
const unsigned int MAX_PARAMETERS = 200; //!< Default value for maximum number of parameters the callback can handle.\see OPTION_MAX_PARAMETERS

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * \see CarlaPlugin::hints
 * @{
 */
const unsigned int PLUGIN_IS_BRIDGE   = 0x01; //!< Plugin is a bridge (ie, BridgePlugin). This hint is required because "bridge" itself is not a plugin type.
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
 * The binary type of a plugin.
 * \see BridgePlugin
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
    PLUGIN_CATEGORY_DYNAMICS  = 5, //!< A 'dynamic' plugin (amplifier, compressor, gate, etc)
    PLUGIN_CATEGORY_MODULATOR = 6, //!< A 'modulator' plugin (chorus, flanger, phaser, etc)
    PLUGIN_CATEGORY_UTILITY   = 7, //!< An 'utility' plugin (analyzer, converter, mixer, etc)
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
    GUI_INTERNAL_QT4   = 1,
    GUI_INTERNAL_COCOA = 2,
    GUI_INTERNAL_HWND  = 3,
    GUI_INTERNAL_X11   = 4,
    GUI_EXTERNAL_LV2   = 5,
    GUI_EXTERNAL_SUIL  = 6,
    GUI_EXTERNAL_OSC   = 7
};

/*!
 * Options used in set_option().\n
 * These options must be set before calling CarlaEngine::init() or after CarlaEngine::close().
 *
 * \see set_option()
 */
enum OptionsType {
    /*!
     * Set the engine processing mode.\n
     * Default is PROCESS_MODE_MULTIPLE_CLIENTS.
     *
     * \param value A value from ProcessModeType
     * \param valueStr Unused
     * \see ProcessModeType
     */
    OPTION_PROCESS_MODE = 1,

    /*!
     * Maximum number of parameters the callback can handle.\n
     * Default is MAX_PARAMETERS.
     *
     * \param value The new value
     * \param valueStr Unused
     */
    OPTION_MAX_PARAMETERS = 2,

    /*!
     * Wherever to use OSC-UI bridges when possible, otherwise UIs will be handled in the main thread.\n
     * Default is yes.
     *
     * \param value Boolean for yes/no
     * \param valueStr Unused
     */
    OPTION_PREFER_UI_BRIDGES = 3,

    /*!
     * Force mono plugins as stereo, by running instances at the same time.\n
     * Supported in LADSPA, DSSI and LV2 plugin types.
     *
     * \param value Boolean for yes/no
     * \param valueStr Unused
     */
    OPTION_FORCE_STEREO = 4,

    /*!
     * High-Precision processing mode.\n
     * When enabled, audio will be processed by blocks 8 samples at a time, no matter what the real buffer size is.\n
     * Default is no (EXPERIMENTAL!).
     *
     * \param value Boolean for yes/no
     * \param valueStr Unused
     */
    OPTION_PROCESS_HIGH_PRECISION = 5,

    /*!
     * Timeout value for how many miliseconds to wait for OSC-GUIs to respond.\n
     * Default is 4000 ms (4 secs).
     *
     * \param value The new value
     * \param valueStr Unused
     */
    OPTION_OSC_GUI_TIMEOUT = 6,

    /*!
     * Wherever to use unofficial dssi-vst chunks feature.\n
     * Default is no. (EXPERIMENTAL!).
     *
     * \param value Boolean for yes/no
     * \param valueStr Unused
     */
    OPTION_USE_DSSI_CHUNKS = 7,

    /*!
     * Set LADSPA_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_LADSPA = 8,

    /*!
     * Set DSSI_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_DSSI = 9,

    /*!
     * Set LV2_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_LV2 = 10,

    /*!
     * Set VST_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_VST = 11,

    /*!
     * Set GIG_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_GIG = 12,

    /*!
     * Set SF2_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_SF2 = 13,

    /*!
     * Set SFZ_PATH environment variable.\n
     * Default undefined.
     *
     * \param value Unused
     * \param valueStr The new path, separated by ":" on Unix or ";" on Windows
     */
    OPTION_PATH_SFZ = 14,

    /*!
     * Set path to the Unix 32bit plugin bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_UNIX32 = 15,

    /*!
     * Set path to the Unix 64bit plugin bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_UNIX64 = 16,

    /*!
     * Set path to the Windows 32bit plugin bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_WIN32 = 17,

    /*!
     * Set path to the Windows 64bit plugin bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_WIN64 = 18,

    /*!
     * Set path to the LV2 Gtk2 UI bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_LV2_GTK2 = 19,

    /*!
     * Set path to the LV2 Qt4 UI bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_LV2_QT4 = 20,

    /*!
     * Set path to the LV2 X11 UI bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_LV2_X11 = 21,

    /*!
     * Set path to the VST X11 UI bridge.\n
     * Default unset.
     *
     * \param value Unused
     * \param valueStr The new path
     */
    OPTION_PATH_BRIDGE_VST_X11 = 22
};

/*!
 * Opcodes sent to the callback, defined by CallbackFunc.
 *
 * \see CarlaEngine::setCallback()
 */
enum CallbackType {
    /*!
     * Debug.\n
     * This opcode is undefined and used only for testing purposes.
     *
     * \param value1 Undefined
     * \param value2 Undefined
     * \param value3 Undefined
     */
    CALLBACK_DEBUG = 0,

    /*!
     * A parameter has been changed.
     *
     * \param value1 The parameter Id
     * \param value3 The new value
     */
    CALLBACK_PARAMETER_CHANGED = 1,

    /*!
     * The current program has has been changed.
     *
     * \param value1 The new program index
     */
    CALLBACK_PROGRAM_CHANGED = 2,

    /*!
     * The current MIDI program has been changed.
     *
     * \param value1 The new MIDI program's bank
     * \param value2 The new MIDI program's program
     */
    CALLBACK_MIDI_PROGRAM_CHANGED = 3,

    /*!
     * A note has been pressed.
     *
     * \param value1 The note
     * \param value2 Velocity of the note
     */
    CALLBACK_NOTE_ON = 4,

    /*!
     * A note has been released.
     *
     * \param value1 The note
     */
    CALLBACK_NOTE_OFF = 5,

    /*!
     * The plugin's custom GUI state has changed.
     *
     * \param value1 The new state is as follows:.\n
     *                0: GUI has been closed or hidden\n
     *                1: GUI has been shown\n
     *               -1: GUI has crashed and should not be shown again\n
     */
    CALLBACK_SHOW_GUI = 6,

    /*!
     * The plugin's custom GUI has been resized.
     *
     * \param value1 The new width
     * \param value2 The new height
     */
    CALLBACK_RESIZE_GUI = 7,

    /*!
     * The plugin needs repaint and/or update.
     */
    CALLBACK_UPDATE = 8,

    /*!
     * The plugin's data/information has been changed.
     */
    CALLBACK_RELOAD_INFO = 9,

    /*!
     * The plugin's parameters have been changed.
     */
    CALLBACK_RELOAD_PARAMETERS = 10,

    /*!
     * The plugin's programs have been changed.
     */
    CALLBACK_RELOAD_PROGRAMS = 11,

    /*!
     * The plugin's state have been changed.
     */
    CALLBACK_RELOAD_ALL = 12,

    /*!
     * The engine has crashed or malfunctioned and will no longer work.
     */
    CALLBACK_QUIT = 13
};

/*!
 * Engine processing mode, changed using set_option().
 *
 * \see ProcessModeType
 */
enum ProcessModeType {
    PROCESS_MODE_SINGLE_CLIENT    = 0, //!< Single client mode (dynamic input/outputs as needed by plugins)
    PROCESS_MODE_MULTIPLE_CLIENTS = 1, //!< Multiple client mode
    PROCESS_MODE_CONTINUOUS_RACK  = 2  //!< Single client "rack" mode. Processes plugins in order of id, with forced stereo input/output.
};

/*!
 * Callback function the backend will call when something interesting happens.
 *
 * \see CallbackType
 * \see CarlaEngine::setCallback()
 */
typedef void (*CallbackFunc)(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3);

struct midi_program_t {
    uint32_t bank;
    uint32_t program;
    const char* name;

    midi_program_t()
        : bank(0),
          program(0),
          name(nullptr) {}
};

struct ParameterData {
    ParameterType type;
    qint32 index;
    qint32 rindex;
    qint32 hints;
    quint8 midiChannel;
    qint16 midiCC;

    ParameterData()
        : type(PARAMETER_UNKNOWN),
          index(-1),
          rindex(-1),
          hints(0),
          midiChannel(0),
          midiCC(-1) {}
};

struct ParameterRanges {
    double def;
    double min;
    double max;
    double step;
    double stepSmall;
    double stepLarge;

    ParameterRanges()
        : def(0.0),
          min(0.0),
          max(1.0),
          step(0.01),
          stepSmall(0.0001),
          stepLarge(0.1) {}
};

struct CustomData {
    CustomDataType type;
    const char* key;
    const char* value;

    CustomData()
        : type(CUSTOM_DATA_INVALID),
          key(nullptr),
          value(nullptr) {}
};

/**@}*/

class CarlaEngine;
class CarlaPlugin;

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_BACKEND_H

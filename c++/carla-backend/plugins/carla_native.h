/*
 * Carla Native Plugin API
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_NATIVE_H
#define CARLA_NATIVE_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

typedef void* PluginHandle;

const uint32_t PLUGIN_IS_SYNTH            = 1 << 0;
const uint32_t PLUGIN_HAS_GUI             = 1 << 1;
const uint32_t PLUGIN_USES_SINGLE_THREAD  = 1 << 2;

const uint32_t PORT_HINT_IS_OUTPUT        = 1 << 0;
const uint32_t PORT_HINT_IS_ENABLED       = 1 << 1;
const uint32_t PORT_HINT_IS_AUTOMABLE     = 1 << 2;
const uint32_t PORT_HINT_IS_BOOLEAN       = 1 << 3;
const uint32_t PORT_HINT_IS_INTEGER       = 1 << 4;
const uint32_t PORT_HINT_IS_LOGARITHMIC   = 1 << 5;
const uint32_t PORT_HINT_USES_SAMPLE_RATE = 1 << 6;
const uint32_t PORT_HINT_USES_SCALEPOINTS = 1 << 7;
const uint32_t PORT_HINT_USES_CUSTOM_TEXT = 1 << 8;

typedef enum _PluginCategory {
    PLUGIN_CATEGORY_NONE      = 0, //!< Null plugin category.
    PLUGIN_CATEGORY_SYNTH     = 1, //!< A synthesizer or generator.
    PLUGIN_CATEGORY_DELAY     = 2, //!< A delay or reverberator.
    PLUGIN_CATEGORY_EQ        = 3, //!< An equalizer.
    PLUGIN_CATEGORY_FILTER    = 4, //!< A filter.
    PLUGIN_CATEGORY_DYNAMICS  = 5, //!< A 'dynamic' plugin (amplifier, compressor, gate, etc).
    PLUGIN_CATEGORY_MODULATOR = 6, //!< A 'modulator' plugin (chorus, flanger, phaser, etc).
    PLUGIN_CATEGORY_UTILITY   = 7, //!< An 'utility' plugin (analyzer, converter, mixer, etc).
    PLUGIN_CATEGORY_OTHER     = 8  //!< Misc plugin (used to check if the plugin has a category).
} PluginCategory;

typedef enum _PortType {
    PORT_TYPE_NULL      = 0,
    PORT_TYPE_AUDIO     = 1,
    PORT_TYPE_MIDI      = 2,
    PORT_TYPE_PARAMETER = 3
} PortType;

typedef struct _ParameterRanges {
    double def;
    double min;
    double max;
    double step;
    double stepSmall;
    double stepLarge;
} ParameterRanges;

typedef struct _MidiEvent {
    uint32_t time;
    uint8_t  size;
    uint8_t  data[4];
} MidiEvent;

typedef struct _MidiProgram {
    uint32_t bank;
    uint32_t program;
    const char* name;
} MidiProgram;

typedef struct _TimeInfoBBT {
    int32_t bar;
    int32_t beat;
    int32_t tick;
    double bar_start_tick;
    float  beats_per_bar;
    float  beat_type;
    double ticks_per_beat;
    double beats_per_minute;
} TimeInfoBBT;

typedef struct _TimeInfo {
    bool playing;
    uint32_t frame;
    uint32_t time;
    uint32_t valid;
    TimeInfoBBT bbt;
} TimeInfo;

typedef struct _PluginPortScalePoint {
    const char* label;
    double value;
} PluginPortScalePoint;

typedef struct _PluginPort {
    PortType type;
    uint32_t hints;
    const char* name;

    uint32_t scalePointCount;
    PluginPortScalePoint* scalePoints;
} PluginPort;

typedef struct _PluginDescriptor {
    PluginCategory category;
    uint32_t    hints;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;

    uint32_t    portCount;
    PluginPort* ports;

    uint32_t     midiProgramCount;
    MidiProgram* midiPrograms;

    PluginHandle (*instantiate)(struct _PluginDescriptor* _this_);
    void         (*activate)(PluginHandle handle);
    void         (*deactivate)(PluginHandle handle);
    void         (*cleanup)(PluginHandle handle);

    void        (*get_parameter_ranges)(PluginHandle handle, uint32_t index, ParameterRanges* ranges);
    double      (*get_parameter_value)(PluginHandle handle, uint32_t index);
    const char* (*get_parameter_text)(PluginHandle handle, uint32_t index);
    const char* (*get_parameter_unit)(PluginHandle handle, uint32_t index);

    void (*set_parameter_value)(PluginHandle handle, uint32_t index, double value);
    void (*set_midi_program)(PluginHandle handle, uint32_t bank, uint32_t program);
    void (*set_custom_data)(PluginHandle handle, const char* key, const char* value);

    void (*show_gui)(PluginHandle handle, bool show);
    void (*idle_gui)(PluginHandle handle);

    // TODO - ui_set_*

    void (*process)(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents);
    // TODO - midi stuff^

    PluginHandle _handle;
    void (*_init)(struct _PluginDescriptor* _this_);
    void (*_fini)(struct _PluginDescriptor* _this_);
} PluginDescriptor;

// -----------------------------------------------------------------------

void carla_register_native_plugin(const PluginDescriptor* desc);

#define CARLA_NATIVE_PARAMETER_RANGES_INIT { 0.0, 0.0, 1.0, 0.01, 0.0001, 0.1 }

#define CARLA_NATIVE_PLUGIN_INIT           {         \
    PLUGIN_CATEGORY_NONE, 0, NULL, NULL, NULL, NULL, \
    0, NULL, 0, NULL,                                \
    NULL, NULL, NULL, NULL,                          \
    NULL, NULL, NULL, NULL,                          \
    NULL, NULL, NULL,                                \
    NULL, NULL,                                      \
    NULL,                                            \
    NULL, NULL, NULL                                 \
    }

#define CARLA_REGISTER_NATIVE_PLUGIN(label, desc)                              \
    void carla_register_native_plugin_##label () __attribute__((constructor)); \
    void carla_register_native_plugin_##label () { carla_register_native_plugin(&desc); }

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CARLA_NATIVE_H

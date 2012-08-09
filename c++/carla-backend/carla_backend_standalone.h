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

#ifndef CARLA_BACKEND_STANDALONE_H
#define CARLA_BACKEND_STANDALONE_H

#include "carla_backend.h"

/*!
 * @defgroup CarlaBackendStandalone Carla Backend Standalone
 *
 * The Carla Backend Standalone
 *
 * @{
 */

struct PluginInfo {
    CarlaBackend::PluginType type;
    CarlaBackend::PluginCategory category;
    unsigned int hints;
    const char* binary;
    const char* name;
    const char* label;
    const char* maker;
    const char* copyright;
    long uniqueId;

    PluginInfo()
        : type(CarlaBackend::PLUGIN_NONE),
          category(CarlaBackend::PLUGIN_CATEGORY_NONE),
          hints(0),
          binary(nullptr),
          name(nullptr),
          label(nullptr),
          maker(nullptr),
          copyright(nullptr),
          uniqueId(0) {}
};

struct PortCountInfo {
    quint32 ins;
    quint32 outs;
    quint32 total;

    PortCountInfo()
        : ins(0),
          outs(0),
          total(0) {}
};

struct ParameterInfo {
    const char* name;
    const char* symbol;
    const char* unit;
    quint32 scalePointCount;

    ParameterInfo()
        : name(nullptr),
          symbol(nullptr),
          unit(nullptr),
          scalePointCount(0) {}
};

struct ScalePointInfo {
    double value;
    const char* label;

    ScalePointInfo()
        : value(0.0),
          label(nullptr) {}
};

struct GuiInfo {
    CarlaBackend::GuiType type;
    bool resizable;

    GuiInfo()
        : type(CarlaBackend::GUI_NONE),
          resizable(false) {}
};

CARLA_EXPORT unsigned int get_engine_driver_count();
CARLA_EXPORT const char* get_engine_driver_name(unsigned int index);

CARLA_EXPORT bool engine_init(const char* driver_name, const char* client_name);
CARLA_EXPORT bool engine_close();
CARLA_EXPORT bool is_engine_running();

CARLA_EXPORT short add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* name, const char* label, void* extra_stuff);
CARLA_EXPORT bool remove_plugin(unsigned short plugin_id);

CARLA_EXPORT const PluginInfo* get_plugin_info(unsigned short plugin_id);
CARLA_EXPORT const PortCountInfo* get_audio_port_count_info(unsigned short plugin_id);
CARLA_EXPORT const PortCountInfo* get_midi_port_count_info(unsigned short plugin_id);
CARLA_EXPORT const PortCountInfo* get_parameter_count_info(unsigned short plugin_id);
CARLA_EXPORT const ParameterInfo* get_parameter_info(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const ScalePointInfo* get_parameter_scalepoint_info(unsigned short plugin_id, quint32 parameter_id, quint32 scalepoint_id);
CARLA_EXPORT const GuiInfo* get_gui_info(unsigned short plugin_id);

CARLA_EXPORT const CarlaBackend::ParameterData* get_parameter_data(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const CarlaBackend::ParameterRanges* get_parameter_ranges(unsigned short plugin_id, quint32 parameter_id);
CARLA_EXPORT const CarlaBackend::midi_program_t* get_midi_program_data(unsigned short plugin_id, quint32 midi_program_id);
CARLA_EXPORT const CarlaBackend::CustomData* get_custom_data(unsigned short plugin_id, quint32 custom_data_id);
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
CARLA_EXPORT void set_parameter_midi_cc(unsigned short plugin_id, quint32 parameter_id, qint16 cc);
CARLA_EXPORT void set_program(unsigned short plugin_id, quint32 program_id);
CARLA_EXPORT void set_midi_program(unsigned short plugin_id, quint32 midi_program_id);

CARLA_EXPORT void set_custom_data(unsigned short plugin_id, CarlaBackend::CustomDataType dtype, const char* key, const char* value);
CARLA_EXPORT void set_chunk_data(unsigned short plugin_id, const char* chunk_data);
CARLA_EXPORT void set_gui_data(unsigned short plugin_id, int data, quintptr gui_addr);

CARLA_EXPORT void show_gui(unsigned short plugin_id, bool yesno);
CARLA_EXPORT void idle_guis();

CARLA_EXPORT void send_midi_note(unsigned short plugin_id, quint8 channel, quint8 note, quint8 velocity);
CARLA_EXPORT void prepare_for_save(unsigned short plugin_id);

CARLA_EXPORT quint32 get_buffer_size();
CARLA_EXPORT double get_sample_rate();

CARLA_EXPORT const char* get_last_error();
CARLA_EXPORT const char* get_host_osc_url();

CARLA_EXPORT void set_callback_function(CarlaBackend::CallbackFunc func);
CARLA_EXPORT void set_option(CarlaBackend::OptionsType option, int value, const char* valueStr);

/**@}*/

#endif // CARLA_BACKEND_STANDALONE_H

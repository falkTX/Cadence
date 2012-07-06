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

#ifndef CARLA_SHARED_H
#define CARLA_SHARED_H

#include "carla_backend.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

const char* binarytype2str(BinaryType type);
const char* plugintype2str(PluginType type);
const char* optionstype2str(OptionsType type);
const char* customdatatype2str(CustomDataType type);

CustomDataType get_customdata_type(const char* const stype);
const char* get_customdata_str(CustomDataType type);
const char* get_binarybridge_path(BinaryType type);

void* get_pointer(quintptr ptr_addr);
PluginCategory get_category_from_name(const char* const name);

const char* get_last_error();
void set_last_error(const char* const error);

#ifndef BUILD_BRIDGE
void set_option(OptionsType option, int value, const char* const valueStr);
void reset_options();

// Global options
struct carla_options_t {
    ProcessModeType process_mode;
    uint max_parameters;
    bool prefer_ui_bridges;
    bool proccess_hq;
    int  osc_gui_timeout;
    bool use_dssi_chunks;
    const char* bridge_unix32;
    const char* bridge_unix64;
    const char* bridge_win32;
    const char* bridge_win64;
    const char* bridge_lv2gtk2;
    const char* bridge_lv2qt4;
    const char* bridge_lv2x11;
    const char* bridge_vstx11;
};
extern carla_options_t carla_options;
#endif

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_SHARED_H

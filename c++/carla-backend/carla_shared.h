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

/*!
 * @defgroup CarlaBackendShared Carla Backend shared code
 * @{
 */

const char* BinaryType2str(const BinaryType type);
const char* PluginType2str(const PluginType type);
const char* PluginCategory2str(const PluginCategory category);
const char* ParameterType2str(const ParameterType type);
const char* InternalParametersIndex2str(const InternalParametersIndex index);
const char* CustomDataType2str(const CustomDataType type);
const char* GuiType2str(const GuiType type);
const char* OptionsType2str(const OptionsType type);
const char* CallbackType2str(const CallbackType type);
const char* ProcessModeType2str(const ProcessModeType type);

CustomDataType getCustomDataStringType(const char* const stype);
const char* getCustomDataTypeString(const CustomDataType type);
const char* getBinaryBidgePath(const BinaryType type);

void* getPointer(const quintptr addr);
PluginCategory getPluginCategoryFromName(const char* const name);

const char* getLastError();
void setLastError(const char* const error);

#ifndef BUILD_BRIDGE
void setOption(const OptionsType option, const int value, const char* const valueStr);
void resetOptions();

// Global options
struct carla_options_t {
    ProcessModeType process_mode;
    uint max_parameters;
    bool prefer_ui_bridges;
    bool force_stereo;
    bool proccess_hp;
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

    carla_options_t()
        : process_mode(PROCESS_MODE_MULTIPLE_CLIENTS),
          max_parameters(MAX_PARAMETERS),
          prefer_ui_bridges(true),
          force_stereo(false),
          proccess_hp(false),
          osc_gui_timeout(4000/100),
          use_dssi_chunks(false),
          bridge_unix32(nullptr),
          bridge_unix64(nullptr),
          bridge_win32(nullptr),
          bridge_win64(nullptr),
          bridge_lv2gtk2(nullptr),
          bridge_lv2qt4(nullptr),
          bridge_lv2x11(nullptr),
          bridge_vstx11(nullptr) {}
};
extern carla_options_t carlaOptions;
#endif

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_SHARED_H

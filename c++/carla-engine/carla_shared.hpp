/*
 * Carla Backend
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#include "carla_backend.hpp"

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendShared Carla Backend Shared Code
 *
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
const char* getPluginTypeString(const PluginType type);

void* getPointer(const uintptr_t addr);
PluginCategory getPluginCategoryFromName(const char* const name);

const char* getLastError();
void setLastError(const char* const error);

#ifndef BUILD_BRIDGE
uint32_t getPluginHintsFromNative(const uint32_t nativeHints);
#endif

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_SHARED_H

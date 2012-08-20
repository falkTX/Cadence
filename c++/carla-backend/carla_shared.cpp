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

#include "carla_shared.h"

#include <QtCore/QString>

CARLA_BACKEND_START_NAMESPACE

static const char* carlaLastError = nullptr;

#ifndef BUILD_BRIDGE
// Global options
carla_options_t carlaOptions;
#endif

// -------------------------------------------------------------------------------------------------------------------

const char* BinaryType2str(const BinaryType type)
{
    switch (type)
    {
    case BINARY_NONE:
        return "BINARY_NONE";
    case BINARY_UNIX32:
        return "BINARY_UNIX32";
    case BINARY_UNIX64:
        return "BINARY_UNIX64";
    case BINARY_WIN32:
        return "BINARY_WIN32";
    case BINARY_WIN64:
        return "BINARY_WIN64";
    case BINARY_OTHER:
        return "BINARY_OTHER";
    }

    qWarning("CarlaBackend::BinaryType2str(%i) - invalid type", type);
    return nullptr;
}

const char* PluginType2str(const PluginType type)
{
    switch (type)
    {
    case PLUGIN_NONE:
        return "PLUGIN_NONE";
    case PLUGIN_LADSPA:
        return "PLUGIN_LADSPA";
    case PLUGIN_DSSI:
        return "PLUGIN_DSSI";
    case PLUGIN_LV2:
        return "PLUGIN_LV2";
    case PLUGIN_VST:
        return "PLUGIN_VST";
    case PLUGIN_GIG:
        return "PLUGIN_GIG";
    case PLUGIN_SF2:
        return "PLUGIN_SF2";
    case PLUGIN_SFZ:
        return "PLUGIN_SFZ";
    }

    qWarning("CarlaBackend::PluginType2str(%i) - invalid type", type);
    return nullptr;
}

const char* PluginCategory2str(const PluginCategory category)
{
    switch (category)
    {
    case PLUGIN_CATEGORY_NONE:
        return "PLUGIN_CATEGORY_NONE";
    case PLUGIN_CATEGORY_SYNTH:
        return "PLUGIN_CATEGORY_SYNTH";
    case PLUGIN_CATEGORY_DELAY:
        return "PLUGIN_CATEGORY_DELAY";
    case PLUGIN_CATEGORY_EQ:
        return "PLUGIN_CATEGORY_EQ";
    case PLUGIN_CATEGORY_FILTER:
        return "PLUGIN_CATEGORY_FILTER";
    case PLUGIN_CATEGORY_DYNAMICS:
        return "PLUGIN_CATEGORY_DYNAMICS";
    case PLUGIN_CATEGORY_MODULATOR:
        return "PLUGIN_CATEGORY_MODULATOR";
    case PLUGIN_CATEGORY_UTILITY:
        return "PLUGIN_CATEGORY_UTILITY";
    case PLUGIN_CATEGORY_OTHER:
        return "PLUGIN_CATEGORY_OTHER";
    }

    qWarning("CarlaBackend::PluginCategory2str(%i) - invalid category", category);
    return nullptr;
}

const char* ParameterType2str(const ParameterType type)
{
    switch (type)
    {
    case PARAMETER_UNKNOWN:
        return "PARAMETER_UNKNOWN";
    case PARAMETER_INPUT:
        return "PARAMETER_INPUT";
    case PARAMETER_OUTPUT:
        return "PARAMETER_OUTPUT";
    case PARAMETER_LATENCY:
        return "PARAMETER_LATENCY";
    case PARAMETER_SAMPLE_RATE:
        return "PARAMETER_SAMPLE_RATE";
    }

    qWarning("CarlaBackend::ParameterType2str(%i) - invalid type", type);
    return nullptr;
}

const char* InternalParametersIndex2str(const InternalParametersIndex index)
{
    switch (index)
    {
    case PARAMETER_NULL:
        return "PARAMETER_NULL";
    case PARAMETER_ACTIVE:
        return "PARAMETER_ACTIVE";
    case PARAMETER_DRYWET:
        return "PARAMETER_DRYWET";
    case PARAMETER_VOLUME:
        return "PARAMETER_VOLUME";
    case PARAMETER_BALANCE_LEFT:
        return "PARAMETER_BALANCE_LEFT";
    case PARAMETER_BALANCE_RIGHT:
        return "PARAMETER_BALANCE_RIGHT";
    }

    qWarning("CarlaBackend::InternalParametersIndex2str(%i) - invalid index", index);
    return nullptr;
}

const char* CustomDataType2str(const CustomDataType type)
{
    switch (type)
    {
    case CUSTOM_DATA_INVALID:
        return "CUSTOM_DATA_INVALID";
    case CUSTOM_DATA_STRING:
        return "CUSTOM_DATA_STRING";
    case CUSTOM_DATA_PATH:
        return "CUSTOM_DATA_PATH";
    case CUSTOM_DATA_CHUNK:
        return "CUSTOM_DATA_CHUNK";
    case CUSTOM_DATA_BINARY:
        return "CUSTOM_DATA_BINARY";
    }

    qWarning("CarlaBackend::CustomDataType2str(%i) - invalid type", type);
    return nullptr;
}

const char* GuiType2str(const GuiType type)
{
    switch (type)
    {
    case GUI_NONE:
        return "GUI_NONE";
    case GUI_INTERNAL_QT4:
        return "GUI_INTERNAL_QT4";
    case GUI_INTERNAL_COCOA:
        return "GUI_INTERNAL_COCOA";
    case GUI_INTERNAL_HWND:
        return "GUI_INTERNAL_HWND";
    case GUI_INTERNAL_X11:
        return "GUI_INTERNAL_X11";
    case GUI_EXTERNAL_LV2:
        return "GUI_EXTERNAL_LV2";
    case GUI_EXTERNAL_SUIL:
        return "GUI_EXTERNAL_SUIL";
    case GUI_EXTERNAL_OSC:
        return "GUI_EXTERNAL_OSC";
    }

    qWarning("CarlaBackend::GuiType2str(%i) - invalid type", type);
    return nullptr;
}

const char* OptionsType2str(const OptionsType type)
{
    switch (type)
    {
    case OPTION_PROCESS_MODE:
        return "OPTION_PROCESS_MODE";
    case OPTION_PROCESS_HIGH_PRECISION:
        return "OPTION_PROCESS_HIGH_PRECISION";
    case OPTION_MAX_PARAMETERS:
        return "OPTION_MAX_PARAMETERS";
    case OPTION_PREFERRED_BUFFER_SIZE:
        return "OPTION_PREFERRED_BUFFER_SIZE";
    case OPTION_PREFERRED_SAMPLE_RATE:
        return "OPTION_PREFERRED_SAMPLE_RATE";
    case OPTION_FORCE_STEREO:
        return "OPTION_FORCE_STEREO";
    case OPTION_USE_DSSI_VST_CHUNKS:
        return "OPTION_USE_DSSI_VST_CHUNKS";
    case OPTION_PREFER_UI_BRIDGES:
        return "OPTION_PREFER_UI_BRIDGES";
    case OPTION_OSC_UI_TIMEOUT:
        return "OPTION_OSC_UI_TIMEOUT";
    case OPTION_PATH_LADSPA:
        return "OPTION_PATH_LADSPA";
    case OPTION_PATH_DSSI:
        return "OPTION_PATH_DSSI";
    case OPTION_PATH_LV2:
        return "OPTION_PATH_LV2";
    case OPTION_PATH_VST:
        return "OPTION_PATH_VST";
    case OPTION_PATH_GIG:
        return "OPTION_PATH_GIG";
    case OPTION_PATH_SF2:
        return "OPTION_PATH_SF2";
    case OPTION_PATH_SFZ:
        return "OPTION_PATH_SFZ";
    case OPTION_PATH_BRIDGE_UNIX32:
        return "OPTION_PATH_BRIDGE_UNIX32";
    case OPTION_PATH_BRIDGE_UNIX64:
        return "OPTION_PATH_BRIDGE_UNIX64";
    case OPTION_PATH_BRIDGE_WIN32:
        return "OPTION_PATH_BRIDGE_WIN32";
    case OPTION_PATH_BRIDGE_WIN64:
        return "OPTION_PATH_BRIDGE_WIN64";
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        return "OPTION_PATH_BRIDGE_LV2_GTK2";
    case OPTION_PATH_BRIDGE_LV2_QT4:
        return "OPTION_PATH_BRIDGE_LV2_QT4";
    case OPTION_PATH_BRIDGE_LV2_X11:
        return "OPTION_PATH_BRIDGE_LV2_X11";
    case OPTION_PATH_BRIDGE_VST_X11:
        return "OPTION_PATH_BRIDGE_VST_X11";
    }

    qWarning("CarlaBackend::OptionsType2str(%i) - invalid type", type);
    return nullptr;
}

const char* CallbackType2str(const CallbackType type)
{
    switch (type)
    {
    case CALLBACK_DEBUG:
        return "CALLBACK_DEBUG";
    case CALLBACK_PARAMETER_VALUE_CHANGED:
        return "CALLBACK_PARAMETER_VALUE_CHANGED";
    case CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        return "CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED";
    case CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        return "CALLBACK_PARAMETER_MIDI_CC_CHANGED";
    case CALLBACK_PROGRAM_CHANGED:
        return "CALLBACK_PROGRAM_CHANGED";
    case CALLBACK_MIDI_PROGRAM_CHANGED:
        return "CALLBACK_MIDI_PROGRAM_CHANGED";
    case CALLBACK_NOTE_ON:
        return "CALLBACK_NOTE_ON";
    case CALLBACK_NOTE_OFF:
        return "CALLBACK_NOTE_OFF";
    case CALLBACK_SHOW_GUI:
        return "CALLBACK_SHOW_GUI";
    case CALLBACK_RESIZE_GUI:
        return "CALLBACK_RESIZE_GUI";
    case CALLBACK_UPDATE:
        return "CALLBACK_UPDATE";
    case CALLBACK_RELOAD_INFO:
        return "CALLBACK_RELOAD_INFO";
    case CALLBACK_RELOAD_PARAMETERS:
        return "CALLBACK_RELOAD_PARAMETERS";
    case CALLBACK_RELOAD_PROGRAMS:
        return "CALLBACK_RELOAD_PROGRAMS";
    case CALLBACK_RELOAD_ALL:
        return "CALLBACK_RELOAD_ALL";
    case CALLBACK_QUIT:
        return "CALLBACK_QUIT";
    }

    qWarning("CarlaBackend::CallbackType2str(%i) - invalid type", type);
    return nullptr;
}

const char* ProcessModeType2str(const ProcessModeType type)
{
    switch (type)
    {
    case PROCESS_MODE_SINGLE_CLIENT:
        return "PROCESS_MODE_SINGLE_CLIENT";
    case PROCESS_MODE_MULTIPLE_CLIENTS:
        return "PROCESS_MODE_MULTIPLE_CLIENTS";
    case PROCESS_MODE_CONTINUOUS_RACK:
        return "PROCESS_MODE_CONTINUOUS_RACK";
    }

    qWarning("CarlaBackend::ProcessModeType2str(%i) - invalid type", type);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CustomDataType getCustomDataStringType(const char* const stype)
{
    qDebug("CarlaBackend::getCustomDataStringType(\"%s\")", stype);

    if (strcmp(stype, "string") == 0)
        return CUSTOM_DATA_STRING;
    if (strcmp(stype, "path") == 0)
        return CUSTOM_DATA_PATH;
    if (strcmp(stype, "chunk") == 0)
        return CUSTOM_DATA_CHUNK;
    if (strcmp(stype, "binary") == 0)
        return CUSTOM_DATA_BINARY;
    return CUSTOM_DATA_INVALID;
}

const char* getCustomDataTypeString(const CustomDataType type)
{
    qDebug("CarlaBackend::getCustomDataTypeString(%s)", CustomDataType2str(type));

    switch (type)
    {
    case CUSTOM_DATA_STRING:
        return "string";
    case CUSTOM_DATA_PATH:
        return "path";
    case CUSTOM_DATA_CHUNK:
        return "chunk";
    case CUSTOM_DATA_BINARY:
        return "binary";
    default:
        return "invalid";
    }
}

const char* getBinaryBidgePath(const BinaryType type)
{
    qDebug("CarlaBackend::getBinaryBidgePath(%s)", BinaryType2str(type));

    switch (type)
    {
#ifndef BUILD_BRIDGE
    case BINARY_UNIX32:
        return carlaOptions.bridge_unix32;
    case BINARY_UNIX64:
        return carlaOptions.bridge_unix64;
    case BINARY_WIN32:
        return carlaOptions.bridge_win32;
    case BINARY_WIN64:
        return carlaOptions.bridge_win64;
#endif
    default:
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------

void* getPointer(const quintptr addr)
{
    Q_ASSERT(addr != 0);
    qDebug("CarlaBackend::getPointer(" P_UINTPTR ")", addr);

    quintptr* const ptr = (quintptr*)addr;
    return (void*)ptr;
}

PluginCategory getPluginCategoryFromName(const char* const name)
{
    Q_ASSERT(name);
    qDebug("CarlaBackend::getPluginCategoryFromName(\"%s\")", name);

    QString qname(name);

    if (qname.isEmpty())
        return PLUGIN_CATEGORY_NONE;

    qname = qname.toLower();

    // generic tags first
    if (qname.contains("delay", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DELAY;
    if (qname.contains("reverb", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DELAY;

    // filter
    if (qname.contains("filter", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_FILTER;

    // dynamics
    if (qname.contains("dynamics", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("amplifier", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("compressor", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("enhancer", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("exciter", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("gate", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;
    if (qname.contains("limiter", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DYNAMICS;

    // modulator
    if (qname.contains("modulator", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_MODULATOR;
    if (qname.contains("chorus", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_MODULATOR;
    if (qname.contains("flanger", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_MODULATOR;
    if (qname.contains("phaser", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_MODULATOR;
    if (qname.contains("saturator", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_MODULATOR;

    // unitily
    if (qname.contains("utility", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;
    if (qname.contains("analyzer", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;
    if (qname.contains("converter", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;
    if (qname.contains("deesser", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;
    if (qname.contains("mixer", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;

    // common tags
    if (qname.contains("verb", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DELAY;

    if (qname.contains("eq", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_EQ;

    if (qname.contains("tool", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_UTILITY;

    return PLUGIN_CATEGORY_NONE;
}

// -------------------------------------------------------------------------------------------------------------------

const char* getLastError()
{
    qDebug("CarlaBackend::getLastError()");

    return carlaLastError;
}

void setLastError(const char* const error)
{
    qDebug("CarlaBackend::setLastError(\"%s\")", error);

    if (carlaLastError)
        free((void*)carlaLastError);

    carlaLastError = error ? strdup(error) : nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void setOption(const OptionsType option, const int value, const char* const valueStr)
{
    qDebug("CarlaBackend::setOption(%s, %i, \"%s\")", OptionsType2str(option), value, valueStr);

    switch (option)
    {
    case OPTION_PROCESS_MODE:
        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_CONTINUOUS_RACK)
            return qCritical("CarlaBackend::setOption(%s, %i, \"%s\") - invalid value", OptionsType2str(option), value, valueStr);
        carlaOptions.processMode = (ProcessModeType)value;
        break;
    case OPTION_PROCESS_HIGH_PRECISION:
        carlaOptions.processHighPrecision = value;
        break;
    case OPTION_MAX_PARAMETERS:
        carlaOptions.maxParameters = (value > 0) ? value : MAX_PARAMETERS;
        break;
    case OPTION_PREFERRED_BUFFER_SIZE:
        carlaOptions.preferredBufferSize = value;
        break;
    case OPTION_PREFERRED_SAMPLE_RATE:
        carlaOptions.preferredSampleRate = value;
        break;
    case OPTION_FORCE_STEREO:
        carlaOptions.forceStereo = value;
        break;
    case OPTION_USE_DSSI_VST_CHUNKS:
        carlaOptions.useDssiVstChunks = value;
        break;
    case OPTION_PREFER_UI_BRIDGES:
        carlaOptions.preferUiBridges = value;
        break;
    case OPTION_OSC_UI_TIMEOUT:
        carlaOptions.oscUiTimeout = value/100;
        break;
    case OPTION_PATH_LADSPA:
        carla_setenv("LADSPA_PATH", valueStr);
        break;
    case OPTION_PATH_DSSI:
        carla_setenv("DSSI_PATH", valueStr);
        break;
    case OPTION_PATH_LV2:
        carla_setenv("LV2_PATH", valueStr);
        break;
    case OPTION_PATH_VST:
        carla_setenv("VST_PATH", valueStr);
        break;
    case OPTION_PATH_GIG:
        carla_setenv("GIG_PATH", valueStr);
        break;
    case OPTION_PATH_SF2:
        carla_setenv("SF2_PATH", valueStr);
        break;
    case OPTION_PATH_SFZ:
        carla_setenv("SFZ_PATH", valueStr);
        break;
    case OPTION_PATH_BRIDGE_UNIX32:
        carlaOptions.bridge_unix32 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_UNIX64:
        carlaOptions.bridge_unix64 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_WIN32:
        carlaOptions.bridge_win32 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_WIN64:
        carlaOptions.bridge_win64 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        carlaOptions.bridge_lv2gtk2 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        carlaOptions.bridge_lv2qt4 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        carlaOptions.bridge_lv2x11 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        carlaOptions.bridge_vstx11 = strdup(valueStr);
        break;
    }
}

void resetOptions()
{
    qDebug("CarlaBackend::resetOptions()");

    if (carlaOptions.bridge_unix32)
        free((void*)carlaOptions.bridge_unix32);

    if (carlaOptions.bridge_unix64)
        free((void*)carlaOptions.bridge_unix64);

    if (carlaOptions.bridge_win32)
        free((void*)carlaOptions.bridge_win32);

    if (carlaOptions.bridge_win64)
        free((void*)carlaOptions.bridge_win64);

    if (carlaOptions.bridge_lv2gtk2)
        free((void*)carlaOptions.bridge_lv2gtk2);

    if (carlaOptions.bridge_lv2qt4)
        free((void*)carlaOptions.bridge_lv2qt4);

    if (carlaOptions.bridge_lv2x11)
        free((void*)carlaOptions.bridge_lv2x11);

    if (carlaOptions.bridge_vstx11)
        free((void*)carlaOptions.bridge_vstx11);

    carlaOptions.processMode          = PROCESS_MODE_MULTIPLE_CLIENTS;
    carlaOptions.processHighPrecision = false;
    carlaOptions.maxParameters        = MAX_PARAMETERS;
    carlaOptions.preferredBufferSize  = 512;
    carlaOptions.preferredSampleRate  = 44100;
    carlaOptions.forceStereo          = false;
    carlaOptions.useDssiVstChunks     = false;
    carlaOptions.preferUiBridges      = true;
    carlaOptions.oscUiTimeout         = 4000/100;

    carlaOptions.bridge_unix32  = nullptr;
    carlaOptions.bridge_unix64  = nullptr;
    carlaOptions.bridge_win32   = nullptr;
    carlaOptions.bridge_win64   = nullptr;
    carlaOptions.bridge_lv2gtk2 = nullptr;
    carlaOptions.bridge_lv2qt4  = nullptr;
    carlaOptions.bridge_lv2x11  = nullptr;
    carlaOptions.bridge_vstx11  = nullptr;
}
#endif // BUILD_BRIDGE

CARLA_BACKEND_END_NAMESPACE

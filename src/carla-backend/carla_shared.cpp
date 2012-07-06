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

#include <cassert>
#include <QtCore/QString>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

static const char* carla_last_error = nullptr;

#ifndef BUILD_BRIDGE
// Global options
carla_options_t carla_options = {
    /* process_mode       */ PROCESS_MODE_MULTIPLE_CLIENTS,
    /* max_parameters     */ MAX_PARAMETERS,
    /* prefer_ui_bridges  */ true,
    /* proccess_hq        */ false,
    /* osc_gui_timeout    */ 4000/100,
    /* use_dssi_chunks    */ false,
    /* bridge_unix32      */ nullptr,
    /* bridge_unix64      */ nullptr,
    /* bridge_win32       */ nullptr,
    /* bridge_win64       */ nullptr,
    /* bridge_lv2gtk2     */ nullptr,
    /* bridge_lv2qt4      */ nullptr,
    /* bridge_lv2x11      */ nullptr,
    /* bridge_vstx11      */ nullptr
};
#endif

// -------------------------------------------------------------------------------------------------------------------

const char* binarytype2str(BinaryType type)
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
    }

    qWarning("CarlaBackend::binarytype2str(%i) - invalid type", type);
    return nullptr;
}

const char* plugintype2str(PluginType type)
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

    qWarning("CarlaBackend::plugintype2str(%i) - invalid type", type);
    return nullptr;
}

const char* optionstype2str(OptionsType type)
{
    switch (type)
    {
    case OPTION_PROCESS_MODE:
        return "OPTION_PROCESS_MODE";
    case OPTION_MAX_PARAMETERS:
        return "OPTION_MAX_PARAMETERS";
    case OPTION_PREFER_UI_BRIDGES:
        return "OPTION_PREFER_UI_BRIDGES";
    case OPTION_PROCESS_HQ:
        return "OPTION_PROCESS_HQ";
    case OPTION_OSC_GUI_TIMEOUT:
        return "OPTION_OSC_GUI_TIMEOUT";
    case OPTION_USE_DSSI_CHUNKS:
        return "OPTION_USE_DSSI_CHUNKS";
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

    qWarning("CarlaBackend::optionstype2str(%i) - invalid type", type);
    return nullptr;
}

const char* customdatatype2str(CustomDataType type)
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

    qWarning("CarlaBackend::customdatatype2str(%i) - invalid type", type);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CustomDataType get_customdata_type(const char* const stype)
{
    qDebug("CarlaBackend::get_customdata_type(%s)", stype);

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

const char* get_customdata_str(CustomDataType type)
{
    qDebug("CarlaBackend::get_customdata_str(%s)", customdatatype2str(type));

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

const char* get_binarybridge_path(BinaryType type)
{
    qDebug("CarlaBackend::get_bridge_path(%s)", binarytype2str(type));

    switch (type)
    {
#ifndef BUILD_BRIDGE
    case BINARY_UNIX32:
        return carla_options.bridge_unix32;
    case BINARY_UNIX64:
        return carla_options.bridge_unix64;
    case BINARY_WIN32:
        return carla_options.bridge_win32;
    case BINARY_WIN64:
        return carla_options.bridge_win64;
#endif
    default:
        return nullptr;
    }
}

// -------------------------------------------------------------------------------------------------------------------

void* get_pointer(quintptr ptr_addr)
{
    qDebug("CarlaBackend::get_pointer(" P_UINTPTR ")", ptr_addr);

    quintptr* ptr = (quintptr*)ptr_addr;
    return (void*)ptr;
}

PluginCategory get_category_from_name(const char* const name)
{
    qDebug("CarlaBackend::get_category_from_name(%s)", name);
    assert(name);

    QString qname(name);

    if (qname.isEmpty())
        return PLUGIN_CATEGORY_NONE;

    qname = qname.toLower();

    // generic tags first
    if (qname.contains("delay", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DELAY;
    if (qname.contains("reverb", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_DELAY;

    if (qname.contains("filter", Qt::CaseSensitive))
        return PLUGIN_CATEGORY_FILTER;

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

const char* get_last_error()
{
    qDebug("CarlaBackend::get_last_error()");

    return carla_last_error;
}

void set_last_error(const char* const error)
{
    qDebug("CarlaBackend::set_last_error(%s)", error);

    if (carla_last_error)
        free((void*)carla_last_error);

    carla_last_error = error ? strdup(error) : nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

#ifndef BUILD_BRIDGE
void set_option(OptionsType option, int value, const char* const valueStr)
{
    qDebug("CarlaBackend::set_option(%s, %i, %s)", optionstype2str(option), value, valueStr);

    switch (option)
    {
    case OPTION_PROCESS_MODE:
        if (value < PROCESS_MODE_SINGLE_CLIENT || value > PROCESS_MODE_CONTINUOUS_RACK)
            value = PROCESS_MODE_MULTIPLE_CLIENTS;
        carla_options.process_mode = (ProcessModeType)value;
        break;
    case OPTION_MAX_PARAMETERS:
        carla_options.max_parameters = (value > 0) ? value : MAX_PARAMETERS;
        break;
    case OPTION_PREFER_UI_BRIDGES:
        carla_options.prefer_ui_bridges = value;
        break;
    case OPTION_PROCESS_HQ:
        carla_options.proccess_hq = value;
        break;
    case OPTION_OSC_GUI_TIMEOUT:
        carla_options.osc_gui_timeout = value/100;
        break;
    case OPTION_USE_DSSI_CHUNKS:
        carla_options.use_dssi_chunks = value;
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
        carla_options.bridge_unix32 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_UNIX64:
        carla_options.bridge_unix64 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_WIN32:
        carla_options.bridge_win32 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_WIN64:
        carla_options.bridge_win64 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_GTK2:
        carla_options.bridge_lv2gtk2 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_QT4:
        carla_options.bridge_lv2qt4 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_LV2_X11:
        carla_options.bridge_lv2x11 = strdup(valueStr);
        break;
    case OPTION_PATH_BRIDGE_VST_X11:
        carla_options.bridge_vstx11 = strdup(valueStr);
        break;
    }
}

void reset_options()
{
    qDebug("CarlaBackend::reset_options()");

    if (carla_options.bridge_unix32)
        free((void*)carla_options.bridge_unix32);

    if (carla_options.bridge_unix64)
        free((void*)carla_options.bridge_unix64);

    if (carla_options.bridge_win32)
        free((void*)carla_options.bridge_win32);

    if (carla_options.bridge_win64)
        free((void*)carla_options.bridge_win64);

    if (carla_options.bridge_lv2gtk2)
        free((void*)carla_options.bridge_lv2gtk2);

    if (carla_options.bridge_lv2qt4)
        free((void*)carla_options.bridge_lv2qt4);

    if (carla_options.bridge_lv2x11)
        free((void*)carla_options.bridge_lv2x11);

    if (carla_options.bridge_vstx11)
        free((void*)carla_options.bridge_vstx11);

    carla_options.process_mode      = PROCESS_MODE_MULTIPLE_CLIENTS;
    carla_options.max_parameters    = MAX_PARAMETERS;
    carla_options.prefer_ui_bridges = true;
    carla_options.proccess_hq       = false;
    carla_options.osc_gui_timeout   = 4000/100;
    carla_options.use_dssi_chunks   = false;
    carla_options.bridge_unix32     = nullptr;
    carla_options.bridge_unix64     = nullptr;
    carla_options.bridge_win32      = nullptr;
    carla_options.bridge_win64      = nullptr;
    carla_options.bridge_lv2gtk2    = nullptr;
    carla_options.bridge_lv2qt4     = nullptr;
    carla_options.bridge_lv2x11     = nullptr;
    carla_options.bridge_vstx11     = nullptr;
}
#endif

CARLA_BACKEND_END_NAMESPACE

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

#include "carla_backend_standalone.h"
#include "carla_plugin.h"

// Single, standalone engine
#if CARLA_ENGINE_JACK
CarlaBackend::CarlaEngineJack carla_engine;
#elif CARLA_ENGINE_RTAUDIO
CarlaBackend::CarlaEngineRtAudio carla_engine;
#else
#error Invalid engine type!
#endif

// -------------------------------------------------------------------------------------------------------------------

bool engine_init(const char* client_name)
{
    qDebug("engine_init(%s)", client_name);

#ifndef Q_OS_WIN
    // TODO - make this an option, put somewhere else
    if (! getenv("WINE_RT"))
    {
        carla_setenv("WINE_RT", "15");
        carla_setenv("WINE_SVR_RT", "10");
    }
#endif

    bool started = carla_engine.init(client_name);

    if (started)
        CarlaBackend::set_last_error("no error");

    return started;
}

bool engine_close()
{
    qDebug("engine_close()");

    bool closed = carla_engine.close();

    for (unsigned short i=0; i < CarlaBackend::MAX_PLUGINS; i++)
        remove_plugin(i);

    // cleanup static data
    get_plugin_info(0);
    get_parameter_info(0, 0);
    get_parameter_scalepoint_info(0, 0, 0);
    get_chunk_data(0);
    get_parameter_text(0, 0);
    get_program_name(0, 0);
    get_midi_program_name(0, 0);
    get_real_plugin_name(0);

    CarlaBackend::reset_options();
    CarlaBackend::set_last_error(nullptr);

    return closed;
}

bool is_engine_running()
{
    qDebug("is_engine_running()");

    return carla_engine.isRunning();
}

// -------------------------------------------------------------------------------------------------------------------

short add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* const name, const char* label, void* extra_stuff)
{
    qDebug("add_plugin(%s, %s, %s, %s, %s, %p)", CarlaBackend::binarytype2str(btype), CarlaBackend::plugintype2str(ptype), filename, name, label, extra_stuff);

    return -1;

#if 0
    CarlaBackend::CarlaPlugin::initializer init = {
        &carla_engine,
        filename,
        name,
        label
    };

#ifndef BUILD_BRIDGE
    if (btype != BINARY_NATIVE)
    {
#ifdef CARLA_ENGINE_JACK
        if (CarlaBackend::carla_options.process_mode != CarlaBackend::PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            CarlaBackend::set_last_error("Can only use bridged plugins in JACK Multi-Client mode");
            return -1;
        }
#else
        CarlaBackend::set_last_error("Can only use bridged plugins with JACK backend");
        return -1;
#endif

        return CarlaBackend::CarlaPlugin::newBridge(init, btype, ptype);
    }
#endif

    switch (ptype)
    {
    case CarlaBackend::PLUGIN_LADSPA:
        return CarlaBackend::CarlaPlugin::newLADSPA(init, extra_stuff);
    case CarlaBackend::PLUGIN_DSSI:
        return CarlaBackend::CarlaPlugin::newDSSI(init, extra_stuff);
    case CarlaBackend::PLUGIN_LV2:
        return CarlaBackend::CarlaPlugin::newLV2(init);
    case CarlaBackend::PLUGIN_VST:
        return CarlaBackend::CarlaPlugin::newVST(init);
    case CarlaBackend::PLUGIN_GIG:
        return CarlaBackend::CarlaPlugin::newGIG(init);
    case CarlaBackend::PLUGIN_SF2:
        return CarlaBackend::CarlaPlugin::newSF2(init);
    case CarlaBackend::PLUGIN_SFZ:
        return CarlaBackend::CarlaPlugin::newSFZ(init);
    default:
        CarlaBackend::set_last_error("Unknown plugin type");
        return -1;
    }
#endif
}

bool remove_plugin(unsigned short plugin_id)
{
    qDebug("remove_plugin(%i)", plugin_id);

    return carla_engine.removePlugin(plugin_id);
}

// -------------------------------------------------------------------------------------------------------------------

const PluginInfo* get_plugin_info(unsigned short plugin_id)
{
    qDebug("get_plugin_info(%i)", plugin_id);

    static PluginInfo info = { CarlaBackend::PLUGIN_NONE, CarlaBackend::PLUGIN_CATEGORY_NONE, 0x0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 };

    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    if (info.maker)
    {
        free((void*)info.maker);
        info.maker = nullptr;
    }

    if (info.copyright)
    {
        free((void*)info.copyright);
        info.copyright = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        char buf_str[STR_MAX] = { 0 };

        info.type     = plugin->type();
        info.category = plugin->category();
        info.hints    = plugin->hints();
        info.binary   = plugin->filename();
        info.name     = plugin->name();
        info.uniqueId = plugin->uniqueId();

        plugin->getLabel(buf_str);
        info.label = strdup(buf_str);

        plugin->getMaker(buf_str);
        info.maker = strdup(buf_str);

        plugin->getCopyright(buf_str);
        info.copyright = strdup(buf_str);

        return &info;
    }

    if (carla_engine.isRunning())
        qCritical("get_plugin_info(%i) - could not find plugin", plugin_id);

    return &info;
}

const PortCountInfo* get_audio_port_count_info(unsigned short plugin_id)
{
    qDebug("get_audio_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { 0, 0, 0 };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        info.ins   = plugin->audioInCount();
        info.outs  = plugin->audioOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("get_audio_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_midi_port_count_info(unsigned short plugin_id)
{
    qDebug("get_midi_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { 0, 0, 0 };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        info.ins   = plugin->midiInCount();
        info.outs  = plugin->midiOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("get_midi_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_parameter_count_info(unsigned short plugin_id)
{
    qDebug("get_parameter_port_count_info(%i)", plugin_id);

    static PortCountInfo info = { 0, 0, 0 };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        plugin->getParameterCountInfo(&info.ins, &info.outs, &info.total);
        return &info;
    }

    qCritical("get_parameter_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const ParameterInfo* get_parameter_info(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_parameter_info(%i, %i)", plugin_id, parameter_id);

    static ParameterInfo info = { nullptr, nullptr, nullptr, 0 };

    if (info.name)
    {
        free((void*)info.name);
        info.name = nullptr;
    }

    if (info.symbol)
    {
        free((void*)info.symbol);
        info.symbol = nullptr;
    }

    if (info.unit)
    {
        free((void*)info.unit);
        info.unit = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            char buf_str[STR_MAX] = { 0 };

            info.scalePointCount = plugin->parameterScalePointCount(parameter_id);

            plugin->getParameterName(parameter_id, buf_str);
            info.name = strdup(buf_str);

            plugin->getParameterSymbol(parameter_id, buf_str);
            info.symbol = strdup(buf_str);

            plugin->getParameterUnit(parameter_id, buf_str);
            info.unit = strdup(buf_str);
        }
        else
            qCritical("get_parameter_info(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

        return &info;
    }

    if (carla_engine.isRunning())
        qCritical("get_parameter_info(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return &info;
}

const ScalePointInfo* get_parameter_scalepoint_info(unsigned short plugin_id, quint32 parameter_id, quint32 scalepoint_id)
{
    qDebug("get_parameter_scalepoint_info(%i, %i, %i)", plugin_id, parameter_id, scalepoint_id);

    static ScalePointInfo info = { 0.0, nullptr };

    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            if (scalepoint_id < plugin->parameterScalePointCount(parameter_id))
            {
                char buf_str[STR_MAX] = { 0 };

                info.value = plugin->getParameterScalePointValue(parameter_id, scalepoint_id);

                plugin->getParameterScalePointLabel(parameter_id, scalepoint_id, buf_str);
                info.label = strdup(buf_str);
            }
            else
                qCritical("get_parameter_scalepoint_info(%i, %i, %i) - scalepoint_id out of bounds", plugin_id, parameter_id, scalepoint_id);
        }
        else
            qCritical("get_parameter_scalepoint_info(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, parameter_id);

        return &info;
    }

    if (carla_engine.isRunning())
        qCritical("get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, scalepoint_id);

    return &info;
}

const GuiInfo* get_gui_info(unsigned short plugin_id)
{
    qDebug("get_gui_info(%i)", plugin_id);

    static GuiInfo info = { CarlaBackend::GUI_NONE, false };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        plugin->getGuiInfo(&info.type, &info.resizable);
        return &info;
    }

    qCritical("get_gui_info(%i) - could not find plugin", plugin_id);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaBackend::ParameterData* get_parameter_data(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_parameter_data(%i, %i)", plugin_id, parameter_id);

    static CarlaBackend::ParameterData data = { CarlaBackend::PARAMETER_UNKNOWN, -1, -1, 0, 0, -1 };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterData(parameter_id);

        qCritical("get_parameter_data(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return &data;
    }

    qCritical("get_parameter_data(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &data;
}

const CarlaBackend::ParameterRanges* get_parameter_ranges(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_parameter_ranges(%i, %i)", plugin_id, parameter_id);

    static CarlaBackend::ParameterRanges ranges = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id);

        qCritical("get_parameter_ranges(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return &ranges;
    }

    qCritical("get_parameter_ranges(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &ranges;
}

const CarlaBackend::midi_program_t* get_midi_program_data(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("get_midi_program_data(%i, %i)", plugin_id, midi_program_id);

    static CarlaBackend::midi_program_t data = { 0, 0, nullptr };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            return plugin->midiProgramData(midi_program_id);

        qCritical("get_midi_program_data(%i, %i) - midi_program_id out of bounds", plugin_id, midi_program_id);
        return &data;
    }

    qCritical("get_midi_program_data(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return &data;
}

const CarlaBackend::CustomData* get_custom_data(unsigned short plugin_id, quint32 custom_data_id)
{
    qDebug("get_custom_data(%i, %i)", plugin_id, custom_data_id);

    static CarlaBackend::CustomData data = { CarlaBackend::CUSTOM_DATA_INVALID, nullptr, nullptr };

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (custom_data_id < plugin->customDataCount())
            return plugin->customData(custom_data_id);

        qCritical("get_custom_data(%i, %i) - custom_data_id out of bounds", plugin_id, custom_data_id);
        return &data;
    }

    qCritical("get_custom_data(%i, %i) - could not find plugin", plugin_id, custom_data_id);
    return &data;
}

const char* get_chunk_data(unsigned short plugin_id)
{
    qDebug("get_chunk_data(%i)", plugin_id);

    static const char* chunk_data = nullptr;

    if (chunk_data)
    {
        free((void*)chunk_data);
        chunk_data = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            int32_t data_size = plugin->chunkData(&data);

            if (data && data_size >= 4)
            {
                QByteArray chunk((const char*)data, data_size);
                chunk_data = strdup(chunk.toBase64().data());
            }
            else
                qCritical("get_chunk_data(%i) - got invalid chunk data", plugin_id);
        }
        else
            qCritical("get_chunk_data(%i) - plugin does not support chunks", plugin_id);

        return chunk_data;
    }

    if (carla_engine.isRunning())
        qCritical("get_chunk_data(%i) - could not find plugin", plugin_id);

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_parameter_count(unsigned short plugin_id)
{
    qDebug("get_parameter_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->parameterCount();

    qCritical("get_parameter_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_program_count(unsigned short plugin_id)
{
    qDebug("get_program_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->programCount();

    qCritical("get_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_midi_program_count(unsigned short plugin_id)
{
    qDebug("get_midi_program_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->midiProgramCount();

    qCritical("get_midi_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_custom_data_count(unsigned short plugin_id)
{
    qDebug("get_custom_data_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->customDataCount();

    qCritical("get_custom_data_count(%i) - could not find plugin", plugin_id);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_parameter_text(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_parameter_text(%i, %i)", plugin_id, parameter_id);

    static char buf_text[STR_MAX] = { 0 };
    memset(buf_text, 0, sizeof(char)*STR_MAX);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            plugin->getParameterText(parameter_id, buf_text);
            return buf_text;
        }

        qCritical("get_parameter_text(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return nullptr;
    }

    if (carla_engine.isRunning())
        qCritical("get_parameter_text(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return nullptr;
}

const char* get_program_name(unsigned short plugin_id, quint32 program_id)
{
    qDebug("get_program_name(%i, %i)", plugin_id, program_id);

    static const char* program_name = nullptr;

    if (program_name)
    {
        free((void*)program_name);
        program_name = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (program_id < plugin->programCount())
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->getProgramName(program_id, buf_str);
            program_name = strdup(buf_str);

            return program_name;
        }

        qCritical("get_program_name(%i, %i) - program_id out of bounds", plugin_id, program_id);
        return nullptr;
    }

    if (carla_engine.isRunning())
        qCritical("get_program_name(%i, %i) - could not find plugin", plugin_id, program_id);

    return nullptr;
}

const char* get_midi_program_name(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("get_midi_program_name(%i, %i)", plugin_id, midi_program_id);

    static const char* midi_program_name = nullptr;

    if (midi_program_name)
        free((void*)midi_program_name);

    midi_program_name = nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->getMidiProgramName(midi_program_id, buf_str);
            midi_program_name = strdup(buf_str);

            return midi_program_name;
        }

        qCritical("get_midi_program_name(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);
        return nullptr;
    }

    if (carla_engine.isRunning())
        qCritical("get_midi_program_name(%i, %i) - could not find plugin", plugin_id, midi_program_id);

    return nullptr;
}

const char* get_real_plugin_name(unsigned short plugin_id)
{
    qDebug("get_real_plugin_name(%i)", plugin_id);

    static const char* real_plugin_name = nullptr;

    if (real_plugin_name)
    {
        free((void*)real_plugin_name);
        real_plugin_name = nullptr;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        char buf_str[STR_MAX] = { 0 };

        plugin->getRealName(buf_str);
        real_plugin_name = strdup(buf_str);

        return real_plugin_name;
    }

    if (carla_engine.isRunning())
        qCritical("get_real_plugin_name(%i) - could not find plugin", plugin_id);

    return real_plugin_name;
}

// -------------------------------------------------------------------------------------------------------------------

qint32 get_current_program_index(unsigned short plugin_id)
{
    qDebug("get_current_program_index(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->currentProgram();

    qCritical("get_current_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

qint32 get_current_midi_program_index(unsigned short plugin_id)
{
    qDebug("get_current_midi_program_index(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->currentMidiProgram();

    qCritical("get_current_midi_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

double get_default_parameter_value(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_default_parameter_value(%i, %i)", plugin_id, parameter_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id)->def;

        qCritical("get_default_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return 0.0;
    }

    qCritical("get_default_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

double get_current_parameter_value(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("get_current_parameter_value(%i, %i)", plugin_id, parameter_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->getParameterValue(parameter_id);

        qCritical("get_current_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return 0.0;
    }

    qCritical("get_current_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

double get_input_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (port_id == 1 || port_id == 2)
        return carla_engine.getInputPeak(plugin_id, port_id-1);
    return 0.0;
}

double get_output_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (port_id == 1 || port_id == 2)
        return carla_engine.getOutputPeak(plugin_id, port_id-1);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

void set_active(unsigned short plugin_id, bool onoff)
{
    qDebug("set_active(%i, %s)", plugin_id, bool2str(onoff));

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setActive(onoff, true, false);

    qCritical("set_active(%i, %s) - could not find plugin", plugin_id, bool2str(onoff));
}

void set_drywet(unsigned short plugin_id, double value)
{
    qDebug("set_drywet(%i, %f)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setDryWet(value, true, false);

    qCritical("set_drywet(%i, %f) - could not find plugin", plugin_id, value);
}

void set_volume(unsigned short plugin_id, double value)
{
    qDebug("set_vol(%i, %f)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setVolume(value, true, false);

    qCritical("set_vol(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_left(unsigned short plugin_id, double value)
{
    qDebug("set_balance_left(%i, %f)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setBalanceLeft(value, true, false);

    qCritical("set_balance_left(%i, %f) - could not find plugin", plugin_id, value);
}

void set_balance_right(unsigned short plugin_id, double value)
{
    qDebug("set_balance_right(%i, %f)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setBalanceRight(value, true, false);

    qCritical("set_balance_right(%i, %f) - could not find plugin", plugin_id, value);
}

// -------------------------------------------------------------------------------------------------------------------

void set_parameter_value(unsigned short plugin_id, quint32 parameter_id, double value)
{
    qDebug("set_parameter_value(%i, %i, %f)", plugin_id, parameter_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterValue(parameter_id, value, true, true, false);

        qCritical("set_parameter_value(%i, %i, %f) - parameter_id out of bounds", plugin_id, parameter_id, value);
        return;
    }

    qCritical("set_parameter_value(%i, %i, %f) - could not find plugin", plugin_id, parameter_id, value);
}

void set_parameter_midi_channel(unsigned short plugin_id, quint32 parameter_id, quint8 channel)
{
    qDebug("set_parameter_midi_channel(%i, %i, %i)", plugin_id, parameter_id, channel);

    if (channel > 15)
    {
        qCritical("set_parameter_midi_channel(%i, %i, %i) - invalid channel number", plugin_id, parameter_id, channel);
        return;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiChannel(parameter_id, channel);

        qCritical("set_parameter_midi_channel(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, channel);
        return;
    }

    qCritical("set_parameter_midi_channel(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, channel);
}

void set_parameter_midi_cc(unsigned short plugin_id, quint32 parameter_id, int16_t midi_cc)
{
    qDebug("set_parameter_midi_cc(%i, %i, %i)", plugin_id, parameter_id, midi_cc);

    if (midi_cc < -1)
    {
        midi_cc = -1;
    }
    else if (midi_cc > 0x5F) // 95
    {
        qCritical("set_parameter_midi_cc(%i, %i, %i) - invalid midi_cc number", plugin_id, parameter_id, midi_cc);
        return;
    }

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiCC(parameter_id, midi_cc);

        qCritical("set_parameter_midi_cc(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, midi_cc);
        return;
    }

    qCritical("set_parameter_midi_cc(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, midi_cc);
}

void set_program(unsigned short plugin_id, quint32 program_id)
{
    qDebug("set_program(%i, %i)", plugin_id, program_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (program_id < plugin->programCount())
            return plugin->setProgram(program_id, true, true, false, true);

        qCritical("set_program(%i, %i) - program_id out of bounds", plugin_id, program_id);
        return;
    }

    qCritical("set_program(%i, %i) - could not find plugin", plugin_id, program_id);
}

void set_midi_program(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("set_midi_program(%i, %i)", plugin_id, midi_program_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            plugin->setMidiProgram(midi_program_id, true, true, false, true);

        qCritical("set_midi_program(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);
        return;
    }

    qCritical("set_midi_program(%i, %i) - could not find plugin", plugin_id, midi_program_id);
}

// -------------------------------------------------------------------------------------------------------------------

void set_custom_data(unsigned short plugin_id, CarlaBackend::CustomDataType type, const char* key, const char* value)
{
    qDebug("set_custom_data(%i, %i, %s, %s)", plugin_id, type, key, value);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->setCustomData(type, key, value, true);

    qCritical("set_custom_data(%i, %i, %s, %s) - could not find plugin", plugin_id, type, key, value);
}

void set_chunk_data(unsigned short plugin_id, const char* chunk_data)
{
    qDebug("set_chunk_data(%i, %s)", plugin_id, chunk_data);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
            return plugin->setChunkData(chunk_data);

        qCritical("set_chunk_data(%i, %s) - plugin does not support chunks", plugin_id, chunk_data);
        return;
    }

    qCritical("set_chunk_data(%i, %s) - could not find plugin", plugin_id, chunk_data);
}

void set_gui_data(unsigned short plugin_id, int data, quintptr gui_addr)
{
    qDebug("set_gui_data(%i, %i, " P_UINTPTR ")", plugin_id, data, gui_addr);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
    {
#ifdef __WINE__
        plugin->setGuiData(data, (HWND)gui_addr);
#else
        plugin->setGuiData(data, (QDialog*)CarlaBackend::get_pointer(gui_addr));
#endif
        return;
    }

    qCritical("set_gui_data(%i, %i, " P_UINTPTR ") - could not find plugin", plugin_id, data, gui_addr);
}

// -------------------------------------------------------------------------------------------------------------------

void show_gui(unsigned short plugin_id, bool yesno)
{
    qDebug("show_gui(%i, %s)", plugin_id, bool2str(yesno));

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->showGui(yesno);

    qCritical("show_gui(%i, %s) - could not find plugin", plugin_id, bool2str(yesno));
}

void idle_guis()
{
    for (unsigned short i=0; i < CarlaBackend::MAX_PLUGINS; i++)
    {
        CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginByIndex(i);

        if (plugin && plugin->enabled())
            plugin->idleGui();
    }
}

// -------------------------------------------------------------------------------------------------------------------

void send_midi_note(unsigned short plugin_id, quint8 note, quint8 velocity)
{
    qDebug("send_midi_note(%i, %i, %i)", plugin_id, note, velocity);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->sendMidiSingleNote(note, velocity, true, true, false);

    qCritical("send_midi_note(%i, %i, %i) - could not find plugin", plugin_id, note, velocity);
}

void prepare_for_save(unsigned short plugin_id)
{
    qDebug("prepare_for_save(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carla_engine.getPluginById(plugin_id);

    if (plugin)
        return plugin->prepareForSave();

    qCritical("prepare_for_save(%i) - could not find plugin", plugin_id);
}

// -------------------------------------------------------------------------------------------------------------------

quint32 get_buffer_size()
{
    qDebug("get_buffer_size()");

    return carla_engine.getBufferSize();
}

double get_sample_rate()
{
    qDebug("get_sample_rate()");

    return carla_engine.getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_last_error()
{
    return CarlaBackend::get_last_error();
}

const char* get_host_osc_url()
{
    qDebug("get_host_osc_url()");

    return carla_engine.getOscUrl();
}

// -------------------------------------------------------------------------------------------------------------------

void set_callback_function(CarlaBackend::CallbackFunc func)
{
    qDebug("set_callback_function(%p)", func);

    carla_engine.setCallback(func);
}

void set_option(CarlaBackend::OptionsType option, int value, const char* valueStr)
{
    CarlaBackend::set_option(option, value, valueStr);
}

// -------------------------------------------------------------------------------------------------------------------

#ifdef QTCREATOR_TEST

#include <QtGui/QApplication>
#include <QtGui/QDialog>

QDialog* gui;

#ifndef CARLA_BACKEND_NO_NAMESPACE
using namespace CarlaBackend;
#endif

void main_callback(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3)
{
    qDebug("Callback(%i, %u, %i, %i, %f)", action, plugin_id, value1, value2, value3);

    switch (action)
    {
    case CALLBACK_SHOW_GUI:
        if (! value1)
            gui->close();
        break;
    case CALLBACK_RESIZE_GUI:
        gui->setFixedSize(value1, value2);
        break;
    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    gui = new QDialog(nullptr);

    if (engine_init("carla_demo"))
    {
        set_callback_function(main_callback);
        set_option(OPTION_PROCESS_MODE, PROCESS_MODE_CONTINUOUS_RACK, nullptr);

        short id = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "xxx", "name!!!", "http://linuxdsp.co.uk/lv2/peq-2a.lv2", nullptr);

        if (id >= 0)
        {
            qDebug("Main Initiated, id = %u", id);

            const GuiInfo* guiInfo = get_gui_info(id);

            if (guiInfo->type == GUI_INTERNAL_QT4 || guiInfo->type == GUI_INTERNAL_X11)
            {
                set_gui_data(id, 0, (quintptr)gui);
                gui->show();
            }

            set_active(id, true);
            show_gui(id, true);
            app.exec();

            remove_plugin(id);
        }
        else
            qCritical("failed: %s", get_last_error());

        engine_close();
    }
    else
        qCritical("failed to start backend engine, reason:\n%s", get_last_error());

    delete gui;
    return 0;
}

#endif

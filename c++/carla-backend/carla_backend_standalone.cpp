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
static CarlaBackend::CarlaEngine* carlaEngine = nullptr;
static CarlaBackend::CallbackFunc carlaFunc = nullptr;

// -------------------------------------------------------------------------------------------------------------------

unsigned int get_engine_driver_count()
{
    qDebug("CarlaBackendStandalone::get_engine_driver_count()");

    unsigned int count = 0;
#ifdef CARLA_ENGINE_JACK
    count += 1;
#endif
#ifdef CARLA_ENGINE_RTAUDIO
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    count += apis.size();
#endif
    return count;
}

const char* get_engine_driver_name(unsigned int index)
{
    qDebug("CarlaBackendStandalone::get_engine_driver_name(%i)", index);

#ifdef CARLA_ENGINE_JACK
    if (index == 0)
        return "JACK";
    index -= 1;
#endif

#ifdef CARLA_ENGINE_RTAUDIO
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    if (index < apis.size())
    {
        RtAudio::Api api = apis[index];

        switch (api)
        {
        case RtAudio::UNSPECIFIED:
            return "Unspecified";
        case RtAudio::LINUX_ALSA:
            return "ALSA";
        case RtAudio::LINUX_PULSE:
            return "PulseAudio";
        case RtAudio::LINUX_OSS:
            return "OSS";
        case RtAudio::UNIX_JACK:
            return "JACK (RtAudio)";
        case RtAudio::MACOSX_CORE:
            return "CoreAudio";
        case RtAudio::WINDOWS_ASIO:
            return "ASIO";
        case RtAudio::WINDOWS_DS:
            return "DirectSound";
        case RtAudio::RTAUDIO_DUMMY:
            return "Dummy";
        }
    }
#endif

    qDebug("CarlaBackendStandalone::get_engine_driver_name(%i) - invalid index", index);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

bool engine_init(const char* driver_name, const char* client_name)
{
    qDebug("CarlaBackendStandalone::engine_init(\"%s\", \"%s\")", driver_name, client_name);

#ifdef CARLA_ENGINE_JACK
    if (strcmp(driver_name, "JACK") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineJack;
#else
    if (false)
        pass();
#endif

#ifdef CARLA_ENGINE_RTAUDIO
#ifdef __LINUX_ALSA__
    else if (strcmp(driver_name, "ALSA") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::LINUX_ALSA);
#endif
#ifdef __LINUX_PULSE__
    else if (strcmp(driver_name, "PulseAudio") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::LINUX_PULSE);
#endif
#ifdef __LINUX_OSS__
    else if (strcmp(driver_name, "OSS") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::LINUX_OSS);
#endif
#ifdef __UNIX_JACK__
    else if (strcmp(driver_name, "JACK (RtAudio)") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::UNIX_JACK);
#endif
#ifdef __MACOSX_CORE__
    else if (strcmp(driver_name, "CoreAudio") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::MACOSX_CORE);
#endif
#ifdef __WINDOWS_ASIO__
    else if (strcmp(driver_name, "ASIO") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::WINDOWS_ASIO);
#endif
#ifdef __WINDOWS_DS__
    else if (strcmp(driver_name, "DirectSound") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::WINDOWS_DS);
#endif
#ifdef __RTAUDIO_DUMMY__
    else if (strcmp(driver_name, "Dummy") == 0)
        carlaEngine = new CarlaBackend::CarlaEngineRtAudio(RtAudio::RTAUDIO_DUMMY);
#endif
#endif
    else
    {
        CarlaBackend::setLastError("The seleted audio driver is not available!");
        return false;
    }

#ifndef Q_OS_WIN
    // TODO: make this an option, put somewhere else
    if (! getenv("WINE_RT"))
    {
        carla_setenv("WINE_RT", "15");
        carla_setenv("WINE_SVR_RT", "10");
    }
#endif

    carlaEngine->setCallback(carlaFunc, nullptr);

    bool started = carlaEngine->init(client_name);

    if (started)
        CarlaBackend::setLastError("no error");

    return started;
}

bool engine_close()
{
    qDebug("CarlaBackendStandalone::engine_close()");

    if (! carlaEngine)
    {
        CarlaBackend::setLastError("Engine is not started");
        return false;
    }

    bool closed = carlaEngine->close();
    carlaEngine->removeAllPlugins();

    // cleanup static data
    get_plugin_info(0);
    get_parameter_info(0, 0);
    get_parameter_scalepoint_info(0, 0, 0);
    get_chunk_data(0);
    //get_parameter_text(0, 0);
    get_program_name(0, 0);
    get_midi_program_name(0, 0);
    get_real_plugin_name(0);

    CarlaBackend::resetOptions();
    CarlaBackend::setLastError(nullptr);

    delete carlaEngine;
    carlaEngine = nullptr;

    return closed;
}

bool is_engine_running()
{
    qDebug("CarlaBackendStandalone::is_engine_running()");

    return carlaEngine && carlaEngine->isRunning();
}

// -------------------------------------------------------------------------------------------------------------------

short add_plugin(CarlaBackend::BinaryType btype, CarlaBackend::PluginType ptype, const char* filename, const char* const name, const char* label, void* extra_stuff)
{
    qDebug("CarlaBackendStandalone::add_plugin(%s, %s, \"%s\", \"%s\", \"%s\", %p)", CarlaBackend::BinaryType2str(btype), CarlaBackend::PluginType2str(ptype), filename, name, label, extra_stuff);

    return carlaEngine->addPlugin(btype, ptype, filename, name, label, extra_stuff);
}

bool remove_plugin(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::remove_plugin(%i)", plugin_id);

    return carlaEngine->removePlugin(plugin_id);
}

// -------------------------------------------------------------------------------------------------------------------

const PluginInfo* get_plugin_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_plugin_info(%i)", plugin_id);

    static PluginInfo info;

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

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

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

    qCritical("CarlaBackendStandalone::get_plugin_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_audio_port_count_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_audio_port_count_info(%i)", plugin_id);

    static PortCountInfo info;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        info.ins   = plugin->audioInCount();
        info.outs  = plugin->audioOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_audio_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_midi_port_count_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_midi_port_count_info(%i)", plugin_id);

    static PortCountInfo info;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        info.ins   = plugin->midiInCount();
        info.outs  = plugin->midiOutCount();
        info.total = info.ins + info.outs;
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_midi_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_parameter_count_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_port_count_info(%i)", plugin_id);

    static PortCountInfo info;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        plugin->getParameterCountInfo(&info.ins, &info.outs, &info.total);
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_port_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const ParameterInfo* get_parameter_info(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_info(%i, %i)", plugin_id, parameter_id);

    static ParameterInfo info;

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

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

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
            qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

        return &info;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return &info;
}

const ScalePointInfo* get_parameter_scalepoint_info(unsigned short plugin_id, quint32 parameter_id, quint32 scalepoint_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i)", plugin_id, parameter_id, scalepoint_id);

    static ScalePointInfo info;

    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

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
                qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - scalepoint_id out of bounds", plugin_id, parameter_id, scalepoint_id);
        }
        else
            qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, parameter_id);

        return &info;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, scalepoint_id);

    return &info;
}

const GuiInfo* get_gui_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_gui_info(%i)", plugin_id);

    static GuiInfo info;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        plugin->getGuiInfo(&info.type, &info.resizable);
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_gui_info(%i) - could not find plugin", plugin_id);
    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

const CarlaBackend::ParameterData* get_parameter_data(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_data(%i, %i)", plugin_id, parameter_id);

    static CarlaBackend::ParameterData data;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterData(parameter_id);

        qCritical("CarlaBackendStandalone::get_parameter_data(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_parameter_data(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &data;
}

const CarlaBackend::ParameterRanges* get_parameter_ranges(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_ranges(%i, %i)", plugin_id, parameter_id);

    static CarlaBackend::ParameterRanges ranges;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id);

        qCritical("CarlaBackendStandalone::get_parameter_ranges(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return &ranges;
    }

    qCritical("CarlaBackendStandalone::get_parameter_ranges(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &ranges;
}

const CarlaBackend::midi_program_t* get_midi_program_data(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_data(%i, %i)", plugin_id, midi_program_id);

    static CarlaBackend::midi_program_t data;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            return plugin->midiProgramData(midi_program_id);

        qCritical("CarlaBackendStandalone::get_midi_program_data(%i, %i) - midi_program_id out of bounds", plugin_id, midi_program_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_midi_program_data(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return &data;
}

const CarlaBackend::CustomData* get_custom_data(unsigned short plugin_id, quint32 custom_data_id)
{
    qDebug("CarlaBackendStandalone::get_custom_data(%i, %i)", plugin_id, custom_data_id);

    static CarlaBackend::CustomData data;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (custom_data_id < plugin->customDataCount())
            return plugin->customData(custom_data_id);

        qCritical("CarlaBackendStandalone::get_custom_data(%i, %i) - custom_data_id out of bounds", plugin_id, custom_data_id);
        return &data;
    }

    qCritical("CarlaBackendStandalone::get_custom_data(%i, %i) - could not find plugin", plugin_id, custom_data_id);
    return &data;
}

const char* get_chunk_data(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_chunk_data(%i)", plugin_id);

    static const char* chunk_data = nullptr;

    if (chunk_data)
    {
        free((void*)chunk_data);
        chunk_data = nullptr;
    }

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

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
                qCritical("CarlaBackendStandalone::get_chunk_data(%i) - got invalid chunk data", plugin_id);
        }
        else
            qCritical("CarlaBackendStandalone::get_chunk_data(%i) - plugin does not support chunks", plugin_id);

        return chunk_data;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_chunk_data(%i) - could not find plugin", plugin_id);

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_parameter_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->parameterCount();

    qCritical("CarlaBackendStandalone::get_parameter_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_program_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_program_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->programCount();

    qCritical("CarlaBackendStandalone::get_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_midi_program_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->midiProgramCount();

    qCritical("CarlaBackendStandalone::get_midi_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_custom_data_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_custom_data_count(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->customDataCount();

    qCritical("CarlaBackendStandalone::get_custom_data_count(%i) - could not find plugin", plugin_id);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_parameter_text(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_text(%i, %i)", plugin_id, parameter_id);

    static char buf_text[STR_MAX] = { 0 };
    memset(buf_text, 0, sizeof(char)*STR_MAX);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            plugin->getParameterText(parameter_id, buf_text);
            return buf_text;
        }

        qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return nullptr;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - could not find plugin", plugin_id, parameter_id);

    return nullptr;
}

const char* get_program_name(unsigned short plugin_id, quint32 program_id)
{
    qDebug("CarlaBackendStandalone::get_program_name(%i, %i)", plugin_id, program_id);

    static const char* program_name = nullptr;

    if (program_name)
    {
        free((void*)program_name);
        program_name = nullptr;
    }

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (program_id < plugin->programCount())
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->getProgramName(program_id, buf_str);
            program_name = strdup(buf_str);

            return program_name;
        }

        qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - program_id out of bounds", plugin_id, program_id);
        return nullptr;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - could not find plugin", plugin_id, program_id);

    return nullptr;
}

const char* get_midi_program_name(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_name(%i, %i)", plugin_id, midi_program_id);

    static const char* midi_program_name = nullptr;

    if (midi_program_name)
        free((void*)midi_program_name);

    midi_program_name = nullptr;

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
        {
            char buf_str[STR_MAX] = { 0 };

            plugin->getMidiProgramName(midi_program_id, buf_str);
            midi_program_name = strdup(buf_str);

            return midi_program_name;
        }

        qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);
        return nullptr;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - could not find plugin", plugin_id, midi_program_id);

    return nullptr;
}

const char* get_real_plugin_name(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_real_plugin_name(%i)", plugin_id);

    static const char* real_plugin_name = nullptr;

    if (real_plugin_name)
    {
        free((void*)real_plugin_name);
        real_plugin_name = nullptr;
    }

    if (! carlaEngine->isRunning())
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        char buf_str[STR_MAX] = { 0 };

        plugin->getRealName(buf_str);
        real_plugin_name = strdup(buf_str);

        return real_plugin_name;
    }

    if (carlaEngine->isRunning())
        qCritical("CarlaBackendStandalone::get_real_plugin_name(%i) - could not find plugin", plugin_id);

    return real_plugin_name;
}

// -------------------------------------------------------------------------------------------------------------------

qint32 get_current_program_index(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_current_program_index(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->currentProgram();

    qCritical("CarlaBackendStandalone::get_current_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

qint32 get_current_midi_program_index(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_current_midi_program_index(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->currentMidiProgram();

    qCritical("CarlaBackendStandalone::get_current_midi_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

double get_default_parameter_value(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_default_parameter_value(%i, %i)", plugin_id, parameter_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->parameterRanges(parameter_id)->def;

        qCritical("CarlaBackendStandalone::get_default_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return 0.0;
    }

    qCritical("CarlaBackendStandalone::get_default_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

double get_current_parameter_value(unsigned short plugin_id, quint32 parameter_id)
{
    qDebug("CarlaBackendStandalone::get_current_parameter_value(%i, %i)", plugin_id, parameter_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->getParameterValue(parameter_id);

        qCritical("CarlaBackendStandalone::get_current_parameter_value(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return 0.0;
    }

    qCritical("CarlaBackendStandalone::get_current_parameter_value(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

double get_input_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (plugin_id < CarlaBackend::MAX_PLUGINS && (port_id == 1 || port_id == 2))
        return carlaEngine->getInputPeak(plugin_id, port_id-1);

    qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid plugin or port value", plugin_id, port_id);
    return 0.0;
}

double get_output_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    if (plugin_id < CarlaBackend::MAX_PLUGINS && (port_id == 1 || port_id == 2))
        return carlaEngine->getOutputPeak(plugin_id, port_id-1);

    qCritical("CarlaBackendStandalone::get_output_peak_value(%i, %i) - invalid plugin or port value", plugin_id, port_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

void set_active(unsigned short plugin_id, bool onoff)
{
    qDebug("CarlaBackendStandalone::set_active(%i, %s)", plugin_id, bool2str(onoff));

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setActive(onoff, true, false);

    qCritical("CarlaBackendStandalone::set_active(%i, %s) - could not find plugin", plugin_id, bool2str(onoff));
}

void set_drywet(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_drywet(%i, %g)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setDryWet(value, true, false);

    qCritical("CarlaBackendStandalone::set_drywet(%i, %g) - could not find plugin", plugin_id, value);
}

void set_volume(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_volume(%i, %g)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setVolume(value, true, false);

    qCritical("CarlaBackendStandalone::set_volume(%i, %g) - could not find plugin", plugin_id, value);
}

void set_balance_left(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_left(%i, %g)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setBalanceLeft(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_left(%i, %g) - could not find plugin", plugin_id, value);
}

void set_balance_right(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_right(%i, %g)", plugin_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setBalanceRight(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_right(%i, %g) - could not find plugin", plugin_id, value);
}

// -------------------------------------------------------------------------------------------------------------------

void set_parameter_value(unsigned short plugin_id, quint32 parameter_id, double value)
{
    qDebug("CarlaBackendStandalone::set_parameter_value(%i, %i, %g)", plugin_id, parameter_id, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterValue(parameter_id, value, true, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_value(%i, %i, %g) - parameter_id out of bounds", plugin_id, parameter_id, value);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_value(%i, %i, %g) - could not find plugin", plugin_id, parameter_id, value);
}

void set_parameter_midi_channel(unsigned short plugin_id, quint32 parameter_id, quint8 channel)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i)", plugin_id, parameter_id, channel);

    if (channel > 15)
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - invalid channel number", plugin_id, parameter_id, channel);
        return;
    }

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiChannel(parameter_id, channel, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, channel);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, channel);
}

void set_parameter_midi_cc(unsigned short plugin_id, quint32 parameter_id, int16_t cc)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i)", plugin_id, parameter_id, cc);

    if (cc < -1)
    {
        cc = -1;
    }
    else if (cc > 0x5F) // 95
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - invalid cc number", plugin_id, parameter_id, cc);
        return;
    }

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
            return plugin->setParameterMidiCC(parameter_id, cc, true, false);

        qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, cc);
        return;
    }

    qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, cc);
}

void set_program(unsigned short plugin_id, quint32 program_id)
{
    qDebug("CarlaBackendStandalone::set_program(%i, %i)", plugin_id, program_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (program_id < plugin->programCount())
            return plugin->setProgram(program_id, true, true, false, true);

        qCritical("CarlaBackendStandalone::set_program(%i, %i) - program_id out of bounds", plugin_id, program_id);
        return;
    }

    qCritical("CarlaBackendStandalone::set_program(%i, %i) - could not find plugin", plugin_id, program_id);
}

void set_midi_program(unsigned short plugin_id, quint32 midi_program_id)
{
    qDebug("CarlaBackendStandalone::set_midi_program(%i, %i)", plugin_id, midi_program_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
            return plugin->setMidiProgram(midi_program_id, true, true, false, true);

        qCritical("CarlaBackendStandalone::set_midi_program(%i, %i) - midi_program_id out of bounds", plugin_id, midi_program_id);
        return;
    }

    qCritical("CarlaBackendStandalone::set_midi_program(%i, %i) - could not find plugin", plugin_id, midi_program_id);
}

// -------------------------------------------------------------------------------------------------------------------

void set_custom_data(unsigned short plugin_id, CarlaBackend::CustomDataType type, const char* key, const char* value)
{
    qDebug("CarlaBackendStandalone::set_custom_data(%i, %i, \"%s\", \"%s\")", plugin_id, type, key, value);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setCustomData(type, key, value, true);

    qCritical("CarlaBackendStandalone::set_custom_data(%i, %i, \"%s\", \"%s\") - could not find plugin", plugin_id, type, key, value);
}

void set_chunk_data(unsigned short plugin_id, const char* chunk_data)
{
    qDebug("CarlaBackendStandalone::set_chunk_data(%i, \"%s\")", plugin_id, chunk_data);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
            return plugin->setChunkData(chunk_data);

        qCritical("CarlaBackendStandalone::set_chunk_data(%i, \"%s\") - plugin does not support chunks", plugin_id, chunk_data);
        return;
    }

    qCritical("CarlaBackendStandalone::set_chunk_data(%i, \"%s\") - could not find plugin", plugin_id, chunk_data);
}

void set_gui_data(unsigned short plugin_id, int data, quintptr gui_addr)
{
    qDebug("CarlaBackendStandalone::set_gui_data(%i, %i, " P_UINTPTR ")", plugin_id, data, gui_addr);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
#ifdef __WINE__
        plugin->setGuiData(data, (HWND)gui_addr);
#else
        plugin->setGuiData(data, (QDialog*)CarlaBackend::getPointer(gui_addr));
#endif
        return;
    }

    qCritical("CarlaBackendStandalone::set_gui_data(%i, %i, " P_UINTPTR ") - could not find plugin", plugin_id, data, gui_addr);
}

// -------------------------------------------------------------------------------------------------------------------

void show_gui(unsigned short plugin_id, bool yesno)
{
    qDebug("CarlaBackendStandalone::show_gui(%i, %s)", plugin_id, bool2str(yesno));

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->showGui(yesno);

    qCritical("CarlaBackendStandalone::show_gui(%i, %s) - could not find plugin", plugin_id, bool2str(yesno));
}

void idle_guis()
{
    carlaEngine->idlePluginGuis();
}

// -------------------------------------------------------------------------------------------------------------------

void send_midi_note(unsigned short plugin_id, quint8 channel, quint8 note, quint8 velocity)
{
    qDebug("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i)", plugin_id, channel, note, velocity);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    qCritical("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i) - could not find plugin", plugin_id, channel, note, velocity);
}

void prepare_for_save(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::prepare_for_save(%i)", plugin_id);

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->prepareForSave();

    qCritical("CarlaBackendStandalone::prepare_for_save(%i) - could not find plugin", plugin_id);
}

// -------------------------------------------------------------------------------------------------------------------

quint32 get_buffer_size()
{
    qDebug("CarlaBackendStandalone::get_buffer_size()");

    return carlaEngine->getBufferSize();
}

double get_sample_rate()
{
    qDebug("CarlaBackendStandalone::get_sample_rate()");

    return carlaEngine->getSampleRate();
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_last_error()
{
    return CarlaBackend::getLastError();
}

const char* get_host_osc_url()
{
    qDebug("CarlaBackendStandalone::get_host_osc_url()");

    return carlaEngine->getOscServerPath();
}

// -------------------------------------------------------------------------------------------------------------------

void set_callback_function(CarlaBackend::CallbackFunc func)
{
    qDebug("CarlaBackendStandalone::set_callback_function(%p)", func);

    carlaFunc = func;

    if (carlaEngine)
        carlaEngine->setCallback(func, nullptr);
}

void set_option(CarlaBackend::OptionsType option, int value, const char* valueStr)
{
    qDebug("CarlaBackendStandalone::set_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2str(option), value, valueStr);

    CarlaBackend::setOption(option, value, valueStr);
}

// -------------------------------------------------------------------------------------------------------------------

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

class CarlaNSM
{
public:
    CarlaNSM()
    {
        m_controlAddr = nullptr;
        m_serverThread = nullptr;
        m_isOpened = false;
        m_isSaved  = false;
    }

    ~CarlaNSM()
    {
        if (m_controlAddr)
            lo_address_free(m_controlAddr);

        if (m_serverThread)
        {
            lo_server_thread_stop(m_serverThread);
            lo_server_thread_del_method(m_serverThread, nullptr, nullptr);
            lo_server_thread_free(m_serverThread);
        }
    }

    void announce(const char* const url, const int pid)
    {
        lo_address addr = lo_address_new_from_url(url);
        int proto = lo_address_get_protocol(addr);

        if (! m_serverThread)
        {
            // create new OSC thread
            m_serverThread = lo_server_thread_new_with_proto(nullptr, proto, error_handler);

            // register message handlers and start OSC thread
            lo_server_thread_add_method(m_serverThread, "/reply", "ssss", _announce_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/open", "sss", _nsm_open_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/save", "", _nsm_save_handler, this);
            lo_server_thread_start(m_serverThread);
        }

        lo_send_from(addr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     "Carla", ":switch:", "carla-git", NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

        lo_address_free(addr);
    }

    void replyOpen()
    {
        m_isOpened = true;
    }

    void replySave()
    {
        m_isSaved = true;
    }

protected:
    int announce_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::announce_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        m_controlAddr = lo_address_new_from_url(lo_address_get_url(lo_message_get_source(msg)));

        const char* const method = &argv[0]->s;

        if (strcmp(method, "/nsm/server/announce") == 0 && carlaFunc)
            carlaFunc(nullptr, CarlaBackend::CALLBACK_NSM_ANNOUNCE, 0, 0, 0, 0.0);

        return 0;
    }

    int nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::nsm_open_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        if (! carlaFunc)
            return 1;

        const char* const projectPath = &argv[0]->s;
        const char* const clientId    = &argv[2]->s;

        CarlaBackend::setLastError(clientId);
        carlaFunc(nullptr, CarlaBackend::CALLBACK_NSM_OPEN1, 0, 0, 0, 0.0);

        CarlaBackend::setLastError(projectPath);
        carlaFunc(nullptr, CarlaBackend::CALLBACK_NSM_OPEN2, 0, 0, 0, 0.0);

        for (int i=0; i < 30 && ! m_isOpened; i++)
            carla_msleep(100);

        if (m_controlAddr)
            lo_send_from(m_controlAddr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/open", "OK");

        return 0;
    }

    int nsm_save_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::nsm_save_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        if (! carlaFunc)
            return 1;

        carlaFunc(nullptr, CarlaBackend::CALLBACK_NSM_SAVE, 0, 0, 0, 0.0);

        for (int i=0; i < 30 && ! m_isSaved; i++)
            carla_msleep(100);

        if (m_controlAddr)
            lo_send_from(m_controlAddr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/reply", "ss", "/nsm/client/save", "OK");

        return 0;
    }

private:
    lo_address m_controlAddr;
    lo_server_thread m_serverThread;
    bool m_isOpened, m_isSaved;

    static int _announce_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->announce_handler(path, types, argv, argc, msg);
    }

    static int _nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_open_handler(path, types, argv, argc, msg);
    }

    static int _nsm_save_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_save_handler(path, types, argv, argc, msg);
    }

    static void error_handler(const int num, const char* const msg, const char* const path)
    {
        qCritical("CarlaNSM::error_handler(%i, %s, %s)", num, msg, path);
    }
};

static CarlaNSM carlaNSM;

void nsm_announce(const char* url, int pid)
{
    carlaNSM.announce(url, pid);
}

void nsm_reply_open()
{
    carlaNSM.replyOpen();
}

void nsm_reply_save()
{
    carlaNSM.replySave();
}

// -------------------------------------------------------------------------------------------------------------------

#ifdef QTCREATOR_TEST

#include <QtGui/QApplication>
#include <QtGui/QDialog>

QDialog* vstGui = nullptr;

void main_callback(void* ptr, CarlaBackend::CallbackType action, unsigned short pluginId, int value1, int value2, double value3)
{
    switch (action)
    {
    case CarlaBackend::CALLBACK_SHOW_GUI:
        if (vstGui && ! value1)
            vstGui->close();
        break;
    case CarlaBackend::CALLBACK_RESIZE_GUI:
        vstGui->setFixedSize(value1, value2);
        break;
    default:
        break;
    }

    Q_UNUSED(ptr);
    Q_UNUSED(pluginId);
    Q_UNUSED(value3);
}

void run_tests_standalone(short idMax)
{
    for (short id = 0; id <= idMax; id++)
    {
        qDebug("------------------- TEST @%i: non-parameter calls --------------------", id);
        get_plugin_info(id);
        get_audio_port_count_info(id);
        get_midi_port_count_info(id);
        get_parameter_count_info(id);
        get_gui_info(id);
        get_chunk_data(id);
        get_parameter_count(id);
        get_program_count(id);
        get_midi_program_count(id);
        get_custom_data_count(id);
        get_real_plugin_name(id);
        get_current_program_index(id);
        get_current_midi_program_index(id);

        qDebug("------------------- TEST @%i: parameter calls [-1] --------------------", id);
        get_parameter_info(id, -1);
        get_parameter_scalepoint_info(id, -1, -1);
        get_parameter_data(id, -1);
        get_parameter_ranges(id, -1);
        get_midi_program_data(id, -1);
        get_custom_data(id, -1);
        get_parameter_text(id, -1);
        get_program_name(id, -1);
        get_midi_program_name(id, -1);
        get_default_parameter_value(id, -1);
        get_current_parameter_value(id, -1);
        get_input_peak_value(id, -1);
        get_output_peak_value(id, -1);

        qDebug("------------------- TEST @%i: parameter calls [0] --------------------", id);
        get_parameter_info(id, 0);
        get_parameter_scalepoint_info(id, 0, -1);
        get_parameter_scalepoint_info(id, 0, 0);
        get_parameter_data(id, 0);
        get_parameter_ranges(id, 0);
        get_midi_program_data(id, 0);
        get_custom_data(id, 0);
        get_parameter_text(id, 0);
        get_program_name(id, 0);
        get_midi_program_name(id, 0);
        get_default_parameter_value(id, 0);
        get_current_parameter_value(id, 0);
        get_input_peak_value(id, 0);
        get_input_peak_value(id, 1);
        get_input_peak_value(id, 2);
        get_output_peak_value(id, 0);
        get_output_peak_value(id, 1);
        get_output_peak_value(id, 2);

        qDebug("------------------- TEST @%i: set internal data --------------------", id);
        set_active(id, false);
        set_active(id, true);
        set_active(id, true);

        set_drywet(id, -999);
        set_volume(id, -999);
        set_balance_left(id, -999);
        set_balance_right(id, 999);

        qDebug("------------------- TEST @%i: set parameter data [-1] --------------------", id);
        set_parameter_value(id, -1, -999);
        set_parameter_midi_channel(id, -1, -1);
        set_parameter_midi_cc(id, -1, -1);
        set_program(id, -1);
        set_midi_program(id, -1);

        qDebug("------------------- TEST @%i: set parameter data [0] --------------------", id);
        set_parameter_value(id, 0, -999);
        set_parameter_midi_channel(id, 0, -1);
        set_parameter_midi_channel(id, 0, 0);
        set_parameter_midi_cc(id, 0, -1);
        set_parameter_midi_cc(id, 0, 0);
        set_program(id, 0);
        set_midi_program(id, 0);

        qDebug("------------------- TEST @%i: set extra data --------------------", id);
        //set_custom_data(id, CarlaBackend::CUSTOM_DATA_INVALID, nullptr, nullptr);
        set_custom_data(id, CarlaBackend::CUSTOM_DATA_INVALID, "", "");
        set_chunk_data(id, nullptr);
        set_gui_data(id, 0, (quintptr)1);

        qDebug("------------------- TEST @%i: gui stuff --------------------", id);
        show_gui(id, false);
        show_gui(id, true);
        show_gui(id, true);

        idle_guis();
        idle_guis();
        idle_guis();

        qDebug("------------------- TEST @%i: other --------------------", id);
        send_midi_note(id, 15,  127,  127);
        send_midi_note(id,  0,  0,  0);

        prepare_for_save(id);
        prepare_for_save(id);
        prepare_for_save(id);
    }
}

int main(int argc, char* argv[])
{
    using namespace CarlaBackend;

    // Qt app
    QApplication app(argc, argv);

    // Qt gui (for vst)
    vstGui = new QDialog(nullptr);

    // set callback and options
    set_callback_function(main_callback);
    set_option(OPTION_PREFER_UI_BRIDGES, 0, nullptr);
    //set_option(OPTION_PROCESS_MODE, PROCESS_MODE_CONTINUOUS_RACK, nullptr);

    // start engine
    if (! engine_init("JACK", "carla_demo"))
    {
        qCritical("failed to start backend engine, reason:\n%s", get_last_error());
        delete vstGui;
        return 1;
    }

    short id_ladspa = add_plugin(BINARY_NATIVE, PLUGIN_LADSPA, "/usr/lib/ladspa/LEET_eqbw2x2.so", "LADSPA plug name, test long name - "
                                 "------- name ------------ name2 ----------- name3 ------------ name4 ------------ name5 ---------- name6", "leet_equalizer_bw2x2", nullptr);

    short id_dssi = add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/fluidsynth-dssi.so", "DSSI pname, short-utf8 _ \xAE", "FluidSynth-DSSI", (void*)"/usr/lib/dssi/fluidsynth-dssi/FluidSynth-DSSI_gtk");

    //short id_lv2 = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    //short id_vst = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    if (id_ladspa < 0 || id_dssi < 0)
    {
        qCritical("failed to start load plugins, reason:\n%s", get_last_error());
        delete vstGui;
        return 1;
    }

    //const GuiInfo* const guiInfo = get_gui_info(id);
    //if (guiInfo->type == CarlaBackend::GUI_INTERNAL_QT4 || guiInfo->type == CarlaBackend::GUI_INTERNAL_X11)
    //{
    //    set_gui_data(id, 0, (quintptr)gui);
    //gui->show();
    //}

    // activate
    set_active(id_ladspa, true);
    set_active(id_dssi, true);

    // start guis
    show_gui(id_dssi, true);
    carla_sleep(1);

    // do tests
    run_tests_standalone(id_dssi+1);

    // lock
    //app.exec();

    remove_plugin(id_ladspa);
    remove_plugin(id_dssi);
    engine_close();

    return 0;
}

#endif

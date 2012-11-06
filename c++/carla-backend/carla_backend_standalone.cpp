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

#include "carla_backend_standalone.hpp"
#include "carla_plugin.hpp"
#include "carla_native.h"

// -------------------------------------------------------------------------------------------------------------------

// Single, standalone engine
static CarlaBackend::CarlaEngine* carlaEngine = nullptr;
static CarlaBackend::CallbackFunc carlaFunc = nullptr;
static const char* extendedLicenseText = nullptr;
static bool carlaEngineStarted = false;

// -------------------------------------------------------------------------------------------------------------------

const char* get_extended_license_text()
{
    qDebug("CarlaBackendStandalone::get_extended_license_text()");

    if (! extendedLicenseText)
    {
        QString text("<p>This current Carla build is using the following features and 3rd-party code:</p>");
        text += "<ul>";

#ifdef WANT_LADSPA
        text += "<li>LADSPA plugin support, http://www.ladspa.org/</li>";
#endif
#ifdef WANT_DSSI
        text += "<li>DSSI plugin support, http://dssi.sourceforge.net/</li>";
#endif
#ifdef WANT_LV2
        text += "<li>LV2 plugin support, http://lv2plug.in/</li>";
#endif
#ifdef WANT_VST
#  ifdef VESTIGE_HEADER
        text += "<li>VST plugin support, using VeSTige header by Javier Serrano Polo</li>";
#  else
        text += "<li>VST plugin support, using official VST SDK 2.4 (trademark of Steinberg Media Technologies GmbH)</li>";
#  endif
#endif
#ifdef WANT_FLUIDSYNTH
        text += "<li>FluidSynth library for SF2 support, http://www.fluidsynth.org/</li>";
#endif
#ifdef WANT_LINUXSAMPLER
        text += "<li>LinuxSampler library for GIG and SFZ support*, http://www.linuxsampler.org/</li>";
#endif
        text += "<li>liblo library for OSC support, http://liblo.sourceforge.net/</li>";
#ifdef WANT_LV2
        text += "<li>serd, sord, sratom and lilv libraries for LV2 discovery, http://drobilla.net/software/lilv/</li>";
#endif
#ifdef CARLA_ENGINE_RTAUDIO
        text += "<li>RtAudio and RtMidi libraries for extra Audio and MIDI support, http://www.music.mcgill.ca/~gary/rtaudio/</li>";
#endif
        text += "</ul>";

#ifdef WANT_LINUXSAMPLER
        text += "<p>(*) Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors.</p>";
#endif

        extendedLicenseText = strdup(text.toUtf8().constData());
    }

    return extendedLicenseText;
}

unsigned int get_engine_driver_count()
{
    qDebug("CarlaBackendStandalone::get_engine_driver_count()");

    return CarlaBackend::CarlaEngine::getDriverCount();
}

const char* get_engine_driver_name(unsigned int index)
{
    qDebug("CarlaBackendStandalone::get_engine_driver_name(%i)", index);

    return CarlaBackend::CarlaEngine::getDriverName(index);
}

// -------------------------------------------------------------------------------------------------------------------

unsigned int get_internal_plugin_count()
{
    qDebug("CarlaBackendStandalone::get_internal_plugin_count()");

    return CarlaBackend::CarlaPlugin::getNativePluginCount();
}

const PluginInfo* get_internal_plugin_info(unsigned int plugin_id)
{
    qDebug("CarlaBackendStandalone::get_internal_plugin_info(%i)", plugin_id);

    static PluginInfo info;

    const PluginDescriptor* const nativePlugin = CarlaBackend::CarlaPlugin::getNativePlugin(plugin_id);

    CARLA_ASSERT(nativePlugin);

    // as internal plugin, this must never fail
    if (! nativePlugin)
        return nullptr;

    info.type      = CarlaBackend::PLUGIN_NONE;
    info.category  = (CarlaBackend::PluginCategory)nativePlugin->category;
    info.hints     = CarlaBackend::getPluginHintsFromNative(nativePlugin->hints);
    info.name      = nativePlugin->name;
    info.label     = nativePlugin->label;
    info.maker     = nativePlugin->maker;
    info.copyright = nativePlugin->copyright;

    return &info;
}

// -------------------------------------------------------------------------------------------------------------------

bool engine_init(const char* driver_name, const char* client_name)
{
    qDebug("CarlaBackendStandalone::engine_init(\"%s\", \"%s\")", driver_name, client_name);
    CARLA_ASSERT(! carlaEngine);

    carlaEngine = CarlaBackend::CarlaEngine::newDriverByName(driver_name);

    if (! carlaEngine)
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

    carlaEngineStarted = carlaEngine->init(client_name);

    if (carlaEngineStarted)
    {
        CarlaBackend::setLastError("no error");
    }
    else if (carlaEngine)
    {
        delete carlaEngine;
        carlaEngine = nullptr;
    }

    return carlaEngineStarted;
}

bool engine_close()
{
    qDebug("CarlaBackendStandalone::engine_close()");
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
    {
        CarlaBackend::setLastError("Engine is not started");
        return false;
    }

    carlaEngine->removeAllPlugins();
    bool closed = carlaEngine->close();

    carlaEngineStarted = false;

    // cleanup static data
    get_plugin_info(0);
    get_parameter_info(0, 0);
    get_parameter_scalepoint_info(0, 0, 0);
    get_chunk_data(0);
    get_program_name(0, 0);
    get_midi_program_name(0, 0);
    get_real_plugin_name(0);

    carlaEngine->resetOptions();
    CarlaBackend::setLastError(nullptr);

    delete carlaEngine;
    carlaEngine = nullptr;

    if (extendedLicenseText)
    {
        free((void*)extendedLicenseText);
        extendedLicenseText = nullptr;
    }

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
    CARLA_ASSERT(carlaEngine);

    if (carlaEngine && carlaEngine->isRunning())
        return carlaEngine->addPlugin(btype, ptype, filename, name, label, extra_stuff);

    CarlaBackend::setLastError("Engine is not started");
    return -1;
}

bool remove_plugin(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::remove_plugin(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (carlaEngine)
        return carlaEngine->removePlugin(plugin_id);

    CarlaBackend::setLastError("Engine is not started");
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

const PluginInfo* get_plugin_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_plugin_info(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    static PluginInfo info;

    // reset
    info.type     = CarlaBackend::PLUGIN_NONE;
    info.category = CarlaBackend::PLUGIN_CATEGORY_NONE;
    info.hints    = 0x0;
    info.binary   = nullptr;
    info.name     = nullptr;
    info.uniqueId = 0;

    // cleanup
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

    if (! carlaEngine)
        return &info;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        char strBufLabel[STR_MAX] = { 0 };
        char strBufMaker[STR_MAX] = { 0 };
        char strBufCopyright[STR_MAX] = { 0 };

        info.type     = plugin->type();
        info.category = plugin->category();
        info.hints    = plugin->hints();
        info.binary   = plugin->filename();
        info.name     = plugin->name();
        info.uniqueId = plugin->uniqueId();

        plugin->getLabel(strBufLabel);
        info.label = strdup(strBufLabel);

        plugin->getMaker(strBufMaker);
        info.maker = strdup(strBufMaker);

        plugin->getCopyright(strBufCopyright);
        info.copyright = strdup(strBufCopyright);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_plugin_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const PortCountInfo* get_audio_port_count_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_audio_port_count_info(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! carlaEngine)
        return &info;

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
    CARLA_ASSERT(carlaEngine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! carlaEngine)
        return &info;

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
    qDebug("CarlaBackendStandalone::get_parameter_count_info(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    static PortCountInfo info;

    // reset
    info.ins   = 0;
    info.outs  = 0;
    info.total = 0;

    if (! carlaEngine)
        return &info;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        plugin->getParameterCountInfo(&info.ins, &info.outs, &info.total);
        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_count_info(%i) - could not find plugin", plugin_id);
    return &info;
}

const ParameterInfo* get_parameter_info(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_info(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    static ParameterInfo info;

    // reset
    info.scalePointCount = 0;

    // cleanup
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

    if (! carlaEngine)
        return &info;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            char strBufName[STR_MAX] = { 0 };
            char strBufSymbol[STR_MAX] = { 0 };
            char strBufUnit[STR_MAX] = { 0 };

            info.scalePointCount = plugin->parameterScalePointCount(parameter_id);

            plugin->getParameterName(parameter_id, strBufName);
            info.name = strdup(strBufName);

            plugin->getParameterSymbol(parameter_id, strBufSymbol);
            info.symbol = strdup(strBufSymbol);

            plugin->getParameterUnit(parameter_id, strBufUnit);
            info.unit = strdup(strBufUnit);
        }
        else
            qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_info(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return &info;
}

const ScalePointInfo* get_parameter_scalepoint_info(unsigned short plugin_id, uint32_t parameter_id, uint32_t scalepoint_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i)", plugin_id, parameter_id, scalepoint_id);
    CARLA_ASSERT(carlaEngine);

    static ScalePointInfo info;

    // reset
    info.value = 0.0;

    // cleanup
    if (info.label)
    {
        free((void*)info.label);
        info.label = nullptr;
    }

    if (! carlaEngine)
        return &info;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            if (scalepoint_id < plugin->parameterScalePointCount(parameter_id))
            {
                char strBufLabel[STR_MAX] = { 0 };

                info.value = plugin->getParameterScalePointValue(parameter_id, scalepoint_id);

                plugin->getParameterScalePointLabel(parameter_id, scalepoint_id, strBufLabel);
                info.label = strdup(strBufLabel);
            }
            else
                qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - scalepoint_id out of bounds", plugin_id, parameter_id, scalepoint_id);
        }
        else
            qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - parameter_id out of bounds", plugin_id, parameter_id, parameter_id);

        return &info;
    }

    qCritical("CarlaBackendStandalone::get_parameter_scalepoint_info(%i, %i, %i) - could not find plugin", plugin_id, parameter_id, scalepoint_id);
    return &info;
}

const GuiInfo* get_gui_info(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_gui_info(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    static GuiInfo info;

    // reset
    info.type      = CarlaBackend::GUI_NONE;
    info.resizable = false;

    if (! carlaEngine)
        return &info;

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

const CarlaBackend::ParameterData* get_parameter_data(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_data(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    static CarlaBackend::ParameterData data;

    if (! carlaEngine)
        return &data;

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

const CarlaBackend::ParameterRanges* get_parameter_ranges(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_ranges(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    static CarlaBackend::ParameterRanges ranges;

    if (! carlaEngine)
        return &ranges;

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

const CarlaBackend::MidiProgramData* get_midi_program_data(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_data(%i, %i)", plugin_id, midi_program_id);
    CARLA_ASSERT(carlaEngine);

    static CarlaBackend::MidiProgramData data;

    if (! carlaEngine)
        return &data;

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

const CarlaBackend::CustomData* get_custom_data(unsigned short plugin_id, uint32_t custom_data_id)
{
    qDebug("CarlaBackendStandalone::get_custom_data(%i, %i)", plugin_id, custom_data_id);
    CARLA_ASSERT(carlaEngine);

    static CarlaBackend::CustomData data;

    if (! carlaEngine)
        return &data;

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
    CARLA_ASSERT(carlaEngine);

    static const char* chunk_data = nullptr;

    // cleanup
    if (chunk_data)
    {
        free((void*)chunk_data);
        chunk_data = nullptr;
    }

    if (! carlaEngine)
        return nullptr;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            const int32_t dataSize = plugin->chunkData(&data);

            if (data && dataSize >= 4)
            {
                const QByteArray chunk((const char*)data, dataSize);
                chunk_data = strdup(chunk.toBase64().constData());
            }
            else
                qCritical("CarlaBackendStandalone::get_chunk_data(%i) - got invalid chunk data", plugin_id);
        }
        else
            qCritical("CarlaBackendStandalone::get_chunk_data(%i) - plugin does not support chunks", plugin_id);

        return chunk_data;
    }

    qCritical("CarlaBackendStandalone::get_chunk_data(%i) - could not find plugin", plugin_id);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_parameter_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_count(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->parameterCount();

    qCritical("CarlaBackendStandalone::get_parameter_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_program_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_program_count(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->programCount();

    qCritical("CarlaBackendStandalone::get_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_midi_program_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_count(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->midiProgramCount();

    qCritical("CarlaBackendStandalone::get_midi_program_count(%i) - could not find plugin", plugin_id);
    return 0;
}

uint32_t get_custom_data_count(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_custom_data_count(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->customDataCount();

    qCritical("CarlaBackendStandalone::get_custom_data_count(%i) - could not find plugin", plugin_id);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

const char* get_parameter_text(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_parameter_text(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    static char textBuf[STR_MAX];
    memset(textBuf, 0, sizeof(char)*STR_MAX);

    if (! carlaEngine)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (parameter_id < plugin->parameterCount())
        {
            plugin->getParameterText(parameter_id, textBuf);
            return textBuf;
        }

        qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - parameter_id out of bounds", plugin_id, parameter_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_parameter_text(%i, %i) - could not find plugin", plugin_id, parameter_id);
    return nullptr;
}

const char* get_program_name(unsigned short plugin_id, uint32_t program_id)
{
    qDebug("CarlaBackendStandalone::get_program_name(%i, %i)", plugin_id, program_id);
    CARLA_ASSERT(carlaEngine);

    static const char* program_name = nullptr;

    // cleanup
    if (program_name)
    {
        free((void*)program_name);
        program_name = nullptr;
    }

    if (! carlaEngine)
        return nullptr;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (program_id < plugin->programCount())
        {
            char strBuf[STR_MAX] = { 0 };

            plugin->getProgramName(program_id, strBuf);
            program_name = strdup(strBuf);

            return program_name;
        }

        qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - program_id out of bounds", plugin_id, program_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_program_name(%i, %i) - could not find plugin", plugin_id, program_id);
    return nullptr;
}

const char* get_midi_program_name(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::get_midi_program_name(%i, %i)", plugin_id, midi_program_id);
    CARLA_ASSERT(carlaEngine);

    static const char* midi_program_name = nullptr;

    // cleanup
    if (midi_program_name)
    {
        free((void*)midi_program_name);
        midi_program_name = nullptr;
    }

    if (! carlaEngine)
        return nullptr;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        if (midi_program_id < plugin->midiProgramCount())
        {
            char strBuf[STR_MAX] = { 0 };

            plugin->getMidiProgramName(midi_program_id, strBuf);
            midi_program_name = strdup(strBuf);

            return midi_program_name;
        }

        qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - program_id out of bounds", plugin_id, midi_program_id);
        return nullptr;
    }

    qCritical("CarlaBackendStandalone::get_midi_program_name(%i, %i) - could not find plugin", plugin_id, midi_program_id);
    return nullptr;
}

const char* get_real_plugin_name(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_real_plugin_name(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    static const char* real_plugin_name = nullptr;

    // cleanup
    if (real_plugin_name)
    {
        free((void*)real_plugin_name);
        real_plugin_name = nullptr;
    }

    if (! carlaEngine)
        return nullptr;

    if (! carlaEngineStarted)
        return nullptr;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
    {
        char strBuf[STR_MAX] = { 0 };

        plugin->getRealName(strBuf);
        real_plugin_name = strdup(strBuf);

        return real_plugin_name;
    }

    qCritical("CarlaBackendStandalone::get_real_plugin_name(%i) - could not find plugin", plugin_id);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int32_t get_current_program_index(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_current_program_index(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return -1;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->currentProgram();

    qCritical("CarlaBackendStandalone::get_current_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

int32_t get_current_midi_program_index(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::get_current_midi_program_index(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return -1;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->currentMidiProgram();

    qCritical("CarlaBackendStandalone::get_current_midi_program_index(%i) - could not find plugin", plugin_id);
    return -1;
}

// -------------------------------------------------------------------------------------------------------------------

double get_default_parameter_value(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_default_parameter_value(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0.0;

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

double get_current_parameter_value(unsigned short plugin_id, uint32_t parameter_id)
{
    qDebug("CarlaBackendStandalone::get_current_parameter_value(%i, %i)", plugin_id, parameter_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0.0;

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
    CARLA_ASSERT(carlaEngine);
    CARLA_ASSERT(plugin_id < CarlaBackend::CarlaEngine::maxPluginNumber());
    CARLA_ASSERT(port_id == 1 || port_id == 2);

    if (! carlaEngine)
        return 0.0;

    if (plugin_id >= CarlaBackend::CarlaEngine::maxPluginNumber())
    {
        qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid plugin value", plugin_id, port_id);
        return 0.0;
    }

    if (port_id == 1 || port_id == 2)
        return carlaEngine->getInputPeak(plugin_id, port_id-1);

    qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid port value", plugin_id, port_id);
    return 0.0;
}

double get_output_peak_value(unsigned short plugin_id, unsigned short port_id)
{
    CARLA_ASSERT(carlaEngine);
    CARLA_ASSERT(plugin_id < CarlaBackend::CarlaEngine::maxPluginNumber());
    CARLA_ASSERT(port_id == 1 || port_id == 2);

    if (! carlaEngine)
        return 0.0;

    if (plugin_id >= CarlaBackend::CarlaEngine::maxPluginNumber())
    {
        qCritical("CarlaBackendStandalone::get_input_peak_value(%i, %i) - invalid plugin value", plugin_id, port_id);
        return 0.0;
    }

    if (port_id == 1 || port_id == 2)
        return carlaEngine->getOutputPeak(plugin_id, port_id-1);

    qCritical("CarlaBackendStandalone::get_output_peak_value(%i, %i) - invalid port value", plugin_id, port_id);
    return 0.0;
}

// -------------------------------------------------------------------------------------------------------------------

void set_active(unsigned short plugin_id, bool onoff)
{
    qDebug("CarlaBackendStandalone::set_active(%i, %s)", plugin_id, bool2str(onoff));
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setActive(onoff, true, false);

    qCritical("CarlaBackendStandalone::set_active(%i, %s) - could not find plugin", plugin_id, bool2str(onoff));
}

void set_drywet(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_drywet(%i, %g)", plugin_id, value);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setDryWet(value, true, false);

    qCritical("CarlaBackendStandalone::set_drywet(%i, %g) - could not find plugin", plugin_id, value);
}

void set_volume(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_volume(%i, %g)", plugin_id, value);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setVolume(value, true, false);

    qCritical("CarlaBackendStandalone::set_volume(%i, %g) - could not find plugin", plugin_id, value);
}

void set_balance_left(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_left(%i, %g)", plugin_id, value);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setBalanceLeft(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_left(%i, %g) - could not find plugin", plugin_id, value);
}

void set_balance_right(unsigned short plugin_id, double value)
{
    qDebug("CarlaBackendStandalone::set_balance_right(%i, %g)", plugin_id, value);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setBalanceRight(value, true, false);

    qCritical("CarlaBackendStandalone::set_balance_right(%i, %g) - could not find plugin", plugin_id, value);
}

// -------------------------------------------------------------------------------------------------------------------

void set_parameter_value(unsigned short plugin_id, uint32_t parameter_id, double value)
{
    qDebug("CarlaBackendStandalone::set_parameter_value(%i, %i, %g)", plugin_id, parameter_id, value);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

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

void set_parameter_midi_channel(unsigned short plugin_id, uint32_t parameter_id, uint8_t channel)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i)", plugin_id, parameter_id, channel);
    CARLA_ASSERT(carlaEngine);
    CARLA_ASSERT(channel < MAX_MIDI_CHANNELS);

    if (channel >= MAX_MIDI_CHANNELS)
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_channel(%i, %i, %i) - invalid channel number", plugin_id, parameter_id, channel);
        return;
    }

    if (! carlaEngine)
        return;

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

void set_parameter_midi_cc(unsigned short plugin_id, uint32_t parameter_id, int16_t cc)
{
    qDebug("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i)", plugin_id, parameter_id, cc);
    CARLA_ASSERT(carlaEngine);
    CARLA_ASSERT(cc >= -1 && cc <= 0x5F);

    if (cc < -1)
    {
        cc = -1;
    }
    else if (cc > 0x5F) // 95
    {
        qCritical("CarlaBackendStandalone::set_parameter_midi_cc(%i, %i, %i) - invalid cc number", plugin_id, parameter_id, cc);
        return;
    }

    if (! carlaEngine)
        return;

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

void set_program(unsigned short plugin_id, uint32_t program_id)
{
    qDebug("CarlaBackendStandalone::set_program(%i, %i)", plugin_id, program_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

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

void set_midi_program(unsigned short plugin_id, uint32_t midi_program_id)
{
    qDebug("CarlaBackendStandalone::set_midi_program(%i, %i)", plugin_id, midi_program_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

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
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setCustomData(type, key, value, true);

    qCritical("CarlaBackendStandalone::set_custom_data(%i, %i, \"%s\", \"%s\") - could not find plugin", plugin_id, type, key, value);
}

void set_chunk_data(unsigned short plugin_id, const char* chunk_data)
{
    qDebug("CarlaBackendStandalone::set_chunk_data(%i, \"%s\")", plugin_id, chunk_data);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

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

void set_gui_container(unsigned short plugin_id, uintptr_t gui_addr)
{
    qDebug("CarlaBackendStandalone::set_gui_container(%i, " P_UINTPTR ")", plugin_id, gui_addr);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->setGuiContainer((GuiContainer*)CarlaBackend::getPointer(gui_addr));

    qCritical("CarlaBackendStandalone::set_gui_container(%i, " P_UINTPTR ") - could not find plugin", plugin_id, gui_addr);
}

// -------------------------------------------------------------------------------------------------------------------

void show_gui(unsigned short plugin_id, bool yesno)
{
    qDebug("CarlaBackendStandalone::show_gui(%i, %s)", plugin_id, bool2str(yesno));
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->showGui(yesno);

    qCritical("CarlaBackendStandalone::show_gui(%i, %s) - could not find plugin", plugin_id, bool2str(yesno));
}

void idle_guis()
{
    CARLA_ASSERT(carlaEngine);

    if (carlaEngine)
        carlaEngine->idlePluginGuis();
}

// -------------------------------------------------------------------------------------------------------------------

void send_midi_note(unsigned short plugin_id, uint8_t channel, uint8_t note, uint8_t velocity)
{
    qDebug("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i)", plugin_id, channel, note, velocity);
    CARLA_ASSERT(carlaEngine);

    if (! (carlaEngine && carlaEngine->isRunning()))
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->sendMidiSingleNote(channel, note, velocity, true, true, false);

    qCritical("CarlaBackendStandalone::send_midi_note(%i, %i, %i, %i) - could not find plugin", plugin_id, channel, note, velocity);
}

void prepare_for_save(unsigned short plugin_id)
{
    qDebug("CarlaBackendStandalone::prepare_for_save(%i)", plugin_id);
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return;

    CarlaBackend::CarlaPlugin* const plugin = carlaEngine->getPlugin(plugin_id);

    if (plugin)
        return plugin->prepareForSave();

    qCritical("CarlaBackendStandalone::prepare_for_save(%i) - could not find plugin", plugin_id);
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t get_buffer_size()
{
    qDebug("CarlaBackendStandalone::get_buffer_size()");
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0;

    return carlaEngine->getBufferSize();
}

double get_sample_rate()
{
    qDebug("CarlaBackendStandalone::get_sample_rate()");
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return 0.0;

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
    CARLA_ASSERT(carlaEngine);

    if (! carlaEngine)
        return nullptr;

    return carlaEngine->getOscServerPathTCP();
}

// -------------------------------------------------------------------------------------------------------------------

void set_callback_function(CarlaBackend::CallbackFunc func)
{
    qDebug("CarlaBackendStandalone::set_callback_function(%p)", func);

    carlaFunc = func;

    if (carlaEngine)
        carlaEngine->setCallback(func, nullptr);
}

void set_option(CarlaBackend::OptionsType option, int value, const char* value_str)
{
    qDebug("CarlaBackendStandalone::set_option(%s, %i, \"%s\")", CarlaBackend::OptionsType2str(option), value, value_str);

    if (carlaEngine)
        carlaEngine->setOption(option, value, value_str);
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
            lo_server_thread_del_method(m_serverThread, "/reply", "ssss");
            lo_server_thread_del_method(m_serverThread, "/nsm/client/open", "sss");
            lo_server_thread_del_method(m_serverThread, "/nsm/client/save", "");
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
            lo_server_thread_add_method(m_serverThread, "/reply", "ssss", _reply_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/open", "sss", _nsm_open_handler, this);
            lo_server_thread_add_method(m_serverThread, "/nsm/client/save", "", _nsm_save_handler, this);
            lo_server_thread_start(m_serverThread);
        }

        lo_send_from(addr, lo_server_thread_get_server(m_serverThread), LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                     "Carla", ":switch:", "carla", NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid);

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
    int reply_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::reply_handler(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

        m_controlAddr = lo_address_new_from_url(lo_address_get_url(lo_message_get_source(msg)));

        const char* const method = &argv[0]->s;

        if (strcmp(method, "/nsm/server/announce") == 0 && carlaFunc)
            carlaFunc(nullptr, CarlaBackend::CALLBACK_NSM_ANNOUNCE, 0, 0, 0, 0.0);

        return 0;
    }

    int nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg)
    {
        qDebug("CarlaNSM::nsm_open_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

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
        qDebug("CarlaNSM::nsm_save_handler(\"%s\", \"%s\", %p, %i, %p)", path, types, argv, argc, msg);

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

    static int _reply_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->reply_handler(path, types, argv, argc, msg);
    }

    static int _nsm_open_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_open_handler(path, types, argv, argc, msg);
    }

    static int _nsm_save_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const data)
    {
        CARLA_ASSERT(data);
        CarlaNSM* const _this_ = (CarlaNSM*)data;
        return _this_->nsm_save_handler(path, types, argv, argc, msg);
    }

    static void error_handler(const int num, const char* const msg, const char* const path)
    {
        qCritical("CarlaNSM::error_handler(%i, \"%s\", \"%s\")", num, msg, path);
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

        qDebug("------------------- TEST @%i: set extra data --------------------", id);
        set_custom_data(id, CarlaBackend::CUSTOM_DATA_STRING, "", "");
        set_chunk_data(id, nullptr);
        set_gui_container(id, (uintptr_t)1);

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

    short id_dssi   = add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/fluidsynth-dssi.so", "DSSI pname, short-utf8 _ \xAE", "FluidSynth-DSSI", (void*)"/usr/lib/dssi/fluidsynth-dssi/FluidSynth-DSSI_gtk");
    short id_native = add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", "ZynHere", "zynaddsubfx", nullptr);

    //short id_lv2 = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    //short id_vst = add_plugin(BINARY_NATIVE, PLUGIN_LV2, "FILENAME", "HAHA name!!!", "http://studionumbersix.com/foo/lv2/yc20", nullptr);

    if (id_ladspa < 0 || id_dssi < 0 || id_native < 0)
    {
        qCritical("failed to start load plugins, reason:\n%s", get_last_error());
        delete vstGui;
        return 1;
    }

    //const GuiInfo* const guiInfo = get_gui_info(id);
    //if (guiInfo->type == CarlaBackend::GUI_INTERNAL_QT4 || guiInfo->type == CarlaBackend::GUI_INTERNAL_X11)
    //{
    //    set_gui_data(id, 0, (uintptr_t)gui);
    //gui->show();
    //}

    // activate
    set_active(id_ladspa, true);
    set_active(id_dssi, true);
    set_active(id_native, true);

    // start guis
    show_gui(id_dssi, true);
    carla_sleep(1);

    // do tests
    run_tests_standalone(id_dssi+1);

    // lock
    app.exec();

    delete vstGui;
    vstGui = nullptr;

    remove_plugin(id_ladspa);
    remove_plugin(id_dssi);
    remove_plugin(id_native);
    engine_close();

    return 0;
}

#endif

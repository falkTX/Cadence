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

#include "carla_osc.h"
#include "carla_plugin.h"

static void osc_error_handler(int num, const char* msg, const char* path)
{
    qCritical("osc_error_handler(%i, %s, %s)", num, msg, path);
}

static int osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
    CarlaOsc* const osc = (CarlaOsc*)user_data;
    return osc->handleMessage(path, argc, argv, types, msg);
    Q_UNUSED(msg);
}

// -------------------------------------------------------------------------------------------------------------------

CarlaOsc::CarlaOsc(CarlaBackend::CarlaEngine* const engine_) :
    engine(engine_)
{
    qDebug("CarlaOsc::CarlaOsc()");
    assert(engine);

    serverData.path = nullptr;
    serverData.source = nullptr;
    serverData.target = nullptr;
    serverPath = nullptr;
    serverThread = nullptr;

    m_name = nullptr;
    m_name_len = 0;
}

CarlaOsc::~CarlaOsc()
{
    qDebug("CarlaOsc::~CarlaOsc()");
}

void CarlaOsc::init(const char* const name)
{
    qDebug("CarlaOsc::init(%s)", name);

    m_name = strdup(name);
    m_name_len = strlen(name);

    // create new OSC thread
    serverThread = lo_server_thread_new(nullptr, osc_error_handler);

    // get our full OSC server path
    char* threadPath = lo_server_thread_get_url(serverThread);
    serverPath = strdup(QString("%1%2").arg(threadPath).arg(name).toUtf8().constData());
    free(threadPath);

    // register message handler and start OSC thread
    lo_server_thread_add_method(serverThread, nullptr, nullptr, osc_message_handler, this);
    lo_server_thread_start(serverThread);
}

void CarlaOsc::close()
{
    qDebug("CarlaOsc::close()");

    osc_clear_data(&serverData);

    lo_server_thread_stop(serverThread);
    lo_server_thread_del_method(serverThread, nullptr, nullptr);
    lo_server_thread_free(serverThread);

    free((void*)serverPath);
    serverPath = nullptr;

    free((void*)m_name);
    m_name = nullptr;
    m_name_len = 0;
}

int CarlaOsc::handleMessage(const char* const path, int argc, lo_arg** const argv, const char* const types, lo_message msg)
{
    qDebug("CarlaOsc::handleMessage(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);

    // Initial path check
    if (strcmp(path, "/register") == 0)
    {
        lo_address source  = lo_message_get_source(msg);
        return handle_register(argv, source);
    }

    if (strcmp(path, "/unregister") == 0)
    {
        return handle_unregister();
    }

    // Check if message is for this client
    if (strlen(path) <= m_name_len || strncmp(path+1, m_name, m_name_len) != 0)
    {
        qWarning("CarlaOsc::handleMessage() - message not for this client -> '%s' != '/%s/'", path, m_name);
        return 1;
    }

    // Get id from message
    int pluginId = 0;

    if (std::isdigit(path[m_name_len+2]))
        pluginId += path[m_name_len+2]-'0';

    if (std::isdigit(path[m_name_len+3]))
        pluginId += (path[m_name_len+3]-'0')*10;

    if (pluginId < 0 || pluginId > CarlaBackend::MAX_PLUGINS)
    {
        qCritical("CarlaOsc::handleMessage() - failed to get pluginId -> %i", pluginId);
        return 1;
    }

    // Get plugin
    CarlaBackend::CarlaPlugin* const plugin = engine->getPluginById(pluginId);

    if (plugin == nullptr || plugin->id() != pluginId)
    {
        qWarning("CarlaOsc::handleMessage() - invalid plugin '%i', probably has been removed", pluginId);
        return 1;
    }

    // Get method from path, "/Carla/i/method"
    int offset = (pluginId >= 10) ? 4 : 3;
    char method[32] = { 0 };
    memcpy(method, path + (m_name_len + offset), 32);

    // Common OSC methods
    if (strcmp(method, "/update") == 0)
    {
        lo_address source = lo_message_get_source(msg);
        return handle_update(plugin, argv, source);
    }
    if (strcmp(method, "/configure") == 0)
        return handle_configure(plugin, argv);
    if (strcmp(method, "/control") == 0)
        return handle_control(plugin, argv);
    if (strcmp(method, "/program") == 0)
        return handle_program(plugin, argv);
    if (strcmp(method, "/midi") == 0)
        return handle_midi(plugin, argv);
    if (strcmp(method, "/exiting") == 0)
        return handle_exiting(plugin);

    // Internal methods
    if (strcmp(method, "/set_active") == 0)
        return handle_set_active(plugin, argv);
    if (strcmp(method, "/set_drywet") == 0)
        return handle_set_drywet(plugin, argv);
    if (strcmp(method, "/set_volume") == 0)
        return handle_set_volume(plugin, argv);
    if (strcmp(method, "/set_balance_left") == 0)
        return handle_set_balance_left(plugin, argv);
    if (strcmp(method, "/set_balance_right") == 0)
        return handle_set_balance_right(plugin, argv);
    if (strcmp(method, "/set_parameter") == 0)
        return handle_set_parameter(plugin, argv);
    if (strcmp(method, "/set_program") == 0)
        return handle_set_program(plugin, argv);
    if (strcmp(method, "/set_midi_program") == 0)
        return handle_set_midi_program(plugin, argv);
    if (strcmp(method, "/note_on") == 0)
        return handle_note_on(plugin, argv);
    if (strcmp(method, "/note_off") == 0)
        return handle_note_off(plugin, argv);

    // Plugin-specific methods
    if (strcmp(method, "/lv2_atom_transfer") == 0)
        return handle_lv2_atom_transfer(plugin, argv);
    if (strcmp(method, "/lv2_event_transfer") == 0)
        return handle_lv2_event_transfer(plugin, argv);

    // Plugin Bridges
    if (plugin->hints() & CarlaBackend::PLUGIN_IS_BRIDGE)
    {
        if (strcmp(method, "/bridge_ains_peak") == 0)
            return handle_bridge_ains_peak(plugin, argv);
        if (strcmp(method, "/bridge_aouts_peak") == 0)
            return handle_bridge_aouts_peak(plugin, argv);
        if (strcmp(method, "/bridge_audio_count") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeAudioCount, argv);
        if (strcmp(method, "/bridge_midi_count") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeMidiCount, argv);
        if (strcmp(method, "/bridge_param_count") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeParameterCount, argv);
        if (strcmp(method, "/bridge_program_count") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeProgramCount, argv);
        if (strcmp(method, "/bridge_midi_program_count") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeMidiProgramCount, argv);
        if (strcmp(method, "/bridge_plugin_info") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgePluginInfo, argv);
        if (strcmp(method, "/bridge_param_info") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeParameterInfo, argv);
        if (strcmp(method, "/bridge_param_data") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeParameterDataInfo, argv);
        if (strcmp(method, "/bridge_param_ranges") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeParameterRangesInfo, argv);
        if (strcmp(method, "/bridge_program_info") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeProgramInfo, argv);
        if (strcmp(method, "/bridge_midi_program_info") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeMidiProgramInfo, argv);
        if (strcmp(method, "/bridge_custom_data") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeCustomData, argv);
        if (strcmp(method, "/bridge_chunk_data") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeChunkData, argv);
        if (strcmp(method, "/bridge_update") == 0)
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeUpdateNow, argv);
    }

    qWarning("CarlaOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_register(lo_arg** argv, lo_address source)
{
    qDebug("CarlaOsc::handle_register()");

    if (! serverData.path)
    {
        const char* const url = (const char*)&argv[0]->s;
        const char* host;
        const char* port;

        qDebug("CarlaOsc::handle_register() - OSC backend registered to %s", url);

        host = lo_address_get_hostname(source);
        port = lo_address_get_port(source);
        serverData.source = lo_address_new(host, port);

        host = lo_url_get_hostname(url);
        port = lo_url_get_port(url);
        serverData.path   = lo_url_get_path(url);
        serverData.target = lo_address_new(host, port);

        free((void*)host);
        free((void*)port);

        for (unsigned short i=0; i < CarlaBackend::MAX_PLUGINS; i++)
        {
            CarlaBackend::CarlaPlugin* const plugin = engine->getPluginByIndex(i);

            if (plugin && plugin->enabled())
                plugin->registerToOsc();
        }

        return 0;
    }

    qWarning("CarlaOsc::handle_register() - OSC backend already registered to %s", serverData.path);
    return 1;
}

int CarlaOsc::handle_unregister()
{
    qDebug("CarlaOsc::handle_unregister()");

    if (serverData.path)
    {
        osc_clear_data(&serverData);
        return 0;
    }

    qWarning("CarlaOsc::handle_unregister() - OSC backend is not registered yet");
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_update(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv, lo_address source)
{
    qDebug("CarlaOsc::handle_update()");

    const char* const url = (const char*)&argv[0]->s;
    plugin->updateOscData(source, url);

    return 0;
}

int CarlaOsc::handle_configure(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_configure()");

    const char* key   = (const char*)&argv[0]->s;
    const char* value = (const char*)&argv[1]->s;

    if (plugin->hints() & CarlaBackend::PLUGIN_IS_BRIDGE)
    {
        if (strcmp(key, CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI) == 0)
        {
            engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
            return 0;
        }

        if (strcmp(key, CarlaBackend::CARLA_BRIDGE_MSG_SAVED) == 0)
        {
            return plugin->setOscBridgeInfo(CarlaBackend::PluginBridgeSaved, nullptr);
        }
    }

    plugin->setCustomData(CarlaBackend::CUSTOM_DATA_STRING, key, value, false);

    return 0;
}

int CarlaOsc::handle_control(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_control()");

    int rindex  = argv[0]->i;
    float value = argv[1]->f;

    plugin->setParameterValueByRIndex(rindex, value, false, true, true);

    return 0;
}

int CarlaOsc::handle_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_program()");

    if (plugin->type() == CarlaBackend::PLUGIN_DSSI)
    {
        uint32_t bank_id    = argv[0]->i;
        uint32_t program_id = argv[1]->i;

        plugin->setMidiProgramById(bank_id, program_id, false, true, true, true);
    }
    else
    {
        uint32_t program_id = argv[0]->i;

        if (program_id < plugin->programCount())
        {
            plugin->setProgram(program_id, false, true, true, true);
            return 0;
        }

        qCritical("osc_handle_program() - program_id '%i' out of bounds", program_id);
    }

    return 1;
}

int CarlaOsc::handle_midi(CarlaBackend::CarlaPlugin* plugin, lo_arg **argv)
{
    qDebug("CarlaOsc::handle_midi()");

    if (plugin->midiInCount() > 0)
    {
        const uint8_t* const data = argv[0]->m;
        uint8_t status  = data[1];
        uint8_t channel = 0; // TODO

        // Fix bad note-off
        if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
            status -= 0x10;

        if (MIDI_IS_STATUS_NOTE_OFF(status))
        {
            uint8_t note = data[2];
            plugin->sendMidiSingleNote(channel, note, 0, false, true, true);
        }
        else if (MIDI_IS_STATUS_NOTE_ON(status))
        {
            uint8_t note = data[2];
            uint8_t velo = data[3];
            plugin->sendMidiSingleNote(channel, note, velo, false, true, true);
        }

        return 0;
    }

    qWarning("osc_handle_midi() - recived midi when plugin has no midi inputs");
    return 1;
}

int CarlaOsc::handle_exiting(CarlaBackend::CarlaPlugin* plugin)
{
    qDebug("CarlaOsc::handle_exiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
    plugin->clearOscData();

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_set_active(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_active()");

    bool value = (bool)argv[0]->i;
    plugin->setActive(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_drywet(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_drywet()");

    double value = argv[0]->f;
    plugin->setDryWet(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_volume(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_volume()");

    double value = argv[0]->f;
    plugin->setVolume(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_balance_left(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_balance_left()");

    double value = argv[0]->f;
    plugin->setBalanceLeft(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_balance_right(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_balance_right()");

    double value = argv[0]->f;
    plugin->setBalanceRight(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_parameter(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_parameter()");

    uint32_t parameter_id = argv[0]->i;
    double value = argv[1]->f;
    plugin->setParameterValue(parameter_id, value, true, false, true);

    return 0;
}

int CarlaOsc::handle_set_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_program()");

    uint32_t index = argv[0]->i;
    plugin->setProgram(index, true, false, true, true);

    return 0;
}

int CarlaOsc::handle_set_midi_program(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_set_midi_program()");

    uint32_t index = argv[0]->i;
    plugin->setMidiProgram(index, true, false, true, true);

    return 0;
}

int CarlaOsc::handle_note_on(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_note_on()");

    int channel = 0; // TODO
    int note = argv[0]->i;
    int velo = argv[1]->i;
    plugin->sendMidiSingleNote(channel, note, velo, true, false, true);

    return 0;
}

int CarlaOsc::handle_note_off(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    qDebug("CarlaOsc::handle_note_off()");

    int channel = 0; // TODO
    int note = argv[0]->i;
    plugin->sendMidiSingleNote(channel, note, 0, true, false, true);

    return 0;
}

int CarlaOsc::handle_bridge_ains_peak(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    int index    = argv[0]->i;
    double value = argv[1]->f;

    engine->setInputPeak(plugin->id(), index-1, value);
    return 0;
}

int CarlaOsc::handle_bridge_aouts_peak(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    int index    = argv[0]->i;
    double value = argv[1]->f;

    engine->setOutputPeak(plugin->id(), index-1, value);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

#if 0
#include <iostream>

// -------------------------------------------------------------------------------------------------------------------

//void osc_send_lv2_atom_transfer(OscData* osc_data, )

void osc_send_lv2_event_transfer(const OscData* const osc_data, const char* type, const char* key, const char* value)
{
    qDebug("osc_send_lv2_event_transfer(%s, %s, %s)", type, key, value);
    if (osc_data->target)
    {
        char target_path[strlen(osc_data->path)+20];
        strcpy(target_path, osc_data->path);
        strcat(target_path, "/lv2_event_transfer");
        lo_send(osc_data->target, target_path, "sss", type, key, value);
    }
}

#endif

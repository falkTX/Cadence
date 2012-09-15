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

#include "carla_osc.h"
#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

void osc_error_handler(const int num, const char* const msg, const char* const path)
{
    qCritical("osc_error_handler(%i, %s, %s)", num, msg, path);
}

CarlaOsc::CarlaOsc(CarlaEngine* const engine_) :
    engine(engine_)
{
    Q_ASSERT(engine);
    qDebug("CarlaOsc::CarlaOsc(%p)", engine_);

    m_serverPath = nullptr;
    m_serverThread = nullptr;
    m_controllerData.path = nullptr;
    m_controllerData.source = nullptr;
    m_controllerData.target = nullptr;

    m_name = nullptr;
    m_name_len = 0;
}

CarlaOsc::~CarlaOsc()
{
    qDebug("CarlaOsc::~CarlaOsc()");
}

void CarlaOsc::init(const char* const name)
{
    Q_ASSERT(name);
    Q_ASSERT(m_name_len == 0);
    qDebug("CarlaOsc::init(\"%s\")", name);

    m_name = strdup(name);
    m_name_len = strlen(name);

    // create new OSC thread
    m_serverThread = lo_server_thread_new(nullptr, osc_error_handler);

    // get our full OSC server path
    char* const threadPath = lo_server_thread_get_url(m_serverThread);
    m_serverPath = strdup(QString("%1%2").arg(threadPath).arg(name).toUtf8().constData());
    free(threadPath);

    // register message handler and start OSC thread
    lo_server_thread_add_method(m_serverThread, nullptr, nullptr, osc_message_handler, this);
    lo_server_thread_start(m_serverThread);
}

void CarlaOsc::close()
{
    Q_ASSERT(m_name);
    qDebug("CarlaOsc::close()");

    osc_clear_data(&m_controllerData);

    lo_server_thread_stop(m_serverThread);
    lo_server_thread_del_method(m_serverThread, nullptr, nullptr);
    lo_server_thread_free(m_serverThread);

    free((void*)m_serverPath);
    m_serverPath = nullptr;

    free((void*)m_name);
    m_name = nullptr;
    m_name_len = 0;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
#if DEBUG
    if (! QString(path).contains("put_peak_value"))
        qDebug("CarlaOsc::handleMessage(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);
#endif

    Q_ASSERT(m_serverThread);
    Q_ASSERT(path);

    // Initial path check
    if (strcmp(path, "/register") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handle_register(argc, argv, types, source);
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

    // Get plugin id from message
    int pluginId = 0;

    if (std::isdigit(path[m_name_len+2]))
        pluginId += path[m_name_len+2]-'0';

    if (std::isdigit(path[m_name_len+3]))
        pluginId += (path[m_name_len+3]-'0')*10;

    if (pluginId < 0 || pluginId > CarlaEngine::maxPluginNumber())
    {
        qCritical("CarlaOsc::handleMessage() - failed to get plugin, wrong id -> %i", pluginId);
        return 1;
    }

    // Get plugin
    CarlaPlugin* const plugin = engine->getPluginUnchecked(pluginId);

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
        const lo_address source = lo_message_get_source(msg);
        return handle_update(plugin, argc, argv, types, source);
    }
    if (strcmp(method, "/configure") == 0)
        return handle_configure(plugin, argc, argv, types);
    if (strcmp(method, "/control") == 0)
        return handle_control(plugin, argc, argv, types);
    if (strcmp(method, "/program") == 0)
        return handle_program(plugin, argc, argv, types);
    if (strcmp(method, "/midi") == 0)
        return handle_midi(plugin, argc, argv, types);
    if (strcmp(method, "/exiting") == 0)
        return handle_exiting(plugin);

    // Internal methods
    if (strcmp(method, "/set_active") == 0)
        return handle_set_active(plugin, argc, argv, types);
    if (strcmp(method, "/set_drywet") == 0)
        return handle_set_drywet(plugin, argc, argv, types);
    if (strcmp(method, "/set_volume") == 0)
        return handle_set_volume(plugin, argc, argv, types);
    if (strcmp(method, "/set_balance_left") == 0)
        return handle_set_balance_left(plugin, argc, argv, types);
    if (strcmp(method, "/set_balance_right") == 0)
        return handle_set_balance_right(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_value") == 0)
        return handle_set_parameter_value(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_midi_cc") == 0)
        return handle_set_parameter_midi_cc(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_midi_channel") == 0)
        return handle_set_parameter_midi_channel(plugin, argc, argv, types);
    if (strcmp(method, "/set_program") == 0)
        return handle_set_program(plugin, argc, argv, types);
    if (strcmp(method, "/set_midi_program") == 0)
        return handle_set_midi_program(plugin, argc, argv, types);
    if (strcmp(method, "/note_on") == 0)
        return handle_note_on(plugin, argc, argv, types);
    if (strcmp(method, "/note_off") == 0)
        return handle_note_off(plugin, argc, argv, types);

    // Plugin-specific methods
    if (strcmp(method, "/lv2_atom_transfer") == 0)
        return handle_lv2_atom_transfer(plugin, argc, argv, types);
    if (strcmp(method, "/lv2_event_transfer") == 0)
        return handle_lv2_event_transfer(plugin, argc, argv, types);

    // Plugin Bridges
    if (plugin->hints() & PLUGIN_IS_BRIDGE)
    {
        if (strcmp(method, "/bridge_set_input_peak_value") == 0)
            return handle_bridge_set_input_peak_value(plugin, argc, argv, types);
        if (strcmp(method, "/bridge_set_output_peak_value") == 0)
            return handle_bridge_set_output_peak_value(plugin, argc, argv, types);
        if (strcmp(method, "/bridge_audio_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeAudioCount, argc, argv, types);
        if (strcmp(method, "/bridge_midi_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiCount, argc, argv, types);
        if (strcmp(method, "/bridge_parameter_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterCount, argc, argv, types);
        if (strcmp(method, "/bridge_program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramCount, argc, argv, types);
        if (strcmp(method, "/bridge_midi_program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramCount, argc, argv, types);
        if (strcmp(method, "/bridge_plugin_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgePluginInfo, argc, argv, types);
        if (strcmp(method, "/bridge_parameter_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterInfo, argc, argv, types);
        if (strcmp(method, "/bridge_parameter_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterData, argc, argv, types);
        if (strcmp(method, "/bridge_parameter_ranges") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterRanges, argc, argv, types);
        if (strcmp(method, "/bridge_program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramInfo, argc, argv, types);
        if (strcmp(method, "/bridge_midi_program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramInfo, argc, argv, types);
        if (strcmp(method, "/bridge_configure") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeConfigure, argc, argv, types);
        if (strcmp(method, "/bridge_set_parameter_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetParameterValue, argc, argv, types);
        if (strcmp(method, "/bridge_set_default_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetDefaultValue, argc, argv, types);
        if (strcmp(method, "/bridge_set_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetProgram, argc, argv, types);
        if (strcmp(method, "/bridge_set_midi_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetMidiProgram, argc, argv, types);
        if (strcmp(method, "/bridge_set_custom_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetCustomData, argc, argv, types);
        if (strcmp(method, "/bridge_set_chunk_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetChunkData, argc, argv, types);
        if (strcmp(method, "/bridge_update") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeUpdateNow, argc, argv, types);
    }

    qWarning("CarlaOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_register(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source)
{
    qDebug("CarlaOsc::handle_register()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "s");

    if (m_controllerData.path)
    {
        qWarning("CarlaOsc::handle_register() - OSC backend already registered to %s", m_controllerData.path);
        return 1;
    }

    const char* const url = (const char*)&argv[0]->s;
    const char* host;
    const char* port;

    qDebug("CarlaOsc::handle_register() - OSC backend registered to %s", url);

    host = lo_address_get_hostname(source);
    port = lo_address_get_port(source);
    m_controllerData.source = lo_address_new(host, port);

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    m_controllerData.path   = lo_url_get_path(url);
    m_controllerData.target = lo_address_new(host, port);

    free((void*)host);
    free((void*)port);

    // FIXME - max plugins
    for (unsigned short i=0; i < CarlaEngine::maxPluginNumber(); i++)
    {
        CarlaPlugin* const plugin = engine->getPluginUnchecked(i);

        if (plugin && plugin->enabled())
            plugin->registerToOsc();
    }

    return 0;
}

int CarlaOsc::handle_unregister()
{
    qDebug("CarlaOsc::handle_unregister()");

    if (! m_controllerData.path)
    {
        qWarning("CarlaOsc::handle_unregister() - OSC backend is not registered yet");
        return 1;
    }

    osc_clear_data(&m_controllerData);
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_update(CARLA_OSC_HANDLE_ARGS2, const lo_address source)
{
    qDebug("CarlaOsc::handle_update()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = (const char*)&argv[0]->s;
    plugin->updateOscData(source, url);

    return 0;
}

int CarlaOsc::handle_configure(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_configure()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ss");

    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    plugin->setCustomData(CUSTOM_DATA_STRING, key, value, false);

    return 0;
}

int CarlaOsc::handle_control(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_control()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "if");

    const int  rindex = argv[0]->i;
    const float value = argv[1]->f;
    plugin->setParameterValueByRIndex(rindex, value, false, true, true);

    return 0;
}

int CarlaOsc::handle_program(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_program()");

    if (argc == 2)
    {
        CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

        const uint32_t bank_id    = argv[0]->i;
        const uint32_t program_id = argv[1]->i;
        plugin->setMidiProgramById(bank_id, program_id, false, true, true, true);

        return 0;
    }
    else
    {
        CARLA_OSC_CHECK_OSC_TYPES(1, "i");

        const uint32_t program_id = argv[0]->i;

        if (program_id < plugin->programCount())
        {
            plugin->setProgram(program_id, false, true, true, true);
            return 0;
        }

        qCritical("CarlaOsc::handle_program() - program_id '%i' out of bounds", program_id);
    }

    return 1;
}

int CarlaOsc::handle_midi(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_midi()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "m");

    if (plugin->midiInCount() > 0)
    {
        const uint8_t* const data = argv[0]->m;
        uint8_t status  = data[1];
        uint8_t channel = status & 0x0F;

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

    qWarning("CarlaOsc::handle_midi() - recived midi when plugin has no midi inputs");
    return 1;
}

int CarlaOsc::handle_exiting(CARLA_OSC_HANDLE_ARGS1)
{
    qDebug("CarlaOsc::handle_exiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    engine->callback(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
    plugin->clearOscData();

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

int CarlaOsc::handle_set_active(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_active()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const bool active = (bool)argv[0]->i;
    plugin->setActive(active, false, true);

    return 0;
}

int CarlaOsc::handle_set_drywet(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_drywet()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "d");

    const double value = argv[0]->d;
    plugin->setDryWet(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_volume(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_volume()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "d");

    const double value = argv[0]->d;
    plugin->setVolume(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_balance_left(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_balance_left()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "d");

    const double value = argv[0]->d;
    plugin->setBalanceLeft(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_balance_right(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_balance_right()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "d");

    const double value = argv[0]->d;
    plugin->setBalanceRight(value, false, true);

    return 0;
}

int CarlaOsc::handle_set_parameter_value(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_parameter_value()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    plugin->setParameterValue(index, value, true, false, true);

    return 0;
}

int CarlaOsc::handle_set_parameter_midi_cc(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_parameter_midi_cc()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index = argv[0]->i;
    const int32_t cc    = argv[1]->i;
    plugin->setParameterMidiCC(index, cc, false, true);

    return 0;
}

int CarlaOsc::handle_set_parameter_midi_channel(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_parameter_midi_channel()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;
    plugin->setParameterMidiChannel(index, channel, false, true);

    return 0;
}

int CarlaOsc::handle_set_program(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_program()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;
    plugin->setProgram(index, true, false, true, true);

    return 0;
}

int CarlaOsc::handle_set_midi_program(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_set_midi_program()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;
    plugin->setMidiProgram(index, true, false, true, true);

    return 0;
}

int CarlaOsc::handle_note_on(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_note_on()");
    CARLA_OSC_CHECK_OSC_TYPES(3, "iii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    const int32_t velo    = argv[2]->i;
    plugin->sendMidiSingleNote(channel, note, velo, true, false, true);

    return 0;
}

int CarlaOsc::handle_note_off(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handle_note_off()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    plugin->sendMidiSingleNote(channel, note, 0, true, false, true);

    return 0;
}

int CarlaOsc::handle_bridge_set_input_peak_value(CARLA_OSC_HANDLE_ARGS2)
{
    CARLA_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setInputPeak(plugin->id(), index-1, value);

    return 0;
}

int CarlaOsc::handle_bridge_set_output_peak_value(CARLA_OSC_HANDLE_ARGS2)
{
    CARLA_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setOutputPeak(plugin->id(), index-1, value);

    return 0;
}

CARLA_BACKEND_END_NAMESPACE

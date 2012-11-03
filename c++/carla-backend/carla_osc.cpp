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

#include "carla_osc.hpp"
#include "carla_plugin.hpp"

CARLA_BACKEND_START_NAMESPACE

unsigned int uintMin(unsigned int value1, unsigned int value2)
{
    return value1 < value2 ? value1 : value2;
}

void osc_error_handlerTCP(const int num, const char* const msg, const char* const path)
{
    qCritical("CarlaBackend::osc_error_handlerTCP(%i, \"%s\", \"%s\")", num, msg, path);
}

void osc_error_handlerUDP(const int num, const char* const msg, const char* const path)
{
    qCritical("CarlaBackend::osc_error_handlerUDP(%i, \"%s\", \"%s\")", num, msg, path);
}

// -----------------------------------------------------------------------

CarlaOsc::CarlaOsc(CarlaEngine* const engine_)
    : engine(engine_)
{
    qDebug("CarlaOsc::CarlaOsc(%p)", engine);
    CARLA_ASSERT(engine);

    m_serverPathTCP = nullptr;
    m_serverPathUDP = nullptr;
    m_serverThreadTCP = nullptr;
    m_serverThreadUDP = nullptr;
    m_controlData.path = nullptr;
    m_controlData.source = nullptr;
    m_controlData.target = nullptr;

    m_name = nullptr;
    m_nameSize = 0;
}

CarlaOsc::~CarlaOsc()
{
    qDebug("CarlaOsc::~CarlaOsc()");
}

void CarlaOsc::init(const char* const name)
{
    qDebug("CarlaOsc::init(\"%s\")", name);
    CARLA_ASSERT(! m_serverPathTCP);
    CARLA_ASSERT(! m_serverPathUDP);
    CARLA_ASSERT(! m_serverThreadTCP);
    CARLA_ASSERT(! m_serverThreadUDP);
    CARLA_ASSERT(name);
    CARLA_ASSERT(m_nameSize == 0);

    m_name = strdup(name ? name : "");
    m_nameSize = strlen(m_name);

    // create new OSC thread
    m_serverThreadTCP = lo_server_thread_new_with_proto(nullptr, LO_TCP, osc_error_handlerTCP);
    m_serverThreadUDP = lo_server_thread_new_with_proto(nullptr, LO_UDP, osc_error_handlerUDP);

    // get our full OSC server path
    char* const threadPathTCP = lo_server_thread_get_url(m_serverThreadTCP);
    m_serverPathTCP = strdup(QString("%1%2").arg(threadPathTCP).arg(m_name).toUtf8().constData());
    free(threadPathTCP);

    char* const threadPathUDP = lo_server_thread_get_url(m_serverThreadUDP);
    m_serverPathUDP = strdup(QString("%1%2").arg(threadPathUDP).arg(m_name).toUtf8().constData());
    free(threadPathUDP);

    // register message handler and start OSC thread
    lo_server_thread_add_method(m_serverThreadTCP, nullptr, nullptr, osc_message_handler, this);
    lo_server_thread_add_method(m_serverThreadUDP, nullptr, nullptr, osc_message_handler, this);
    lo_server_thread_start(m_serverThreadTCP);
    lo_server_thread_start(m_serverThreadUDP);
}

void CarlaOsc::close()
{
    qDebug("CarlaOsc::close()");
    CARLA_ASSERT(m_serverPathTCP);
    CARLA_ASSERT(m_serverPathUDP);
    CARLA_ASSERT(m_serverThreadTCP);
    CARLA_ASSERT(m_serverThreadUDP);
    CARLA_ASSERT(m_name);

    m_controlData.free();

    lo_server_thread_stop(m_serverThreadTCP);
    lo_server_thread_stop(m_serverThreadUDP);
    lo_server_thread_del_method(m_serverThreadTCP, nullptr, nullptr);
    lo_server_thread_del_method(m_serverThreadUDP, nullptr, nullptr);
    lo_server_thread_free(m_serverThreadTCP);
    lo_server_thread_free(m_serverThreadUDP);

    free((void*)m_serverPathTCP);
    free((void*)m_serverPathUDP);
    m_serverPathTCP = nullptr;
    m_serverPathUDP = nullptr;
    m_serverThreadTCP = nullptr;
    m_serverThreadUDP = nullptr;

    free(m_name);
    m_name = nullptr;
    m_nameSize = 0;
}

void CarlaOsc::waitForEvents()
{
    if (m_serverThreadTCP)
    {
        while (lo_server_thread_events_pending(m_serverThreadTCP))
            carla_msleep(10);
    }

    if (m_serverThreadUDP)
    {
        while (lo_server_thread_events_pending(m_serverThreadUDP))
            carla_msleep(10);
    }
}

// -----------------------------------------------------------------------

int CarlaOsc::handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
#if DEBUG
    if (! QString(path).endsWith("peak"))
        qDebug("CarlaOsc::handleMessage(%s, %i, %p, %s, %p)", path, argc, argv, types, msg);
#endif
    CARLA_ASSERT(m_serverPathTCP || m_serverPathUDP);
    CARLA_ASSERT(m_serverThreadTCP || m_serverThreadUDP);
    CARLA_ASSERT(path);

    if (! path)
        return 1;

    // Initial path check
    if (strcmp(path, "/register") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgRegister(argc, argv, types, source);
    }
    if (strcmp(path, "/unregister") == 0)
    {
        return handleMsgUnregister();
    }

    // Check if message is for this client
    if (strlen(path) <= m_nameSize || strncmp(path+1, m_name, m_nameSize) != 0)
    {
        qWarning("CarlaOsc::handleMessage() - message not for this client -> '%s' != '/%s/'", path, m_name);
        return 1;
    }

    // Get plugin id from message
    int pluginId = 0;

    if (std::isdigit(path[m_nameSize+2]))
        pluginId += path[m_nameSize+2]-'0';

    if (std::isdigit(path[m_nameSize+3]))
        pluginId += (path[m_nameSize+3]-'0')*10;

    if (pluginId < 0 || pluginId > CarlaEngine::maxPluginNumber())
    {
        qCritical("CarlaOsc::handleMessage() - failed to get plugin, wrong id '%i'", pluginId);
        return 1;
    }

    // Get plugin
    CarlaPlugin* const plugin = engine->getPluginUnchecked(pluginId);

    if (plugin == nullptr || plugin->id() != pluginId)
    {
        qWarning("CarlaOsc::handleMessage() - invalid plugin id '%i', probably has been removed", pluginId);
        return 1;
    }

    // Get method from path, "/Carla/i/method"
    const int offset = (pluginId >= 10) ? 4 : 3;
    char  method[32] = { 0 };
    memcpy(method, path + (m_nameSize + offset), uintMin(strlen(path), 32));

    if (method[0] == 0 || method[0] != '/')
        return 1;

    // Common OSC methods (DSSI and internal UIs)
    if (strcmp(method, "/update") == 0)
    {
        const lo_address source = lo_message_get_source(msg);
        return handleMsgUpdate(plugin, argc, argv, types, source);
    }
    if (strcmp(method, "/configure") == 0)
        return handleMsgConfigure(plugin, argc, argv, types);
    if (strcmp(method, "/control") == 0)
        return handleMsgControl(plugin, argc, argv, types);
    if (strcmp(method, "/program") == 0)
        return handleMsgProgram(plugin, argc, argv, types);
    if (strcmp(method, "/midi") == 0)
        return handleMsgMidi(plugin, argc, argv, types);
    if (strcmp(method, "/exiting") == 0)
        return handleMsgExiting(plugin);

    // Internal methods
    if (strcmp(method, "/set_active") == 0)
        return handleMsgSetActive(plugin, argc, argv, types);
    if (strcmp(method, "/set_drywet") == 0)
        return handleMsgSetDryWet(plugin, argc, argv, types);
    if (strcmp(method, "/set_volume") == 0)
        return handleMsgSetVolume(plugin, argc, argv, types);
    if (strcmp(method, "/set_balance_left") == 0)
        return handleMsgSetBalanceLeft(plugin, argc, argv, types);
    if (strcmp(method, "/set_balance_right") == 0)
        return handleMsgSetBalanceRight(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_value") == 0)
        return handleMsgSetParameterValue(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_midi_cc") == 0)
        return handleMsgSetParameterMidiCC(plugin, argc, argv, types);
    if (strcmp(method, "/set_parameter_midi_channel") == 0)
        return handleMsgSetParameterMidiChannel(plugin, argc, argv, types);
    if (strcmp(method, "/set_program") == 0)
        return handleMsgSetProgram(plugin, argc, argv, types);
    if (strcmp(method, "/set_midi_program") == 0)
        return handleMsgSetMidiProgram(plugin, argc, argv, types);
    if (strcmp(method, "/note_on") == 0)
        return handleMsgNoteOn(plugin, argc, argv, types);
    if (strcmp(method, "/note_off") == 0)
        return handleMsgNoteOff(plugin, argc, argv, types);

    // Plugin-specific methods
#ifdef WANT_LV2
    if (strcmp(method, "/lv2_atom_transfer") == 0)
        return handleMsgLv2AtomTransfer(plugin, argc, argv, types);
    if (strcmp(method, "/lv2_event_transfer") == 0)
        return handleMsgLv2EventTransfer(plugin, argc, argv, types);
#endif

    // Plugin Bridges
    if (strncmp(method, "/bridge_", 8) == 0 && (plugin->hints() & PLUGIN_IS_BRIDGE) > 0)
    {
        if (strcmp(method+8, "set_inpeak") == 0)
            return handleMsgBridgeSetInPeak(plugin, argc, argv, types);
        if (strcmp(method+8, "set_outpeak") == 0)
            return handleMsgBridgeSetOutPeak(plugin, argc, argv, types);
        if (strcmp(method+8, "audio_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeAudioCount, argc, argv, types);
        if (strcmp(method+8, "midi_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiCount, argc, argv, types);
        if (strcmp(method+8, "parameter_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterCount, argc, argv, types);
        if (strcmp(method+8, "program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramCount, argc, argv, types);
        if (strcmp(method+8, "midi_program_count") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramCount, argc, argv, types);
        if (strcmp(method+8, "plugin_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgePluginInfo, argc, argv, types);
        if (strcmp(method+8, "parameter_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterInfo, argc, argv, types);
        if (strcmp(method+8, "parameter_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterData, argc, argv, types);
        if (strcmp(method+8, "parameter_ranges") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeParameterRanges, argc, argv, types);
        if (strcmp(method+8, "program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeProgramInfo, argc, argv, types);
        if (strcmp(method+8, "midi_program_info") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeMidiProgramInfo, argc, argv, types);
        if (strcmp(method+8, "configure") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeConfigure, argc, argv, types);
        if (strcmp(method+8, "set_parameter_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetParameterValue, argc, argv, types);
        if (strcmp(method+8, "set_default_value") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetDefaultValue, argc, argv, types);
        if (strcmp(method+8, "set_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetProgram, argc, argv, types);
        if (strcmp(method+8, "set_midi_program") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetMidiProgram, argc, argv, types);
        if (strcmp(method+8, "set_custom_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetCustomData, argc, argv, types);
        if (strcmp(method+8, "set_chunk_data") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeSetChunkData, argc, argv, types);
        if (strcmp(method+8, "update") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeUpdateNow, argc, argv, types);
        if (strcmp(method+8, "error") == 0)
            return plugin->setOscBridgeInfo(PluginBridgeError, argc, argv, types);
    }

    qWarning("CarlaOsc::handleMessage() - unsupported OSC method '%s'", method);
    return 1;
}

// -----------------------------------------------------------------------

int CarlaOsc::handleMsgRegister(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source)
{
    qDebug("CarlaOsc::handleMsgRegister()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "s");

    if (m_controlData.path)
    {
        qWarning("CarlaOsc::handleMsgRegister() - OSC backend already registered to %s", m_controlData.path);
        return 1;
    }

    const char* const url = (const char*)&argv[0]->s;
    const char* host;
    const char* port;

    qDebug("CarlaOsc::handleMsgRegister() - OSC backend registered to %s", url);

    host = lo_address_get_hostname(source);
    port = lo_address_get_port(source);
    m_controlData.source = lo_address_new_with_proto(LO_TCP, host, port);

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    m_controlData.path   = lo_url_get_path(url);
    m_controlData.target = lo_address_new_with_proto(LO_TCP, host, port);

    free((void*)host);
    free((void*)port);

    // FIXME - max plugins
    for (unsigned short i=0; i < CarlaEngine::maxPluginNumber(); i++)
    {
        CarlaPlugin* const plugin = engine->getPluginUnchecked(i);

        if (plugin && plugin->enabled())
            plugin->registerToOscControl();
    }

    return 0;
}

int CarlaOsc::handleMsgUnregister()
{
    qDebug("CarlaOsc::handleMsgUnregister()");

    if (! m_controlData.path)
    {
        qWarning("CarlaOsc::handleMsgUnregister() - OSC backend is not registered yet");
        return 1;
    }

    m_controlData.free();
    return 0;
}

// -----------------------------------------------------------------------

int CarlaOsc::handleMsgUpdate(CARLA_OSC_HANDLE_ARGS2, const lo_address source)
{
    qDebug("CarlaOsc::handleMsgUpdate()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "s");

    const char* const url = (const char*)&argv[0]->s;
    plugin->updateOscData(source, url);

    return 0;
}

int CarlaOsc::handleMsgConfigure(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgConfigure()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ss");

    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    plugin->setCustomData(CUSTOM_DATA_STRING, key, value, false);

    return 0;
}

int CarlaOsc::handleMsgControl(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgControl()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "if");

    const int  rindex = argv[0]->i;
    const float value = argv[1]->f;
    plugin->setParameterValueByRIndex(rindex, value, false, true, true);

    return 0;
}

int CarlaOsc::handleMsgProgram(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgProgram()");

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

        qCritical("CarlaOsc::handleMsgProgram() - program_id '%i' out of bounds", program_id);
    }

    return 1;
}

int CarlaOsc::handleMsgMidi(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgMidi()");
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

    qWarning("CarlaOsc::handleMsgMidi() - recived midi when plugin has no midi inputs");
    return 1;
}

int CarlaOsc::handleMsgExiting(CARLA_OSC_HANDLE_ARGS1)
{
    qDebug("CarlaOsc::handleMsgExiting()");

    // TODO - check for non-UIs (dssi-vst) and set to -1 instead
    engine->callback(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
    plugin->freeOscData();

    return 0;
}

// -----------------------------------------------------------------------

int CarlaOsc::handleMsgSetActive(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetActive()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const bool active = (bool)argv[0]->i;
    plugin->setActive(active, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetDryWet(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetDryWet()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;
    plugin->setDryWet(value, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetVolume(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetVolume()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;
    plugin->setVolume(value, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetBalanceLeft(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetBalanceLeft()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;
    plugin->setBalanceLeft(value, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetBalanceRight(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetBalanceRight()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "f");

    const float value = argv[0]->f;
    plugin->setBalanceRight(value, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetParameterValue(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetParameterValue()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "if");

    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;
    plugin->setParameterValue(index, value, true, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetParameterMidiCC(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetParameterMidiCC()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index = argv[0]->i;
    const int32_t cc    = argv[1]->i;
    plugin->setParameterMidiCC(index, cc, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetParameterMidiChannel(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetParameterMidiChannel()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t index   = argv[0]->i;
    const int32_t channel = argv[1]->i;
    plugin->setParameterMidiChannel(index, channel, false, true);

    return 0;
}

int CarlaOsc::handleMsgSetProgram(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetProgram()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;
    plugin->setProgram(index, true, false, true, true);

    if (index >= 0)
    {
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
            engine->osc_send_control_set_parameter_value(plugin->id(), i, plugin->getParameterValue(i));
    }

    return 0;
}

int CarlaOsc::handleMsgSetMidiProgram(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgSetMidiProgram()");
    CARLA_OSC_CHECK_OSC_TYPES(1, "i");

    const int32_t index = argv[0]->i;
    plugin->setMidiProgram(index, true, false, true, true);

    if (index >= 0)
    {
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
            engine->osc_send_control_set_parameter_value(plugin->id(), i, plugin->getParameterValue(i));
    }

    return 0;
}

int CarlaOsc::handleMsgNoteOn(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgNoteOn()");
    CARLA_OSC_CHECK_OSC_TYPES(3, "iii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    const int32_t velo    = argv[2]->i;
    plugin->sendMidiSingleNote(channel, note, velo, true, false, true);

    return 0;
}

int CarlaOsc::handleMsgNoteOff(CARLA_OSC_HANDLE_ARGS2)
{
    qDebug("CarlaOsc::handleMsgNoteOff()");
    CARLA_OSC_CHECK_OSC_TYPES(2, "ii");

    const int32_t channel = argv[0]->i;
    const int32_t note    = argv[1]->i;
    plugin->sendMidiSingleNote(channel, note, 0, true, false, true);

    return 0;
}

int CarlaOsc::handleMsgBridgeSetInPeak(CARLA_OSC_HANDLE_ARGS2)
{
    CARLA_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setInputPeak(plugin->id(), index-1, value);

    return 0;
}

int CarlaOsc::handleMsgBridgeSetOutPeak(CARLA_OSC_HANDLE_ARGS2)
{
    CARLA_OSC_CHECK_OSC_TYPES(2, "id");

    const int32_t index = argv[0]->i;
    const double  value = argv[1]->d;
    engine->setOutputPeak(plugin->id(), index-1, value);

    return 0;
}

CARLA_BACKEND_END_NAMESPACE

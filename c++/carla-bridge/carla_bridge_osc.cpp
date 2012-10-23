/*
 * Carla Plugin bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

#include "carla_bridge_osc.h"
#include "carla_bridge_client.h"
#include "carla_midi.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

CARLA_BRIDGE_START_NAMESPACE

unsigned int uintMin(unsigned int value1, unsigned int value2)
{
    return value1 < value2 ? value1 : value2;
}

void osc_error_handler(const int num, const char* const msg, const char* const path)
{
    qCritical("osc_error_handler(%i, \"%s\", \"%s\")", num, msg, path);
}

// -----------------------------------------------------------------------

CarlaBridgeOsc::CarlaBridgeOsc(CarlaClient* const client_, const char* const name)
    : client(client_)
{
    qDebug("CarlaBridgeOsc::CarlaOsc(%p, \"%s\")", client, name);
    CARLA_ASSERT(client);
    CARLA_ASSERT(name);

    m_server = nullptr;
    m_serverPath = nullptr;
    m_controlData.path = nullptr;
    m_controlData.source = nullptr; // unused
    m_controlData.target = nullptr;

    m_name = strdup(name ? name : "");
    m_nameSize = strlen(m_name);
}

CarlaBridgeOsc::~CarlaBridgeOsc()
{
    qDebug("CarlaBridgeOsc::~CarlaOsc()");

    if (m_name)
        free(m_name);
}

bool CarlaBridgeOsc::init(const char* const url)
{
    qDebug("CarlaBridgeOsc::init(\"%s\")", url);
    CARLA_ASSERT(! m_server);
    CARLA_ASSERT(! m_serverPath);
    CARLA_ASSERT(url);

    char* host = lo_url_get_hostname(url);
    char* port = lo_url_get_port(url);

    m_controlData.path   = lo_url_get_path(url);
    m_controlData.target = lo_address_new_with_proto(LO_TCP, host, port);

    free(host);
    free(port);

    if (! m_controlData.path)
    {
        qCritical("CarlaBridgeOsc::init(\"%s\") - failed to init OSC", url);
        return false;
    }

    // create new OSC thread
    m_server = lo_server_new_with_proto(nullptr, LO_TCP, osc_error_handler);

    // get our full OSC server path
    char* const threadPath = lo_server_get_url(m_server);
    m_serverPath = strdup(QString("%1%2").arg(threadPath).arg(m_name).toUtf8().constData());
    free(threadPath);

    // register message handler
    lo_server_add_method(m_server, nullptr, nullptr, osc_message_handler, this);

    return true;
}

void CarlaBridgeOsc::close()
{
    qDebug("CarlaBridgeOsc::close()");
    CARLA_ASSERT(m_server);
    CARLA_ASSERT(m_serverPath);

    osc_clear_data(&m_controlData);

    lo_server_del_method(m_server, nullptr, nullptr);
    lo_server_free(m_server);

    free((void*)m_serverPath);
    m_serverPath = nullptr;
}

// -----------------------------------------------------------------------

int CarlaBridgeOsc::handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg)
{
    qDebug("CarlaBridgeOsc::handleMessage(\"%s\", %i, %p, \"%s\", %p)", path, argc, argv, types, msg);
    CARLA_ASSERT(m_server);
    CARLA_ASSERT(m_serverPath);
    CARLA_ASSERT(path);

    // Check if message is for this client
    if ((! path) || strlen(path) <= m_nameSize || strncmp(path+1, m_name, m_nameSize) != 0)
    {
        qWarning("CarlaBridgeOsc::handleMessage() - message not for this client: '%s' != '/%s/'", path, m_name);
        return 1;
    }

    char method[32] = { 0 };
    memcpy(method, path + (m_nameSize + 1), uintMin(strlen(path), 32));

    if (method[0] == 0)
        return 1;

    // Common OSC methods
    if (strcmp(method, "/configure") == 0)
        return handleMsgConfigure(argc, argv, types);
    if (strcmp(method, "/control") == 0)
        return handleMsgControl(argc, argv, types);
    if (strcmp(method, "/program") == 0)
        return handleMsgProgram(argc, argv, types);
    if (strcmp(method, "/midi_program") == 0)
        return handleMsgMidiProgram(argc, argv, types);
    if (strcmp(method, "/midi") == 0)
        return handleMsgMidi(argc, argv, types);
    if (strcmp(method, "/sample-rate") == 0)
        return 0; // unused
    if (strcmp(method, "/show") == 0)
        return handleMsgShow();
    if (strcmp(method, "/hide") == 0)
        return handleMsgHide();
    if (strcmp(method, "/quit") == 0)
        return handleMsgQuit();

#ifdef BRIDGE_LV2
    if (strcmp(method, "/lv2_atom_transfer") == 0)
        return handleMsgLv2TransferAtom(argc, argv, types);
    if (strcmp(method, "/lv2_event_transfer") == 0)
        return handleMsgLv2TransferEvent(argc, argv, types);
#endif

#if 0
    else if (strcmp(method, "set_parameter_midi_channel") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
    else if (strcmp(method, "set_parameter_midi_cc") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
#endif

    qWarning("CarlaBridgeOsc::handleMessage(\"%s\", ...) - got unsupported OSC method '%s'", path, method);
    return 1;
}

int CarlaBridgeOsc::handleMsgConfigure(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgConfigure()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ss");

    if (! client)
        return 1;

#ifdef BUILD_BRIDGE_PLUGIN
    const char* const key   = (const char*)&argv[0]->s;
    const char* const value = (const char*)&argv[1]->s;

    if (strcmp(key, CarlaBackend::CARLA_BRIDGE_MSG_SAVE_NOW) == 0)
    {
        client->saveNow();
    }
    else if (strcmp(key, CarlaBackend::CARLA_BRIDGE_MSG_SET_CHUNK) == 0)
    {
        client->setChunkData(value);
    }
    else if (strcmp(key, CarlaBackend::CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
    {
        QStringList vList = QString(value).split("·", QString::KeepEmptyParts);

        if (vList.size() == 3)
        {
            const char* const cType  = vList.at(0).toUtf8().constData();
            const char* const cKey   = vList.at(1).toUtf8().constData();
            const char* const cValue = vList.at(2).toUtf8().constData();

            client->setCustomData(cType, cKey, cValue);
        }
    }
#else
    Q_UNUSED(argv);
#endif

    return 0;
}

int CarlaBridgeOsc::handleMsgControl(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgControl()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "if");

    if (! client)
        return 1;

    const int32_t index = argv[0]->i;
    const float   value = argv[1]->f;
    client->setParameter(index, value);

    return 0;
}

int CarlaBridgeOsc::handleMsgProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgProgram()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "i");

    if (! client)
        return 1;

    const int32_t index = argv[0]->i;
    client->setProgram(index);

    return 0;
}

int CarlaBridgeOsc::handleMsgMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgMidiProgram()");
#ifdef BUILD_BRIDGE_PLUGIN
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "i");
#else
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ii");
#endif

    if (! client)
        return 1;

#ifdef BUILD_BRIDGE_PLUGIN
    const int32_t index = argv[0]->i;
    client->setProgram(index);
#else
    const int32_t bank    = argv[0]->i;
    const int32_t program = argv[1]->i;
    client->setMidiProgram(bank, program);
#endif

    return 0;
}

int CarlaBridgeOsc::handleMsgMidi(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaBridgeOsc::handleMsgMidi()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(1, "m");

    if (! client)
        return 1;

    const uint8_t* const mdata = argv[0]->m;
    const uint8_t      data[4] = { mdata[0], mdata[1], mdata[2], mdata[3] };

    uint8_t status  = data[1];
    uint8_t channel = status & 0x0F;

    // Fix bad note-off
    if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
        status -= 0x10;

    if (MIDI_IS_STATUS_NOTE_OFF(status))
    {
        uint8_t note = data[2];
        client->noteOff(channel, note);
    }
    else if (MIDI_IS_STATUS_NOTE_ON(status))
    {
        uint8_t note = data[2];
        uint8_t velo = data[3];
        client->noteOn(channel, note, velo);
    }

    return 0;
}

int CarlaBridgeOsc::handleMsgShow()
{
    qDebug("CarlaBridgeOsc::handleMsgShow()");

    if (! client)
        return 1;

    client->toolkitShow();

    return 0;
}

int CarlaBridgeOsc::handleMsgHide()
{
    qDebug("CarlaBridgeOsc::handleMsgHide()");

    if (! client)
        return 1;

    client->toolkitHide();

    return 0;
}

int CarlaBridgeOsc::handleMsgQuit()
{
    qDebug("CarlaBridgeOsc::handleMsgQuit()");

    if (! client)
        return 1;

    client->toolkitQuit();

    return 0;
}

CARLA_BRIDGE_END_NAMESPACE

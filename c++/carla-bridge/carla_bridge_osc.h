/*
 * Carla bridge code
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

#ifndef CARLA_BRIDGE_OSC_H
#define CARLA_BRIDGE_OSC_H

#include "carla_bridge.h"
#include "carla_osc_includes.h"

#define CARLA_BRIDGE_OSC_HANDLE_ARGS const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                  \
    Q_ASSERT(m_serverPath);                                                                                                 \
    Q_ASSERT(m_serverThread);                                                                                               \
    /* check argument count */                                                                                              \
    if (argc != argcToCompare)                                                                                              \
    {                                                                                                                       \
        qCritical("CarlaBridgeOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                           \
    }                                                                                                                       \
    if (argc > 0)                                                                                                           \
    {                                                                                                                       \
        /* check for nullness */                                                                                            \
        if (! (types && typesToCompare))                                                                                    \
        {                                                                                                                   \
            qCritical("CarlaBridgeOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                       \
        }                                                                                                                   \
        /* check argument types */                                                                                          \
        if (strcmp(types, typesToCompare) != 0)                                                                             \
        {                                                                                                                   \
            qCritical("CarlaBridgeOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                       \
        }                                                                                                                   \
    }

CARLA_BRIDGE_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeOSC Carla Bridge OSC
 *
 * The Carla Bridge OSC.
 * @{
 */

class CarlaBridgeOsc
{
public:
    CarlaBridgeOsc(CarlaClient* const client, const char* const name);
    ~CarlaBridgeOsc();

    bool init(const char* const url);
    void close();

    // -------------------------------------------------------------------

    const CarlaOscData* getControllerData() const
    {
        return &m_controlData;
    }

    void sendOscConfigure(const char* const key, const char* const value)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_configure(&m_controlData, key, value);
    }

    void sendOscControl(const int32_t index, const float value)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_control(&m_controlData, index, value);
    }

    void sendOscProgram(const int32_t index)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_program(&m_controlData, index);
    }

    void sendOscMidiProgram(const int32_t index)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_midi_program(&m_controlData, index);
    }

    void sendOscMidi(const uint8_t midiBuf[4])
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_midi(&m_controlData, midiBuf);
    }

    void sendOscUpdate()
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_update(&m_controlData, m_serverPath);
    }

    void sendOscExiting()
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_exiting(&m_controlData);
    }

#ifdef BUILD_BRIDGE_PLUGIN
    void sendOscBridgeUpdate()
    {
        Q_ASSERT(m_controlData.target && m_serverPath);

        if (m_controlData.target && m_serverPath)
            osc_send_bridge_update(&m_controlData, m_serverPath);
    }

    void sendOscBridgeError(const char* const error)
    {
        Q_ASSERT(m_controlData.target && m_serverPath);
        Q_ASSERT(error);

        if (m_controlData.target && m_serverPath)
            osc_send_bridge_error(&m_controlData, error);
    }
#endif

#ifdef BRIDGE_LV2
    void sendOscLv2TransferAtom(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_lv2_transfer_atom(&m_controlData, portIndex, typeStr, atomBuf);
    }

    void sendOscLv2TransferEvent(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
    {
        Q_ASSERT(m_controlData.target);

        if (m_controlData.target)
            osc_send_lv2_transfer_event(&m_controlData, portIndex, typeStr, atomBuf);
    }
#endif

    // -------------------------------------------------------------------

protected:
    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);
    int handleMsgConfigure(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgControl(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgMidiProgram(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgMidi(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgShow();
    int handleMsgHide();
    int handleMsgQuit();

#ifdef BRIDGE_LV2
    int handleMsgLv2TransferAtom(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handleMsgLv2TransferEvent(CARLA_BRIDGE_OSC_HANDLE_ARGS);
#endif

    // -------------------------------------------------------------------

private:
    CarlaClient* const client;

    const char* m_serverPath;
    lo_server_thread m_serverThread;
    CarlaOscData m_controlData;

    char*  m_name;
    size_t m_nameSize;

    // -------------------------------------------------------------------

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const user_data)
    {
        Q_ASSERT(user_data);
        CarlaBridgeOsc* const _this_ = (CarlaBridgeOsc*)user_data;
        return _this_->handleMessage(path, argc, argv, types, msg);
    }
};

/**@}*/

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_OSC_H

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

#ifndef CARLA_OSC_H
#define CARLA_OSC_H

#include "carla_backend.h"
#include "carla_osc_utils.hpp"

#define CARLA_OSC_HANDLE_ARGS1 CarlaPlugin* const plugin
#define CARLA_OSC_HANDLE_ARGS2 CARLA_OSC_HANDLE_ARGS1, const int argc, const lo_arg* const* const argv, const char* const types

#define CARLA_OSC_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                   \
    /* check argument count */                                                                                        \
    if (argc != argcToCompare)                                                                                        \
    {                                                                                                                 \
        qCritical("CarlaOsc::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                     \
    }                                                                                                                 \
    if (argc > 0)                                                                                                     \
    {                                                                                                                 \
        /* check for nullness */                                                                                      \
        if (! (types && typesToCompare))                                                                              \
        {                                                                                                             \
            qCritical("CarlaOsc::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                 \
        }                                                                                                             \
        /* check argument types */                                                                                    \
        if (strcmp(types, typesToCompare) != 0)                                                                       \
        {                                                                                                             \
            qCritical("CarlaOsc::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                 \
        }                                                                                                             \
    }

CARLA_BACKEND_START_NAMESPACE

class CarlaOsc
{
public:
    CarlaOsc(CarlaEngine* const engine);
    ~CarlaOsc();

    void init(const char* const name);
    void close();
    void waitForEvents();

    // -------------------------------------------------------------------

    bool isControlRegistered() const
    {
        return bool(m_controlData.target);
    }

    const CarlaOscData* getControlData() const
    {
        return &m_controlData;
    }

    const char* getServerPathTCP() const
    {
        return m_serverPathTCP;
    }

    const char* getServerPathUDP() const
    {
        return m_serverPathUDP;
    }

private:
    CarlaEngine* const engine;

    const char* m_serverPathTCP;
    const char* m_serverPathUDP;
    lo_server_thread m_serverThreadTCP;
    lo_server_thread m_serverThreadUDP;
    CarlaOscData m_controlData; // for carla-control

    char*  m_name;
    size_t m_nameSize;

    // -------------------------------------------------------------------

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handleMsgRegister(const int argc, const lo_arg* const* const argv, const char* const types, const lo_address source);
    int handleMsgUnregister();

    int handleMsgUpdate(CARLA_OSC_HANDLE_ARGS2, const lo_address source);
    int handleMsgConfigure(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgControl(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgProgram(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgMidi(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgExiting(CARLA_OSC_HANDLE_ARGS1);

    int handleMsgSetActive(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetDryWet(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetVolume(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceLeft(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetBalanceRight(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterValue(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiCC(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetParameterMidiChannel(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetProgram(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgSetMidiProgram(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgNoteOn(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgNoteOff(CARLA_OSC_HANDLE_ARGS2);

#ifdef WANT_LV2
    int handleMsgLv2AtomTransfer(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgLv2EventTransfer(CARLA_OSC_HANDLE_ARGS2);
#endif

    int handleMsgBridgeSetInPeak(CARLA_OSC_HANDLE_ARGS2);
    int handleMsgBridgeSetOutPeak(CARLA_OSC_HANDLE_ARGS2);

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const userData)
    {
        CARLA_ASSERT(userData);
        CarlaOsc* const _this_ = (CarlaOsc*)userData;
        return _this_->handleMessage(path, argc, argv, types, msg);
    }
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_OSC_H

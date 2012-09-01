/*
 * Carla bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

class CarlaBridgeOsc
{
public:
    CarlaBridgeOsc(CarlaBridgeClient* const client, const char* const name);
    ~CarlaBridgeOsc();

    bool init(const char* const url);
    void close();

    const CarlaOscData* getControllerData() const
    {
        return &m_controlData;
    }

    void sendOscConfigure(const char* const key, const char* const value)
    {
        osc_send_configure(&m_controlData, key, value);
    }

    void sendOscControl(int32_t index, float value)
    {
        osc_send_control(&m_controlData, index, value);
    }

    void sendOscUpdate()
    {
        osc_send_update(&m_controlData, m_serverPath);
    }

    void sendOscExiting()
    {
        osc_send_exiting(&m_controlData);
    }

    void sendOscLv2TransferAtom(const char* const type, const char* const value)
    {
        osc_send_lv2_transfer_atom(&m_controlData, type, value);
    }

    void sendOscLv2TransferEvent(const char* const type, const char* const value)
    {
        osc_send_lv2_transfer_event(&m_controlData, type, value);
    }

private:
    CarlaBridgeClient* const client;

    const char* m_serverPath;
    lo_server_thread m_serverThread;
    CarlaOscData m_controlData;

    char*  m_name;
    size_t m_nameSize;

    // -------------------------------------------------------------------

    static int osc_message_handler(const char* const path, const char* const types, lo_arg** const argv, const int argc, const lo_message msg, void* const user_data)
    {
        CarlaBridgeOsc* const _this_ = (CarlaBridgeOsc*)user_data;

        if (! _this_->client)
            return 1;

        return _this_->handleMessage(path, argc, argv, types, msg);
    }

    int handleMessage(const char* const path, const int argc, const lo_arg* const* const argv, const char* const types, const lo_message msg);

    int handle_configure(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_control(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_program(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_midi_program(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_midi(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_show();
    int handle_hide();
    int handle_quit();

#ifdef BRIDGE_LV2
    int handle_lv2_transfer_atom(CARLA_BRIDGE_OSC_HANDLE_ARGS);
    int handle_lv2_transfer_event(CARLA_BRIDGE_OSC_HANDLE_ARGS);
#endif
};

#ifdef BUILD_BRIDGE_PLUGIN
void osc_send_bridge_ains_peak(int index, double value);
void osc_send_bridge_aouts_peak(int index, double value);
void osc_send_bridge_audio_count(int ins, int outs, int total);
void osc_send_bridge_midi_count(int ins, int outs, int total);
void osc_send_bridge_param_count(int ins, int outs, int total);
void osc_send_bridge_program_count(int count);
void osc_send_bridge_midi_program_count(int count);
void osc_send_bridge_plugin_info(int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long uniqueId);
void osc_send_bridge_param_info(int index, const char* name, const char* unit);
void osc_send_bridge_param_data(int index, int type, int rindex, int hints, int midi_channel, int midi_cc);
void osc_send_bridge_param_ranges(int index, double def, double min, double max, double step, double step_small, double step_large);
void osc_send_bridge_program_info(int index, const char* name);
void osc_send_bridge_midi_program_info(int index, int bank, int program, const char* label);
void osc_send_bridge_custom_data(const char* stype, const char* key, const char* value);
void osc_send_bridge_chunk_data(const char* string_data);
void osc_send_bridge_update();
#endif

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_OSC_H

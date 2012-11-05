/*
 * Carla bridge code
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

#ifndef CARLA_BRIDGE_CLIENT_H
#define CARLA_BRIDGE_CLIENT_H

#include "carla_bridge_osc.hpp"
#include "carla_bridge_toolkit.hpp"

#ifdef BUILD_BRIDGE_PLUGIN
# include "carla_engine.hpp"
#else
# include "carla_lib_utils.hpp"
#endif

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <QtCore/QMutex>

CARLA_BRIDGE_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeClient Carla Bridge Client
 *
 * The Carla Bridge Client.
 * @{
 */

class CarlaClient
{
public:
    CarlaClient(CarlaToolkit* const toolkit)
#ifdef BUILD_BRIDGE_PLUGIN
        : m_osc(this, "carla-bridge-plugin"),
#else
        : m_osc(this, "carla-bridge-ui"),
#endif
          m_toolkit(toolkit)
    {
#ifdef BUILD_BRIDGE_UI
        m_filename = nullptr;
        m_lib = nullptr;
        m_quit = false;
#endif
        m_toolkit->m_client = this;
    }

    virtual ~CarlaClient()
    {
#ifdef BUILD_BRIDGE_UI
        if (m_filename)
            free(m_filename);
#endif
    }

    // ---------------------------------------------------------------------

#if 0
    void quequeMessage(const MessageType type, const int32_t value1, const int32_t value2, const double value3)
    {
        const QMutexLocker locker(&m_messages.lock);

        for (unsigned int i=0; i < MAX_BRIDGE_MESSAGES; i++)
        {
            Message* const m = &m_messages.data[i];

            if (m->type == MESSAGE_NULL)
            {
                m->type   = type;
                m->value1 = value1;
                m->value2 = value2;
                m->value3 = value3;
                break;
            }
        }
    }

    bool runMessages()
    {
        const QMutexLocker locker(&m_messages.lock);

        for (unsigned int i=0; i < MAX_BRIDGE_MESSAGES; i++)
        {
            Message* const m = &m_messages.data[i];

            switch (m->type)
            {
            case MESSAGE_NULL:
                return true;

            case MESSAGE_PARAMETER:
                setParameter(m->value1, m->value3);
                break;

            case MESSAGE_PROGRAM:
                setProgram(m->value1);
                break;

            case MESSAGE_MIDI_PROGRAM:
#ifdef BUILD_BRIDGE_PLUGIN
                setMidiProgram(m->value1);
#else
                setMidiProgram(m->value1, m->value2);
#endif
                break;

            case MESSAGE_NOTE_ON:
                noteOn(m->value1, m->value2, rint(m->value3));
                break;

            case MESSAGE_NOTE_OFF:
                noteOff(m->value1, m->value2);
                break;

            case MESSAGE_SHOW_GUI:
                if (m->value1)
                    m_toolkit->show();
                else
                    m_toolkit->hide();
                break;

            case MESSAGE_RESIZE_GUI:
                m_toolkit->resize(m->value1, m->value2);
                break;

            case MESSAGE_SAVE_NOW:
#ifdef BUILD_BRIDGE_PLUGIN
                saveNow();
#endif
                break;

            case MESSAGE_QUIT:
                m_toolkit->quit();
                return false;
            }

            m->type = MESSAGE_NULL;
        }

        return true;
    }
#endif

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    // ui initialization
    virtual bool init(const char* const, const char* const)
    {
        m_quit = false;
        return false;
    }

    virtual void close()
    {
        if (! m_quit)
        {
            m_quit = true;
            sendOscExiting();
        }
    }

    // ui management
    virtual void* getWidget() const = 0;
    virtual bool  isResizable() const = 0;
    virtual bool  needsReparent() const = 0;
#endif

    // processing
    virtual void setParameter(const int32_t rindex, const double value) = 0;
    virtual void setProgram(const uint32_t index) = 0;
#ifdef BUILD_BRIDGE_PLUGIN
    virtual void setMidiProgram(const uint32_t index) = 0;
#endif
#ifdef BUILD_BRIDGE_UI
    virtual void setMidiProgram(const uint32_t bank, const uint32_t program) = 0;
#endif
    virtual void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) = 0;
    virtual void noteOff(const uint8_t channel, const uint8_t note) = 0;

#ifdef BUILD_BRIDGE_PLUGIN
    // plugin
    virtual void saveNow() = 0;
    virtual void setCustomData(const char* const type, const char* const key, const char* const value) = 0;
    virtual void setChunkData(const char* const filePath) = 0;
#endif

    // ---------------------------------------------------------------------

    bool oscInit(const char* const url)
    {
        qDebug("CarlaClient::oscInit(\"%s\")", url);
        return m_osc.init(url);
    }

    bool oscIdle()
    {
        if (m_osc.m_server)
            while (lo_server_recv_noblock(m_osc.m_server, 0) != 0) {}

#ifdef BUILD_BRIDGE_UI
        return ! m_quit;
#else
        return true;
#endif
    }

    void oscClose()
    {
        qDebug("CarlaClient::oscClose()");
        m_osc.close();
    }

    void sendOscUpdate()
    {
        qDebug("CarlaClient::sendOscUpdate()");
        CARLA_ASSERT(m_osc.m_controlData.target);

        if (m_osc.m_controlData.target)
            osc_send_update(&m_osc.m_controlData, m_osc.m_serverPath);
    }

#ifdef BUILD_BRIDGE_PLUGIN
    void sendOscBridgeError(const char* const error)
    {
        qDebug("CarlaClient::sendOscBridgeError(\"%s\")", error);
        CARLA_ASSERT(m_osc.m_controlData.target);
        CARLA_ASSERT(error);

        if (m_osc.m_controlData.target)
            osc_send_bridge_error(&m_osc.m_controlData, error);
    }

    void registerOscEngine(CarlaBackend::CarlaEngine* const engine)
    {
        qDebug("CarlaClient::registerOscEngine(%p)", engine);
        engine->setOscBridgeData(&m_osc.m_controlData);
    }
#endif

    // ---------------------------------------------------------------------

    void toolkitShow()
    {
        m_toolkit->show();
    }

    void toolkitHide()
    {
        m_toolkit->hide();
    }

    void toolkitResize(int width, int height)
    {
        m_toolkit->resize(width, height);
    }

    void toolkitQuit()
    {
#ifdef BUILD_BRIDGE_UI
        m_quit = true;
#endif
        m_toolkit->quit();
    }

    // ---------------------------------------------------------------------

protected:
    void sendOscConfigure(const char* const key, const char* const value)
    {
        qDebug("CarlaClient::sendOscConfigure(\"%s\", \"%s\")", key, value);

        if (m_osc.m_controlData.target)
            osc_send_configure(&m_osc.m_controlData, key, value);
    }

    void sendOscControl(const int32_t index, const float value)
    {
        qDebug("CarlaClient::sendOscControl(%i, %f)", index, value);

        if (m_osc.m_controlData.target)
            osc_send_control(&m_osc.m_controlData, index, value);
    }

    void sendOscProgram(const int32_t index)
    {
        qDebug("CarlaClient::sendOscProgram(%i)", index);

        if (m_osc.m_controlData.target)
            osc_send_program(&m_osc.m_controlData, index);
    }

    void sendOscMidiProgram(const int32_t index)
    {
        qDebug("CarlaClient::sendOscMidiProgram(%i)", index);

        if (m_osc.m_controlData.target)
            osc_send_midi_program(&m_osc.m_controlData, index);
    }

    void sendOscMidi(const uint8_t midiBuf[4])
    {
        qDebug("CarlaClient::sendOscMidi(%p)", midiBuf);

        if (m_osc.m_controlData.target)
            osc_send_midi(&m_osc.m_controlData, midiBuf);
    }

    void sendOscExiting()
    {
        qDebug("CarlaClient::sendOscExiting()");

        if (m_osc.m_controlData.target)
            osc_send_exiting(&m_osc.m_controlData);
    }

#ifdef BUILD_BRIDGE_PLUGIN
    void sendOscBridgeUpdate()
    {
        qDebug("CarlaClient::sendOscBridgeUpdate()");
        CARLA_ASSERT(m_osc.m_controlData.target && m_osc.m_serverPath);

        if (m_osc.m_controlData.target && m_osc.m_serverPath)
            osc_send_bridge_update(&m_osc.m_controlData, m_osc.m_serverPath);
    }
#endif

#ifdef BRIDGE_LV2
    void sendOscLv2TransferAtom(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
    {
        qDebug("CarlaClient::sendOscLv2TransferAtom(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);

        if (m_osc.m_controlData.target)
            osc_send_lv2_transfer_atom(&m_osc.m_controlData, portIndex, typeStr, atomBuf);
    }

    void sendOscLv2TransferEvent(const int32_t portIndex, const char* const typeStr, const char* const atomBuf)
    {
        qDebug("CarlaClient::sendOscLv2TransferEvent(%i, \"%s\", \"%s\")", portIndex, typeStr, atomBuf);

        if (m_osc.m_controlData.target)
            osc_send_lv2_transfer_event(&m_osc.m_controlData, portIndex, typeStr, atomBuf);
    }
#endif

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    void* getContainerId()
    {
        return m_toolkit->getContainerId();
    }

    bool libOpen(const char* const filename)
    {
        CARLA_ASSERT(filename);

        if (m_filename)
            free(m_filename);

        m_lib = lib_open(filename);
        m_filename = strdup(filename ? filename : "");

        return bool(m_lib);
    }

    bool libClose()
    {
        if (m_lib)
        {
            const bool closed = lib_close(m_lib);
            m_lib = nullptr;
            return closed;
        }

        return false;
    }

    void* libSymbol(const char* const symbol)
    {
        if (m_lib)
            return lib_symbol(m_lib, symbol);

        return nullptr;
    }

    const char* libError()
    {
        return lib_error(m_filename);
    }
#endif

    // ---------------------------------------------------------------------

private:
    CarlaBridgeOsc m_osc;
    CarlaToolkit* const m_toolkit;

#ifdef BUILD_BRIDGE_UI
    char* m_filename;
    void* m_lib;
    bool  m_quit;
#endif
};

/**@}*/

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_CLIENT_H

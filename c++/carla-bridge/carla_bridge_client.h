/*
 * Carla bridge code
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

#ifndef CARLA_BRIDGE_CLIENT_H
#define CARLA_BRIDGE_CLIENT_H

#include "carla_bridge_osc.h"
#include "carla_bridge_toolkit.h"

#ifdef BUILD_BRIDGE_PLUGIN
#  include "carla_engine.h"
#else
#  include "carla_lib_includes.h"
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
#endif
    }

    virtual ~CarlaClient()
    {
#ifdef BUILD_BRIDGE_UI
        if (m_filename)
            free(m_filename);
#endif
    }

    // ---------------------------------------------------------------------

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
                setMidiProgram(m->value1, m->value2);
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

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    // ui initialization
    virtual bool init(const char* const, const char* const) = 0;
    virtual void close() = 0;

    // ui management
    virtual void* getWidget() const = 0;
    virtual bool  isResizable() const = 0;
    virtual bool  needsReparent() const = 0;
#endif

    // processing
    virtual void setParameter(const int32_t rindex, const double value) = 0;
    virtual void setProgram(const uint32_t index) = 0;
    virtual void setMidiProgram(const uint32_t bank, const uint32_t program) = 0;
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

    void oscClose()
    {
        qDebug("CarlaClient::oscClose()");
        m_osc.close();
    }

#ifdef BUILD_BRIDGE_PLUGIN
    void registerOscEngine(CarlaBackend::CarlaEngine* const engine)
    {
        qDebug("CarlaClient::registerOscEngine(%p)", engine);
        engine->setOscBridgeData(m_osc.getControllerData());
    }
#endif

    void sendOscConfigure(const char* const key, const char* const value)
    {
        qDebug("CarlaClient::sendOscConfigure(\"%s\", \"%s\")", key, value);
        m_osc.sendOscConfigure(key, value);
    }

    void sendOscControl(const int32_t index, const float value)
    {
        qDebug("CarlaClient::sendOscControl(%i, %f)", index, value);
        m_osc.sendOscControl(index, value);
    }

    void sendOscMidi(const uint8_t midiBuf[4])
    {
        qDebug("CarlaClient::sendOscMidi(%p)", midiBuf);
        m_osc.sendOscMidi(midiBuf);
    }

    void sendOscUpdate()
    {
        qDebug("CarlaClient::sendOscUpdate()");
        m_osc.sendOscUpdate();
    }

    void sendOscExiting()
    {
        qDebug("CarlaClient::sendOscExiting()");
        m_osc.sendOscExiting();
    }

#ifdef BUILD_BRIDGE_PLUGIN
    void sendOscBridgeUpdate()
    {
        qDebug("CarlaClient::sendOscBridgeUpdate()");
        m_osc.sendOscBridgeUpdate();
    }
#endif

#ifdef BRIDGE_LV2
    void sendOscLv2TransferAtom(const char* const type, const char* const value)
    {
        qDebug("CarlaClient::sendOscLv2TransferAtom(\"%s\", \"%s\")", type, value);
        m_osc.sendOscLv2TransferAtom(type, value);
    }

    void sendOscLv2TransferEvent(const char* const type, const char* const value)
    {
        qDebug("CarlaClient::sendOscLv2TransferEvent(\"%s\", \"%s\")", type, value);
        m_osc.sendOscLv2TransferEvent(type, value);
    }
#endif

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
protected:
    bool libOpen(const char* const filename)
    {
        Q_ASSERT(filename);

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
    CarlaOsc m_osc;
    CarlaToolkit* const m_toolkit;

    struct {
        Message data[MAX_BRIDGE_MESSAGES];
        QMutex lock;
    } m_messages;

#ifdef BUILD_BRIDGE_UI
    char* m_filename;
    void* m_lib;
#endif
};

/**@}*/

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_CLIENT_H

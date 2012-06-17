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

#ifndef CARLA_BRIDGE_H
#define CARLA_BRIDGE_H

#include "carla_includes.h"

#ifdef BUILD_BRIDGE_UI
#include "carla_lib_includes.h"
#endif

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <QtCore/QMutex>

void toolkit_init();
void toolkit_loop();
void toolkit_quit();
void toolkit_window_show();
void toolkit_window_hide();
void toolkit_window_resize(int width, int height);

// -------------------------------------------------------------------------

#define MAX_BRIDGE_MESSAGES 256

enum BridgeMessageType {
    BRIDGE_MESSAGE_NULL         = 0,
    BRIDGE_MESSAGE_PARAMETER    = 1, // index, 0, value
    BRIDGE_MESSAGE_PROGRAM      = 2, // index, 0, 0
    BRIDGE_MESSAGE_MIDI_PROGRAM = 3, // bank, program, 0
    BRIDGE_MESSAGE_NOTE_ON      = 4, // note, velocity, 0
    BRIDGE_MESSAGE_NOTE_OFF     = 5, // note, 0, 0
    BRIDGE_MESSAGE_SHOW_GUI     = 6, // show, 0, 0
    BRIDGE_MESSAGE_RESIZE_GUI   = 7, // width, height, 0
    BRIDGE_MESSAGE_QUIT         = 8
};

struct QuequeBridgeMessage {
    BridgeMessageType type;
    int value1;
    int value2;
    double value3;
};

// -------------------------------------------------------------------------

class ClientData
{
public:
    ClientData(const char* ui_title)
    {
        m_title = strdup(ui_title);

        for (unsigned int i=0; i < MAX_BRIDGE_MESSAGES; i++)
        {
            QuequeBridgeMessages[i].type   = BRIDGE_MESSAGE_NULL;
            QuequeBridgeMessages[i].value1 = 0;
            QuequeBridgeMessages[i].value2 = 0;
            QuequeBridgeMessages[i].value3 = 0.0;
        }

#ifdef BUILD_BRIDGE_UI
        m_filename = nullptr;
        m_lib = nullptr;
#endif
    }

    virtual ~ClientData()
    {
        free(m_title);

#ifdef BUILD_BRIDGE_UI
        if (m_filename)
            free(m_filename);
#endif
    }

    void queque_message(BridgeMessageType type, int value1, int value2, double value3)
    {
        m_lock.lock();
        for (unsigned int i=0; i<MAX_BRIDGE_MESSAGES; i++)
        {
            if (QuequeBridgeMessages[i].type == BRIDGE_MESSAGE_NULL)
            {
                QuequeBridgeMessages[i].type   = type;
                QuequeBridgeMessages[i].value1 = value1;
                QuequeBridgeMessages[i].value2 = value2;
                QuequeBridgeMessages[i].value3 = value3;
                break;
            }
        }
        m_lock.unlock();
    }

    bool run_messages()
    {
        m_lock.lock();
        for (unsigned int i=0; i < MAX_BRIDGE_MESSAGES; i++)
        {
            if (QuequeBridgeMessages[i].type != BRIDGE_MESSAGE_NULL)
            {
                const QuequeBridgeMessage* const m = &QuequeBridgeMessages[i];

                switch (m->type)
                {
                case BRIDGE_MESSAGE_PARAMETER:
                    set_parameter(m->value1, m->value3);
                    break;
                case BRIDGE_MESSAGE_PROGRAM:
                    set_program(m->value1);
                    break;
                case BRIDGE_MESSAGE_MIDI_PROGRAM:
                    set_midi_program(m->value1, m->value2);
                    break;
                case BRIDGE_MESSAGE_NOTE_ON:
                    note_on(m->value1, m->value2);
                    break;
                case BRIDGE_MESSAGE_NOTE_OFF:
                    note_off(m->value1);
                    break;
                case BRIDGE_MESSAGE_SHOW_GUI:
                    if (m->value1)
                        toolkit_window_show();
                    else
                        toolkit_window_hide();
                    break;
                case BRIDGE_MESSAGE_RESIZE_GUI:
                    toolkit_window_resize(m->value1, m->value2);
                    break;
                case BRIDGE_MESSAGE_QUIT:
                    toolkit_quit();
                    m_lock.unlock();
                    return false;
                default:
                    break;
                }

                QuequeBridgeMessages[i].type = BRIDGE_MESSAGE_NULL;
            }
            else
                break;
        }
        m_lock.unlock();
        return true;
    }

    const char* get_title() const
    {
        return m_title;
    }

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    // initialization
    virtual bool init(const char*, const char*) = 0;
    virtual void close() = 0;
#endif

    // processing
    virtual void set_parameter(int32_t rindex, double value) = 0;
    virtual void set_program(uint32_t index) = 0;
    virtual void set_midi_program(uint32_t bank, uint32_t program) = 0;
    virtual void note_on(uint8_t note, uint8_t velocity) = 0;
    virtual void note_off(uint8_t note) = 0;

#ifdef BUILD_BRIDGE_PLUGIN
    // plugin
    virtual void save_now() = 0;
    virtual void set_chunk_data(const char* string_data) = 0;
#else
    // gui
    virtual void* get_widget() const = 0;
    virtual bool is_resizable() const = 0;
    virtual bool needs_reparent() const = 0;
#endif

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    bool lib_open(const char* filename)
    {
        m_lib = ::lib_open(filename);
        m_filename = strdup(filename);
        return bool(m_lib);
    }

    bool lib_close()
    {
        if (m_lib)
            return ::lib_close(m_lib);
        return false;
    }

    void* lib_symbol(const char* symbol)
    {
        if (m_lib)
            return ::lib_symbol(m_lib, symbol);
        return nullptr;
    }

    const char* lib_error()
    {
        return ::lib_error(m_filename ? m_filename : "");
    }
#endif

    // ---------------------------------------------------------------------

private:
    char* m_title;
    QMutex m_lock;
    QuequeBridgeMessage QuequeBridgeMessages[MAX_BRIDGE_MESSAGES];

#ifdef BUILD_BRIDGE_UI
    char* m_filename;
    void* m_lib;
#endif
};

// -------------------------------------------------------------------------

extern ClientData* client;

#endif // CARLA_BRIDGE_H

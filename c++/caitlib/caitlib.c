/*
 * Caitlib
 * Copyright (C) 2007 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "caitlib.h"
#include "list.h"
#include "memory_atomic.h"

#define MAX_EVENT_DATA_SIZE          100
#define MIN_PREALLOCATED_EVENT_COUNT 100
#define MAX_PREALLOCATED_EVENT_COUNT 1000

//#define USE_LIST_HEAD_OUTS // incomplete

// ------------------------------------------------------------------------------------------

typedef struct list_head ListHead;
typedef pthread_mutex_t  Mutex;

typedef struct _RawMidiEvent {
    ListHead siblings;

    jack_midi_data_t data[MAX_EVENT_DATA_SIZE];
    size_t           dataSize;
    jack_nframes_t   time;
} RawMidiEvent;

typedef struct _CaitlibOutPort {
#ifdef USE_LIST_HEAD_OUTS
    ListHead siblings;
#endif
    uint32_t id;
    ListHead queue;
    jack_port_t* port;
} CaitlibOutPort;

typedef struct _CaitlibInstance {
    bool doProcess, wantEvents;

    jack_client_t* client;
    jack_port_t*   midiIn;

#ifdef USE_LIST_HEAD_OUTS
    ListHead outPorts;
#else
    uint32_t         midiOutCount;
    CaitlibOutPort** midiOuts;
#endif

    ListHead inQueue;
    ListHead inQueuePendingRT;
    Mutex    mutex;

    rtsafe_memory_pool_handle mempool;
} CaitlibInstance;

// ------------------------------------------------------------------------------------------

#define handlePtr ((CaitlibInstance*)ptr)

static
int jack_process(jack_nframes_t nframes, void* ptr)
{
    if (! handlePtr->doProcess)
        return 0;

    void* portBuffer;
    RawMidiEvent* eventPtr;

    jack_position_t transportPos;
    jack_transport_state_t transportState = jack_transport_query(handlePtr->client, &transportPos);

    if (transportPos.unique_1 != transportPos.unique_2)
        transportPos.frame = 0;

    // MIDI In
    if (handlePtr->wantEvents)
    {
        jack_midi_event_t inEvent;
        jack_nframes_t    inEventCount;

        portBuffer   = jack_port_get_buffer(handlePtr->midiIn, nframes);
        inEventCount = jack_midi_get_event_count(portBuffer);

        for (jack_nframes_t i = 0 ; i < inEventCount; i++)
        {
            if (jack_midi_event_get(&inEvent, portBuffer, i) != 0)
                break;

            if (inEvent.size > MAX_EVENT_DATA_SIZE)
                continue;

            /* allocate memory for buffer copy */
            eventPtr = rtsafe_memory_pool_allocate(handlePtr->mempool);
            if (eventPtr == NULL)
            {
                //LOG_ERROR("Ignored midi event with size %u because memory allocation failed.", (unsigned int)inEvent.size);
                continue;
            }

            /* copy buffer data */
            memcpy(eventPtr->data, inEvent.buffer, inEvent.size);
            eventPtr->dataSize = inEvent.size;
            eventPtr->time     = transportPos.frame + inEvent.time;

            /* Add event buffer to inQueuePendingRT list */
            list_add(&eventPtr->siblings, &handlePtr->inQueuePendingRT);
        }
    }

    if (pthread_mutex_trylock(&handlePtr->mutex) != 0)
        return 0;

    if (handlePtr->wantEvents)
        list_splice_init(&handlePtr->inQueuePendingRT, &handlePtr->inQueue);

#ifdef USE_LIST_HEAD_OUTS
    if (transportState == JackTransportRolling)
    {
    }
#else
    // MIDI Out
    if (handlePtr->midiOutCount > 0 && transportState == JackTransportRolling)
    {
        ListHead* entryPtr;
        CaitlibOutPort* outPortPtr;

        for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
        {
            outPortPtr = handlePtr->midiOuts[i];
            portBuffer = jack_port_get_buffer(outPortPtr->port, nframes);
            jack_midi_clear_buffer(portBuffer);

            list_for_each(entryPtr, &outPortPtr->queue)
            {
                eventPtr = list_entry(entryPtr, RawMidiEvent, siblings);

                if (transportPos.frame > eventPtr->time || transportPos.frame + nframes <= eventPtr->time)
                    continue;

                if (jack_midi_event_write(portBuffer, eventPtr->time - transportPos.frame, eventPtr->data, eventPtr->dataSize) != 0)
                    break;
            }
        }
    }
#endif

    pthread_mutex_unlock(&handlePtr->mutex);

    return 0;
}

#undef handlePtr

// ------------------------------------------------------------------------------------------
// Initialization

CaitlibHandle caitlib_init(const char* instanceName)
{
    CaitlibInstance* handlePtr = (CaitlibInstance*)malloc(sizeof(CaitlibInstance));

    if (handlePtr == NULL)
        goto fail;

    handlePtr->doProcess  = true;
    handlePtr->wantEvents = false;

#ifdef USE_LIST_HEAD_OUTS
    INIT_LIST_HEAD(&handlePtr->outPorts);
#else
    handlePtr->midiOuts     = NULL;
    handlePtr->midiOutCount = 0;
#endif

    INIT_LIST_HEAD(&handlePtr->inQueue);
    INIT_LIST_HEAD(&handlePtr->inQueuePendingRT);

    if (! rtsafe_memory_pool_create(sizeof(RawMidiEvent), MIN_PREALLOCATED_EVENT_COUNT, MAX_PREALLOCATED_EVENT_COUNT, &handlePtr->mempool))
        goto fail_free;

    pthread_mutex_init(&handlePtr->mutex, NULL);

    handlePtr->client = jack_client_open(instanceName, JackNullOption, 0);

    if (handlePtr->client == NULL)
        goto fail_destroyMutex;

    handlePtr->midiIn = jack_port_register(handlePtr->client, "midi-in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    if (handlePtr->midiIn == NULL)
        goto fail_closeJack;

    if (jack_set_process_callback(handlePtr->client, jack_process, handlePtr) != 0)
        goto fail_closeJack;

    if (jack_activate(handlePtr->client) != 0)
        goto fail_closeJack;

    return handlePtr;

fail_closeJack:
    jack_client_close(handlePtr->client);

fail_destroyMutex:
    pthread_mutex_destroy(&handlePtr->mutex);

//fail_destroy_pool:
    rtsafe_memory_pool_destroy(handlePtr->mempool);

fail_free:
    free(handlePtr);

fail:
    return NULL;
}

#define handlePtr ((CaitlibInstance*)handle)

void caitlib_close(CaitlibHandle handle)
{
    // wait for jack processing to end
    handlePtr->doProcess = false;
    pthread_mutex_lock(&handlePtr->mutex);

    if (handlePtr->client)
    {
#ifdef USE_LIST_HEAD_OUTS
        ListHead* entryPtr;
        CaitlibOutPort* outPortPtr;
#endif

        jack_deactivate(handlePtr->client);
        jack_port_unregister(handlePtr->client, handlePtr->midiIn);

#ifdef USE_LIST_HEAD_OUTS
        list_for_each(entryPtr, &handlePtr->outPorts)
        {
            outPortPtr = list_entry(entryPtr, CaitlibOutPort, siblings);
            jack_port_unregister(handlePtr->client, outPortPtr->port);
        }
#else
        for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
        {
            jack_port_unregister(handlePtr->client, handlePtr->midiOuts[i]->port);
            free(handlePtr->midiOuts[i]);
        }
#endif

        jack_client_close(handlePtr->client);
    }

    pthread_mutex_unlock(&handlePtr->mutex);
    pthread_mutex_destroy(&handlePtr->mutex);

    rtsafe_memory_pool_destroy(handlePtr->mempool);

    free(handlePtr);
}

uint32_t caitlib_create_port(CaitlibHandle handle, const char* portName)
{
    uint32_t nextId = 0;

    // wait for jack processing to end
    handlePtr->doProcess = false;
    pthread_mutex_lock(&handlePtr->mutex);

    // re-allocate pointers (midiOutCount + 1) and find next available ID
    {
#ifdef USE_LIST_HEAD_OUTS
        ListHead* entryPtr;
        CaitlibOutPort* outPortPtr;

        list_for_each(entryPtr, &handlePtr->outPorts)
        {
            outPortPtr = list_entry(entryPtr, CaitlibOutPort, siblings);

            if (outPortPtr->id == nextId)
            {
                nextId++;
                continue;
            }
        }
#else
        CaitlibOutPort* oldMidiOuts[handlePtr->midiOutCount];

        for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
        {
            oldMidiOuts[i] = handlePtr->midiOuts[i];

            if (handlePtr->midiOuts[i]->id == nextId)
                nextId++;
        }

        if (handlePtr->midiOuts)
            free(handlePtr->midiOuts);

        handlePtr->midiOuts = (CaitlibOutPort**)malloc(sizeof(CaitlibOutPort*) * (handlePtr->midiOutCount+1));

        for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
            handlePtr->midiOuts[i] = oldMidiOuts[i];
#endif
    }

    // we can continue normal operation now
    pthread_mutex_unlock(&handlePtr->mutex);
    handlePtr->doProcess = true;

#ifdef USE_LIST_HEAD_OUTS
#else
    // create new port
    {
        CaitlibOutPort* newPort = (CaitlibOutPort*)malloc(sizeof(CaitlibOutPort));

        newPort->id   = nextId;
        newPort->port = jack_port_register(handlePtr->client, portName, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        INIT_LIST_HEAD(&newPort->queue);

        handlePtr->midiOuts[handlePtr->midiOutCount++] = newPort;
    }
#endif

    return nextId;
}

void caitlib_destroy_port(CaitlibHandle handle, uint32_t port)
{
    // wait for jack processing to end
    handlePtr->doProcess = false;
    pthread_mutex_lock(&handlePtr->mutex);

#ifdef USE_LIST_HEAD_OUTS
#else
    // re-allocate pointers (midiOutCount - 1)
    {
        CaitlibOutPort* oldMidiOuts[handlePtr->midiOutCount];

        for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
            oldMidiOuts[i] = handlePtr->midiOuts[i];

        if (handlePtr->midiOuts)
            free(handlePtr->midiOuts);

        if (handlePtr->midiOutCount == 1)
        {
            handlePtr->midiOuts = NULL;
        }
        else
        {
            handlePtr->midiOuts = (CaitlibOutPort**)malloc(sizeof(CaitlibOutPort*) * (handlePtr->midiOutCount-1));

            for (uint32_t i = 0, j = 0; i < handlePtr->midiOutCount; i++)
            {
                if (oldMidiOuts[i]->id != port)
                    handlePtr->midiOuts[j++] = oldMidiOuts[i];
            }
        }

        handlePtr->midiOutCount--;
    }
#endif

    pthread_mutex_unlock(&handlePtr->mutex);
    handlePtr->doProcess = true;

    return;
    (void)port;
}

// ------------------------------------------------------------------------------------------
// Input

void caitlib_want_events(CaitlibHandle handle, bool yesNo)
{
    pthread_mutex_lock(&handlePtr->mutex);
    handlePtr->wantEvents = yesNo;
    pthread_mutex_unlock(&handlePtr->mutex);
}

MidiEvent* caitlib_get_event(CaitlibHandle handle)
{
    static MidiEvent midiEvent;

    ListHead* nodePtr;
    RawMidiEvent* rawEventPtr;

    pthread_mutex_lock(&handlePtr->mutex);
    if (list_empty(&handlePtr->inQueue))
    {
        pthread_mutex_unlock(&handlePtr->mutex);
        return NULL;
    }

    nodePtr = handlePtr->inQueue.prev;

    list_del(nodePtr);

    rawEventPtr = list_entry(nodePtr, RawMidiEvent, siblings);

    pthread_mutex_unlock(&handlePtr->mutex);

    // note off
    if (rawEventPtr->dataSize == 3 && (rawEventPtr->data[0] & 0xF0) == 0x80)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_NOTE_OFF;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.note.note     = rawEventPtr->data[1];
        midiEvent.data.note.velocity = rawEventPtr->data[2];
    }

    // note on
    else if (rawEventPtr->dataSize == 3 && (rawEventPtr->data[0] & 0xF0) == 0x90)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_NOTE_ON;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.note.note     = rawEventPtr->data[1];
        midiEvent.data.note.velocity = rawEventPtr->data[2];
    }

    // aftertouch
    else if (rawEventPtr->dataSize == 3 && (rawEventPtr->data[0] & 0xF0) == 0xA0)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_AFTER_TOUCH;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.note.note     = rawEventPtr->data[1];
        midiEvent.data.note.velocity = rawEventPtr->data[2];
    }

    // control
    else if (rawEventPtr->dataSize == 3 && (rawEventPtr->data[0] & 0xF0) == 0xB0)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_CONTROL;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.control.controller = rawEventPtr->data[1];
        midiEvent.data.control.value      = rawEventPtr->data[2];
    }

    // program
    else if (rawEventPtr->dataSize == 2 && (rawEventPtr->data[0] & 0xF0) == 0xC0)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_PROGRAM;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.program.value = rawEventPtr->data[1];
    }

    // channel pressure
    else if (rawEventPtr->dataSize == 2 && (rawEventPtr->data[0] & 0xF0) == 0xD0)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_CHANNEL_PRESSURE;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.pressure.value = rawEventPtr->data[1];
    }

    // pitch wheel
    else if (rawEventPtr->dataSize == 3 && (rawEventPtr->data[0] & 0xF0) == 0xE0)
    {
        midiEvent.type    = MIDI_EVENT_TYPE_PITCH_WHEEL;
        midiEvent.channel = rawEventPtr->data[0] & 0x0F;
        midiEvent.data.pitchwheel.value = ((rawEventPtr->data[2] << 7) | rawEventPtr->data[1]) - 8192;
    }

    else
    {
        return NULL;
    }

    rtsafe_memory_pool_deallocate(handlePtr->mempool, rawEventPtr);

    return &midiEvent;
}

// ------------------------------------------------------------------------------------------
// Output (utils)

void caitlib_midi_encode_control(RawMidiEvent* eventPtr, uint8_t channel, uint8_t control, uint8_t value)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_CONTROL | (channel & 0x0F);
    eventPtr->data[1]  = control;
    eventPtr->data[2]  = value;
    eventPtr->dataSize = 3;
}

void caitlib_midi_encode_note_on(RawMidiEvent* eventPtr, uint8_t channel, uint8_t note, uint8_t velocity)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_NOTE_ON | (channel & 0x0F);
    eventPtr->data[1]  = note;
    eventPtr->data[2]  = velocity;
    eventPtr->dataSize = 3;
}

void caitlib_midi_encode_note_off(RawMidiEvent* eventPtr, uint8_t channel, uint8_t note, uint8_t velocity)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_NOTE_OFF | (channel & 0x0F);
    eventPtr->data[1]  = note;
    eventPtr->data[2]  = velocity;
    eventPtr->dataSize = 3;
}

void caitlib_midi_encode_aftertouch(RawMidiEvent* eventPtr, uint8_t channel, uint8_t note, uint8_t pressure)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_AFTER_TOUCH | (channel & 0x0F);
    eventPtr->data[1]  = note;
    eventPtr->data[2]  = pressure;
    eventPtr->dataSize = 3;
}

void caitlib_midi_encode_channel_pressure(RawMidiEvent* eventPtr, uint8_t channel, uint8_t pressure)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_CHANNEL_PRESSURE | (channel & 0x0F);
    eventPtr->data[1]  = pressure;
    eventPtr->dataSize = 2;
}

void caitlib_midi_encode_program(RawMidiEvent* eventPtr, uint8_t channel, uint8_t program)
{
    eventPtr->data[0]  = MIDI_EVENT_TYPE_PROGRAM | (channel & 0x0F);
    eventPtr->data[1]  = program;
    eventPtr->dataSize = 2;
}

void caitlib_midi_encode_pitchwheel(RawMidiEvent* eventPtr, uint8_t channel, int16_t pitchwheel)
{
    pitchwheel += 8192;
    eventPtr->data[0]  = MIDI_EVENT_TYPE_PITCH_WHEEL | (channel & 0x0F);
    eventPtr->data[1]  = pitchwheel & 0x7F;
    eventPtr->data[2]  = pitchwheel >> 7;
    eventPtr->dataSize = 3;
}

// ------------------------------------------------------------------------------------------
// Output

void caitlib_put_event(CaitlibHandle handle, uint32_t port, const MidiEvent* event)
{
#ifdef USE_LIST_HEAD_OUTS
    ListHead* entryPtr;
#endif
    RawMidiEvent* eventPtr;
    CaitlibOutPort* outPortPtr;
    bool portFound = false;

    eventPtr = rtsafe_memory_pool_allocate_sleepy(handlePtr->mempool);
    eventPtr->time = event->time;

    switch (event->type)
    {
    case MIDI_EVENT_TYPE_CONTROL:
        caitlib_midi_encode_control(eventPtr, event->channel, event->data.control.controller, event->data.control.value);
        break;
    case MIDI_EVENT_TYPE_NOTE_ON:
        caitlib_midi_encode_note_on(eventPtr, event->channel, event->data.note.note, event->data.note.velocity);
        break;
    case MIDI_EVENT_TYPE_NOTE_OFF:
        caitlib_midi_encode_note_off(eventPtr, event->channel, event->data.note.note, event->data.note.velocity);
        break;
    case MIDI_EVENT_TYPE_AFTER_TOUCH:
        caitlib_midi_encode_aftertouch(eventPtr, event->channel, event->data.note.note, event->data.note.velocity);
        break;
    case MIDI_EVENT_TYPE_CHANNEL_PRESSURE:
        caitlib_midi_encode_channel_pressure(eventPtr, event->channel, event->data.pressure.value);
        break;
    case MIDI_EVENT_TYPE_PROGRAM:
        caitlib_midi_encode_program(eventPtr, event->channel, event->data.program.value);
        break;
    case MIDI_EVENT_TYPE_PITCH_WHEEL:
        caitlib_midi_encode_pitchwheel(eventPtr, event->channel, event->data.pitchwheel.value);
        break;
    default:
        return;
    }

    pthread_mutex_lock(&handlePtr->mutex);

#ifdef USE_LIST_HEAD_OUTS
    list_for_each(entryPtr, &handlePtr->outPorts)
    {
        outPortPtr = list_entry(entryPtr, CaitlibOutPort, siblings);

        if (outPortPtr->id == port)
        {
            list_add_tail(&eventPtr->siblings, &outPortPtr->queue);
            break;
        }
    }
#else
    for (uint32_t i = 0; i < handlePtr->midiOutCount; i++)
    {
        outPortPtr = handlePtr->midiOuts[i];

        if (outPortPtr->id == port)
        {
            portFound = true;
            list_add_tail(&eventPtr->siblings, &outPortPtr->queue);
            break;
        }
    }
#endif

    if (! portFound)
        printf("caitlib_put_event(%p, %i, %p) - failed to find port", handle, port, event);

    pthread_mutex_unlock(&handlePtr->mutex);
}

void caitlib_put_control(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t control, uint8_t value)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_CONTROL;
    event.channel = channel;
    event.time    = time;
    event.data.control.controller = control;
    event.data.control.value      = value;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_note_on(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t velocity)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_NOTE_ON;
    event.channel = channel;
    event.time    = time;
    event.data.note.note     = note;
    event.data.note.velocity = velocity;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_note_off(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t velocity)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_NOTE_OFF;
    event.channel = channel;
    event.time    = time;
    event.data.note.note     = note;
    event.data.note.velocity = velocity;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_aftertouch(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t pressure)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_AFTER_TOUCH;
    event.channel = channel;
    event.time    = time;
    event.data.note.note     = note;
    event.data.note.velocity = pressure;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_channel_pressure(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t pressure)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_CHANNEL_PRESSURE;
    event.channel = channel;
    event.time    = time;
    event.data.pressure.value = pressure;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_program(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t program)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_PROGRAM;
    event.channel = channel;
    event.time    = time;
    event.data.program.value = program;
    caitlib_put_event(handle, port, &event);
}

void caitlib_put_pitchwheel(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, int16_t value)
{
    MidiEvent event;
    event.type    = MIDI_EVENT_TYPE_PITCH_WHEEL;
    event.channel = channel;
    event.time    = time;
    event.data.pitchwheel.value = value;
    caitlib_put_event(handle, port, &event);
}

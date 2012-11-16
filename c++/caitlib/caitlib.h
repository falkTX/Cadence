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

#ifndef CAITLIB_INCLUDED
#define CAITLIB_INCLUDED

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include <stdint.h>

#ifdef _WIN32
#define CAITLIB_EXPORT __declspec (dllexport)
#else
#define CAITLIB_EXPORT __attribute__ ((visibility("default")))
#endif

/*!
 * @defgroup CaitlibAPI Caitlib API
 *
 * The Caitlib API
 *
 * @{
 */

/*!
 * Native handle for all Caitlib calls.
 */
typedef void* CaitlibHandle;

/*!
 * @defgroup MidiEventType MidiEvent Type
 * @{
 */
#define MIDI_EVENT_TYPE_NULL             0x00 //!< Null Event.
#define MIDI_EVENT_TYPE_NOTE_OFF         0x80 //!< Note-Off Event, uses Note data.
#define MIDI_EVENT_TYPE_NOTE_ON          0x90 //!< Note-On Event, uses Note data.
#define MIDI_EVENT_TYPE_AFTER_TOUCH      0xA0 //!< [Key] AfterTouch Event, uses Note data.
#define MIDI_EVENT_TYPE_CONTROL          0xB0 //!< Control Event, uses Control data.
#define MIDI_EVENT_TYPE_PROGRAM          0xC0 //!< Program Event, uses Program data.
#define MIDI_EVENT_TYPE_CHANNEL_PRESSURE 0xD0 //!< Channel Pressure Event, uses Pressure data.
#define MIDI_EVENT_TYPE_PITCH_WHEEL      0xE0 //!< PitchWheel Event, uses PitchWheel data.
#define MIDI_EVENT_TYPE_TIME             0xF1 //!< Time Event, uses Time Data.
/**@}*/

/*!
 * MidiEvent in Caitlib
 */
typedef struct _MidiEvent
{
    /*!
     * MidiEvent Type
     */
    uint16_t type;

    /*!
     * MidiEvent Channel (0 - 16)
     */
    uint8_t channel;

    /*!
     * MidiEvent Data (values depend on type).
     * \note Time event types ignore channel value.
     */
    union MidiEventData {
        struct MidiEventControl {
            uint8_t controller;
            uint8_t value;
        } control;

        struct MidiEventNote {
            uint8_t note;
            uint8_t velocity;
        } note;

        struct MidiEventPressure {
            uint8_t value;
        } pressure;

        struct MidiEventProgram {
            uint8_t value;
        } program;

        struct MidiEventPitchWheel {
            int16_t value;
        } pitchwheel;

        struct MidiEventTime {
            double  bpm;
            uint8_t sigNum;
            uint8_t sigDenum;
        } time;

#ifndef DOXYGEN
        // padding for future events
        struct _MidiEventPadding {
            uint8_t pad[32];
        } __padding;
#endif
    } data;

    /*!
     * MidiEvent Time (in frames)
     */
    uint32_t time;

} MidiEvent;

// ------------------------------------------------------------------------------------------

/*!
 * @defgroup Initialization
 *
 * Functions for initialization and destruction of Caitlib instances.
 * @{
 */

/*!
 * Initialize a new Caitlib instance with name \a instanceName.\n
 * Must be closed when no longer needed with caitlib_close().
 *
 * \note MIDI Input is disabled by default, call caitlib_want_events() to enable them.
 * \note There are no MIDI Output ports by default, call caitlib_create_port() for that.
 */
CAITLIB_EXPORT
CaitlibHandle caitlib_init(const char* instanceName);

/*!
 * Close a previously opened Caitlib instance.\n
 */
CAITLIB_EXPORT
void caitlib_close(CaitlibHandle handle);

/*!
 * Create a new MIDI Output port with name \a portName.\n
 * The return value is the ID for the port, which must be passed for any functions in the Output group.\n
 * The ID will be >= 0 for a valid port, or -1 if an error occurred.
 */
CAITLIB_EXPORT
uint32_t caitlib_create_port(CaitlibHandle handle, const char* portName);

/*!
 * Close a previously opened Caitlib instance.\n
 * There's no need to call this function before caitlib_close().
 */
CAITLIB_EXPORT
void caitlib_destroy_port(CaitlibHandle handle, uint32_t port);

/**@}*/

// ------------------------------------------------------------------------------------------

/*!
 * @defgroup Input
 *
 * Functions for MIDI Input handling.
 * @{
 */

/*!
 * Tell a Caitlib instance wherever if we're interested in MIDI Input.\n
 * By default MIDI Input is disabled.\n
 * It's safe to call this function multiple times during the lifetime of an instance.
 */
CAITLIB_EXPORT
void caitlib_want_events(CaitlibHandle handle, bool yesNo);

/*!
 * Get a MidiEvent from a Caitlib instance buffer.\n
 * When there are no more messages in the buffer, this function returns NULL.
 */
CAITLIB_EXPORT
MidiEvent* caitlib_get_event(CaitlibHandle handle);

/**@}*/

// ------------------------------------------------------------------------------------------

/*!
 * @defgroup Output
 *
 * Functions for putting data into Caitlib instances (MIDI Output).
 * @{
 */

/*!
 * Put a MIDI event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_event(CaitlibHandle handle, uint32_t port, const MidiEvent* event);

/*!
 * Put a MIDI Control event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_control(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t controller, uint8_t value);

/*!
 * Put a MIDI Note On event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_note_on(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t velocity);

/*!
 * Put a MIDI Note Off event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_note_off(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t velocity);

/*!
 * Put a MIDI AfterTouch event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_aftertouch(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t note, uint8_t pressure);

/*!
 * Put a MIDI Channel Pressure event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_channel_pressure(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t pressure);

/*!
 * Put a MIDI Program event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_program(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, uint8_t program);

/*!
 * Put a MIDI PitchWheel event into a Caitlib instance port.
 */
CAITLIB_EXPORT
void caitlib_put_pitchwheel(CaitlibHandle handle, uint32_t port, uint32_t time, uint8_t channel, int16_t value);

/*!
 * Put a MIDI Time event into a Caitlib instance.
 */
CAITLIB_EXPORT
void caitlib_put_time(CaitlibHandle handle, uint32_t time, double bpm, uint8_t sigNum, uint8_t sigDenum);

/**@}*/

// ------------------------------------------------------------------------------------------

/**@}*/

#ifdef __cplusplus
}
#endif

#endif // CAITLIB_INCLUDED

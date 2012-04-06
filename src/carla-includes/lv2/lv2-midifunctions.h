/****************************************************************************
    
    lv2-midifunctions.h - support file for using MIDI in LV2 plugins
    
    Copyright (C) 2006-2007  Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

/** @file
    This file contains static helper functions for the LV2 MIDI datatype
    extension. 
*/

#ifndef LV2_MIDIFUNCTIONS
#define LV2_MIDIFUNCTIONS

#include <string.h>

#include "lv2-midiport.h"


/** This structure contains information about a MIDI port buffer, the 
    current period size, and the position in the MIDI data buffer that
    we are currently reading from or writing to. It needs to be recreated
    or at least reinitialised every process() call. */
typedef struct {

  /** The MIDI port structure that we want to read or write. */
  LV2_MIDI* midi;
  
  /** The number of frames in this process cycle. */
  uint32_t frame_count;
  
  /** The current position in the data buffer. Should be initialised to 0. */
  uint32_t position;

} LV2_MIDIState;


/** This function reads one event from the port associated with the @c state
    parameter and writes its timestamp, size and a pointer to its data bytes
    into the parameters @c timestamp, @c size and @c data, respectively.
    It does not advance the read position in the MIDI data buffer, two 
    subsequent calls to lv2midi_get_event() will read the same event.
    
    The function returns the timestamp for the read event, or the @c frame_count
    member of @c state if there are no more events in the buffer. */
static double lv2midi_get_event(LV2_MIDIState* state,
                                double* timestamp, 
                                uint32_t* size, 
                                unsigned char** data) {
  
  if (state->position >= state->midi->size) {
    state->position = state->midi->size;
    *timestamp = state->frame_count;
    *size = 0;
    *data = NULL;
    return *timestamp;
  }
  
  *timestamp = *(double*)(state->midi->data + state->position);
  *size = *(uint32_t*)(state->midi->data + state->position + sizeof(double));
  *data = state->midi->data + state->position + 
    sizeof(double) + sizeof(uint32_t);
  return *timestamp;
}


/** This function advances the read/write position in @c state to the next 
    event and returns its timestamp, or the @c frame_count member of @c state
    is there are no more events. */
static double lv2midi_step(LV2_MIDIState* state) {

  if (state->position + sizeof(double) + sizeof(uint32_t) >= state->midi->size) {
    state->position = state->midi->size;
    return state->frame_count;
  }
  
  state->position += sizeof(double);
  uint32_t size = *(uint32_t*)(state->midi->data + state->position);
  state->position += sizeof(uint32_t);
  state->position += size;
  
  if (state->position >= state->midi->size)
    return state->frame_count;
  
  return *(double*)(state->midi->data + state->position);
}


/** This function writes one MIDI event to the port buffer associated with
    @c state. It returns 0 when the event was written successfully to the
    buffer, and -1 when there was not enough room. The read/write position
    is advanced automatically. */
static int lv2midi_put_event(LV2_MIDIState* state, 
                             double timestamp,
                             uint32_t size,
                             const unsigned char* data) {

  if (state->midi->capacity - state->midi->size < 
      sizeof(double) + sizeof(uint32_t) + size)
    return -1;
  
  *(double*)(state->midi->data + state->midi->size) = timestamp;
  state->midi->size += sizeof(double);
  *(uint32_t*)(state->midi->data + state->midi->size) = size;
  state->midi->size += sizeof(uint32_t);
  memcpy(state->midi->data + state->midi->size, data, size);
  state->midi->size += size;
  
  ++state->midi->event_count;
  
  return 0;
}


#endif


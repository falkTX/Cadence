/*
  Copyright 2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file midi.h
   C definitions for the LV2 MIDI extension <http://lv2plug.in/ns/ext/midi>.
*/

#ifndef LV2_MIDI_H
#define LV2_MIDI_H

#define LV2_MIDI_URI "http://lv2plug.in/ns/ext/midi"

#define LV2_MIDI__ActiveSense      LV2_MIDI_URI "#ActiveSense"
#define LV2_MIDI__Aftertouch       LV2_MIDI_URI "#Aftertouch"
#define LV2_MIDI__Bender           LV2_MIDI_URI "#Bender"
#define LV2_MIDI__ChannelPressure  LV2_MIDI_URI "#ChannelPressure"
#define LV2_MIDI__Chunk            LV2_MIDI_URI "#Chunk"
#define LV2_MIDI__Clock            LV2_MIDI_URI "#Clock"
#define LV2_MIDI__Continue         LV2_MIDI_URI "#Continue"
#define LV2_MIDI__Controller       LV2_MIDI_URI "#Controller"
#define LV2_MIDI__MidiEvent        LV2_MIDI_URI "#MidiEvent"
#define LV2_MIDI__NoteOff          LV2_MIDI_URI "#NoteOff"
#define LV2_MIDI__NoteOn           LV2_MIDI_URI "#NoteOn"
#define LV2_MIDI__ProgramChange    LV2_MIDI_URI "#ProgramChange"
#define LV2_MIDI__QuarterFrame     LV2_MIDI_URI "#QuarterFrame"
#define LV2_MIDI__Reset            LV2_MIDI_URI "#Reset"
#define LV2_MIDI__SongPosition     LV2_MIDI_URI "#SongPosition"
#define LV2_MIDI__SongSelect       LV2_MIDI_URI "#SongSelect"
#define LV2_MIDI__Start            LV2_MIDI_URI "#Start"
#define LV2_MIDI__Stop             LV2_MIDI_URI "#Stop"
#define LV2_MIDI__SystemCommon     LV2_MIDI_URI "#SystemCommon"
#define LV2_MIDI__SystemExclusive  LV2_MIDI_URI "#SystemExclusive"
#define LV2_MIDI__SystemMessage    LV2_MIDI_URI "#SystemMessage"
#define LV2_MIDI__SystemRealtime   LV2_MIDI_URI "#SystemRealtime"
#define LV2_MIDI__Tick             LV2_MIDI_URI "#Tick"
#define LV2_MIDI__TuneRequest      LV2_MIDI_URI "#TuneRequest"
#define LV2_MIDI__VoiceMessage     LV2_MIDI_URI "#VoiceMessage"
#define LV2_MIDI__benderValue      LV2_MIDI_URI "#benderValue"
#define LV2_MIDI__byteNumber       LV2_MIDI_URI "#byteNumber"
#define LV2_MIDI__chunk            LV2_MIDI_URI "#chunk"
#define LV2_MIDI__controllerNumber LV2_MIDI_URI "#controllerNumber"
#define LV2_MIDI__controllerValue  LV2_MIDI_URI "#controllerValue"
#define LV2_MIDI__noteNumber       LV2_MIDI_URI "#noteNumber"
#define LV2_MIDI__pressure         LV2_MIDI_URI "#pressure"
#define LV2_MIDI__programNumber    LV2_MIDI_URI "#programNumber"
#define LV2_MIDI__property         LV2_MIDI_URI "#property"
#define LV2_MIDI__songNumber       LV2_MIDI_URI "#songNumber"
#define LV2_MIDI__songPosition     LV2_MIDI_URI "#songPosition"
#define LV2_MIDI__status           LV2_MIDI_URI "#status"
#define LV2_MIDI__statusMask       LV2_MIDI_URI "#statusMask"
#define LV2_MIDI__velocity         LV2_MIDI_URI "#velocity"

#endif  /* LV2_MIDI_H */

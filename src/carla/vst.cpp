/*
 * JACK Backend code for Carla
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

#include "carla_plugin.h"

#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"

#if VESTIGE_HEADER
#warning Using vestige header
#define kVstVersion 2400
#define effGetPlugCategory 35
#define effSetBlockSizeAndSampleRate 43
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kVstProcessPrecision32 0
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#endif

class VstPlugin : public CarlaPlugin
{
public:
    VstPlugin() : CarlaPlugin()
    {
        qDebug("VstPlugin::VstPlugin()");
        m_type = PLUGIN_VST;

        effect = nullptr;
        events.numEvents = 0;
        events.reserved  = 0;

        // FIXME?
        memset(midi_events, 0, sizeof(VstMidiEvent)*MAX_MIDI_EVENTS);

        for (unsigned short i=0; i<MAX_MIDI_EVENTS; i++)
            events.data[i] = (VstEvent*)&midi_events[i];
    }

    virtual ~VstPlugin()
    {
        qDebug("VstPlugin::~VstPlugin()");

        if (effect)
        {
            // close UI
            if (m_hints & PLUGIN_HAS_GUI)
                effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);

            if (m_active_before)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            }

            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        }
    }

#if 0
    virtual PluginCategory category()
    {
        intptr_t VstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        switch (VstCategory)
        {
        case kPlugCategSynth:
            return PLUGIN_CATEGORY_SYNTH;
        case kPlugCategAnalysis:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategMastering:
            return PLUGIN_CATEGORY_DYNAMICS;
        case kPlugCategRoomFx:
            return PLUGIN_CATEGORY_DELAY;
        case kPlugCategRestoration:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategGenerator:
            return PLUGIN_CATEGORY_SYNTH;
        }

        if (effect->flags & effFlagsIsSynth)
            return PLUGIN_CATEGORY_SYNTH;

        // TODO - try to get category from label
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return effect->uniqueID;
    }

    virtual void get_label(char* buf_str)
    {
        effect->dispatcher(effect, effGetProductString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_maker(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_copyright(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_real_name(char* buf_str)
    {
        effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
    }
#endif

private:
    AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_MIDI_EVENTS];
    } events;
    VstMidiEvent midi_events[MAX_MIDI_EVENTS];
};

short add_plugin_vst(const char* filename, const char* label)
{
    qDebug("add_plugin_vst(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        set_last_error("Not implemented yet");
        id = -1;
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

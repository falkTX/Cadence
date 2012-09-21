/*
 * Carla Native Plugins
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

#include "carla_nativemm.h"

#include <cstring>

class MidiSplitPlugin : public PluginDescriptorClass
{
public:
    MidiSplitPlugin() : PluginDescriptorClass()
    {
    }

    MidiSplitPlugin(PluginDescriptorClass* that) : PluginDescriptorClass(that)
    {
    }

    ~MidiSplitPlugin()
    {
    }

    // -------------------------------------------------------------------

protected:
    PluginDescriptorClass* createMe()
    {
        return new MidiSplitPlugin(this);
    }

    void deleteMe()
    {
        delete this;
    }

    // -------------------------------------------------------------------

    PluginCategory getCategory()
    {
        return PLUGIN_CATEGORY_UTILITY;
    }

    uint32_t getHints()
    {
        return 0;
    }

    const char* getName()
    {
        return "MIDI Split";
    }

    const char* getLabel()
    {
        return "midiSplit";
    }

    const char* getMaker()
    {
        return "falkTX";
    }

    const char* getCopyright()
    {
        return "GNU GPL v2+";
    }

    // -------------------------------------------------------------------

    uint32_t getPortCount()
    {
        return PORT_MAX;
    }

    PortType getPortType(uint32_t)
    {
        return PORT_TYPE_MIDI;
    }

    uint32_t getPortHints(uint32_t index)
    {
        return (index == 0) ? 0 : PORT_HINT_IS_OUTPUT;
    }

    const char* getPortName(const uint32_t index)
    {
        switch (index)
        {
        case PORT_INPUT:
            return "input";
        case PORT_OUTPUT_1:
            return "output-01";
        case PORT_OUTPUT_2:
            return "output-02";
        case PORT_OUTPUT_3:
            return "output-03";
        case PORT_OUTPUT_4:
            return "output-04";
        case PORT_OUTPUT_5:
            return "output-05";
        case PORT_OUTPUT_6:
            return "output-06";
        case PORT_OUTPUT_7:
            return "output-07";
        case PORT_OUTPUT_8:
            return "output-08";
        case PORT_OUTPUT_9:
            return "output-09";
        case PORT_OUTPUT_10:
            return "output-10";
        case PORT_OUTPUT_11:
            return "output-11";
        case PORT_OUTPUT_12:
            return "output-12";
        case PORT_OUTPUT_13:
            return "output-13";
        case PORT_OUTPUT_14:
            return "output-14";
        case PORT_OUTPUT_15:
            return "output-15";
        case PORT_OUTPUT_16:
            return "output-16";
        default:
            return "";
        }
    }

    // -------------------------------------------------------------------

    void activate()
    {
        memset(events, 0, sizeof(MidiEvent) * MAX_MIDI_EVENTS);
    }

    // -------------------------------------------------------------------

    void process(float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        Q_UNUSED(inBuffer);
        Q_UNUSED(outBuffer);
        Q_UNUSED(frames);
    }

    // -------------------------------------------------------------------

private:
    enum Ports {
        PORT_INPUT = 0,
        PORT_OUTPUT_1,
        PORT_OUTPUT_2,
        PORT_OUTPUT_3,
        PORT_OUTPUT_4,
        PORT_OUTPUT_5,
        PORT_OUTPUT_6,
        PORT_OUTPUT_7,
        PORT_OUTPUT_8,
        PORT_OUTPUT_9,
        PORT_OUTPUT_10,
        PORT_OUTPUT_11,
        PORT_OUTPUT_12,
        PORT_OUTPUT_13,
        PORT_OUTPUT_14,
        PORT_OUTPUT_15,
        PORT_OUTPUT_16,
        PORT_MAX
    };

    static const unsigned short MAX_MIDI_EVENTS = 512;

    MidiEvent events[MAX_MIDI_EVENTS];
};

static MidiSplitPlugin midiSplitPlugin;

CARLA_REGISTER_NATIVE_PLUGIN_MM(midiSplit, midiSplitPlugin)

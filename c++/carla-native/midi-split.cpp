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

#include "carla_native.hpp"

class MidiSplitPlugin : public PluginDescriptorClass
{
public:
    MidiSplitPlugin(const HostDescriptor* host)
        : PluginDescriptorClass(host)
    {
    }

    ~MidiSplitPlugin()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        memset(events, 0, sizeof(MidiEvent) * MAX_MIDI_EVENTS);
    }

    void process(float**, float**, uint32_t, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        MidiEvent midiEvent;

        for (uint32_t i=0; i < midiEventCount; i++)
        {
            memcpy(&midiEvent, &midiEvents[i], sizeof(MidiEvent));

            uint8_t status  = midiEvent.data[0];
            uint8_t channel = status & 0x0F;

            CARLA_ASSERT(channel < 16);

            if (channel >= 16)
                continue;

            status -= channel;

            midiEvent.port    = channel;
            midiEvent.data[0] = status;

            writeMidiEvent(&midiEvent);
        }
    }

    // -------------------------------------------------------------------

private:
    static const unsigned short MAX_MIDI_EVENTS = 512;
    MidiEvent events[MAX_MIDI_EVENTS];

    PluginDescriptorClassEND(MidiSplitPlugin)
};

// -----------------------------------------------------------------------

static PluginDescriptor midiSplitDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ 0x0,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 16,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Split",
    /* label     */ "midiSplit",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(MidiSplitPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiSplit()
{
    carla_register_native_plugin(&midiSplitDesc);
}

// -----------------------------------------------------------------------

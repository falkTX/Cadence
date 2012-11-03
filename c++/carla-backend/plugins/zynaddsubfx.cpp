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

#include "carla_midi.hpp"
#include "carla_nativemm.h"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"

#include <climits>

SYNTH_T* synth = nullptr;

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    ZynAddSubFxPlugin(const PluginDescriptorClass* master)
        : PluginDescriptorClass(master)
    {
        zyn_master = nullptr;

        // first init, ignore
        if (! master)
            return;

        if (s_instanceCount == 0)
        {
            synth = new SYNTH_T;
            synth->buffersize = getBufferSize();
            synth->samplerate = getSampleRate();
            synth->alias();

            config.init();
            config.cfg.SoundBufferSize = getBufferSize();
            config.cfg.SampleRate      = getSampleRate();
            config.cfg.GzipCompression = 0;

            sprng(time(NULL));
            denormalkillbuf = new float [synth->buffersize];
            for (int i=0; i < synth->buffersize; i++)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16;
        }

        zyn_master = new Master();
        zyn_master->defaults();
        zyn_master->swaplr = false;

        // refresh banks
        zyn_master->bank.rescanforbanks();

        for (size_t i=0, size = zyn_master->bank.banks.size(); i < size; i++)
        {
            if (! zyn_master->bank.banks[i].dir.empty())
            {
                zyn_master->bank.loadbank(zyn_master->bank.banks[i].dir);

                for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
                {
                    const std::string insName = zyn_master->bank.getname(instrument);

                    if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                        continue;

                    ProgramInfo pInfo;
                    pInfo.bank = i;
                    pInfo.prog = instrument;
                    pInfo.name = insName;
                    programs.push_back(pInfo);
                }
            }
        }

        s_instanceCount++;
    }

    ~ZynAddSubFxPlugin()
    {
        if (! zyn_master)
            return;

        programs.clear();

        delete zyn_master;

        if (--s_instanceCount == 0)
        {
            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }

    // -------------------------------------------------------------------

protected:
    PluginDescriptorClass* createMe()
    {
        return new ZynAddSubFxPlugin(this);
    }

    void deleteMe()
    {
        delete this;
    }

    // -------------------------------------------------------------------

    PluginCategory getCategory()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    uint32_t getHints()
    {
        return (PLUGIN_IS_SYNTH | PLUGIN_HAS_GUI | PLUGIN_USES_SINGLE_THREAD);
    }

    const char* getName()
    {
        return "ZynAddSubFX";
    }

    const char* getLabel()
    {
        return "zynaddsubfx";
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

    PortType getPortType(uint32_t index)
    {
        switch (index)
        {
        case ZYN_PORT_INPUT_MIDI:
            return PORT_TYPE_MIDI;
        case ZYN_PORT_OUTPUT_AUDIO_L:
        case ZYN_PORT_OUTPUT_AUDIO_R:
            return PORT_TYPE_AUDIO;
        default:
            return PORT_TYPE_PARAMETER;
        }
    }

    uint32_t getPortHints(uint32_t index)
    {
        switch (index)
        {
        case ZYN_PORT_INPUT_MIDI:
            return 0;
        case ZYN_PORT_OUTPUT_AUDIO_L:
        case ZYN_PORT_OUTPUT_AUDIO_R:
            return PORT_HINT_IS_OUTPUT;
        default:
            return (PORT_HINT_IS_ENABLED | PORT_HINT_IS_AUTOMABLE);
        }
    }

    const char* getPortName(const uint32_t index)
    {
        switch (index)
        {
        case ZYN_PORT_INPUT_MIDI:
            return "midi-in";
        case ZYN_PORT_OUTPUT_AUDIO_L:
            return "output-left";
        case ZYN_PORT_OUTPUT_AUDIO_R:
            return "output-right";
        case ZYN_PARAMETER_MASTER:
            return "Master Volume";
        default:
            return "";
        }
    }

    void getParameterRanges(uint32_t index, ParameterRanges* const ranges)
    {
        switch (index)
        {
        case ZYN_PARAMETER_MASTER:
            ranges->min = 0.0f;
            ranges->max = 100.0f;
            ranges->def = 100.0f;
            break;
        }
    }

    double getParameterValue(uint32_t index)
    {
        if (! zyn_master)
            return 0.0;

        switch (index)
        {
        case ZYN_PARAMETER_MASTER:
            return zyn_master->Pvolume;
        default:
            return 0.0;
        }
    }

    const char* getParameterUnit(uint32_t index)
    {
        switch (index)
        {
        case ZYN_PARAMETER_MASTER:
            return "dB - test";
        default:
            return nullptr;
        }
    }

    // -------------------------------------------------------------------

    const MidiProgram* getMidiProgram(uint32_t index)
    {
        if (index >= programs.size())
            return nullptr;

        const ProgramInfo pInfo(programs[index]);

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo.bank;
        midiProgram.program = pInfo.prog;
        midiProgram.name    = pInfo.name.c_str();

        return &midiProgram;
    }

    void setParameterValue(uint32_t index, double value)
    {
        if (! zyn_master)
            return;

        switch (index)
        {
        case ZYN_PARAMETER_MASTER:
            zyn_master->setPvolume((char)rint(value));
            break;
        }
    }

    // -------------------------------------------------------------------

    void setMidiProgram(uint32_t bank, uint32_t program)
    {
        if (! zyn_master)
            return;
        if (bank >= zyn_master->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        const std::string bankdir = zyn_master->bank.banks[bank].dir;

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&zyn_master->mutex);

            zyn_master->bank.loadbank(bankdir);
            zyn_master->bank.loadfromslot(program, zyn_master->part[0]);

            pthread_mutex_unlock(&zyn_master->mutex);
        }
    }

    // -------------------------------------------------------------------

    void activate()
    {
        if (! zyn_master)
            return;

        zyn_master->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    // -------------------------------------------------------------------

    void process(float**, float** outBuffer, uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        if (! zyn_master)
            return;

        unsigned long from_frame       = 0;
        unsigned long event_index      = 0;
        unsigned long next_event_frame = 0;
        unsigned long to_frame = 0;
        pthread_mutex_lock(&zyn_master->mutex);

        do {
            /* Find the time of the next event, if any */
            if (event_index >= midiEventCount)
                next_event_frame = ULONG_MAX;
            else
                next_event_frame = midiEvents[event_index].time;

            /* find the end of the sub-sample to be processed this time round... */
            /* if the next event falls within the desired sample interval... */
            if ((next_event_frame < frames) && (next_event_frame >= to_frame))
                /* set the end to be at that event */
                to_frame = next_event_frame;
            else
                /* ...else go for the whole remaining sample */
                to_frame = frames;

            if (from_frame < to_frame)
            {
                // call master to fill from `from_frame` to `to_frame`:
                zyn_master->GetAudioOutSamples(to_frame - from_frame, (int)getSampleRate(), &outBuffer[0][from_frame], &outBuffer[1][from_frame]);
                // next sub-sample please...
                from_frame = to_frame;
            }

            // Now process any event(s) at the current timing point
            while (event_index < midiEventCount && midiEvents[event_index].time == to_frame)
            {
                uint8_t status  = midiEvents[event_index].data[0];
                uint8_t channel = status & 0x0F;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = midiEvents[event_index].data[1];

                    zyn_master->noteOff(channel, note);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = midiEvents[event_index].data[1];
                    uint8_t velo = midiEvents[event_index].data[2];

                    zyn_master->noteOn(channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = midiEvents[event_index].data[1];
                    uint8_t pressure = midiEvents[event_index].data[2];

                    zyn_master->polyphonicAftertouch(channel, note, pressure);
                }

                event_index++;
            }

        // Keep going until we have the desired total length of sample...
        } while (to_frame < frames);

        pthread_mutex_unlock(&zyn_master->mutex);
    }

    // -------------------------------------------------------------------

private:
    enum Ports {
        ZYN_PORT_INPUT_MIDI = 0,
        ZYN_PORT_OUTPUT_AUDIO_L,
        ZYN_PORT_OUTPUT_AUDIO_R,
        ZYN_PARAMETER_MASTER,
        PORT_MAX
    };

    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        std::string name;
    };
    std::vector<ProgramInfo> programs;

    Master* zyn_master;

    static int s_instanceCount;
};

int ZynAddSubFxPlugin::s_instanceCount = 0;

static ZynAddSubFxPlugin zynAddSubFxPlugin(nullptr);

CARLA_REGISTER_NATIVE_PLUGIN_MM(zynAddSubFx, zynAddSubFxPlugin)

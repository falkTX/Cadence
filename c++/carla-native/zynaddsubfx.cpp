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
#include "carla_native.hpp"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"

#include <climits>

SYNTH_T* synth = nullptr;

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    enum Parameters {
        PARAMETER_MASTER,
        PARAMETER_MAX
    };

    ZynAddSubFxPlugin(const HostDescriptor* host)
        : PluginDescriptorClass(host)
    {
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

        master = new Master();
        master->defaults();
        master->swaplr = false;

        // refresh banks
        master->bank.rescanforbanks();

        for (size_t i=0, size = master->bank.banks.size(); i < size; i++)
        {
            if (! master->bank.banks[i].dir.empty())
            {
                master->bank.loadbank(master->bank.banks[i].dir);

                for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
                {
                    const std::string insName = master->bank.getname(instrument);

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
        programs.clear();

        delete master;

        if (--s_instanceCount == 0)
        {
            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return PARAMETER_MAX;
    }

    const Parameter* getParameterInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        if (index >= PARAMETER_MAX)
            return nullptr;

        static Parameter param;

        param.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case PARAMETER_MASTER:
            param.hints = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
            param.name  = "Master Volume";
            param.unit  = nullptr;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.ranges.def = 100.0f;
            break;
        }

        return &param;
    }

    float getParameterValue(uint32_t index)
    {
        switch (index)
        {
        case PARAMETER_MASTER:
            return master->Pvolume;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount()
    {
        return programs.size();
    }

    const MidiProgram* getMidiProgramInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= programs.size())
            return nullptr;

        const ProgramInfo pInfo(programs[index]);

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo.bank;
        midiProgram.program = pInfo.prog;
        midiProgram.name    = pInfo.name.c_str();

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(uint32_t index, float value)
    {
        switch (index)
        {
        case PARAMETER_MASTER:
            master->setPvolume((char)rint(value));
            break;
        }
    }

    void setMidiProgram(uint32_t bank, uint32_t program)
    {
        if (bank >= master->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        const std::string bankdir = master->bank.banks[bank].dir;

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&master->mutex);

            master->bank.loadbank(bankdir);
            master->bank.loadfromslot(program, master->part[0]);

            pthread_mutex_unlock(&master->mutex);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        master->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    void process(float**, float** outBuffer, uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        unsigned long from_frame       = 0;
        unsigned long event_index      = 0;
        unsigned long next_event_frame = 0;
        unsigned long to_frame = 0;
        pthread_mutex_lock(&master->mutex);

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
                master->GetAudioOutSamples(to_frame - from_frame, (int)getSampleRate(), &outBuffer[0][from_frame], &outBuffer[1][from_frame]);
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

                    master->noteOff(channel, note);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = midiEvents[event_index].data[1];
                    uint8_t velo = midiEvents[event_index].data[2];

                    master->noteOn(channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = midiEvents[event_index].data[1];
                    uint8_t pressure = midiEvents[event_index].data[2];

                    master->polyphonicAftertouch(channel, note, pressure);
                }

                event_index++;
            }

        // Keep going until we have the desired total length of sample...
        } while (to_frame < frames);

        pthread_mutex_unlock(&master->mutex);
    }

    // -------------------------------------------------------------------

private:
    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        std::string name;
    };
    std::vector<ProgramInfo> programs;

    Master* master;

    static int s_instanceCount;

    PluginDescriptorClassEND(ZynAddSubFxPlugin)
};

int ZynAddSubFxPlugin::s_instanceCount = 0;

// -----------------------------------------------------------------------

static PluginDescriptor zynAddSubFxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
    /* hints     */ PLUGIN_IS_SYNTH | /*PLUGIN_HAS_GUI |*/ PLUGIN_USES_SINGLE_THREAD,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::PARAMETER_MAX,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

CARLA_REGISTER_NATIVE_PLUGIN(zynaddsubfx, zynAddSubFxDesc)

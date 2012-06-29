/*
 * Carla Backend
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

#ifdef BUILD_BRIDGE
#error Should not use linuxsampler for bridges!
#endif

#include "carla_plugin.h"

#ifdef WANT_LINUXSAMPLER
#include <linuxsampler/Sampler.h>
#include "linuxsampler/EngineFactory.h"

#include <QtCore/QFileInfo>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendLinuxSamplerPlugin Carla Backend LinuxSampler Plugin
 *
 * The Carla Backend LinuxSampler Plugin.\n
 * http://www.linuxsampler.org/
 * @{
 */

// TODO - setMidiProgram()

#define LINUXSAMPLER_VOLUME_MAX 3.16227766f    // +10 dB
#define LINUXSAMPLER_VOLUME_MIN 0.0f           // -inf dB

class AudioOutputDevicePlugin : public LinuxSampler::AudioOutputDevice
{
public:
    AudioOutputDevicePlugin(CarlaPlugin* plugin) :
        AudioOutputDevice(std::map<String,LinuxSampler::DeviceCreationParameter*>()),
        m_plugin(plugin)
    {
    }

    // -------------------------------------------------------------------
    // LinuxSampler virtual methods

    void Play()
    {
    }

    bool IsPlaying()
    {
        return m_plugin && m_plugin->enabled();
    }

    void Stop()
    {
    }

    uint MaxSamplesPerCycle()
    {
        return get_buffer_size();
    }

    uint SampleRate()
    {
        return get_sample_rate();
    }

    String Driver()
    {
        return "AudioOutputDevicePlugin";
    }

    LinuxSampler::AudioChannel* CreateChannel(uint channelNr)
    {
        return new LinuxSampler::AudioChannel(channelNr, nullptr, 0);
    }

    // -------------------------------------------------------------------

    int Render(uint samples)
    {
        return RenderAudio(samples);
    }

private:
    CarlaPlugin* m_plugin;
};

class MidiInputDevicePlugin : public LinuxSampler::MidiInputDevice
{
public:
    MidiInputDevicePlugin(LinuxSampler::Sampler* sampler) : LinuxSampler::MidiInputDevice(std::map<String, LinuxSampler::DeviceCreationParameter*>(), sampler)
    {
    }

    // -------------------------------------------------------------------
    // MIDI Port implementation for this plugin MIDI input driver

    class MidiInputPortPlugin : public LinuxSampler::MidiInputPort
    {
    protected:
        MidiInputPortPlugin(MidiInputDevicePlugin* device, int portNumber) : LinuxSampler::MidiInputPort(device, portNumber)
        {
        }
        friend class MidiInputDevicePlugin;
    };

    // -------------------------------------------------------------------
    // LinuxSampler virtual methods

    void Listen()
    {
    }

    void StopListen()
    {
    }

    String Driver()
    {
        return "MidiInputDevicePlugin";
    }

    LinuxSampler::MidiInputPort* CreateMidiPort()
    {
        return new MidiInputPortPlugin(this, Ports.size());
    }

    // -------------------------------------------------------------------

    void DeleteMidiPort(LinuxSampler::MidiInputPort* port)
    {
        delete (MidiInputPortPlugin*)port;
    }
};

class LinuxSamplerPlugin : public CarlaPlugin
{
public:
    LinuxSamplerPlugin(unsigned short id, bool isGIG) : CarlaPlugin(id)
    {
        qDebug("LinuxSamplerPlugin::LinuxSamplerPlugin()");

        m_type  = isGIG ? PLUGIN_GIG : PLUGIN_SFZ;
        sampler = new LinuxSampler::Sampler;
        sampler_channel = nullptr;

        engine = nullptr;
        engine_channel = nullptr;
        instrument = nullptr;

        audioOutputDevice = new AudioOutputDevicePlugin(this);
        midiInputDevice   = new MidiInputDevicePlugin(sampler);
        midiInputPort     = midiInputDevice->CreateMidiPort();

        m_isGIG = isGIG;
        m_label = nullptr;
        m_maker = nullptr;
    }

    ~LinuxSamplerPlugin()
    {
        qDebug("LinuxSamplerPlugin::~LinuxSamplerPlugin()");

        if (sampler_channel)
        {
            midiInputPort->Disconnect(sampler_channel->GetEngineChannel());
            sampler->RemoveSamplerChannel(sampler_channel);
        }

        midiInputDevice->DeleteMidiPort(midiInputPort);

        delete audioOutputDevice;
        delete midiInputDevice;
        delete sampler;

        instrumentIds.clear();

        if (m_label)
            free((void*)m_label);

        if (m_maker)
            free((void*)m_maker);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return PLUGIN_CATEGORY_SYNTH;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    void getLabel(char* const strBuf)
    {
        strncpy(strBuf, m_label, STR_MAX);
    }

    void getMaker(char* const strBuf)
    {
        strncpy(strBuf, m_maker, STR_MAX);
    }

    void getCopyright(char* const strBuf)
    {
        getMaker(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        strncpy(strBuf, m_name, STR_MAX);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("LinuxSamplerPlugin::reload() - start");

        // Safely disable plugin for reload
        const CarlaPluginScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aouts;
        aouts  = 2;

        aout.ports    = new CarlaEngineAudioPort*[aouts];
        aout.rindexes = new uint32_t[aouts];

        char portName[STR_MAX];

        // ---------------------------------------
        // Audio Outputs

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":out-left");
        }
        else
#endif
            strcpy(portName, "out-left");

        aout.ports[0]    = (CarlaEngineAudioPort*)x_client->addPort(portName, CarlaEnginePortTypeAudio, false);
        aout.rindexes[0] = 0;

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":out-right");
        }
        else
#endif
            strcpy(portName, "out-right");

        aout.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(portName, CarlaEnginePortTypeAudio, false);
        aout.rindexes[1] = 1;

        // ---------------------------------------
        // MIDI Input

#ifndef BUILD_BRIDGE
        if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":midi-in");
        }
        else
#endif
            strcpy(portName, "midi-in");

        midi.portMin = (CarlaEngineMidiPort*)x_client->addPort(portName, CarlaEnginePortTypeMIDI, true);

        // ---------------------------------------

        aout.count  = aouts;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        m_hints |= PLUGIN_IS_SYNTH;
        m_hints |= PLUGIN_CAN_VOLUME;
        m_hints |= PLUGIN_CAN_BALANCE;

        reloadPrograms(true);

        x_client->activate();

        qDebug("LinuxSamplerPlugin::reload() - end");
    }

    void reloadPrograms(bool init)
    {
        qDebug("LinuxSamplerPlugin::reloadPrograms(%s)", bool2str(init));

        // Delete old programs
        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < midiprog.count; i++)
                free((void*)midiprog.data[i].name);

            delete[] midiprog.data;
        }

        midiprog.count = 0;
        midiprog.data  = nullptr;

        // Query new programs
        uint32_t i = 0;
        midiprog.count += instrumentIds.size();

        if (midiprog.count > 0)
            midiprog.data = new midi_program_t [midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            LinuxSampler::InstrumentManager::instrument_info_t info = instrument->GetInstrumentInfo(instrumentIds[i]);

            // FIXME - use % 128 stuff
            midiprog.data[i].bank    = 0;
            midiprog.data[i].program = i;
            midiprog.data[i].name    = strdup(info.InstrumentName.c_str());
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        osc_global_send_set_midi_program_count(m_id, midiprog.count);

        for (i=0; i < midiprog.count; i++)
            osc_global_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float**, float** outBuffer, uint32_t frames, uint32_t framesOffset)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

        double aouts_peak_tmp[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (cin_channel >= 0 && cin_channel < 16 && m_active && m_activeBefore)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
            {
                if (! extMidiNotes[i].valid)
                    break;

                if (extMidiNotes[i].velo)
                    midiInputPort->DispatchNoteOn(extMidiNotes[i].note, extMidiNotes[i].velo, cin_channel, framesOffset);
                else
                    midiInputPort->DispatchNoteOff(extMidiNotes[i].note, extMidiNotes[i].velo, cin_channel, framesOffset);

                extMidiNotes[i].valid = false;
                midiEventCount += 1;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (System)

        if (m_active && m_activeBefore)
        {
            void* minBuffer = midi.portMin->getBuffer();

            const CarlaEngineMidiEvent* minEvent;
            uint32_t time, nEvents = midi.portMin->getEventCount(minBuffer);

            for (i=0; i < nEvents && midiEventCount < MAX_MIDI_EVENTS; i++)
            {
                minEvent = midi.portMin->getEvent(minBuffer, i);

                if (! minEvent)
                    continue;

                time = minEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                uint8_t status  = minEvent->data[0];
                uint8_t channel = status & 0x0F;

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                    status -= 0x10;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = minEvent->data[1];

                    midiInputPort->DispatchNoteOff(note, 0, channel, time);

                    if (channel == cin_channel)
                        postponeEvent(PluginPostEventNoteOff, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = minEvent->data[1];
                    uint8_t velo = minEvent->data[2];

                    midiInputPort->DispatchNoteOn(note, velo, channel, time);

                    if (channel == cin_channel)
                        postponeEvent(PluginPostEventNoteOn, note, velo);
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    uint8_t pressure = minEvent->data[1];

                    midiInputPort->DispatchControlChange(MIDI_STATUS_AFTERTOUCH, pressure, channel, time);
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    uint8_t lsb = minEvent->data[1];
                    uint8_t msb = minEvent->data[2];

                    midiInputPort->DispatchPitchbend(((msb << 7) | lsb) - 8192, channel, time);
                }
                else
                    continue;

                midiEventCount += 1;
            }
        } // End of MIDI Input (System)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (cin_channel >= 0 && cin_channel < 16)
                {
                    midiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, cin_channel);
                    midiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, cin_channel);
                }
            }

            audioOutputDevice->Channel(0)->SetBuffer(outBuffer[0]);
            audioOutputDevice->Channel(1)->SetBuffer(outBuffer[1]);
            // QUESTION: Need to clear it before?
            audioOutputDevice->Render(frames);
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_volume  = x_vol != 1.0;
            bool do_balance = (x_bal_left != -1.0 || x_bal_right != 1.0);

            double bal_rangeL, bal_rangeR;
            float oldBufLeft[do_balance ? frames : 0];

            for (i=0; i < aout.count; i++)
            {
                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_vol;
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; i < 2 && k < frames; k++)
                {
                    if (abs(outBuffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs(outBuffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(outBuffer[i], 0.0f, sizeof(float)*frames);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        aouts_peak[(m_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(m_id*2)+1] = aouts_peak_tmp[1];

        m_activeBefore = m_active;
    }

    // -------------------------------------------------------------------

    bool init(const char* filename, const char* const name, const char* label)
    {
        QFileInfo file(filename);

        if (file.exists() && file.isFile() && file.isReadable())
        {
            const char* stype = m_isGIG ? "gig" : "sfz";

            try {
                engine = LinuxSampler::EngineFactory::Create(stype);
            }
            catch (LinuxSampler::Exception& e)
            {
                set_last_error(e.what());
                return false;
            }

            try {
                instrument = engine->GetInstrumentManager();
            }
            catch (LinuxSampler::Exception& e)
            {
                set_last_error(e.what());
                return false;
            }

            try {
                instrumentIds = instrument->GetInstrumentFileContent(filename);
            }
            catch (LinuxSampler::Exception& e)
            {
                set_last_error(e.what());
                return false;
            }

            if (instrumentIds.size() > 0)
            {
                LinuxSampler::InstrumentManager::instrument_info_t info = instrument->GetInstrumentInfo(instrumentIds[0]);

                m_label = strdup(info.Product.c_str());
                m_maker = strdup(info.Artists.c_str());
                m_filename = strdup(filename);

                if (name)
                    m_name = get_unique_name(name);
                else
                    m_name = get_unique_name(label && label[0] ? label : info.InstrumentName.c_str());

                sampler_channel = sampler->AddSamplerChannel();
                sampler_channel->SetEngineType(stype);
                sampler_channel->SetAudioOutputDevice(audioOutputDevice);
                //sampler_channel->SetMidiInputDevice(midiInputDevice);
                //sampler_channel->SetMidiInputChannel(LinuxSampler::midi_chan_1);
                midiInputPort->Connect(sampler_channel->GetEngineChannel(), LinuxSampler::midi_chan_all);

                engine_channel = sampler_channel->GetEngineChannel();
                engine_channel->Connect(audioOutputDevice);
                engine_channel->PrepareLoadInstrument(filename, 0); // todo - find instrument from label
                engine_channel->LoadInstrument();
                engine_channel->Volume(LINUXSAMPLER_VOLUME_MAX);

                x_client = new CarlaEngineClient(this);

                if (x_client->isOk())
                    return true;
                else
                    set_last_error("Failed to register plugin client");
            }
            else
                set_last_error("Failed to find any instruments");
        }
        else
            set_last_error("Requested file is not valid or does not exist");

        return false;
    }

private:
    LinuxSampler::Sampler* sampler;
    LinuxSampler::SamplerChannel* sampler_channel;
    LinuxSampler::Engine* engine;
    LinuxSampler::EngineChannel* engine_channel;
    LinuxSampler::InstrumentManager* instrument;
    std::vector<LinuxSampler::InstrumentManager::instrument_id_t> instrumentIds;

    AudioOutputDevicePlugin* audioOutputDevice;
    MidiInputDevicePlugin* midiInputDevice;
    LinuxSampler::MidiInputPort* midiInputPort;

    bool m_isGIG;
    const char* m_label;
    const char* m_maker;
};
#endif

short add_plugin_linuxsampler(const char* const filename, const char* const name, const char* const label, bool isGIG)
{
    qDebug("add_plugin_linuxsampler(%s, %s, %s, %s)", filename, name, label, bool2str(isGIG));

#ifdef WANT_LINUXSAMPLER
    short id = get_new_plugin_id();

    if (id < 0)
    {
        set_last_error("Maximum number of plugins reached");
        return -1;
    }

    LinuxSamplerPlugin* plugin = new LinuxSamplerPlugin(id, isGIG);

    if (! plugin->init(filename, name, label))
    {
        delete plugin;
        return -1;
    }

    plugin->reload();

    unique_names[id] = plugin->name();
    CarlaPlugins[id] = plugin;

    plugin->registerToOsc();

    return id;
#else
    set_last_error("linuxsampler support not available");
    return -1;
#endif
}

short add_plugin_gig(const char* const filename, const char* const name, const char* const label)
{
    qDebug("add_plugin_gig(%s, %s, %s)", filename, name, label);
    return add_plugin_linuxsampler(filename, name, label, true);
}

short add_plugin_sfz(const char* const filename, const char* const name, const char* const label)
{
    qDebug("add_plugin_sfz(%s, %s, %s)", filename, name, label);
    return add_plugin_linuxsampler(filename, name, label, false);
}

/**@}*/

CARLA_BACKEND_END_NAMESPACE

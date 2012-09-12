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

// TODO - setMidiProgram()

#include "carla_plugin.h"

#ifdef WANT_LINUXSAMPLER
#include "carla_linuxsampler.h"
#else
#warning linuxsampler not available (no GIG and SFZ support)
#endif

#include <QtCore/QFileInfo>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

#ifdef WANT_LINUXSAMPLER

/*!
 * @defgroup CarlaBackendLinuxSamplerPlugin Carla Backend LinuxSampler Plugin
 *
 * The Carla Backend LinuxSampler Plugin.\n
 * http://www.linuxsampler.org/
 * @{
 */

class LinuxSamplerPlugin : public CarlaPlugin
{
public:
    LinuxSamplerPlugin(CarlaEngine* const engine_, unsigned short id, bool isGIG) : CarlaPlugin(engine_, id)
    {
        qDebug("LinuxSamplerPlugin::LinuxSamplerPlugin()");

        m_type  = isGIG ? PLUGIN_GIG : PLUGIN_SFZ;

        sampler = new LinuxSampler::Sampler;
        sampler_channel = nullptr;

        engine = nullptr;
        engine_channel = nullptr;
        instrument = nullptr;

        audioOutputDevice = new LinuxSampler::AudioOutputDevicePlugin(engine_, this);
        midiInputDevice   = new LinuxSampler::MidiInputDevicePlugin(sampler);
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
        Q_ASSERT(instrument);

        // Safely disable plugin for reload
        const ScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t aOuts;
        aOuts  = 2;

        aOut.ports    = new CarlaEngineAudioPort*[aOuts];
        aOut.rindexes = new uint32_t[aOuts];

        const int portNameSize = CarlaEngine::maxPortNameSize() - 1;
        char portName[portNameSize];

        // ---------------------------------------
        // Audio Outputs

#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":out-left");
        }
        else
#endif
            strcpy(portName, "out-left");

        aOut.ports[0]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
        aOut.rindexes[0] = 0;

#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":out-right");
        }
        else
#endif
            strcpy(portName, "out-right");

        aOut.ports[1]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
        aOut.rindexes[1] = 1;

        // ---------------------------------------
        // MIDI Input

#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode != PROCESS_MODE_MULTIPLE_CLIENTS)
        {
            strcpy(portName, m_name);
            strcat(portName, ":midi-in");
        }
        else
#endif
            strcpy(portName, "midi-in");

        midi.portMin = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);

        // ---------------------------------------

        aOut.count  = aOuts;

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

        // sound kits must always have at least 1 midi-program
        Q_ASSERT(midiprog.count > 0);

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
        if (x_engine->isOscControllerRegisted())
        {
            x_engine->osc_send_control_set_midi_program_count(m_id, midiprog.count);

            for (i=0; i < midiprog.count; i++)
                x_engine->osc_send_control_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);
        }
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
        }
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** const, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

        double aOutsPeak[2] = { 0.0 };

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input

        if (m_active && m_activeBefore)
        {
            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (External)

            {
                engineMidiLock();

                for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    if (extMidiNotes[i].channel < 0)
                        break;

                    if (extMidiNotes[i].velo)
                        midiInputPort->DispatchNoteOn(extMidiNotes[i].note, extMidiNotes[i].velo, m_ctrlInChannel, 0);
                    else
                        midiInputPort->DispatchNoteOff(extMidiNotes[i].note, extMidiNotes[i].velo, m_ctrlInChannel, 0);

                    extMidiNotes[i].channel = -1; // mark as invalid
                    midiEventCount += 1;
                }

                engineMidiUnlock();

            } // End of MIDI Input (External)

            CARLA_PROCESS_CONTINUE_CHECK;

            // ----------------------------------------------------------------------------------------------------
            // MIDI Input (System)

            {
                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = midi.portMin->getEventCount();

                for (i=0; i < nEvents && midiEventCount < MAX_MIDI_EVENTS; i++)
                {
                    minEvent = midi.portMin->getEvent(i);

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

                        postponeEvent(PluginPostEventNoteOff, channel, note, 0.0);
                    }
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                    {
                        uint8_t note = minEvent->data[1];
                        uint8_t velo = minEvent->data[2];

                        midiInputPort->DispatchNoteOn(note, velo, channel, time);

                        postponeEvent(PluginPostEventNoteOn, channel, note, velo);
                    }
                    else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                    {
                        //uint8_t note     = minEvent->data[1];
                        //uint8_t pressure = minEvent->data[2];

                        // TODO, not in linuxsampler API?
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

        } // End of MIDI Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                if (m_ctrlInChannel >= 0 && m_ctrlInChannel < 16)
                {
                    midiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_SOUND_OFF, 0, m_ctrlInChannel);
                    midiInputPort->DispatchControlChange(MIDI_CONTROL_ALL_NOTES_OFF, 0, m_ctrlInChannel);
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
            bool do_volume  = x_volume != 1.0;
            bool do_balance = (x_balanceLeft != -1.0 || x_balanceRight != 1.0);

            double bal_rangeL, bal_rangeR;
            float oldBufLeft[do_balance ? frames : 0];

            for (i=0; i < aOut.count; i++)
            {
                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_balanceLeft+1.0)/2;
                    bal_rangeR = (x_balanceRight+1.0)/2;

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

                // Volume
                if (do_volume)
                {
                    for (k=0; k < frames; k++)
                        outBuffer[i][k] *= x_volume;
                }

                // Output VU
                for (k=0; i < 2 && k < frames; k++)
                {
                    if (abs(outBuffer[i][k]) > aOutsPeak[i])
                        aOutsPeak[i] = abs(outBuffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aOut.count; i++)
                memset(outBuffer[i], 0.0f, sizeof(float)*frames);

            aOutsPeak[0] = 0.0;
            aOutsPeak[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setOutputPeak(m_id, 0, aOutsPeak[0]);
        x_engine->setOutputPeak(m_id, 1, aOutsPeak[1]);

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
                setLastError(e.what());
                return false;
            }

            try {
                instrument = engine->GetInstrumentManager();
            }
            catch (LinuxSampler::Exception& e)
            {
                setLastError(e.what());
                return false;
            }

            try {
                instrumentIds = instrument->GetInstrumentFileContent(filename);
            }
            catch (LinuxSampler::Exception& e)
            {
                setLastError(e.what());
                return false;
            }

            if (instrumentIds.size() > 0)
            {
                LinuxSampler::InstrumentManager::instrument_info_t info = instrument->GetInstrumentInfo(instrumentIds[0]);

                m_label = strdup(info.Product.c_str());
                m_maker = strdup(info.Artists.c_str());
                m_filename = strdup(filename);

                if (name)
                    m_name = x_engine->getUniqueName(name);
                else
                    m_name = x_engine->getUniqueName(label && label[0] ? label : info.InstrumentName.c_str());

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
                engine_channel->Volume(LinuxSampler::VOLUME_MAX);

                x_client = x_engine->addClient(this);

                if (x_client->isOk())
                    return true;
                else
                    setLastError("Failed to register plugin client");
            }
            else
                setLastError("Failed to find any instruments");
        }
        else
            setLastError("Requested file is not valid or does not exist");

        return false;
    }

    // -------------------------------------------------------------------

    static CarlaPlugin* newLinuxSampler(const initializer& init, bool isGIG);

private:
    LinuxSampler::Sampler* sampler;
    LinuxSampler::SamplerChannel* sampler_channel;
    LinuxSampler::Engine* engine;
    LinuxSampler::EngineChannel* engine_channel;
    LinuxSampler::InstrumentManager* instrument;
    std::vector<LinuxSampler::InstrumentManager::instrument_id_t> instrumentIds;

    LinuxSampler::AudioOutputDevicePlugin* audioOutputDevice;
    LinuxSampler::MidiInputDevicePlugin* midiInputDevice;
    LinuxSampler::MidiInputPort* midiInputPort;

    bool m_isGIG;
    const char* m_label;
    const char* m_maker;
};

CarlaPlugin* LinuxSamplerPlugin::newLinuxSampler(const initializer& init, bool isGIG)
{
    qDebug("LinuxSamplerPlugin::newLinuxSampler(%p, \"%s\", \"%s\", \"%s\", %s)", init.engine, init.filename, init.name, init.label, bool2str(isGIG));

    short id = init.engine->getNewPluginId();

    if (id < 0 || id > CarlaEngine::maxPluginNumber())
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    LinuxSamplerPlugin* const plugin = new LinuxSamplerPlugin(init.engine, id, isGIG);

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    plugin->reload();
    plugin->registerToOsc();

    return plugin;
}
#endif // WANT_LINUXSAMPLER

CarlaPlugin* CarlaPlugin::newGIG(const initializer& init)
{
    qDebug("CarlaPlugin::newGIG(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);
#ifdef WANT_LINUXSAMPLER
    return LinuxSamplerPlugin::newLinuxSampler(init, true);
#else
    setLastError("linuxsampler support not available");
    return nullptr;
#endif
}

CarlaPlugin* CarlaPlugin::newSFZ(const initializer& init)
{
    qDebug("CarlaPlugin::newSFZ(%p, \"%s\", \"%s\", \"%s\")", init.engine, init.filename, init.name, init.label);
#ifdef WANT_LINUXSAMPLER
    return LinuxSamplerPlugin::newLinuxSampler(init, false);
#else
    setLastError("linuxsampler support not available");
    return nullptr;
#endif
}

/**@}*/

CARLA_BACKEND_END_NAMESPACE

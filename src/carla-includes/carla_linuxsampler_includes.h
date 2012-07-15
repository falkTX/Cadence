/*
 * Carla common LinuxSampler code
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#ifndef CARLA_LINUXSAMPLER_INCLUDES_H
#define CARLA_LINUXSAMPLER_INCLUDES_H

#include <linuxsampler/engines/Engine.h>
#include <linuxsampler/Sampler.h>

#include <set>
#include <vector>

namespace LinuxSampler {

class EngineFactory
{
public:
    static std::vector<String> AvailableEngineTypes();
    static String AvailableEngineTypesAsString();
    static Engine* Create(String EngineType) throw (Exception);
    static void Destroy(Engine* pEngine);
    static const std::set<Engine*>& EngineInstances();

protected:
    static void Erase(Engine* pEngine);
    friend class Engine;
};

#ifndef BUILD_BRIDGE

#include "carla_plugin.h"

#define LINUXSAMPLER_VOLUME_MAX 3.16227766f    // +10 dB
#define LINUXSAMPLER_VOLUME_MIN 0.0f           // -inf dB

class AudioOutputDevicePlugin : public AudioOutputDevice
{
public:
    AudioOutputDevicePlugin(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin) :
        AudioOutputDevice(std::map<String, DeviceCreationParameter*>()),
        m_engine(engine),
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
        return m_engine->isRunning() && m_plugin->enabled();
    }

    void Stop()
    {
    }

    uint MaxSamplesPerCycle()
    {
        return m_engine->getBufferSize();
    }

    uint SampleRate()
    {
        return m_engine->getSampleRate();
    }

    String Driver()
    {
        return "AudioOutputDevicePlugin";
    }

    AudioChannel* CreateChannel(uint channelNr)
    {
        return new AudioChannel(channelNr, nullptr, 0);
    }

    // -------------------------------------------------------------------

    int Render(uint samples)
    {
        return RenderAudio(samples);
    }

private:
    CarlaBackend::CarlaEngine* const m_engine;
    CarlaBackend::CarlaPlugin* const m_plugin;
};

class MidiInputDevicePlugin : public MidiInputDevice
{
public:
    MidiInputDevicePlugin(Sampler* const sampler) :
        MidiInputDevice(std::map<String, DeviceCreationParameter*>(), sampler)
    {
    }

    // -------------------------------------------------------------------
    // MIDI Port implementation for this plugin MIDI input driver

    class MidiInputPortPlugin : public MidiInputPort
    {
    protected:
        MidiInputPortPlugin(MidiInputDevicePlugin* const device, const int portNumber) :
            MidiInputPort(device, portNumber)
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

    MidiInputPort* CreateMidiPort()
    {
        return new MidiInputPortPlugin(this, Ports.size());
    }

    // -------------------------------------------------------------------

    void DeleteMidiPort(MidiInputPort* const port)
    {
        delete (MidiInputPortPlugin*)port;
    }
};

#endif // BUILD_BRIDGE

} // namespace LinuxSampler

#endif // CARLA_LINUXSAMPLER_INCLUDES_H

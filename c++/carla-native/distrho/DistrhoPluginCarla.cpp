/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "carla_native.hpp"

//#include "src/DistrhoDefines.h"

#include "DistrhoPluginMain.cpp"
#include "DistrhoUIMain.cpp"

// -------------------------------------------------

 START_NAMESPACE_DISTRHO

class CarlaDistrhoPlugin : public PluginDescriptorClass
{
public:
    CarlaDistrhoPlugin(const HostDescriptor* host)
        : PluginDescriptorClass(host)
    {
        d_lastBufferSize = getBufferSize();
        d_lastSampleRate = getSampleRate();
        setLastUiSampleRate(d_lastSampleRate);
    }

    ~CarlaDistrhoPlugin()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return plugin.parameterCount();
    }

    const ::Parameter* getParameterInfo(uint32_t index)
    {
        static ::Parameter param;

        param.hints  = 0x0;
#if DISTRHO_PLUGIN_IS_SYNTH
        param.hints |= PLUGIN_IS_SYNTH;
#endif
#if DISTRHO_PLUGIN_HAS_UI
        param.hints |= PLUGIN_HAS_GUI;
# ifdef DISTRHO_UI_QT4
        param.hints |= PLUGIN_USES_SINGLE_THREAD;
# endif
#endif

        param.name  = plugin.parameterName(index);
        param.unit  = plugin.parameterUnit(index);

        {
            const ParameterRanges* ranges(plugin.parameterRanges(index));
            param.ranges.def = ranges->def;
            param.ranges.min = ranges->min;
            param.ranges.max = ranges->max;
            param.ranges.step = ranges->step;
            param.ranges.stepSmall = ranges->stepSmall;
            param.ranges.stepLarge = ranges->stepLarge;
        }

        param.scalePointCount = 0;
        param.scalePoints = nullptr;

        return &param;
    }

    float getParameterValue(uint32_t index)
    {
        return plugin.parameterValue(index);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    // TODO
    virtual uint32_t getMidiProgramCount()
    {
        return 0;
    }

    virtual const MidiProgram* getMidiProgramInfo(uint32_t index)
    {
        return nullptr;
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(uint32_t index, double value)
    {
        plugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    // TODO
    void setMidiProgram(uint32_t bank, uint32_t program)
    {
        Q_UNUSED(bank);
        Q_UNUSED(program);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    // TODO
    void setCustomData(const char* key, const char* value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(bool show)
    {
        Q_UNUSED(show);
    }

    void uiIdle()
    {
    }

    void uiSetParameterValue(uint32_t index, double value)
    {
        CARLA_ASSERT(index < getParameterCount());
        Q_UNUSED(value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    // TODO
    void uiSetMidiProgram(uint32_t bank, uint32_t program)
    {
        Q_UNUSED(bank);
        Q_UNUSED(program);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    // TODO
    void uiSetCustomData(const char* key, const char* value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        plugin.activate();
    }

    void deactivate()
    {
        plugin.deactivate();
    }

    void process(float**, float**, uint32_t, uint32_t, ::MidiEvent*)
    {
        //plugin->d_run();
    }

    // -------------------------------------------------------------------

protected:
    PluginInternal plugin;

    PluginDescriptorClassEND(CarlaDistrhoPlugin)
};

 END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

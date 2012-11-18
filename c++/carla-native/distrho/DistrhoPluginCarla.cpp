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

#include "src/DistrhoDefines.h"

#undef START_NAMESPACE_DISTRHO
//#undef END_NAMESPACE_DISTRHO
#undef USE_NAMESPACE_DISTRHO

#define START_NAMESPACE_DISTRHO namespace DISTRHO_NAMESPACE {
//#define END_NAMESPACE_DISTRHO }
#define USE_NAMESPACE_DISTRHO using namespace DISTRHO_NAMESPACE;

#include "DistrhoPluginMain.cpp"
#include "DistrhoUIMain.cpp"

//#include "src/DistrhoPluginInternal.h"

//#if DISTRHO_PLUGIN_HAS_UI
//# include "src/DistrhoUIInternal.h"
//#endif

// -------------------------------------------------

// START_NAMESPACE_DISTRHO

class CarlaDistrhoPlugin : public PluginDescriptorClass
{
public:
    CarlaDistrhoPlugin(const HostDescriptor* host)
        : PluginDescriptorClass(host)
    {
    }

    ~CarlaDistrhoPlugin()
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
    }

    void process(float**, float**, uint32_t, uint32_t, MidiEvent*)
    {
    }

    // -------------------------------------------------------------------

private:
    PluginDescriptorClassEND(CarlaDistrhoPlugin)
};

// END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

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

// Set plugin data
#include "3bandeq/DistrhoPluginInfo.h"

/// Set namespace for this plugin
#define DISTRHO_NAMESPACE DISTRHO_3BEQ

// Include DISTRHO code
#include "DistrhoPluginCarla.cpp"

// Include Plugin code
#include "3bandeq/DistrhoArtwork3BandEQ.cpp"
#include "3bandeq/DistrhoPlugin3BandEQ.cpp"
#include "3bandeq/DistrhoUI3BandEQ.cpp"

// -----------------------------------------------------------------------

class CarlaDistrhoPlugin3BandEQ : public CarlaDistrhoPlugin
{
public:
    CarlaDistrhoPlugin3BandEQ(const HostDescriptor* host)
        : CarlaDistrhoPlugin(host)
    {
    }

    ~CarlaDistrhoPlugin3BandEQ()
    {
    }

    // -------------------------------------------------------------------

private:
    PluginDescriptorClassEND(CarlaDistrhoPlugin3BandEQ)
};

// -----------------------------------------------------------------------

static PluginDescriptor tBandEqDesc = {
    /* category  */ PLUGIN_CATEGORY_EQ,
    /* hints     */ 0x0,
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ DISTRHO_NAMESPACE::DistrhoPlugin3BandEQ::paramCount,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "3BandEQ",
    /* maker     */ "falkTX",
    /* copyright */ "LGPL",
    PluginDescriptorFILL(CarlaDistrhoPlugin3BandEQ)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_3BandEQ()
{
    carla_register_native_plugin(&tBandEqDesc);
}

// -----------------------------------------------------------------------

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

// Include Plugin code
#include "3bandeq/DistrhoPlugin3BandEQ.h"
#include "3bandeq/DistrhoUI3BandEQ.h"

// Include DISTRHO code
#include "DistrhoPluginCarla.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

//class CarlaDistrhoPlugin : public PluginDescriptorClass
//{
//public:
//    CarlaDistrhoPlugin(const HostDescriptor* host)
//        : PluginDescriptorClass(host)
//    {
//    }

//    ~CarlaDistrhoPlugin()
//    {
//    }

//protected:
//    // -------------------------------------------------------------------
//    // Plugin process calls

//    // -------------------------------------------------------------------

//public:
//    static PluginHandle _instantiate(struct _PluginDescriptor*, HostDescriptor* host)
//    {
//        return new CarlaDistrhoPlugin(host);
//    }
//    static void _cleanup(PluginHandle handle)
//    {
//        delete (CarlaDistrhoPlugin*)handle;
//    }
//};

// -----------------------------------------------------------------------

static PluginDescriptor tBandEqDesc = {
    /* category  */ PLUGIN_CATEGORY_EQ,
    /* hints     */ 0x0,
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ DistrhoPlugin3BandEQ::paramCount,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "3BandEQ",
    /* maker     */ "falkTX",
    /* copyright */ "LGPL",
    PluginDescriptorFILL(CarlaDistrhoPlugin)
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

void carla_register_native_plugin_3BandEQ()
{
    USE_NAMESPACE_DISTRHO
    carla_register_native_plugin(&tBandEqDesc);
}

// -----------------------------------------------------------------------

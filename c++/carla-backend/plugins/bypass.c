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

#include "carla_native.h"

#include <stdlib.h>
#include <string.h>

enum ByPassPorts {
    PORT_IN1  = 0,
    PORT_IN2  = 1,
    PORT_OUT1 = 2,
    PORT_OUT2 = 3,
    PORT_MAX  = 4
};

void bypass_init(struct _PluginDescriptor* _this_)
{
    _this_->portCount = PORT_MAX;
    _this_->ports = malloc(sizeof(PluginPort) * PORT_MAX);

    _this_->ports[PORT_IN1].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_IN1].hints = 0;
    _this_->ports[PORT_IN1].name  = "in1";

    _this_->ports[PORT_IN2].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_IN2].hints = 0;
    _this_->ports[PORT_IN2].name  = "in2";

    _this_->ports[PORT_OUT1].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_OUT1].hints = PORT_HINT_IS_OUTPUT;
    _this_->ports[PORT_OUT1].name  = "out1";

    _this_->ports[PORT_OUT2].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_OUT2].hints = PORT_HINT_IS_OUTPUT;
    _this_->ports[PORT_OUT2].name  = "out2";
}

void bypass_fini(struct _PluginDescriptor* _this_)
{
    free(_this_->ports);

    _this_->portCount = 0;
    _this_->ports     = NULL;
}

PluginHandle bypass_instantiate(struct _PluginDescriptor* _this_, HostDescriptor* host)
{
    // dummy, return non-NULL
    return (PluginHandle)1;

    // unused
    (void)_this_;
    (void)host;
}

void bypass_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
{
    float* input1  = inBuffer[0];
    float* input2  = inBuffer[1];
    float* output1 = outBuffer[0];
    float* output2 = outBuffer[1];

    memcpy(output1, input1, sizeof(float)*frames);
    memcpy(output2, input2, sizeof(float)*frames);

    return;

    // unused
    (void)handle;
    (void)midiEventCount;
    (void)midiEvents;
}

static PluginDescriptor bypassDesc = {
    .category   = PLUGIN_CATEGORY_NONE,
    .name       = "ByPass",
    .label      = "bypass",
    .maker      = "falkTX",
    .copyright  = "GNU GPL v2+",

    .portCount  = 0,
    .ports      = NULL,

    .midiProgramCount  = 0,
    .midiPrograms      = NULL,

    .instantiate = bypass_instantiate,
    .activate    = NULL,
    .deactivate  = NULL,
    .cleanup     = NULL,

    .get_parameter_ranges = NULL,
    .get_parameter_value  = NULL,
    .get_parameter_text   = NULL,
    .get_parameter_unit   = NULL,

    .set_parameter_value = NULL,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .show_gui = NULL,
    .idle_gui = NULL,

    .process  = bypass_process,

    ._singleton = NULL,
    ._init      = bypass_init,
    ._fini      = bypass_fini
};

CARLA_REGISTER_NATIVE_PLUGIN(bypass, bypassDesc)

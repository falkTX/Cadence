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

enum ByPassPorts {
    PORT_IN,
    PORT_OUT,
    PORT_MAX
};

struct ByPassInstance {
};

void bypass_init(struct _PluginDescriptor* _this_)
{
    _this_->portCount = PORT_MAX;
    _this_->ports = malloc(sizeof(PluginPort) * PORT_MAX);

    _this_->ports[PORT_IN].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_IN].hints = 0;
    _this_->ports[PORT_IN].name  = "in";


    _this_->ports[PORT_OUT].type  = PORT_TYPE_AUDIO;
    _this_->ports[PORT_OUT].hints = PORT_HINT_IS_OUTPUT;
    _this_->ports[PORT_OUT].name  = "out";
}

void bypass_fini(struct _PluginDescriptor* _this_)
{
    free(_this_->ports);

    _this_->portCount = 0;
    _this_->ports     = NULL;
}

static PluginDescriptor bypassDesc = {
    .category  = PLUGIN_CATEGORY_NONE,
    .name      = "ByPass",
    .label     = "bypass",
    .portCount = 0,
    .ports     = NULL,
    ._handle   = NULL,
    ._init     = NULL,
    ._fini     = NULL
};

CARLA_REGISTER_NATIVE_PLUGIN(bypass, bypassDesc)

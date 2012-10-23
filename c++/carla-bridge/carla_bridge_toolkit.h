/*
 * Carla bridge code
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BRIDGE_TOOLKIT_H
#define CARLA_BRIDGE_TOOLKIT_H

#include "carla_bridge.h"

#include <cstdlib>
#include <cstring>

CARLA_BRIDGE_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeToolkit Carla Bridge Toolkit
 *
 * The Carla Bridge Toolkit.
 * @{
 */

class CarlaToolkit
{
public:
    CarlaToolkit(const char* const title)
    {
        CARLA_ASSERT(title);

        m_title  = strdup(title ? title : "(null)");
        m_client = nullptr;
    }

    virtual ~CarlaToolkit()
    {
        if (m_title)
            free(m_title);
    }

    virtual void init() = 0;
    virtual void exec(CarlaClient* const client, const bool showGui) = 0;
    virtual void quit() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void resize(int width, int height) = 0;

#if BUILD_BRIDGE_UI
    virtual void* getContainerId()
    {
        return nullptr;
    }

    static CarlaToolkit* createNew(const char* const title);
#endif

protected:
    char* m_title;
    CarlaClient* m_client;
    friend class CarlaClient;
};

/**@}*/

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_TOOLKIT_H

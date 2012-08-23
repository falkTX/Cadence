/*
 * Carla bridge code
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

#ifndef CARLA_BRIDGE_TOOLKIT_H
#define CARLA_BRIDGE_TOOLKIT_H

#include "carla_bridge.h"

#include <cstdlib>
#include <cstring>

CARLA_BRIDGE_START_NAMESPACE

class CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkit(const char* const title)
    {
        Q_ASSERT(title);

        m_title  = strdup(title);
        m_client = nullptr;
    }

    virtual ~CarlaBridgeToolkit()
    {
        free(m_title);
    }

    virtual void init() = 0;
    virtual void exec(CarlaBridgeClient* const client) = 0;
    virtual void quit() = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void resize(int width, int height) = 0;

    static CarlaBridgeToolkit* createNew(const char* const title);

protected:
    char* m_title;
    CarlaBridgeClient* m_client;
};

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_TOOLKIT_H

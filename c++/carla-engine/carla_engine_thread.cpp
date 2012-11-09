/*
 * Carla Engine Thread
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

#include "carla_engine.hpp"
#include "carla_plugin.hpp"

CARLA_BACKEND_START_NAMESPACE

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine_, QObject* const parent)
    : QThread(parent),
      engine(engine_)
{
    qDebug("CarlaEngineThread::CarlaEngineThread(%p, %p)", engine, parent);
    CARLA_ASSERT(engine);

    m_stopNow = true;
}

CarlaEngineThread::~CarlaEngineThread()
{
    qDebug("CarlaEngineThread::~CarlaEngineThread()");
    CARLA_ASSERT(m_stopNow);
}

void CarlaEngineThread::startNow()
{
    qDebug("CarlaEngineThread::startNow()");
    CARLA_ASSERT(m_stopNow);

    m_stopNow = false;

    start(QThread::HighPriority);
}

void CarlaEngineThread::stopNow()
{
    qDebug("CarlaEngineThread::stopNow()");

    if (m_stopNow)
        return;

    m_stopNow = true;

    const ScopedLocker m(this);

    if (isRunning() && ! wait(200))
    {
        quit();

        if (isRunning() && ! wait(300))
            terminate();
    }
}

void CarlaEngineThread::run()
{
    qDebug("CarlaEngineThread::run()");
    CARLA_ASSERT(engine->isRunning());

    bool oscControlRegisted, usesSingleThread;
    unsigned short id;
    double value;

    while (engine->isRunning() && ! m_stopNow)
    {
        const ScopedLocker m(this);
        oscControlRegisted = engine->isOscControlRegisted();

        for (unsigned short i=0; i < engine->maxPluginNumber(); i++)
        {
            CarlaPlugin* const plugin = engine->getPluginUnchecked(i);

            if (plugin && plugin->enabled())
            {
                id = plugin->id();
                usesSingleThread = (plugin->hints() & PLUGIN_USES_SINGLE_THREAD);

                // -------------------------------------------------------
                // Process postponed events

                if (! usesSingleThread)
                    plugin->postEventsRun();

                // -------------------------------------------------------
                // Update parameter outputs

                if (oscControlRegisted || ! usesSingleThread)
                {
                    for (uint32_t i=0; i < plugin->parameterCount(); i++)
                    {
                        if (! plugin->parameterIsOutput(i))
                            continue;

                        value = plugin->getParameterValue(i);

                        // Update UI
                        if (! usesSingleThread)
                            plugin->uiParameterChange(i, value);

                        // Update OSC control client
                        if (oscControlRegisted)
                        {
#ifdef BUILD_BRIDGE
                            engine->osc_send_bridge_set_parameter_value(i, value);
#else
                            engine->osc_send_control_set_parameter_value(id, i, value);
#endif
                        }
                    }
                }

                // -------------------------------------------------------
                // Update OSC control client (peaks)

                if (oscControlRegisted)
                {
                    // Peak values
                    if (plugin->audioInCount() > 0)
                    {
#ifdef BUILD_BRIDGE
                        engine->osc_send_bridge_set_inpeak(1);
                        engine->osc_send_bridge_set_inpeak(2);
#else
                        engine->osc_send_control_set_input_peak_value(id, 1);
                        engine->osc_send_control_set_input_peak_value(id, 2);
#endif
                    }
                    if (plugin->audioOutCount() > 0)
                    {
#ifdef BUILD_BRIDGE
                        engine->osc_send_bridge_set_outpeak(1);
                        engine->osc_send_bridge_set_outpeak(2);
#else
                        engine->osc_send_control_set_output_peak_value(id, 1);
                        engine->osc_send_control_set_output_peak_value(id, 2);
#endif
                    }
                }
            }
        }

        if (! engine->idleOsc())
            msleep(50);
    }
}

CARLA_BACKEND_END_NAMESPACE

/*
 * Carla Backend
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

#include "carla_threads.h"
#include "carla_plugin.h"

#include <QtCore/QDebug>
#include <QtCore/QProcess>

// -----------------------------------------------------------------------
// CarlaCheckThread

CarlaCheckThread::CarlaCheckThread(CarlaBackend::CarlaEngine* const engine_, QObject* const parent)
    : QThread(parent),
      engine(engine_)
{
    qDebug("CarlaCheckThread::CarlaCheckThread(%p, %p)", engine, parent);
    Q_ASSERT(engine);
}

CarlaCheckThread::~CarlaCheckThread()
{
    qDebug("CarlaCheckThread::~CarlaCheckThread()");
}

void CarlaCheckThread::startNow()
{
    start(QThread::HighPriority);
}

void CarlaCheckThread::stopNow()
{
    m_stopNow = true;

    // TESTING - let processing finish first
    QMutexLocker(&this->mutex); // FIXME

    if (isRunning() && ! wait(200))
    {
        quit();

        if (isRunning() && ! wait(300))
            terminate();
    }
}

void CarlaCheckThread::run()
{
    qDebug("CarlaCheckThread::run()");

    using namespace CarlaBackend;

    bool oscControllerRegisted, usesSingleThread;
    unsigned short id, maxPluginNumber = CarlaEngine::maxPluginNumber();
    double value;

    m_stopNow = false;

    while (engine->isRunning() && ! m_stopNow)
    {
        const ScopedLocker m(this);

#ifdef BUILD_BRIDGE
        oscControllerRegisted = true;
#else
        oscControllerRegisted = engine->isOscControllerRegisted();
#endif

        for (unsigned short i=0; i < maxPluginNumber; i++)
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

                if (oscControllerRegisted || ! usesSingleThread)
                {
                    for (uint32_t i=0; i < plugin->parameterCount(); i++)
                    {
                        if (plugin->parameterIsOutput(i))
                        {
                            value = plugin->getParameterValue(i);

                            // Update UI
                            if (! usesSingleThread)
                                plugin->uiParameterChange(i, value);

                            // Update OSC control client
                            if (oscControllerRegisted)
#ifdef BUILD_BRIDGE
                                engine->osc_send_bridge_set_parameter_value(i, value);
#else
                                engine->osc_send_control_set_parameter_value(id, i, value);
#endif
                        }
                    }
                }

                // -------------------------------------------------------
                // Update OSC control client

                if (oscControllerRegisted)
                {
                    // Peak values
                    if (plugin->audioInCount() > 0)
                    {
#ifdef BUILD_BRIDGE
                        engine->osc_send_bridge_set_input_peak_value(1, engine->getInputPeak(id, 0));
                        engine->osc_send_bridge_set_input_peak_value(2, engine->getInputPeak(id, 1));
#else
                        engine->osc_send_control_set_input_peak_value(id, 1, engine->getInputPeak(id, 0));
                        engine->osc_send_control_set_input_peak_value(id, 2, engine->getInputPeak(id, 1));
#endif
                    }
                    if (plugin->audioOutCount() > 0)
                    {
#ifdef BUILD_BRIDGE
                        engine->osc_send_bridge_set_output_peak_value(1, engine->getOutputPeak(id, 0));
                        engine->osc_send_bridge_set_output_peak_value(2, engine->getOutputPeak(id, 1));
#else
                        engine->osc_send_control_set_output_peak_value(id, 1, engine->getOutputPeak(id, 0));
                        engine->osc_send_control_set_output_peak_value(id, 2, engine->getOutputPeak(id, 1));
#endif
                    }
                }
            }
        }

        msleep(50);
    }
}

// -----------------------------------------------------------------------
// CarlaPluginThread

#ifndef BUILD_BRIDGE

const char* PluginThreadMode2str(const CarlaPluginThread::PluginThreadMode mode)
{
    switch (mode)
    {
    case CarlaPluginThread::PLUGIN_THREAD_DSSI_GUI:
        return "PLUGIN_THREAD_DSSI_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_LV2_GUI:
        return "PLUGIN_THREAD_LV2_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_VST_GUI:
        return "PLUGIN_THREAD_VST_GUI";
    case CarlaPluginThread::PLUGIN_THREAD_BRIDGE:
        return "PLUGIN_THREAD_BRIDGE";
    }

    qWarning("CarlaPluginThread::PluginThreadMode2str(%i) - invalid mode", mode);
    return nullptr;
}

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine_, CarlaBackend::CarlaPlugin* const plugin_, const PluginThreadMode mode_, QObject* const parent) :
    QThread(parent),
    engine(engine_),
    plugin(plugin_),
    mode(mode_)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(plugin:\"%s\", engine:\"%s\", %s)", plugin->name(), engine->getName(), PluginThreadMode2str(mode));

    m_process = nullptr;
}

CarlaPluginThread::~CarlaPluginThread()
{
    if (m_process)
        delete m_process;
}

void CarlaPluginThread::setOscData(const char* const binary, const char* const label, const char* const data1)
{
    m_binary = QString(binary);
    m_label  = QString(label);
    m_data1  = QString(data1);
}

void CarlaPluginThread::run()
{
    qDebug("CarlaPluginThread::run()");

    if (! m_process)
        m_process = new QProcess(nullptr);

    m_process->setProcessChannelMode(QProcess::ForwardedChannels);

    QStringList arguments;

    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPath()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* label    */ arguments << m_label;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPath()).arg(plugin->id());
        /* URI      */ arguments << m_label;
        /* ui-URI   */ arguments << m_data1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPath()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
    {
        const char* name = plugin->name();

        if (! name)
            name = "(none)";

        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscServerPath()).arg(plugin->id());
        /* stype    */ arguments << m_data1;
        /* filename */ arguments << plugin->filename();
        /* name     */ arguments << name;
        /* label    */ arguments << m_label;
        break;
    }
    }

    qDebug() << m_binary;
    qDebug() << arguments;

    m_process->start(m_binary, arguments);
    m_process->waitForStarted();

    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
    case PLUGIN_THREAD_VST_GUI:
        if (plugin->showOscGui())
        {
            m_process->waitForFinished(-1);

            if (m_process->exitCode() == 0)
            {
                // Hide
                engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
                qWarning("CarlaPluginThread::run() - GUI closed");
            }
            else
            {
                // Kill
                engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), -1, 0, 0.0);
                qWarning("CarlaPluginThread::run() - GUI crashed");
                break;
            }
        }
        else
        {
            qDebug("CarlaPluginThread::run() - GUI timeout");
            engine->callback(CarlaBackend::CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
        }
        break;

    case PLUGIN_THREAD_BRIDGE:
        qDebug("CarlaPluginThread::run() - bridge starting...");
        m_process->waitForFinished(-1);
        qDebug("CarlaPluginThread::run() - bridge ended");

#ifdef DEBUG
        if (m_process->exitCode() == 0)
            qDebug("CarlaPluginThread::run() - bridge closed");
        else
            qDebug("CarlaPluginThread::run() - bridge crashed");

        qDebug("%s", QString(m_process->readAllStandardOutput()).toUtf8().constData());
#endif

        break;
    }
}

#endif

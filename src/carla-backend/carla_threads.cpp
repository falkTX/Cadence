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

// --------------------------------------------------------------------------------------------------------
// CarlaCheckThread

CarlaCheckThread::CarlaCheckThread(CarlaBackend::CarlaEngine* const engine_, QObject* parent) :
    QThread(parent),
    engine(engine_)
{
    qDebug("CarlaCheckThread::CarlaCheckThread(%p)", parent);
}

void CarlaCheckThread::stopNow()
{
    m_stopNow = true;

    if (! wait(200))
        quit();

    if (isRunning() && ! wait(200))
        terminate();
}

void CarlaCheckThread::run()
{
    qDebug("CarlaCheckThread::run()");

    m_stopNow = false;
    while (engine->isRunning() && ! m_stopNow)
    {
        for (unsigned short i=0; i < CarlaBackend::MAX_PLUGINS; i++)
        {
            CarlaBackend::CarlaPlugin* const plugin = engine->getPluginByIndex(i);

            if (plugin && plugin->enabled())
            {
                // --------------------------------------------------------------------------------------------------------
                // Process postponed events

                plugin->postEventsRun();

                // --------------------------------------------------------------------------------------------------------
                // Update parameters (OSC)

                plugin->updateOscParameterOutputs();

                // --------------------------------------------------------------------------------------------------------
                // Send peak values (OSC)

                if (engine->isOscControllerRegisted())
                {
                    const unsigned short id = plugin->id();

                    if (plugin->audioInCount() > 0)
                    {
                        engine->osc_send_set_input_peak_value(id, 1, engine->getInputPeak(id, 0));
                        engine->osc_send_set_input_peak_value(id, 2, engine->getInputPeak(id, 1));
                    }
                    if (plugin->audioOutCount() > 0)
                    {
                        engine->osc_send_set_output_peak_value(id, 1, engine->getOutputPeak(id, 0));
                        engine->osc_send_set_output_peak_value(id, 2, engine->getOutputPeak(id, 1));
                    }
                }
            }
        }

        msleep(50);
    }
}

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

const char* pluginthreadmode2str(CarlaPluginThread::PluginThreadMode mode)
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

    qWarning("CarlaPluginThread::pluginthreadmode2str(%i) - invalid mode", mode);
    return nullptr;
}

CarlaPluginThread::CarlaPluginThread(CarlaBackend::CarlaEngine* const engine_, CarlaBackend::CarlaPlugin* const plugin_, PluginThreadMode mode_, QObject* parent) :
    QThread(parent),
    engine(engine_),
    plugin(plugin_),
    mode(mode_)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(%s, %s, %s)", plugin->name(), engine->getName(), pluginthreadmode2str(mode));

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

    if (m_process == nullptr)
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
        /* label    */ arguments << m_label;
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
        m_process->waitForFinished(-1);

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

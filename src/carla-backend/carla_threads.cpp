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
    using namespace CarlaBackend;

    uint32_t j, k;
    double value;
    const ParameterData* paramData;
    PluginPostEvent postEvents[MAX_POST_EVENTS];

    m_stopNow = false;
    while (/*is_engine_running() &&*/ ! m_stopNow)
    {
        for (unsigned short i=0; i < MAX_PLUGINS; i++)
        {
            CarlaPlugin* const plugin = engine->getPluginByIndex(i);

            if (plugin && plugin->enabled())
            {
                // --------------------------------------------------------------------------------------------------------
                // Process postponed events

                // Make a safe copy of events, and clear them
                plugin->postEventsCopy(postEvents);

                const OscData* const osc_data = plugin->oscData();

                // Process events now
                for (j=0; j < MAX_POST_EVENTS; j++)
                {
                    if (postEvents[j].type == PluginPostEventNull)
                      break;

                    switch (postEvents[j].type)
                    {
                    case PluginPostEventDebug:
                        engine->callback(CALLBACK_DEBUG, plugin->id(), postEvents[j].index, 0, postEvents[j].value);
                        break;

                    case PluginPostEventParameterChange:
                        // Update OSC based UIs
                        //osc_send_control(osc_data, postEvents[j].index, postEvents[j].value);

                        // Update OSC control client
                        //osc_global_send_set_parameter_value(plugin->id(), postEvents[j].index, postEvents[j].value);

                        // Update Host
                        engine->callback(CALLBACK_PARAMETER_CHANGED, plugin->id(), postEvents[j].index, 0, postEvents[j].value);

                        break;

                    case PluginPostEventProgramChange:
                        // Update OSC based UIs
                        //osc_send_program(osc_data, postEvents[j].index);

                        // Update OSC control client
                        //osc_global_send_set_program(plugin->id(), postEvents[j].index);

                        //for (k=0; k < plugin->parameterCount(); k++)
                        //    osc_global_send_set_default_value(plugin->id(), k, plugin->parameterRanges(k)->def);

                        // Update Host
                        engine->callback(CALLBACK_PROGRAM_CHANGED, plugin->id(), postEvents[j].index, 0, 0.0);

                        break;

                    case PluginPostEventMidiProgramChange:
                        if (postEvents[j].index < (int32_t)plugin->midiProgramCount())
                        {
                            if (postEvents[j].index >= 0)
                            {
                                const midi_program_t* const midiprog = plugin->midiProgramData(postEvents[j].index);

                                // Update OSC based UIs
                                //osc_send_midi_program(osc_data, midiprog->bank, midiprog->program, (plugin->type() == PLUGIN_DSSI));
                            }

                            // Update OSC control client
                            //osc_global_send_set_midi_program(plugin->id(), postEvents[j].index);

                            //for (k=0; k < plugin->parameterCount(); k++)
                            //    osc_global_send_set_default_value(plugin->id(), k, plugin->parameterRanges(k)->def);

                            // Update Host
                            engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, plugin->id(), postEvents[j].index, 0, 0.0);
                        }

                        break;

                    case PluginPostEventNoteOn:
                        // Update OSC based UIs
                        //if (plugin->type() == PLUGIN_LV2)
                        //    osc_send_note_on(osc_data, plugin->id(), post_events[j].index, post_events[j].value);

                        // Update OSC control client
                        //osc_global_send_note_on(plugin->id(), postEvents[j].index, postEvents[j].value);

                        // Update Host
                        engine->callback(CALLBACK_NOTE_ON, plugin->id(), postEvents[j].index, postEvents[j].value, 0.0);

                        break;

                    case PluginPostEventNoteOff:
                        // Update OSC based UIs
                        //if (plugin->type() == PLUGIN_LV2)
                        //    osc_send_note_off(osc_data, plugin->id(), post_events[j].index, 0);

                        // Update OSC control client
                        //osc_global_send_note_off(plugin->id(), postEvents[j].index);

                        // Update Host
                        engine->callback(CALLBACK_NOTE_OFF, plugin->id(), postEvents[j].index, 0, 0.0);

                        break;

                    default:
                        break;
                    }
                }

                // --------------------------------------------------------------------------------------------------------
                // Update ports

                // Check if it needs update
                bool update_ports_gui = (osc_data->target && (plugin->hints() & PLUGIN_IS_BRIDGE) == 0);

#if 0
                if (osc_global_registered() == false && update_ports_gui == false)
                    continue;

                // Update
                for (j=0; j < plugin->parameterCount(); j++)
                {
                    paramData = plugin->parameterData(j);

                    if (paramData->type == PARAMETER_OUTPUT && (paramData->hints & PARAMETER_IS_AUTOMABLE) > 0)
                    {
                        value = plugin->getParameterValue(j);

                        if (update_ports_gui)
                            osc_send_control(osc_data, paramData->rindex, value);

                        osc_global_send_set_parameter_value(plugin->id(), j, value);
                    }
                }

                // --------------------------------------------------------------------------------------------------------
                // Send peak values (OSC)

                if (osc_global_registered())
                {
                    if (plugin->audioInCount() > 0)
                    {
                        osc_global_send_set_input_peak_value(plugin->id(), 1, engine->getInputPeak(plugin->id(), 0));
                        osc_global_send_set_input_peak_value(plugin->id(), 2, engine->getInputPeak(plugin->id(), 1));
                    }
                    if (plugin->audioOutCount() > 0)
                    {
                        osc_global_send_set_output_peak_value(plugin->id(), 1, engine->getOutputPeak(plugin->id(), 0));
                        osc_global_send_set_output_peak_value(plugin->id(), 2, engine->getOutputPeak(plugin->id(), 1));
                    }
                }
#endif
            }
        }
        msleep(50);
    }
}

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

const char* CarlaPluginThread::pluginthreadmode2str(PluginThreadMode mode)
{
    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
        return "PLUGIN_THREAD_DSSI_GUI";
    case PLUGIN_THREAD_LV2_GUI:
        return "PLUGIN_THREAD_LV2_GUI";
    case PLUGIN_THREAD_VST_GUI:
        return "PLUGIN_THREAD_VST_GUI";
    case PLUGIN_THREAD_BRIDGE:
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
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscUrl()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* label    */ arguments << m_label;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscUrl()).arg(plugin->id());
        /* URI      */ arguments << m_label;
        /* ui-URI   */ arguments << m_data1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_VST_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscUrl()).arg(plugin->id());
        /* filename */ arguments << plugin->filename();
        /* label    */ arguments << m_label;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(plugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
    {
        const char* name = plugin->name();

        if (! name)
            name = "(none)";

        /* osc_url  */ arguments << QString("%1/%2").arg(engine->getOscUrl()).arg(plugin->id());
        /* stype    */ arguments << m_data1;
        /* filename */ arguments << plugin->filename();
        /* name     */ arguments << name;
        /* label    */ arguments << m_label;
        break;
    }

    default:
        break;
    }

    qWarning() << m_binary;
    qWarning() << arguments;

    m_process->start(m_binary, arguments);
    m_process->waitForStarted();

    switch (mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
        if (/*plugin->showOscGui()*/1)
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

        if (m_process->exitCode() == 0)
            qDebug("CarlaPluginThread::run() - bridge closed");
        else
            qDebug("CarlaPluginThread::run() - bridge crashed");

        qDebug("%s", QString(m_process->readAllStandardOutput()).toUtf8().constData());

        break;

    default:
        break;
    }
}

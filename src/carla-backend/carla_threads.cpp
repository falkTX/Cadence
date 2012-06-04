/*
 * JACK Backend code for Carla
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

#include <QtCore/QProcess>

#ifndef CARLA_BACKEND_NO_NAMESPACE
using namespace CarlaBackend;
#endif

// --------------------------------------------------------------------------------------------------------
// CarlaCheckThread

CarlaCheckThread::CarlaCheckThread(QObject *parent) :
    QThread(parent)
{
    qDebug("CarlaCheckThread::CarlaCheckThread(%p)", parent);
}

void CarlaCheckThread::stopNow()
{
    m_stopNow = true;

    while (isRunning())
    {
        msleep(10);
    }
}

void CarlaCheckThread::run()
{
    qDebug("CarlaCheckThread::run()");

    uint32_t j, k;
    double value;
    const ParameterData* paramData;
    PluginPostEvent postEvents[MAX_POST_EVENTS];

    m_stopNow = false;
    while (is_engine_running() && ! m_stopNow)
    {
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* plugin = CarlaPlugins[i];
            if (plugin && plugin->enabled())
            {
                // --------------------------------------------------------------------------------------------------------
                // Process postponed events

                // Make a safe copy of events, and clear them
                plugin->post_events_copy(postEvents);

                OscData* osc_data = plugin->osc_data();

                // Process events now
                for (j=0; j < MAX_POST_EVENTS; j++)
                {
                    if (postEvents[j].type != PluginPostEventNull)
                    {
                        switch (postEvents[j].type)
                        {
                        case PluginPostEventDebug:
                            callback_action(CALLBACK_DEBUG, plugin->id(), postEvents[j].index, 0, postEvents[j].value);
                            break;

                        case PluginPostEventParameterChange:
                            // Update OSC based UIs
                            osc_send_control(osc_data, postEvents[j].index, postEvents[j].value);

                            // Update OSC control client
                            osc_global_send_set_parameter_value(plugin->id(), postEvents[j].index, postEvents[j].value);

                            // Update Host
                            callback_action(CALLBACK_PARAMETER_CHANGED, plugin->id(), postEvents[j].index, 0, postEvents[j].value);

                            break;

                        case PluginPostEventProgramChange:
                            // Update OSC based UIs
                            osc_send_program(osc_data, postEvents[j].index);

                            // Update OSC control client
                            osc_global_send_set_program(plugin->id(), postEvents[j].index);

                            for (k=0; k < plugin->param_count(); k++)
                                osc_global_send_set_default_value(plugin->id(), k, plugin->param_ranges(k)->def);

                            // Update Host
                            callback_action(CALLBACK_PROGRAM_CHANGED, plugin->id(), postEvents[j].index, 0, 0.0);

                            break;

                        case PluginPostEventMidiProgramChange:
                            if (postEvents[j].index < (int32_t)plugin->midiprog_count())
                            {
                                MidiProgramInfo midiprog = { false, 0, 0, nullptr };
                                plugin->get_midi_program_info(&midiprog, postEvents[j].index);

                                // Update OSC based UIs
                                osc_send_midi_program(osc_data, midiprog.bank, midiprog.program, (plugin->type() == PLUGIN_DSSI));

                                // Update OSC control client
                                osc_global_send_set_midi_program(plugin->id(), postEvents[j].index);

                                for (k=0; k < plugin->param_count(); k++)
                                    osc_global_send_set_default_value(plugin->id(), k, plugin->param_ranges(k)->def);

                                // Update Host
                                callback_action(CALLBACK_MIDI_PROGRAM_CHANGED, plugin->id(), postEvents[j].index, 0, 0.0);
                            }

                            break;

                        case PluginPostEventNoteOn:
                            // Update OSC based UIs
                            //if (plugin->type() == PLUGIN_LV2)
                            //    osc_send_note_on(osc_data, plugin->id(), post_events[j].index, post_events[j].value);

                            // Update OSC control client
                            osc_global_send_note_on(plugin->id(), postEvents[j].index, postEvents[j].value);

                            // Update Host
                            callback_action(CALLBACK_NOTE_ON, plugin->id(), postEvents[j].index, postEvents[j].value, 0.0);

                            break;

                        case PluginPostEventNoteOff:
                            // Update OSC based UIs
                            //if (plugin->type() == PLUGIN_LV2)
                            //    osc_send_note_off(osc_data, plugin->id(), post_events[j].index, 0);

                            // Update OSC control client
                            osc_global_send_note_off(plugin->id(), postEvents[j].index);

                            // Update Host
                            callback_action(CALLBACK_NOTE_OFF, plugin->id(), postEvents[j].index, 0, 0.0);

                            break;

                        case PluginPostEventCustom:
                            plugin->run_custom_event(&postEvents[j]);
                            break;

                        default:
                            break;
                        }
                    }
                }

                // --------------------------------------------------------------------------------------------------------
                // Update ports

                // Check if it needs update
                bool update_ports_gui = (osc_data->target && (plugin->hints() & PLUGIN_IS_BRIDGE) == 0);

                if (osc_global_registered() == false && update_ports_gui == false)
                    continue;

                // Update
                for (j=0; j < plugin->param_count(); j++)
                {
                    paramData = plugin->param_data(j);

                    if (paramData->type == PARAMETER_OUTPUT && (paramData->hints & PARAMETER_IS_AUTOMABLE) > 0)
                    {
                        value = plugin->get_parameter_value(j);

                        if (update_ports_gui)
                            osc_send_control(osc_data, paramData->rindex, value);

                        osc_global_send_set_parameter_value(plugin->id(), j, value);
                    }
                }

                // --------------------------------------------------------------------------------------------------------
                // Send peak values (OSC)

                if (osc_global_registered())
                {
                    if (plugin->ain_count() > 0)
                    {
                        osc_global_send_set_input_peak_value(plugin->id(), 1, ains_peak[ (plugin->id() * 2) + 0 ]);
                        osc_global_send_set_input_peak_value(plugin->id(), 2, ains_peak[ (plugin->id() * 2) + 1 ]);
                    }
                    if (plugin->aout_count() > 0)
                    {
                        osc_global_send_set_output_peak_value(plugin->id(), 1, aouts_peak[ (plugin->id() * 2) + 0 ]);
                        osc_global_send_set_output_peak_value(plugin->id(), 2, aouts_peak[ (plugin->id() * 2) + 1 ]);
                    }
                }
            }
        }
        msleep(50);
    }
}

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

CarlaPluginThread::CarlaPluginThread(CarlaPlugin* plugin, PluginThreadMode mode) :
    QThread (nullptr),
    m_plugin (plugin),
    m_mode (mode)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(%p, %i)", plugin, mode);

    m_process = nullptr;
}

CarlaPluginThread::~CarlaPluginThread()
{
    if (m_process)
        delete m_process;
}

void CarlaPluginThread::setOscData(const char* binary, const char* label, const char* data1)
{
    m_binary = QString(binary);
    m_label  = QString(label);
    m_data1  = QString(data1);
}

#include <QtCore/QDebug>

void CarlaPluginThread::run()
{
    qDebug("CarlaPluginThread::run()");

    if (m_process == nullptr)
        m_process = new QProcess(nullptr);

    m_process->setProcessChannelMode(QProcess::ForwardedChannels);

    QStringList arguments;

    switch (m_mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(get_host_osc_url()).arg(m_plugin->id());
        /* filename */ arguments << m_plugin->filename();
        /* label    */ arguments << m_label;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(m_plugin->name());
        break;

    case PLUGIN_THREAD_LV2_GUI:
        /* osc_url  */ arguments << QString("%1/%2").arg(get_host_osc_url()).arg(m_plugin->id());
        /* URI      */ arguments << m_label;
        /* ui-URI   */ arguments << m_data1;
        /* ui-title */ arguments << QString("%1 (GUI)").arg(m_plugin->name());
        break;

    case PLUGIN_THREAD_BRIDGE:
        /* osc_url  */ arguments << QString("%1/%2").arg(get_host_osc_url()).arg(m_plugin->id());
        /* stype    */ arguments << m_data1;
        /* filename */ arguments << m_plugin->filename();
        /* label    */ arguments << m_label;
        break;

    default:
        break;
    }

    qWarning() << m_binary;
    qWarning() << arguments;

    m_process->start(m_binary, arguments);
    m_process->waitForStarted();

    switch (m_mode)
    {
    case PLUGIN_THREAD_DSSI_GUI:
    case PLUGIN_THREAD_LV2_GUI:
        if (m_plugin->show_osc_gui())
        {
            m_process->waitForFinished(-1);

            if (m_process->exitCode() == 0)
            {
                // Hide
                callback_action(CALLBACK_SHOW_GUI, m_plugin->id(), 0, 0, 0.0);
                qWarning("CarlaPluginThread::run() - GUI closed");
            }
            else
            {
                // Kill
                callback_action(CALLBACK_SHOW_GUI, m_plugin->id(), -1, 0, 0.0);
                qWarning("CarlaPluginThread::run() - GUI crashed");
                break;
            }
        }
        else
        {
            qDebug("CarlaPluginThread::run() - GUI timeout");
            callback_action(CALLBACK_SHOW_GUI, m_plugin->id(), 0, 0, 0.0);
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

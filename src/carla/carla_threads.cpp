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

// --------------------------------------------------------------------------------------------------------
// CarlaCheckThread

CarlaCheckThread::CarlaCheckThread(QObject *parent) :
    QThread(parent)
{
    qDebug("CarlaCheckThread::CarlaCheckThread(%p)", parent);
}

void CarlaCheckThread::run()
{
    qDebug("CarlaCheckThread::run()");

    uint32_t j;
    PluginPostEvent post_events[MAX_POSTEVENTS];

    while (carla_is_engine_running())
    {
        for (unsigned short i=0; i<MAX_PLUGINS; i++)
        {
            CarlaPlugin* plugin = CarlaPlugins[i];
            if (plugin && plugin->id() >= 0)
            {
                // --------------------------------------------------------------------------------------------------------
                // Process postponed events

                // Make a safe copy of events, and clear them
                plugin->post_events_copy(post_events);

                // Process events now
                for (j=0; j < MAX_POSTEVENTS; j++)
                {
                    if (post_events[j].valid)
                    {
                        switch (post_events[j].type)
                        {
                        case PostEventDebug:
                            callback_action(CALLBACK_DEBUG, plugin->id(), post_events[j].index, 0, post_events[j].value);
                            break;

                        case PostEventParameterChange:
                            //osc_send_set_parameter_value(&global_osc_data, plugin->id, post_events[j].index, post_events[j].value);
                            callback_action(CALLBACK_PARAMETER_CHANGED, plugin->id(), post_events[j].index, 0, post_events[j].value);

                            //if (plugin->hints & PLUGIN_IS_BRIDGE)
                            //    osc_send_control(&plugin->osc.data, post_events[j].index, post_events[j].value);

                            break;

                        case PostEventProgramChange:
                            //osc_send_set_program(&global_osc_data, plugin->id, post_events[j].index);
                            callback_action(CALLBACK_PROGRAM_CHANGED, plugin->id(), post_events[j].index, 0, 0.0);

                            //if (plugin->hints & PLUGIN_IS_BRIDGE)
                            //    osc_send_program(&plugin->osc.data, post_events[j].index);

                            //for (uint32_t k=0; k < plugin->param.count; k++)
                            //    osc_send_set_default_value(&global_osc_data, plugin->id, k, plugin->param.ranges[k].def);

                            break;

                        case PostEventMidiProgramChange:
                            //osc_send_set_midi_program(&global_osc_data, plugin->id, post_events[j].index);
                            callback_action(CALLBACK_MIDI_PROGRAM_CHANGED, plugin->id(), post_events[j].index, 0, 0.0);

                            //if (plugin->type == PLUGIN_DSSI)
                            //    osc_send_program_as_midi(&plugin->osc.data, plugin->midiprog.data[post_events[j].index].bank, plugin->midiprog.data[post_events[j].index].program);

                            //if (plugin->hints & PLUGIN_IS_BRIDGE)
                            //    osc_send_midi_program(&plugin->osc.data, plugin->midiprog.data[post_events[j].index].bank, plugin->midiprog.data[post_events[j].index].program);

                            //for (uint32_t k=0; k < plugin->param.count; k++)
                            //    osc_send_set_default_value(&global_osc_data, plugin->id, k, plugin->param.ranges[k].def);

                            break;

                        case PostEventNoteOn:
                            //osc_send_note_on(&global_osc_data, plugin->id, post_events[j].index, post_events[j].value);
                            callback_action(CALLBACK_NOTE_ON, plugin->id(), post_events[j].index, post_events[j].value, 0.0);

                            //if (plugin->hints & PLUGIN_IS_BRIDGE)
                            //    osc_send_note_on(&plugin->osc.data, plugin->id, post_events[j].index, post_events[j].value);

                            break;

                        case PostEventNoteOff:
                            //osc_send_note_off(&global_osc_data, plugin->id, post_events[j].index, 0);
                            callback_action(CALLBACK_NOTE_OFF, plugin->id(), post_events[j].index, 0, 0.0);

                            //if (plugin->hints & PLUGIN_IS_BRIDGE)
                            //    osc_send_note_off(&plugin->osc.data, plugin->id, post_events[j].index, 0);

                            break;

                        default:
                            break;
                        }
                    }
                }

                // --------------------------------------------------------------------------------------------------------
                // Idle plugin
//                if (plugin->gui.visible && plugin->gui.type != GUI_EXTERNAL_OSC)
//                    plugin->idle_gui();

                // --------------------------------------------------------------------------------------------------------
                // Update ports

                // Check if it needs update
//                bool update_ports_gui = (plugin->gui.visible && plugin->gui.type == GUI_EXTERNAL_OSC && plugin->osc.data.target);

//                if (!global_osc_data.target && !update_ports_gui)
//                    continue;

//                // Update
//                for (j=0; j < plugin->param.count; j++)
//                {
//                    if (plugin->param.data[j].type == PARAMETER_OUTPUT && (plugin->param.data[j].hints & PARAMETER_IS_AUTOMABLE) > 0)
//                    {
//                        if (update_ports_gui)
//                            osc_send_control(&plugin->osc.data, plugin->param.data[j].rindex, plugin->param.buffers[j]);

//                        osc_send_set_parameter_value(&global_osc_data, plugin->id, j, plugin->param.buffers[j]);
//                    }
//                }

                // --------------------------------------------------------------------------------------------------------
                // Send peak values (OSC)
//                if (global_osc_data.target)
//                {
//                    if (plugin->ain.count > 0)
//                    {
//                        osc_send_set_input_peak_value(&global_osc_data, plugin->id, 1, ains_peak[(plugin->id*2)+0]);
//                        osc_send_set_input_peak_value(&global_osc_data, plugin->id, 2, ains_peak[(plugin->id*2)+1]);
//                    }
//                    if (plugin->aout.count > 0)
//                    {
//                        osc_send_set_output_peak_value(&global_osc_data, plugin->id, 1, aouts_peak[(plugin->id*2)+0]);
//                        osc_send_set_output_peak_value(&global_osc_data, plugin->id, 2, aouts_peak[(plugin->id*2)+1]);
//                    }
//                }
            }
        }
        usleep(50000); // 50 ms
    }
}

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

CarlaPluginThread::CarlaPluginThread(QObject *parent) :
    QThread(parent)
{
    qDebug("CarlaPluginThread::CarlaPluginThread(%p)", parent);
    //m_process = new QProcess(parent);
}

CarlaPluginThread::~CarlaPluginThread()
{
    // TODO - kill process
    //delete m_process;
}

void CarlaPluginThread::set_plugin(CarlaPlugin* plugin)
{
    m_plugin = plugin;
}

void CarlaPluginThread::run()
{
}

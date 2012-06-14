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

#include "carla_plugin.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

struct BridgeParamInfo {
    QString name;
    QString unit;
};

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(BinaryType btype, PluginType ptype, unsigned short id) : CarlaPlugin(id),
        m_binary(btype)
    {
        qDebug("BridgePlugin::BridgePlugin()");
        m_type   = ptype;
        m_hints  = PLUGIN_IS_BRIDGE;
        m_label  = nullptr;

        m_info.category  = PLUGIN_CATEGORY_NONE;
        m_info.hints     = 0;
        m_info.name      = nullptr;
        m_info.maker     = nullptr;
        m_info.unique_id = 0;
        m_info.ains  = 0;
        m_info.aouts = 0;
        m_info.mins  = 0;
        m_info.mouts = 0;

        m_thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_BRIDGE);
    }

    ~BridgePlugin()
    {
        qDebug("BridgePlugin::~BridgePlugin()");

        if (osc.data.target)
        {
            osc_send_hide(&osc.data);
            osc_send_quit(&osc.data);
        }

        // Wait a bit first, try safe quit else force kill
        if (m_thread->isRunning())
        {
            if (m_thread->wait(2000) == false)
                m_thread->quit();

            if (m_thread->isRunning() && m_thread->wait(1000) == false)
            {
                qWarning("Failed to properly stop Bridge thread");
                m_thread->terminate();
            }
        }

        delete m_thread;

        osc_clear_data(&osc.data);

        if (m_label)
            free((void*)m_label);

        if (m_info.name)
            free((void*)m_info.name);

        if (m_info.maker)
            free((void*)m_info.maker);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return m_info.category;
    }

    long unique_id()
    {
        return m_info.unique_id;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t ain_count()
    {
        return m_info.ains;
    }

    uint32_t aout_count()
    {
        return m_info.aouts;
    }

    uint32_t min_count()
    {
        return m_info.mins;
    }

    uint32_t mout_count()
    {
        return m_info.mouts;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double get_parameter_value(uint32_t param_id)
    {
        return param_buffers[param_id];
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, m_label, STR_MAX);
    }

    void get_maker(char* buf_str)
    {
        strncpy(buf_str, m_info.maker, STR_MAX);
    }

    void get_copyright(char* buf_str)
    {
        strncpy(buf_str, m_info.maker, STR_MAX);
    }

    void get_real_name(char* buf_str)
    {
        strncpy(buf_str, m_info.name, STR_MAX);
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        strncpy(buf_str, param_info[param_id].name.toUtf8().constData(), STR_MAX);
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        strncpy(buf_str, param_info[param_id].unit.toUtf8().constData(), STR_MAX);
    }

    void get_gui_info(GuiInfo* info)
    {
        info->type = GUI_NONE;
        info->resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    int set_osc_bridge_info(PluginBridgeInfoType intoType, lo_arg** argv)
    {
        qDebug("set_osc_bridge_info(%i, %p)", intoType, argv);

        //        PluginBridgeProgramCountInfo,
        //        PluginBridgeMidiProgramCountInfo,
        //        PluginBridgePluginInfo,
        //        PluginBridgeParameterInfo,

        //        PluginBridgeProgramInfo,
        //        PluginBridgeMidiProgramInfo,

        switch (intoType)
        {
        case PluginBridgeAudioCount:
        {
            m_info.ains  = argv[0]->i;
            m_info.aouts = argv[1]->i;
            break;
        }

        case PluginBridgeMidiCount:
        {
            m_info.mins  = argv[0]->i;
            m_info.mouts = argv[1]->i;
            break;
        }

        case PluginBridgeParameterCount:
        {
            // delete old data
            if (param.count > 0)
            {
                delete[] param.data;
                delete[] param.ranges;
                delete[] param_buffers;
            }

            // create new if needed
            param.count = argv[2]->i;

            if (param.count > 0 && param.count < carla_options.max_parameters)
            {
                param.data    = new ParameterData[param.count];
                param.ranges  = new ParameterRanges[param.count];
                param_buffers = new double[param.count];
                param_info    = new BridgeParamInfo[param.count];
            }
            else
                param.count = 0;

            // initialize
            for (uint32_t i=0; i < param.count; i++)
            {
                param.data[i].type   = PARAMETER_UNKNOWN;
                param.data[i].index  = -1;
                param.data[i].rindex = -1;
                param.data[i].hints  = 0;
                param.data[i].midi_channel = 0;
                param.data[i].midi_cc = -1;

                param.ranges[i].def  = 0.0;
                param.ranges[i].min  = 0.0;
                param.ranges[i].max  = 1.0;
                param.ranges[i].step = 0.01;
                param.ranges[i].step_small = 0.0001;
                param.ranges[i].step_large = 0.1;

                param_buffers[i] = 0.0;

                param_info[i].name = QString();
                param_info[i].unit = QString();
            }
            break;
        }

        case PluginBridgeParameterInfo:
        {
            int index = argv[0]->i;
            if (index >= 0 && index < (int32_t)param.count)
            {
                param_info[index].name = QString((const char*)&argv[1]->s);
                param_info[index].unit = QString((const char*)&argv[2]->s);
            }
            break;
        }

        case PluginBridgeParameterDataInfo:
        {
            int index = argv[1]->i;
            if (index >= 0 && index < (int32_t)param.count)
            {
                param.data[index].type    = static_cast<ParameterType>(argv[0]->i);
                param.data[index].index   = index;
                param.data[index].rindex  = argv[2]->i;
                param.data[index].hints   = argv[3]->i;
                param.data[index].midi_channel = argv[4]->i;
                param.data[index].midi_cc = argv[5]->i;
            }
            break;
        }

        case PluginBridgeParameterRangesInfo:
        {
            int index = argv[0]->i;
            if (index >= 0 && index < (int32_t)param.count)
            {
                param.ranges[index].def  = argv[1]->f;
                param.ranges[index].min  = argv[2]->f;
                param.ranges[index].max  = argv[3]->f;
                param.ranges[index].step = argv[4]->f;
                param.ranges[index].step_small = argv[5]->f;
                param.ranges[index].step_large = argv[6]->f;
            }
            break;
        }

        case PluginBridgeUpdateNow:
            callback_action(CALLBACK_RELOAD_ALL, m_id, 0, 0, 0.0);
            break;

        default:
            break;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        param_buffers[param_id] = fix_parameter_value(value, param.ranges[param_id]);

        if (param.data[param_id].type == PARAMETER_INPUT)
            osc_send_control(&osc.data, param.data[param_id].rindex, value);

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void show_gui(bool yesno)
    {
        if (yesno)
            osc_send_show(&osc.data);
        else
            osc_send_hide(&osc.data);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (m_info.aouts > 0 && (m_info.ains == m_info.aouts || m_info.ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (m_info.aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (m_info.aouts >= 2 && m_info.aouts % 2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        m_hints |= m_info.hints;
    }

    void delete_buffers()
    {
        qDebug("BridgePlugin::delete_buffers() - start");

        if (param.count > 0)
        {
            delete[] param_buffers;
            delete[] param_info;
        }

        param_buffers = nullptr;
        param_info    = nullptr;

        qDebug("BridgePlugin::delete_buffers() - end");
    }

    bool init(const char* filename, const char* label, PluginBridgeInfo* info)
    {
        if (info)
        {
            m_info.category  = info->category;
            m_info.hints     = info->hints;
            m_info.unique_id = info->unique_id;
            m_info.ains  = info->ains;
            m_info.aouts = info->aouts;
            m_info.mins  = info->mins;
            m_info.mouts = info->mouts;

            m_info.name  = strdup(info->name);
            m_info.maker = strdup(info->maker);

            m_label = strdup(label);
            m_name  = get_unique_name(info->name);
            m_filename = strdup(filename);

            const char* bridge_binary = binarytype2str(m_binary);

            if (bridge_binary)
            {
                m_thread->setOscData(bridge_binary, label, plugintype2str(m_type));
                m_thread->start();

                return true;
            }
            else
                set_last_error("Bridge not possible, bridge-binary not found");
        }
        else
            set_last_error("Invalid bridge info, cannot continue");

        return false;
    }

private:
    const char* m_label;
    const BinaryType m_binary;
    PluginBridgeInfo m_info;
    CarlaPluginThread* m_thread;

    double* param_buffers;
    BridgeParamInfo* param_info;
};

short add_plugin_bridge(BinaryType btype, PluginType ptype, const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_bridge(%i, %i, %s, %s, %p)", btype, ptype, filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        BridgePlugin* plugin = new BridgePlugin(btype, ptype, id);

        if (plugin->init(filename, label, (PluginBridgeInfo*)extra_stuff))
        {
            plugin->reload();

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

            plugin->osc_register_new();
        }
        else
        {
            delete plugin;
            id = -1;
        }
    }
    else
        set_last_error("Maximum number of plugins reached");

    return id;
}

CARLA_BACKEND_END_NAMESPACE

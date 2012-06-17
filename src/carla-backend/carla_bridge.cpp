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

#include <QtCore/QFile>
#include <QtCore/QTextStream>

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

struct BridgeParamInfo {
    double value;
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

        initiated = saved = false;

        info.ains  = 0;
        info.aouts = 0;
        info.mins  = 0;
        info.mouts = 0;

        info.category = PLUGIN_CATEGORY_NONE;
        info.uniqueId = 0;

        info.label = nullptr;
        info.maker = nullptr;
        info.chunk = nullptr;
        info.copyright = nullptr;

        params = nullptr;

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

        if (info.label)
            free((void*)info.label);

        if (info.maker)
            free((void*)info.maker);

        if (info.copyright)
            free((void*)info.copyright);

        if (info.chunk)
            free((void*)info.chunk);
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return info.category;
    }

    long unique_id()
    {
        return info.uniqueId;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t ain_count()
    {
        return info.ains;
    }

    uint32_t aout_count()
    {
        return info.aouts;
    }

    uint32_t min_count()
    {
        return info.mins;
    }

    uint32_t mout_count()
    {
        return info.mouts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunk_data(void** data_ptr)
    {
        if (info.chunk)
        {
            static QByteArray chunk;
            chunk = QByteArray::fromBase64(info.chunk);
            *data_ptr = chunk.data();
            return chunk.size();
        }
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double get_parameter_value(uint32_t param_id)
    {
        return params[param_id].value;
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, info.label, STR_MAX);
    }

    void get_maker(char* buf_str)
    {
        strncpy(buf_str, info.maker, STR_MAX);
    }

    void get_copyright(char* buf_str)
    {
        strncpy(buf_str, info.copyright, STR_MAX);
    }

    void get_real_name(char* buf_str)
    {
        strncpy(buf_str, info.name, STR_MAX);
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        strncpy(buf_str, params[param_id].name.toUtf8().constData(), STR_MAX);
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        strncpy(buf_str, params[param_id].unit.toUtf8().constData(), STR_MAX);
    }

    void get_gui_info(GuiInfo* info)
    {
        if (m_hints & PLUGIN_HAS_GUI)
            info->type = GUI_EXTERNAL_OSC;
        else
            info->type = GUI_NONE;
        info->resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    int set_osc_bridge_info(PluginBridgeInfoType intoType, lo_arg** argv)
    {
        qDebug("set_osc_bridge_info(%i, %p)", intoType, argv);

        switch (intoType)
        {
        case PluginBridgeAudioCount:
        {
            int aIns   = argv[0]->i;
            int aOuts  = argv[1]->i;
            int aTotal = argv[2]->i;

            info.ains  = aIns;
            info.aouts = aOuts;

            Q_UNUSED(aTotal);
            break;
        }

        case PluginBridgeMidiCount:
        {
            int mIns   = argv[0]->i;
            int mOuts  = argv[1]->i;
            int mTotal = argv[2]->i;

            info.mins  = mIns;
            info.mouts = mOuts;

            Q_UNUSED(mTotal);
            break;
        }

        case PluginBridgeParameterCount:
        {
            int pIns   = argv[0]->i;
            int pOuts  = argv[1]->i;
            int pTotal = argv[2]->i;

            // delete old data
            if (param.count > 0)
            {
                delete[] param.data;
                delete[] param.ranges;
                delete[] params;
            }

            // create new if needed
            param.count = pTotal;

            if (param.count > 0 && param.count < carla_options.max_parameters)
            {
                param.data   = new ParameterData[param.count];
                param.ranges = new ParameterRanges[param.count];
                params       = new BridgeParamInfo[param.count];
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

                params[i].value = 0.0;
                params[i].name = QString();
                params[i].unit = QString();
            }

            Q_UNUSED(pIns);
            Q_UNUSED(pOuts);
            break;
        }

        case PluginBridgeProgramCount:
        {
            int count = argv[0]->i;

            // Delete old programs
            if (prog.count > 0)
            {
                for (uint32_t i=0; i < prog.count; i++)
                    free((void*)prog.names[i]);

                delete[] prog.names;
            }

            prog.count = 0;
            prog.names = nullptr;

            // Query new programs
            prog.count = count;

            if (prog.count > 0)
                prog.names = new const char* [prog.count];

            // Update names (NULL)
            for (uint32_t i=0; i < prog.count; i++)
                prog.names[i] = nullptr;

            break;
        }

        case PluginBridgeMidiProgramCount:
        {
            int count = argv[0]->i;

            // Delete old programs
            if (midiprog.count > 0)
            {
                for (uint32_t i=0; i < midiprog.count; i++)
                    free((void*)midiprog.data[i].name);

                delete[] midiprog.data;
            }

            midiprog.count = 0;
            midiprog.data  = nullptr;

            // Query new programs
            midiprog.count = count;

            if (midiprog.count > 0)
                midiprog.data = new midi_program_t [midiprog.count];

            // Update data (NULL)
            for (uint32_t i=0; i < midiprog.count; i++)
            {
                midiprog.data[i].bank    = 0;
                midiprog.data[i].program = 0;
                midiprog.data[i].name    = nullptr;
            }

            break;
        }

        case PluginBridgePluginInfo:
        {
            int category = argv[0]->i;
            int hints    = argv[1]->i;
            const char* name  = (const char*)&argv[2]->s;
            const char* label = (const char*)&argv[3]->s;
            const char* maker = (const char*)&argv[4]->s;
            const char* copyright = (const char*)&argv[5]->s;
            long unique_id = argv[6]->i;

            m_hints = hints | PLUGIN_IS_BRIDGE;
            info.category = (PluginCategory)category;
            info.uniqueId = unique_id;

            info.name  = strdup(name);
            info.label = strdup(label);
            info.maker = strdup(maker);
            info.copyright = strdup(copyright);

            m_name = get_unique_name(name);
        }

        case PluginBridgeParameterInfo:
        {
            int index = argv[0]->i;
            const char* name = (const char*)&argv[1]->s;
            const char* unit = (const char*)&argv[2]->s;

            if (index >= 0 && index < (int32_t)param.count)
            {
                params[index].name = QString(name);
                params[index].unit = QString(unit);
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

        case PluginBridgeProgramInfo:
        {
            int index = argv[0]->i;
            const char* name = (const char*)&argv[1]->s;

            prog.names[index] = strdup(name);
            break;
        }

        case PluginBridgeMidiProgramInfo:
        {
            int index   = argv[0]->i;
            int bank    = argv[1]->i;
            int program = argv[2]->i;
            const char* name = (const char*)&argv[3]->s;

            midiprog.data[index].bank    = bank;
            midiprog.data[index].program = program;
            midiprog.data[index].name    = strdup(name);
            break;
        }

        case PluginBridgeCustomData:
        {
            const char* stype = (const char*)&argv[0]->s;
            const char* key   = (const char*)&argv[1]->s;
            const char* value = (const char*)&argv[2]->s;

            set_custom_data(customdatastr2type(stype), key, value, false);
            break;
        }

        case PluginBridgeChunkData:
        {
            const char* string_data = (const char*)&argv[0]->s;
            if (info.chunk)
                free((void*)info.chunk);
            info.chunk = strdup(string_data);
            break;
        }

        case PluginBridgeUpdateNow:
            callback_action(CALLBACK_RELOAD_ALL, m_id, 0, 0, 0.0);
            initiated = true;
            break;

        case PluginBridgeSaved:
            saved = true;
            break;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        params[param_id].value = fix_parameter_value(value, param.ranges[param_id]);

        if (gui_send)
            osc_send_control(&osc.data, param.data[param_id].rindex, value);

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_chunk_data(const char* string_data)
    {
        osc_send_configure(&osc.data, "CarlaBridgeChunk", string_data);
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
    }

    void prepare_for_save()
    {
        saved = false;
        osc_send_configure(&osc.data, "CarlaBridgeSaveNow", "");

        for (int i=0; i < 100; i++)
        {
            if (saved)
                break;
            carla_msleep(100);
        }

        if (! saved)
            qWarning("BridgePlugin::prepare_for_save() - Timeout while requesting save state");
        else
            qWarning("BridgePlugin::prepare_for_save() - success!");
    }

    // -------------------------------------------------------------------

    void delete_buffers()
    {
        qDebug("BridgePlugin::delete_buffers() - start");

        if (param.count > 0)
            delete[] params;

        params = nullptr;

        qDebug("BridgePlugin::delete_buffers() - end");
    }

    bool init(const char* filename, const char* label)
    {
        const char* bridge_binary = binarytype2str(m_binary);

        if (bridge_binary)
        {
            m_filename = strdup(filename);

            // register plugin now so we can receive OSC (and wait for it)
            CarlaPlugins[m_id] = this;

            m_thread->setOscData(bridge_binary, label, plugintype2str(m_type));
            m_thread->start();

            for (int i=0; i < 100; i++)
            {
                if (initiated)
                    break;
                carla_msleep(100);
            }

            if (! initiated)
                set_last_error("Timeout while waiting for a response from plugin-bridge");

            return initiated;
        }
        else
            set_last_error("Bridge not possible, bridge-binary not found");

        return false;
    }

private:
    bool initiated;
    bool saved;

    const BinaryType m_binary;
    CarlaPluginThread* m_thread;

    struct {
        uint32_t ains, aouts;
        uint32_t mins, mouts;
        PluginCategory category;
        long uniqueId;
        const char* name;
        const char* label;
        const char* maker;
        const char* copyright;
        const char* chunk;
    } info;

    BridgeParamInfo* params;
};

short add_plugin_bridge(BinaryType btype, PluginType ptype, const char* filename, const char* label)
{
    qDebug("add_plugin_bridge(%i, %i, %s, %s)", btype, ptype, filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        BridgePlugin* plugin = new BridgePlugin(btype, ptype, id);

        if (plugin->init(filename, label))
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

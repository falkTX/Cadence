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

#include "lv2_rdf.h"
#include "sratom/sratom.h"

extern "C" {
#include "lv2-rtmempool/rtmempool.h"
}

#ifndef __WINE__
#include <QtGui/QDialog>
#include <QtGui/QLayout>
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x0100;
const unsigned int PLUGIN_HAS_EXTENSION_PROGRAMS = 0x0200;
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x0400;
const unsigned int PLUGIN_HAS_EXTENSION_WORKER   = 0x0800;

// extra parameter hints
const unsigned int PARAMETER_IS_TRIGGER          = 0x1000;
const unsigned int PARAMETER_IS_STRICT_BOUNDS    = 0x2000;

// feature ids
const uint32_t lv2_feature_id_event           = 0;
const uint32_t lv2_feature_id_logs            = 1;
const uint32_t lv2_feature_id_programs        = 2;
const uint32_t lv2_feature_id_rtmempool       = 3;
const uint32_t lv2_feature_id_state_make_path = 4;
const uint32_t lv2_feature_id_state_map_path  = 5;
const uint32_t lv2_feature_id_strict_bounds   = 6;
const uint32_t lv2_feature_id_uri_map         = 7;
const uint32_t lv2_feature_id_urid_map        = 8;
const uint32_t lv2_feature_id_urid_unmap      = 9;
const uint32_t lv2_feature_id_worker          = 10;
const uint32_t lv2_feature_id_data_access     = 11;
const uint32_t lv2_feature_id_instance_access = 12;
const uint32_t lv2_feature_id_ui_parent       = 13;
const uint32_t lv2_feature_id_ui_port_map     = 14;
const uint32_t lv2_feature_id_ui_resize       = 15;
const uint32_t lv2_feature_id_external_ui     = 16;
const uint32_t lv2_feature_id_external_ui_old = 17;
const uint32_t lv2_feature_count              = 18;

// event data/types
const unsigned int CARLA_EVENT_DATA_ATOM      = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT     = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL   = 0x04;
const unsigned int CARLA_EVENT_TYPE_MESSAGE   = 0x10;
const unsigned int CARLA_EVENT_TYPE_MIDI      = 0x20;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL          = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK    = 1;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH     = 2;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE = 3;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING   = 4;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM  = 5;
const uint32_t CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT = 6;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR     = 7;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE      = 8;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE     = 9;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING   = 10;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT    = 11;
const uint32_t CARLA_URI_MAP_ID_COUNT         = 12;

enum Lv2ParameterDataType {
    LV2_PARAMETER_TYPE_CONTROL
};

struct EventData {
    unsigned int type;
    jack_port_t* port;
    union {
        LV2_Atom_Sequence* atom;
        LV2_Event_Buffer* event;
        LV2_MIDI* midi;
    };
};

struct Lv2ParameterData {
    Lv2ParameterDataType type;
    union {
        float control;
    };
};

struct PluginEventData {
    uint32_t count;
    EventData* data;
};

const char* lv2bridge2str(LV2_Property type)
{
    switch (type)
    {
#ifndef BUILD_BRIDGE
    case LV2_UI_GTK2:
        return carla_options.bridge_lv2gtk2;
    case LV2_UI_QT4:
        return carla_options.bridge_lv2qt4;
    case LV2_UI_X11:
        return carla_options.bridge_lv2x11;
#endif
    default:
        return nullptr;
    }
}

class Lv2Plugin : public CarlaPlugin
{
public:
    Lv2Plugin(unsigned short id) : CarlaPlugin(id)
    {
        qDebug("Lv2Plugin::Lv2Plugin()");
        m_type = PLUGIN_LV2;

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        ext.dynparam   = nullptr;
        ext.state      = nullptr;
        ext.worker     = nullptr;
        ext.programs   = nullptr;
        ext.uiprograms = nullptr;

        ui.lib = nullptr;
        ui.handle = nullptr;
        ui.descriptor = nullptr;
        ui.rdf_descriptor = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        lv2param = nullptr;

        gui.type = GUI_NONE;
        gui.visible = false;
        gui.resizable = false;
        gui.width = 0;
        gui.height = 0;

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            custom_uri_ids.append(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;

        Lv2World.init();
    }

    ~Lv2Plugin()
    {
        qDebug("Lv2Plugin::~Lv2Plugin()");

        // close UI
        if (m_hints & PLUGIN_HAS_GUI)
        {
            switch(gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_X11:
                break;

#ifndef BUILD_BRIDGE
            case GUI_EXTERNAL_OSC:
                if (osc.data.target)
                {
                    osc_send_hide(&osc.data);
                    osc_send_quit(&osc.data);
                }

                if (osc.thread)
                {
                    // Wait a bit first, try safe quit else force kill
                    if (osc.thread->isRunning())
                    {
                        if (osc.thread->wait(2000) == false)
                            osc.thread->quit();

                        if (osc.thread->isRunning() && osc.thread->wait(1000) == false)
                        {
                            qWarning("Failed to properly stop LV2 OSC-GUI thread");
                            osc.thread->terminate();
                        }
                    }

                    delete osc.thread;
                }

                osc_clear_data(&osc.data);

                break;
#endif

            case GUI_EXTERNAL_LV2:
                if (gui.visible && ui.widget)
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);

                break;

            default:
                break;
            }

            if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
                ui.descriptor->cleanup(ui.handle);

            if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
                delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

            if (features[lv2_feature_id_ui_port_map] && features[lv2_feature_id_ui_port_map]->data)
                delete (LV2UI_Port_Map*)features[lv2_feature_id_ui_port_map]->data;

            if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
                delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

            if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
            {
                free((void*)((lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data)->plugin_human_id);
                delete (lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data;
            }

            ui_lib_close();
        }

        if (handle && descriptor && descriptor->deactivate && m_active_before)
            descriptor->deactivate(handle);

        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

        if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
            delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

        if (features[lv2_feature_id_logs] && features[lv2_feature_id_logs]->data)
            delete (LV2_Log_Log*)features[lv2_feature_id_logs]->data;

        if (features[lv2_feature_id_state_make_path] && features[lv2_feature_id_state_make_path]->data)
            delete (LV2_State_Make_Path*)features[lv2_feature_id_state_make_path]->data;

        if (features[lv2_feature_id_state_map_path] && features[lv2_feature_id_state_map_path]->data)
            delete (LV2_State_Map_Path*)features[lv2_feature_id_state_map_path]->data;

        if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
            delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

        if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
            delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

        if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
            delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

        if (features[lv2_feature_id_worker] && features[lv2_feature_id_worker]->data)
            delete (LV2_Worker_Schedule*)features[lv2_feature_id_worker]->data;

        if (features[lv2_feature_id_programs] && features[lv2_feature_id_programs]->data)
            delete (LV2_Programs_Host*)features[lv2_feature_id_programs]->data;

        if (features[lv2_feature_id_rtmempool] && features[lv2_feature_id_rtmempool]->data)
            delete (lv2_rtsafe_memory_pool_provider*)features[lv2_feature_id_rtmempool]->data;

        for (uint32_t i=0; i < lv2_feature_count; i++)
        {
            if (features[i])
                delete features[i];
        }

        for (int i=0; i < custom_uri_ids.count(); i++)
        {
            if (custom_uri_ids[i])
                free((void*)custom_uri_ids[i]);
        }

        custom_uri_ids.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        LV2_Property Category = rdf_descriptor->Type;

        if (LV2_IS_DELAY(Category))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_DISTORTION(Category))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_DYNAMICS(Category))
            return PLUGIN_CATEGORY_DYNAMICS;
        if (LV2_IS_EQ(Category))
            return PLUGIN_CATEGORY_EQ;
        if (LV2_IS_FILTER(Category))
            return PLUGIN_CATEGORY_FILTER;
        if (LV2_IS_GENERATOR(Category))
            return PLUGIN_CATEGORY_SYNTH;
        if (LV2_IS_MODULATOR(Category))
            return PLUGIN_CATEGORY_MODULATOR;
        if (LV2_IS_REVERB(Category))
            return PLUGIN_CATEGORY_DELAY;
        if (LV2_IS_SIMULATOR(Category))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPATIAL(Category))
            return PLUGIN_CATEGORY_OTHER;
        if (LV2_IS_SPECTRAL(Category))
            return PLUGIN_CATEGORY_UTILITY;
        if (LV2_IS_UTILITY(Category))
            return PLUGIN_CATEGORY_UTILITY;

        return get_category_from_name(m_name);
    }

    long unique_id()
    {
        return rdf_descriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t min_count()
    {
        uint32_t i, count = 0;

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t mout_count()
    {
        uint32_t i, count = 0;

        for (i=0; i < evout.count; i++)
        {
            if (evout.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t param_scalepoint_count(uint32_t param_id)
    {
        int32_t rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[rindex].ScalePointCount;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double get_parameter_value(uint32_t param_id)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            return lv2param[param_id].control;
        default:
            return 0.0;
        }
    }

    double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Value;
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->URI, STR_MAX);
    }

    void get_maker(char* buf_str)
    {
        if (rdf_descriptor->Author)
            strncpy(buf_str, rdf_descriptor->Author, STR_MAX);
        else
            *buf_str = 0;
    }

    void get_copyright(char* buf_str)
    {
        if (rdf_descriptor->License)
            strncpy(buf_str, rdf_descriptor->License, STR_MAX);
        else
            *buf_str = 0;
    }

    void get_real_name(char* buf_str)
    {
        if (rdf_descriptor->Name)
            strncpy(buf_str, rdf_descriptor->Name, STR_MAX);
        else
            *buf_str = 0;
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Name, STR_MAX);
    }

    void get_parameter_symbol(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Symbol, STR_MAX);
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[rindex];

        if (LV2_HAVE_UNIT_SYMBOL(Port->Unit.Hints))
            strncpy(buf_str, Port->Unit.Symbol, STR_MAX);

        else if (LV2_HAVE_UNIT(Port->Unit.Hints))
        {
            switch (Port->Unit.Type)
            {
            case LV2_UNIT_BAR:
                strncpy(buf_str, "bars", STR_MAX);
                return;
            case LV2_UNIT_BEAT:
                strncpy(buf_str, "beats", STR_MAX);
                return;
            case LV2_UNIT_BPM:
                strncpy(buf_str, "BPM", STR_MAX);
                return;
            case LV2_UNIT_CENT:
                strncpy(buf_str, "ct", STR_MAX);
                return;
            case LV2_UNIT_CM:
                strncpy(buf_str, "cm", STR_MAX);
                return;
            case LV2_UNIT_COEF:
                strncpy(buf_str, "(coef)", STR_MAX);
                return;
            case LV2_UNIT_DB:
                strncpy(buf_str, "dB", STR_MAX);
                return;
            case LV2_UNIT_DEGREE:
                strncpy(buf_str, "deg", STR_MAX);
                return;
            case LV2_UNIT_FRAME:
                strncpy(buf_str, "frames", STR_MAX);
                return;
            case LV2_UNIT_HZ:
                strncpy(buf_str, "Hz", STR_MAX);
                return;
            case LV2_UNIT_INCH:
                strncpy(buf_str, "in", STR_MAX);
                return;
            case LV2_UNIT_KHZ:
                strncpy(buf_str, "kHz", STR_MAX);
                return;
            case LV2_UNIT_KM:
                strncpy(buf_str, "km", STR_MAX);
                return;
            case LV2_UNIT_M:
                strncpy(buf_str, "m", STR_MAX);
                return;
            case LV2_UNIT_MHZ:
                strncpy(buf_str, "MHz", STR_MAX);
                return;
            case LV2_UNIT_MIDINOTE:
                strncpy(buf_str, "note", STR_MAX);
                return;
            case LV2_UNIT_MILE:
                strncpy(buf_str, "mi", STR_MAX);
                return;
            case LV2_UNIT_MIN:
                strncpy(buf_str, "min", STR_MAX);
                return;
            case LV2_UNIT_MM:
                strncpy(buf_str, "mm", STR_MAX);
                return;
            case LV2_UNIT_MS:
                strncpy(buf_str, "ms", STR_MAX);
                return;
            case LV2_UNIT_OCT:
                strncpy(buf_str, "oct", STR_MAX);
                return;
            case LV2_UNIT_PC:
                strncpy(buf_str, "%", STR_MAX);
                return;
            case LV2_UNIT_S:
                strncpy(buf_str, "s", STR_MAX);
                return;
            case LV2_UNIT_SEMITONE:
                strncpy(buf_str, "semi", STR_MAX);
                return;
            }
        }
        *buf_str = 0;
    }

    void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Label, STR_MAX);
    }

    void get_gui_info(GuiInfo* info)
    {
        info->type      = gui.type;
        info->resizable = gui.resizable;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            lv2param[param_id].control = fix_parameter_value(value, param.ranges[param_id]);
            break;
        default:
            break;
        }

        if (gui_send)
        {
            switch (gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_X11:
            case GUI_EXTERNAL_LV2:
                if (ui.handle && ui.descriptor && ui.descriptor->port_event)
                {
                    float fvalue = value;
                    ui.descriptor->port_event(ui.handle, param.data[param_id].rindex, sizeof(float), 0, &fvalue);
                }
                break;

#ifndef BUILD_BRIDGE
            case GUI_EXTERNAL_OSC:
                osc_send_control(&osc.data, param.data[param_id].rindex, value);
                break;
#endif

            default:
                break;
            }
        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send)
    {
        CarlaPlugin::set_custom_data(dtype, key, value, gui_send);

        if (ext.state)
        {
            LV2_State_Status status = ext.state->restore(handle, carla_lv2_state_retrieve, this, 0, features);

            const char* stype = customdatatype2str(dtype);

            switch (status)
            {
            case LV2_STATE_SUCCESS:
                qDebug("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - success", stype, key, bool2str(gui_send));
                break;
            case LV2_STATE_ERR_UNKNOWN:
                qWarning("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - unknown error", stype, key, bool2str(gui_send));
                break;
            case LV2_STATE_ERR_BAD_TYPE:
                qWarning("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - error, bad type", stype, key, bool2str(gui_send));
                break;
            case LV2_STATE_ERR_BAD_FLAGS:
                qWarning("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - error, bad flags", stype, key, bool2str(gui_send));
                break;
            case LV2_STATE_ERR_NO_FEATURE:
                qWarning("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - error, missing feature", stype, key, bool2str(gui_send));
                break;
            case LV2_STATE_ERR_NO_PROPERTY:
                qWarning("Lv2Plugin::set_custom_data(%s, %s, <value>, %s) - error, missing property", stype, key, bool2str(gui_send));
                break;
            }
        }
    }

    void set_midi_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        if (ext.programs && index >= 0)
        {
            if (CarlaEngine::isOffline())
            {
                if (block) carla_proc_lock();
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (block) carla_proc_unlock();
            }
            else
            {
                const CarlaPluginScopedDisabler m(this, block);
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }

            if (gui_send)
            {
#ifndef BUILD_BRIDGE
                if (gui.type == GUI_EXTERNAL_OSC)
                    osc_send_midi_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program, false);
                else
#endif
                    if (ext.uiprograms)
                        ext.uiprograms->select_program(ui.handle, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::set_midi_program(index, gui_send, osc_send, callback_send, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void set_gui_data(int, QDialog* dialog)
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            if (ui.widget)
            {
                QWidget* widget = (QWidget*)ui.widget;
                dialog->layout()->addWidget(widget);
                widget->adjustSize();
                widget->setParent(dialog);
                widget->show();
            }
            break;

        case GUI_INTERNAL_X11:
            if (ui.descriptor)
            {
                features[lv2_feature_id_ui_parent]->data = (void*)dialog->winId();

                ui.handle = ui.descriptor->instantiate(ui.descriptor,
                                                       descriptor->URI,
                                                       ui.rdf_descriptor->Bundle,
                                                       carla_lv2_ui_write_function,
                                                       this,
                                                       &ui.widget,
                                                       features);
                update_ui();
            }
            break;

        default:
            break;
        }
    }

    void show_gui(bool yesno)
    {
        // FIXME - is gui.visible needed at all?
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            gui.visible = yesno;
            break;

        case GUI_INTERNAL_X11:
            gui.visible = yesno;

            if (yesno && gui.width > 0 && gui.height > 0)
                callback_action(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);

            break;

#ifndef BUILD_BRIDGE
        case GUI_EXTERNAL_OSC:
            if (yesno)
            {
                osc.thread->start();
            }
            else
            {
                osc_send_hide(&osc.data);
                osc_send_quit(&osc.data);
                osc_clear_data(&osc.data);
            }
            break;
#endif

        case GUI_EXTERNAL_LV2:
            if (! ui.handle)
                reinit_external_ui();

            if (ui.handle && ui.descriptor && ui.widget)
            {
                if (yesno)
                {
                    LV2_EXTERNAL_UI_SHOW((lv2_external_ui*)ui.widget);
                    gui.visible = true;
                }
                else
                {
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);
                    gui.visible = false;

                    if (strcmp(rdf_descriptor->Author, "linuxDSP") == 0)
                    {
                        qWarning("linuxDSP LV2 UI hack (force close instead of hide)");

                        if (ui.descriptor->cleanup)
                            ui.descriptor->cleanup(ui.handle);

                        ui.handle = nullptr;
                    }
                }
            }
            else
            {
                // failed to init UI
                gui.visible = false;
                callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
            }
            break;

        default:
            break;
        }
    }

    void idle_gui()
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
        case GUI_INTERNAL_X11:
        case GUI_EXTERNAL_LV2:
            if (ui.handle && ui.descriptor)
            {
                if (ui.descriptor->port_event)
                {
                    float value;

                    for (uint32_t i=0; i < param.count; i++)
                    {
                        if (param.data[i].type == PARAMETER_OUTPUT)
                        {
                            value = get_parameter_value(i);
                            ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
                        }
                    }
                }

                if (gui.type == GUI_EXTERNAL_LV2)
                    LV2_EXTERNAL_UI_RUN((lv2_external_ui*)ui.widget);
            }
            break;

        default:
            break;
        }
    }

#if 0
    void reload()
    {
        qDebug("Lv2Plugin::reload() - start");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        remove_from_jack(bool(_id >= 0));

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, cv_ins, cv_outs, ev_ins, ev_outs, params, j;
        ains = aouts = cv_ins = cv_outs = ev_ins = ev_outs = params = 0;

        const uint32_t PortCount = rdf_descriptor->PortCount;
        unsigned int event_data_type = 0;

        for (uint32_t i=0; i<PortCount; i++)
        {
            const LV2_Property PortType = rdf_descriptor->Ports[i].Type;
            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ains += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    aouts += 1;
            }
            else if (LV2_IS_PORT_CV(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    cv_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    cv_outs += 1;
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_ATOM;
            }
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_EVENT;
            }
            else if (LV2_IS_PORT_MIDI_LL(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    ev_ins += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    ev_outs += 1;
                event_data_type = CARLA_EVENT_DATA_MIDI_LL;
            }
            else if (LV2_IS_PORT_CONTROL(PortType))
                params += 1;
            else
                qDebug("Unknown port type found, index: %i, name: %s", i, rdf_descriptor->Ports[i].Name);
        }

        if (ains > 0)
        {
            ain.ports    = new jack_port_t*[ains];
            ain.rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new jack_port_t*[aouts];
            aout.rindexes = new uint32_t[aouts];
        }

        if (ev_ins > 0)
        {
            evin.data = new EventData[ev_ins];

            for (j=0; j < ev_ins; j++)
            {
                evin.data[j].port = nullptr;

                if (event_data_type == CARLA_EVENT_DATA_ATOM)
                {
                    evin.data[j].type     = CARLA_EVENT_DATA_ATOM;
                    evin.data[j].buffer.a = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evin.data[j].buffer.a->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evin.data[j].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evin.data[j].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                    evin.data[j].buffer.a->body.pad  = 0;
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evin.data[j].type     = CARLA_EVENT_DATA_EVENT;
                    evin.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evin.data[j].type     = CARLA_EVENT_DATA_MIDI_LL;
                    evin.data[j].buffer.m = new LV2_MIDI;
                    evin.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evin.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evin.data[j].type = 0;
            }
        }

        if (ev_outs > 0)
        {
            evout.data = new EventData[ev_outs];

            for (j=0; j < ev_outs; j++)
            {
                evout.data[j].port = nullptr;

                if (event_data_type == CARLA_EVENT_DATA_ATOM)
                {
                    evout.data[j].type     = CARLA_EVENT_DATA_ATOM;
                    evout.data[j].buffer.a = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evout.data[j].buffer.a->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evout.data[j].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evout.data[j].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                    evout.data[j].buffer.a->body.pad  = 0;
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evout.data[j].type     = CARLA_EVENT_DATA_EVENT;
                    evout.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evout.data[j].type     = CARLA_EVENT_DATA_MIDI_LL;
                    evout.data[j].buffer.m = new LV2_MIDI;
                    evout.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evout.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evout.data[j].type = 0;
            }
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            lv2param     = new Lv2ParameterData[params];
        }

        const int port_name_size = jack_port_name_size() - 1;
        char port_name[port_name_size];
        bool needs_cin  = false;
        bool needs_cout = false;

        for (uint32_t i=0; i < PortCount; i++)
        {
            const LV2_Property PortType  = rdf_descriptor->Ports[i].Type;

            if (LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_ATOM_SEQUENCE(PortType) || LV2_IS_PORT_CV(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carla_options.global_jack_client)
                {
                    strcpy(port_name, m_name);
                    strcat(port_name, ":");
                    strncat(port_name, rdf_descriptor->Ports[i].Name, port_name_size/2);
                }
                else
#endif
                    strncpy(port_name, rdf_descriptor->Ports[i].Name, port_name_size);
            }

            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                    ain.rindexes[j] = i;
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    aout.rindexes[j] = i;
                    needs_cin = true;
                }
                else
                    qWarning("WARNING - Got a broken Port (Audio, but not input or output)");
            }
            else if (LV2_IS_PORT_CV(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    qWarning("WARNING - CV Ports are not supported yet");
                }
                else
                    qWarning("WARNING - Got a broken Port (CV, but not input or output)");

                descriptor->connect_port(handle, i, nullptr);
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.a);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_PATCH_MESSAGE)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.a);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_PATCH_MESSAGE)
                    {
                        evout.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Atom Sequence, but not input or output)");
            }
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.e);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.e);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                    }
                }
                else
                    qWarning("WARNING - Got a broken Port (Event, but not input or output)");
            }
            else if (LV2_IS_PORT_MIDI_LL(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.m);

                    evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    evin.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.m);

                    evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    evout.data[j].port  = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                }
                else
                    qWarning("WARNING - Got a broken Port (Midi, but not input or output)");
            }
            else if (LV2_IS_PORT_CONTROL(PortType))
            {
                const LV2_Property PortProps = rdf_descriptor->Ports[i].Properties;
                const LV2_RDF_PortPoints PortPoints = rdf_descriptor->Ports[i].Points;

                j = param.count++;
                param.data[j].index  = j;
                param.data[j].rindex = i;
                param.data[j].hints  = 0;
                param.data[j].midi_channel = 0;
                param.data[j].midi_cc = -1;

                double min, max, def, step, step_small, step_large;

                // min value
                if (LV2_HAVE_MINIMUM_PORT_POINT(PortPoints.Hints))
                    min = PortPoints.Minimum;
                else
                    min = 0.0;

                // max value
                if (LV2_HAVE_MAXIMUM_PORT_POINT(PortPoints.Hints))
                    max = PortPoints.Maximum;
                else
                    max = 1.0;

                if (min > max)
                    max = min;
                else if (max < min)
                    min = max;

                if (max - min == 0.0)
                {
                    qWarning("Broken plugin parameter: max - min == 0");
                    max = min + 0.1;
                }

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(PortPoints.Hints))
                {
                    def = PortPoints.Default;
                }
                else
                {
                    // no default value
                    if (min < 0.0 && max > 0.0)
                        def = 0.0;
                    else
                        def = min;
                }

                if (def < min)
                    def = min;
                else if (def > max)
                    def = max;

                if (LV2_IS_PORT_SAMPLE_RATE(PortProps))
                {
                    double sample_rate = get_sample_rate();
                    min *= sample_rate;
                    max *= sample_rate;
                    def *= sample_rate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_TOGGLED(PortProps))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (LV2_IS_PORT_INTEGER(PortProps))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }

                if (LV2_IS_PORT_INPUT(PortType))
                {
                    param.data[j].type   = PARAMETER_INPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;
                    param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                    needs_cin = true;

                    // MIDI CC value
                    LV2_RDF_PortMidiMap* PortMidiMap = &rdf_descriptor->Ports[i].MidiMap;
                    if (LV2_IS_PORT_MIDI_MAP_CC(PortMidiMap->Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(PortMidiMap->Number))
                            param.data[j].midi_cc = PortMidiMap->Number;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    if (LV2_IS_PORT_LATENCY(PortProps))
                    {
                        min = 0;
                        max = get_sample_rate();
                        def = 0;
                        step = 1;
                        step_small = 1;
                        step_large = 1;

                        param.data[j].type  = PARAMETER_LATENCY;
                        param.data[j].hints = 0;
                    }
                    else
                    {
                        param.data[j].type   = PARAMETER_OUTPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needs_cout = true;
                    }
                }
                else
                {
                    param.data[j].type = PARAMETER_UNKNOWN;
                    qWarning("WARNING - Got a broken Port (Control, but not input or output)");
                }

                // extra parameter hints
                if (LV2_IS_PORT_ENUMERATION(PortProps))
                    param.data[j].hints |= PARAMETER_USES_SCALEPOINTS;

                if (LV2_IS_PORT_LOGARITHMIC(PortProps))
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;

                if (LV2_IS_PORT_TRIGGER(PortProps))
                    param.data[j].hints |= PARAMETER_IS_TRIGGER;

                if (LV2_IS_PORT_STRICT_BOUNDS(PortProps))
                    param.data[j].hints |= PARAMETER_IS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_ON_GUI(PortProps))
                    param.data[j].hints &= ~PARAMETER_IS_ENABLED;

                if (LV2_IS_PORT_CAUSES_ARTIFACTS(PortProps) || LV2_IS_PORT_EXPENSIVE(PortProps) || LV2_IS_PORT_NOT_AUTOMATIC(PortProps))
                    param.data[j].hints &= ~PARAMETER_IS_AUTOMABLE;

                param.ranges[j].min = min;
                param.ranges[j].max = max;
                param.ranges[j].def = def;
                param.ranges[j].step = step;
                param.ranges[j].step_small = step_small;
                param.ranges[j].step_large = step_large;

                // Set LV2 params as needed
                lv2param[j].type = LV2_PARAMETER_TYPE_CONTROL;
                lv2param[j].control = def;

                descriptor->connect_port(handle, i, &lv2param[j].control);
            }
            else
                // Port Type not supported, but it's optional anyway
                descriptor->connect_port(handle, i, nullptr);
        }

        if (needs_cin)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-in");
            }
            else
#endif
                strcpy(port_name, "control-in");

            param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        if (needs_cout)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-out");
            }
            else
#endif
                strcpy(port_name, "control-out");

            param.port_cout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }

        ain.count   = ains;
        aout.count  = aouts;
        evin.count  = ev_ins;
        evout.count = ev_outs;
        param.count = params;

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        if (LV2_IS_GENERATOR(rdf_descriptor->Type))
            m_hints |= PLUGIN_IS_SYNTH;

        if (aouts > 0 && (ains == aouts || ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aouts >= 2 && aouts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        // check extensions
        ext.dynparam   = nullptr;
        ext.state      = nullptr;
        ext.worker     = nullptr;
        ext.programs   = nullptr;

        if (descriptor->extension_data)
        {
            if (m_hints & PLUGIN_HAS_EXTENSION_DYNPARAM)
                ext.dynparam = (lv2dynparam_plugin_callbacks*)descriptor->extension_data(LV2DYNPARAM_URI);

            if (m_hints & PLUGIN_HAS_EXTENSION_PROGRAMS)
                ext.programs = (LV2_Programs_Interface*)descriptor->extension_data(LV2_PROGRAMS__Interface);

            if (m_hints & PLUGIN_HAS_EXTENSION_STATE)
                ext.state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (m_hints & PLUGIN_HAS_EXTENSION_WORKER)
                ext.worker = (LV2_Worker_Interface*)descriptor->extension_data(LV2_WORKER__interface);
        }

        // TODO - apply same to others (reload_programs() after hints)
        //reload_programs(true);

        //if (ext.dynparam)
        //    ext.dynparam->host_attach(handle, &dynparam_host, this);

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

#ifndef BUILD_BRIDGE
        if (! carla_options.global_jack_client)
#endif
            jack_activate(jack_client);

        qDebug("Lv2Plugin::reload() - end");
    }

    void reload_programs(bool init)
    {
        qDebug("Lv2Plugin::reload_programs(%s)", bool2str(init));
        uint32_t i, old_count = midiprog.count;

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
        if (ext.programs)
        {
            while (ext.programs->get_program(handle, midiprog.count))
                midiprog.count += 1;
        }

        if (midiprog.count > 0)
            midiprog.data  = new midi_program_t [midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const LV2_Program_Descriptor* pdesc = ext.programs->get_program(handle, i);
            if (pdesc)
            {
                midiprog.data[i].bank    = pdesc->bank;
                midiprog.data[i].program = pdesc->program;
                midiprog.data[i].name    = strdup(pdesc->name);
            }
            else
            {
                midiprog.data[i].bank    = 0;
                midiprog.data[i].program = 0;
                midiprog.data[i].name    = strdup("(error)");
            }
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        osc_global_send_set_midi_program_count(m_id, midiprog.count);

        for (i=0; i < midiprog.count; i++)
            osc_global_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (midiprog.count > 0)
                set_midi_program(0, false, false, false, true);
        }
        else
        {
            callback_action(CALLBACK_UPDATE, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool program_changed = false;

            if (midiprog.count == old_count+1)
            {
                // one midi program added, probably created by user
                midiprog.current = old_count;
                program_changed  = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                program_changed  = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                program_changed  = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                program_changed  = true;
            }

            if (program_changed)
                set_midi_program(midiprog.current, true, true, true, true);
        }
    }

    void prepare_for_save()
    {
        if (ext.state)
            ext.state->save(handle, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);
    }

    void process(jack_nframes_t nframes, jack_nframes_t nframesOffset)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        uint32_t midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_audio_sample_t* ains_buffer[ain.count];
        jack_audio_sample_t* aouts_buffer[aout.count];
        void* evins_buffer[evin.count];
        void* evouts_buffer[evout.count];

        // different midi APIs
        uint32_t atomSequenceOffsets[evin.count];
        LV2_Event_Iterator evin_iters[evin.count];
        LV2_MIDIState evin_states[evin.count];

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                atomSequenceOffsets[i] = 0;
                evin.data[i].buffer.a->atom.size = 0;
                evin.data[i].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evin.data[i].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                evin.data[i].buffer.a->body.pad  = 0;
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evin.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evin.data[i].buffer.e + 1));
                lv2_event_begin(&evin_iters[i], evin.data[i].buffer.e);
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                evin_states[i].midi = evin.data[i].buffer.m;
                evin_states[i].frame_count = nframes;
                evin_states[i].position = 0;

                evin_states[i].midi->event_count = 0;
                evin_states[i].midi->size = 0;
            }

            if (evin.data[i].port)
                evins_buffer[i] = jack_port_get_buffer(evin.data[i].port, nframes);
            else
                evins_buffer[i] = nullptr;
        }

        for (i=0; i < evout.count; i++)
        {
            if (evout.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evout.data[i].buffer.a->atom.size = 0;
                evout.data[i].buffer.a->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evout.data[i].buffer.a->body.unit = CARLA_URI_MAP_ID_NULL;
                evout.data[i].buffer.a->body.pad  = 0;
            }
            else if (evout.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evout.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evout.data[i].buffer.e + 1));
            }
            else if (evout.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }

            if (evout.data[i].port)
                evouts_buffer[i] = jack_port_get_buffer(evout.data[i].port, nframes);
            else
                evouts_buffer[i] = nullptr;
        }

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            if (ain.count == 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                }
            }
            else if (ain.count >= 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);

                    if (abs_d(ains_buffer[1][k]) > ains_peak_tmp[1])
                        ains_peak_tmp[1] = abs_d(ains_buffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin)
        {
            void* pin_buffer = jack_port_get_buffer(param.port_cin, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            unsigned char next_bank_id = 0;
            if (midiprog.current > 0 && midiprog.count > 0)
                next_bank_id = midiprog.data[midiprog.current].bank;

            for (i=0; i < n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                jack_midi_data_t status  = pin_event.buffer[0];
                jack_midi_data_t channel = status & 0x0F;

                // Control change
                if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    jack_midi_data_t control = pin_event.buffer[1];
                    jack_midi_data_t c_value = pin_event.buffer[2];

                    // Bank Select
                    if (MIDI_IS_CONTROL_BANK_SELECT(control))
                    {
                        next_bank_id = c_value;
                        continue;
                    }

                    double value;

                    // Control backend stuff
                    if (channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(control) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = double(c_value)/127;
                            set_drywet(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(control) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = double(c_value)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_BALANCE(control) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = (double(c_value)-63.5)/63.5;

                            if (value < 0)
                            {
                                left  = -1.0;
                                right = (value*2)+1.0;
                            }
                            else if (value > 0)
                            {
                                left  = (value*2)-1.0;
                                right = 1.0;
                            }
                            else
                            {
                                left  = -1.0;
                                right = 1.0;
                            }

                            set_balance_left(left, false, false);
                            set_balance_right(right, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();

                            if (m_active && m_active_before)
                            {
                                if (descriptor->deactivate)
                                    descriptor->deactivate(handle);

                                if (descriptor->activate)
                                    descriptor->activate(handle);
                            }
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midi_channel == channel && param.data[k].midi_cc == control && param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = c_value > 0 ? param.ranges[k].max : param.ranges[k].min;
                            }
                            else
                            {
                                value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    uint32_t mbank_id = next_bank_id;
                    uint32_t mprog_id = pin_event.buffer[1];

                    for (k=0; k < midiprog.count; k++)
                    {
                        if (midiprog.data[k].bank == mbank_id && midiprog.data[k].program == mprog_id)
                        {
                            set_midi_program(k, false, false, false, false);
                            postpone_event(PostEventMidiProgramChange, k, 0.0);
                            break;
                        }
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (evin.count > 0)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (ext_midi_notes[i].valid)
                {
                    uint8_t midi_event[4] = { 0 };
                    midi_event[0] = ext_midi_notes[i].onoff ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midi_event[1] = ext_midi_notes[i].note;
                    midi_event[2] = ext_midi_notes[i].velo;

                    // send to all midi inputs
                    for (k=0; k < evin.count; k++)
                    {
                        if (evin.data[k].type & CARLA_EVENT_TYPE_MIDI)
                        {
                            if (evin.data[k].type & CARLA_EVENT_DATA_ATOM)
                            {
                                LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evin.data[k].buffer.a) + atomSequenceOffsets[k]);
                                aev->time.frames = 0;
                                aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                                aev->body.size   = 3;
                                memcpy(LV2_ATOM_BODY(&aev->body), midi_event, 3);

                                uint32_t size           = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 3);
                                atomSequenceOffsets[k] += size;
                                evin.data[k].buffer.a->atom.size += size;
                            }
                            else if (evin.data[k].type & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evin_iters[k], 0, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midi_event);

                            else if (evin.data[k].type & CARLA_EVENT_DATA_MIDI_LL)
                                lv2midi_put_event(&evin_states[k], 0, 3, midi_event);
                        }
                    }

                    ext_midi_notes[i].valid = false;
                    midi_event_count += 1;
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (JACK)

        for (i=0; i < evin.count; i++)
        {
            if (evins_buffer[i] == nullptr)
                continue;

            jack_midi_event_t min_event;
            uint32_t n_min_events = jack_midi_get_event_count(evins_buffer[i]);

            for (k=0; k < n_min_events && midi_event_count < MAX_MIDI_EVENTS; k++)
            {
                if (jack_midi_event_get(&min_event, evins_buffer[i], k) != 0)
                    break;

                jack_midi_data_t status = min_event.buffer[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && min_event.buffer[2] == 0)
                {
                    min_event.buffer[0] -= 0x10;
                    status = min_event.buffer[0];
                }

                // write supported status types
                if (MIDI_IS_STATUS_NOTE_OFF(status) || MIDI_IS_STATUS_NOTE_ON(status) || MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) || MIDI_IS_STATUS_AFTERTOUCH(status) || MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
                    {
                        LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evin.data[i].buffer.a) + atomSequenceOffsets[i]);
                        aev->time.frames = min_event.time;
                        aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                        aev->body.size   = min_event.size;
                        memcpy(LV2_ATOM_BODY(&aev->body), min_event.buffer, min_event.size);

                        uint32_t size           = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + min_event.size);
                        atomSequenceOffsets[i] += size;
                        evin.data[i].buffer.a->atom.size += size;
                    }
                    else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&evin_iters[i], min_event.time, 0, CARLA_URI_MAP_ID_MIDI_EVENT, min_event.size, min_event.buffer);
                    else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                        lv2midi_put_event(&evin_states[i], min_event.time, min_event.size, min_event.buffer);

                    if (MIDI_IS_STATUS_NOTE_OFF(status))
                        postpone_event(PostEventNoteOff, min_event.buffer[1], 0.0);
                    else if (MIDI_IS_STATUS_NOTE_ON(status))
                        postpone_event(PostEventNoteOn, min_event.buffer[1], min_event.buffer[2]);
                }

                midi_event_count += 1;
            }
        } // End of MIDI Input (JACK)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

#if 0
        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;
#endif

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                descriptor->connect_port(handle, ain.rindexes[i], ains_buffer[i]);

            for (i=0; i < aout.count; i++)
                descriptor->connect_port(handle, aout.rindexes[i], aouts_buffer[i]);

            if (descriptor->run)
                descriptor->run(handle, nframes);
        }
        else
        {
            if (m_active_before)
            {
                if (descriptor->deactivate)
                    descriptor->deactivate(handle);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Post-processing (dry/wet, volume and balance)

        if (m_active)
        {
            bool do_drywet  = (m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0;
            bool do_volume  = (m_hints & PLUGIN_CAN_VOLUME) > 0 && x_vol != 1.0;
            bool do_balance = (m_hints & PLUGIN_CAN_BALANCE) > 0 && (x_bal_left != -1.0 || x_bal_right != 1.0);

            double bal_rangeL, bal_rangeR;
            jack_audio_sample_t old_bal_left[do_balance ? nframes : 0];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                if (do_drywet || do_volume)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (do_drywet)
                        {
                            if (aout.count == 1)
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                            else
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                        }

                        if (do_volume)
                            aouts_buffer[i][k] *= x_vol;
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_audio_sample_t)*nframes);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k<nframes; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            aouts_buffer[i][k]  = old_bal_left[k]*(1.0-bal_rangeL);
                            aouts_buffer[i][k] += aouts_buffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            aouts_buffer[i][k]  = aouts_buffer[i][k]*bal_rangeR;
                            aouts_buffer[i][k] += old_bal_left[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; k < nframes && i < 2; k++)
                {
                    if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.port_cout)
        {
            void* cout_buffer = jack_port_get_buffer(param.port_cout, nframes);
            jack_midi_clear_buffer(cout_buffer);

            double value, rvalue;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT && param.data[k].midi_cc > 0)
                {
                    switch (lv2param[k].type)
                    {
                    case LV2_PARAMETER_TYPE_CONTROL:
                        value = lv2param[k].control;
                        break;
                    default:
                        value = param.ranges[k].min;
                        break;
                    }

                    rvalue = (value - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min) * 127;

                    jack_midi_data_t* event_buffer = jack_midi_event_reserve(cout_buffer, 0, 3);
                    event_buffer[0] = MIDI_STATUS_CONTROL_CHANGE + param.data[k].midi_channel;
                    event_buffer[1] = param.data[k].midi_cc;
                    event_buffer[2] = rvalue;
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        for (i=0; i < evout.count; i++)
        {
            if (evouts_buffer[i] == nullptr)
                continue;

            jack_midi_clear_buffer(evouts_buffer[i]);

            if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                // TODO
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                LV2_Event* ev;
                LV2_Event_Iterator iter;

                uint8_t* data;
                lv2_event_begin(&iter, evout.data[i].buffer.e);

                for (k=0; k < iter.buf->event_count; k++)
                {
                    ev = lv2_event_get(&iter, &data);
                    if (ev && data)
                        jack_midi_event_write(evouts_buffer[i], ev->frames, data, ev->size);

                    lv2_event_increment(&iter);
                }
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                LV2_MIDIState state = { evout.data[i].buffer.m, nframes, 0 };

                uint32_t event_size;
                double event_timestamp;
                unsigned char* event_data;

                while (lv2midi_get_event(&state, &event_timestamp, &event_size, &event_data) < nframes)
                {
                    jack_midi_event_write(evouts_buffer[i], event_timestamp, event_data, event_size);
                    lv2midi_step(&state);
                }
            }
        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        ains_peak[(plugin_id*2)+0]  = ains_peak_tmp[0];
        ains_peak[(plugin_id*2)+1]  = ains_peak_tmp[1];
        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    void delete_buffers()
    {
        qDebug("Lv2Plugin::delete_buffers() - start");

        if (param.count > 0)
            delete[] lv2param;

        if (evin.count > 0)
        {
            for (uint32_t i=0; i < evin.count; i++)
            {
                if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    free(evin.data[i].buffer.a);
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evin.data[i].buffer.e);
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evin.data[i].buffer.m->data;
                    delete evin.data[i].buffer.m;
                }
            }

            delete[] evin.data;
        }

        if (evout.count > 0)
        {
            for (uint32_t i=0; i < evout.count; i++)
            {
                if (evout.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    free(evout.data[i].buffer.a);
                }
                else if (evout.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evout.data[i].buffer.e);
                }
                else if (evout.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evout.data[i].buffer.m->data;
                    delete evout.data[i].buffer.m;
                }
            }

            delete[] evout.data;
        }

        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        qDebug("Lv2Plugin::delete_buffers() - end");
    }

    // TODO, remove = true
    void remove_from_jack(bool deactivate)
    {
        qDebug("Lv2Plugin::remove_from_jack() - start");

        CarlaPlugin::remove_from_jack(deactivate);

        for (uint32_t i=0; i < evin.count; i++)
        {
            if (evin.data[i].port)
                jack_port_unregister(jack_client, evin.data[i].port);
        }

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].port)
                jack_port_unregister(jack_client, evout.data[i].port);
        }

        qDebug("Lv2Plugin::remove_from_jack() - end");
    }
#endif

    void run_custom_event(PluginPostEvent* event)
    {
        //if (ext.worker)
        //{
        //    ext.worker->work(handle, carla_lv2_worker_respond, this, event->index, event->cdata);
        //    ext.worker->end_run(handle);
        //}
        Q_UNUSED(event);
    }

    void handle_event_transfer(const char* type, const char* key, const char* string_data)
    {
        qDebug("Lv2Plugin::handle_event_transfer() - %s | %s | %s", type, key, string_data);

        QByteArray chunk;
        chunk = QByteArray::fromBase64(string_data);

        LV2_Atom* atom = (LV2_Atom*)chunk.constData();
        LV2_URID urid_atom_Blank = get_custom_uri_id(LV2_ATOM__Blank);
        LV2_URID urid_patch_body = get_custom_uri_id(LV2_PATCH__body);
        LV2_URID urid_patch_Set  = get_custom_uri_id(LV2_PATCH__Set);

        if (atom->type == urid_atom_Blank)
        {
            qDebug("Is blank");

            LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;

            if (obj->body.otype == urid_patch_Set)
            {
                qDebug("Is Patch Set");

                const LV2_Atom_Object* body = nullptr;
                lv2_atom_object_get(obj, urid_patch_body, &body, 0);

                if (body)
                {
                    qDebug("Has body");

                    LV2_ATOM_OBJECT_FOREACH(body, iter)
                    {
                        CustomDataType dtype = CUSTOM_DATA_INVALID;
                        const char* key   = get_custom_uri_string(iter->key);
                        const char* value = nullptr;

                        if (iter->value.type == CARLA_URI_MAP_ID_ATOM_STRING || iter->value.type == CARLA_URI_MAP_ID_ATOM_PATH)
                        {
                            value = strdup((const char*)LV2_ATOM_BODY(&iter->value));
                            dtype = (iter->value.type == CARLA_URI_MAP_ID_ATOM_STRING) ? CUSTOM_DATA_STRING : CUSTOM_DATA_PATH;
                        }
                        else if (iter->value.type == CARLA_URI_MAP_ID_ATOM_CHUNK || iter->value.type >= CARLA_URI_MAP_ID_COUNT)
                        {
                            QByteArray chunk((const char*)LV2_ATOM_BODY(&iter->value), iter->value.size);
                            value = strdup(chunk.toBase64().constData());
                            dtype = (iter->value.type == CARLA_URI_MAP_ID_ATOM_CHUNK) ? CUSTOM_DATA_CHUNK : CUSTOM_DATA_BINARY; // FIXME - binary should be a custom type
                        }
                        else
                            value = strdup("");

                        set_custom_data(dtype, key, value, false);
                        free((void*)value);
                    }
                }
            }
        }
    }

    uint32_t get_custom_uri_id(const char* uri)
    {
        qDebug("Lv2Plugin::get_custom_uri_id(%s)", uri);

        for (int i=0; i < custom_uri_ids.count(); i++)
        {
            if (custom_uri_ids[i] && strcmp(custom_uri_ids[i], uri) == 0)
                return i;
        }

        custom_uri_ids.append(strdup(uri));
        return custom_uri_ids.count()-1;
    }

    const char* get_custom_uri_string(int uri_id)
    {
        qDebug("Lv2Plugin::get_custom_uri_string(%i)", uri_id);

        if (uri_id < custom_uri_ids.count())
            return custom_uri_ids.at(uri_id);
        else
            return nullptr;
    }

    void update_program(int32_t index)
    {
        if (index == -1)
        {
            const CarlaPluginScopedDisabler m(this);
            reload_programs(false);
        }
        else
        {
            if (ext.programs && index >= 0 && index < (int32_t)prog.count)
            {
                const char* prog_name = ext.programs->get_program(handle, index)->name;

                if (prog_name && ! (prog.names[index] && strcmp(prog_name, prog.names[index]) == 0))
                {
                    if (prog.names[index])
                        free((void*)prog.names[index]);

                    prog.names[index] = strdup(prog_name);
                }
            }
        }

#ifndef BUILD_BRIDGE
        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif
    }

    bool is_ui_resizable()
    {
        if (ui.rdf_descriptor)
        {
            for (uint32_t i=0; i < ui.rdf_descriptor->FeatureCount; i++)
            {
                if (strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(ui.rdf_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                    return false;
            }
            return true;
        }
        return false;
    }

    bool is_ui_bridgeable(uint32_t ui_id)
    {
#ifdef BUILD_BRIDGE
        return false;
#else
        const LV2_RDF_UI* const rdf_ui = &rdf_descriptor->UIs[ui_id];

        for (uint32_t i=0; i < rdf_ui->FeatureCount; i++)
        {
            if (strcmp(rdf_ui->Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0 || strcmp(rdf_ui->Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
#endif
    }

    void reinit_external_ui()
    {
        qDebug("Lv2Plugin::reinit_external_ui()");
        ui.widget = nullptr;
        ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle && ui.widget)
        {
            update_ui();
        }
        else
        {
            qDebug("Lv2Plugin::reinit_external_ui() - Failed to re-initiate external UI");

            ui.handle = nullptr;
            ui.widget = nullptr;
            callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
        }
    }

    void update_ui()
    {
        qDebug("Lv2Plugin::update_ui()");
        ext.uiprograms = nullptr;

        if (ui.handle && ui.descriptor)
        {
            if (ui.descriptor->extension_data)
            {
                ext.uiprograms = (LV2_Programs_UI_Interface*)ui.descriptor->extension_data(LV2_PROGRAMS__UIInterface);

                if (ext.uiprograms && midiprog.count > 0 && midiprog.current >= 0)
                    ext.uiprograms->select_program(ui.handle, midiprog.data[midiprog.current].bank, midiprog.data[midiprog.current].program);
            }

            if (ui.descriptor->port_event)
            {
                // state
                for (int i=0; i < custom.count(); i++)
                {
                    if (custom[i].type == CUSTOM_DATA_INVALID)
                        continue;

                    LV2_URID_Map* URID_Map = (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

                    Sratom*   sratom = sratom_new(URID_Map);
                    SerdChunk chunk  = { nullptr, 0 };

                    LV2_Atom_Forge forge;
                    lv2_atom_forge_init(&forge, URID_Map);
                    lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

                    LV2_URID urid_patch_Set  = get_custom_uri_id(LV2_PATCH__Set);
                    LV2_URID urid_patch_body = get_custom_uri_id(LV2_PATCH__body);

                    LV2_Atom_Forge_Frame ref_frame;
                    LV2_Atom_Forge_Ref ref = lv2_atom_forge_blank(&forge, &ref_frame, 1, urid_patch_Set);

                    lv2_atom_forge_property_head(&forge, urid_patch_body, 0);
                    LV2_Atom_Forge_Frame body_frame;
                    lv2_atom_forge_blank(&forge, &body_frame, 2, 0);

                    lv2_atom_forge_property_head(&forge, get_custom_uri_id(custom[i].key), 0);

                    if (custom[i].type == CUSTOM_DATA_STRING)
                        lv2_atom_forge_string(&forge, custom[i].value, strlen(custom[i].value));
                    else if (custom[i].type == CUSTOM_DATA_PATH)
                        lv2_atom_forge_path(&forge, custom[i].value, strlen(custom[i].value));
                    else if (custom[i].type == CUSTOM_DATA_CHUNK)
                        lv2_atom_forge_literal(&forge, custom[i].value, strlen(custom[i].value), CARLA_URI_MAP_ID_ATOM_CHUNK, 0);
                    else
                        lv2_atom_forge_literal(&forge, custom[i].value, strlen(custom[i].value), get_custom_uri_id(custom[i].key), 0);

                    lv2_atom_forge_pop(&forge, &body_frame);
                    lv2_atom_forge_pop(&forge, &ref_frame);

                    LV2_Atom* atom = lv2_atom_forge_deref(&forge, ref);
                    ui.descriptor->port_event(ui.handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

                    free((void*)chunk.buf);
                    sratom_free(sratom);
                }

                // control ports
                float value;
                for (uint32_t i=0; i < param.count; i++)
                {
                    value = get_parameter_value(i);
                    ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
                }
            }
        }
    }

    bool init(const char* bundle, const char* URI)
    {
        rdf_descriptor = lv2_rdf_new(URI);

        if (rdf_descriptor)
        {
            if (lib_open(rdf_descriptor->Binary))
            {
                LV2_Descriptor_Function descfn = (LV2_Descriptor_Function)lib_symbol("lv2_descriptor");

                if (descfn)
                {
                    uint32_t i = 0;
                    while ((descriptor = descfn(i++)))
                    {
                        if (strcmp(descriptor->URI, URI) == 0)
                            break;
                    }

                    if (descriptor)
                    {
                        bool can_continue = true;

                        // Check supported ports
                        for (i=0; i < rdf_descriptor->PortCount; i++)
                        {
                            LV2_Property PortType = rdf_descriptor->Ports[i].Type;
                            if (bool(LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_CONTROL(PortType) || LV2_IS_PORT_ATOM_SEQUENCE(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType)) == false)
                            {
                                qCritical("Got unsupported port -> %i", PortType);
                                if (! LV2_IS_PORT_OPTIONAL(rdf_descriptor->Ports[i].Properties))
                                {
                                    set_last_error("Plugin requires a port that is not currently supported");
                                    can_continue = false;
                                    break;
                                }
                            }
                        }

                        // Check supported features
                        for (i=0; i < rdf_descriptor->FeatureCount; i++)
                        {
                            if (LV2_IS_FEATURE_REQUIRED(rdf_descriptor->Features[i].Type) && is_lv2_feature_supported(rdf_descriptor->Features[i].URI) == false)
                            {
                                QString msg = QString("Plugin requires a feature that is not supported:\n%1").arg(rdf_descriptor->Features[i].URI);
                                set_last_error(msg.toUtf8().constData());
                                can_continue = false;
                                break;
                            }
                        }

                        // Check extensions
                        for (i=0; i < rdf_descriptor->ExtensionCount; i++)
                        {
                            if (strcmp(rdf_descriptor->Extensions[i], LV2DYNPARAM_URI) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_DYNPARAM;
                            else if (strcmp(rdf_descriptor->Extensions[i], LV2_PROGRAMS__Interface) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_PROGRAMS;
                            else if (strcmp(rdf_descriptor->Extensions[i], LV2_STATE__interface) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_STATE;
                            else if (strcmp(rdf_descriptor->Extensions[i], LV2_WORKER__interface) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_WORKER;
                            else
                                qDebug("Plugin has non-supported extension: '%s'", rdf_descriptor->Extensions[i]);
                        }

                        if (can_continue)
                        {
                            // Initialize features
                            LV2_Event_Feature* Event_Feature     = new LV2_Event_Feature;
                            Event_Feature->callback_data         = this;
                            Event_Feature->lv2_event_ref         = carla_lv2_event_ref;
                            Event_Feature->lv2_event_unref       = carla_lv2_event_unref;

                            LV2_Log_Log* Log_Feature             = new LV2_Log_Log;
                            Log_Feature->handle                  = this;
                            Log_Feature->printf                  = carla_lv2_log_printf;
                            Log_Feature->vprintf                 = carla_lv2_log_vprintf;

                            LV2_Programs_Host* Programs_Feature  = new LV2_Programs_Host;
                            Programs_Feature->handle             = this;
                            Programs_Feature->program_changed    = carla_lv2_program_changed;

                            LV2_State_Make_Path* State_MakePath_Feature = new LV2_State_Make_Path;
                            State_MakePath_Feature->handle       = this;
                            State_MakePath_Feature->path         = carla_lv2_state_make_path;

                            LV2_State_Map_Path* State_MapPath_Feature = new LV2_State_Map_Path;
                            State_MapPath_Feature->handle        = this;
                            State_MapPath_Feature->abstract_path = carla_lv2_state_map_abstract_path;
                            State_MapPath_Feature->absolute_path = carla_lv2_state_map_absolute_path;

                            LV2_URI_Map_Feature* URI_Map_Feature = new LV2_URI_Map_Feature;
                            URI_Map_Feature->callback_data       = this;
                            URI_Map_Feature->uri_to_id           = carla_lv2_uri_to_id;

                            LV2_URID_Map* URID_Map_Feature       = new LV2_URID_Map;
                            URID_Map_Feature->handle             = this;
                            URID_Map_Feature->map                = carla_lv2_urid_map;

                            LV2_URID_Unmap* URID_Unmap_Feature   = new LV2_URID_Unmap;
                            URID_Unmap_Feature->handle           = this;
                            URID_Unmap_Feature->unmap            = carla_lv2_urid_unmap;

                            LV2_Worker_Schedule* Worker_Feature  = new LV2_Worker_Schedule;
                            Worker_Feature->handle               = this;
                            Worker_Feature->schedule_work        = carla_lv2_worker_schedule;

                            lv2_rtsafe_memory_pool_provider* RT_MemPool_Feature = new lv2_rtsafe_memory_pool_provider;
                            rtmempool_allocator_init(RT_MemPool_Feature);

                            features[lv2_feature_id_event]            = new LV2_Feature;
                            features[lv2_feature_id_event]->URI       = LV2_EVENT_URI;
                            features[lv2_feature_id_event]->data      = Event_Feature;

                            features[lv2_feature_id_logs]             = new LV2_Feature;
                            features[lv2_feature_id_logs]->URI        = LV2_LOG__log;
                            features[lv2_feature_id_logs]->data       = Log_Feature;

                            features[lv2_feature_id_programs]         = new LV2_Feature;
                            features[lv2_feature_id_programs]->URI    = LV2_PROGRAMS__Host;
                            features[lv2_feature_id_programs]->data   = Programs_Feature;

                            features[lv2_feature_id_rtmempool]        = new LV2_Feature;
                            features[lv2_feature_id_rtmempool]->URI   = LV2_RTSAFE_MEMORY_POOL_URI;
                            features[lv2_feature_id_rtmempool]->data  = RT_MemPool_Feature;

                            features[lv2_feature_id_state_make_path]  = new LV2_Feature;
                            features[lv2_feature_id_state_make_path]->URI  = LV2_STATE__makePath;
                            features[lv2_feature_id_state_make_path]->data = State_MakePath_Feature;

                            features[lv2_feature_id_state_map_path]   = new LV2_Feature;
                            features[lv2_feature_id_state_map_path]->URI  = LV2_STATE__mapPath;
                            features[lv2_feature_id_state_map_path]->data = State_MapPath_Feature;

                            features[lv2_feature_id_strict_bounds]    = new LV2_Feature;
                            features[lv2_feature_id_strict_bounds]->URI  = LV2_PORT_PROPS__supportsStrictBounds;
                            features[lv2_feature_id_strict_bounds]->data = nullptr;

                            features[lv2_feature_id_uri_map]          = new LV2_Feature;
                            features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
                            features[lv2_feature_id_uri_map]->data    = URI_Map_Feature;

                            features[lv2_feature_id_urid_map]         = new LV2_Feature;
                            features[lv2_feature_id_urid_map]->URI    = LV2_URID__map;
                            features[lv2_feature_id_urid_map]->data   = URID_Map_Feature;

                            features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
                            features[lv2_feature_id_urid_unmap]->URI  = LV2_URID__unmap;
                            features[lv2_feature_id_urid_unmap]->data = URID_Unmap_Feature;

                            features[lv2_feature_id_worker]           = new LV2_Feature;
                            features[lv2_feature_id_worker]->URI      = LV2_WORKER__schedule;
                            features[lv2_feature_id_worker]->data     = Worker_Feature;

                            //lv2_atom_forge_init(&atom_forge, URID_Map_Feature);

                            handle = descriptor->instantiate(descriptor, get_sample_rate(), rdf_descriptor->Bundle, features);

                            if (handle)
                            {
                                m_filename = strdup(bundle);
                                m_name   = get_unique_name(rdf_descriptor->Name);
                                x_client = new CarlaEngineClient(this);

                                if (x_client->isOk())
                                {
                                    // ----------------- GUI Stuff -------------------------------------------------------

#if 1
                                    uint32_t UICount = rdf_descriptor->UICount;

                                    if (UICount > 0)
                                    {
                                        // Find more appropriate UI (Qt4 -> X11 -> Gtk2 -> External, use bridges whenever possible)
                                        int eQt4, eX11, eGtk2, iX11, iQt4, iExt, iFinal;
                                        eQt4 = eX11 = eGtk2 = iQt4 = iX11 = iExt = iFinal = -1;

                                        for (i=0; i < UICount; i++)
                                        {
                                            switch (rdf_descriptor->UIs[i].Type)
                                            {
                                            case LV2_UI_QT4:
                                                if (is_ui_bridgeable(i))
                                                    eQt4 = i;
                                                else
                                                    iQt4 = i;
                                                break;

                                            case LV2_UI_X11:
                                                if (is_ui_bridgeable(i))
                                                    eX11 = i;
                                                else
                                                    iX11 = i;
                                                break;

                                            case LV2_UI_GTK2:
                                                if (is_ui_bridgeable(i))
                                                    eGtk2 = i;
                                                break;

                                            case LV2_UI_EXTERNAL:
                                            case LV2_UI_OLD_EXTERNAL:
                                                iExt = i;
                                                break;

                                            default:
                                                break;
                                            }
                                        }

                                        if (eQt4 >= 0)
                                            iFinal = eQt4;
                                        else if (eX11 >= 0)
                                            iFinal = eX11;
                                        else if (eGtk2 >= 0)
                                            iFinal = eGtk2;
                                        else if (iQt4 >= 0)
                                            iFinal = iQt4;
                                        else if (iX11 >= 0)
                                            iFinal = iX11;
                                        else if (iExt >= 0)
                                            iFinal = iExt;

#ifndef BUILD_BRIDGE
                                        bool is_bridged = (iFinal == eQt4 || iFinal == eX11 || iFinal == eGtk2);
#endif

                                        // Use proper UI now
                                        if (iFinal >= 0)
                                        {
                                            ui.rdf_descriptor = &rdf_descriptor->UIs[iFinal];

                                            // Check supported UI features
                                            can_continue = true;

                                            for (i=0; i < ui.rdf_descriptor->FeatureCount; i++)
                                            {
                                                if (LV2_IS_FEATURE_REQUIRED(ui.rdf_descriptor->Features[i].Type) && is_lv2_ui_feature_supported(ui.rdf_descriptor->Features[i].URI) == false)
                                                {
                                                    qCritical("Plugin UI requires a feature that is not supported:\n%s", ui.rdf_descriptor->Features[i].URI);
                                                    can_continue = false;
                                                    break;
                                                }
                                            }

                                            if (can_continue)
                                            {
                                                if (ui_lib_open(ui.rdf_descriptor->Binary))
                                                {
                                                    LV2UI_DescriptorFunction ui_descfn = (LV2UI_DescriptorFunction)ui_lib_symbol("lv2ui_descriptor");

                                                    if (ui_descfn)
                                                    {
                                                        i = 0;
                                                        while ((ui.descriptor = ui_descfn(i++)))
                                                        {
                                                            if (strcmp(ui.descriptor->URI, ui.rdf_descriptor->URI) == 0)
                                                                break;
                                                        }

                                                        if (ui.descriptor)
                                                        {
                                                            // UI Window Title
                                                            QString gui_title = QString("%1 (GUI)").arg(m_name);
                                                            LV2_Property UiType = ui.rdf_descriptor->Type;

#ifndef BUILD_BRIDGE
                                                            if (is_bridged)
                                                            {
                                                                const char* osc_binary = lv2bridge2str(UiType);

                                                                if (osc_binary)
                                                                {
                                                                    // TESTING
                                                                    quintptr address = (quintptr)&rdf_descriptor;
                                                                    setenv("INSTANCE_ADDRESS", QString::number(address).toUtf8().constData(), 1);
                                                                    qWarning("Sending Pointer %p " P_UINTPTR, &rdf_descriptor, address);

                                                                    gui.type = GUI_EXTERNAL_OSC;
                                                                    osc.thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
                                                                    osc.thread->setOscData(osc_binary, descriptor->URI, ui.descriptor->URI);
                                                                }
                                                            }
                                                            else
#endif
                                                            {
                                                                // Initialize UI features
                                                                LV2_Extension_Data_Feature* UI_Data_Feature    = new LV2_Extension_Data_Feature;
                                                                UI_Data_Feature->data_access                   = descriptor->extension_data;

                                                                LV2UI_Port_Map* UI_PortMap_Feature             = new LV2UI_Port_Map;
                                                                UI_PortMap_Feature->handle                     = this;
                                                                UI_PortMap_Feature->port_index                 = carla_lv2_ui_port_map;

                                                                LV2UI_Resize* UI_Resize_Feature                = new LV2UI_Resize;
                                                                UI_Resize_Feature->handle                      = this;
                                                                UI_Resize_Feature->ui_resize                   = carla_lv2_ui_resize;

                                                                lv2_external_ui_host* External_UI_Feature      = new lv2_external_ui_host;
                                                                External_UI_Feature->ui_closed                 = carla_lv2_external_ui_closed;
                                                                External_UI_Feature->plugin_human_id           = strdup(gui_title.toUtf8().constData());

                                                                features[lv2_feature_id_data_access]           = new LV2_Feature;
                                                                features[lv2_feature_id_data_access]->URI      = LV2_DATA_ACCESS_URI;
                                                                features[lv2_feature_id_data_access]->data     = UI_Data_Feature;

                                                                features[lv2_feature_id_instance_access]       = new LV2_Feature;
                                                                features[lv2_feature_id_instance_access]->URI  = LV2_INSTANCE_ACCESS_URI;
                                                                features[lv2_feature_id_instance_access]->data = handle;

                                                                features[lv2_feature_id_ui_parent]             = new LV2_Feature;
                                                                features[lv2_feature_id_ui_parent]->URI        = LV2_UI__parent;
                                                                features[lv2_feature_id_ui_parent]->data       = nullptr;

                                                                features[lv2_feature_id_ui_port_map]           = new LV2_Feature;
                                                                features[lv2_feature_id_ui_port_map]->URI      = LV2_UI__portMap;
                                                                features[lv2_feature_id_ui_port_map]->data     = UI_PortMap_Feature;

                                                                features[lv2_feature_id_ui_resize]             = new LV2_Feature;
                                                                features[lv2_feature_id_ui_resize]->URI        = LV2_UI__resize;
                                                                features[lv2_feature_id_ui_resize]->data       = UI_Resize_Feature;

                                                                features[lv2_feature_id_external_ui]           = new LV2_Feature;
                                                                features[lv2_feature_id_external_ui]->URI      = LV2_EXTERNAL_UI_URI;
                                                                features[lv2_feature_id_external_ui]->data     = External_UI_Feature;

                                                                features[lv2_feature_id_external_ui_old]       = new LV2_Feature;
                                                                features[lv2_feature_id_external_ui_old]->URI  = LV2_EXTERNAL_UI_DEPRECATED_URI;
                                                                features[lv2_feature_id_external_ui_old]->data = External_UI_Feature;

                                                                switch (UiType)
                                                                {
                                                                case LV2_UI_QT4:
                                                                    qDebug("Will use LV2 Qt4 UI");
                                                                    gui.type      = GUI_INTERNAL_QT4;
                                                                    gui.resizable = is_ui_resizable();

                                                                    ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);
                                                                    update_ui();

                                                                    break;

                                                                case LV2_UI_X11:
                                                                    qDebug("Will use LV2 X11 UI");
                                                                    gui.type      = GUI_INTERNAL_X11;
                                                                    gui.resizable = is_ui_resizable();
                                                                    break;

                                                                case LV2_UI_GTK2:
                                                                    qDebug("Will use LV2 Gtk2 UI, NOT!");
                                                                    break;

                                                                case LV2_UI_EXTERNAL:
                                                                case LV2_UI_OLD_EXTERNAL:
                                                                    qDebug("Will use LV2 External UI");
                                                                    gui.type = GUI_EXTERNAL_LV2;
                                                                    break;

                                                                default:
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            qCritical("Could not find the requested GUI in the plugin UI library");
                                                            ui_lib_close();
                                                            ui.lib = nullptr;
                                                            ui.rdf_descriptor = nullptr;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        qCritical("Could not find the LV2UI Descriptor in the UI library");
                                                        ui_lib_close();
                                                        ui.lib = nullptr;
                                                        ui.rdf_descriptor = nullptr;
                                                    }
                                                }
                                                else
                                                    qCritical("Could not load UI library, error was:\n%s", ui_lib_error());
                                            }
                                            else
                                                // cannot continue, UI Feature not supported
                                                ui.rdf_descriptor = nullptr;
                                        }
                                        else
                                            qWarning("Failed to find an appropriate LV2 UI for this plugin");

                                    } // End of GUI Stuff

                                    if (gui.type != GUI_NONE)
                                        m_hints |= PLUGIN_HAS_GUI;
#endif

                                    return true;
                                }
                                else
                                    set_last_error("Failed to register plugin client");
                            }
                            else
                                set_last_error("Plugin failed to initialize");
                        }
                        // error already set
                    }
                    else
                        set_last_error("Could not find the requested plugin URI in the plugin library");
                }
                else
                    set_last_error("Could not find the LV2 Descriptor in the plugin library");
            }
            else
                set_last_error(lib_error());
        }
        else
            set_last_error("Failed to find the requested plugin in the LV2 Bundle");

        return false;
    }

    bool ui_lib_open(const char* filename)
    {
#ifdef Q_OS_WIN
        ui.lib = LoadLibraryA(filename);
#else
        ui.lib = dlopen(filename, RTLD_LAZY);
#endif
        return bool(ui.lib);
    }

    bool ui_lib_close()
    {
        if (ui.lib)
#ifdef Q_OS_WIN
            return FreeLibrary((HMODULE)ui.lib) != 0;
#else
            return dlclose(ui.lib) != 0;
#endif
        else
            return false;
    }

    void* ui_lib_symbol(const char* symbol)
    {
        if (ui.lib)
#ifdef Q_OS_WIN
            return (void*)GetProcAddress((HMODULE)ui.lib, symbol);
#else
            return dlsym(ui.lib, symbol);
#endif
        else
            return nullptr;
    }

    const char* ui_lib_error()
    {
#ifdef Q_OS_WIN
        static char libError[2048];
        memset(libError, 0, sizeof(char)*2048);

        LPVOID winErrorString;
        DWORD  winErrorCode = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

        snprintf(libError, 2048, "%s: error code %i: %s", m_filename, winErrorCode, (const char*)winErrorString);
        LocalFree(winErrorString);

        return libError;
#else
        return dlerror();
#endif
    }

    // ----------------- DynParam Feature ------------------------------------------------
    // TODO

    // ----------------- Event Feature ---------------------------------------------------
    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_ref(%p, %p)", callback_data, event);
        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_unref(%p, %p)", callback_data, event);
        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------
    static int carla_lv2_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
    {
        qDebug("Lv2Plugin::carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);

        char buf[8196];
        vsprintf(buf, fmt, ap);

        if (buf[0] == 0)
            return 0;

        switch (type)
        {
        case CARLA_URI_MAP_ID_LOG_ERROR:
            qCritical("%s", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_NOTE:
            printf("%s\n", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_TRACE:
            qDebug("%s", buf);
            break;
        case CARLA_URI_MAP_ID_LOG_WARNING:
            qWarning("%s", buf);
            break;
        default:
            break;
        }

        return strlen(buf);
    }

    static int carla_lv2_log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
    {
        qDebug("Lv2Plugin::carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);

        va_list args;
        va_start(args, fmt);
        const int ret = carla_lv2_log_vprintf(handle, type, fmt, args);
        va_end(args);

        return ret;
    }

    // ----------------- Programs Feature ------------------------------------------------
    static void carla_lv2_program_changed(LV2_Programs_Handle handle, int32_t index)
    {
        qDebug("Lv2Plugin::carla_lv2_program_changed(%p, %i)", handle, index);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            plugin->update_program(index);
        }
    }

    // ----------------- State Feature ---------------------------------------------------
    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_make_path(%p, %p)", handle, path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        return strdup(absolute_path);
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
        return strdup(abstract_path);
    }

    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            const char* uri_key = plugin->get_custom_uri_string(key);

            if (uri_key && value && (flags & LV2_STATE_IS_POD) > 0)
            {
                qDebug("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Got uri_key and flags", handle, key, value, size, type, flags);

                CustomDataType dtype;

                if (type == CARLA_URI_MAP_ID_ATOM_STRING)
                    dtype = CUSTOM_DATA_STRING;
                else if (type == CARLA_URI_MAP_ID_ATOM_PATH)
                    dtype = CUSTOM_DATA_PATH;
                else if (type == CARLA_URI_MAP_ID_ATOM_CHUNK)
                    dtype = CUSTOM_DATA_CHUNK;
                else if (type >= CARLA_URI_MAP_ID_COUNT)
                    dtype = CUSTOM_DATA_BINARY;
                else
                    dtype = CUSTOM_DATA_INVALID;

                if (dtype != CUSTOM_DATA_INVALID)
                {
                    // Check if we already have this key
                    for (int i=0; i < plugin->custom.count(); i++)
                    {
                        if (strcmp(plugin->custom[i].key, uri_key) == 0)
                        {
                            free((void*)plugin->custom[i].value);

                            if (dtype == CUSTOM_DATA_STRING || dtype == CUSTOM_DATA_PATH)
                            {
                                plugin->custom[i].value = strdup((const char*)value);
                            }
                            else
                            {
                                QByteArray chunk((const char*)value, size);
                                plugin->custom[i].value = strdup(chunk.toBase64().constData());
                            }

                            return LV2_STATE_SUCCESS;
                        }
                    }

                    // Add a new one then
                    CustomData new_data;
                    new_data.type = dtype;
                    new_data.key  = strdup(uri_key);

                    if (dtype == CUSTOM_DATA_STRING || dtype == CUSTOM_DATA_PATH)
                    {
                        new_data.value = strdup((const char*)value);
                    }
                    else
                    {
                        QByteArray chunk((const char*)value, size);
                        new_data.value = strdup(chunk.toBase64().constData());
                    }

                    plugin->custom.append(new_data);

                    return LV2_STATE_SUCCESS;
                }
                else
                {
                    qCritical("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid type '%s'", handle, key, value, size, type, flags, customdatatype2str(dtype));
                    return LV2_STATE_ERR_BAD_TYPE;
                }
            }
            else
            {
                qWarning("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid key, flags or value", handle, key, value, size, type, flags);
                return LV2_STATE_ERR_BAD_FLAGS;
            }
        }
        else
        {
            qCritical("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid handle", handle, key, value, size, type, flags);
            return LV2_STATE_ERR_UNKNOWN;
        }
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            const char* uri_key = plugin->get_custom_uri_string(key);

            if (uri_key)
            {
                const char* string_data = nullptr;
                CustomDataType dtype = CUSTOM_DATA_INVALID;

                for (int i=0; i < plugin->custom.count(); i++)
                {
                    if (strcmp(plugin->custom[i].key, uri_key) == 0)
                    {
                        dtype = plugin->custom[i].type;
                        string_data = plugin->custom[i].value;
                        break;
                    }
                }

                if (string_data)
                {
                    *size  = 0;
                    *type  = 0;
                    *flags = 0;

                    if (dtype == CUSTOM_DATA_STRING)
                    {
                        *size = strlen(string_data);
                        *type = CARLA_URI_MAP_ID_ATOM_STRING;
                        return string_data;
                    }
                    else if (dtype == CUSTOM_DATA_PATH)
                    {
                        *size = strlen(string_data);
                        *type = CARLA_URI_MAP_ID_ATOM_PATH;
                        return string_data;
                    }
                    else if (dtype == CUSTOM_DATA_CHUNK || dtype == CUSTOM_DATA_BINARY)
                    {
                        static QByteArray chunk;
                        chunk = QByteArray::fromBase64(string_data);

                        *size = chunk.size();
                        *type = (dtype == CUSTOM_DATA_CHUNK) ? CARLA_URI_MAP_ID_ATOM_CHUNK : key;
                        return chunk.constData();
                    }
                    else
                        qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key type '%s'", handle, key, size, type, flags, customdatatype2str(dtype));
                }
                else
                    qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key '%s'", handle, key, size, type, flags, uri_key);
            }
            else
                qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Failed to find key", handle, key, size, type, flags);
        }
        else
            qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid handle", handle, key, size, type, flags);

        return nullptr;
    }

    // ----------------- URI-Map Feature -------------------------------------------------
    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("Lv2Plugin::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        else if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
        else if (strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM;
        else if (strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT;

        // Log types
        else if (strcmp(uri, LV2_LOG__Error) == 0)
            return CARLA_URI_MAP_ID_LOG_ERROR;
        else if (strcmp(uri, LV2_LOG__Note) == 0)
            return CARLA_URI_MAP_ID_LOG_NOTE;
        else if (strcmp(uri, LV2_LOG__Trace) == 0)
            return CARLA_URI_MAP_ID_LOG_TRACE;
        else if (strcmp(uri, LV2_LOG__Warning) == 0)
            return CARLA_URI_MAP_ID_LOG_WARNING;

        // Others
        else if (strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;

        // Custom types
        if (data)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)data;
            return plugin->get_custom_uri_id(uri);
        }

        return CARLA_URI_MAP_ID_NULL;
    }

    // ----------------- URID Feature ----------------------------------------------------
    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_map(%p, %s)", handle, uri);

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        else if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
        else if (strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM;
        else if (strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT;

        // Log types
        else if (strcmp(uri, LV2_LOG__Error) == 0)
            return CARLA_URI_MAP_ID_LOG_ERROR;
        else if (strcmp(uri, LV2_LOG__Note) == 0)
            return CARLA_URI_MAP_ID_LOG_NOTE;
        else if (strcmp(uri, LV2_LOG__Trace) == 0)
            return CARLA_URI_MAP_ID_LOG_TRACE;
        else if (strcmp(uri, LV2_LOG__Warning) == 0)
            return CARLA_URI_MAP_ID_LOG_WARNING;

        // Others
        else if (strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_id(uri);
        }

        return CARLA_URI_MAP_ID_NULL;
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_unmap(%p, %i)", handle, urid);

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        else if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        else if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        else if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;
        else if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
            return LV2_ATOM__atomTransfer;
        else if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
            return LV2_ATOM__eventTransfer;

        // Log types
        else if (urid == CARLA_URI_MAP_ID_LOG_ERROR)
            return LV2_LOG__Error;
        else if (urid == CARLA_URI_MAP_ID_LOG_NOTE)
            return LV2_LOG__Note;
        else if (urid == CARLA_URI_MAP_ID_LOG_TRACE)
            return LV2_LOG__Trace;
        else if (urid == CARLA_URI_MAP_ID_LOG_WARNING)
            return LV2_LOG__Warning;

        // Others
        else if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return LV2_MIDI__MidiEvent;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_string(urid);
        }

        return nullptr;
    }

    // ----------------- Worker Feature --------------------------------------------------
    static LV2_Worker_Status carla_lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);

        if (handle)
        {
            //            Lv2Plugin* plugin = (Lv2Plugin*)handle;

            //            if (carla_jack_on_freewheel())
            //            {
            //                PluginPostEvent event;
            //                event.valid = true;
            //                event.type  = PostEventCustom;
            //                event.index = size;
            //                event.value = 0.0;
            //                event.cdata = data;
            //                plugin->run_custom_event(&event);
            //            }
            //            else
            //                plugin->postpone_event(PostEventCustom, size, 0.0, data);

            return LV2_WORKER_SUCCESS;
        }

        return LV2_WORKER_ERR_UNKNOWN;
    }

    static LV2_Worker_Status carla_lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;

            if (plugin->ext.worker)
            {
                //plugin->ext.worker->work_response(plugin->handle, size, data);
                return LV2_WORKER_SUCCESS;
            }
        }

        return LV2_WORKER_ERR_UNKNOWN;
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------
    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_port_map(%p, %s)", handle, symbol);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;

            for (uint32_t i=0; i < plugin->rdf_descriptor->PortCount; i++)
            {
                if (strcmp(plugin->rdf_descriptor->Ports[i].Symbol, symbol) == 0)
                    return i;
            }
        }

        return 0;
    }

    // ----------------- UI Resize Feature -----------------------------------------------
    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            plugin->gui.width  = width;
            plugin->gui.height = height;
            callback_action(CALLBACK_RESIZE_GUI, plugin->m_id, width, height, 0.0);
            return 0;
        }

        return 1;
    }

    // ----------------- External UI Feature ---------------------------------------------
    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        qDebug("Lv2Plugin::carla_lv2_external_ui_closed(%p)", controller);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;
            plugin->gui.visible = false;

            if (plugin->ui.handle && plugin->ui.descriptor && plugin->ui.descriptor->cleanup)
                plugin->ui.descriptor->cleanup(plugin->ui.handle);

            plugin->ui.handle = nullptr;
            callback_action(CALLBACK_SHOW_GUI, plugin->m_id, 0, 0, 0.0);
        }
    }

    // ----------------- UI Extension ----------------------------------------------------
    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;

            if (format == 0)
            {
                if (buffer_size == sizeof(float))
                {
                    float value = *(float*)buffer;
                    plugin->set_parameter_value_by_rindex(port_index, value, false, true, true);
                }
            }
            else if (format == CARLA_URI_MAP_ID_MIDI_EVENT)
            {
                const uint8_t* data = (const uint8_t*)buffer;
                uint8_t status = data[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && data[2] == 0)
                    status -= 0x10;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = data[2];
                    plugin->send_midi_note(false, note, 0, false, true, true);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = data[2];
                    uint8_t velo = data[3];
                    plugin->send_midi_note(true, note, velo, false, true, true);
                }
            }
            else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
            {
                // TODO
                //LV2_Atom* atom = (LV2_Atom*)buffer;
                //QByteArray chunk((const char*)buffer, buffer_size);
            }
            else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
            {
                LV2_Atom* atom = (LV2_Atom*)buffer;
                plugin->ui.descriptor->port_event(plugin->ui.handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
            }
        }
    }

private:
    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    const LV2_RDF_Descriptor* rdf_descriptor;
    LV2_Feature* features[lv2_feature_count+1];

    struct {
        lv2dynparam_plugin_callbacks* dynparam;
        LV2_State_Interface* state;
        LV2_Worker_Interface* worker;
        LV2_Programs_Interface* programs;
        LV2_Programs_UI_Interface* uiprograms;
    } ext;

    struct {
        void* lib;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI* rdf_descriptor;
    } ui;

    struct {
        GuiType type;
        bool visible;
        bool resizable;
        int width;
        int height;
    } gui;

    PluginEventData evin;
    PluginEventData evout;
    Lv2ParameterData* lv2param;

    //LV2_Atom_Forge atom_forge;
    QVector<const char*> custom_uri_ids;
};

int osc_handle_lv2_event_transfer(CarlaPlugin* plugin, lo_arg** argv)
{
    Lv2Plugin* lv2plugin = (Lv2Plugin*)plugin;

    const char* type  = (const char*)&argv[0]->s;
    const char* key   = (const char*)&argv[1]->s;
    const char* value = (const char*)&argv[2]->s;
    lv2plugin->handle_event_transfer(type, key, value);

    return 0;
}

short add_plugin_lv2(const char* filename, const char* label)
{
    qDebug("add_plugin_lv2(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        Lv2Plugin* plugin = new Lv2Plugin(id);

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

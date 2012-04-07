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

#ifdef BUILD_BRIDGE
#error Cannot use bridge for lv2 plugins
#endif

#include "carla_plugin.h"

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/data-access.h"
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/instance-access.h"
#include "lv2/state.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"
#include "lv2/ui.h"

#include "lv2/lv2-miditype.h"
#include "lv2/lv2-midifunctions.h"
#include "lv2/lv2_rtmempool.h"
#include "lv2/lv2_external_ui.h"

#include "lv2_rdf.h"

extern "C" {
#include "lv2-rtmempool/rtmempool.h"
}

#include <QtGui/QDialog>
#include <QtGui/QLayout>

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x7FFF; // 32767

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x1000;
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x2000;

// parameter hints
const unsigned int PARAMETER_IS_TRIGGER          = 0x100;
const unsigned int PARAMETER_HAS_STRICT_BOUNDS   = 0x200;

// feature ids
const uint32_t lv2_feature_id_uri_map         = 0;
const uint32_t lv2_feature_id_urid_map        = 1;
const uint32_t lv2_feature_id_urid_unmap      = 2;
const uint32_t lv2_feature_id_event           = 3;
const uint32_t lv2_feature_id_rtmempool       = 4;
const uint32_t lv2_feature_id_data_access     = 5;
const uint32_t lv2_feature_id_instance_access = 6;
const uint32_t lv2_feature_id_ui_parent       = 7;
const uint32_t lv2_feature_id_ui_resize       = 8;
const uint32_t lv2_feature_id_external_ui     = 9;
const uint32_t lv2_feature_id_external_ui_old = 10;
const uint32_t lv2_feature_count              = 11;

// event data/types
const unsigned int CARLA_EVENT_DATA_ATOM    = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT   = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL = 0x04;
const unsigned int CARLA_EVENT_TYPE_MIDI    = 0x10;
const unsigned int CARLA_EVENT_TYPE_TIME    = 0x20;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL        = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING = 1;
const uint32_t CARLA_URI_MAP_ID_EVENT_MIDI  = 2;
const uint32_t CARLA_URI_MAP_ID_EVENT_TIME  = 3;
const uint32_t CARLA_URI_MAP_ID_COUNT       = 4;

enum Lv2ParameterDataType {
    LV2_PARAMETER_TYPE_CONTROL,
    LV2_PARAMETER_TYPE_SOMETHING_ELSE_HERE
};

struct EventData {
    unsigned int types;
    jack_port_t* port;
    union {
        LV2_Event_Buffer* e;
        LV2_MIDI* m;
    } buffer;
};

struct PluginEventData {
    uint32_t count;
    EventData* data;
};

struct Lv2ParameterData {
    Lv2ParameterDataType type;
    union {
        float control;
    };
};

class Lv2Plugin : public CarlaPlugin
{
public:
    Lv2Plugin() :
        CarlaPlugin()
    {
        qDebug("Lv2Plugin::Lv2Plugin()");
        m_type = PLUGIN_LV2;

        ain_rindexes  = nullptr;
        aout_rindexes = nullptr;
        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        ui.lib = nullptr;
        ui.handle = nullptr;
        ui.descriptor = nullptr;
        ui.rdf_descriptor = nullptr;

        gui.type = GUI_NONE;
        gui.visible = false;
        gui.resizable = false;
        gui.width = 0;
        gui.height = 0;

        // Fill pre-set URI keys
        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            custom_uri_ids.append(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;
    }

    virtual ~Lv2Plugin()
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

            case GUI_EXTERNAL_LV2:
                if (gui.visible && ui.widget)
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);

                break;

            default:
                break;
            }

            if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
                ui.descriptor->cleanup(ui.handle);

            ui.lib = nullptr;
            ui.handle = nullptr;
            ui.descriptor = nullptr;
            ui.rdf_descriptor = nullptr;

            if (features[lv2_feature_id_data_access] && features[lv2_feature_id_data_access]->data)
                delete (LV2_Extension_Data_Feature*)features[lv2_feature_id_data_access]->data;

            if (features[lv2_feature_id_ui_resize] && features[lv2_feature_id_ui_resize]->data)
                delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

            if (features[lv2_feature_id_external_ui] && features[lv2_feature_id_external_ui]->data)
            {
                free((void*)((lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data)->plugin_human_id);
                delete (lv2_external_ui_host*)features[lv2_feature_id_external_ui]->data;
            }

            ui_lib_close();
        }

        if (handle && descriptor->deactivate && m_active_before)
            descriptor->deactivate(handle);

        if (handle && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
            delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

        if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
            delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

        if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
            delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

        if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
            delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

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

    virtual PluginCategory category()
    {
        LV2_Property Category = rdf_descriptor->Type;

        // Specific Types
        if (Category & LV2_CLASS_REVERB)
            return PLUGIN_CATEGORY_DELAY;

        // Pre-set LV2 Types
        else if (LV2_IS_GENERATOR(Category))
            return PLUGIN_CATEGORY_SYNTH;
        else if (LV2_IS_UTILITY(Category))
            return PLUGIN_CATEGORY_UTILITY;
        else if (LV2_IS_SIMULATOR(Category))
            return PLUGIN_CATEGORY_OUTRO;
        else if (LV2_IS_DELAY(Category))
            return PLUGIN_CATEGORY_DELAY;
        else if (LV2_IS_MODULATOR(Category))
            return PLUGIN_CATEGORY_MODULATOR;
        else if (LV2_IS_FILTER(Category))
            return PLUGIN_CATEGORY_FILTER;
        else if (LV2_IS_EQUALISER(Category))
            return PLUGIN_CATEGORY_EQ;
        else if (LV2_IS_SPECTRAL(Category))
            return PLUGIN_CATEGORY_UTILITY;
        else if (LV2_IS_DISTORTION(Category))
            return PLUGIN_CATEGORY_OUTRO;
        else if (LV2_IS_DYNAMICS(Category))
            return PLUGIN_CATEGORY_DYNAMICS;

        // TODO - try to get category from label
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return rdf_descriptor->UniqueID;
    }

    virtual uint32_t min_count()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i < evin.count; i++)
        {
            if (evin.data[i].types & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    virtual uint32_t mout_count()
    {
        uint32_t count = 0;

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].types & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    virtual uint32_t param_scalepoint_count(uint32_t param_id)
    {
        int32_t rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[rindex].ScalePointCount;
    }

    virtual double get_parameter_value(uint32_t param_id)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            return lv2param[param_id].control;
        default:
            return 0.0;
        }
    }

    virtual double get_parameter_scalepoint_value(uint32_t param_id, uint32_t scalepoint_id)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        return rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Value;
    }

    virtual void get_label(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->URI, STR_MAX);
    }

    virtual void get_maker(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->Author, STR_MAX);
    }

    virtual void get_copyright(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->License, STR_MAX);
    }

    virtual void get_real_name(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->Name, STR_MAX);
    }

    virtual void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Name, STR_MAX);
    }

    virtual void get_parameter_symbol(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[rindex].Symbol, STR_MAX);
    }

    virtual void get_parameter_label(uint32_t param_id, char* buf_str)
    {
        int32_t rindex = param.data[param_id].rindex;

        LV2_RDF_Port* Port = &rdf_descriptor->Ports[rindex];

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

    virtual void get_parameter_scalepoint_label(uint32_t param_id, uint32_t scalepoint_id, char* buf_str)
    {
        int32_t param_rindex = param.data[param_id].rindex;
        strncpy(buf_str, rdf_descriptor->Ports[param_rindex].ScalePoints[scalepoint_id].Label, STR_MAX);
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        info->type      = gui.type;
        info->resizable = gui.resizable;
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        switch (lv2param[param_id].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            lv2param[param_id].control = value;
            break;
        default:
            break;
        }

        if (gui_send && gui.visible)
        {
            switch (gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_X11:
            case GUI_EXTERNAL_LV2:
                if (ui.descriptor->port_event)
                {
                    float fvalue = value;
                    ui.descriptor->port_event(ui.handle, param.data[param_id].rindex, sizeof(float), 0, &fvalue);
                }
                break;

            case GUI_EXTERNAL_OSC:
                //osc_send_control(&osc.data, param.data[parameter_id].rindex, value);
                break;

            default:
                break;
            }
        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    virtual void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send)
    {
        if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
        {
            LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (state)
                state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
        }

        CarlaPlugin::set_custom_data(dtype, key, value, gui_send);
    }

    virtual void set_gui_data(int, void* ptr)
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            if (ui.widget)
            {
                QDialog* qtPtr  = (QDialog*)ptr;
                QWidget* widget = (QWidget*)ui.widget;

                qtPtr->layout()->addWidget(widget);
                widget->adjustSize();
                widget->setParent(qtPtr);
                widget->show();
            }
            break;

        case GUI_INTERNAL_X11:
            if (ui.descriptor)
            {
                QDialog* qtPtr  = (QDialog*)ptr;
                features[lv2_feature_id_ui_parent]->data = (void*)qtPtr->winId();

                ui.handle = ui.descriptor->instantiate(ui.descriptor,
                                                       descriptor->URI,
                                                       ui.rdf_descriptor->Bundle,
                                                       carla_lv2_ui_write_function,
                                                       this,
                                                       &ui.widget,
                                                       features);
                update_ui_ports();
            }
            break;

        default:
            break;
        }
    }

    virtual void show_gui(bool yesno)
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            gui.visible = yesno;
            break;

        case GUI_INTERNAL_X11:
            gui.visible = yesno;

            if (gui.visible && gui.width > 0 && gui.height > 0)
                callback_action(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);

            break;

//        case GUI_EXTERNAL_OSC:
//            if (yesno)
//            {
//                gui.show_now = true;

//                // 40 re-tries, 4 secs
//                for (int j=1; j<40 && gui.show_now; j++)
//                {
//                    if (osc.data.target)
//                    {
//                        osc_send_show(&osc.data);
//                        gui.visible = true;
//                        return;
//                    }
//                    else
//                        // 100 ms
//                        usleep(100000);
//                }

//                qDebug("Lv2AudioPlugin::show_gui() - GUI timeout");
//                callback_action(CALLBACK_SHOW_GUI, id, 0, 0, 0.0);
//            }
//            else
//            {
//                gui.visible = false;
//                osc_send_hide(&osc.data);
//                osc_send_quit(&osc.data);
//                osc_clear_data(&osc.data);
//            }
//            break;

        case GUI_EXTERNAL_LV2:
            if (ui.handle == nullptr)
                reinit_external_ui();

            if (ui.handle && ui.widget)
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

                    if (ui.descriptor->cleanup)
                        ui.descriptor->cleanup(ui.handle);

                    ui.handle = nullptr;
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

    virtual void idle_gui()
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
                    for (uint32_t i=0; i < param.count; i++)
                    {
                        if (param.data[i].type == PARAMETER_OUTPUT && (param.data[i].hints & PARAMETER_IS_AUTOMABLE) > 0)
                        {
                            float value = get_parameter_value(i);
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

    virtual void reload()
    {
        qDebug("Lv2Plugin::reload()");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        if (_id >= 0)
            remove_from_jack();

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
        }

//        if (params == 0 && (hints & PLUGIN_HAS_EXTENSION_DYNPARAM) > 0)
//        {
//            dynparam_plugin = (lv2dynparam_plugin_callbacks*)descriptor->extension_data(LV2DYNPARAM_URI);
//            if (dynparam_plugin)
//                dynparam_plugin->host_attach(handle, &dynparam_host, this);
//        }

        if (ains > 0)
        {
            ain.ports    = new jack_port_t*[ains];
            ain_rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new jack_port_t*[aouts];
            aout_rindexes = new uint32_t[aouts];
        }

        if (ev_ins > 0)
        {
            evin.data = new EventData[ev_ins];

            for (j=0; j < ev_ins; j++)
            {
                evin.data[j].port = nullptr;

                if (event_data_type == CARLA_EVENT_DATA_ATOM)
                {
                    evin.data[j].types = CARLA_EVENT_DATA_ATOM;
                    // TODO
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evin.data[j].types    = CARLA_EVENT_DATA_EVENT;
                    evin.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evin.data[j].types    = CARLA_EVENT_DATA_MIDI_LL;
                    evin.data[j].buffer.m = new LV2_MIDI;
                    evin.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evin.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evin.data[j].types  = 0;
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
                    evout.data[j].types = CARLA_EVENT_DATA_ATOM;
                    // TODO
                }
                else if (event_data_type == CARLA_EVENT_DATA_EVENT)
                {
                    evout.data[j].types    = CARLA_EVENT_DATA_EVENT;
                    evout.data[j].buffer.e = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (event_data_type == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evout.data[j].types     = CARLA_EVENT_DATA_MIDI_LL;
                    evout.data[j].buffer.m = new LV2_MIDI;
                    evout.data[j].buffer.m->capacity = MAX_EVENT_BUFFER;
                    evout.data[j].buffer.m->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
                else
                    evout.data[j].types  = 0;
            }
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            lv2param     = new Lv2ParameterData[params];
        }

        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];
        bool needs_cin  = false;
        bool needs_cout = false;

        for (uint32_t i=0; i < PortCount; i++)
        {
            const LV2_Property PortType  = rdf_descriptor->Ports[i].Type;

            if (LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_CV(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType))
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
                    strncpy(port_name, rdf_descriptor->Ports[i].Name, port_name_size/2);
            }

            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                    ain_rindexes[j] = i;
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    aout_rindexes[j] = i;
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
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer.e);

                    if (LV2_IS_PORT_EVENT_MIDI(PortType))
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                    if (LV2_IS_PORT_EVENT_TIME(PortType))
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_TIME;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.e);

                    if (LV2_IS_PORT_EVENT_MIDI(PortType))
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
                    }
                    if (LV2_IS_PORT_EVENT_TIME(PortType))
                    {
                        evout.data[j].types |= CARLA_EVENT_TYPE_TIME;
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

                    evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                    evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer.m);

                    evout.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                    evout.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
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

                // default value
                if (LV2_HAVE_DEFAULT_PORT_POINT(PortPoints.Hints))
                    def = PortPoints.Default;
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

                if (max - min <= 0.0)
                {
                    qWarning("Broken plugin parameter -> max - min <= 0");
                    max = min + 0.1;
                }

                if (LV2_IS_PORT_SAMPLE_RATE(PortProps))
                {
                    double sample_rate = get_sample_rate();
                    min *= sample_rate;
                    max *= sample_rate;
                    def *= sample_rate;
                    param.data[j].hints |= PARAMETER_USES_SAMPLERATE;
                }

                if (LV2_IS_PORT_INTEGER(PortProps))
                {
                    step = 1.0;
                    step_small = 1.0;
                    step_large = 10.0;
                }
                else if (LV2_IS_PORT_TOGGLED(PortProps))
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
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
                    param.data[j].type = PARAMETER_INPUT;
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
                    param.data[j].type = PARAMETER_OUTPUT;
                    param.data[j].hints |= PARAMETER_IS_ENABLED;

                    if (LV2_IS_PORT_LATENCY(PortProps) == false)
                    {
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needs_cout = true;
                    }
                    else
                    {
                        // latency parameter
                        min = 0;
                        max = get_sample_rate();
                        def = 0;
                        step = 1;
                        step_small = 1;
                        step_large = 1;
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

                if (LV2_IS_PORT_HAS_STRICT_BOUNDS(PortProps) /*|| (force_strict_bounds && LV2_IS_PORT_OUTPUT(PortType))*/)
                    param.data[j].hints |= PARAMETER_HAS_STRICT_BOUNDS;

                // check if parameter is not enabled or automable
                if (LV2_IS_PORT_NOT_AUTOMATIC(PortProps) || LV2_IS_PORT_NOT_ON_GUI(PortProps))
                    param.data[j].hints &= ~PARAMETER_IS_ENABLED;

                if (LV2_IS_PORT_CAUSES_ARTIFACTS(PortProps) || LV2_IS_PORT_EXPENSIVE(PortProps))
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

        reload_programs(true);

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

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client == false)
#endif
            jack_activate(jack_client);
    }

    virtual void prepare_for_save()
    {
        if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
        {
            LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE__interface);

            if (state)
                state->save(handle, carla_lv2_state_store, this, 0, features);
        }
    }

    virtual void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        uint32_t midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* ains_buffer[ain.count];
        jack_default_audio_sample_t* aouts_buffer[aout.count];
        void* evins_buffer[evin.count];
        void* evouts_buffer[evout.count];

        // different midi APIs
        LV2_MIDIState evin_states[evin.count];
        LV2_Event_Iterator evin_iters[evin.count];

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evin.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evin.data[i].buffer.e + 1));
                lv2_event_begin(&evin_iters[i], evin.data[i].buffer.e);
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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
            if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evout.data[i].buffer.e, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evout.data[i].buffer.e + 1));
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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
            short j2 = (ain.count == 1) ? 0 : 1;

            for (k=0; k<nframes; k++)
            {
                if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                    ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                if (abs_d(ains_buffer[j2][k]) > ains_peak_tmp[1])
                    ains_peak_tmp[1] = abs_d(ains_buffer[j2][k]);
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

                jack_midi_data_t status = pin_event.buffer[0];
                unsigned char channel   = status & 0x0F;

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

                    // Control GUI stuff (channel 0 only)
                    if (channel == 0)
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

                                m_active_before = false;
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
                        if (param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0 && param.data[k].midi_channel == channel && param.data[k].midi_cc == control)
                        {
                            value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;
                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    uint32_t mbank_id = next_bank_id;
                    uint32_t mprog_id = pin_event.buffer[1]; // & 0x7F;

                    // TODO ...

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
                        if (evin.data[k].types & CARLA_EVENT_TYPE_MIDI)
                        {
                            if (evin.data[k].types & CARLA_EVENT_DATA_ATOM)
                                continue; // TODO
                            else if (evin.data[k].types & CARLA_EVENT_DATA_EVENT)
                                lv2_event_write(&evin_iters[k], 0, 0, CARLA_URI_MAP_ID_EVENT_MIDI, 3, midi_event);
                            else if (evin.data[k].types & CARLA_EVENT_DATA_MIDI_LL)
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
                    if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
                        continue; // TODO
                    else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
                        lv2_event_write(&evin_iters[i], min_event.time, 0, CARLA_URI_MAP_ID_EVENT_MIDI, min_event.size, min_event.buffer);
                    else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                descriptor->connect_port(handle, ain_rindexes[i], ains_buffer[i]);

            for (i=0; i < aout.count; i++)
                descriptor->connect_port(handle, aout_rindexes[i], aouts_buffer[i]);

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
            double bal_rangeL, bal_rangeR;
            jack_default_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                for (k=0; k<nframes; k++)
                {
                    if ((m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0)
                    {
                        if (aout.count == 1)
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                        else
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                    }

                    if (m_hints & PLUGIN_CAN_VOLUME)
                        aouts_buffer[i][k] *= x_vol;
                }

                // Balance
                if (m_hints & PLUGIN_CAN_BALANCE)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_default_audio_sample_t)*nframes);

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
                if (i < 2)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                            aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                    }
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_default_audio_sample_t)*nframes);

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
                    event_buffer[0] = 0xB0 + param.data[k].midi_channel;
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

            if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
            {
            }
            else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
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
            else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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

    virtual void delete_buffers()
    {
        qDebug("Lv2Plugin::delete_buffers() - start");

        if (ain.count > 0)
            delete[] ain_rindexes;

        if (aout.count > 0)
            delete[] aout_rindexes;

        if (param.count > 0)
            delete[] lv2param;

        if (evin.count > 0)
        {
            for (uint32_t i=0; i < evin.count; i++)
            {
                if (evin.data[i].types & CARLA_EVENT_DATA_ATOM)
                {
                }
                else if (evin.data[i].types & CARLA_EVENT_DATA_EVENT)
                {
                    free(evin.data[i].buffer.e);
                }
                else if (evin.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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
                if (evout.data[i].types & CARLA_EVENT_DATA_ATOM)
                {
                }
                else if (evout.data[i].types & CARLA_EVENT_DATA_EVENT)
                {
                    free(evout.data[i].buffer.e);
                }
                else if (evout.data[i].types & CARLA_EVENT_DATA_MIDI_LL)
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
        LV2_RDF_UI* rdf_ui = &rdf_descriptor->UIs[ui_id];

        for (uint32_t i=0; i < rdf_ui->FeatureCount; i++)
        {
            if (strcmp(rdf_ui->Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0 || strcmp(rdf_ui->Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
    }

    void reinit_external_ui()
    {
        qDebug("Lv2Plugin::reinit_external_ui()");

        ui.widget = nullptr;
        ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle && ui.widget)
        {
            update_ui_ports();
        }
        else
        {
            qDebug("Lv2Plugin::reinit_external_ui() - Failed to re-initiate external UI");

            ui.handle = nullptr;
            ui.widget = nullptr;
            callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
        }
    }

    void update_ui_ports()
    {
        qDebug("Lv2Plugin::update_ui_ports()");

        if (ui.handle && ui.descriptor && ui.descriptor->port_event)
        {
            float value;
            for (uint32_t i=0; i < param.count; i++)
            {
                value = get_parameter_value(i);
                ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
            }
        }
    }

#if 0
    // FIXME - resolve jack deactivate
    void lv2_remove_from_jack()
    {
        qDebug("Lv2Plugin::lv2_remove_from_jack() - start");

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

        qDebug("Lv2Plugin::lv2_remove_from_jack() - end");
    }
#endif

    bool init(const char* filename, const char* URI, void* extra_stuff)
    {
        LV2_RDF_Descriptor* rdf_descriptor_ = (LV2_RDF_Descriptor*)extra_stuff;

        if (rdf_descriptor_)
        {
            rdf_descriptor = lv2_rdf_dup(rdf_descriptor_);

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
                            if (bool(LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_CONTROL(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType)) == false)
                            {
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
                            else
                                qDebug("Plugin wants a feature that is not supported:\n%s", rdf_descriptor->Features[i].URI);
                        }

                        // Check extensions (...)
                        for (i=0; i < rdf_descriptor->ExtensionCount; i++)
                        {
                            if (strcmp(rdf_descriptor->Extensions[i], LV2_STATE__interface) == 0)
                                m_hints |= PLUGIN_HAS_EXTENSION_STATE;
                            //else if (strcmp(rdf_descriptor->Extensions[i], LV2DYNPARAM_URI) == 0)
                            //    plugin->hints |= PLUGIN_HAS_EXTENSION_DYNPARAM;
                            else
                                qDebug("Plugin has non-supported extension: '%s'", rdf_descriptor->Extensions[i]);
                        }

                        if (can_continue)
                        {
                            // Initialize features
                            LV2_URI_Map_Feature* URI_Map_Feature = new LV2_URI_Map_Feature;
                            URI_Map_Feature->callback_data       = this;
                            URI_Map_Feature->uri_to_id           = carla_lv2_uri_to_id;

                            LV2_URID_Map* URID_Map_Feature       = new LV2_URID_Map;
                            URID_Map_Feature->handle             = this;
                            URID_Map_Feature->map                = carla_lv2_urid_map;

                            LV2_URID_Unmap* URID_Unmap_Feature   = new LV2_URID_Unmap;
                            URID_Unmap_Feature->handle           = this;
                            URID_Unmap_Feature->unmap            = carla_lv2_urid_unmap;

                            LV2_Event_Feature* Event_Feature     = new LV2_Event_Feature;
                            Event_Feature->callback_data         = this;
                            Event_Feature->lv2_event_ref         = nullptr;
                            Event_Feature->lv2_event_unref       = nullptr;

                            lv2_rtsafe_memory_pool_provider* RT_MemPool_Feature = new lv2_rtsafe_memory_pool_provider;
                            rtmempool_allocator_init(RT_MemPool_Feature);

                            features[lv2_feature_id_uri_map]          = new LV2_Feature;
                            features[lv2_feature_id_uri_map]->URI     = LV2_URI_MAP_URI;
                            features[lv2_feature_id_uri_map]->data    = URI_Map_Feature;

                            features[lv2_feature_id_urid_map]         = new LV2_Feature;
                            features[lv2_feature_id_urid_map]->URI    = LV2_URID_MAP_URI;
                            features[lv2_feature_id_urid_map]->data   = URID_Map_Feature;

                            features[lv2_feature_id_urid_unmap]       = new LV2_Feature;
                            features[lv2_feature_id_urid_unmap]->URI  = LV2_URID_UNMAP_URI;
                            features[lv2_feature_id_urid_unmap]->data = URID_Unmap_Feature;

                            features[lv2_feature_id_event]            = new LV2_Feature;
                            features[lv2_feature_id_event]->URI       = LV2_EVENT_URI;
                            features[lv2_feature_id_event]->data      = Event_Feature;

                            features[lv2_feature_id_rtmempool]        = new LV2_Feature;
                            features[lv2_feature_id_rtmempool]->URI   = LV2_RTSAFE_MEMORY_POOL_URI;
                            features[lv2_feature_id_rtmempool]->data  = RT_MemPool_Feature;

                            handle = descriptor->instantiate(descriptor, get_sample_rate(), rdf_descriptor->Bundle, features);

                            if (handle)
                            {
                                m_filename = strdup(filename);
                                m_name = get_unique_name(rdf_descriptor->Name);

                                if (carla_jack_register_plugin(this, &jack_client))
                                {
                                    // ----------------- GUI Stuff -------------------------------------------------------

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

                                        bool is_bridged = false; //(iFinal == eQt4 || iFinal == eX11 || iFinal == eGtk2);

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

                                                            if (is_bridged)
                                                            {
                                                                const char* osc_binary;

                                                                switch (UiType)
                                                                {
                                                                case LV2_UI_QT4:
                                                                    qDebug("Will use LV2 Qt4 UI, bridged");
                                                                    osc_binary = nullptr;
                                                                    break;
                                                                case LV2_UI_X11:
                                                                    qDebug("Will use LV2 X11 UI, bridged");
                                                                    osc_binary = nullptr;
                                                                    break;
                                                                case LV2_UI_GTK2:
                                                                    qDebug("Will use LV2 Gtk2 UI, bridged");
                                                                    osc_binary = nullptr;
                                                                    break;
                                                                default:
                                                                    qDebug("Will use LV2 Unknown UI, bridged");
                                                                    osc_binary = nullptr;
                                                                    break;
                                                                }

                                                                if (osc_binary)
                                                                {
                                                                    gui.type = GUI_EXTERNAL_OSC;
                                                                    osc.thread = new CarlaPluginThread(this, CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
                                                                    osc.thread->setOscData("lv2-gtk here", descriptor->URI, ui.descriptor->URI, ui.rdf_descriptor->Binary, ui.rdf_descriptor->Bundle);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                // Initialize UI features
                                                                LV2_Extension_Data_Feature* UI_Data_Feature = new LV2_Extension_Data_Feature;
                                                                UI_Data_Feature->data_access                = descriptor->extension_data;

                                                                lv2_external_ui_host* External_UI_Feature   = new lv2_external_ui_host;
                                                                External_UI_Feature->ui_closed              = carla_lv2_external_ui_closed;
                                                                External_UI_Feature->plugin_human_id        = strdup(gui_title.toUtf8().constData()); // NOTE

                                                                LV2UI_Resize* UI_Resize_Feature             = new LV2UI_Resize;
                                                                UI_Resize_Feature->handle                   = this;
                                                                UI_Resize_Feature->ui_resize                = carla_lv2_ui_resize;

                                                                features[lv2_feature_id_data_access]           = new LV2_Feature;
                                                                features[lv2_feature_id_data_access]->URI      = LV2_DATA_ACCESS_URI;
                                                                features[lv2_feature_id_data_access]->data     = UI_Data_Feature;

                                                                features[lv2_feature_id_instance_access]       = new LV2_Feature;
                                                                features[lv2_feature_id_instance_access]->URI  = LV2_INSTANCE_ACCESS_URI;
                                                                features[lv2_feature_id_instance_access]->data = handle;

                                                                features[lv2_feature_id_ui_parent]             = new LV2_Feature;
                                                                features[lv2_feature_id_ui_parent]->URI        = LV2_UI__parent;
                                                                features[lv2_feature_id_ui_parent]->data       = nullptr;

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
                                                                    update_ui_ports();

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

                                    return true;
                                }
                                else
                                    set_last_error("Failed to register plugin in JACK");
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

    // ----------------- URI-Map Feature -------------------------------------------------
    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

        if (map && strcmp(map, LV2_EVENT_URI) == 0)
        {
            // Event types
            if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
                return CARLA_URI_MAP_ID_EVENT_MIDI;
            else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
                return CARLA_URI_MAP_ID_EVENT_TIME;
        }
        else if (strcmp(uri, LV2_ATOM__String) == 0)
        {
            return CARLA_URI_MAP_ID_ATOM_STRING;
        }

        // Custom types
        if (data)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)data;
            return plugin->get_custom_uri_id(uri);
        }

        return 0;
    }

    // ----------------- URID Feature ----------------------------------------------------
    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_urid_map(%p, %s)", handle, uri);

        if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
            return CARLA_URI_MAP_ID_EVENT_MIDI;
        else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
            return CARLA_URI_MAP_ID_EVENT_TIME;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_id(uri);
        }

        return 0;
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_urid_unmap(%p, %i)", handle, urid);

        if (urid == CARLA_URI_MAP_ID_EVENT_MIDI)
            return "http://lv2plug.in/ns/ext/midi#MidiEvent";
        else if (urid == CARLA_URI_MAP_ID_EVENT_TIME)
            return "http://lv2plug.in/ns/ext/time#Position";
        else if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;

        // Custom types
        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            return plugin->get_custom_uri_string(urid);
        }

        return nullptr;
    }

    // ----------------- State Feature ---------------------------------------------------
    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);

        if (handle)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)handle;
            const char* uri_key = plugin->get_custom_uri_string(key);

            if (uri_key > 0 && (flags & LV2_STATE_IS_POD) > 0)
            {
                qDebug("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Got uri_key and flags", handle, key, value, size, type, flags);

                CustomDataType dtype;

                if (type == CARLA_URI_MAP_ID_ATOM_STRING)
                    dtype = CUSTOM_DATA_STRING;
                else if (type >= CARLA_URI_MAP_ID_COUNT)
                    dtype = CUSTOM_DATA_BINARY;
                else
                    dtype = CUSTOM_DATA_INVALID;

                if (value && dtype != CUSTOM_DATA_INVALID)
                {
                    // Check if we already have this key
                    for (int i=0; i < plugin->custom.count(); i++)
                    {
                        if (strcmp(plugin->custom[i].key, uri_key) == 0)
                        {
                            free((void*)plugin->custom[i].value);

                            if (dtype == CUSTOM_DATA_STRING)
                                plugin->custom[i].value = strdup((const char*)value);
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

                    if (dtype == CUSTOM_DATA_STRING)
                        new_data.value = strdup((const char*)value);
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
                    qCritical("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid type", handle, key, value, size, type, flags);
                    return LV2_STATE_ERR_BAD_TYPE;
                }
            }
            else
            {
                qWarning("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid attributes", handle, key, value, size, type, flags);
                return LV2_STATE_ERR_BAD_FLAGS;
            }
        }
        else
        {
            qCritical("Lv2AudioPlugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid handle", handle, key, value, size, type, flags);
            return LV2_STATE_ERR_UNKNOWN;
        }
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);

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
                        *type = CARLA_URI_MAP_ID_ATOM_STRING;
                        return string_data;
                    }
                    else if (dtype == CUSTOM_DATA_BINARY)
                    {
                        static QByteArray chunk;
                        chunk = QByteArray::fromBase64(string_data);

                        *size = chunk.size();
                        *type = key;
                        return chunk.constData();
                    }
                    else
                        qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key type", handle, key, size, type, flags);
                }
                else
                    qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key", handle, key, size, type, flags);
            }
            else
                qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Failed to find key", handle, key, size, type, flags);
        }
        else
            qCritical("Lv2AudioPlugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid handle", handle, key, size, type, flags);

        return nullptr;
    }

    // ----------------- External UI Feature ---------------------------------------------
    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_external_ui_closed(%p)", controller);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;
            plugin->gui.visible = false;

            if (plugin->ui.descriptor->cleanup)
                plugin->ui.descriptor->cleanup(plugin->ui.handle);

            plugin->ui.handle = nullptr;
            callback_action(CALLBACK_SHOW_GUI, plugin->id(), 0, 0, 0.0);
        }
    }

    // ----------------- UI Resize Feature -----------------------------------------------
    static int carla_lv2_ui_resize(LV2UI_Feature_Handle data, int width, int height)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_ui_resized(%p, %i, %i)", data, width, height);

        if (data)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)data;
            plugin->gui.width  = width;
            plugin->gui.height = height;
            callback_action(CALLBACK_RESIZE_GUI, plugin->id(), width, height, 0.0);
            return 0;
        }

        return 1;
    }

    // ----------------- UI Extension ----------------------------------------------------
    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("Lv2AudioPlugin::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller)
        {
            Lv2Plugin* plugin = (Lv2Plugin*)controller;

            if (format == 0 && buffer_size == sizeof(float))
            {
                int32_t param_id = -1;
                float value = *(float*)buffer;

                for (uint32_t i=0; i < plugin->param.count; i++)
                {
                    if (plugin->param.data[i].rindex == (int32_t)port_index)
                    {
                        param_id = i; //plugin->param.data[i].index;
                        break;
                    }
                }

                if (param_id > -1) // && plugin->ctrl.data[param_id].type == PARAMETER_INPUT
                    plugin->set_parameter_value(param_id, value, false, true, true);
            }
        }
    }

private:
    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    const LV2_RDF_Descriptor* rdf_descriptor;
    LV2_Feature* features[lv2_feature_count+1];

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

    uint32_t* ain_rindexes;
    uint32_t* aout_rindexes;
    Lv2ParameterData* lv2param;
    PluginEventData evin;
    PluginEventData evout;
    QList<const char*> custom_uri_ids;
};

short add_plugin_lv2(const char* filename, const char* label, void* extra_stuff)
{
    qDebug("add_plugin_lv2(%s, %s, %p)", filename, label, extra_stuff);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        Lv2Plugin* plugin = new Lv2Plugin;

        if (plugin->init(filename, label, extra_stuff))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
            //osc_new_plugin(plugin);
#endif
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

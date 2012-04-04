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
#include "lv2/event.h"
#include "lv2/event-helpers.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"
#include "lv2/ui.h"
#include "lv2_rdf.h"

// static max values
const unsigned int MAX_EVENT_BUFFER = 0x7FFF; // 32767

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x1000;
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x2000;

// feature ids
const uint32_t lv2_feature_id_uri_map         = 0;
const uint32_t lv2_feature_id_urid_map        = 1;
const uint32_t lv2_feature_id_urid_unmap      = 2;
const uint32_t lv2_feature_id_event           = 3;
//const uint32_t lv2_feature_id_rtmempool       = 4;
//const uint32_t lv2_feature_id_data_access     = 5;
//const uint32_t lv2_feature_id_instance_access = 6;
//const uint32_t lv2_feature_id_ui_resize       = 7;
//const uint32_t lv2_feature_id_ui_parent       = 8;
//const uint32_t lv2_feature_id_external_ui     = 9;
//const uint32_t lv2_feature_id_external_ui_old = 10;
const uint32_t lv2_feature_count              = 4;

// event types
const unsigned int CARLA_EVENT_TYPE_MIDI = 0x1;
const unsigned int CARLA_EVENT_TYPE_TIME = 0x2;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING = 1;
const uint32_t CARLA_URI_MAP_ID_EVENT_MIDI  = 2;
const uint32_t CARLA_URI_MAP_ID_EVENT_TIME  = 3;
const uint32_t CARLA_URI_MAP_ID_COUNT       = 4;

//enum CarlaLv2ParameterType {
//    LV2_PARAMETER_CONTROL
//};

struct EventData {
    unsigned int types;
    jack_port_t* port;
    LV2_Event_Buffer* buffer;
};

struct PluginEventData {
    uint32_t count;
    EventData* data;
};

struct Lv2ParameterData {
    //PluginLV2ParameterType type;
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

        // Fill pre-set URI keys
        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            custom_uri_ids.append(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;
    }

    virtual ~Lv2Plugin()
    {
        qDebug("Lv2Plugin::~Lv2Plugin()");

        if (handle && descriptor->deactivate && m_active_before)
            descriptor->deactivate(handle);

        if (handle && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

        if (features[lv2_feature_id_uri_map] && features[lv2_feature_id_uri_map]->data)
            delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;

        if (features[lv2_feature_id_urid_map] && features[lv2_feature_id_urid_map]->data)
            delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;

        if (features[lv2_feature_id_urid_unmap] && features[lv2_feature_id_urid_unmap]->data)
            delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;

        if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
            delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

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
        //switch (lv2param[param_id])
        return lv2param[param_id].control;
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
        info->type = GUI_NONE;
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        //switch (lv2param[param_id])
        lv2param[param_id].control = value;

//        if (gui_send && gui.visible)
//        {
//            switch(gui.type)
//            {
//            case GUI_INTERNAL_QT4:
//            case GUI_INTERNAL_X11:
//            case GUI_EXTERNAL_LV2:
//                if (ui.descriptor->port_event)
//                {
//                    float fvalue = value;
//                    ui.descriptor->port_event(ui.handle, param.data[parameter_id].rindex, sizeof(float), 0, &fvalue);
//                }
//                break;

//            case GUI_EXTERNAL_OSC:
//                osc_send_control(&osc.data, param.data[parameter_id].rindex, value);
//                break;

//            default:
//                break;
//            }
//        }

        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    virtual void set_custom_data(CustomDataType dtype, const char* key, const char* value, bool gui_send)
    {
        if ((m_hints & PLUGIN_HAS_EXTENSION_STATE) > 0 && descriptor->extension_data)
        {
            //LV2_State_Interface* state = (LV2_State_Interface*)descriptor->extension_data(LV2_STATE_INTERFACE_URI);

            //if (state)
            //    state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
        }

        CarlaPlugin::set_custom_data(dtype, key, value, gui_send);
    }

    virtual void set_gui_data(int, void* ptr)
    {
//        switch(gui.type)
//        {
//        case GUI_INTERNAL_QT4:
//            if (ui.widget)
//            {
//                QDialog* qtPtr  = (QDialog*)ptr;
//                QWidget* widget = (QWidget*)ui.widget;

//                qtPtr->layout()->addWidget(widget);
//                widget->setParent(qtPtr);
//                widget->show();
//            }
//            break;

//        case GUI_INTERNAL_X11:
//            if (ui.descriptor)
//            {
//                QDialog* qtPtr  = (QDialog*)ptr;
//                features[lv2_feature_id_ui_parent]->data = (void*)qtPtr->winId();

//                ui.handle = ui.descriptor->instantiate(ui.descriptor,
//                                                       descriptor->URI,
//                                                       ui.rdf_descriptor->Bundle,
//                                                       carla_lv2_ui_write_function,
//                                                       this,
//                                                       &ui.widget,
//                                                       features);

//                if (ui.handle && ui.descriptor->port_event)
//                    update_ui_ports();
//            }
//            break;

//        default:
//            break;
//        }
    }

    virtual void show_gui(bool yesno)
    {
//        switch(gui.type)
//        {
//        case GUI_INTERNAL_QT4:
//            gui.visible = yesno;
//            break;

//        case GUI_INTERNAL_X11:
//            gui.visible = yesno;

//            if (gui.visible && gui.width > 0 && gui.height > 0)
//                callback_action(CALLBACK_RESIZE_GUI, id, gui.width, gui.height, 0.0);

//            break;

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

//        case GUI_EXTERNAL_LV2:
//            if (!ui.handle)
//                reinit_external_ui();

//            if (ui.handle && ui.widget)
//            {
//                if (yesno)
//                {
//                    LV2_EXTERNAL_UI_SHOW((lv2_external_ui*)ui.widget);
//                    gui.visible = true;
//                }
//                else
//                {
//                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);
//                    gui.visible = false;

//                    if (ui.descriptor->cleanup)
//                        ui.descriptor->cleanup(ui.handle);

//                    ui.handle = nullptr;
//                }
//            }
//            else
//            {
//                // failed to init UI
//                gui.visible = false;
//                callback_action(CALLBACK_SHOW_GUI, id, -1, 0, 0.0);
//            }
//            break;

//        default:
//            break;
//        }
    }

    virtual void idle_gui()
    {
//        switch(gui.type)
//        {
//        case GUI_INTERNAL_QT4:
//        case GUI_INTERNAL_X11:
//        case GUI_EXTERNAL_LV2:
//            if (ui.descriptor->port_event)
//            {
//                for (uint32_t i=0; i < param.count; i++)
//                {
//                    if (param.data[i].type == PARAMETER_OUTPUT && (param.data[i].hints & PARAMETER_IS_AUTOMABLE) > 0)
//                        ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &param.buffers[i]);
//                }
//            }

//            if (gui.type == GUI_EXTERNAL_LV2)
//                LV2_EXTERNAL_UI_RUN((lv2_external_ui*)ui.widget);

//            break;

//        default:
//            break;
//        }
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
                evin.data[j].types  = 0;
                evin.data[j].port   = nullptr;
                evin.data[j].buffer = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
            }
        }

        if (ev_outs > 0)
        {
            evout.data = new EventData[ev_outs];

            for (j=0; j < ev_outs; j++)
            {
                evout.data[j].types  = 0;
                evout.data[j].port   = nullptr;
                evout.data[j].buffer = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
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
            const LV2_Property PortProps = rdf_descriptor->Ports[i].Properties;
            //const LV2_RDF_PortPoints PortPoints = rdf_descriptor->Ports[i].Points;

            if (LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_CV(PortType) || LV2_IS_PORT_EVENT(PortType))
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
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = evin.count++;
                    descriptor->connect_port(handle, i, evin.data[j].buffer);

                    if (LV2_IS_PORT_EVENT_MIDI(PortType))
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port   = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
                    }
                    if (LV2_IS_PORT_EVENT_TIME(PortType))
                    {
                        evin.data[j].types |= CARLA_EVENT_TYPE_TIME;
                        //wants_time_pos = true;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].buffer);

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
        }
    }

#if 0
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
                free(evin.data[i].buffer);

            delete[] evin.data;
        }

        if (evout.count > 0)
        {
            for (uint32_t i=0; i < evout.count; i++)
                free(evout.data[i].buffer);

            delete[] evout.data;
        }

        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        qDebug("Lv2Plugin::delete_buffers() - end");
    }

    bool init(const char* filename, const char* URI, void* extra_stuff)
    {
        LV2_RDF_Descriptor* rdf_descriptor_ = (LV2_RDF_Descriptor*)extra_stuff;

        if (rdf_descriptor_)
        {
            if (lib_open(rdf_descriptor_->Binary))
            {
                LV2_Descriptor_Function descfn = (LV2_Descriptor_Function)lib_symbol("lv2_descriptor");

                if (descfn)
                {
                    uint32_t i = 0;
                    while ((descriptor = descfn(i++)))
                    {
                        qDebug("%s | %s", descriptor->URI, URI);
                        if (strcmp(descriptor->URI, URI) == 0)
                            break;
                    }

                    if (descriptor)
                    {
                        // TODO - can continue

                        handle = descriptor->instantiate(descriptor, get_sample_rate(), rdf_descriptor_->Bundle, features);

                        if (handle)
                        {
                            m_filename = strdup(filename);
                            m_name = get_unique_name(rdf_descriptor_->Name);

                            rdf_descriptor = lv2_rdf_dup(rdf_descriptor_);

                            if (carla_jack_register_plugin(this, &jack_client))
                                return true;
                            else
                                set_last_error("Failed to register plugin in JACK");
                        }
                        else
                            set_last_error("Plugin failed to initialize");
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

private:
    LV2_Handle handle;
    const LV2_Descriptor* descriptor;
    const LV2_RDF_Descriptor* rdf_descriptor;
    LV2_Feature* features[lv2_feature_count+1];

    //    struct {
    //        void* lib;
    //        LV2UI_Handle handle;
    //        LV2UI_Widget widget;
    //        const LV2UI_Descriptor* descriptor;
    //        const LV2_RDF_UI* rdf_descriptor;
    //    } ui;

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

            //osc_new_plugin(plugin);
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

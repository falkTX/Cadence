/*
 * Carla UI bridge code
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

#include "carla_bridge_osc.h"
#include "carla_bridge_ui.h"
#include "carla_midi.h"

#include "lv2_rdf.h"

#ifdef BRIDGE_LV2_X11
// FIXME, use x11 something
#include <QtGui/QDialog>
#endif

UiData* ui = nullptr;

// -------------------------------------------------------------------------

// feature ids
const uint32_t lv2_feature_id_event           = 0;
const uint32_t lv2_feature_id_logs            = 1;
const uint32_t lv2_feature_id_programs        = 2;
const uint32_t lv2_feature_id_state_make_path = 3;
const uint32_t lv2_feature_id_state_map_path  = 4;
const uint32_t lv2_feature_id_uri_map         = 5;
const uint32_t lv2_feature_id_urid_map        = 6;
const uint32_t lv2_feature_id_urid_unmap      = 7;
const uint32_t lv2_feature_id_ui_parent       = 8;
const uint32_t lv2_feature_id_ui_port_map     = 9;
const uint32_t lv2_feature_id_ui_resize       = 10;
const uint32_t lv2_feature_count              = 11;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL          = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_CHUNK    = 1;
const uint32_t CARLA_URI_MAP_ID_ATOM_PATH     = 2;
const uint32_t CARLA_URI_MAP_ID_ATOM_SEQUENCE = 3;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING   = 4;
const uint32_t CARLA_URI_MAP_ID_LOG_ERROR     = 5;
const uint32_t CARLA_URI_MAP_ID_LOG_NOTE      = 6;
const uint32_t CARLA_URI_MAP_ID_LOG_TRACE     = 7;
const uint32_t CARLA_URI_MAP_ID_LOG_WARNING   = 8;
const uint32_t CARLA_URI_MAP_ID_MIDI_EVENT    = 9;
const uint32_t CARLA_URI_MAP_ID_COUNT         = 10;

// -------------------------------------------------------------------------

class Lv2UiData : public UiData
{
public:
    Lv2UiData(const char* ui_title) : UiData(ui_title)
    {
        handle = nullptr;
        widget = nullptr;
        descriptor = nullptr;

        rdf_descriptor = nullptr;
        rdf_ui_descriptor = nullptr;

        programs = nullptr;

#ifdef BRIDGE_LV2_X11
        x11_widget = new QDialog;
#endif

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

        LV2UI_Port_Map* UI_PortMap_Feature   = new LV2UI_Port_Map;
        UI_PortMap_Feature->handle           = this;
        UI_PortMap_Feature->port_index       = carla_lv2_ui_port_map;

        LV2UI_Resize* UI_Resize_Feature      = new LV2UI_Resize;
        UI_Resize_Feature->handle            = this;
        UI_Resize_Feature->ui_resize         = carla_lv2_ui_resize;

        features[lv2_feature_id_event]             = new LV2_Feature;
        features[lv2_feature_id_event]->URI        = LV2_EVENT_URI;
        features[lv2_feature_id_event]->data       = Event_Feature;

        features[lv2_feature_id_logs]              = new LV2_Feature;
        features[lv2_feature_id_logs]->URI         = LV2_LOG__log;
        features[lv2_feature_id_logs]->data        = Log_Feature;

        features[lv2_feature_id_programs]          = new LV2_Feature;
        features[lv2_feature_id_programs]->URI     = LV2_PROGRAMS__Host;
        features[lv2_feature_id_programs]->data    = Programs_Feature;

        features[lv2_feature_id_state_make_path]   = new LV2_Feature;
        features[lv2_feature_id_state_make_path]->URI  = LV2_STATE__makePath;
        features[lv2_feature_id_state_make_path]->data = State_MakePath_Feature;

        features[lv2_feature_id_state_map_path]    = new LV2_Feature;
        features[lv2_feature_id_state_map_path]->URI  = LV2_STATE__mapPath;
        features[lv2_feature_id_state_map_path]->data = State_MapPath_Feature;

        features[lv2_feature_id_uri_map]           = new LV2_Feature;
        features[lv2_feature_id_uri_map]->URI      = LV2_URI_MAP_URI;
        features[lv2_feature_id_uri_map]->data     = URI_Map_Feature;

        features[lv2_feature_id_urid_map]          = new LV2_Feature;
        features[lv2_feature_id_urid_map]->URI     = LV2_URID__map;
        features[lv2_feature_id_urid_map]->data    = URID_Map_Feature;

        features[lv2_feature_id_urid_unmap]        = new LV2_Feature;
        features[lv2_feature_id_urid_unmap]->URI   = LV2_URID__unmap;
        features[lv2_feature_id_urid_unmap]->data  = URID_Unmap_Feature;

        features[lv2_feature_id_ui_parent]         = new LV2_Feature;
        features[lv2_feature_id_ui_parent]->URI    = LV2_UI__parent;
#ifdef BRIDGE_LV2_X11
        features[lv2_feature_id_ui_parent]->data   = (void*)x11_widget->winId();
#else
        features[lv2_feature_id_ui_parent]->data   = nullptr;
#endif

        features[lv2_feature_id_ui_port_map]       = new LV2_Feature;
        features[lv2_feature_id_ui_port_map]->URI  = LV2_UI__portMap;
        features[lv2_feature_id_ui_port_map]->data = UI_PortMap_Feature;

        features[lv2_feature_id_ui_resize]         = new LV2_Feature;
        features[lv2_feature_id_ui_resize]->URI    = LV2_UI__resize;
        features[lv2_feature_id_ui_resize]->data   = UI_Resize_Feature;
    }

    ~Lv2UiData()
    {
        delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;
        delete (LV2_Log_Log*)features[lv2_feature_id_logs]->data;
        delete (LV2_Programs_Host*)features[lv2_feature_id_programs]->data;
        delete (LV2_State_Make_Path*)features[lv2_feature_id_state_make_path]->data;
        delete (LV2_State_Map_Path*)features[lv2_feature_id_state_map_path]->data;
        delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;
        delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
        delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;
        delete (LV2UI_Port_Map*)features[lv2_feature_id_ui_port_map]->data;
        delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

        for (uint32_t i=0; i < lv2_feature_count; i++)
        {
            if (features[i])
                delete features[i];
        }

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

#ifdef WANT_X11
        //delete x11_widget;
#endif
    }

    // ---------------------------------------------------------------------
    // initialization

    bool init(const char* plugin_uri, const char* ui_uri)
    {
        Lv2World.init();
        rdf_descriptor = lv2_rdf_new(plugin_uri);

        if (rdf_descriptor)
        {
            for (uint32_t i=0; i < rdf_descriptor->UICount; i++)
            {
                if (strcmp(rdf_descriptor->UIs[i].URI, ui_uri) == 0)
                {
                    rdf_ui_descriptor = &rdf_descriptor->UIs[i];
                    break;
                }
            }

            if (rdf_ui_descriptor && lib_open(rdf_ui_descriptor->Binary))
            {
                LV2UI_DescriptorFunction ui_descfn = (LV2UI_DescriptorFunction)lib_symbol("lv2ui_descriptor");

                if (ui_descfn)
                {
                    uint32_t i = 0;
                    while ((descriptor = ui_descfn(i++)))
                    {
                        if (strcmp(descriptor->URI, ui_uri) == 0)
                            break;
                    }

                    if (descriptor)
                    {
                        handle = descriptor->instantiate(descriptor,
                                                         plugin_uri,
                                                         rdf_ui_descriptor->Bundle,
                                                         carla_lv2_ui_write_function,
                                                         this,
                                                         &widget,
                                                         features);

                        if (handle && descriptor->extension_data)
                        {
                            for (uint32_t j=0; j < rdf_ui_descriptor->ExtensionCount; j++)
                            {
                                if (strcmp(rdf_ui_descriptor->Extensions[j], LV2_PROGRAMS__UIInterface) == 0)
                                {
                                    programs = (LV2_Programs_UI_Interface*)descriptor->extension_data(LV2_PROGRAMS__UIInterface);
                                    break;
                                }
                            }
                        }

                        return bool(handle);
                    }
                }
            }
        }

        return false;
    }

    void close()
    {
        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);

        lib_close();
    }

    // ---------------------------------------------------------------------
    // processing

    void set_parameter(int index, double value)
    {
        if (descriptor && descriptor->port_event)
        {
            float fvalue = value;
            descriptor->port_event(handle, index, sizeof(float), 0, &fvalue);
        }
    }

    void set_program(int) {}

    void set_midi_program(int bank, int program)
    {
        if (programs)
            programs->select_program(handle, bank, program);
    }

    void send_note_on(int, int) {}
    void send_note_off(int) {}

    // ---------------------------------------------------------------------
    // gui

    bool has_parent() const
    {
#ifdef BRIDGE_LV2_X11
        return false;
#else
        return true;
#endif
    }

    bool is_resizable() const
    {
        if (rdf_ui_descriptor)
        {
            for (uint32_t i=0; i < rdf_ui_descriptor->FeatureCount; i++)
            {
                if (strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
                    return false;
            }
            return true;
        }
        return false;
    }

    void* get_widget() const
    {
#ifdef BRIDGE_LV2_X11
        return x11_widget;
#else
        return widget;
#endif
    }

    // ----------------- Event Feature ---------------------------------------------------
    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("carla_lv2_event_ref(%p, %p)", callback_data, event);
        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("carla_lv2_event_unref(%p, %p)", callback_data, event);
        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------
    static int carla_lv2_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
    {
        qDebug("carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);

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
        qDebug("carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);

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

        osc_send_configure(nullptr, "reloadprograms", "");
        // QString::number(index).toUtf8().constData()
    }

    // ----------------- State Feature ---------------------------------------------------
    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        qDebug("carla_lv2_state_make_path(%p, %p)", handle, path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        qDebug("carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        return strdup(absolute_path);
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        qDebug("carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
        return strdup(abstract_path);
    }

    // ----------------- URI-Map Feature ---------------------------------------
    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        else if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;

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

        return CARLA_URI_MAP_ID_NULL;
    }

    // ----------------- URID Feature ------------------------------------------
    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("carla_lv2_urid_map(%p, %s)", handle, uri);

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        else if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        else if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        else if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;

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

        return CARLA_URI_MAP_ID_NULL;
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("carla_lv2_urid_unmap(%p, %i)", handle, urid);

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        else if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        else if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        else if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;

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

        return nullptr;
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------
    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_port_map(%p, %s)", handle, symbol);

        if (handle)
        {
            Lv2UiData* lv2ui = (Lv2UiData*)handle;

            for (uint32_t i=0; i < lv2ui->rdf_descriptor->PortCount; i++)
            {
                if (strcmp(lv2ui->rdf_descriptor->Ports[i].Symbol, symbol) == 0)
                    return i;
            }
        }

        return 0;
    }

    // ----------------- UI Resize Feature -------------------------------------
    static int carla_lv2_ui_resize(LV2UI_Feature_Handle data, int width, int height)
    {
        qDebug("carla_lv2_ui_resized(%p, %i, %i)", data, width, height);

        if (data)
        {
            Lv2UiData* lv2ui = (Lv2UiData*)data;
            lv2ui->queque_message(BRIDGE_MESSAGE_RESIZE_GUI, width, height, 0.0);
            return 0;
        }

        return 1;
    }

    // ----------------- UI Extension ------------------------------------------
    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);

        if (controller)
        {
            if (format == 0)
            {
                if (buffer_size == sizeof(float))
                {
                    float value = *(float*)buffer;
                    osc_send_control(nullptr, port_index, value);
                }
            }
            else if (format == CARLA_URI_MAP_ID_MIDI_EVENT)
            {
                const uint8_t* data = (const uint8_t*)buffer;
                uint8_t status = data[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && data[2] == 0)
                    status -= 0x10;

                uint8_t midi_buf[4] = { 0, status, data[2], data[3] };
                osc_send_midi(nullptr, midi_buf);
            }
        }
    }

private:
    LV2UI_Handle handle;
    LV2UI_Widget widget;
    const LV2UI_Descriptor* descriptor;
    LV2_Feature* features[lv2_feature_count+1];

    const LV2_RDF_Descriptor* rdf_descriptor;
    const LV2_RDF_UI* rdf_ui_descriptor;

    const LV2_Programs_UI_Interface* programs;

#ifdef BRIDGE_LV2_X11
    QDialog* x11_widget;
#endif
};

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
       qCritical("%s: bad arguments", argv[0]);
       return 1;
    }

    const char* osc_url    = argv[1];
    const char* plugin_uri = argv[2];
    const char* ui_uri     = argv[3];
    const char* ui_title   = argv[4];

    // Init toolkit
    toolkit_init();

    // Init LV2-UI
    ui = new Lv2UiData(ui_title);

    // Init OSC
    osc_init(osc_url);

    // Load UI
    int ret;

    if (ui->init(plugin_uri, ui_uri))
    {
        toolkit_loop();
        ret = 0;
    }
    else
    {
        qCritical("Failed to load LV2 UI");
        ret = 1;
    }

    // Close OSC
    osc_send_exiting(nullptr);
    osc_close();

    // Close LV2-UI
    ui->close();

    // Close toolkit
    if (! ret)
        toolkit_quit();

    delete ui;
    ui = nullptr;

    return ret;
}

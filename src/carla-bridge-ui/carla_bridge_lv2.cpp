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

#include "carla_bridge_ui.h"
#include "../carla-bridge/carla_osc.h"

#include "lv2/lv2.h"
#include "lv2/atom.h"
#include "lv2/event.h"
#include "lv2/uri-map.h"
#include "lv2/urid.h"
#include "lv2/ui.h"

#include <cstring>

UiData* ui = nullptr;

// -------------------------------------------------------------------------

// feature ids
const uint32_t lv2_feature_id_uri_map         = 0;
const uint32_t lv2_feature_id_urid_map        = 1;
const uint32_t lv2_feature_id_urid_unmap      = 2;
const uint32_t lv2_feature_id_event           = 3;
const uint32_t lv2_feature_id_ui_parent       = 4;
const uint32_t lv2_feature_id_ui_resize       = 5;
const uint32_t lv2_feature_count              = 6;

// pre-set uri[d] map ids
const uint32_t CARLA_URI_MAP_ID_NULL        = 0;
const uint32_t CARLA_URI_MAP_ID_ATOM_STRING = 1;
const uint32_t CARLA_URI_MAP_ID_EVENT_MIDI  = 2;
const uint32_t CARLA_URI_MAP_ID_EVENT_TIME  = 3;
const uint32_t CARLA_URI_MAP_ID_COUNT       = 4;

// ----------------- URI-Map Feature ---------------------------------------
static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
{
    qDebug("carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);

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

    return CARLA_URI_MAP_ID_NULL;
}

// ----------------- URID Feature ------------------------------------------
static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
    qDebug("carla_lv2_urid_map(%p, %s)", handle, uri);

    if (strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent") == 0)
        return CARLA_URI_MAP_ID_EVENT_MIDI;
    else if (strcmp(uri, "http://lv2plug.in/ns/ext/time#Position") == 0)
        return CARLA_URI_MAP_ID_EVENT_TIME;
    else if (strcmp(uri, LV2_ATOM__String) == 0)
        return CARLA_URI_MAP_ID_ATOM_STRING;

    return CARLA_URI_MAP_ID_NULL;
}

static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
    qDebug("carla_lv2_urid_unmap(%p, %i)", handle, urid);

    if (urid == CARLA_URI_MAP_ID_EVENT_MIDI)
        return "http://lv2plug.in/ns/ext/midi#MidiEvent";
    else if (urid == CARLA_URI_MAP_ID_EVENT_TIME)
        return "http://lv2plug.in/ns/ext/time#Position";
    else if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
        return LV2_ATOM__String;

    return nullptr;
}

// ----------------- UI Resize Feature -------------------------------------
static int carla_lv2_ui_resize(LV2UI_Feature_Handle data, int width, int height)
{
    qDebug("carla_lv2_ui_resized(%p, %i, %i)", data, width, height);

    if (data)
    {
        UiData* ui = (UiData*)data;
        ui->queque_message(BRIDGE_MESSAGE_RESIZE_GUI, width, height, 0.0);
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
        //UiData* ui = (UiData*)controller;

        if (format == 0 && buffer_size == sizeof(float))
        {
            float value = *(float*)buffer;
            osc_send_control(nullptr, port_index, value);
        }
    }
}

// -------------------------------------------------------------------------

class Lv2UiData : public UiData
{
public:
    Lv2UiData() : UiData()
    {
        handle = nullptr;
        widget = nullptr;
        descriptor = nullptr;

#ifdef BRIDGE_LV2_X11
        x11_widget = new QDialog();
#endif

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

        LV2UI_Resize* UI_Resize_Feature      = new LV2UI_Resize;
        UI_Resize_Feature->handle            = this;
        UI_Resize_Feature->ui_resize         = carla_lv2_ui_resize;

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

        features[lv2_feature_id_ui_parent]        = new LV2_Feature;
        features[lv2_feature_id_ui_parent]->URI   = LV2_UI__parent;
        features[lv2_feature_id_ui_parent]->data  = nullptr;

        features[lv2_feature_id_ui_resize]        = new LV2_Feature;
        features[lv2_feature_id_ui_resize]->URI   = LV2_UI__resize;
        features[lv2_feature_id_ui_resize]->data  = UI_Resize_Feature;

        features[lv2_feature_count] = nullptr;

        resizable = true;
    }

    ~Lv2UiData()
    {
        delete (LV2_URI_Map_Feature*)features[lv2_feature_id_uri_map]->data;
        delete (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
        delete (LV2_URID_Unmap*)features[lv2_feature_id_urid_unmap]->data;
        delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;
        delete (LV2UI_Resize*)features[lv2_feature_id_ui_resize]->data;

        for (uint32_t i=0; i < lv2_feature_count; i++)
        {
            if (features[i])
                delete features[i];
        }
#ifdef WANT_X11
        //delete x11_widget;
#endif
    }

    virtual bool init(const char* plugin_uri, const char* ui_uri, const char* ui_bundle, bool resizable_)
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
                                                 ui_bundle,
                                                 carla_lv2_ui_write_function,
                                                 this,
                                                 &widget,
                                                 features);

                resizable = resizable_;

                return bool(handle);
            }
        }

        return false;
    }

    virtual void close()
    {
        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);
    }

    virtual void update_parameter(int index, double value)
    {
        if (descriptor && descriptor->port_event)
        {
            float fvalue = value;
            descriptor->port_event(handle, index, sizeof(float), 0, &fvalue);
        }
    }

    virtual void update_program(int) {}
    virtual void update_midi_program(int, int) {}
    virtual void send_note_on(int, int) {}
    virtual void send_note_off(int) {}

    virtual void* get_widget()
    {
#ifdef BRIDGE_LV2_X11
        return x11_widget;
#else
        return widget;
#endif
    }

    virtual bool is_resizable()
    {
        return resizable;
    }

private:
    LV2UI_Handle handle;
    LV2UI_Widget widget;
    const LV2UI_Descriptor* descriptor;
    LV2_Feature* features[lv2_feature_count+1];
    bool resizable;

#ifdef BRIDGE_LV2_X11
    LV2UI_Widget x11_widget;
#endif
};

int main(int argc, char* argv[])
{
    if (argc != 7)
    {
       qCritical("%s: bad arguments", argv[0]);
       return 1;
    }

    const char* osc_url     = argv[1];
    const char* plugin_uri  = argv[2];
    const char* ui_uri      = argv[3];
    const char* ui_binary   = argv[4];
    const char* ui_bundle   = argv[5];
    const char* ui_title    = argv[6];

    // Init LV2
    ui = new Lv2UiData;

    // Init toolkit
    toolkit_init();

    // Init OSC
    osc_init("lv2-ui-bridge", osc_url);

    if (ui->lib_open(ui_binary))
    {
        if (ui->init(plugin_uri, ui_uri, ui_bundle, false) == false) // (strcmp(resizable, "true") == 0)
        {
            qCritical("Failed to load LV2 UI");
            ui->lib_close();
            osc_close();
            return 1;
        }
    }
    else
    {
        qCritical("Failed to load UI, error was:\n%s", ui->lib_error());
        osc_close();
        return 1;
    }

#ifdef BRIDGE_LV2_X11
    bool reparent = false;
#else
    bool reparent = true;
#endif

    toolkit_loop(ui_title, reparent);

    ui->close();
    ui->lib_close();

    osc_send_exiting();
    osc_close();

    toolkit_quit();

    delete ui;
    ui = nullptr;

    return 0;
}

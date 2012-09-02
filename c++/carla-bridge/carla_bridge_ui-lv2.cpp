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

#ifdef BRIDGE_LV2

#include "carla_bridge_client.h"
#include "carla_lv2.h"
#include "carla_midi.h"

#include <vector>
#include <QtCore/QDir>

#ifdef BRIDGE_LV2_X11
#  include <QtGui/QDialog>
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

// feature ids
const uint32_t lv2_feature_id_event           = 0;
const uint32_t lv2_feature_id_logs            = 1;
const uint32_t lv2_feature_id_programs        = 2;
const uint32_t lv2_feature_id_state_make_path = 3;
const uint32_t lv2_feature_id_state_map_path  = 4;
const uint32_t lv2_feature_id_strict_bounds   = 5;
const uint32_t lv2_feature_id_uri_map         = 6;
const uint32_t lv2_feature_id_urid_map        = 7;
const uint32_t lv2_feature_id_urid_unmap      = 8;
const uint32_t lv2_feature_id_ui_parent       = 9;
const uint32_t lv2_feature_id_ui_port_map     = 10;
const uint32_t lv2_feature_id_ui_resize       = 11;
const uint32_t lv2_feature_count              = 12;

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

// -------------------------------------------------------------------------

class CarlaLv2Client : public CarlaClient
{
public:
    CarlaLv2Client(CarlaToolkit* const toolkit)
        : CarlaClient(toolkit)
    {
        handle = nullptr;
        widget = nullptr;
        descriptor = nullptr;

        rdf_descriptor = nullptr;
        rdf_ui_descriptor = nullptr;

        programs = nullptr;

#ifdef BRIDGE_LV2_X11
        m_resizable = false;
        x11_widget = new QDialog;
#else
        m_resizable = true;
#endif

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            customURIDs.push_back(nullptr);

        for (uint32_t i=0; i < lv2_feature_count+1; i++)
            features[i] = nullptr;

        // -----------------------------------------------------------------
        // initialize features

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

        features[lv2_feature_id_strict_bounds]     = new LV2_Feature;
        features[lv2_feature_id_strict_bounds]->URI  = LV2_PORT_PROPS__supportsStrictBounds;
        features[lv2_feature_id_strict_bounds]->data = nullptr;

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

    ~CarlaLv2Client()
    {
        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

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

        for (size_t i=0; i < customURIDs.size(); i++)
        {
            if (customURIDs[i])
                free((void*)customURIDs[i]);
        }

        customURIDs.clear();
    }

    // ---------------------------------------------------------------------
    // ui initialization

    bool init(const char* plugin_uri, const char* ui_uri)
    {
        // -----------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        Lv2World.init();
        rdf_descriptor = lv2_rdf_new(plugin_uri);

        if (! rdf_descriptor)
            return false;

        // -----------------------------------------------------------------
        // find requested UI

        for (uint32_t i=0; i < rdf_descriptor->UICount; i++)
        {
            if (strcmp(rdf_descriptor->UIs[i].URI, ui_uri) == 0)
            {
                rdf_ui_descriptor = &rdf_descriptor->UIs[i];
                break;
            }
        }

        if (! rdf_ui_descriptor)
            return false;

        // -----------------------------------------------------------------
        // open DLL

        if (! libOpen(rdf_ui_descriptor->Binary))
            return false;

        // -----------------------------------------------------------------
        // get DLL main entry

        LV2UI_DescriptorFunction const ui_descfn = (LV2UI_DescriptorFunction)libSymbol("lv2ui_descriptor");

        if (! ui_descfn)
            return false;

        // -----------------------------------------------------------
        // get descriptor that matches URI

        uint32_t i = 0;
        while ((descriptor = ui_descfn(i++)))
        {
            if (strcmp(descriptor->URI, ui_uri) == 0)
                break;
        }

        if (! descriptor)
            return false;

        // -----------------------------------------------------------
        // initialize UI

        handle = descriptor->instantiate(descriptor, plugin_uri, rdf_ui_descriptor->Bundle, carla_lv2_ui_write_function, this, &widget, features);

        if (! handle)
            return false;

        // -----------------------------------------------------------
        // check if not resizable

#ifndef BRIDGE_LV2_X11
        for (uint32_t i=0; i < rdf_ui_descriptor->FeatureCount; i++)
        {
            if (strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__fixedSize) == 0 || strcmp(rdf_ui_descriptor->Features[i].URI, LV2_UI__noUserResize) == 0)
            {
                m_resizable = false;
                break;
            }
        }
#endif

        // -----------------------------------------------------------
        // check for known extensions

        for (uint32_t i=0; descriptor->extension_data && i < rdf_ui_descriptor->ExtensionCount; i++)
        {
            if (strcmp(rdf_ui_descriptor->Extensions[i], LV2_PROGRAMS__UIInterface) == 0)
            {
                programs = (LV2_Programs_UI_Interface*)descriptor->extension_data(LV2_PROGRAMS__UIInterface);
                break;
            }
        }

        return true;
    }

    void close()
    {
        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);

        libClose();
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(int32_t rindex, double value)
    {
        Q_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            float fvalue = value;
            descriptor->port_event(handle, rindex, sizeof(float), 0, &fvalue);
        }
    }

    void setProgram(uint32_t)
    {
    }

    void setMidiProgram(uint32_t bank, uint32_t program)
    {
        Q_ASSERT(handle);

        if (handle && programs)
            programs->select_program(handle, bank, program);
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        Q_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            uint8_t buf[3] = { uint8_t(0x90 + channel), note, velo };
            descriptor->port_event(handle, 0, 3, CARLA_URI_MAP_ID_MIDI_EVENT, buf);
        }
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        Q_ASSERT(handle && descriptor);

        if (handle && descriptor && descriptor->port_event)
        {
            uint8_t buf[3] = { uint8_t(0x80 + channel), note, 0 };
            descriptor->port_event(handle, 0, 3, CARLA_URI_MAP_ID_MIDI_EVENT, buf);
        }
    }

    // ---------------------------------------------------------------------
    // gui

    void* getWidget() const
    {
#ifdef BRIDGE_LV2_X11
        return x11_widget;
#else
        return widget;
#endif
    }

    bool isResizable() const
    {
        return m_resizable;
    }

    bool needsReparent() const
    {
#ifdef BRIDGE_LV2_X11
        return true;
#else
        return false;
#endif
    }

    // ---------------------------------------------------------------------

    uint32_t getCustomURID(const char* const uri)
    {
        qDebug("CarlaLv2Client::getCustomURID(%s)", uri);
        Q_ASSERT(uri);

        for (size_t i=0; i < customURIDs.size(); i++)
        {
            if (customURIDs[i] && strcmp(customURIDs[i], uri) == 0)
                return i;
        }

        customURIDs.push_back(strdup(uri));
        return customURIDs.size()-1;
    }

    const char* getCustomURIString(LV2_URID urid)
    {
        qDebug("CarlaLv2Client::getCustomURIString(%i)", urid);
        Q_ASSERT(urid != 0);

        if (urid < customURIDs.size())
            return customURIDs[urid];

        return nullptr;
    }

    void handleTransferAtom(const char* const type, const char* const value)
    {
        qDebug("CarlaLv2Client::handleTransferEvent(%s, %s)", type, value);
        Q_ASSERT(type);
        Q_ASSERT(value);
    }

    void handleTransferEvent(const char* const type, const char* const value)
    {
        qDebug("CarlaLv2Client::handleTransferEvent(%s, %s)", type, value);
        Q_ASSERT(type);
        Q_ASSERT(value);

        if (handle && descriptor && descriptor->port_event)
        {
            LV2_URID_Map* const URID_Map = (LV2_URID_Map*)features[lv2_feature_id_urid_map]->data;
            const LV2_URID uridPatchSet  = getCustomURID(LV2_PATCH__Set);
            const LV2_URID uridPatchBody = getCustomURID(LV2_PATCH__body);

            Sratom*   sratom = sratom_new(URID_Map);
            SerdChunk chunk  = { nullptr, 0 };

            LV2_Atom_Forge forge;
            lv2_atom_forge_init(&forge, URID_Map);
            lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &chunk);

            LV2_Atom_Forge_Frame refFrame, bodyFrame;
            LV2_Atom_Forge_Ref   ref = lv2_atom_forge_blank(&forge, &refFrame, 1, uridPatchSet);

            lv2_atom_forge_property_head(&forge, uridPatchBody, CARLA_URI_MAP_ID_NULL);
            lv2_atom_forge_blank(&forge, &bodyFrame, 2, CARLA_URI_MAP_ID_NULL);

            //lv2_atom_forge_property_head(&forge, getCustomURID(key), CARLA_URI_MAP_ID_NULL);

            if (strcmp(type, "string") == 0)
                lv2_atom_forge_string(&forge, value, strlen(value));
            else if (strcmp(type, "path") == 0)
                lv2_atom_forge_path(&forge, value, strlen(value));
            else if (strcmp(type, "chunk") == 0)
                lv2_atom_forge_literal(&forge, value, strlen(value), CARLA_URI_MAP_ID_ATOM_CHUNK, CARLA_URI_MAP_ID_NULL);
            //else
            //    lv2_atom_forge_literal(&forge, value, strlen(value), getCustomURID(key), CARLA_URI_MAP_ID_NULL);

            lv2_atom_forge_pop(&forge, &bodyFrame);
            lv2_atom_forge_pop(&forge, &refFrame);

            const LV2_Atom* const atom = lv2_atom_forge_deref(&forge, ref);
            descriptor->port_event(handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

            free((void*)chunk.buf);
            sratom_free(sratom);
        }
    }

    void handleProgramChanged(int32_t /*index*/)
    {
        sendOscConfigure("reloadprograms", "");
    }

    uint32_t handleUiPortMap(const char* const symbol)
    {
        Q_ASSERT(symbol);

        for (uint32_t i=0; i < rdf_descriptor->PortCount; i++)
        {
            if (strcmp(rdf_descriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }


    int handleUiResize(int width, int height)
    {
        Q_ASSERT(width > 0);
        Q_ASSERT(height > 0);

        quequeMessage(MESSAGE_RESIZE_GUI, width, height, 0.0);

        return 0;
    }


    void handleUiWrite(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (format == 0)
        {
            Q_ASSERT(bufferSize == sizeof(float));

            if (bufferSize == sizeof(float))
            {
                float value = *(float*)buffer;
                sendOscControl(portIndex, value);
            }
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
        {
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            QByteArray chunk((const char*)buffer, bufferSize);
            sendOscLv2TransferAtom(getCustomURIString(atom->type), chunk.toBase64().constData());

            if (descriptor && descriptor->port_event)
                descriptor->port_event(handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM, atom);
        }
        else if (format == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
        {
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            QByteArray chunk((const char*)buffer, bufferSize);
            sendOscLv2TransferEvent(getCustomURIString(atom->type), chunk.toBase64().constData());

            if (descriptor && descriptor->port_event)
                descriptor->port_event(handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

        }
    }

    // ----------------- Event Feature ---------------------------------------------------

    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("CarlaLv2Client::carla_lv2_event_ref(%p, %p)", callback_data, event);
        Q_ASSERT(callback_data);
        Q_ASSERT(event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("CarlaLv2Client::carla_lv2_event_unref(%p, %p)", callback_data, event);
        Q_ASSERT(callback_data);
        Q_ASSERT(event);

        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------

    static int carla_lv2_log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
    {
        qDebug("CarlaLv2Client::carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);
        Q_ASSERT(handle);
        Q_ASSERT(type > 0);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        va_list args;
        va_start(args, fmt);
        const int ret = carla_lv2_log_vprintf(handle, type, fmt, args);
        va_end(args);

        return ret;
    }

    static int carla_lv2_log_vprintf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, va_list ap)
    {
        qDebug("CarlaLv2Client::carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);
        Q_ASSERT(handle);
        Q_ASSERT(type > 0);

#ifndef DEBUG
        if (type == CARLA_URI_MAP_ID_LOG_TRACE)
            return 0;
#endif

        char buf[8196];
        vsprintf(buf, fmt, ap);

        if (*buf == 0)
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

    // ----------------- Programs Feature ------------------------------------------------

    static void carla_lv2_program_changed(LV2_Programs_Handle handle, int32_t index)
    {
        qDebug("CarlaLv2Client::carla_lv2_program_changed(%p, %i)", handle, index);
        Q_ASSERT(handle);

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        client->handleProgramChanged(index);
    }

    // ----------------- State Feature ---------------------------------------------------

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        qDebug("CarlaLv2Client::carla_lv2_state_make_path(%p, %p)", handle, path);
        Q_ASSERT(handle);
        Q_ASSERT(path);

        QDir dir;
        dir.mkpath(path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        qDebug("CarlaLv2Client::carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        Q_ASSERT(handle);
        Q_ASSERT(absolute_path);

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        qDebug("CarlaLv2Client::carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
        Q_ASSERT(handle);
        Q_ASSERT(abstract_path);

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
    }

    // ----------------- URI-Map Feature ---------------------------------------

    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("CarlaLv2Client::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);
        return carla_lv2_urid_map(data, uri);
    }

    // ----------------- URID Feature ------------------------------------------

    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("CarlaLv2Client::carla_lv2_urid_map(%p, %s)", handle, uri);
        Q_ASSERT(handle);
        Q_ASSERT(uri);

        // Atom types
        if (strcmp(uri, LV2_ATOM__Chunk) == 0)
            return CARLA_URI_MAP_ID_ATOM_CHUNK;
        if (strcmp(uri, LV2_ATOM__Path) == 0)
            return CARLA_URI_MAP_ID_ATOM_PATH;
        if (strcmp(uri, LV2_ATOM__Sequence) == 0)
            return CARLA_URI_MAP_ID_ATOM_SEQUENCE;
        if (strcmp(uri, LV2_ATOM__String) == 0)
            return CARLA_URI_MAP_ID_ATOM_STRING;
        if (strcmp(uri, LV2_ATOM__atomTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM;
        if (strcmp(uri, LV2_ATOM__eventTransfer) == 0)
            return CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT;

        // Log types
        if (strcmp(uri, LV2_LOG__Error) == 0)
            return CARLA_URI_MAP_ID_LOG_ERROR;
        if (strcmp(uri, LV2_LOG__Note) == 0)
            return CARLA_URI_MAP_ID_LOG_NOTE;
        if (strcmp(uri, LV2_LOG__Trace) == 0)
            return CARLA_URI_MAP_ID_LOG_TRACE;
        if (strcmp(uri, LV2_LOG__Warning) == 0)
            return CARLA_URI_MAP_ID_LOG_WARNING;

        // Others
        if (strcmp(uri, LV2_MIDI__MidiEvent) == 0)
            return CARLA_URI_MAP_ID_MIDI_EVENT;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("CarlaLv2Client::carla_lv2_urid_unmap(%p, %i)", handle, urid);
        Q_ASSERT(handle);
        Q_ASSERT(urid > 0);

        // Atom types
        if (urid == CARLA_URI_MAP_ID_ATOM_CHUNK)
            return LV2_ATOM__Chunk;
        if (urid == CARLA_URI_MAP_ID_ATOM_PATH)
            return LV2_ATOM__Path;
        if (urid == CARLA_URI_MAP_ID_ATOM_SEQUENCE)
            return LV2_ATOM__Sequence;
        if (urid == CARLA_URI_MAP_ID_ATOM_STRING)
            return LV2_ATOM__String;
        if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_ATOM)
            return LV2_ATOM__atomTransfer;
        if (urid == CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT)
            return LV2_ATOM__eventTransfer;

        // Log types
        if (urid == CARLA_URI_MAP_ID_LOG_ERROR)
            return LV2_LOG__Error;
        if (urid == CARLA_URI_MAP_ID_LOG_NOTE)
            return LV2_LOG__Note;
        if (urid == CARLA_URI_MAP_ID_LOG_TRACE)
            return LV2_LOG__Trace;
        if (urid == CARLA_URI_MAP_ID_LOG_WARNING)
            return LV2_LOG__Warning;

        // Others
        if (urid == CARLA_URI_MAP_ID_MIDI_EVENT)
            return LV2_MIDI__MidiEvent;

        // Custom types
        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->getCustomURIString(urid);
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_port_map(%p, %s)", handle, symbol);
        Q_ASSERT(handle);

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiPortMap(symbol);
    }


    // ----------------- UI Resize Feature -------------------------------------

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        Q_ASSERT(handle);

        CarlaLv2Client* const client = (CarlaLv2Client*)handle;
        return client->handleUiResize(width, height);
    }

    // ----------------- UI Extension ------------------------------------------

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("CarlaLv2Client::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
        Q_ASSERT(controller);

        CarlaLv2Client* const client = (CarlaLv2Client*)controller;
        client->handleUiWrite(port_index, buffer_size, format, buffer);
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

    bool m_resizable;
    std::vector<const char*> customURIDs;
};

int CarlaOsc::handleMsgLv2TransferAtom(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaOsc::handle_lv2_atom_transfer()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ss");

    if (! client)
        return 1;

    const char* type  = (const char*)&argv[0]->s;
    const char* value = (const char*)&argv[2]->s;

    CarlaLv2Client* const lv2client = (CarlaLv2Client*)client;
    lv2client->handleTransferAtom(type, value);

    return 0;
}

int CarlaOsc::handleMsgLv2TransferEvent(CARLA_BRIDGE_OSC_HANDLE_ARGS)
{
    qDebug("CarlaOsc::handle_lv2_event_transfer()");
    CARLA_BRIDGE_OSC_CHECK_OSC_TYPES(2, "ss");

    if (! client)
        return 1;

    const char* type  = (const char*)&argv[0]->s;
    const char* value = (const char*)&argv[2]->s;

    CarlaLv2Client* const lv2client = (CarlaLv2Client*)client;
    lv2client->handleTransferEvent(type, value);

    return 0;
}

CARLA_BRIDGE_END_NAMESPACE

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

    using namespace CarlaBridge;

    // Init toolkit
    CarlaToolkit* const toolkit = CarlaToolkit::createNew(ui_title);
    toolkit->init();

    // Init LV2-UI
    CarlaLv2Client client(toolkit);

    // Init OSC
    if (! client.oscInit(osc_url))
    {
        toolkit->quit();
        delete toolkit;
        return -1;
    }

    // Load UI
    int ret;

    if (client.init(plugin_uri, ui_uri))
    {
        toolkit->exec(&client);
        ret = 0;
    }
    else
    {
        qCritical("Failed to load LV2 UI");
        ret = 1;
    }

    // Close OSC
    client.sendOscExiting();
    client.oscClose();

    // Close LV2-UI
    client.close();

    // Close toolkit
    toolkit->quit();
    delete toolkit;

    return ret;
}

#endif

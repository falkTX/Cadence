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

#include <QtCore/QDir>

#ifndef __WINE__
#include <QtGui/QLayout>
#endif

#ifdef HAVE_SUIL
#include <suil/suil.h>
struct SuilInstanceImpl {
    void*                   lib_handle;
    const LV2UI_Descriptor* descriptor;
    LV2UI_Handle            handle;
    // ...
};
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendLv2Plugin Carla Backend LV2 Plugin
 *
 * The Carla Backend LV2 Plugin.\n
 * http://lv2plug.in/
 * @{
 */

// static max values
const unsigned int MAX_EVENT_BUFFER = 8192; // 0x2000
//const unsigned int MAX_EVENT_BUFFER = (sizeof(LV2_Atom_Event) + 4) * MAX_MIDI_EVENTS;


/*!
 * @defgroup PluginHints Plugin Hints
 * @{
 */
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x100; //!< LV2 Plugin has DynParam extension
const unsigned int PLUGIN_HAS_EXTENSION_PROGRAMS = 0x200; //!< LV2 Plugin has Programs extension
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x400; //!< LV2 Plugin has State extension
const unsigned int PLUGIN_HAS_EXTENSION_WORKER   = 0x800; //!< LV2 Plugin has Worker extension
/**@}*/

/*!
 * @defgroup ParameterHints Parameter Hints
 * @{
 */
const unsigned int PARAMETER_IS_STRICT_BOUNDS = 0x1000; //!< LV2 Parameter needs strict bounds
const unsigned int PARAMETER_IS_TRIGGER       = 0x2000; //!< LV2 Parameter is trigger (current value should be changed to default after run())
/**@}*/

/*!
 * @defgroup Lv2FeatureIds LV2 Feature Ids
 *
 * Static index list of the internal LV2 Feature Ids.
 * \see CarlaPlugin::hints
 * @{
 */
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
/**@}*/

/*!
 * @defgroup Lv2EventTypes LV2 Event Data/Types
 *
 * Data and buffer types for LV2 EventData ports.
 * \see CarlaPlugin::hints
 * @{
 */
const unsigned int CARLA_EVENT_DATA_ATOM      = 0x01;
const unsigned int CARLA_EVENT_DATA_EVENT     = 0x02;
const unsigned int CARLA_EVENT_DATA_MIDI_LL   = 0x04;
const unsigned int CARLA_EVENT_TYPE_MESSAGE   = 0x10;
const unsigned int CARLA_EVENT_TYPE_MIDI      = 0x20;
/**@}*/

/*!
 * @defgroup Lv2UriMapIds LV2 URI Map Ids
 *
 * Static index list of the internal LV2 URI Map Ids.
 * \see CarlaPlugin::hints
 * @{
 */
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
/**@}*/

enum Lv2ParameterDataType {
    LV2_PARAMETER_TYPE_CONTROL
};

struct EventData {
    unsigned int type;
    CarlaEngineMidiPort* port;
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
        //case LV2_UI_HWND:
        //    return carla_options.bridge_lv2hwnd;
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
    Lv2Plugin(CarlaEngine* const engine, unsigned short id) : CarlaPlugin(engine, id)
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
        gui.resizable = false;
        gui.width  = 0;
        gui.height = 0;

#ifdef HAVE_SUIL
        suil.host = nullptr;
        suil.handle = nullptr;
#endif

        for (uint32_t i=0; i < CARLA_URI_MAP_ID_COUNT; i++)
            customURIDs.push_back(nullptr);

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
            case GUI_INTERNAL_HWND:
            case GUI_INTERNAL_X11:
                break;

            case GUI_EXTERNAL_LV2:
                //if (ui.widget)
                //    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);
                break;

#ifndef __WINE__
            case GUI_EXTERNAL_SUIL:
                if (ui.widget)
                    ((QWidget*)ui.widget)->close();
                break;
#endif

#ifndef BUILD_BRIDGE
            case GUI_EXTERNAL_OSC:
                if (osc.data.target)
                {
                    //osc_send_hide(&osc.data);
                    //osc_send_quit(&osc.data);
                }

                if (osc.thread)
                {
                    // Wait a bit first, try safe quit else force kill
                    if (osc.thread->isRunning())
                    {
                        if (! osc.thread->wait(2000))
                            osc.thread->quit();

                        if (osc.thread->isRunning() && ! osc.thread->wait(1000))
                        {
                            qWarning("Failed to properly stop LV2 OSC-GUI thread");
                            osc.thread->terminate();
                        }
                    }

                    delete osc.thread;
                }

                //osc_clear_data(&osc.data);

                break;
#endif

            default:
                break;
            }

#ifdef HAVE_SUIL
            if (suil.handle)
                suil_instance_free(suil.handle);

            if (suil.host)
                suil_host_free(suil.host);

            ui.handle = nullptr;
            ui.descriptor = nullptr;
#endif

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

            uiLibClose();
        }

        if (handle && descriptor && descriptor->deactivate && m_activeBefore)
            descriptor->deactivate(handle);

        if (handle && descriptor && descriptor->cleanup)
            descriptor->cleanup(handle);

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);

        if (features[lv2_feature_id_event] && features[lv2_feature_id_event]->data)
            delete (LV2_Event_Feature*)features[lv2_feature_id_event]->data;

        if (features[lv2_feature_id_logs] && features[lv2_feature_id_logs]->data)
            delete (LV2_Log_Log*)features[lv2_feature_id_logs]->data;

        if (features[lv2_feature_id_programs] && features[lv2_feature_id_programs]->data)
            delete (LV2_Programs_Host*)features[lv2_feature_id_programs]->data;

        if (features[lv2_feature_id_rtmempool] && features[lv2_feature_id_rtmempool]->data)
            delete (lv2_rtsafe_memory_pool_provider*)features[lv2_feature_id_rtmempool]->data;

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

    long uniqueId()
    {
        return rdf_descriptor->UniqueID;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t midiInCount()
    {
        uint32_t i, count = 0;

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t midiOutCount()
    {
        uint32_t i, count = 0;

        for (i=0; i < evout.count; i++)
        {
            if (evout.data[i].type & CARLA_EVENT_TYPE_MIDI)
                count += 1;
        }

        return count;
    }

    uint32_t parameterScalePointCount(uint32_t parameterId)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;
        return rdf_descriptor->Ports[rindex].ScalePointCount;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(uint32_t parameterId)
    {
        assert(parameterId < param.count);

        switch (lv2param[parameterId].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            return lv2param[parameterId].control;
        default:
            return 0.0;
        }
    }

    double getParameterScalePointValue(uint32_t parameterId, uint32_t scalePointId)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        int32_t rindex = param.data[parameterId].rindex;
        return rdf_descriptor->Ports[rindex].ScalePoints[scalePointId].Value;
    }

    void getLabel(char* const strBuf)
    {
        strncpy(strBuf, rdf_descriptor->URI, STR_MAX);
    }

    void getMaker(char* const strBuf)
    {
        if (rdf_descriptor->Author)
            strncpy(strBuf, rdf_descriptor->Author, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        if (rdf_descriptor->License)
            strncpy(strBuf, rdf_descriptor->License, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        if (rdf_descriptor->Name)
            strncpy(strBuf, rdf_descriptor->Name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;
        strncpy(strBuf, rdf_descriptor->Ports[rindex].Name, STR_MAX);
    }

    void getParameterSymbol(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;
        strncpy(strBuf, rdf_descriptor->Ports[rindex].Symbol, STR_MAX);
    }

    void getParameterUnit(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        int32_t rindex = param.data[parameterId].rindex;
        const LV2_RDF_Port* const Port = &rdf_descriptor->Ports[rindex];

        if (LV2_HAVE_UNIT_SYMBOL(Port->Unit.Hints))
            strncpy(strBuf, Port->Unit.Symbol, STR_MAX);

        else if (LV2_HAVE_UNIT(Port->Unit.Hints))
        {
            switch (Port->Unit.Type)
            {
            case LV2_UNIT_BAR:
                strncpy(strBuf, "bars", STR_MAX);
                return;
            case LV2_UNIT_BEAT:
                strncpy(strBuf, "beats", STR_MAX);
                return;
            case LV2_UNIT_BPM:
                strncpy(strBuf, "BPM", STR_MAX);
                return;
            case LV2_UNIT_CENT:
                strncpy(strBuf, "ct", STR_MAX);
                return;
            case LV2_UNIT_CM:
                strncpy(strBuf, "cm", STR_MAX);
                return;
            case LV2_UNIT_COEF:
                strncpy(strBuf, "(coef)", STR_MAX);
                return;
            case LV2_UNIT_DB:
                strncpy(strBuf, "dB", STR_MAX);
                return;
            case LV2_UNIT_DEGREE:
                strncpy(strBuf, "deg", STR_MAX);
                return;
            case LV2_UNIT_FRAME:
                strncpy(strBuf, "frames", STR_MAX);
                return;
            case LV2_UNIT_HZ:
                strncpy(strBuf, "Hz", STR_MAX);
                return;
            case LV2_UNIT_INCH:
                strncpy(strBuf, "in", STR_MAX);
                return;
            case LV2_UNIT_KHZ:
                strncpy(strBuf, "kHz", STR_MAX);
                return;
            case LV2_UNIT_KM:
                strncpy(strBuf, "km", STR_MAX);
                return;
            case LV2_UNIT_M:
                strncpy(strBuf, "m", STR_MAX);
                return;
            case LV2_UNIT_MHZ:
                strncpy(strBuf, "MHz", STR_MAX);
                return;
            case LV2_UNIT_MIDINOTE:
                strncpy(strBuf, "note", STR_MAX);
                return;
            case LV2_UNIT_MILE:
                strncpy(strBuf, "mi", STR_MAX);
                return;
            case LV2_UNIT_MIN:
                strncpy(strBuf, "min", STR_MAX);
                return;
            case LV2_UNIT_MM:
                strncpy(strBuf, "mm", STR_MAX);
                return;
            case LV2_UNIT_MS:
                strncpy(strBuf, "ms", STR_MAX);
                return;
            case LV2_UNIT_OCT:
                strncpy(strBuf, "oct", STR_MAX);
                return;
            case LV2_UNIT_PC:
                strncpy(strBuf, "%", STR_MAX);
                return;
            case LV2_UNIT_S:
                strncpy(strBuf, "s", STR_MAX);
                return;
            case LV2_UNIT_SEMITONE:
                strncpy(strBuf, "semi", STR_MAX);
                return;
            }
        }

        *strBuf = 0;
    }

    void getParameterScalePointLabel(uint32_t parameterId, uint32_t scalePointId, char* const strBuf)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        int32_t rindex = param.data[parameterId].rindex;
        strncpy(strBuf, rdf_descriptor->Ports[rindex].ScalePoints[scalePointId].Label, STR_MAX);
    }

    void getGuiInfo(GuiType* type, bool* resizable)
    {
        *type      = gui.type;
        *resizable = gui.resizable;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(uint32_t parameterId, double value, bool sendGui, bool sendOsc, bool sendCallback)
    {
        assert(parameterId < param.count);

        switch (lv2param[parameterId].type)
        {
        case LV2_PARAMETER_TYPE_CONTROL:
            lv2param[parameterId].control = fixParameterValue(value, param.ranges[parameterId]);
            break;
        default:
            break;
        }

        if (sendGui)
        {
            switch (gui.type)
            {
            case GUI_INTERNAL_QT4:
            case GUI_INTERNAL_HWND:
            case GUI_INTERNAL_X11:
            case GUI_EXTERNAL_LV2:
            case GUI_EXTERNAL_SUIL:
                if (ui.handle && ui.descriptor && ui.descriptor->port_event)
                {
                    float fvalue = value;
                    ui.descriptor->port_event(ui.handle, param.data[parameterId].rindex, sizeof(float), 0, &fvalue);
                }
                break;

#ifndef BUILD_BRIDGE
            case GUI_EXTERNAL_OSC:
                //osc_send_control(&osc.data, param.data[parameterId].rindex, value);
                break;
#endif
            default:
                break;
            }
        }

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(CustomDataType type, const char* key, const char* value, bool sendGui)
    {
        CarlaPlugin::setCustomData(type, key, value, sendGui);

        if (ext.state)
        {
            const char* const stype = customdatatype2str(type);
            LV2_State_Status status;

            if (x_engine->isOffline())
            {
                engineProcessLock();
                status = ext.state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
                engineProcessUnlock();
            }
            else
            {
                const CarlaPluginScopedDisabler m(this);
                status = ext.state->restore(handle, carla_lv2_state_retrieve, this, 0, features);
            }

            switch (status)
            {
            case LV2_STATE_SUCCESS:
                qDebug("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - success", stype, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_UNKNOWN:
                qWarning("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - unknown error", stype, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_TYPE:
                qWarning("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - error, bad type", stype, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_BAD_FLAGS:
                qWarning("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - error, bad flags", stype, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_FEATURE:
                qWarning("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - error, missing feature", stype, key, bool2str(sendGui));
                break;
            case LV2_STATE_ERR_NO_PROPERTY:
                qWarning("Lv2Plugin::setCustomData(%s, %s, <value>, %s) - error, missing property", stype, key, bool2str(sendGui));
                break;
            }
        }
    }

    void setMidiProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        assert(index < (int32_t)midiprog.count);

        if (ext.programs && index >= 0)
        {
            if (x_engine->isOffline())
            {
                if (block) engineProcessLock();
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
                if (block) engineProcessUnlock();
            }
            else
            {
                const CarlaPluginScopedDisabler m(this, block);
                ext.programs->select_program(handle, midiprog.data[index].bank, midiprog.data[index].program);
            }

            if (sendGui)
            {
#ifndef BUILD_BRIDGE
                //if (gui.type == GUI_EXTERNAL_OSC)
                //    osc_send_midi_program(&osc.data, midiprog.data[index].bank, midiprog.data[index].program, false);
                //else
#endif
                    if (ext.uiprograms)
                        ext.uiprograms->select_program(ui.handle, midiprog.data[index].bank, midiprog.data[index].program);
            }
        }

        CarlaPlugin::setMidiProgram(index, sendGui, sendOsc, sendCallback, block);
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void setGuiData(int, GuiDataHandle handle)
    {
        switch(gui.type)
        {
#ifndef __WINE__
        case GUI_INTERNAL_QT4:
            if (ui.widget)
            {
                QDialog* dialog = handle;
                QWidget* widget = (QWidget*)ui.widget;
                dialog->layout()->addWidget(widget);
                widget->adjustSize();
                widget->setParent(dialog);
                widget->show();
            }
            break;
#endif

        case GUI_INTERNAL_HWND:
        case GUI_INTERNAL_X11:
            if (ui.descriptor)
            {
#ifdef __WINE__
                features[lv2_feature_id_ui_parent]->data = (void*)handle;
#else
                features[lv2_feature_id_ui_parent]->data = (void*)handle->winId();
#endif
                ui.handle = ui.descriptor->instantiate(ui.descriptor,
                                                       descriptor->URI, ui.rdf_descriptor->Bundle,
                                                       carla_lv2_ui_write_function, this, &ui.widget,features);
                updateUi();
            }
            break;

        default:
            break;
        }
    }

    void showGui(bool yesNo)
    {
        switch(gui.type)
        {
        case GUI_INTERNAL_QT4:
            break;

        case GUI_INTERNAL_HWND:
        case GUI_INTERNAL_X11:
            if (yesNo && gui.width > 0 && gui.height > 0)
                x_engine->callback(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);
            break;

        case GUI_EXTERNAL_LV2:
            if (! ui.handle)
                initExternalUi();

            if (ui.handle && ui.descriptor && ui.widget)
            {
                if (yesNo)
                {
                    LV2_EXTERNAL_UI_SHOW((lv2_external_ui*)ui.widget);
                }
                else
                {
                    LV2_EXTERNAL_UI_HIDE((lv2_external_ui*)ui.widget);

                    if (rdf_descriptor->Author && strcmp(rdf_descriptor->Author, "linuxDSP") == 0)
                    {
                        qWarning("linuxDSP LV2 UI hack (force close instead of hide)");

                        if (ui.descriptor->cleanup)
                            ui.descriptor->cleanup(ui.handle);

                        ui.handle = nullptr;
                    }
                }
            }
            else
                // failed to init UI
                x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);

            break;

#ifndef __WINE__
        case GUI_EXTERNAL_SUIL:
            if (ui.widget)
            {
                QWidget* widget = (QWidget*)ui.widget;

#ifdef HAVE_SUIL
                if (yesNo)
                    widget->restoreGeometry(suil.pos);
                else
                    suil.pos = widget->saveGeometry();
#endif
                widget->setVisible(yesNo);
            }
            break;
#endif

#ifndef BUILD_BRIDGE
        case GUI_EXTERNAL_OSC:
            if (yesNo)
            {
                osc.thread->start();
            }
            else
            {
                //osc_send_hide(&osc.data);
                //osc_send_quit(&osc.data);
                //osc_clear_data(&osc.data);
            }
            break;
#endif

        default:
            break;
        }
    }

    void idleGui()
    {
        if (ui.handle && ui.descriptor && gui.type != GUI_EXTERNAL_OSC)
        {
            // Update output port values
            if (ui.descriptor->port_event)
            {
                float value;

                for (uint32_t i=0; i < param.count; i++)
                {
                    if (param.data[i].type == PARAMETER_OUTPUT)
                    {
                        value = getParameterValue(i);
                        ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
                    }
                }
            }

            // Update UI
            if (ui.widget && gui.type == GUI_EXTERNAL_LV2)
                LV2_EXTERNAL_UI_RUN((lv2_external_ui*)ui.widget);
        }
    }

    // -------------------------------------------------------------------
    // Plugin state

    void reload()
    {
        qDebug("Lv2Plugin::reload() - start");

        // Safely disable plugin for reload
        const CarlaPluginScopedDisabler m(this);

        if (x_client->isActive())
            x_client->deactivate();

        // Remove client ports
        removeClientPorts();

        // Delete old data
        deleteBuffers();

        uint32_t ains, aouts, cvIns, cvOuts, params, j;
        ains = aouts = cvIns = cvOuts = params = 0;
        std::vector<unsigned int> evIns, evOuts;

        const double sampleRate = x_engine->getSampleRate();
        const uint32_t PortCount = rdf_descriptor->PortCount;

        for (uint32_t i=0; i < PortCount; i++)
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
                    cvIns += 1;
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    cvOuts += 1;
            }
            else if (LV2_IS_PORT_ATOM_SEQUENCE(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    evIns.push_back(CARLA_EVENT_DATA_ATOM);
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    evOuts.push_back(CARLA_EVENT_DATA_ATOM);
            }
            else if (LV2_IS_PORT_EVENT(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    evIns.push_back(CARLA_EVENT_DATA_EVENT);
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    evOuts.push_back(CARLA_EVENT_DATA_EVENT);
            }
            else if (LV2_IS_PORT_MIDI_LL(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                    evIns.push_back(CARLA_EVENT_DATA_MIDI_LL);
                else if (LV2_IS_PORT_OUTPUT(PortType))
                    evOuts.push_back(CARLA_EVENT_DATA_MIDI_LL);
            }
            else if (LV2_IS_PORT_CONTROL(PortType))
                params += 1;
        }

        if (ains > 0)
        {
            ain.ports    = new CarlaEngineAudioPort*[ains];
            ain.rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new CarlaEngineAudioPort*[aouts];
            aout.rindexes = new uint32_t[aouts];
        }

        if (evIns.size() > 0)
        {
            const size_t size = evIns.size();
            evin.data = new EventData[size];

            for (j=0; j < size; j++)
            {
                evin.data[j].port = nullptr;
                evin.data[j].type = 0;

                if (evIns[j] == CARLA_EVENT_DATA_ATOM)
                {
                    evin.data[j].type = CARLA_EVENT_DATA_ATOM;
                    evin.data[j].atom = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evin.data[j].atom->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evin.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evin.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    evin.data[j].atom->body.pad  = 0;
                }
                else if (evIns[j] == CARLA_EVENT_DATA_EVENT)
                {
                    evin.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    evin.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (evIns[j] == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evin.data[j].type  = CARLA_EVENT_DATA_MIDI_LL;
                    evin.data[j].midi  = new LV2_MIDI;
                    evin.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    evin.data[j].midi->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
            }
        }

        if (evOuts.size() > 0)
        {
            const size_t size = evOuts.size();
            evout.data = new EventData[size];

            for (j=0; j < size; j++)
            {
                evout.data[j].port = nullptr;
                evout.data[j].type = 0;

                if (evOuts[j] == CARLA_EVENT_DATA_ATOM)
                {
                    evout.data[j].type = CARLA_EVENT_DATA_ATOM;
                    evout.data[j].atom = (LV2_Atom_Sequence*)malloc(sizeof(LV2_Atom_Sequence) + MAX_EVENT_BUFFER);
                    evout.data[j].atom->atom.size = sizeof(LV2_Atom_Sequence_Body);
                    evout.data[j].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                    evout.data[j].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                    evout.data[j].atom->body.pad  = 0;
                }
                else if (evOuts[j] == CARLA_EVENT_DATA_EVENT)
                {
                    evout.data[j].type  = CARLA_EVENT_DATA_EVENT;
                    evout.data[j].event = lv2_event_buffer_new(MAX_EVENT_BUFFER, LV2_EVENT_AUDIO_STAMP);
                }
                else if (evOuts[j] == CARLA_EVENT_DATA_MIDI_LL)
                {
                    evout.data[j].type  = CARLA_EVENT_DATA_MIDI_LL;
                    evout.data[j].midi  = new LV2_MIDI;
                    evout.data[j].midi->capacity = MAX_EVENT_BUFFER;
                    evout.data[j].midi->data     = new unsigned char [MAX_EVENT_BUFFER];
                }
            }
        }

        if (params > 0)
        {
            param.data   = new ParameterData[params];
            param.ranges = new ParameterRanges[params];
            lv2param     = new Lv2ParameterData[params];
        }

        const int portNameSize = CarlaEngine::maxPortNameSize() - 1;
        char portName[portNameSize];
        bool needsCin  = false;
        bool needsCout = false;

        for (uint32_t i=0; i < PortCount; i++)
        {
            const LV2_Property PortType  = rdf_descriptor->Ports[i].Type;

            if (LV2_IS_PORT_AUDIO(PortType) || LV2_IS_PORT_ATOM_SEQUENCE(PortType) || LV2_IS_PORT_CV(PortType) || LV2_IS_PORT_EVENT(PortType) || LV2_IS_PORT_MIDI_LL(PortType))
            {
#ifndef BUILD_BRIDGE
                if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
                {
                    strcpy(portName, m_name);
                    strcat(portName, ":");
                    strncat(portName, rdf_descriptor->Ports[i].Name, portNameSize/2);
                }
                else
#endif
                    strncpy(portName, rdf_descriptor->Ports[i].Name, portNameSize);
            }

            if (LV2_IS_PORT_AUDIO(PortType))
            {
                if (LV2_IS_PORT_INPUT(PortType))
                {
                    j = ain.count++;
                    ain.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, true);
                    ain.rindexes[j] = i;
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = aout.count++;
                    aout.ports[j]    = (CarlaEngineAudioPort*)x_client->addPort(CarlaEnginePortTypeAudio, portName, false);
                    aout.rindexes[j] = i;
                    needsCin = true;
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
                    descriptor->connect_port(handle, i, evin.data[j].atom);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    }
                    if (PortType & LV2_PORT_SUPPORTS_PATCH_MESSAGE)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MESSAGE;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].atom);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
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
                    descriptor->connect_port(handle, i, evin.data[j].event);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evin.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].event);

                    if (PortType & LV2_PORT_SUPPORTS_MIDI_EVENT)
                    {
                        evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                        evout.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
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
                    descriptor->connect_port(handle, i, evin.data[j].midi);

                    evin.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    evin.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, true);
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    j = evout.count++;
                    descriptor->connect_port(handle, i, evout.data[j].midi);

                    evout.data[j].type |= CARLA_EVENT_TYPE_MIDI;
                    evout.data[j].port  = (CarlaEngineMidiPort*)x_client->addPort(CarlaEnginePortTypeMIDI, portName, false);
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
                param.data[j].midiChannel = 0;
                param.data[j].midiCC = -1;

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

                // stupid hack for ir.lv2 (broken plugin)
                if (strcmp(rdf_descriptor->URI, "http://factorial.hu/plugins/lv2/ir") == 0 && strncmp(rdf_descriptor->Ports[i].Name, "FileHash", 8) == 0)
                {
                    min = 0.0;
                    max = 16777215.0; // 0xffffff
                }

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
                    min *= sampleRate;
                    max *= sampleRate;
                    def *= sampleRate;
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

                    if (rdf_descriptor->Ports[i].Designation == 0)
                    {
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCin = true;
                    }

                    // MIDI CC value
                    LV2_RDF_PortMidiMap* PortMidiMap = &rdf_descriptor->Ports[i].MidiMap;
                    if (LV2_IS_PORT_MIDI_MAP_CC(PortMidiMap->Type))
                    {
                        if (! MIDI_IS_CONTROL_BANK_SELECT(PortMidiMap->Number))
                            param.data[j].midiCC = PortMidiMap->Number;
                    }
                }
                else if (LV2_IS_PORT_OUTPUT(PortType))
                {
                    if (LV2_IS_PORT_LATENCY(PortProps))
                    {
                        min = 0.0;
                        max = sampleRate;
                        def = 0.0;
                        step = 1.0;
                        step_small = 1.0;
                        step_large = 1.0;

                        param.data[j].type  = PARAMETER_LATENCY;
                        param.data[j].hints = 0;
                    }
                    else
                    {
                        param.data[j].type   = PARAMETER_OUTPUT;
                        param.data[j].hints |= PARAMETER_IS_ENABLED;
                        param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
                        needsCout = true;
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
                param.ranges[j].stepSmall = step_small;
                param.ranges[j].stepLarge = step_large;

                // Set LV2 params as needed
                lv2param[j].type = LV2_PARAMETER_TYPE_CONTROL;
                lv2param[j].control = def;

                descriptor->connect_port(handle, i, &lv2param[j].control);
            }
            else
                // Port Type not supported, but it's optional anyway
                descriptor->connect_port(handle, i, nullptr);
        }

        if (needsCin)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-in");
            }
            else
#endif
                strcpy(portName, "control-in");

            param.portCin = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, true);
        }

        if (needsCout)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.process_mode != PROCESS_MODE_MULTIPLE_CLIENTS)
            {
                strcpy(portName, m_name);
                strcat(portName, ":control-out");
            }
            else
#endif
                strcpy(portName, "control-out");

            param.portCout = (CarlaEngineControlPort*)x_client->addPort(CarlaEnginePortTypeControl, portName, false);
        }

        ain.count   = ains;
        aout.count  = aouts;
        evin.count  = evIns.size();
        evout.count = evOuts.size();
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

        reloadPrograms(true);

        //if (ext.dynparam)
        //    ext.dynparam->host_attach(handle, &dynparam_host, this);

        x_client->activate();

        qDebug("Lv2Plugin::reload() - end");
    }

    void reloadPrograms(bool init)
    {
        qDebug("Lv2Plugin::reloadPrograms(%s)", bool2str(init));
        uint32_t i, oldCount = midiprog.count;

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
            midiprog.data  = new midi_program_t[midiprog.count];

        // Update data
        for (i=0; i < midiprog.count; i++)
        {
            const LV2_Program_Descriptor* const pdesc = ext.programs->get_program(handle, i);
            assert(pdesc);

            midiprog.data[i].bank    = pdesc->bank;
            midiprog.data[i].program = pdesc->program;
            midiprog.data[i].name    = strdup(pdesc->name);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        //osc_global_send_set_midi_program_count(m_id, midiprog.count);

        //for (i=0; i < midiprog.count; i++)
        //    osc_global_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (midiprog.count > 0)
                setMidiProgram(0, false, false, false, true);
        }
        else
        {
            x_engine->callback(CALLBACK_UPDATE, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool programChanged = false;

            if (midiprog.count == oldCount+1)
            {
                // one midi program added, probably created by user
                midiprog.current = oldCount;
                programChanged   = true;
            }
            else if (midiprog.current >= (int32_t)midiprog.count)
            {
                // current midi program > count
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current < 0 && midiprog.count > 0)
            {
                // programs exist now, but not before
                midiprog.current = 0;
                programChanged   = true;
            }
            else if (midiprog.current >= 0 && midiprog.count == 0)
            {
                // programs existed before, but not anymore
                midiprog.current = -1;
                programChanged   = true;
            }

            if (programChanged)
                setMidiProgram(midiprog.current, true, true, true, true);
        }
    }

    void prepareForSave()
    {
        if (ext.state)
            ext.state->save(handle, carla_lv2_state_store, this, LV2_STATE_IS_POD, features);
    }

    // -------------------------------------------------------------------
    // Plugin processing

    void process(float** inBuffer, float** outBuffer, uint32_t frames, uint32_t framesOffset)
    {
        uint32_t i, k;
        uint32_t midiEventCount = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        // handle midi from different APIs
        uint32_t evinAtomOffsets[evin.count];
        LV2_Event_Iterator evinEventIters[evin.count];
        LV2_MIDIState evinMidiStates[evin.count];

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evinAtomOffsets[i] = 0;
                evin.data[i].atom->atom.size = 0;
                evin.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evin.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                evin.data[i].atom->body.pad  = 0;
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evin.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evin.data[i].event + 1));
                lv2_event_begin(&evinEventIters[i], evin.data[i].event);
            }
            else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                evinMidiStates[i].midi = evin.data[i].midi;
                evinMidiStates[i].frame_count = frames;
                evinMidiStates[i].position = 0;
                evinMidiStates[i].midi->event_count = 0;
                evinMidiStates[i].midi->size = 0;
            }
        }

        for (i=0; i < evout.count; i++)
        {
            if (evout.data[i].type & CARLA_EVENT_DATA_ATOM)
            {
                evout.data[i].atom->atom.size = 0;
                evout.data[i].atom->atom.type = CARLA_URI_MAP_ID_ATOM_SEQUENCE;
                evout.data[i].atom->body.unit = CARLA_URI_MAP_ID_NULL;
                evout.data[i].atom->body.pad  = 0;
            }
            else if (evout.data[i].type & CARLA_EVENT_DATA_EVENT)
            {
                lv2_event_buffer_reset(evout.data[i].event, LV2_EVENT_AUDIO_STAMP, (uint8_t*)(evout.data[i].event + 1));
            }
            else if (evout.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
            {
                // not needed
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            if (ain.count == 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs(inBuffer[0][k]);
                }
            }
            else if (ain.count >= 1)
            {
                for (k=0; k < frames; k++)
                {
                    if (abs(inBuffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs(inBuffer[0][k]);

                    if (abs(inBuffer[1][k]) > ains_peak_tmp[1])
                        ains_peak_tmp[1] = abs(inBuffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.portCin && m_active && m_activeBefore)
        {
            bool allNotesOffSent = false;

            const CarlaEngineControlEvent* cinEvent;
            uint32_t time, nEvents = param.portCin->getEventCount();

            uint32_t nextBankId = 0;
            if (midiprog.current >= 0 && midiprog.count > 0)
                nextBankId = midiprog.data[midiprog.current].bank;

            for (i=0; i < nEvents; i++)
            {
                cinEvent = param.portCin->getEvent(i);

                if (! cinEvent)
                    continue;

                time = cinEvent->time - framesOffset;

                if (time >= frames)
                    continue;

                // Control change
                switch (cinEvent->type)
                {
                case CarlaEngineEventNull:
                    break;

                case CarlaEngineEventControlChange:
                {
                    double value;

                    // Control backend stuff
                    if (cinEvent->channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(cinEvent->controller) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = cinEvent->value;
                            setDryWet(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_CHANNEL_VOLUME(cinEvent->controller) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = cinEvent->value*127/100;
                            setVolume(value, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }

                        if (MIDI_IS_CONTROL_BALANCE(cinEvent->controller) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = cinEvent->value/0.5 - 1.0;

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

                            setBalanceLeft(left, false, false);
                            setBalanceRight(right, false, false);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postponeEvent(PluginPostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midiChannel != cinEvent->channel)
                            continue;
                        if (param.data[k].midiCC != cinEvent->controller)
                            continue;
                        if (param.data[k].type != PARAMETER_INPUT)
                            continue;

                        if (param.data[k].hints & PARAMETER_IS_AUTOMABLE)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = cinEvent->value < 0.5 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = cinEvent->value * (param.ranges[k].max - param.ranges[k].min) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            setParameterValue(k, value, false, false, false);
                            postponeEvent(PluginPostEventParameterChange, k, value);
                        }
                    }

                    break;
                }

                case CarlaEngineEventMidiBankChange:
                    if (cinEvent->channel == cin_channel)
                        nextBankId = rint(cinEvent->value);
                    break;

                case CarlaEngineEventMidiProgramChange:
                    if (cinEvent->channel == cin_channel)
                    {
                        uint32_t nextProgramId = rint(cinEvent->value);

                        for (k=0; k < midiprog.count; k++)
                        {
                            if (midiprog.data[k].bank == nextBankId && midiprog.data[k].program == nextProgramId)
                            {
                                setMidiProgram(k, false, false, false, false);
                                postponeEvent(PluginPostEventMidiProgramChange, k, 0.0);
                                break;
                            }
                        }
                    }
                    break;

                case CarlaEngineEventAllSoundOff:
                    if (cinEvent->channel == cin_channel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        if (descriptor->deactivate)
                            descriptor->deactivate(handle);

                        if (descriptor->activate)
                            descriptor->activate(handle);

                        allNotesOffSent = true;
                    }
                    break;

                case CarlaEngineEventAllNotesOff:
                    if (cinEvent->channel == cin_channel)
                    {
                        if (midi.portMin && ! allNotesOffSent)
                            sendMidiAllNotesOff();

                        allNotesOffSent = true;
                    }
                    break;
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (evin.count > 0 && cin_channel >= 0 && cin_channel < 16 && m_active && m_activeBefore)
        {
            engineMidiLock();

            for (i=0; i < MAX_MIDI_EVENTS && midiEventCount < MAX_MIDI_EVENTS; i++)
            {
                if (! extMidiNotes[i].valid)
                    break;

                uint8_t midiEvent[4] = { 0 };
                midiEvent[0] = cin_channel + extMidiNotes[i].velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                midiEvent[1] = extMidiNotes[i].note;
                midiEvent[2] = extMidiNotes[i].velo;

                // send to all midi inputs
                for (k=0; k < evin.count; k++)
                {
                    if (evin.data[k].type & CARLA_EVENT_TYPE_MIDI)
                    {
                        if (evin.data[k].type & CARLA_EVENT_DATA_ATOM)
                        {
                            LV2_Atom_Event* const aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evin.data[k].atom) + evinAtomOffsets[k]);
                            aev->time.frames = framesOffset;
                            aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                            aev->body.size   = 3;
                            memcpy(LV2_ATOM_BODY(&aev->body), midiEvent, 3);

                            const uint32_t padSize = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + 3);
                            evinAtomOffsets[k]           += padSize;
                            evin.data[k].atom->atom.size += padSize;
                        }
                        else if (evin.data[k].type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evinEventIters[k], framesOffset, 0, CARLA_URI_MAP_ID_MIDI_EVENT, 3, midiEvent);

                        else if (evin.data[k].type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evinMidiStates[k], framesOffset, 3, midiEvent);
                    }
                }

                extMidiNotes[i].valid = false;
                midiEventCount += 1;
            }

            engineMidiUnlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (System)

        if (evin.count > 0 && m_active && m_activeBefore)
        {
            for (i=0; i < evin.count; i++)
            {
                if (! evin.data[i].port)
                    continue;

                const CarlaEngineMidiEvent* minEvent;
                uint32_t time, nEvents = evin.data[i].port->getEventCount();

                for (k=0; k < nEvents && midiEventCount < MAX_MIDI_EVENTS; k++)
                {
                    minEvent = evin.data[i].port->getEvent(k);

                    if (! minEvent)
                        continue;

                    time = minEvent->time - framesOffset;

                    if (time >= frames)
                        continue;

                    uint8_t status  = minEvent->data[0];
                    uint8_t channel = status & 0x0F;

                    // Fix bad note-off
                    if (MIDI_IS_STATUS_NOTE_ON(status) && minEvent->data[2] == 0)
                        status -= 0x10;

                    // write supported status types
                    if (MIDI_IS_STATUS_NOTE_OFF(status) || MIDI_IS_STATUS_NOTE_ON(status) || MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status) || MIDI_IS_STATUS_AFTERTOUCH(status) || MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                    {
                        if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
                        {
                            LV2_Atom_Event* aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, evin.data[i].atom) + evinAtomOffsets[i]);
                            aev->time.frames = time;
                            aev->body.type   = CARLA_URI_MAP_ID_MIDI_EVENT;
                            aev->body.size   = minEvent->size;
                            memcpy(LV2_ATOM_BODY(&aev->body), minEvent->data, minEvent->size);

                            const uint32_t padSize = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + minEvent->size);
                            evinAtomOffsets[i]           += padSize;
                            evin.data[i].atom->atom.size += padSize;
                        }
                        else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
                            lv2_event_write(&evinEventIters[i], time, 0, CARLA_URI_MAP_ID_MIDI_EVENT, minEvent->size, minEvent->data);

                        else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                            lv2midi_put_event(&evinMidiStates[i], time, minEvent->size, minEvent->data);

                        if (channel == cin_channel)
                        {
                            if (MIDI_IS_STATUS_NOTE_OFF(status))
                                postponeEvent(PluginPostEventNoteOff, minEvent->data[1], 0.0);
                            else if (MIDI_IS_STATUS_NOTE_ON(status))
                                postponeEvent(PluginPostEventNoteOn, minEvent->data[1], minEvent->data[2]);
                        }
                    }

                    midiEventCount += 1;
                }
            }
        }// End of MIDI Input (System)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Special Parameters

        int32_t rindex;
        const CarlaTimeInfo* const timeInfo = x_engine->getTimeInfo();

        for (k=0; k < param.count; k++)
        {
            if (param.data[k].type == PARAMETER_LATENCY)
            {
                // TODO
                continue;
            }

            rindex = param.data[k].rindex;

            switch (rdf_descriptor->Ports[rindex].Designation)
            {
            case LV2_PORT_TIME_BAR:
                setParameterValue(k, timeInfo->bbt.bar, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.bar);
                break;
            case LV2_PORT_TIME_BAR_BEAT:
                setParameterValue(k, timeInfo->bbt.tick, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.tick);
                break;
            case LV2_PORT_TIME_BEAT:
                setParameterValue(k, timeInfo->bbt.beat, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.beat);
                break;
            case LV2_PORT_TIME_BEAT_UNIT:
                setParameterValue(k, timeInfo->bbt.beat_type, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.beat_type);
                break;
            case LV2_PORT_TIME_BEATS_PER_BAR:
                setParameterValue(k, timeInfo->bbt.beats_per_bar, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.beats_per_bar);
                break;
            case LV2_PORT_TIME_BEATS_PER_MINUTE:
                setParameterValue(k, timeInfo->bbt.beats_per_minute, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->bbt.beats_per_minute);
                break;
            case LV2_PORT_TIME_FRAME:
                setParameterValue(k, timeInfo->frame, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->frame);
                break;
            case LV2_PORT_TIME_FRAMES_PER_SECOND:
                break;
            case LV2_PORT_TIME_POSITION:
                setParameterValue(k, timeInfo->time, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->time);
                break;
            case LV2_PORT_TIME_SPEED:
                setParameterValue(k, timeInfo->playing ? 1.0 : 0.0, false, false, false);
                postponeEvent(PluginPostEventParameterChange, k, timeInfo->playing ? 1.0 : 0.0);
                break;
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (! m_activeBefore)
            {
                // TODO - send sound-off notes-off events here

                if (descriptor->activate)
                    descriptor->activate(handle);
            }

            for (i=0; i < ain.count; i++)
                descriptor->connect_port(handle, ain.rindexes[i], inBuffer[i]);

            for (i=0; i < aout.count; i++)
                descriptor->connect_port(handle, aout.rindexes[i], outBuffer[i]);

            if (descriptor->run)
                descriptor->run(handle, frames);
        }
        else
        {
            if (m_activeBefore)
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
            float oldBufLeft[do_balance ? frames : 0];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                if (do_drywet || do_volume)
                {
                    for (k=0; k < frames; k++)
                    {
                        if (do_drywet)
                        {
                            if (aout.count == 1)
                                outBuffer[i][k] = (outBuffer[i][k]*x_drywet)+(inBuffer[0][k]*(1.0-x_drywet));
                            else
                                outBuffer[i][k] = (outBuffer[i][k]*x_drywet)+(inBuffer[i][k]*(1.0-x_drywet));
                        }

                        if (do_volume)
                            outBuffer[i][k] *= x_vol;
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&oldBufLeft, outBuffer[i], sizeof(float)*frames);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k < frames; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            outBuffer[i][k]  = oldBufLeft[k]*(1.0-bal_rangeL);
                            outBuffer[i][k] += outBuffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            outBuffer[i][k]  = outBuffer[i][k]*bal_rangeR;
                            outBuffer[i][k] += oldBufLeft[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; i < 2 && k < frames; k++)
                {
                    if (abs(outBuffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs(outBuffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(outBuffer[i], 0.0f, sizeof(float)*frames);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Control Output

        if (param.portCout && m_active)
        {
            double value, rvalue;

            for (k=0; k < param.count; k++)
            {
                if (param.data[k].type == PARAMETER_OUTPUT)
                {
                    if (lv2param[k].type == LV2_PARAMETER_TYPE_CONTROL)
                        fixParameterValue(lv2param[k].control, param.ranges[k]);

                    if (param.data[k].midiCC > 0)
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

                        rvalue = (value - param.ranges[k].min) / (param.ranges[k].max - param.ranges[k].min);
                        param.portCout->writeEvent(CarlaEngineEventControlChange, framesOffset, param.data[k].midiChannel, param.data[k].midiCC, rvalue);
                    }
                }
            }
        } // End of Control Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (evout.count > 0 && m_active)
        {
            for (i=0; i < evout.count; i++)
            {
                if (! evout.data[i].port)
                    continue;

                if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    // TODO
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    LV2_Event* ev;
                    LV2_Event_Iterator iter;

                    uint8_t* data;
                    lv2_event_begin(&iter, evout.data[i].event);

                    for (k=0; k < iter.buf->event_count; k++)
                    {
                        ev = lv2_event_get(&iter, &data);

                        if (ev && data)
                            evout.data[i].port->writeEvent(ev->frames, data, ev->size);

                        lv2_event_increment(&iter);
                    }
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    LV2_MIDIState state = { evout.data[i].midi, frames, 0 };

                    uint32_t eventSize;
                    double   eventTime;
                    unsigned char* eventData;

                    while (lv2midi_get_event(&state, &eventTime, &eventSize, &eventData) < frames)
                    {
                        evout.data[i].port->writeEvent(eventTime, eventData, eventSize);
                        lv2midi_step(&state);
                    }
                }
            }
        }// End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        x_engine->setInputPeak(m_id, 0, ains_peak_tmp[0]);
        x_engine->setInputPeak(m_id, 1, ains_peak_tmp[1]);
        x_engine->setOutputPeak(m_id, 0, aouts_peak_tmp[0]);
        x_engine->setOutputPeak(m_id, 1, aouts_peak_tmp[1]);

        m_activeBefore = m_active;
    }

    // -------------------------------------------------------------------
    // Cleanup

    void removeClientPorts()
    {
        qDebug("Lv2Plugin::removeClientPorts() - start");

        for (uint32_t i=0; i < evin.count; i++)
        {
            if (evin.data[i].port)
            {
                delete evin.data[i].port;
                evin.data[i].port = nullptr;
            }
        }

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].port)
            {
                delete evout.data[i].port;
                evout.data[i].port = nullptr;
            }
        }

        CarlaPlugin::removeClientPorts();

        qDebug("Lv2Plugin::removeClientPorts() - end");
    }

    void initBuffers()
    {
        uint32_t i;

        for (i=0; i < evin.count; i++)
        {
            if (evin.data[i].port)
                evin.data[i].port->initBuffer(x_engine);
        }

        for (uint32_t i=0; i < evout.count; i++)
        {
            if (evout.data[i].port)
                evout.data[i].port->initBuffer(x_engine);
        }

        CarlaPlugin::initBuffers();
    }

    void deleteBuffers()
    {
        qDebug("Lv2Plugin::deleteBuffers() - start");

        if (param.count > 0)
            delete[] lv2param;

        if (evin.count > 0)
        {
            for (uint32_t i=0; i < evin.count; i++)
            {
                if (evin.data[i].type & CARLA_EVENT_DATA_ATOM)
                {
                    free(evin.data[i].atom);
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evin.data[i].event);
                }
                else if (evin.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evin.data[i].midi->data;
                    delete evin.data[i].midi;
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
                    free(evout.data[i].atom);
                }
                else if (evout.data[i].type & CARLA_EVENT_DATA_EVENT)
                {
                    free(evout.data[i].event);
                }
                else if (evout.data[i].type & CARLA_EVENT_DATA_MIDI_LL)
                {
                    delete[] evout.data[i].midi->data;
                    delete evout.data[i].midi;
                }
            }

            delete[] evout.data;
        }

        lv2param = nullptr;

        evin.count = 0;
        evin.data  = nullptr;

        evout.count = 0;
        evout.data  = nullptr;

        qDebug("Lv2Plugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------

    uint32_t getCustomURID(const char* const uri)
    {
        qDebug("Lv2Plugin::getCustomURID(%s)", uri);

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
        qDebug("Lv2Plugin::getCustomURIString(%i)", urid);

        if (urid < customURIDs.size())
            return customURIDs[urid];

        return nullptr;
    }

    void handleAtomTransfer()
    {
        // TODO
    }

    void handleEventTransfer(const char* const type, const char* const key, const char* const stringData)
    {
        qDebug("Lv2Plugin::handleEventTransfer(%s, %s, %s)", type, key, stringData);
        assert(type);
        assert(key);
        assert(stringData);

        QByteArray chunk;
        chunk = QByteArray::fromBase64(stringData);

        const LV2_Atom* const atom = (LV2_Atom*)chunk.constData();
        const LV2_URID uridAtomBlank = getCustomURID(LV2_ATOM__Blank);
        const LV2_URID uridPatchBody = getCustomURID(LV2_PATCH__body);
        const LV2_URID uridPatchSet  = getCustomURID(LV2_PATCH__Set);

        if (atom->type != uridAtomBlank)
        {
            qWarning("Lv2Plugin::handleEventTransfer() - Not blank");
            return;
        }

        const LV2_Atom_Object* const obj = (LV2_Atom_Object*)atom;

        if (obj->body.otype != uridPatchSet)
        {
            qWarning("Lv2Plugin::handleEventTransfer() - Not Patch Set");
            return;
        }

        const LV2_Atom_Object* body = nullptr;
        lv2_atom_object_get(obj, uridPatchBody, &body, 0);

        if (! body)
        {
            qWarning("Lv2Plugin::handleEventTransfer() - Has no body");
            return;
        }

        LV2_ATOM_OBJECT_FOREACH(body, iter)
        {
            CustomDataType dtype  = CUSTOM_DATA_INVALID;
            const char* const key = getCustomURIString(iter->key);
            const char* value     = nullptr;

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

            setCustomData(dtype, key, value, false);

            free((void*)value);
        }
    }

    void handleProgramChanged(int32_t index)
    {
        if (index == -1)
        {
            const CarlaPluginScopedDisabler m(this);
            reloadPrograms(false);
        }
        else
        {
            if (ext.programs && index >= 0 && index < (int32_t)prog.count)
            {
                const char* progName = ext.programs->get_program(handle, index)->name;

                if (prog.names[index])
                    free((void*)prog.names[index]);

                prog.names[index] = strdup(progName);
            }
        }

        x_engine->callback(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
    }

    LV2_State_Status handleStateStore(uint32_t key, const void* const value, size_t size, uint32_t type, uint32_t flags)
    {
        assert(key > 0);
        assert(value);

        CustomDataType dtype;
        const char* const uriKey = getCustomURIString(key);

        if (type == CARLA_URI_MAP_ID_ATOM_CHUNK)
            dtype = CUSTOM_DATA_CHUNK;
        else if (type == CARLA_URI_MAP_ID_ATOM_PATH)
            dtype = CUSTOM_DATA_PATH;
        else if (type == CARLA_URI_MAP_ID_ATOM_STRING)
            dtype = CUSTOM_DATA_STRING;
        else if (type >= CARLA_URI_MAP_ID_COUNT)
            dtype = CUSTOM_DATA_BINARY;
        else
            dtype = CUSTOM_DATA_INVALID;

        // do basic checks
        if (! uriKey)
        {
            qWarning("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid key", handle, key, value, size, type, flags);
            return LV2_STATE_ERR_NO_PROPERTY;
        }

        if (! flags & LV2_STATE_IS_POD)
        {
            qWarning("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid flags", handle, key, value, size, type, flags);
            return LV2_STATE_ERR_BAD_FLAGS;
        }

        if (dtype == CUSTOM_DATA_INVALID)
        {
            qCritical("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i) - Invalid type '%s'", handle, key, value, size, type, flags, customdatatype2str(dtype));
            return LV2_STATE_ERR_BAD_TYPE;
        }

        // Check if we already have this key
        for (size_t i=0; i < custom.size(); i++)
        {
            if (strcmp(custom[i].key, uriKey) == 0)
            {
                free((void*)custom[i].value);

                if (dtype == CUSTOM_DATA_STRING || dtype == CUSTOM_DATA_PATH)
                    custom[i].value = strdup((const char*)value);
                else
                    custom[i].value = strdup(QByteArray((const char*)value, size).toBase64().constData());

                return LV2_STATE_SUCCESS;
            }
        }

        // Add a new one then
        CustomData newData;
        newData.type = dtype;
        newData.key  = strdup(uriKey);

        if (dtype == CUSTOM_DATA_STRING || dtype == CUSTOM_DATA_PATH)
            newData.value = strdup((const char*)value);
        else
            newData.value = strdup(QByteArray((const char*)value, size).toBase64().constData());

        custom.push_back(newData);

        return LV2_STATE_SUCCESS;
    }

    const void* handleStateRetrieve(uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        assert(key > 0);

        const char* const uriKey = getCustomURIString(key);

        if (! uriKey)
        {
            qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Failed to find key", handle, key, size, type, flags);
            return nullptr;
        }

        const char* stringData = nullptr;
        CustomDataType dtype = CUSTOM_DATA_INVALID;

        for (size_t i=0; i < custom.size(); i++)
        {
            if (strcmp(custom[i].key, uriKey) == 0)
            {
                dtype      = custom[i].type;
                stringData = custom[i].value;
                break;
            }
        }

        if (! stringData)
        {
            qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key '%s'", handle, key, size, type, flags, uriKey);
            return nullptr;
        }

        *size  = 0;
        *type  = 0;
        *flags = LV2_STATE_IS_POD;

        if (dtype == CUSTOM_DATA_STRING)
        {
            *size = strlen(stringData);
            *type = CARLA_URI_MAP_ID_ATOM_STRING;
            return stringData;
        }

        if (dtype == CUSTOM_DATA_PATH)
        {
            *size = strlen(stringData);
            *type = CARLA_URI_MAP_ID_ATOM_PATH;
            return stringData;
        }

        if (dtype == CUSTOM_DATA_CHUNK || dtype == CUSTOM_DATA_BINARY)
        {
            static QByteArray chunk;
            chunk = QByteArray::fromBase64(stringData);

            *size = chunk.size();
            *type = (dtype == CUSTOM_DATA_CHUNK) ? CARLA_URI_MAP_ID_ATOM_CHUNK : key;
            return chunk.constData();
        }

        qCritical("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p) - Invalid key type '%s'", handle, key, size, type, flags, customdatatype2str(dtype));
        return nullptr;
    }

    LV2_Worker_Status handleWorkerSchedule(uint32_t /*size*/, const void* const /*data*/)
    {
        return LV2_WORKER_SUCCESS;
    }

    LV2_Worker_Status handleWorkerRespond(uint32_t /*size*/, const void* const /*data*/)
    {
        if (ext.worker)
            return LV2_WORKER_SUCCESS;
        return LV2_WORKER_ERR_UNKNOWN;
    }

    void handleExternalUiClosed()
    {
        if (ui.handle && ui.descriptor && ui.descriptor->cleanup)
            ui.descriptor->cleanup(ui.handle);

        ui.handle = nullptr;
        x_engine->callback(CALLBACK_SHOW_GUI, m_id, 0, 0, 0.0);
    }

    uint32_t handleUiPortMap(const char* const symbol)
    {
        assert(symbol);

        for (uint32_t i=0; i < rdf_descriptor->PortCount; i++)
        {
            if (strcmp(rdf_descriptor->Ports[i].Symbol, symbol) == 0)
                return i;
        }

        return LV2UI_INVALID_PORT_INDEX;
    }

    int handleUiResize(int width, int height)
    {
        assert(width > 0);
        assert(height > 0);

        gui.width  = width;
        gui.height = height;
        x_engine->callback(CALLBACK_RESIZE_GUI, m_id, width, height, 0.0);

        return 0;
    }

    void handleUiWrite(uint32_t rindex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (format == 0)
        {
            assert(bufferSize == sizeof(float));
            float value = *(float*)buffer;

            for (uint32_t i=0; i < param.count; i++)
            {
                if (param.data[i].rindex == (int32_t)rindex)
                    return setParameterValue(i, value, false, true, true);
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
            const LV2_Atom* const atom = (LV2_Atom*)buffer;

            if (ui.handle && ui.descriptor && ui.descriptor->port_event)
                ui.descriptor->port_event(ui.handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);
        }
    }

    // -------------------------------------------------------------------

#ifndef BUILD_BRIDGE
    bool isUiBridgeable(uint32_t uiId)
    {
        const LV2_RDF_UI* const rdf_ui = &rdf_descriptor->UIs[uiId];

        for (uint32_t i=0; i < rdf_ui->FeatureCount; i++)
        {
            if (strcmp(rdf_ui->Features[i].URI, LV2_INSTANCE_ACCESS_URI) == 0 || strcmp(rdf_ui->Features[i].URI, LV2_DATA_ACCESS_URI) == 0)
                return false;
        }

        return true;
    }
#endif

    bool isUiResizable()
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

    void initExternalUi()
    {
        qDebug("Lv2Plugin::initExternalUi()");
        ui.widget = nullptr;
        ui.handle = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);

        if (ui.handle && ui.widget)
        {
            updateUi();
        }
        else
        {
            ui.handle = nullptr;
            ui.widget = nullptr;
            x_engine->callback(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);
        }
    }

    void updateUi()
    {
        qDebug("Lv2Plugin::updateUi()");
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
                // update state (custom data)
                for (size_t i=0; i < custom.size(); i++)
                {
                    if (custom[i].type == CUSTOM_DATA_INVALID)
                        continue;

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

                    lv2_atom_forge_property_head(&forge, getCustomURID(custom[i].key), CARLA_URI_MAP_ID_NULL);

                    if (custom[i].type == CUSTOM_DATA_STRING)
                        lv2_atom_forge_string(&forge, custom[i].value, strlen(custom[i].value));
                    else if (custom[i].type == CUSTOM_DATA_PATH)
                        lv2_atom_forge_path(&forge, custom[i].value, strlen(custom[i].value));
                    else if (custom[i].type == CUSTOM_DATA_CHUNK)
                        lv2_atom_forge_literal(&forge, custom[i].value, strlen(custom[i].value), CARLA_URI_MAP_ID_ATOM_CHUNK, CARLA_URI_MAP_ID_NULL);
                    else
                        lv2_atom_forge_literal(&forge, custom[i].value, strlen(custom[i].value), getCustomURID(custom[i].key), CARLA_URI_MAP_ID_NULL);

                    lv2_atom_forge_pop(&forge, &bodyFrame);
                    lv2_atom_forge_pop(&forge, &refFrame);

                    const LV2_Atom* const atom = lv2_atom_forge_deref(&forge, ref);
                    ui.descriptor->port_event(ui.handle, 0, atom->size, CARLA_URI_MAP_ID_ATOM_TRANSFER_EVENT, atom);

                    free((void*)chunk.buf);
                    sratom_free(sratom);
                }

                // update control ports
                float value;
                for (uint32_t i=0; i < param.count; i++)
                {
                    value = getParameterValue(i);
                    ui.descriptor->port_event(ui.handle, param.data[i].rindex, sizeof(float), 0, &value);
                }
            }
        }
    }

    // ----------------- DynParam Feature ------------------------------------------------
    // TODO

    // ----------------- Event Feature ---------------------------------------------------

    static uint32_t carla_lv2_event_ref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_ref(%p, %p)", callback_data, event);
        assert(callback_data);
        assert(event);

        return 0;
    }

    static uint32_t carla_lv2_event_unref(LV2_Event_Callback_Data callback_data, LV2_Event* event)
    {
        qDebug("Lv2Plugin::carla_lv2_event_unref(%p, %p)", callback_data, event);
        assert(callback_data);
        assert(event);

        return 0;
    }

    // ----------------- Logs Feature ----------------------------------------------------

    static int carla_lv2_log_printf(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
    {
        qDebug("Lv2Plugin::carla_lv2_log_printf(%p, %i, %s, ...)", handle, type, fmt);
        assert(handle);
        assert(type > 0);

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
        qDebug("Lv2Plugin::carla_lv2_log_vprintf(%p, %i, %s, ...)", handle, type, fmt);
        assert(handle);
        assert(type > 0);

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
        qDebug("Lv2Plugin::carla_lv2_program_changed(%p, %i)", handle, index);
        assert(handle);

        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        plugin->handleProgramChanged(index);
    }

    // ----------------- State Feature ---------------------------------------------------

    static char* carla_lv2_state_make_path(LV2_State_Make_Path_Handle handle, const char* path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_make_path(%p, %p)", handle, path);
        assert(handle);
        assert(path);

        QDir dir;
        dir.mkpath(path);
        return strdup(path);
    }

    static char* carla_lv2_state_map_abstract_path(LV2_State_Map_Path_Handle handle, const char* absolute_path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_map_abstract_path(%p, %p)", handle, absolute_path);
        assert(handle);
        assert(absolute_path);

        QDir dir(absolute_path);
        return strdup(dir.canonicalPath().toUtf8().constData());
    }

    static char* carla_lv2_state_map_absolute_path(LV2_State_Map_Path_Handle handle, const char* abstract_path)
    {
        qDebug("Lv2Plugin::carla_lv2_state_map_absolute_path(%p, %p)", handle, abstract_path);
        assert(handle);
        assert(abstract_path);

        QDir dir(abstract_path);
        return strdup(dir.absolutePath().toUtf8().constData());
    }

    static LV2_State_Status carla_lv2_state_store(LV2_State_Handle handle, uint32_t key, const void* value, size_t size, uint32_t type, uint32_t flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_store(%p, %i, %p, " P_SIZE ", %i, %i)", handle, key, value, size, type, flags);
        assert(handle);

        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->handleStateStore(key, value, size, type, flags);
    }

    static const void* carla_lv2_state_retrieve(LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags)
    {
        qDebug("Lv2Plugin::carla_lv2_state_retrieve(%p, %i, %p, %p, %p)", handle, key, size, type, flags);
        assert(handle);

        Lv2Plugin*  plugin = (Lv2Plugin*)handle;
        return plugin->handleStateRetrieve(key, size, type, flags);
    }

    // ----------------- URI-Map Feature -------------------------------------------------

    static uint32_t carla_lv2_uri_to_id(LV2_URI_Map_Callback_Data data, const char* map, const char* uri)
    {
        qDebug("Lv2Plugin::carla_lv2_uri_to_id(%p, %s, %s)", data, map, uri);
        return carla_lv2_urid_map(data, uri);
    }

    // ----------------- URID Feature ----------------------------------------------------

    static LV2_URID carla_lv2_urid_map(LV2_URID_Map_Handle handle, const char* uri)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_map(%p, %s)", handle, uri);
        assert(handle);
        assert(uri);

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
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURID(uri);
    }

    static const char* carla_lv2_urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        qDebug("Lv2Plugin::carla_lv2_urid_unmap(%p, %i)", handle, urid);
        assert(handle);
        assert(urid > 0);

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
        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->getCustomURIString(urid);
    }

    // ----------------- Worker Feature --------------------------------------------------

    static LV2_Worker_Status carla_lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_schedule(%p, %i, %p)", handle, size, data);
        assert(handle);

        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->handleWorkerSchedule(size, data);
    }

    static LV2_Worker_Status carla_lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
    {
        qDebug("Lv2Plugin::carla_lv2_worker_respond(%p, %i, %p)", handle, size, data);
        assert(handle);

        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->handleWorkerRespond(size, data);
    }

    // ----------------- UI Port-Map Feature ---------------------------------------------

    static uint32_t carla_lv2_ui_port_map(LV2UI_Feature_Handle handle, const char* symbol)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_port_map(%p, %s)", handle, symbol);
        assert(handle);

        Lv2Plugin* plugin = (Lv2Plugin*)handle;
        return plugin->handleUiPortMap(symbol);
    }

    // ----------------- UI Resize Feature -----------------------------------------------

    static int carla_lv2_ui_resize(LV2UI_Feature_Handle handle, int width, int height)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_resize(%p, %i, %i)", handle, width, height);
        assert(handle);

        Lv2Plugin* plugin  = (Lv2Plugin*)handle;
        return plugin->handleUiResize(width, height);
    }

    // ----------------- External UI Feature ---------------------------------------------

    static void carla_lv2_external_ui_closed(LV2UI_Controller controller)
    {
        qDebug("Lv2Plugin::carla_lv2_external_ui_closed(%p)", controller);
        assert(controller);

        Lv2Plugin* plugin = (Lv2Plugin*)controller;
        plugin->handleExternalUiClosed();
    }

    // ----------------- UI Extension ----------------------------------------------------

    static void carla_lv2_ui_write_function(LV2UI_Controller controller, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
    {
        qDebug("Lv2Plugin::carla_lv2_ui_write_function(%p, %i, %i, %i, %p)", controller, port_index, buffer_size, format, buffer);
        assert(controller);

        Lv2Plugin* plugin = (Lv2Plugin*)controller;
        plugin->handleUiWrite(port_index, buffer_size, format, buffer);
    }

    // -------------------------------------------------------------------

    bool uiLibOpen(const char* filename)
    {
        ui.lib = lib_open(filename);
        return bool(ui.lib);
    }

    bool uiLibClose()
    {
        if (ui.lib)
            return lib_close(ui.lib);
        return false;
    }

    void* uiLibSymbol(const char* symbol)
    {
        if (ui.lib)
            return lib_symbol(ui.lib, symbol);
        return nullptr;
    }

    // -------------------------------------------------------------------

    bool init(const char* bundle, const char* const name, const char* URI)
    {
        // ---------------------------------------------------------------
        // get plugin from lv2_rdf (lilv)

        rdf_descriptor = lv2_rdf_new(URI);

        if (! rdf_descriptor)
        {
            set_last_error("Failed to find the requested plugin in the LV2 Bundle");
            return false;
        }

        // ---------------------------------------------------------------
        // open DLL

        if (! libOpen(rdf_descriptor->Binary))
        {
            set_last_error(libError(rdf_descriptor->Binary));
            return false;
        }

        // ---------------------------------------------------------------
        // get DLL main entry

        LV2_Descriptor_Function descfn = (LV2_Descriptor_Function)libSymbol("lv2_descriptor");

        if (! descfn)
        {
            set_last_error("Could not find the LV2 Descriptor in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // get descriptor that matches URI

        uint32_t i = 0;
        while ((descriptor = descfn(i++)))
        {
            if (strcmp(descriptor->URI, URI) == 0)
                break;
        }

        if (! descriptor)
        {
            set_last_error("Could not find the requested plugin URI in the plugin library");
            return false;
        }

        // ---------------------------------------------------------------
        // check supported port-types and features

        bool canContinue = true;

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
                    canContinue = false;
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
                canContinue = false;
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

        if (! canContinue)
        {
            // error already set
            return false;
        }

        // ---------------------------------------------------------------
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

        // ---------------------------------------------------------------
        // initialize plugin

        handle = descriptor->instantiate(descriptor, x_engine->getSampleRate(), rdf_descriptor->Bundle, features);

        if (! handle)
        {
            set_last_error("Plugin failed to initialize");
            return false;
        }

        // ---------------------------------------------------------------
        // get info

        m_filename = strdup(bundle);

        if (name)
            m_name = x_engine->getUniqueName(name);
        else
            m_name = x_engine->getUniqueName(rdf_descriptor->Name);

        // ---------------------------------------------------------------
        // register client

        x_client = x_engine->addClient(this);

        if (! x_client->isOk())
        {
            set_last_error("Failed to register plugin client");
            return false;
        }

        // ---------------------------------------------------------------
        // gui stuff

        if (rdf_descriptor->UICount > 0)
        {
            qDebug("Has UI");

            // -----------------------------------------------------------
            // find more appropriate ui

            int eQt4, eHWND, eX11, eGtk2, iHWND, iX11, iQt4, iExt, iSuil, iFinal;
            eQt4 = eHWND = eX11 = eGtk2 = iQt4 = iHWND = iX11 = iExt = iSuil = iFinal = -1;

            for (i=0; i < rdf_descriptor->UICount; i++)
            {
                switch (rdf_descriptor->UIs[i].Type)
                {
                case LV2_UI_QT4:
#ifndef BUILD_BRIDGE
                    if (isUiBridgeable(i) && carla_options.prefer_ui_bridges)
                        eQt4 = i;
#endif
                    iQt4 = i;
                    break;

                case LV2_UI_HWND:
#ifndef BUILD_BRIDGE
                    if (isUiBridgeable(i) && carla_options.prefer_ui_bridges)
                        eHWND = i;
#endif
                    iHWND = i;
                    break;

                case LV2_UI_X11:
#ifndef BUILD_BRIDGE
                    if (isUiBridgeable(i) && carla_options.prefer_ui_bridges)
                        eX11 = i;
#endif
                    iX11 = i;
                    break;

                case LV2_UI_GTK2:
#ifdef BUILD_BRIDGE
                    if (false)
#else
#  ifdef HAVE_SUIL
                    if (isUiBridgeable(i) && carla_options.prefer_ui_bridges)
#  else
                    if (isUiBridgeable(i))
#  endif
#endif
                        eGtk2 = i;
#ifdef HAVE_SUIL
                    iSuil = i;
#endif
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
            else if (eHWND >= 0)
                iFinal = eHWND;
            else if (eX11 >= 0)
                iFinal = eX11;
            else if (eGtk2 >= 0)
                iFinal = eGtk2;
            else if (iQt4 >= 0)
                iFinal = iQt4;
            else if (iHWND >= 0)
                iFinal = iHWND;
            else if (iX11 >= 0)
                iFinal = iX11;
            else if (iExt >= 0)
                iFinal = iExt;
            else if (iSuil >= 0)
                iFinal = iSuil;

#ifndef BUILD_BRIDGE
            bool isBridged = (iFinal == eQt4 || iFinal == eX11 || iFinal == eGtk2);
#endif
#ifdef HAVE_SUIL
            bool isSuil = (iFinal == iSuil && !isBridged);
#endif

            if (iFinal < 0)
            {
                qWarning("Failed to find an appropriate LV2 UI for this plugin");
                return true;
            }

            ui.rdf_descriptor = &rdf_descriptor->UIs[iFinal];

            // -----------------------------------------------------------
            // check supported ui features

            canContinue = true;

            for (i=0; i < ui.rdf_descriptor->FeatureCount; i++)
            {
                if (LV2_IS_FEATURE_REQUIRED(ui.rdf_descriptor->Features[i].Type) && is_lv2_ui_feature_supported(ui.rdf_descriptor->Features[i].URI) == false)
                {
                    qCritical("Plugin UI requires a feature that is not supported:\n%s", ui.rdf_descriptor->Features[i].URI);
                    canContinue = false;
                    break;
                }
            }

            if (! canContinue)
            {
                ui.rdf_descriptor = nullptr;
                return true;
            }

#ifdef HAVE_SUIL
            if (isSuil)
            {
                // -------------------------------------------------------
                // init suil host

                suil.host = suil_host_new(carla_lv2_ui_write_function, carla_lv2_ui_port_map, nullptr, nullptr);
            }
            else
#endif
            {
                // -------------------------------------------------------
                // open DLL

                if (! uiLibOpen(ui.rdf_descriptor->Binary))
                {
                    qCritical("Could not load UI library, error was:\n%s", libError(ui.rdf_descriptor->Binary));
                    ui.rdf_descriptor = nullptr;
                    return true;
                }

                // -------------------------------------------------------
                // get DLL main entry

                LV2UI_DescriptorFunction ui_descfn = (LV2UI_DescriptorFunction)uiLibSymbol("lv2ui_descriptor");

                if (! ui_descfn)
                {
                    qCritical("Could not find the LV2UI Descriptor in the UI library");
                    uiLibClose();
                    ui.lib = nullptr;
                    ui.rdf_descriptor = nullptr;
                    return true;
                }

                // -------------------------------------------------------
                // get descriptor that matches URI

                i = 0;
                while ((ui.descriptor = ui_descfn(i++)))
                {
                    if (strcmp(ui.descriptor->URI, ui.rdf_descriptor->URI) == 0)
                        break;
                }

                if (! ui.descriptor)
                {
                    qCritical("Could not find the requested GUI in the plugin UI library");
                    uiLibClose();
                    ui.lib = nullptr;
                    ui.rdf_descriptor = nullptr;
                    return true;
                }
            }

            // -----------------------------------------------------------
            // initialize ui according to type

            LV2_Property UiType = ui.rdf_descriptor->Type;

#ifndef BUILD_BRIDGE
            if (isBridged)
            {
                // -------------------------------------------------------
                // initialize ui bridge

                const char* const oscBinary = "/home/falktx/Personal/FOSS/GIT/Cadence/src/carla-bridge/carla-bridge-lv2-gtk2"; //lv2bridge2str(UiType);
                qDebug("Has UI - is bridge, uitype = %i : %s", UiType, oscBinary);

                if (oscBinary)
                {
                    qDebug("Has UI - has binary");
                    gui.type = GUI_EXTERNAL_OSC;
                    osc.thread = new CarlaPluginThread(x_engine, this, CarlaPluginThread::PLUGIN_THREAD_LV2_GUI);
                    osc.thread->setOscData(oscBinary, descriptor->URI, ui.descriptor->URI);
                }
                else
                    qDebug("Has UI - NOT binary");
            }
            else
#endif
            {
                // -------------------------------------------------------
                // initialize ui features

                QString guiTitle = QString("%1 (GUI)").arg(m_name);

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
                External_UI_Feature->plugin_human_id           = strdup(guiTitle.toUtf8().constData());

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

                // -------------------------------------------------------
                // initialize ui

                switch (UiType)
                {
                case LV2_UI_QT4:
                    qDebug("Will use LV2 Qt4 UI");
                    gui.type      = GUI_INTERNAL_QT4;
                    gui.resizable = isUiResizable();
                    ui.handle     = ui.descriptor->instantiate(ui.descriptor, descriptor->URI, ui.rdf_descriptor->Bundle, carla_lv2_ui_write_function, this, &ui.widget, features);
                    updateUi();
                    break;

                case LV2_UI_HWND:
                    qDebug("Will use LV2 HWND UI");
                    gui.type      = GUI_INTERNAL_HWND;
                    gui.resizable = isUiResizable();
                    break;

                case LV2_UI_X11:
                    qDebug("Will use LV2 X11 UI");
                    gui.type      = GUI_INTERNAL_X11;
                    gui.resizable = isUiResizable();
                    break;

                case LV2_UI_GTK2:
#ifdef HAVE_SUIL
                    qDebug("Will use LV2 Gtk2 UI (suil)");
                    gui.type      = GUI_EXTERNAL_SUIL;
                    gui.resizable = isUiResizable();
                    suil.handle   = suil_instance_new(suil.host, this, LV2_UI__Qt4UI, rdf_descriptor->URI, ui.rdf_descriptor->URI, lv2_get_ui_uri(ui.rdf_descriptor->Type), ui.rdf_descriptor->Bundle, ui.rdf_descriptor->Binary, features);

                    if (suil.handle)
                    {
                        ui.handle     = ((SuilInstanceImpl*)suil.handle)->handle;
                        ui.descriptor = ((SuilInstanceImpl*)suil.handle)->descriptor;
                        ui.widget     = suil_instance_get_widget(suil.handle);

                        if (ui.widget)
                        {
                            QWidget* widget = (QWidget*)ui.widget;
                            widget->setWindowTitle(guiTitle);

                            // FIXME - need a proper way for this
                            if (strcmp(ui.rdf_descriptor->URI, "http://factorial.hu/plugins/lv2/ir/gui") == 0)
                                widget->resize(930, 460);
                        }
                    }
#else
                    qDebug("Will use LV2 Gtk2 UI, NOT!");
#endif
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

            if (gui.type != GUI_NONE)
                m_hints |= PLUGIN_HAS_GUI;

        } // End of GUI Stuff

        return true;
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
        bool resizable;
        int width;
        int height;
    } gui;

#ifdef HAVE_SUIL
    struct {
        SuilHost* host;
        SuilInstance* handle;
        QByteArray pos;
    } suil;
#endif

    PluginEventData evin;
    PluginEventData evout;
    Lv2ParameterData* lv2param;
    std::vector<const char*> customURIDs;
};

short CarlaPlugin::newLV2(const initializer& init)
{
    qDebug("CarlaPlugin::newLV2(%p, %s, %s, %s)", init.engine, init.filename, init.name, init.label);

    short id = init.engine->getNewPluginIndex();

    if (id < 0)
    {
        set_last_error("Maximum number of plugins reached");
        return -1;
    }

    Lv2Plugin* plugin = new Lv2Plugin(init.engine, id);

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return -1;
    }

    plugin->reload();

#ifndef BUILD_BRIDGE
    if (carla_options.process_mode == PROCESS_MODE_CONTINUOUS_RACK)
    {
        if (/* inputs */ ((plugin->audioInCount() != 0 && plugin->audioInCount() != 2)) || /* outputs */ ((plugin->audioOutCount() != 0 && plugin->audioOutCount() != 2)))
        {
            set_last_error("Carla Rack Mode can only work with Stereo plugins, sorry!");
            delete plugin;
            return -1;
        }

    }
#endif

    plugin->registerToOsc();
    init.engine->addPlugin(id, plugin);

    return id;
}

/**@}*/

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

int osc_handle_lv2_atom_transfer(CarlaBackend::CarlaPlugin* plugin, lo_arg** /*argv*/)
{
    CarlaBackend::Lv2Plugin* lv2plugin = (CarlaBackend::Lv2Plugin*)plugin;

    lv2plugin->handleAtomTransfer();

    return 0;
}

int osc_handle_lv2_event_transfer(CarlaBackend::CarlaPlugin* plugin, lo_arg** argv)
{
    CarlaBackend::Lv2Plugin* lv2plugin = (CarlaBackend::Lv2Plugin*)plugin;

    const char* type  = (const char*)&argv[0]->s;
    const char* key   = (const char*)&argv[1]->s;
    const char* value = (const char*)&argv[2]->s;
    lv2plugin->handleEventTransfer(type, key, value);

    return 0;
}

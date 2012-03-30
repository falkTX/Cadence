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

#include "carla_plugin.h"

#include "lv2/lv2.h"
#include "lv2/ui.h"

#include "lv2_rdf.h"

// static max values
const unsigned int MAX_EVENT_BUFFER = 0x7FFF; // 32767

// feature ids
//const uint32_t lv2_feature_id_uri_map         = 0;
//const uint32_t lv2_feature_id_urid_map        = 1;
//const uint32_t lv2_feature_id_urid_unmap      = 2;
//const uint32_t lv2_feature_id_event           = 3;
//const uint32_t lv2_feature_id_rtmempool       = 4;
//const uint32_t lv2_feature_id_data_access     = 5;
//const uint32_t lv2_feature_id_instance_access = 6;
//const uint32_t lv2_feature_id_ui_resize       = 7;
//const uint32_t lv2_feature_id_ui_parent       = 8;
//const uint32_t lv2_feature_id_external_ui     = 9;
//const uint32_t lv2_feature_id_external_ui_old = 10;
const uint32_t lv2_feature_count              = 0; //11;

// extra plugin hints
const unsigned int PLUGIN_HAS_EXTENSION_STATE    = 0x1000;
const unsigned int PLUGIN_HAS_EXTENSION_DYNPARAM = 0x2000;

class Lv2Plugin : public CarlaPlugin
{
public:
    Lv2Plugin() :
        CarlaPlugin()
    {
        qDebug("Lv2Plugin::Lv2Plugin()");
        m_type = PLUGIN_LV2;

        handle = nullptr;
        descriptor = nullptr;
        rdf_descriptor = nullptr;

        ui.lib = nullptr;
        ui.handle = nullptr;
        ui.descriptor = nullptr;
        ui.rdf_descriptor = nullptr;

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

        //for (uint32_t i=0; i < lv2_feature_count && features[i]; i++)
        //    delete features[i];

        if (rdf_descriptor)
            lv2_rdf_free(rdf_descriptor);
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
        else
            return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return rdf_descriptor->UniqueID;
    }

    void get_label(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->URI, STR_MAX);
    }

    void get_maker(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->Author, STR_MAX);
    }

    void get_copyright(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->License, STR_MAX);
    }

    void get_real_name(char* buf_str)
    {
        strncpy(buf_str, rdf_descriptor->Name, STR_MAX);
    }

    bool init(const char* filename, const char* URI, void* extra_stuff)
    {
        LV2_RDF_Descriptor* rdf_descriptor_ = (LV2_RDF_Descriptor*)extra_stuff;

        qDebug("INIT %s", URI);

        if (rdf_descriptor_)
        {
            qDebug("INIT 002");
            if (lib_open(rdf_descriptor_->Binary))
            {
                qDebug("INIT 003");
                LV2_Descriptor_Function descfn = (LV2_Descriptor_Function)lib_symbol("lv2_descriptor");

                if (descfn)
                {
                    qDebug("INIT 004");
                    uint32_t i = 0;
                    while ((descriptor = descfn(i++)))
                    {
                        qDebug("%s | %s", descriptor->URI, URI);
                        if (strcmp(descriptor->URI, URI) == 0)
                            break;
                    }
                    qDebug("INIT 005");

                    if (descriptor)
                    {
                        // TODO - can continue

                        handle = descriptor->instantiate(descriptor, get_sample_rate(), rdf_descriptor_->Bundle, features);

                        if (handle)
                        {
                            m_filename = strdup(filename);
                            m_name = get_unique_name(rdf_descriptor_->Name);

                            rdf_descriptor = lv2_rdf_dup(rdf_descriptor_);

                            if (register_jack_plugin())
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

    struct {
        void* lib;
        LV2UI_Handle handle;
        LV2UI_Widget widget;
        const LV2UI_Descriptor* descriptor;
        const LV2_RDF_UI* rdf_descriptor;
    } ui;
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

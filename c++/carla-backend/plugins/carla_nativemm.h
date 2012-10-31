/*
 * Carla Native Plugin API
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_NATIVE_MM_H
#define CARLA_NATIVE_MM_H

#include "carla_native.h"
#include "carla_includes.h"

class PluginDescriptorClass {
public:
    PluginDescriptorClass(const PluginDescriptorClass* master)
    {
        if (master)
        {
            desc.category  = master->desc.category;
            desc.hints     = master->desc.hints;
            desc.name      = master->desc.name;
            desc.label     = master->desc.label;
            desc.maker     = master->desc.maker;
            desc.copyright = master->desc.copyright;

            desc.portCount = master->desc.portCount;
            desc.ports     = master->desc.ports;

            host = master->host;
        }
        else
        {
            desc.category  = PLUGIN_CATEGORY_NONE;
            desc.hints     = 0;
            desc.name      = nullptr;
            desc.label     = nullptr;
            desc.maker     = nullptr;
            desc.copyright = nullptr;

            desc.portCount = 0;
            desc.ports     = nullptr;

            host = nullptr;
        }

        _initDescriptor();
    }

    virtual ~PluginDescriptorClass()
    {
    }

    uint32_t getBufferSize() const
    {
        CARLA_ASSERT(host);

        if (host)
            return host->get_buffer_size(host->handle);

        return 0;
    }

    double getSampleRate() const
    {
        CARLA_ASSERT(host);

        if (host)
            return host->get_sample_rate(host->handle);

        return 0.0;
    }

    const TimeInfo* getTimeInfo() const
    {
        CARLA_ASSERT(host);

        if (host)
            return host->get_time_info(host->handle);

        return nullptr;
    }

    void writeMidiEvent(MidiEvent* event)
    {
        CARLA_ASSERT(host);

        if (host)
            host->write_midi_event(host->handle, event);
    }

    // -------------------------------------------------------------------

    const PluginDescriptor* descriptorInit()
    {
        desc.category  = getCategory();
        desc.hints     = getHints();
        desc.name      = getName();
        desc.label     = getLabel();
        desc.maker     = getMaker();
        desc.copyright = getCopyright();
        return &desc;
    }

    // -------------------------------------------------------------------

protected:
    virtual PluginDescriptorClass* createMe() = 0;
    virtual void deleteMe() = 0;

    virtual PluginCategory getCategory()
    {
        return PLUGIN_CATEGORY_NONE;
    }

    virtual uint32_t getHints()
    {
        return 0;
    }

    virtual const char* getName()
    {
        return nullptr;
    }

    virtual const char* getLabel()
    {
        return nullptr;
    }

    virtual const char* getMaker()
    {
        return nullptr;
    }

    virtual const char* getCopyright()
    {
        return nullptr;
    }

    // -------------------------------------------------------------------

    virtual uint32_t getPortCount()
    {
        return 0;
    }

    virtual PortType getPortType(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return PORT_TYPE_NULL;
    }

    virtual uint32_t getPortHints(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return 0;
    }

    virtual const char* getPortName(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return nullptr;
    }

    virtual void getParameterRanges(uint32_t index, ParameterRanges* const ranges)
    {
        CARLA_ASSERT(index < getPortCount());
        CARLA_ASSERT(ranges);
    }

    virtual double getParameterValue(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return 0.0;
    }

    virtual const char* getParameterText(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return nullptr;
    }

    virtual const char* getParameterUnit(uint32_t index)
    {
        CARLA_ASSERT(index < getPortCount());

        return nullptr;
    }

    // -------------------------------------------------------------------

    virtual const MidiProgram* getMidiProgram(uint32_t index)
    {
        Q_UNUSED(index);
        return nullptr;
    }

    // -------------------------------------------------------------------

    virtual void setParameterValue(uint32_t index, double value)
    {
        CARLA_ASSERT(index < getPortCount());
        Q_UNUSED(value);
    }

    virtual void setMidiProgram(uint32_t bank, uint32_t program)
    {
        CARLA_ASSERT(program < 128);
        Q_UNUSED(bank);
    }

    virtual void setCustomData(const char* key, const char* value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }

    // -------------------------------------------------------------------

    virtual void activate()
    {
    }

    virtual void deactivate()
    {
    }

    // -------------------------------------------------------------------

    virtual void showGui(bool show)
    {
        Q_UNUSED(show);
    }

    virtual void idleGui()
    {
    }

    // -------------------------------------------------------------------

    virtual void process(float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        CARLA_ASSERT(inBuffer);
        CARLA_ASSERT(outBuffer);
        CARLA_ASSERT(midiEvents);

        Q_UNUSED(frames);
        Q_UNUSED(midiEventCount);
    }

    // -------------------------------------------------------------------

private:
    PluginDescriptor desc;
    const HostDescriptor* host;

    void _initDescriptor()
    {
        desc.instantiate = _instantiate;
        desc.activate    = _activate;
        desc.deactivate  = _deactivate;
        desc.cleanup     = _cleanup;

        desc.get_parameter_ranges = _get_parameter_ranges;
        desc.get_parameter_value  = _get_parameter_value;
        desc.get_parameter_text   = _get_parameter_text;
        desc.get_parameter_unit   = _get_parameter_unit;

        desc.get_midi_program     = _get_midi_program;

        desc.set_parameter_value  = _set_parameter_value;
        desc.set_midi_program     = _set_midi_program;
        desc.set_custom_data      = _set_custom_data;

        desc.show_gui = _show_gui;
        desc.idle_gui = _idle_gui;

        desc.process = _process;

        desc._singleton = this;
        desc._init   = _init;
        desc._fini   = _fini;
    }

    static PluginHandle _instantiate(struct _PluginDescriptor* _this_, HostDescriptor* host)
    {
        PluginDescriptorClass* singleton = (PluginDescriptorClass*)_this_->_singleton;
        singleton->host = host;
        return singleton->createMe();
    }

    static void _activate(PluginHandle handle)
    {
        ((PluginDescriptorClass*)handle)->activate();
    }

    static void _deactivate(PluginHandle handle)
    {
        ((PluginDescriptorClass*)handle)->deactivate();
    }

    static void _cleanup(PluginHandle handle)
    {
        ((PluginDescriptorClass*)handle)->deleteMe();
    }

    static void _get_parameter_ranges(PluginHandle handle, uint32_t index, ParameterRanges* ranges)
    {
        ((PluginDescriptorClass*)handle)->getParameterRanges(index, ranges);
    }

    static double _get_parameter_value(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterValue(index);
    }

    static const char* _get_parameter_text(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterText(index);
    }

    static const char* _get_parameter_unit(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterUnit(index);
    }

    static const MidiProgram* _get_midi_program(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getMidiProgram(index);
    }

    static void _set_parameter_value(PluginHandle handle, uint32_t index, double value)
    {
        return ((PluginDescriptorClass*)handle)->setParameterValue(index, value);
    }

    static void _set_midi_program(PluginHandle handle, uint32_t bank, uint32_t program)
    {
        return ((PluginDescriptorClass*)handle)->setMidiProgram(bank, program);
    }

    static void _set_custom_data(PluginHandle handle, const char* key, const char* value)
    {
        return ((PluginDescriptorClass*)handle)->setCustomData(key, value);
    }

    static void _show_gui(PluginHandle handle, bool show)
    {
        return ((PluginDescriptorClass*)handle)->showGui(show);
    }

    static void _idle_gui(PluginHandle handle)
    {
        return ((PluginDescriptorClass*)handle)->idleGui();
    }

    static void _process(PluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        return ((PluginDescriptorClass*)handle)->process(inBuffer, outBuffer, frames, midiEventCount, midiEvents);
    }

    static void _init(PluginDescriptor* const _this_)
    {
        ((PluginDescriptorClass*)_this_->_singleton)->_handleInit();
    }

    static void _fini(PluginDescriptor* const _this_)
    {
        ((PluginDescriptorClass*)_this_->_singleton)->_handleFini();
    }

    void _handleInit()
    {
        desc.portCount = getPortCount();

        if (desc.portCount > 0)
        {
            desc.ports = new PluginPort [desc.portCount];

            for (uint32_t i=0; i < desc.portCount; i++)
            {
                PluginPort* const port = &desc.ports[i];

                port->type  = getPortType(i);
                port->hints = getPortHints(i);
                port->name  = getPortName(i);

                port->scalePointCount = 0;
                port->scalePoints = nullptr;
            }
        }
    }

    void _handleFini()
    {
        if (desc.portCount > 0 && desc.ports)
        {
            for (uint32_t i=0; i < desc.portCount; i++)
            {
                PluginPort* const port = &desc.ports[i];

                if (port->scalePointCount > 0 && port->scalePoints)
                    delete[] port->scalePoints;
            }

            delete[] desc.ports;
        }

        desc.portCount = 0;
        desc.ports = nullptr;
    }
};

// -----------------------------------------------------------------------

#define CARLA_REGISTER_NATIVE_PLUGIN_MM(label, descMM) \
    void carla_register_native_plugin_##label () { carla_register_native_plugin(descMM.descriptorInit()); }

#endif // CARLA_NATIVE_MM_H

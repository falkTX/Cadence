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

#ifndef CARLA_NATIVE_HPP
#define CARLA_NATIVE_HPP

#include "carla_native.h"
#include "carla_utils.hpp"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

class PluginDescriptorClass {
public:
    PluginDescriptorClass(const HostDescriptor* host)
    {
        this->host = host;
    }

    virtual ~PluginDescriptorClass()
    {
    }

    // -------------------------------------------------------------------
    // Host calls

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

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    virtual uint32_t getParameterCount()
    {
        return 0;
    }

    virtual const Parameter* getParameterInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return nullptr;
    }

    virtual float getParameterValue(uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return 0.0f;
    }

    virtual const char* getParameterText(uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    virtual uint32_t getMidiProgramCount()
    {
        return 0;
    }

    virtual const MidiProgram* getMidiProgramInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    virtual void setParameterValue(uint32_t index, double value)
    {
        CARLA_ASSERT(index < getParameterCount());
        Q_UNUSED(value);
    }

    virtual void setMidiProgram(uint32_t bank, uint32_t program)
    {
        Q_UNUSED(bank);
        Q_UNUSED(program);
    }

    virtual void setCustomData(const char* key, const char* value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    virtual void showGui(bool show)
    {
        Q_UNUSED(show);
    }

    virtual void idleGui()
    {
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    virtual void activate()
    {
    }

    virtual void deactivate()
    {
    }

    virtual void process(float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents) = 0;

    // -------------------------------------------------------------------

private:
    const HostDescriptor* host;

    // -------------------------------------------------------------------

#ifndef DOXYGEN
public:
    static uint32_t _get_parameter_count(PluginHandle handle)
    {
        return ((PluginDescriptorClass*)handle)->getParameterCount();
    }

    static const Parameter* _get_parameter_info(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterInfo(index);
    }

    static float _get_parameter_value(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterValue(index);
    }

    static const char* _get_parameter_text(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getParameterText(index);
    }

    static uint32_t _get_midi_program_count(PluginHandle handle)
    {
        return ((PluginDescriptorClass*)handle)->getMidiProgramCount();
    }

    static const MidiProgram* _get_midi_program_info(PluginHandle handle, uint32_t index)
    {
        return ((PluginDescriptorClass*)handle)->getMidiProgramInfo(index);
    }

    static void _set_parameter_value(PluginHandle handle, uint32_t index, float value)
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

    static void _activate(PluginHandle handle)
    {
        ((PluginDescriptorClass*)handle)->activate();
    }

    static void _deactivate(PluginHandle handle)
    {
        ((PluginDescriptorClass*)handle)->deactivate();
    }

    static void _process(PluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        return ((PluginDescriptorClass*)handle)->process(inBuffer, outBuffer, frames, midiEventCount, midiEvents);
    }
#endif
};

/**@}*/

// -----------------------------------------------------------------------

#define PluginDescriptorClassEND(CLASS)                                               \
public:                                                                               \
    static PluginHandle _instantiate(struct _PluginDescriptor*, HostDescriptor* host) \
    {                                                                                 \
        return new CLASS(host);                                                       \
    }                                                                                 \
    static void _cleanup(PluginHandle handle)                                         \
    {                                                                                 \
        delete (CLASS*)handle;                                                        \
    }

#define PluginDescriptorFILL(CLASS) \
    CLASS::_instantiate,            \
    CLASS::_get_parameter_count,    \
    CLASS::_get_parameter_info,     \
    CLASS::_get_parameter_value,    \
    CLASS::_get_parameter_text,     \
    CLASS::_get_midi_program_count, \
    CLASS::_get_midi_program_info,  \
    CLASS::_set_parameter_value,    \
    CLASS::_set_midi_program,       \
    CLASS::_set_custom_data,        \
    CLASS::_show_gui,               \
    CLASS::_idle_gui,               \
    CLASS::_activate,               \
    CLASS::_deactivate,             \
    CLASS::_cleanup,                \
    CLASS::_process

#endif // CARLA_NATIVE_HPP

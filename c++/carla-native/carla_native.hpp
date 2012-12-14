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

class PluginDescriptorClass
{
public:
    PluginDescriptorClass(const HostDescriptor* const host)
        : m_host(host)
    {
    }

    virtual ~PluginDescriptorClass()
    {
    }

    // -------------------------------------------------------------------
    // Host calls

    const HostDescriptor* getHostHandle() const
    {
        return m_host;
    }

    uint32_t getBufferSize() const
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            return m_host->get_buffer_size(m_host->handle);

        return 0;
    }

    double getSampleRate() const
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            return m_host->get_sample_rate(m_host->handle);

        return 0.0;
    }

    const TimeInfo* getTimeInfo() const
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            return m_host->get_time_info(m_host->handle);

        return nullptr;
    }

    void writeMidiEvent(MidiEvent* const event)
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            m_host->write_midi_event(m_host->handle, event);
    }

    void uiParameterChanged(const uint32_t index, const float value)
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            m_host->ui_parameter_changed(m_host->handle, index, value);
    }

    void uiMidiProgramChanged(const uint32_t bank, const uint32_t program)
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            m_host->ui_midi_program_changed(m_host->handle, bank, program);
    }

    void uiCustomDataChanged(const char* const key, const char* const value)
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            m_host->ui_custom_data_changed(m_host->handle, key, value);
    }

    void uiClosed()
    {
        CARLA_ASSERT(m_host);

        if (m_host)
            m_host->ui_closed(m_host->handle);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    virtual uint32_t getParameterCount()
    {
        return 0;
    }

    virtual const Parameter* getParameterInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return nullptr;
    }

    virtual float getParameterValue(const uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        return 0.0f;
    }

    virtual const char* getParameterText(const uint32_t index)
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

    virtual const MidiProgram* getMidiProgramInfo(const uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        return nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    virtual void setParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());
        Q_UNUSED(value);
    }

    virtual void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        Q_UNUSED(bank);
        Q_UNUSED(program);
    }

    virtual void setCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    virtual void uiShow(const bool show)
    {
        Q_UNUSED(show);
    }

    virtual void uiIdle()
    {
    }

    virtual void uiSetParameterValue(const uint32_t index, const float value)
    {
        CARLA_ASSERT(index < getParameterCount());
        Q_UNUSED(value);
    }

    virtual void uiSetMidiProgram(const uint32_t bank, const uint32_t program)
    {
        Q_UNUSED(bank);
        Q_UNUSED(program);
    }

    virtual void uiSetCustomData(const char* const key, const char* const value)
    {
        CARLA_ASSERT(key);
        CARLA_ASSERT(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    virtual void activate()
    {
    }

    virtual void deactivate()
    {
    }

    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents) = 0;

    // -------------------------------------------------------------------

private:
    const HostDescriptor* const m_host;

    // -------------------------------------------------------------------

#ifndef DOXYGEN
public:
    #define handlePtr ((PluginDescriptorClass*)handle)

    static uint32_t _get_parameter_count(PluginHandle handle)
    {
        return handlePtr->getParameterCount();
    }

    static const Parameter* _get_parameter_info(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterInfo(index);
    }

    static float _get_parameter_value(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterValue(index);
    }

    static const char* _get_parameter_text(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getParameterText(index);
    }

    static uint32_t _get_midi_program_count(PluginHandle handle)
    {
        return handlePtr->getMidiProgramCount();
    }

    static const MidiProgram* _get_midi_program_info(PluginHandle handle, uint32_t index)
    {
        return handlePtr->getMidiProgramInfo(index);
    }

    static void _set_parameter_value(PluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->setParameterValue(index, value);
    }

    static void _set_midi_program(PluginHandle handle, uint32_t bank, uint32_t program)
    {
        return handlePtr->setMidiProgram(bank, program);
    }

    static void _set_custom_data(PluginHandle handle, const char* key, const char* value)
    {
        return handlePtr->setCustomData(key, value);
    }

    static void _ui_show(PluginHandle handle, bool show)
    {
        return handlePtr->uiShow(show);
    }

    static void _ui_idle(PluginHandle handle)
    {
        return handlePtr->uiIdle();
    }

    static void _ui_set_parameter_value(PluginHandle handle, uint32_t index, float value)
    {
        return handlePtr->uiSetParameterValue(index, value);
    }

    static void _ui_set_midi_program(PluginHandle handle, uint32_t bank, uint32_t program)
    {
        return handlePtr->uiSetMidiProgram(bank, program);
    }

    static void _ui_set_custom_data(PluginHandle handle, const char* key, const char* value)
    {
        return handlePtr->uiSetCustomData(key, value);
    }

    static void _activate(PluginHandle handle)
    {
        handlePtr->activate();
    }

    static void _deactivate(PluginHandle handle)
    {
        handlePtr->deactivate();
    }

    static void _process(PluginHandle handle, float** inBuffer, float** outBuffer, const uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
    {
        return handlePtr->process(inBuffer, outBuffer, frames, midiEventCount, midiEvents);
    }
#endif
};

/**@}*/

// -----------------------------------------------------------------------

#define PluginDescriptorClassEND(CLASS)                                             \
public:                                                                             \
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host) \
    {                                                                               \
        return new CLASS(host);                                                     \
    }                                                                               \
    static void _cleanup(PluginHandle handle)                                       \
    {                                                                               \
        delete (CLASS*)handle;                                                      \
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
    CLASS::_ui_show,                \
    CLASS::_ui_idle,                \
    CLASS::_ui_set_parameter_value, \
    CLASS::_ui_set_midi_program,    \
    CLASS::_ui_set_custom_data,     \
    CLASS::_activate,               \
    CLASS::_deactivate,             \
    CLASS::_cleanup,                \
    CLASS::_process

#endif // CARLA_NATIVE_HPP

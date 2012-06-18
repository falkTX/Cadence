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

#ifndef CARLA_PLUGIN_H
#define CARLA_PLUGIN_H

#include "carla_engine.h"
#include "carla_midi.h"
#include "carla_shared.h"

#include "carla_lib_includes.h"

#ifdef BUILD_BRIDGE
#include "carla_bridge_osc.h"
#else
#include "carla_osc.h"
#include "carla_threads.h"
#endif

// common includes
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <QtCore/QMutex>
#include <QtCore/QString>

#ifdef __WINE__
typedef HWND GuiDataHandle;
#else
#include <QtGui/QDialog>
typedef QDialog* GuiDataHandle;
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

#define CARLA_PROCESS_CONTINUE_CHECK if (! m_enabled) { return callback_action(CALLBACK_DEBUG, m_id, m_enabled, 0, 0.0); }

const unsigned short MAX_MIDI_EVENTS = 512;
const unsigned short MAX_POST_EVENTS = 152;

enum PluginPostEventType {
    PluginPostEventNull,
    PluginPostEventDebug,
    PluginPostEventParameterChange,
    PluginPostEventProgramChange,
    PluginPostEventMidiProgramChange,
    PluginPostEventNoteOn,
    PluginPostEventNoteOff,
    PluginPostEventCustom
};

#ifndef BUILD_BRIDGE
enum PluginBridgeInfoType {
    PluginBridgeAudioCount,
    PluginBridgeMidiCount,
    PluginBridgeParameterCount,
    PluginBridgeProgramCount,
    PluginBridgeMidiProgramCount,
    PluginBridgePluginInfo,
    PluginBridgeParameterInfo,
    PluginBridgeParameterDataInfo,
    PluginBridgeParameterRangesInfo,
    PluginBridgeProgramInfo,
    PluginBridgeMidiProgramInfo,
    PluginBridgeCustomData,
    PluginBridgeChunkData,
    PluginBridgeUpdateNow,
    PluginBridgeSaved
};
#endif

struct midi_program_t {
    uint32_t bank;
    uint32_t program;
    const char* name;
};

struct PluginAudioData {
    uint32_t count;
    uint32_t* rindexes;
    CarlaEngineAudioPort** ports;
};

struct PluginMidiData {
    CarlaEngineMidiPort* portMin;
    CarlaEngineMidiPort* portMout;
};

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    CarlaEngineControlPort* portCin;
    CarlaEngineControlPort* portCout;
};

struct PluginProgramData {
    uint32_t count;
    int32_t current;
    const char** names;
};

struct PluginMidiProgramData {
    uint32_t count;
    int32_t current;
    midi_program_t* data;
};

struct PluginPostEvent {
    PluginPostEventType type;
    int32_t index;
    double value;
    const void* cdata;
};

struct ExternalMidiNote {
    bool valid;
    uint8_t note;
    uint8_t velo;
};

class CarlaPlugin
{
public:
    CarlaPlugin(unsigned short id)
    {
        qDebug("CarlaPlugin::CarlaPlugin()");

        m_type  = PLUGIN_NONE;
        m_id    = id;
        m_hints = 0;

        m_active = false;
        m_activeBefore = false;
        m_enabled = false;

        m_lib  = nullptr;
        m_name = nullptr;
        m_filename = nullptr;

        cin_channel = 0;

        x_drywet = 1.0;
        x_vol    = 1.0;
        x_bal_left = -1.0;
        x_bal_right = 1.0;
        x_client = nullptr;

        ain.count = 0;
        ain.ports = nullptr;
        ain.rindexes = nullptr;

        aout.count = 0;
        aout.ports = nullptr;
        aout.rindexes = nullptr;

        midi.portMin  = nullptr;
        midi.portMout = nullptr;

        param.count  = 0;
        param.data   = nullptr;
        param.ranges = nullptr;
        param.portCin  = nullptr;
        param.portCout = nullptr;

        prog.count   = 0;
        prog.current = -1;
        prog.names   = nullptr;

        midiprog.count   = 0;
        midiprog.current = -1;
        midiprog.data    = nullptr;

#ifndef BUILD_BRIDGE
        osc.data.path   = nullptr;
        osc.data.source = nullptr;
        osc.data.target = nullptr;
        osc.thread = nullptr;
#endif

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            postEvents.data[i].type = PluginPostEventNull;

        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
            extMidiNotes[i].valid = false;
    }

    virtual ~CarlaPlugin()
    {
        qDebug("CarlaPlugin::~CarlaPlugin()");

        // Remove client and ports
        if (x_client)
        {
            if (x_client->isActive())
                x_client->deactivate();

            remove_client_ports();
            delete x_client;
        }

        // Delete data
        delete_buffers();

        // Unload DLL
        lib_close();

        if (m_name)
            free((void*)m_name);

        if (m_filename)
            free((void*)m_filename);

        if (prog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
            {
                if (prog.names[i])
                    free((void*)prog.names[i]);
            }
            delete[] prog.names;
        }

        if (midiprog.count > 0)
        {
            for (uint32_t i=0; i < midiprog.count; i++)
            {
                if (midiprog.data[i].name)
                    free((void*)midiprog.data[i].name);
            }
            delete[] midiprog.data;
        }

        if (custom.size() > 0)
        {
            for (size_t i=0; i < custom.size(); i++)
            {
                if (custom[i].key)
                    free((void*)custom[i].key);

                if (custom[i].value)
                    free((void*)custom[i].value);
            }
            custom.clear();
        }
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginType type() const
    {
        return m_type;
    }

    unsigned short id() const
    {
        return m_id;
    }

    unsigned int hints() const
    {
        return m_hints;
    }

    bool enabled() const
    {
        return m_enabled;
    }

    const char* name() const
    {
        return m_name;
    }

    const char* filename() const
    {
        return m_filename;
    }

    virtual PluginCategory category()
    {
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long uniqueId()
    {
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    virtual uint32_t ainCount()
    {
        return ain.count;
    }

    virtual uint32_t aoutCount()
    {
        return aout.count;
    }

    virtual uint32_t minCount()
    {
        return midi.portMin ? 1 : 0;
    }

    virtual uint32_t moutCount()
    {
        return midi.portMout ? 1 : 0;
    }

    uint32_t paramCount() const
    {
        return param.count;
    }

    virtual uint32_t paramScalePointCount(uint32_t paramId)
    {
        assert(paramId < param.count);
        return 0;
    }

    uint32_t progCount() const
    {
        return prog.count;
    }

    uint32_t midiprogCount() const
    {
        return midiprog.count;
    }

    uint32_t customCount() const
    {
        return custom.size();
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t progCurrent() const
    {
        return prog.current;
    }

    int32_t midiprogCurrent() const
    {
        return midiprog.current;
    }

    const ParameterData* paramData(uint32_t index) const
    {
        assert(index < param.count);
        return &param.data[index];
    }

    const ParameterRanges* paramRanges(uint32_t index) const
    {
        assert(index < param.count);
        return &param.ranges[index];
    }

    const CustomData* customData(uint32_t index) const
    {
        assert(index < custom.size());
        return &custom[index];
    }

    virtual int32_t chunkData(void** dataPtr)
    {
        assert(dataPtr);
        return 0;
    }

#ifndef BUILD_BRIDGE
    const OscData* oscData() const
    {
        return &osc.data;
    }
#endif

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    virtual double getParameterValue(uint32_t paramId)
    {
        assert(paramId < param.count);
        return 0.0;
    }

    virtual double getParameterScalePointValue(uint32_t paramId, uint32_t scalepointId)
    {
        assert(paramId < param.count);
        assert(scalepointId < paramScalePointCount(paramId));
        return 0.0;
    }

    virtual void getLabel(char* strBuf)
    {
        *strBuf = 0;
    }

    virtual void getMaker(char* strBuf)
    {
        *strBuf = 0;
    }

    virtual void getCopyright(char* strBuf)
    {
        *strBuf = 0;
    }

    virtual void getRealName(char* strBuf)
    {
        *strBuf = 0;;
    }

    virtual void getParameterName(uint32_t paramId, char* strBuf)
    {
        assert(paramId < param.count);
        *strBuf = 0;
    }

    virtual void getParameterSymbol(uint32_t paramId, char* strBuf)
    {
        assert(paramId < param.count);
        *strBuf = 0;
    }

    virtual void getParameterText(uint32_t paramId, char* strBuf)
    {
        assert(paramId < param.count);
        *strBuf = 0;
    }

    virtual void getParameterUnit(uint32_t paramId, char* strBuf)
    {
        assert(paramId < param.count);
        *strBuf = 0;
    }

    virtual void getParameterScalePointLabel(uint32_t paramId, uint32_t scalePointId, char* strBuf)
    {
        assert(paramId < param.count);
        assert(scalePointId < paramScalePointCount(paramId));
        *strBuf = 0;
    }

    void getProgramName(uint32_t index, char* strBuf)
    {
        assert(index < prog.count);
        strncpy(strBuf, prog.names[index], STR_MAX);
    }

    void getMidiProgramName(uint32_t index, char* strBuf)
    {
        assert(index < midiprog.count);
        strncpy(strBuf, midiprog.data[index].name, STR_MAX);
    }

    void getParameterCountInfo(PortCountInfo* const info)
    {
        info->ins   = 0;
        info->outs  = 0;
        info->total = param.count;

        for (uint32_t i=0; i < param.count; i++)
        {
            if (param.data[i].type == PARAMETER_INPUT)
                info->ins += 1;
            else if (param.data[i].type == PARAMETER_OUTPUT)
                info->outs += 1;
        }
    }

    void getMidiProgramInfo(MidiProgramInfo* const info, uint32_t index)
    {
        assert(index < midiprog.count);
        info->bank    = midiprog.data[index].bank;
        info->program = midiprog.data[index].program;
        info->label   = midiprog.data[index].name;
    }

    virtual void getGuiInfo(GuiInfo* const info)
    {
        info->type = GUI_NONE;
        info->resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    void setActive(bool active, bool sendOsc, bool sendCallback)
    {
        m_active = active;

        double value = active ? 1.0 : 0.0;

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_ACTIVE, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, PARAMETER_ACTIVE, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_ACTIVE, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
    }

    void setDryWet(double value, bool sendOsc, bool sendCallback)
    {
        if (value < 0.0)
            value = 0.0;
        else if (value > 1.0)
            value = 1.0;

        x_drywet = value;

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_DRYWET, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, PARAMETER_DRYWET, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_DRYWET, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
    }

    void setVolume(double value, bool sendOsc, bool sendCallback)
    {
        if (value < 0.0)
            value = 0.0;
        else if (value > 1.27)
            value = 1.27;

        x_vol = value;

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_VOLUME, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, PARAMETER_VOLUME, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_VOLUME, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
    }

    void setBalanceLeft(double value, bool sendOsc, bool sendCallback)
    {
        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_bal_left = value;

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_BALANCE_LEFT, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, PARAMETER_BALANCE_LEFT, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
    }

    void setBalanceRight(double value, bool sendOsc, bool sendCallback)
    {
        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_bal_right = value;

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, PARAMETER_BALANCE_RIGHT, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, PARAMETER_BALANCE_RIGHT, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
    }

#ifndef BUILD_BRIDGE
    virtual int set_osc_bridge_info(PluginBridgeInfoType itype, lo_arg** argv)
    {
        return 1;
        Q_UNUSED(itype);
        Q_UNUSED(argv);
    }
#endif

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    virtual void setParameterValue(uint32_t paramId, double value, bool sendGui, bool sendOsc, bool sendCallback)
    {
        assert(paramId < param.count);
        fix_parameter_value(value, param.ranges[paramId]);

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, paramId, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, paramId, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, paramId, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
        Q_UNUSED(sendGui);
    }

    // FIXME? (better name?)
    void setParameterValueByRIndex(int32_t rindex, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        if (rindex == PARAMETER_ACTIVE)
            return setActive(value > 0.0, osc_send, callback_send);
        if (rindex == PARAMETER_DRYWET)
            return setDryWet(value, osc_send, callback_send);
        if (rindex == PARAMETER_VOLUME)
            return setVolume(value, osc_send, callback_send);
        if (rindex == PARAMETER_BALANCE_LEFT)
            return setBalanceLeft(value, osc_send, callback_send);
        if (rindex == PARAMETER_BALANCE_RIGHT)
            return setBalanceRight(value, osc_send, callback_send);

        for (uint32_t i=0; i < param.count; i++)
        {
            if (param.data[i].rindex == rindex)
                return setParameterValue(i, value, gui_send, osc_send, callback_send);
        }
    }

    void setParameterMidiChannel(uint32_t index, uint8_t channel)
    {
        assert(index < param.count && channel < 16);
        param.data[index].midi_channel = channel;

#ifndef BUILD_BRIDGE
        // FIXME
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_channel(&osc.data, m_id, index, channel);
#endif
    }

    void setParameterMidiCC(uint32_t index, int16_t midiCC)
    {
        assert(index < param.count);
        param.data[index].midi_cc = midiCC;

#ifndef BUILD_BRIDGE
        // FIXME
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_cc(&osc.data, m_id, index, midi_cc);
#endif
    }

    virtual void setCustomData(CustomDataType type, const char* key, const char* value, bool sendGui)
    {
        qDebug("setCustomData(%i, %s, %s, %s)", type, key, value, bool2str(sendGui));

        bool save_data = true;

        switch (type)
        {
        case CUSTOM_DATA_INVALID:
            save_data = false;
            break;
        case CUSTOM_DATA_STRING:
            // Ignore some keys
            if (strncmp(key, "OSC:", 4) == 0 || strcmp(key, "guiVisible") || strcmp(key, "CarlaBridgeSaveNow") == 0)
                save_data = false;
            break;
        default:
            break;
        }

        if (save_data)
        {
            assert(key);
            assert(value);

            // Check if we already have this key
            for (size_t i=0; i < custom.size(); i++)
            {
                if (strcmp(custom[i].key, key) == 0)
                {
                    free((void*)custom[i].value);
                    custom[i].value = strdup(value);
                    return;
                }
            }

            // False if we get here, so store it
            CustomData new_data;
            new_data.type  = type;
            new_data.key   = strdup(key);
            new_data.value = strdup(value);
            custom.push_back(new_data);
        }
    }

    virtual void setChunkData(const char* stringData)
    {
        assert(stringData);
    }

    virtual void setProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        assert(index < (int32_t)prog.count);
        prog.current = index;

        if (sendCallback)
            callback_action(CALLBACK_PROGRAM_CHANGED, m_id, prog.current, 0, 0.0);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_program(m_id, prog.current);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_program(&osc.data, prog.current);
        }
#endif

        // Change default parameter values
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = getParameterValue(i);

#ifndef BUILD_BRIDGE
            if (sendOsc)
                osc_global_send_set_default_value(m_id, i, param.ranges[i].def);
#endif
        }

#ifdef BUILD_BRIDGE
        Q_UNUSED(sendOsc);
#endif
        Q_UNUSED(sendGui);
        Q_UNUSED(block);
    }

    virtual void setMidiProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        assert(index < (int32_t)midiprog.count);
        midiprog.current = index;

        if (sendCallback)
            callback_action(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, midiprog.current, 0, 0.0);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_midi_program(m_id, midiprog.current);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_program(&osc.data, midiprog.current);
        }
#endif

        // Sound banks never change defaults
        if (m_type == PLUGIN_GIG || m_type == PLUGIN_SF2 || m_type == PLUGIN_SFZ)
            return;

        // Change default parameter values
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = getParameterValue(i);

#ifndef BUILD_BRIDGE
            if (sendOsc)
                osc_global_send_set_default_value(m_id, i, param.ranges[i].def);
#endif
        }

#ifdef BUILD_BRIDGE
        Q_UNUSED(sendOsc);
#endif
        Q_UNUSED(sendGui);
        Q_UNUSED(block);
    }

    void setMidiProgramById(uint32_t bank, uint32_t program, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        for (uint32_t i=0; i < midiprog.count; i++)
        {
            if (midiprog.data[i].bank == bank && midiprog.data[i].program == program)
                return setMidiProgram(i, sendGui, sendOsc, sendCallback, block);
        }
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    virtual void setGuiData(int data, GuiDataHandle handle)
    {
        Q_UNUSED(data);
        Q_UNUSED(handle);
    }

    virtual void showGui(bool yesNo)
    {
        Q_UNUSED(yesNo);
    }

    virtual void idleGui()
    {
    }

    // -------------------------------------------------------------------
    // Plugin state

    virtual void reload()
    {
    }

    virtual void reloadPrograms(bool init)
    {
        Q_UNUSED(init);
    }

    virtual void prepareForSave()
    {
    }

    // -------------------------------------------------------------------
    // Plugin processing

    virtual void process(float** ains_buffer, float** aouts_buffer, uint32_t nframes, uint32_t nframesOffset = 0)
    {
        Q_UNUSED(ains_buffer);
        Q_UNUSED(aouts_buffer);
        Q_UNUSED(nframes);
        Q_UNUSED(nframesOffset);
    }

#ifdef CARLA_ENGINE_JACK
    void process_jack(uint32_t nframes)
    {
        float* ains_buffer[ain.count];
        float* aouts_buffer[aout.count];

        for (uint32_t i=0; i < ain.count; i++)
            ains_buffer[i] = (float*)ain.ports[i]->getBuffer();

        for (uint32_t i=0; i < aout.count; i++)
            aouts_buffer[i] = (float*)aout.ports[i]->getBuffer();

#ifndef BUILD_BRIDGE
        if (carla_options.proccess_hq)
        {
            float* ains_buffer2[ain.count];
            float* aouts_buffer2[aout.count];

            for (uint32_t i=0, j; i < nframes; i += 8)
            {
                for (j=0; j < ain.count; j++)
                    ains_buffer2[j] = ains_buffer[j] + i;

                for (j=0; j < aout.count; j++)
                    aouts_buffer2[j] = aouts_buffer[j] + i;

                process(ains_buffer2, aouts_buffer2, 8, i);
            }
        }
        else
#endif
            process(ains_buffer, aouts_buffer, nframes);
    }
#endif

    virtual void bufferSizeChanged(uint32_t newBufferSize)
    {
        Q_UNUSED(newBufferSize);
    }

    // -------------------------------------------------------------------
    // OSC stuff

    void osc_register_new()
    {
#ifdef BUILD_BRIDGE
        // Base data
        {
            char bufName[STR_MAX]  = { 0 };
            char bufLabel[STR_MAX] = { 0 };
            char bufMaker[STR_MAX] = { 0 };
            char bufCopyright[STR_MAX] = { 0 };
            getRealName(bufName);
            getLabel(bufLabel);
            getMaker(bufMaker);
            getCopyright(bufCopyright);
            osc_send_bridge_plugin_info(category(), m_hints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
        }

        osc_send_bridge_audio_count(ainCount(), aoutCount(), ainCount() + aoutCount());
        osc_send_bridge_midi_count(minCount(), moutCount(), minCount() + moutCount());

        PortCountInfo param_info = { false, 0, 0, 0 };
        getParameterCountInfo(&param_info);
        osc_send_bridge_param_count(param_info.ins, param_info.outs, param_info.total);

        // Parameters
        uint32_t i;
        char bufName[STR_MAX], bufUnit[STR_MAX];

        if (param.count > 0 && param.count < MAX_PARAMETERS)
        {
            for (i=0; i < param.count; i++)
            {
                getParameterName(i, bufName);
                getParameterUnit(i, bufUnit);
                osc_send_bridge_param_info(i, bufName, bufUnit);
                osc_send_bridge_param_data(param.data[i].type, i, param.data[i].rindex, param.data[i].hints, param.data[i].midi_channel, param.data[i].midi_cc);
                osc_send_bridge_param_ranges(i, param.ranges[i].def, param.ranges[i].min, param.ranges[i].max, param.ranges[i].step, param.ranges[i].step_small, param.ranges[i].step_large);

                setParameterValue(i, param.ranges[i].def, false, false, true);
            }
        }

        // Programs
        osc_send_bridge_program_count(prog.count);

        for (i=0; i < prog.count; i++)
            osc_send_bridge_program_info(i, prog.names[i]);

        if (prog.current >= 0)
            osc_send_program(prog.current);

        // MIDI Programs
        osc_send_bridge_midi_program_count(midiprog.count);

        for (i=0; i < midiprog.count; i++)
            osc_send_bridge_midi_program_info(i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

        if (midiprog.current >= 0 && midiprog.count > 0)
            osc_send_midi_program(midiprog.data[midiprog.current].bank, midiprog.data[midiprog.current].program, false);
#else
        if (osc_global_registered())
        {
            // Base data
            osc_global_send_add_plugin(m_id, m_name);

            const PluginInfo* const info = get_plugin_info(m_id);
            osc_global_send_set_plugin_data(m_id, m_type, category(), m_hints, get_real_plugin_name(m_id), info->label, info->maker, info->copyright, uniqueId());

            PortCountInfo param_info = { false, 0, 0, 0 };
            get_parameter_count_info(&param_info);
            osc_global_send_set_plugin_ports(m_id, ain.count, aout.count, minCount(), moutCount(), param_info.ins, param_info.outs, param_info.total);

            // Parameters
            osc_global_send_set_parameter_value(m_id, PARAMETER_ACTIVE, m_active ? 1.0f : 0.0f);
            osc_global_send_set_parameter_value(m_id, PARAMETER_DRYWET, x_drywet);
            osc_global_send_set_parameter_value(m_id, PARAMETER_VOLUME, x_vol);
            osc_global_send_set_parameter_value(m_id, PARAMETER_BALANCE_LEFT, x_bal_left);
            osc_global_send_set_parameter_value(m_id, PARAMETER_BALANCE_RIGHT, x_bal_right);

            uint32_t i;

            if (param.count > 0 && param.count < carla_options.max_parameters)
            {
                for (i=0; i < param.count; i++)
                {
                    const ParameterInfo* const info = get_parameter_info(m_id, i);

                    osc_global_send_set_parameter_data(m_id, i, param.data[i].type, param.data[i].hints, info->name, info->unit, getParameterValue(i));
                    osc_global_send_set_parameter_ranges(m_id, i, param.ranges[i].min, param.ranges[i].max, param.ranges[i].def, param.ranges[i].step, param.ranges[i].step_small, param.ranges[i].step_large);
                }
            }

            // Programs
            osc_global_send_set_program_count(m_id, prog.count);

            for (i=0; i < prog.count; i++)
                osc_global_send_set_program_name(m_id, i, prog.names[i]);

            osc_global_send_set_program(m_id, prog.current);

            // MIDI Programs
            osc_global_send_set_midi_program_count(m_id, midiprog.count);

            for (i=0; i < midiprog.count; i++)
                osc_global_send_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

            osc_global_send_set_midi_program(m_id, midiprog.current);
        }
#endif
    }

#ifndef BUILD_BRIDGE
    void update_osc_data(lo_address source, const char* url)
    {
        const char* host;
        const char* port;

        osc_clear_data(&osc.data);

        host = lo_address_get_hostname(source);
        port = lo_address_get_port(source);
        osc.data.source = lo_address_new(host, port);

        host = lo_url_get_hostname(url);
        port = lo_url_get_port(url);
        osc.data.path   = lo_url_get_path(url);
        osc.data.target = lo_address_new(host, port);

        free((void*)host);
        free((void*)port);

        for (size_t i=0; i < custom.size(); i++)
        {
            if (m_type == PLUGIN_LV2)
                osc_send_lv2_event_transfer(&osc.data, customdatatype2str(custom[i].type), custom[i].key, custom[i].value);
            else if (custom[i].type == CUSTOM_DATA_STRING)
                osc_send_configure(&osc.data, custom[i].key, custom[i].value);
        }

        if (prog.current >= 0)
            osc_send_program(&osc.data, prog.current);

        if (midiprog.current >= 0)
        {
            int32_t id = midiprog.current;
            osc_send_midi_program(&osc.data, midiprog.data[id].bank, midiprog.data[id].program, (m_type == PLUGIN_DSSI));
        }

        for (uint32_t i=0; i < param.count; i++)
            osc_send_control(&osc.data, param.data[i].rindex, getParameterValue(i));

        if (m_hints & PLUGIN_IS_BRIDGE)
        {
            osc_send_control(&osc.data, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
            osc_send_control(&osc.data, PARAMETER_DRYWET, x_drywet);
            osc_send_control(&osc.data, PARAMETER_VOLUME, x_vol);
            osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, x_bal_left);
            osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, x_bal_right);
        }
    }

    void clear_osc_data()
    {
        osc_clear_data(&osc.data);
    }

    bool show_osc_gui()
    {
        // wait for UI 'update' call
        for (int i=0; i < carla_options.osc_gui_timeout; i++)
        {
            if (osc.data.target)
            {
                osc_send_show(&osc.data);
                return true;
            }
            else
                carla_msleep(100);
        }
        return false;
    }
#endif

    // -------------------------------------------------------------------
    // MIDI events

    virtual void send_midi_note(uint8_t note, uint8_t velo, bool gui_send, bool osc_send, bool callback_send)
    {
        carla_midi_lock();
        for (unsigned short i=0; i<MAX_MIDI_EVENTS; i++)
        {
            if (extMidiNotes[i].valid == false)
            {
                extMidiNotes[i].valid = true;
                extMidiNotes[i].note  = note;
                extMidiNotes[i].velo  = velo;
                break;
            }
        }
        carla_midi_unlock();

#ifndef BUILD_BRIDGE
        if (osc_send)
        {
            if (velo)
                osc_global_send_note_on(m_id, note, velo);
            else
                osc_global_send_note_off(m_id, note);

            if (m_hints & PLUGIN_IS_BRIDGE)
            {
                uint8_t mdata[4] = { 0 };
                mdata[1] = velo ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                mdata[2] = note;
                mdata[3] = velo;

                osc_send_midi(&osc.data, mdata);
            }
        }

        if (callback_send)
            callback_action(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, m_id, note, velo, 0.0);
#else
        Q_UNUSED(osc_send);
        Q_UNUSED(callback_send);
#endif
        Q_UNUSED(gui_send);
    }

    void send_midi_all_notes_off()
    {
        carla_midi_lock();
        postEvents.mutex.lock();

        unsigned short pe_pad = 0;

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
        {
            if (postEvents.data[i].type == PluginPostEventNull)
            {
                pe_pad = i;
                break;
            }
            else if (i + MAX_POST_EVENTS == MAX_MIDI_EVENTS)
            {
                qWarning("post-events buffer full, making room for all notes off now");
                pe_pad = i - 1;
                break;
            }
        }

        for (unsigned short i=0; i < 128; i++)
        {
            extMidiNotes[i].valid = true;
            extMidiNotes[i].note  = i;
            extMidiNotes[i].velo  = 0;

            postEvents.data[i+pe_pad].type  = PluginPostEventNoteOff;
            postEvents.data[i+pe_pad].index = i;
            postEvents.data[i+pe_pad].value = 0.0;
        }

        postEvents.mutex.unlock();
        carla_midi_unlock();
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void postpone_event(PluginPostEventType type, int32_t index, double value, const void* cdata = nullptr)
    {
        postEvents.mutex.lock();

        for (unsigned short i=0; i<MAX_POST_EVENTS; i++)
        {
            if (postEvents.data[i].type == PluginPostEventNull)
            {
                postEvents.data[i].type  = type;
                postEvents.data[i].index = index;
                postEvents.data[i].value = value;
                postEvents.data[i].cdata = cdata;
                break;
            }
        }

        postEvents.mutex.unlock();
    }

    void post_events_copy(PluginPostEvent* postEventsDest)
    {
        postEvents.mutex.lock();

        memcpy(postEventsDest, postEvents.data, sizeof(PluginPostEvent)*MAX_POST_EVENTS);

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            postEvents.data[i].type = PluginPostEventNull;

        postEvents.mutex.unlock();
    }

    virtual void run_custom_event(PluginPostEvent* event)
    {
        Q_UNUSED(event);
    }

    // -------------------------------------------------------------------
    // Cleanup

    virtual void remove_client_ports()
    {
        qDebug("CarlaPlugin::remove_client_ports() - start");

        for (uint32_t i=0; i < ain.count; i++)
        {
            delete ain.ports[i];
            ain.ports[i] = nullptr;
        }

        for (uint32_t i=0; i < aout.count; i++)
        {
            delete aout.ports[i];
            aout.ports[i] = nullptr;
        }

        if (midi.portMin)
        {
            delete midi.portMin;
            midi.portMin = nullptr;
        }

        if (midi.portMout)
        {
            delete midi.portMout;
            midi.portMout = nullptr;
        }

        if (param.portCin)
        {
            delete param.portCin;
            param.portCin = nullptr;
        }

        if (param.portCout)
        {
            delete param.portCout;
            param.portCout = nullptr;
        }

        qDebug("CarlaPlugin::remove_client_ports() - end");
    }

    virtual void delete_buffers()
    {
        qDebug("CarlaPlugin::delete_buffers() - start");

        if (ain.count > 0)
        {
            delete[] ain.ports;
            delete[] ain.rindexes;
        }

        if (aout.count > 0)
        {
            delete[] aout.ports;
            delete[] aout.rindexes;
        }

        if (param.count > 0)
        {
            delete[] param.data;
            delete[] param.ranges;
        }

        ain.count = 0;
        ain.ports = nullptr;
        ain.rindexes = nullptr;

        aout.count = 0;
        aout.ports = nullptr;
        aout.rindexes = nullptr;

        midi.portMin  = nullptr;
        midi.portMout = nullptr;

        param.count    = 0;
        param.data     = nullptr;
        param.ranges   = nullptr;
        param.portCin  = nullptr;
        param.portCout = nullptr;

        qDebug("CarlaPlugin::delete_buffers() - end");
    }

    // -------------------------------------------------------------------
    // Library functions

    bool lib_open(const char* filename)
    {
        m_lib = ::lib_open(filename);
        return bool(m_lib);
    }

    bool lib_close()
    {
        if (m_lib)
            return ::lib_close(m_lib);
        return false;
    }

    void* lib_symbol(const char* symbol)
    {
        if (m_lib)
            return ::lib_symbol(m_lib, symbol);
        return nullptr;
    }

    const char* lib_error(const char* filename)
    {
        return ::lib_error(filename);
    }

    // -------------------------------------------------------------------

protected:
    PluginType m_type;
    unsigned short m_id;
    unsigned int m_hints;

    bool m_active;
    bool m_activeBefore;
    bool m_enabled;

    void* m_lib;
    const char* m_name;
    const char* m_filename;

    int8_t cin_channel;

    double x_drywet, x_vol, x_bal_left, x_bal_right;
    CarlaEngineClient* x_client;

    // -------------------------------------------------------------------
    // Storage Data

    PluginAudioData ain;
    PluginAudioData aout;
    PluginMidiData midi;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    std::vector<CustomData> custom;

    // -------------------------------------------------------------------
    // Extra

#ifndef BUILD_BRIDGE
    struct {
        OscData data;
        CarlaPluginThread* thread;
    } osc;
#endif

    struct {
        QMutex mutex;
        PluginPostEvent data[MAX_POST_EVENTS];
    } postEvents;

    ExternalMidiNote extMidiNotes[MAX_MIDI_EVENTS];

    // -------------------------------------------------------------------
    // Utilities

    static double fix_parameter_value(double& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    static float fix_parameter_value(float& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    static double abs_d(const double& value)
    {
        return (value < 0.0) ? -value : value;
    }
};

class CarlaPluginScopedDisabler
{
public:
    CarlaPluginScopedDisabler(CarlaPlugin* const plugin, bool disable = true) :
        m_plugin(plugin),
        m_disable(disable)
    {
        if (m_disable)
        {
            carla_proc_lock();
            m_plugin->setEnabled(false);
            carla_proc_unlock();
        }
    }

    ~CarlaPluginScopedDisabler()
    {
        if (m_disable)
        {
            carla_proc_lock();
            m_plugin->setEnabled(true);
            carla_proc_unlock();
        }
    }

private:
    CarlaPlugin* const m_plugin;
    const bool m_disable;
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_H

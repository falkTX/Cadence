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

#ifndef __WINE__
#include <QtGui/QDialog>
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* adjust editor indent */
#endif

/*!
 * @defgroup CarlaBackendPlugin Carla Backend Plugin
 *
 * The Carla Backend Plugin.
 * @{
 */

#define CARLA_PROCESS_CONTINUE_CHECK if (! m_enabled) { return callback_action(CALLBACK_DEBUG, m_id, m_enabled, 0, 0.0); }

#ifdef __WINE__
typedef HWND GuiDataHandle;
#else
typedef QDialog* GuiDataHandle;
#endif

const unsigned short MAX_MIDI_EVENTS = 512;
const unsigned short MAX_POST_EVENTS = 152;

const char* const CARLA_BRIDGE_MSG_HIDE_GUI   = "CarlaBridgeHideGUI";   //!< Plugin -> Host call, tells host GUI is now hidden
const char* const CARLA_BRIDGE_MSG_SAVED      = "CarlaBridgeSaved";     //!< Plugin -> Host call, tells host state is saved
const char* const CARLA_BRIDGE_MSG_SAVE_NOW   = "CarlaBridgeSaveNow";   //!< Host -> Plugin call, tells plugin to save state now
const char* const CARLA_BRIDGE_MSG_SET_CHUNK  = "CarlaBridgeSetChunk";  //!< Host -> Plugin call, tells plugin to set chunk as \a value
const char* const CARLA_BRIDGE_MSG_SET_CUSTOM = "CarlaBridgeSetCustom"; //!< Host -> Plugin call, tells plugin to set a custom data set using \a value ("type·key·value")

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

enum PluginPostEventType {
    PluginPostEventNull,
    PluginPostEventDebug,
    PluginPostEventParameterChange,
    PluginPostEventProgramChange,
    PluginPostEventMidiProgramChange,
    PluginPostEventNoteOn,
    PluginPostEventNoteOff
};

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
};

struct ExternalMidiNote {
    bool valid;
    uint8_t note;
    uint8_t velo;
};

/*!
 * \class CarlaPlugin
 *
 * \brief Carla Backend base plugin class
 *
 * This is the base class for all available plugin types available in Carla Backend.\n
 * All virtual calls are implemented in this class as fallback, so it's safe to only override needed calls.
 *
 * \see PluginType
 */
class CarlaPlugin
{
public:
    /*!
     * This is the constructor of the base plugin class.
     *
     * \param id The 'id' of this plugin, must between 0 and MAX_PLUGINS
     */
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

            removeClientPorts();
            delete x_client;
        }

        // Delete data
        deleteBuffers();

        // Unload DLL
        libClose();

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

    /*!
     * Get the plugin's type (ie, a subclass of CarlaPlugin).
     *
     * \note Plugin bridges will return their respective plugin type, there is no plugin type such as "bridge".\n
     *       To check if a plugin is a bridge use:
     * \code
     * if (hints() & PLUGIN_IS_BRIDGE)
     *     ...
     * \endcode
     */
    PluginType type() const
    {
        return m_type;
    }

    /*!
     * Get the plugin's id (as passed in the constructor).
     */
    unsigned short id() const
    {
        return m_id;
    }

    /*!
     * Get the plugin's hints.
     *
     * \see PluginHints
     */
    unsigned int hints() const
    {
        return m_hints;
    }

    /*!
     * Check if the plugin is enabled.
     *
     * \see setEnabled()
     */
    bool enabled() const
    {
        return m_enabled;
    }

    /*!
     * Get the plugin's internal name.\n
     * This name is unique within all plugins (same as getRealName() but with suffix added if needed).
     *
     * \see getRealName()
     */
    const char* name() const
    {
        return m_name;
    }

    /*!
     * Get the currently loaded DLL filename for this plugin.\n
     * (Sound kits return their exact filename).
     */
    const char* filename() const
    {
        return m_filename;
    }

    /*!
     * Get the plugin's category (delay, filter, synth, etc).
     */
    virtual PluginCategory category()
    {
        return PLUGIN_CATEGORY_NONE;
    }

    /*!
     * Get the plugin's native unique Id.\n
     * May return 0 on plugin types that don't support Ids.
     */
    virtual long uniqueId()
    {
        return 0;
    }

    // -------------------------------------------------------------------
    // Information (count)

    /*!
     * Get the number of audio inputs.
     */
    virtual uint32_t audioInCount()
    {
        return ain.count;
    }

    /*!
     * Get the number of audio outputs.
     */
    virtual uint32_t audioOutCount()
    {
        return aout.count;
    }

    /*!
     * Get the number of MIDI inputs.
     */
    virtual uint32_t midiInCount()
    {
        return midi.portMin ? 1 : 0;
    }

    /*!
     * Get the number of MIDI outputs.
     */
    virtual uint32_t midiOutCount()
    {
        return midi.portMout ? 1 : 0;
    }

    /*!
     * Get the number of parameters.\n
     * To know the number of parameter inputs and outputs, use getParameterCountInfo() instead.
     */
    uint32_t parameterCount() const
    {
        return param.count;
    }

    /*!
     * Get the number of scalepoints for parameter \a parameterId.
     */
    virtual uint32_t parameterScalePointCount(uint32_t parameterId)
    {
        assert(parameterId < param.count);
        return 0;
    }

    /*!
     * Get the number of programs.
     */
    uint32_t programCount() const
    {
        return prog.count;
    }

    /*!
     * Get the number of MIDI programs.
     */
    uint32_t midiProgramCount() const
    {
        return midiprog.count;
    }

    /*!
     * Get the number of custom data sets.
     */
    uint32_t customDataCount() const
    {
        return custom.size();
    }

    // -------------------------------------------------------------------
    // Information (current data)

    /*!
     * Get the current program number (-1 if unset).
     *
     * \see setProgram()
     */
    int32_t currentProgram() const
    {
        return prog.current;
    }

    /*!
     * Get the current MIDI program number (-1 if unset).
     *
     * \see setMidiProgram()
     * \see setMidiProgramById()
     */
    int32_t currentMidiProgram() const
    {
        return midiprog.current;
    }

    /*!
     * Get the parameter data of \a parameterId.
     */
    const ParameterData* parameterData(uint32_t parameterId) const
    {
        assert(parameterId < param.count);
        return &param.data[parameterId];
    }

    /*!
     * Get the parameter ranges of \a parameterId.
     */
    const ParameterRanges* parameterRanges(uint32_t parameterId) const
    {
        assert(parameterId < param.count);
        return &param.ranges[parameterId];
    }

    /*!
     * Get the custom data set at \a index.
     *
     * \see setCustomData()
     */
    const CustomData* customData(uint32_t index) const
    {
        assert(index < custom.size());
        return &custom[index];
    }

    /*!
     * Get the complete plugin chunk data.
     *
     * \param dataPtr TODO
     * \return The size of the chunk.
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     *
     * \see setChunkData()
     */
    virtual int32_t chunkData(void** const dataPtr)
    {
        assert(dataPtr);
        return 0;
    }

#ifndef BUILD_BRIDGE
    /*!
     * Get the plugin's internal OSC data.
     */
    const OscData* oscData() const
    {
        return &osc.data;
    }
#endif

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*!
     * Get the parameter value of \a parameterId.
     */
    virtual double getParameterValue(uint32_t parameterId)
    {
        assert(parameterId < param.count);
        return 0.0;
    }

    /*!
     * Get the scalepoint \a scalePointId value of the parameter \a parameterId.
     */
    virtual double getParameterScalePointValue(uint32_t parameterId, uint32_t scalePointId)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        return 0.0;
    }

    /*!
     * Get the plugin's label (URI for PLUGIN_LV2).
     */
    virtual void getLabel(char* const strBuf)
    {
        *strBuf = 0;
    }

    /*!
     * Get the plugin's maker.
     */
    virtual void getMaker(char* const strBuf)
    {
        *strBuf = 0;
    }

    /*!
     * Get the plugin's copyright/license.
     */
    virtual void getCopyright(char* const strBuf)
    {
        *strBuf = 0;
    }

    /*!
     * Get the plugin's (real) name.
     *
     * \see name()
     */
    virtual void getRealName(char* const strBuf)
    {
        *strBuf = 0;;
    }

    /*!
     * Get the name of the parameter \a parameterId.
     */
    virtual void getParameterName(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        *strBuf = 0;
    }

    /*!
     * Get the symbol of the parameter \a parameterId.
     */
    virtual void getParameterSymbol(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        *strBuf = 0;
    }

    /*!
     * Get the custom text of the parameter \a parameterId.
     */
    virtual void getParameterText(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        *strBuf = 0;
    }

    /*!
     * Get the unit of the parameter \a parameterId.
     */
    virtual void getParameterUnit(uint32_t parameterId, char* const strBuf)
    {
        assert(parameterId < param.count);
        *strBuf = 0;
    }

    /*!
     * Get the scalepoint \a scalePointId label of the parameter \a parameterId.
     */
    virtual void getParameterScalePointLabel(uint32_t parameterId, uint32_t scalePointId, char* const strBuf)
    {
        assert(parameterId < param.count);
        assert(scalePointId < parameterScalePointCount(parameterId));
        *strBuf = 0;
    }

    /*!
     * Get the name of the program at \a index.
     */
    void getProgramName(uint32_t index, char* const strBuf)
    {
        assert(index < prog.count);
        strncpy(strBuf, prog.names[index], STR_MAX);
    }

    /*!
     * Get the name of the MIDI program at \a index.
     *
     * \see getMidiProgramInfo()
     */
    void getMidiProgramName(uint32_t index, char* const strBuf)
    {
        assert(index < midiprog.count);
        strncpy(strBuf, midiprog.data[index].name, STR_MAX);
    }

    /*!
     * Get information about the plugin's parameter count.\n
     * This is used to check how many input, output and total parameters are available.\n
     *
     * \note Some parameters might not be input or output (ie, invalid).
     *
     * \see parameterCount()
     */
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

    /*!
     * Get information about the MIDI program at \a index.
     *
     * \see getMidiProgramName()
     */
    void getMidiProgramInfo(MidiProgramInfo* const info, uint32_t index)
    {
        assert(index < midiprog.count);
        info->bank    = midiprog.data[index].bank;
        info->program = midiprog.data[index].program;
        info->label   = midiprog.data[index].name;
    }

    /*!
     * Get information about the plugin's custom GUI, if provided.
     */
    virtual void getGuiInfo(GuiInfo* const info)
    {
        info->type = GUI_NONE;
        info->resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*!
     * Enable or disable the plugin according to \a yesNo.
     *
     * When a plugin is disabled, it will never be processed or managed in any way.\n
     * If you want to "bypass" a plugin, use setActive() instead.
     *
     * \see enabled()
     */
    void setEnabled(bool yesNo)
    {
        m_enabled = yesNo;
    }

    /*!
     * Set plugin as active according to \a active.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
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

    /*!
     * Set the plugin's dry/wet signal value to \a value.\n
     * \a value must be between 0.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
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

    /*!
     * Set the plugin's output volume to \a value.\n
     * \a value must be between 0.0 and 1.27.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
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

    /*!
     * Set the plugin's output balance-left value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
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

    /*!
     * Set the plugin's output balance-right value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
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
    /*!
     * TODO
     */
    virtual int setOscBridgeInfo(PluginBridgeInfoType type, lo_arg** const argv)
    {
        return 1;
        Q_UNUSED(type);
        Q_UNUSED(argv);
    }
#endif

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    /*!
     * Set a plugin's parameter value.\n
     * \a value must be within the parameter's range.
     *
     * \param parameterId The parameter to change
     * \param value The new parameter value
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \see getParameterValue()
     */
    virtual void setParameterValue(uint32_t parameterId, double value, bool sendGui, bool sendOsc, bool sendCallback)
    {
        assert(parameterId < param.count);

        if (sendCallback)
            callback_action(CALLBACK_PARAMETER_CHANGED, m_id, parameterId, 0, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            osc_global_send_set_parameter_value(m_id, parameterId, value);

            if (m_hints & PLUGIN_IS_BRIDGE)
                osc_send_control(&osc.data, parameterId, value);
        }
#else
        Q_UNUSED(sendOsc);
#endif
        Q_UNUSED(sendGui);
    }

    /*!
     * Set a plugin's parameter value, including internal parameters.\n
     * \a rindex can be negative to allow internal parameters change (as defined in InternalParametersIndex).
     *
     * \see setActive()
     * \see setDryWet()
     * \see setVolume()
     * \see setBalanceLeft()
     * \see setBalanceRight()
     * \see setParameterValue()
     */
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

    /*!
     * Set parameter's \a parameterId MIDI channel to \a channel.\n
     * \a channel must be between 0 and 15.
     */
    void setParameterMidiChannel(uint32_t parameterId, uint8_t channel)
    {
        assert(parameterId < param.count && channel < 16);
        param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
        // FIXME
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_channel(&osc.data, m_id, parameterId, channel);
#endif
    }

    /*!
     * Set parameter's \a parameterId MIDI CC to \a cc.\n
     * \a cc must be between 0 and 15.
     */
    void setParameterMidiCC(uint32_t parameterId, int16_t cc)
    {
        assert(parameterId < param.count);
        param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
        // FIXME
        //if (m_hints & PLUGIN_IS_BRIDGE)
        //    osc_send_set_parameter_midi_cc(&osc.data, m_id, parameterId, midi_cc);
#endif
    }

    /*!
     * Add a custom data set.\n
     * If \a key already exists, its value will be swapped with \a value.
     *
     * \param type Type of data used in \a value.
     * \param key A key identifing this data set.
     * \param value The value of the data set, of type \a type.
     *
     * \see customData()
     */
    virtual void setCustomData(CustomDataType type, const char* const key, const char* const value, bool sendGui)
    {
        qDebug("setCustomData(%i, %s, %s, %s)", type, key, value, bool2str(sendGui));
        assert(key);
        assert(value);

        bool saveData = true;

        switch (type)
        {
        case CUSTOM_DATA_INVALID:
            saveData = false;
            break;
        case CUSTOM_DATA_STRING:
            // Ignore some keys
            if (strncmp(key, "OSC:", 4) == 0 || strcmp(key, "guiVisible") || strcmp(key, "CarlaBridgeSaveNow") == 0)
                saveData = false;
            break;
        default:
            break;
        }

        if (saveData)
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
            CustomData newData;
            newData.type  = type;
            newData.key   = strdup(key);
            newData.value = strdup(value);
            custom.push_back(newData);
        }
    }

    /*!
     * Set the complete chunk data as \a stringData.
     * \a stringData must a base64 encoded string of binary data.
     *
     * \see chunkData()
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     */
    virtual void setChunkData(const char* const stringData)
    {
        assert(stringData);
    }

    /*!
     * Change the current plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        assert(index < (int32_t)prog.count);

        if (index < -1)
            index = -1;

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
#else
        Q_UNUSED(sendOsc);
#endif

        // Change default parameter values
        if (index >= 0)
        {
            for (uint32_t i=0; i < param.count; i++)
            {
                param.ranges[i].def = getParameterValue(i);

#ifndef BUILD_BRIDGE
                if (sendOsc)
                    osc_global_send_set_default_value(m_id, i, param.ranges[i].def);
#endif
            }
        }

        Q_UNUSED(sendGui);
        Q_UNUSED(block);
    }

    /*!
     * Change the current MIDI plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setMidiProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool block)
    {
        assert(index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;

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
#else
        Q_UNUSED(sendOsc);
#endif

        // Change default parameter values
        if (index >= 0)
        {
            // Sound banks never change defaults
            if (m_type == PLUGIN_GIG || m_type == PLUGIN_SF2 || m_type == PLUGIN_SFZ)
                return;

            for (uint32_t i=0; i < param.count; i++)
            {
                param.ranges[i].def = getParameterValue(i);

#ifndef BUILD_BRIDGE
                if (sendOsc)
                    osc_global_send_set_default_value(m_id, i, param.ranges[i].def);
#endif
            }
        }

        Q_UNUSED(sendGui);
        Q_UNUSED(block);
    }

    /*!
     * This is an overloaded call to setMidiProgram().\n
     * It changes the current MIDI program using \a bank and \a program values instead of index.
     */
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

    /*!
     * Set plugin's custom GUI stuff.\n
     * Parameters change between plugin types.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void setGuiData(int data, GuiDataHandle handle)
    {
        Q_UNUSED(data);
        Q_UNUSED(handle);
    }

    /*!
     * Show (or hide) a plugin's custom GUI according to \a yesNo.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void showGui(bool yesNo)
    {
        Q_UNUSED(yesNo);
    }

    /*!
     * Idle plugin's custom GUI.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void idleGui()
    {
    }

    // -------------------------------------------------------------------
    // Plugin state

    /*!
     * Reload the plugin's entire state (including programs).\n
     * The plugin will be disabled during this call.
     */
    virtual void reload()
    {
    }

    /*!
     * Reload the plugin's programs state.
     */
    virtual void reloadPrograms(bool init)
    {
        Q_UNUSED(init);
    }

    /*!
     * Tell the plugin to prepare for save.
     */
    virtual void prepareForSave()
    {
    }

    // -------------------------------------------------------------------
    // Plugin processing

    /*!
     * Plugin process callback.
     */
    virtual void process(float** inBuffer, float** outBuffer, uint32_t frames, uint32_t framesOffset = 0)
    {
        Q_UNUSED(inBuffer);
        Q_UNUSED(outBuffer);
        Q_UNUSED(frames);
        Q_UNUSED(framesOffset);
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

    /*!
     * Tell the plugin the current buffer size has changed.
     */
    virtual void bufferSizeChanged(uint32_t newBufferSize)
    {
        Q_UNUSED(newBufferSize);
    }

    // -------------------------------------------------------------------
    // OSC stuff

    /*!
     * Register this plugin to the global OSC client.
     */
    void registerToOsc()
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

        osc_send_bridge_audio_count(audioInCount(), audioOutCount(), audioInCount() + audioOutCount());
        osc_send_bridge_midi_count(midiInCount(), midiOutCount(), midiInCount() + midiOutCount());

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
                osc_send_bridge_param_data(i, param.data[i].type, param.data[i].rindex, param.data[i].hints, param.data[i].midiChannel, param.data[i].midiCC);
                osc_send_bridge_param_ranges(i, param.ranges[i].def, param.ranges[i].min, param.ranges[i].max, param.ranges[i].step, param.ranges[i].stepSmall, param.ranges[i].stepLarge);

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
            getParameterCountInfo(&param_info);
            osc_global_send_set_plugin_ports(m_id, audioInCount(), audioOutCount(), midiInCount(), midiOutCount(), param_info.ins, param_info.outs, param_info.total);

            // Parameters
            osc_global_send_set_parameter_value(m_id, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
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
                    osc_global_send_set_parameter_ranges(m_id, i, param.ranges[i].min, param.ranges[i].max, param.ranges[i].def, param.ranges[i].step, param.ranges[i].stepSmall, param.ranges[i].stepLarge);
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
    /*!
     * Update the plugin's internal OSC data according to \a source and \a url.\n
     * This is used for OSC-GUI bridges.
     */
    void updateOscData(lo_address source, const char* url)
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

        // TODO
        //osc_send_sample_rate(%osc.data, get_sample_rate());

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

    /*!
     * Clear the plugin's internal OSC data.
     */
    void clearOscData()
    {
        osc_clear_data(&osc.data);
    }

    /*!
     * Show the plugin's OSC based GUI.\n
     * This is a handy function that waits for the GUI to respond and automatically asks it to show itself.
     */
    bool showOscGui()
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

    /*!
     * Send a single midi note to be processed in the next audio callback.\n
     * A note with 0 velocity means note-off.
     */
    virtual void sendMidiSingleNote(uint8_t note, uint8_t velo, bool sendGui, bool sendOsc, bool sendCallback)
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

        if (sendCallback)
            callback_action(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, m_id, note, velo, 0.0);

#ifndef BUILD_BRIDGE
        if (sendOsc)
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
#else
        Q_UNUSED(sendOsc);
#endif
        Q_UNUSED(sendGui);
    }

    /*!
     * Send all midi notes off for the next audio callback.\n
     * This doesn't send the actual MIDI All-Notes-Off event, but 128 note-offs instead.
     */
    void sendMidiAllNotesOff()
    {
        carla_midi_lock();
        postEvents.mutex.lock();

        unsigned short postPad = 0;

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
        {
            if (postEvents.data[i].type == PluginPostEventNull)
            {
                postPad = i;
                break;
            }
        }

        if (postPad == MAX_POST_EVENTS - 1)
        {
            qWarning("post-events buffer full, making room for all notes off now");
            postPad -= 128;
        }

        for (unsigned short i=0; i < 128; i++)
        {
            extMidiNotes[i].valid = true;
            extMidiNotes[i].note  = i;
            extMidiNotes[i].velo  = 0;

            postEvents.data[i + postPad].type  = PluginPostEventNoteOff;
            postEvents.data[i + postPad].index = i;
            postEvents.data[i + postPad].value = 0.0;
        }

        postEvents.mutex.unlock();
        carla_midi_unlock();
    }

    // -------------------------------------------------------------------
    // Post-poned events

    /*!
     * Post pone an event of type \a type.\n
     * The event will be processed later in a high-priority thread (but not the main one).
     */
    void postponeEvent(PluginPostEventType type, int32_t index, double value)
    {
        postEvents.mutex.lock();
        for (unsigned short i=0; i<MAX_POST_EVENTS; i++)
        {
            if (postEvents.data[i].type == PluginPostEventNull)
            {
                postEvents.data[i].type  = type;
                postEvents.data[i].index = index;
                postEvents.data[i].value = value;
                break;
            }
        }
        postEvents.mutex.unlock();
    }

    /*!
     * Copy all the plugin's post-poned events into \a postEventsDest, and clear all events from the plugin.
     */
    void postEventsCopy(PluginPostEvent* const postEventsDest)
    {
        postEvents.mutex.lock();
        memcpy(postEventsDest, postEvents.data, sizeof(PluginPostEvent)*MAX_POST_EVENTS);

        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            postEvents.data[i].type = PluginPostEventNull;

        postEvents.mutex.unlock();
    }

    // -------------------------------------------------------------------
    // Cleanup

    /*!
     * Clear the engine client ports of the plugin.
     */
    virtual void removeClientPorts()
    {
        qDebug("CarlaPlugin::removeClientPorts() - start");

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

        qDebug("CarlaPlugin::removeClientPorts() - end");
    }

    /*!
     * Delete all temporary buffers of the plugin.
     */
    virtual void deleteBuffers()
    {
        qDebug("CarlaPlugin::deleteBuffers() - start");

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

        param.count    = 0;
        param.data     = nullptr;
        param.ranges   = nullptr;
        param.portCin  = nullptr;
        param.portCout = nullptr;

        qDebug("CarlaPlugin::deleteBuffers() - end");
    }

    // -------------------------------------------------------------------
    // Library functions

    /*!
     * Open the DLL \a filename.
     */
    bool libOpen(const char* filename)
    {
        m_lib = lib_open(filename);
        return bool(m_lib);
    }

    /*!
     * Close the DLL previously loaded in libOpen().
     */
    bool libClose()
    {
        if (m_lib)
            return lib_close(m_lib);
        return false;
    }

    /*!
     * Get the symbol entry \a symbol of the currently loaded DLL.
     */
    void* libSymbol(const char* symbol)
    {
        if (m_lib)
            return lib_symbol(m_lib, symbol);
        return nullptr;
    }

    /*!
     * Get the last DLL related error.
     */
    const char* libError(const char* filename)
    {
        return lib_error(filename);
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

    static double fixParameterValue(double& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    static float fixParameterValue(float& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    static double abs(const double& value)
    {
        return (value < 0.0) ? -value : value;
    }
};

/*!
 * \class CarlaPluginScopedDisabler
 *
 * \brief Carla plugin scoped disabler
 *
 * This is a handy class that temporarily disables a plugin during a function scope.\n
 * It should be used when the plugin needs reload or state change, something like this:
 * \code
 * {
 *      const CarlaPluginScopedDisabler m(plugin);
 *      plugin->setChunkData(data);
 * }
 * \endcode
 */
class CarlaPluginScopedDisabler
{
public:
    /*!
     * Disable plugin \a plugin if \a disable is true.
     * The plugin is re-enabled in the deconstructor of this class if \a disable is true.
     *
     * \param plugin The plugin to disable
     * \param disable Wherever to disable the plugin or not, true by default
     */
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

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_H

/*
 * Carla Backend
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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
#  include "carla_bridge_osc.h"
#endif

// common includes
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <QtGui/QDialog>

#ifdef Q_WS_X11
#include <QtGui/QX11EmbedContainer>
typedef QX11EmbedContainer GuiContainer;
#else
typedef QWidget GuiContainer;
#endif

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendPlugin Carla Backend Plugin
 *
 * The Carla Backend Plugin.
 * @{
 */

#define CARLA_PROCESS_CONTINUE_CHECK if (! m_enabled) { x_engine->callback(CALLBACK_DEBUG, m_id, m_enabled, 0, 0.0); return; }

const unsigned short MAX_MIDI_EVENTS = 512;
const unsigned short MAX_POST_EVENTS = 152;

#ifndef BUILD_BRIDGE
enum PluginBridgeInfoType {
    PluginBridgeAudioCount,
    PluginBridgeMidiCount,
    PluginBridgeParameterCount,
    PluginBridgeProgramCount,
    PluginBridgeMidiProgramCount,
    PluginBridgePluginInfo,
    PluginBridgeParameterInfo,
    PluginBridgeParameterData,
    PluginBridgeParameterRanges,
    PluginBridgeProgramInfo,
    PluginBridgeMidiProgramInfo,
    PluginBridgeConfigure,
    PluginBridgeSetParameterValue,
    PluginBridgeSetDefaultValue,
    PluginBridgeSetProgram,
    PluginBridgeSetMidiProgram,
    PluginBridgeSetCustomData,
    PluginBridgeSetChunkData,
    PluginBridgeUpdateNow,
    PluginBridgeError
};
#endif

enum PluginPostEventType {
    PluginPostEventNull,
    PluginPostEventDebug,
    PluginPostEventParameterChange,   // param, N, value
    PluginPostEventProgramChange,     // index
    PluginPostEventMidiProgramChange, // index
    PluginPostEventNoteOn,            // channel, note, velo
    PluginPostEventNoteOff,           // channel, note
    PluginPostEventCustom
};

struct PluginAudioData {
    uint32_t count;
    uint32_t* rindexes;
    CarlaEngineAudioPort** ports;

    PluginAudioData()
        : count(0),
          rindexes(nullptr),
          ports(nullptr) {}
};

struct PluginMidiData {
    CarlaEngineMidiPort* portMin;
    CarlaEngineMidiPort* portMout;

    PluginMidiData()
        : portMin(nullptr),
          portMout(nullptr) {}
};

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    CarlaEngineControlPort* portCin;
    CarlaEngineControlPort* portCout;

    PluginParameterData()
        : count(0),
          data(nullptr),
          ranges(nullptr),
          portCin(nullptr),
          portCout(nullptr) {}
};

struct PluginProgramData {
    uint32_t count;
    int32_t current;
    const char** names;

    PluginProgramData()
        : count(0),
          current(-1),
          names(nullptr) {}
};

struct PluginMidiProgramData {
    uint32_t count;
    int32_t current;
    midi_program_t* data;

    PluginMidiProgramData()
        : count(0),
          current(-1),
          data(nullptr) {}
};

struct PluginPostEvent {
    PluginPostEventType type;
    int32_t value1;
    int32_t value2;
    double  value3;
    const void* cdata;

    PluginPostEvent()
        : type(PluginPostEventNull),
          value1(-1),
          value2(-1),
          value3(0.0),
          cdata(nullptr) {}
};

struct ExternalMidiNote {
    int8_t channel; // invalid = -1
    uint8_t note;
    uint8_t velo;

    ExternalMidiNote()
        : channel(-1),
          note(0),
          velo(0) {}
};

// fallback data
static ParameterData   paramDataNull;
static ParameterRanges paramRangesNull;
static midi_program_t  midiProgramNull;
static CustomData      customDataNull;

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
     * \param engine The engine which this plugin belongs to, must not be null
     * \param id     The 'id' of this plugin, must between 0 and CarlaEngine::maxPluginNumber()
     */
    CarlaPlugin(CarlaEngine* const engine, const unsigned short id)
        : m_id(id),
          x_engine(engine),
          x_client(nullptr),
          x_dryWet(1.0),
          x_volume(1.0),
          x_balanceLeft(-1.0),
          x_balanceRight(1.0)
    {
        Q_ASSERT(engine);
        Q_ASSERT(id < CarlaEngine::maxPluginNumber());
        qDebug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

        m_type  = PLUGIN_NONE;
        m_hints = 0;

        m_active = false;
        m_activeBefore = false;
        m_enabled = false;

        m_lib  = nullptr;
        m_name = nullptr;
        m_filename = nullptr;

#ifndef BUILD_BRIDGE
        if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            m_ctrlInChannel = m_id;
        else
#endif
            m_ctrlInChannel = 0;

#ifndef BUILD_BRIDGE
        osc.data.path   = nullptr;
        osc.data.source = nullptr;
        osc.data.target = nullptr;
        osc.thread = nullptr;
#endif
    }

    /*!
     * This is the de-constructor of the base plugin class.
     */
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
     * if (m_hints & PLUGIN_IS_BRIDGE)
     *     ...
     * \endcode
     */
    PluginType type() const
    {
        return m_type;
    }

    /*!
     * Get the plugin's id (as passed in the constructor).
     *
     * \see setId()
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
     * This name is unique within all plugins in an engine.
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
        return aIn.count;
    }

    /*!
     * Get the number of audio outputs.
     */
    virtual uint32_t audioOutCount()
    {
        return aOut.count;
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
     * To know the number of parameter inputs and outputs separately use getParameterCountInfo() instead.
     */
    uint32_t parameterCount() const
    {
        return param.count;
    }

    /*!
     * Get the number of scalepoints for parameter \a parameterId.
     */
    virtual uint32_t parameterScalePointCount(const uint32_t parameterId)
    {
        Q_ASSERT(parameterId < param.count);

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
    size_t customDataCount() const
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
    const ParameterData* parameterData(const uint32_t parameterId) const
    {
        Q_ASSERT(parameterId < param.count);

        if (parameterId < param.count)
            return &param.data[parameterId];

        return &paramDataNull;
    }

    /*!
     * Get the parameter ranges of \a parameterId.
     */
    const ParameterRanges* parameterRanges(const uint32_t parameterId) const
    {
        Q_ASSERT(parameterId < param.count);

        if (parameterId < param.count)
            return &param.ranges[parameterId];

        return &paramRangesNull;
    }

    /*!
     * Check if parameter \a parameterId is of output type.
     */
    bool parameterIsOutput(const uint32_t parameterId) const
    {
        Q_ASSERT(parameterId < param.count);

        if (parameterId < param.count)
            return (param.data[parameterId].type == PARAMETER_OUTPUT);

        return false;
    }

    /*!
     * Get the MIDI program at \a index.
     *
     * \see getMidiProgramName()
     */
    const midi_program_t* midiProgramData(const uint32_t index) const
    {
        Q_ASSERT(index < midiprog.count);

        if (index < midiprog.count)
            return &midiprog.data[index];

        return &midiProgramNull;
    }

    /*!
     * Get the custom data set at \a index.
     *
     * \see setCustomData()
     */
    const CustomData* customData(const size_t index) const
    {
        Q_ASSERT(index < custom.size());

        if (index < custom.size())
            return &custom[index];

        return &customDataNull;
    }

    /*!
     * Get the complete plugin chunk data into \a dataPtr.
     *
     * \return The size of the chunk or 0 if invalid.
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     *
     * \see setChunkData()
     */
    virtual int32_t chunkData(void** const dataPtr)
    {
        Q_ASSERT(dataPtr);

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*!
     * Get the current parameter value of \a parameterId.
     */
    virtual double getParameterValue(const uint32_t parameterId)
    {
        Q_ASSERT(parameterId < param.count);

        return 0.0;
    }

    /*!
     * Get the scalepoint \a scalePointId value of the parameter \a parameterId.
     */
    virtual double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
    {
        Q_ASSERT(parameterId < param.count);
        Q_ASSERT(scalePointId < parameterScalePointCount(parameterId));

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
    virtual void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        *strBuf = 0;
    }

    /*!
     * Get the symbol of the parameter \a parameterId.
     */
    virtual void getParameterSymbol(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        *strBuf = 0;
    }

    /*!
     * Get the custom text of the parameter \a parameterId.
     */
    virtual void getParameterText(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        *strBuf = 0;
    }

    /*!
     * Get the unit of the parameter \a parameterId.
     */
    virtual void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        *strBuf = 0;
    }

    /*!
     * Get the scalepoint \a scalePointId label of the parameter \a parameterId.
     */
    virtual void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);
        Q_ASSERT(scalePointId < parameterScalePointCount(parameterId));

        *strBuf = 0;
    }

    /*!
     * Get the name of the program at \a index.
     */
    void getProgramName(const uint32_t index, char* const strBuf)
    {
        Q_ASSERT(index < prog.count);

        if (index < prog.count && prog.names[index])
            strncpy(strBuf, prog.names[index], STR_MAX);
        else
            *strBuf = 0;
    }

    /*!
     * Get the name of the MIDI program at \a index.
     *
     * \see getMidiProgramInfo()
     */
    void getMidiProgramName(const uint32_t index, char* const strBuf)
    {
        Q_ASSERT(index < midiprog.count);

        if (index < midiprog.count && midiprog.data[index].name)
            strncpy(strBuf, midiprog.data[index].name, STR_MAX);
        else
            *strBuf = 0;
    }

    /*!
     * Get information about the plugin's parameter count.\n
     * This is used to check how many input, output and total parameters are available.\n
     *
     * \note Some parameters might not be input or output (ie, invalid).
     *
     * \see parameterCount()
     */
    void getParameterCountInfo(uint32_t* const ins, uint32_t* const outs, uint32_t* const total)
    {
        *ins   = 0;
        *outs  = 0;
        *total = param.count;

        for (uint32_t i=0; i < param.count; i++)
        {
            if (param.data[i].type == PARAMETER_INPUT)
                *ins += 1;
            else if (param.data[i].type == PARAMETER_OUTPUT)
                *outs += 1;
        }
    }

    /*!
     * Get information about the plugin's custom GUI, if provided.
     */
    virtual void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        *type = GUI_NONE;
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

#ifndef BUILD_BRIDGE
    /*!
     * Set the plugin's id to \a id.
     *
     * \see id()
     */
    void setId(const unsigned short id)
    {
        m_id = id;

        if (carlaOptions.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            m_ctrlInChannel = id;
    }
#endif

    /*!
     * Enable or disable the plugin according to \a yesNo.
     *
     * When a plugin is disabled, it will never be processed or managed in any way.\n
     * To 'bypass' a plugin use setActive() instead.
     *
     * \see enabled()
     */
    void setEnabled(const bool yesNo)
    {
        m_enabled = yesNo;
    }

    /*!
     * Set plugin as active according to \a active.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setActive(const bool active, const bool sendOsc, const bool sendCallback)
    {
        m_active = active;
        double value = active ? 1.0 : 0.0;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_ACTIVE, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_ACTIVE, 0, value);
#ifndef BUILD_BRIDGE
        else if (m_hints & PLUGIN_IS_BRIDGE)
            osc_send_control(&osc.data, PARAMETER_ACTIVE, value);
#endif
    }

    /*!
     * Set the plugin's dry/wet signal value to \a value.\n
     * \a value must be between 0.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setDryWet(double value, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(value >= 0.0 && value <= 1.0);

        if (value < 0.0)
            value = 0.0;
        else if (value > 1.0)
            value = 1.0;

        x_dryWet = value;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_DRYWET, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_DRYWET, 0, value);
#ifndef BUILD_BRIDGE
        else if (m_hints & PLUGIN_IS_BRIDGE)
            osc_send_control(&osc.data, PARAMETER_DRYWET, value);
#endif
    }

    /*!
     * Set the plugin's output volume to \a value.\n
     * \a value must be between 0.0 and 1.27.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setVolume(double value, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(value >= 0.0 && value <= 1.27);

        if (value < 0.0)
            value = 0.0;
        else if (value > 1.27)
            value = 1.27;

        x_volume = value;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_VOLUME, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_VOLUME, 0, value);
#ifndef BUILD_BRIDGE
        else if (m_hints & PLUGIN_IS_BRIDGE)
            osc_send_control(&osc.data, PARAMETER_VOLUME, value);
#endif
    }

    /*!
     * Set the plugin's output left balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setBalanceLeft(double value, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(value >= -1.0 && value <= 1.0);

        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_balanceLeft = value;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_LEFT, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_BALANCE_LEFT, 0, value);
#ifndef BUILD_BRIDGE
        else if (m_hints & PLUGIN_IS_BRIDGE)
            osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, value);
#endif
    }

    /*!
     * Set the plugin's output right balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setBalanceRight(double value, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(value >= -1.0 && value <= 1.0);

        if (value < -1.0)
            value = -1.0;
        else if (value > 1.0)
            value = 1.0;

        x_balanceRight = value;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_RIGHT, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_BALANCE_RIGHT, 0, value);
#ifndef BUILD_BRIDGE
        else if (m_hints & PLUGIN_IS_BRIDGE)
            osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, value);
#endif
    }

#ifndef BUILD_BRIDGE
    /*!
     * BridgePlugin call used to set internal data.
     */
    virtual int setOscBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types)
    {
        return 1;
        Q_UNUSED(type);
        Q_UNUSED(argc);
        Q_UNUSED(argv);
        Q_UNUSED(types);
    }
#endif

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    /*!
     * Set a plugin's parameter value.
     *
     * \param parameterId The parameter to change
     * \param value The new parameter value, must be within the parameter's range
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \see getParameterValue()
     */
    virtual void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count);

        if (sendGui)
            uiParameterChange(parameterId, value);

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_value(m_id, parameterId, value);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, parameterId, 0, value);
    }

    /*!
     * Set a plugin's parameter value, including internal parameters.\n
     * \a rindex can be negative to allow internal parameters change (as defined in InternalParametersIndex).
     *
     * \see setParameterValue()
     * \see setActive()
     * \see setDryWet()
     * \see setVolume()
     * \see setBalanceLeft()
     * \see setBalanceRight()
     */
    void setParameterValueByRIndex(const int32_t rindex, const double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(rindex >= PARAMETER_BALANCE_RIGHT && rindex != PARAMETER_NULL);

        if (rindex == PARAMETER_ACTIVE)
            return setActive(value > 0.0, sendOsc, sendCallback);
        if (rindex == PARAMETER_DRYWET)
            return setDryWet(value, sendOsc, sendCallback);
        if (rindex == PARAMETER_VOLUME)
            return setVolume(value, sendOsc, sendCallback);
        if (rindex == PARAMETER_BALANCE_LEFT)
            return setBalanceLeft(value, sendOsc, sendCallback);
        if (rindex == PARAMETER_BALANCE_RIGHT)
            return setBalanceRight(value, sendOsc, sendCallback);

        for (uint32_t i=0; i < param.count; i++)
        {
            if (param.data[i].rindex == rindex)
                return setParameterValue(i, value, sendGui, sendOsc, sendCallback);
        }
    }

    /*!
     * Set parameter's \a parameterId MIDI channel to \a channel.\n
     * \a channel must be between 0 and 15.
     */
    void setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count && channel < 16);

        if (channel >= 16)
            channel = 16;

        param.data[parameterId].midiChannel = channel;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_midi_channel(m_id, parameterId, channel);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, m_id, parameterId, channel, 0.0);
    }

    /*!
     * Set parameter's \a parameterId MIDI CC to \a cc.\n
     * \a cc must be between 0 and 95 (0x5F), or -1 for invalid.
     */
    void setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count && cc >= -1);

        if (cc < -1 || cc > 0x5F)
            cc = -1;

        param.data[parameterId].midiCC = cc;

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_parameter_midi_cc(m_id, parameterId, cc);
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, m_id, parameterId, cc, 0.0);
    }

    /*!
     * Add a custom data set.\n
     * If \a key already exists, its current value will be swapped with \a value.
     *
     * \param type Type of data used in \a value.
     * \param key A key identifing this data set.
     * \param value The value of the data set, of type \a type.
     * \param sendGui Send message change to plugin's custom GUI, if any
     *
     * \see customData()
     */
    virtual void setCustomData(const CustomDataType type, const char* const key, const char* const value, const bool sendGui)
    {
        Q_ASSERT(type != CUSTOM_DATA_INVALID);
        Q_ASSERT(key);
        Q_ASSERT(value);

        if (type == CUSTOM_DATA_INVALID)
            return qCritical("CarlaPlugin::setCustomData(%s, \"%s\", \"%s\", %s) - type is invalid", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (! key)
            return qCritical("CarlaPlugin::setCustomData(%s, \"%s\", \"%s\", %s) - key is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        if (! value)
            return qCritical("CarlaPlugin::setCustomData(%s, \"%s\", \"%s\", %s) - value is null", CustomDataType2str(type), key, value, bool2str(sendGui));

        bool saveData = true;

        switch (type)
        {
        case CUSTOM_DATA_INVALID:
            saveData = false;
            break;
        case CUSTOM_DATA_STRING:
            // Ignore some keys
            if (strncmp(key, "OSC:", 4) == 0 || strcmp(key, "guiVisible") == 0)
                saveData = false;
            else if (strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
                saveData = false;
            break;
        default:
            break;
        }

        if (saveData)
        {
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

            // Otherwise store it
            CustomData newData;
            newData.type  = type;
            newData.key   = strdup(key);
            newData.value = strdup(value);
            custom.push_back(newData);
        }
    }

    /*!
     * Set the complete chunk data as \a stringData.\n
     * \a stringData must a base64 encoded string of binary data.
     *
     * \see chunkData()
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     */
    virtual void setChunkData(const char* const stringData)
    {
        Q_ASSERT(stringData);
    }

    /*!
     * Change the current plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.\n
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        Q_ASSERT(index >= -1 && index < (int32_t)prog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)prog.count)
            return;

        prog.current = index;

        if (sendGui && index >= 0)
            uiProgramChange(index);

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_program(m_id, index);
#else
        Q_UNUSED(sendOsc);
#endif

        // Change default parameter values
        if (index >= 0)
        {
            for (uint32_t i=0; i < param.count; i++)
            {
                param.ranges[i].def = getParameterValue(i);
                fixParameterValue(param.ranges[i].def, param.ranges[i]);

#ifndef BUILD_BRIDGE
                if (sendOsc)
                    x_engine->osc_send_control_set_default_value(m_id, i, param.ranges[i].def);
#endif
            }
        }

        if (sendCallback)
            x_engine->callback(CALLBACK_PROGRAM_CHANGED, m_id, index, 0, 0.0);

        Q_UNUSED(block);
    }

    /*!
     * Change the current MIDI plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.\n
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        Q_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

        if (index < -1)
            index = -1;
        else if (index > (int32_t)midiprog.count)
            return;

        midiprog.current = index;

        if (sendGui && index >= 0)
            uiMidiProgramChange(index);

#ifndef BUILD_BRIDGE
        if (sendOsc)
            x_engine->osc_send_control_set_midi_program(m_id, index);
#else
        Q_UNUSED(sendOsc);
#endif

        // Change default parameter values (sound banks never change defaults)
        if (index >= 0 && m_type != PLUGIN_GIG && m_type != PLUGIN_SF2 && m_type != PLUGIN_SFZ)
        {
            for (uint32_t i=0; i < param.count; i++)
            {
                param.ranges[i].def = getParameterValue(i);
                fixParameterValue(param.ranges[i].def, param.ranges[i]);

#ifndef BUILD_BRIDGE
                if (sendOsc)
                    x_engine->osc_send_control_set_default_value(m_id, i, param.ranges[i].def);
#endif
            }
        }

        if (sendCallback)
            x_engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, index, 0, 0.0);

        Q_UNUSED(block);
    }

    /*!
     * This is an overloaded call to setMidiProgram().\n
     * It changes the current MIDI program using \a bank and \a program values instead of index.
     */
    void setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
    {
        Q_ASSERT(program < 128);

        for (uint32_t i=0; i < midiprog.count; i++)
        {
            if (midiprog.data[i].bank == bank && midiprog.data[i].program == program)
                return setMidiProgram(i, sendGui, sendOsc, sendCallback, block);
        }
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    /*!
     * Set the plugin's custom GUI container.\n
     *
     * \note This function must be always called from the main thread.
     */
    virtual void setGuiContainer(GuiContainer* const container)
    {
        Q_UNUSED(container);
    }

    /*!
     * Show (or hide) the plugin's custom GUI according to \a yesNo.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void showGui(const bool yesNo)
    {
        Q_UNUSED(yesNo);
    }

    /*!
     * Idle the plugin's custom GUI.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void idleGui()
    {
        if (! m_enabled)
            return;

        if (m_hints & PLUGIN_USES_SINGLE_THREAD)
        {
            // Process postponed events
            postEventsRun();

            // Update parameter outputs
            for (uint32_t i=0; i < param.count; i++)
            {
                if (param.data[i].type == PARAMETER_OUTPUT)
                   uiParameterChange(i, getParameterValue(i));
            }
        }
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
    virtual void reloadPrograms(const bool init)
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
    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset = 0)
    {
        Q_UNUSED(inBuffer);
        Q_UNUSED(outBuffer);
        Q_UNUSED(frames);
        Q_UNUSED(framesOffset);
    }

#ifdef CARLA_ENGINE_JACK
    /*!
     * Plugin process callback, JACK helper version.
     */
    void process_jack(const uint32_t nframes)
    {
        float* inBuffer[aIn.count];
        float* outBuffer[aOut.count];

        for (uint32_t i=0; i < aIn.count; i++)
            inBuffer[i] = aIn.ports[i]->getJackAudioBuffer(nframes);

        for (uint32_t i=0; i < aOut.count; i++)
            outBuffer[i] = aOut.ports[i]->getJackAudioBuffer(nframes);

#ifndef BUILD_BRIDGE
        if (carlaOptions.processHighPrecision)
        {
            float* inBuffer2[aIn.count];
            float* outBuffer2[aOut.count];

            for (uint32_t i=0, j; i < nframes; i += 8)
            {
                for (j=0; j < aIn.count; j++)
                    inBuffer2[j] = inBuffer[j] + i;

                for (j=0; j < aOut.count; j++)
                    outBuffer2[j] = outBuffer[j] + i;

                process(inBuffer2, outBuffer2, 8, i);
            }
        }
        else
#endif
            process(inBuffer, outBuffer, nframes);
    }
#endif

    /*!
     * Tell the plugin the current buffer size has changed.
     */
    virtual void bufferSizeChanged(const uint32_t newBufferSize)
    {
        Q_UNUSED(newBufferSize);
    }

    // -------------------------------------------------------------------
    // OSC stuff

    /*!
     * Register this plugin to the engine's OSC controller.
     */
    void registerToOsc()
    {
        if (! x_engine->isOscControllerRegisted())
            return;

#ifndef BUILD_BRIDGE
        x_engine->osc_send_control_add_plugin(m_id, m_name);
#endif

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

#ifdef BUILD_BRIDGE
            x_engine->osc_send_bridge_plugin_info(category(), m_hints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#else
            x_engine->osc_send_control_set_plugin_data(m_id, m_type, category(), m_hints, bufName, bufLabel, bufMaker, bufCopyright, uniqueId());
#endif
        }

        // Base count
        {
            uint32_t cIns, cOuts, cTotals;
            getParameterCountInfo(&cIns, &cOuts, &cTotals);

#ifdef BUILD_BRIDGE
            x_engine->osc_send_bridge_audio_count(audioInCount(), audioOutCount(), audioInCount() + audioOutCount());
            x_engine->osc_send_bridge_midi_count(midiInCount(), midiOutCount(), midiInCount() + midiOutCount());
            x_engine->osc_send_bridge_parameter_count(cIns, cOuts, cTotals);
#else
            x_engine->osc_send_control_set_plugin_ports(m_id, audioInCount(), audioOutCount(), midiInCount(), midiOutCount(), cIns, cOuts, cTotals);
#endif
        }

        // Internal Parameters
        {
#ifndef BUILD_BRIDGE
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_DRYWET, x_dryWet);
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_VOLUME, x_volume);
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_LEFT, x_balanceLeft);
            x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_RIGHT, x_balanceRight);
#endif
        }

        // Plugin Parameters
#ifdef BUILD_BRIDGE
        uint32_t maxParameters = MAX_PARAMETERS;
#else
        uint32_t maxParameters = carlaOptions.maxParameters;
#endif
        if (param.count > 0 && param.count < maxParameters)
        {
            char bufName[STR_MAX], bufUnit[STR_MAX];

            for (uint32_t i=0; i < param.count; i++)
            {
                getParameterName(i, bufName);
                getParameterUnit(i, bufUnit);

#ifdef BUILD_BRIDGE
                x_engine->osc_send_bridge_parameter_info(i, bufName, bufUnit);
                x_engine->osc_send_bridge_parameter_data(i, param.data[i].type, param.data[i].rindex, param.data[i].hints, param.data[i].midiChannel, param.data[i].midiCC);
                x_engine->osc_send_bridge_parameter_ranges(i, param.ranges[i].def, param.ranges[i].min, param.ranges[i].max, param.ranges[i].step, param.ranges[i].stepSmall, param.ranges[i].stepLarge);
                x_engine->osc_send_bridge_set_parameter_value(i, getParameterValue(i));
#else
                x_engine->osc_send_control_set_parameter_data(m_id, i, param.data[i].type, param.data[i].hints, bufName, bufUnit, getParameterValue(i));
                x_engine->osc_send_control_set_parameter_ranges(m_id, i, param.ranges[i].min, param.ranges[i].max, param.ranges[i].def, param.ranges[i].step, param.ranges[i].stepSmall, param.ranges[i].stepLarge);
                x_engine->osc_send_control_set_parameter_value(m_id, i, getParameterValue(i));
#endif
            }
        }

        // Programs
        if (prog.count > 0)
        {
#ifdef BUILD_BRIDGE
            x_engine->osc_send_bridge_program_count(prog.count);

            for (uint32_t i=0; i < prog.count; i++)
                x_engine->osc_send_bridge_program_info(i, prog.names[i]);

            x_engine->osc_send_bridge_set_program(prog.current);
#else
            x_engine->osc_send_control_set_program_count(m_id, prog.count);

            for (uint32_t i=0; i < prog.count; i++)
                x_engine->osc_send_control_set_program_name(m_id, i, prog.names[i]);

            x_engine->osc_send_control_set_program(m_id, prog.current);
#endif
        }

        // MIDI Programs
        if (midiprog.count > 0)
        {
#ifdef BUILD_BRIDGE
            x_engine->osc_send_bridge_midi_program_count(midiprog.count);

            for (uint32_t i=0; i < midiprog.count; i++)
                x_engine->osc_send_bridge_midi_program_info(i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

            x_engine->osc_send_bridge_set_midi_program(prog.current);
#else
            x_engine->osc_send_control_set_midi_program_count(m_id, midiprog.count);

            for (uint32_t i=0; i < midiprog.count; i++)
                x_engine->osc_send_control_set_midi_program_data(m_id, i, midiprog.data[i].bank, midiprog.data[i].program, midiprog.data[i].name);

            x_engine->osc_send_control_set_midi_program(m_id, midiprog.current);
#endif
        }
    }

#ifndef BUILD_BRIDGE
    /*!
     * Update the plugin's internal OSC data according to \a source and \a url.\n
     * This is used for OSC-GUI bridges.
     */
    void updateOscData(const lo_address source, const char* const url)
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

        if (m_hints & PLUGIN_IS_BRIDGE)
            return;

        osc_send_sample_rate(&osc.data, x_engine->getSampleRate());

        for (size_t i=0; i < custom.size(); i++)
        {
            // TODO
            //if (m_type == PLUGIN_LV2)
                //osc_send_lv2_transfer_event(&osc.data, getCustomDataTypeString(custom[i].type), /*custom[i].key,*/ custom[i].value);
            //else
            if (custom[i].type == CUSTOM_DATA_STRING)
                osc_send_configure(&osc.data, custom[i].key, custom[i].value);
        }

        if (prog.current >= 0)
            osc_send_program(&osc.data, prog.current);

        if (midiprog.current >= 0)
        {
            if (m_type == PLUGIN_DSSI)
                osc_send_program(&osc.data, midiprog.data[midiprog.current].bank, midiprog.data[midiprog.current].program);
            else
                osc_send_midi_program(&osc.data, midiprog.data[midiprog.current].bank, midiprog.data[midiprog.current].program);
        }

        for (uint32_t i=0; i < param.count; i++)
            osc_send_control(&osc.data, param.data[i].rindex, getParameterValue(i));

//        if (m_hints & PLUGIN_IS_BRIDGE)
//        {
//            osc_send_control(&osc.data, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
//            osc_send_control(&osc.data, PARAMETER_DRYWET, x_dryWet);
//            osc_send_control(&osc.data, PARAMETER_VOLUME, x_volume);
//            osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, x_balanceLeft);
//            osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, x_balanceRight);
//        }
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
        for (uint i=0; i < carlaOptions.oscUiTimeout; i++)
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
    void sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);
        Q_ASSERT(velo < 128);

        engineMidiLock();
        for (unsigned short i=0; i < MAX_MIDI_EVENTS; i++)
        {
            if (extMidiNotes[i].channel < 0)
            {
                extMidiNotes[i].channel = channel;
                extMidiNotes[i].note = note;
                extMidiNotes[i].velo = velo;
                break;
            }
        }
        engineMidiUnlock();

        if (sendGui)
        {
            if (velo > 0)
                uiNoteOn(channel, note, velo);
            else
                uiNoteOff(channel, note);
        }

#ifndef BUILD_BRIDGE
        if (sendOsc)
        {
            if (velo > 0)
                x_engine->osc_send_control_note_on(m_id, channel, note, velo);
            else
                x_engine->osc_send_control_note_off(m_id, channel, note);
        }
#else
        Q_UNUSED(sendOsc);
#endif

        if (sendCallback)
            x_engine->callback(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, m_id, note, velo, 0.0);
    }

    /*!
     * Send all midi notes off for the next audio callback.\n
     * This doesn't send the actual MIDI All-Notes-Off event, but 128 note-offs instead.
     */
    void sendMidiAllNotesOff()
    {
        engineMidiLock();
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
            extMidiNotes[i].channel = m_ctrlInChannel;
            extMidiNotes[i].note = i;
            extMidiNotes[i].velo = 0;

            postEvents.data[i + postPad].type  = PluginPostEventNoteOff;
            postEvents.data[i + postPad].value1 = i;
            postEvents.data[i + postPad].value2 = 0;
            postEvents.data[i + postPad].value3 = 0.0;
        }

        postEvents.mutex.unlock();
        engineMidiUnlock();
    }

    // -------------------------------------------------------------------
    // Post-poned events

    /*!
     * Post pone an event of type \a type.\n
     * The event will be processed later, but as soon as possible.
     */
    void postponeEvent(const PluginPostEventType type, const int32_t value1, const int32_t value2, const double value3, const void* const cdata = nullptr)
    {
        postEvents.mutex.lock();
        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
        {
            if (postEvents.data[i].type == PluginPostEventNull)
            {
                postEvents.data[i].type   = type;
                postEvents.data[i].value1 = value1;
                postEvents.data[i].value2 = value2;
                postEvents.data[i].value3 = value3;
                postEvents.data[i].cdata  = cdata;
                break;
            }
        }
        postEvents.mutex.unlock();
    }

    /*!
     * Process all the post-poned events.
     * This function will only be called from the main thread if PLUGIN_USES_SINGLE_THREAD is set.
     */
    void postEventsRun()
    {
        PluginPostEvent newPostEvents[MAX_POST_EVENTS];

        // Make a safe copy of events, and clear them
        postEvents.mutex.lock();
        memcpy(newPostEvents, postEvents.data, sizeof(PluginPostEvent)*MAX_POST_EVENTS);
        for (unsigned short i=0; i < MAX_POST_EVENTS; i++)
            postEvents.data[i].type = PluginPostEventNull;
        postEvents.mutex.unlock();

        // Handle events now
        for (uint32_t i=0; i < MAX_POST_EVENTS; i++)
        {
            const PluginPostEvent* const event = &newPostEvents[i];

            switch (event->type)
            {
            case PluginPostEventNull:
                break;

            case PluginPostEventDebug:
                x_engine->callback(CALLBACK_DEBUG, m_id, event->value1, event->value2, event->value3);
                break;

            case PluginPostEventParameterChange:
                // Update UI
                if (event->value1 >= 0)
                    uiParameterChange(event->value1, event->value3);

#ifndef BUILD_BRIDGE
                // Update OSC control client
                x_engine->osc_send_control_set_parameter_value(m_id, event->value1, event->value3);
#endif

                // Update Host
                x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, event->value1, 0, event->value3);
                break;

            case PluginPostEventProgramChange:
                // Update UI
                if (event->value1 >= 0)
                    uiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
                // Update OSC control client
                x_engine->osc_send_control_set_program(m_id, event->value1);

                for (uint32_t j=0; j < param.count; j++)
                    x_engine->osc_send_control_set_default_value(m_id, j, param.ranges[j].def);
#endif

                // Update Host
                x_engine->callback(CALLBACK_PROGRAM_CHANGED, m_id, event->value1, 0, 0.0);
                break;

            case PluginPostEventMidiProgramChange:
                // Update UI
                if (event->value1 >= 0)
                    uiMidiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
                // Update OSC control client
                x_engine->osc_send_control_set_midi_program(m_id, event->value1);

                for (uint32_t j=0; j < param.count; j++)
                    x_engine->osc_send_control_set_default_value(m_id, j, param.ranges[j].def);
#endif

                // Update Host
                x_engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, event->value1, 0, 0.0);
                break;

            case PluginPostEventNoteOn:
                // Update UI
                uiNoteOn(event->value1, event->value2, rint(event->value3));

#ifndef BUILD_BRIDGE
                // Update OSC control client
                x_engine->osc_send_control_note_on(m_id, event->value1, event->value2, rint(event->value3));
#endif

                // Update Host
                x_engine->callback(CALLBACK_NOTE_ON, m_id, event->value1, event->value2, event->value3);
                break;

            case PluginPostEventNoteOff:
                // Update UI
                uiNoteOff(event->value1, event->value2);

#ifndef BUILD_BRIDGE
                // Update OSC control client
                x_engine->osc_send_control_note_off(m_id, event->value1, event->value2);
#endif

                // Update Host
                x_engine->callback(CALLBACK_NOTE_OFF, m_id, event->value1, event->value2, 0.0);
                break;

            case PluginPostEventCustom:
                // Handle custom event
                postEventHandleCustom(event->value1, event->value2, event->value3, event->cdata);
                break;
            }
        }
    }


    /*!
     * Handle custom post event.\n
     * Implementation depends on plugin type.
     */
    virtual void postEventHandleCustom(const int32_t value1, const int32_t value2, const double value3, const void* const cdata)
    {
        Q_UNUSED(value1);
        Q_UNUSED(value2);
        Q_UNUSED(value3);
        Q_UNUSED(cdata);
    }

    /*!
     * Tell the UI a parameter has changed.
     */
    virtual void uiParameterChange(const uint32_t index, const double value)
    {
        Q_ASSERT(index < param.count);

        Q_UNUSED(value);
    }

    /*!
     * Tell the UI the current program has changed.
     */
    virtual void uiProgramChange(const uint32_t index)
    {
        Q_ASSERT(index < prog.count);
    }

    /*!
     * Tell the UI the current midi program has changed.
     */
    virtual void uiMidiProgramChange(const uint32_t index)
    {
        Q_ASSERT(index < midiprog.count);
    }

    /*!
     * Tell the UI a note has been pressed.
     */
    virtual void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);
        Q_ASSERT(velo > 0 && velo < 128);
    }

    /*!
     * Tell the UI a note has been released.
     */
    virtual void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);
    }

    // -------------------------------------------------------------------
    // Cleanup

    /*!
     * Clear the engine client ports of the plugin.
     */
    virtual void removeClientPorts()
    {
        qDebug("CarlaPlugin::removeClientPorts() - start");

        for (uint32_t i=0; i < aIn.count; i++)
        {
            delete aIn.ports[i];
            aIn.ports[i] = nullptr;
        }

        for (uint32_t i=0; i < aOut.count; i++)
        {
            delete aOut.ports[i];
            aOut.ports[i] = nullptr;
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
     * Initializes all RT buffers of the plugin.
     */
    virtual void initBuffers()
    {
        uint32_t i;

        for (i=0; i < aIn.count; i++)
        {
            if (aIn.ports[i])
                aIn.ports[i]->initBuffer(x_engine);
        }

        for (i=0; i < aOut.count; i++)
        {
            if (aOut.ports[i])
                aOut.ports[i]->initBuffer(x_engine);
        }

        if (param.portCin)
            param.portCin->initBuffer(x_engine);

        if (param.portCout)
            param.portCout->initBuffer(x_engine);

        if (midi.portMin)
            midi.portMin->initBuffer(x_engine);

        if (midi.portMout)
            midi.portMout->initBuffer(x_engine);
    }

    /*!
     * Delete all temporary buffers of the plugin.
     */
    virtual void deleteBuffers()
    {
        qDebug("CarlaPlugin::deleteBuffers() - start");

        if (aIn.count > 0)
        {
            delete[] aIn.ports;
            delete[] aIn.rindexes;
        }

        if (aOut.count > 0)
        {
            delete[] aOut.ports;
            delete[] aOut.rindexes;
        }

        if (param.count > 0)
        {
            delete[] param.data;
            delete[] param.ranges;
        }

        aIn.count = 0;
        aIn.ports = nullptr;
        aIn.rindexes = nullptr;

        aOut.count = 0;
        aOut.ports = nullptr;
        aOut.rindexes = nullptr;

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
    bool libOpen(const char* const filename)
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
    void* libSymbol(const char* const symbol)
    {
        if (m_lib)
            return lib_symbol(m_lib, symbol);
        return nullptr;
    }

    /*!
     * Get the last DLL related error.
     */
    const char* libError(const char* const filename)
    {
        return lib_error(filename);
    }

    // -------------------------------------------------------------------
    // Locks

    void engineProcessLock()
    {
        x_engine->processLock();
    }

    void engineProcessUnlock()
    {
        x_engine->processUnlock();
    }

    void engineMidiLock()
    {
        x_engine->midiLock();
    }

    void engineMidiUnlock()
    {
        x_engine->midiUnlock();
    }

    // -------------------------------------------------------------------
    // Plugin initializers

    struct initializer {
        CarlaEngine* const engine;
        const char* const filename;
        const char* const name;
        const char* const label;
    };

    static CarlaPlugin* newLADSPA(const initializer& init, const void* const extra);
    static CarlaPlugin* newDSSI(const initializer& init, const void* const extra);
    static CarlaPlugin* newLV2(const initializer& init);
    static CarlaPlugin* newVST(const initializer& init);
    static CarlaPlugin* newGIG(const initializer& init);
    static CarlaPlugin* newSF2(const initializer& init);
    static CarlaPlugin* newSFZ(const initializer& init);
#ifndef BUILD_BRIDGE
    static CarlaPlugin* newBridge(const initializer& init, const BinaryType btype, const PluginType ptype);
#endif
    static CarlaPlugin* newNative(const initializer& init);

    static size_t getNativePluginCount();

    // -------------------------------------------------------------------

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
    class ScopedDisabler
    {
    public:
        /*!
         * Disable plugin \a plugin if \a disable is true.
         * The plugin is re-enabled in the deconstructor of this class if \a disable is true.
         *
         * \param plugin The plugin to disable
         * \param disable Wherever to disable the plugin or not, true by default
         */
        ScopedDisabler(CarlaPlugin* const plugin, const bool disable = true) :
            m_plugin(plugin),
            m_disable(disable)
        {
            if (m_disable)
            {
                m_plugin->engineProcessLock();
                m_plugin->setEnabled(false);
                m_plugin->engineProcessUnlock();
            }
        }

        ~ScopedDisabler()
        {
            if (m_disable)
            {
                m_plugin->engineProcessLock();
                m_plugin->setEnabled(true);
                m_plugin->engineProcessUnlock();
            }
        }

    private:
        CarlaPlugin* const m_plugin;
        const bool m_disable;
    };

    // -------------------------------------------------------------------

protected:
    unsigned short m_id;
    CarlaEngine* const x_engine;
    CarlaEngineClient* x_client;
    double x_dryWet, x_volume;
    double x_balanceLeft, x_balanceRight;

    PluginType m_type;
    unsigned int m_hints;

    bool m_active;
    bool m_activeBefore;
    bool m_enabled;

    void* m_lib;
    const char* m_name;
    const char* m_filename;

    int8_t m_ctrlInChannel;

    // -------------------------------------------------------------------
    // Storage Data

    PluginAudioData aIn;
    PluginAudioData aOut;
    PluginMidiData midi;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    std::vector<CustomData> custom;

    // -------------------------------------------------------------------
    // Extra

#ifndef BUILD_BRIDGE
    struct {
        CarlaOscData data;
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

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_H

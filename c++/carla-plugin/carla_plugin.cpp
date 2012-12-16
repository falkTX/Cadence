/*
 * Carla Plugin
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

#include "carla_plugin.hpp"
#include "carla_lib_utils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------
// fallback data

static ParameterData    paramDataNull;
static ParameterRanges  paramRangesNull;
static MidiProgramData  midiProgramDataNull;
static CustomData       customDataNull;

// -------------------------------------------------------------------

CarlaPlugin::CarlaPlugin(CarlaEngine* const engine, const unsigned short id)
    : m_id(id),
      x_engine(engine),
      x_client(nullptr),
      x_dryWet(1.0),
      x_volume(1.0),
      x_balanceLeft(-1.0),
      x_balanceRight(1.0)
{
    CARLA_ASSERT(engine);
    CARLA_ASSERT(id < x_engine->maxPluginNumber());
    qDebug("CarlaPlugin::CarlaPlugin(%p, %i)", engine, id);

    m_type  = PLUGIN_NONE;
    m_hints = 0;

    m_active = false;
    m_activeBefore = false;
    m_enabled = false;

    m_lib  = nullptr;
    m_name = nullptr;
    m_filename = nullptr;

    // options
    m_ctrlInChannel = 0;
    m_fixedBufferSize = true;
    m_processHighPrecision = false;

#ifndef BUILD_BRIDGE
    {
        const CarlaEngineOptions& options(x_engine->getOptions());

        m_processHighPrecision = options.processHighPrecision;

        if (options.processMode == PROCESS_MODE_CONTINUOUS_RACK)
            m_ctrlInChannel = m_id;
    }
#endif

    // latency
    m_latency = 0;
    m_latencyBuffers = nullptr;

#ifndef BUILD_BRIDGE
    osc.data.path   = nullptr;
    osc.data.source = nullptr;
    osc.data.target = nullptr;
    osc.thread = nullptr;
#endif
}

CarlaPlugin::~CarlaPlugin()
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

    if (m_latencyBuffers)
    {
        for (uint32_t i=0; i < aIn.count; i++)
            delete[] m_latencyBuffers[i];

        delete[] m_latencyBuffers;
    }
}

// -------------------------------------------------------------------
// Information (base)

PluginType CarlaPlugin::type() const
{
    return m_type;
}

unsigned short CarlaPlugin::id() const
{
    return m_id;
}

unsigned int CarlaPlugin::hints() const
{
    return m_hints;
}

bool CarlaPlugin::enabled() const
{
    return m_enabled;
}

const char* CarlaPlugin::name() const
{
    return m_name;
}

const char* CarlaPlugin::filename() const
{
    return m_filename;
}

PluginCategory CarlaPlugin::category()
{
    return PLUGIN_CATEGORY_NONE;
}

long CarlaPlugin::uniqueId()
{
    return 0;
}

// -------------------------------------------------------------------
// Information (count)

uint32_t CarlaPlugin::audioInCount()
{
    return aIn.count;
}

uint32_t CarlaPlugin::audioOutCount()
{
    return aOut.count;
}

uint32_t CarlaPlugin::midiInCount()
{
    return midi.portMin ? 1 : 0;
}

uint32_t CarlaPlugin::midiOutCount()
{
    return midi.portMout ? 1 : 0;
}

uint32_t CarlaPlugin::parameterCount() const
{
    return param.count;
}

uint32_t CarlaPlugin::parameterScalePointCount(const uint32_t parameterId)
{
    CARLA_ASSERT(parameterId < param.count);

    return 0;
}

uint32_t CarlaPlugin::programCount() const
{
    return prog.count;
}

uint32_t CarlaPlugin::midiProgramCount() const
{
    return midiprog.count;
}

size_t CarlaPlugin::customDataCount() const
{
    return custom.size();
}

// -------------------------------------------------------------------
// Information (current data)

int32_t CarlaPlugin::currentProgram() const
{
    return prog.current;
}

int32_t CarlaPlugin::currentMidiProgram() const
{
    return midiprog.current;
}

const ParameterData* CarlaPlugin::parameterData(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < param.count);

    if (parameterId < param.count)
        return &param.data[parameterId];

    return &paramDataNull;
}

const ParameterRanges* CarlaPlugin::parameterRanges(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < param.count);

    if (parameterId < param.count)
        return &param.ranges[parameterId];

    return &paramRangesNull;
}

bool CarlaPlugin::parameterIsOutput(const uint32_t parameterId) const
{
    CARLA_ASSERT(parameterId < param.count);

    if (parameterId < param.count)
        return (param.data[parameterId].type == PARAMETER_OUTPUT);

    return false;
}

const MidiProgramData* CarlaPlugin::midiProgramData(const uint32_t index) const
{
    CARLA_ASSERT(index < midiprog.count);

    if (index < midiprog.count)
        return &midiprog.data[index];

    return &midiProgramDataNull;
}

const CustomData* CarlaPlugin::customData(const size_t index) const
{
    CARLA_ASSERT(index < custom.size());

    if (index < custom.size())
        return &custom[index];

    return &customDataNull;
}

int32_t CarlaPlugin::chunkData(void** const dataPtr)
{
    CARLA_ASSERT(dataPtr);

    return 0;
}

// -------------------------------------------------------------------
// Information (per-plugin data)

double CarlaPlugin::getParameterValue(const uint32_t parameterId)
{
    CARLA_ASSERT(parameterId < param.count);

    return 0.0;
}

double CarlaPlugin::getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId)
{
    CARLA_ASSERT(parameterId < param.count);
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

    return 0.0;
}

void CarlaPlugin::getLabel(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getMaker(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getCopyright(char* const strBuf)
{
    *strBuf = 0;
}

void CarlaPlugin::getRealName(char* const strBuf)
{
    *strBuf = 0;;
}

void CarlaPlugin::getParameterName(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < param.count);

    *strBuf = 0;
}

void CarlaPlugin::getParameterSymbol(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < param.count);

    *strBuf = 0;
}

void CarlaPlugin::getParameterText(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < param.count);

    *strBuf = 0;
}

void CarlaPlugin::getParameterUnit(const uint32_t parameterId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < param.count);

    *strBuf = 0;
}

void CarlaPlugin::getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf)
{
    CARLA_ASSERT(parameterId < param.count);
    CARLA_ASSERT(scalePointId < parameterScalePointCount(parameterId));

    *strBuf = 0;
}

void CarlaPlugin::getProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < prog.count);

    if (index < prog.count && prog.names[index])
        strncpy(strBuf, prog.names[index], STR_MAX);
    else
        *strBuf = 0;
}

void CarlaPlugin::getMidiProgramName(const uint32_t index, char* const strBuf)
{
    CARLA_ASSERT(index < midiprog.count);

    if (index < midiprog.count && midiprog.data[index].name)
        strncpy(strBuf, midiprog.data[index].name, STR_MAX);
    else
        *strBuf = 0;
}

void CarlaPlugin::getParameterCountInfo(uint32_t* const ins, uint32_t* const outs, uint32_t* const total)
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

void CarlaPlugin::getGuiInfo(GuiType* const type, bool* const resizable)
{
    *type = GUI_NONE;
    *resizable = false;
}

// -------------------------------------------------------------------
// Set data (internal stuff)

#ifndef BUILD_BRIDGE
void CarlaPlugin::setId(const unsigned short id)
{
    m_id = id;

    if (x_engine->getOptions().processMode == PROCESS_MODE_CONTINUOUS_RACK)
        m_ctrlInChannel = id;
}
#endif

void CarlaPlugin::setEnabled(const bool yesNo)
{
    m_enabled = yesNo;
}

void CarlaPlugin::setActive(const bool active, const bool sendOsc, const bool sendCallback)
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
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_ACTIVE, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (m_hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&osc.data, PARAMETER_ACTIVE, value);
#endif
}

void CarlaPlugin::setDryWet(double value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0 && value <= 1.0);

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
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_DRYWET, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (m_hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&osc.data, PARAMETER_DRYWET, value);
#endif
}

void CarlaPlugin::setVolume(double value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= 0.0 && value <= 1.27);

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
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_VOLUME, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (m_hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&osc.data, PARAMETER_VOLUME, value);
#endif
}

void CarlaPlugin::setBalanceLeft(double value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0 && value <= 1.0);

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
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_BALANCE_LEFT, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (m_hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&osc.data, PARAMETER_BALANCE_LEFT, value);
#endif
}

void CarlaPlugin::setBalanceRight(double value, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(value >= -1.0 && value <= 1.0);

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
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, PARAMETER_BALANCE_RIGHT, 0, value, nullptr);
#ifndef BUILD_BRIDGE
    else if (m_hints & PLUGIN_IS_BRIDGE)
        osc_send_control(&osc.data, PARAMETER_BALANCE_RIGHT, value);
#endif
}

#ifndef BUILD_BRIDGE
int CarlaPlugin::setOscBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types)
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

void CarlaPlugin::setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < param.count);

    if (sendGui)
        uiParameterChange(parameterId, value);

#ifndef BUILD_BRIDGE
    if (sendOsc)
        x_engine->osc_send_control_set_parameter_value(m_id, parameterId, value);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, parameterId, 0, value, nullptr);
}

void CarlaPlugin::setParameterValueByRIndex(const int32_t rindex, const double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(rindex >= PARAMETER_BALANCE_RIGHT && rindex != PARAMETER_NULL);

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

void CarlaPlugin::setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < param.count);
    CARLA_ASSERT(channel < 16);

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
        x_engine->callback(CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED, m_id, parameterId, channel, 0.0, nullptr);
}

void CarlaPlugin::setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(parameterId < param.count);
    CARLA_ASSERT(cc >= -1);

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
        x_engine->callback(CALLBACK_PARAMETER_MIDI_CC_CHANGED, m_id, parameterId, cc, 0.0, nullptr);
}

void CarlaPlugin::setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui)
{
    CARLA_ASSERT(type);
    CARLA_ASSERT(key);
    CARLA_ASSERT(value);

    if (! type)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - type is invalid", type, key, value, bool2str(sendGui));

    if (! key)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - key is null", type, key, value, bool2str(sendGui));

    if (! value)
        return qCritical("CarlaPlugin::setCustomData(\"%s\", \"%s\", \"%s\", %s) - value is null", type, key, value, bool2str(sendGui));

    bool saveData = true;

    if (strcmp(type, CUSTOM_DATA_STRING) == 0)
    {
        // Ignore some keys
        if (strncmp(key, "OSC:", 4) == 0 || strcmp(key, "guiVisible") == 0)
            saveData = false;
        else if (strcmp(key, CARLA_BRIDGE_MSG_SAVE_NOW) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CHUNK) == 0 || strcmp(key, CARLA_BRIDGE_MSG_SET_CUSTOM) == 0)
            saveData = false;
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
        newData.type  = strdup(type);
        newData.key   = strdup(key);
        newData.value = strdup(value);
        custom.push_back(newData);
    }
}

void CarlaPlugin::setChunkData(const char* const stringData)
{
    CARLA_ASSERT(stringData);
}

void CarlaPlugin::setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
{
    CARLA_ASSERT(index >= -1 && index < (int32_t)prog.count);

    if (index < -1)
        index = -1;
    else if (index > (int32_t)prog.count)
        return;

    prog.current = index;

    if (sendGui && index >= 0)
        uiProgramChange(index);

    // Change default parameter values
    if (index >= 0)
    {
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = getParameterValue(i);
            fixParameterValue(param.ranges[i].def, param.ranges[i]);

#ifndef BUILD_BRIDGE
            if (sendOsc)
            {
                x_engine->osc_send_control_set_default_value(m_id, i, param.ranges[i].def);
                x_engine->osc_send_control_set_parameter_value(m_id, i, param.ranges[i].def);
            }
#endif
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        x_engine->osc_send_control_set_program(m_id, index);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        x_engine->callback(CALLBACK_PROGRAM_CHANGED, m_id, index, 0, 0.0, nullptr);

    Q_UNUSED(block);
}

void CarlaPlugin::setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
{
    CARLA_ASSERT(index >= -1 && index < (int32_t)midiprog.count);

    if (index < -1)
        index = -1;
    else if (index > (int32_t)midiprog.count)
        return;

    midiprog.current = index;

    if (sendGui && index >= 0)
        uiMidiProgramChange(index);

    // Change default parameter values (sound banks never change defaults)
    if (index >= 0 && m_type != PLUGIN_GIG && m_type != PLUGIN_SF2 && m_type != PLUGIN_SFZ)
    {
        for (uint32_t i=0; i < param.count; i++)
        {
            param.ranges[i].def = getParameterValue(i);
            fixParameterValue(param.ranges[i].def, param.ranges[i]);

#ifndef BUILD_BRIDGE
            if (sendOsc)
            {
                x_engine->osc_send_control_set_default_value(m_id, i, param.ranges[i].def);
                x_engine->osc_send_control_set_parameter_value(m_id, i, param.ranges[i].def);
            }
#endif
        }
    }

#ifndef BUILD_BRIDGE
    if (sendOsc)
        x_engine->osc_send_control_set_midi_program(m_id, index);
#else
    Q_UNUSED(sendOsc);
#endif

    if (sendCallback)
        x_engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, index, 0, 0.0, nullptr);

    Q_UNUSED(block);
}

void CarlaPlugin::setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block)
{
    CARLA_ASSERT(program < 128);

    for (uint32_t i=0; i < midiprog.count; i++)
    {
        if (midiprog.data[i].bank == bank && midiprog.data[i].program == program)
            return setMidiProgram(i, sendGui, sendOsc, sendCallback, block);
    }
}

// -------------------------------------------------------------------
// Set gui stuff

void CarlaPlugin::setGuiContainer(GuiContainer* const container)
{
    Q_UNUSED(container);
}

void CarlaPlugin::showGui(const bool yesNo)
{
    Q_UNUSED(yesNo);
}

void CarlaPlugin::idleGui()
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

void CarlaPlugin::reload()
{
}

void CarlaPlugin::reloadPrograms(const bool init)
{
    Q_UNUSED(init);
}

void CarlaPlugin::prepareForSave()
{
}

// -------------------------------------------------------------------
// Plugin processing

void CarlaPlugin::process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset)
{
    Q_UNUSED(inBuffer);
    Q_UNUSED(outBuffer);
    Q_UNUSED(frames);
    Q_UNUSED(framesOffset);
}

void CarlaPlugin::bufferSizeChanged(const uint32_t newBufferSize)
{
    Q_UNUSED(newBufferSize);
}

void CarlaPlugin::recreateLatencyBuffers()
{
    if (m_latencyBuffers)
    {
        for (uint32_t i=0; i < aIn.count; i++)
            delete[] m_latencyBuffers[i];

        delete[] m_latencyBuffers;
    }

    if (aIn.count > 0 && m_latency > 0)
    {
        m_latencyBuffers = new float* [aIn.count];

        for (uint32_t i=0; i < aIn.count; i++)
            m_latencyBuffers[i] = new float [m_latency];
    }
    else
        m_latencyBuffers = nullptr;
}

// -------------------------------------------------------------------
// OSC stuff

void CarlaPlugin::registerToOscControl()
{
    if (! x_engine->isOscControlRegisted())
        return;

#ifndef BUILD_BRIDGE
    x_engine->osc_send_control_add_plugin_start(m_id, m_name);
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

    // Plugin Parameters
#ifdef BUILD_BRIDGE
    const uint32_t maxParameters = MAX_PARAMETERS;
#else
    const uint32_t maxParameters = x_engine->getOptions().maxParameters;
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
            x_engine->osc_send_control_set_parameter_midi_cc(m_id, i, param.data[i].midiCC);
            x_engine->osc_send_control_set_parameter_midi_channel(m_id, i, param.data[i].midiChannel);
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

#ifndef BUILD_BRIDGE
    x_engine->osc_send_control_add_plugin_end(m_id);

    // Internal Parameters
    {
        x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_ACTIVE, m_active ? 1.0 : 0.0);
        x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_DRYWET, x_dryWet);
        x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_VOLUME, x_volume);
        x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_LEFT, x_balanceLeft);
        x_engine->osc_send_control_set_parameter_value(m_id, PARAMETER_BALANCE_RIGHT, x_balanceRight);
    }
#endif
}

#ifndef BUILD_BRIDGE
void CarlaPlugin::updateOscData(const lo_address source, const char* const url)
{
    // FIXME - remove debug prints later
    qWarning("CarlaPlugin::updateOscData(%p, \"%s\")", source, url);

    const char* host;
    const char* port;
    const int proto = lo_address_get_protocol(source);

    osc.data.free();

    host = lo_address_get_hostname(source);
    port = lo_address_get_port(source);
    osc.data.source = lo_address_new_with_proto(proto, host, port);
    qWarning("CarlaPlugin::updateOscData() - source: host \"%s\", port \"%s\"", host, port);

    host = lo_url_get_hostname(url);
    port = lo_url_get_port(url);
    osc.data.path   = lo_url_get_path(url);
    osc.data.target = lo_address_new_with_proto(proto, host, port);
    qWarning("CarlaPlugin::updateOscData() - target: host \"%s\", port \"%s\", path \"%s\"", host, port, osc.data.path);

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

    qWarning("CarlaPlugin::updateOscData() - done");
}

void CarlaPlugin::freeOscData()
{
    osc.data.free();
}

bool CarlaPlugin::waitForOscGuiShow()
{
    qWarning("CarlaPlugin::waitForOscGuiShow()");

    const uint oscUiTimeout = x_engine->getOptions().oscUiTimeout;

    // wait for UI 'update' call
    for (uint i=0; i < oscUiTimeout; i++)
    {
        if (osc.data.target)
        {
            qWarning("CarlaPlugin::waitForOscGuiShow() - got response, asking UI to show itself now");
            osc_send_show(&osc.data);
            return true;
        }
        else
            carla_msleep(100);
    }

    qWarning("CarlaPlugin::waitForOscGuiShow() - Timeout while waiting for UI to respond (waited %u msecs)", oscUiTimeout);
    return false;
}
#endif

// -------------------------------------------------------------------
// MIDI events

void CarlaPlugin::sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback)
{
    CARLA_ASSERT(channel < 16);
    CARLA_ASSERT(note < 128);
    CARLA_ASSERT(velo < 128);

    if (! m_active)
        return;

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
        x_engine->callback(velo ? CALLBACK_NOTE_ON : CALLBACK_NOTE_OFF, m_id, channel, note, velo, nullptr);
}

void CarlaPlugin::sendMidiAllNotesOff()
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

        postEvents.data[i + postPad].type   = PluginPostEventNoteOff;
        postEvents.data[i + postPad].value1 = m_ctrlInChannel;
        postEvents.data[i + postPad].value2 = i;
        postEvents.data[i + postPad].value3 = 0.0;
    }

    postEvents.mutex.unlock();
    engineMidiUnlock();
}

// -------------------------------------------------------------------
// Post-poned events

void CarlaPlugin::postponeEvent(const PluginPostEventType type, const int32_t value1, const int32_t value2, const double value3, const void* const cdata)
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

void CarlaPlugin::postEventsRun()
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
            x_engine->callback(CALLBACK_DEBUG, m_id, event->value1, event->value2, event->value3, nullptr);
            break;

        case PluginPostEventParameterChange:
            // Update UI
            if (event->value1 >= 0)
                uiParameterChange(event->value1, event->value3);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (x_engine->isOscControlRegisted())
                x_engine->osc_send_control_set_parameter_value(m_id, event->value1, event->value3);
#endif

            // Update Host
            x_engine->callback(CALLBACK_PARAMETER_VALUE_CHANGED, m_id, event->value1, 0, event->value3, nullptr);
            break;

        case PluginPostEventProgramChange:
            // Update UI
            if (event->value1 >= 0)
                uiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (x_engine->isOscControlRegisted())
            {
                x_engine->osc_send_control_set_program(m_id, event->value1);

                for (uint32_t j=0; j < param.count; j++)
                    x_engine->osc_send_control_set_default_value(m_id, j, param.ranges[j].def);
            }
#endif

            // Update Host
            x_engine->callback(CALLBACK_PROGRAM_CHANGED, m_id, event->value1, 0, 0.0, nullptr);
            break;

        case PluginPostEventMidiProgramChange:
            // Update UI
            if (event->value1 >= 0)
                uiMidiProgramChange(event->value1);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (x_engine->isOscControlRegisted())
            {
                x_engine->osc_send_control_set_midi_program(m_id, event->value1);

                for (uint32_t j=0; j < param.count; j++)
                    x_engine->osc_send_control_set_default_value(m_id, j, param.ranges[j].def);
            }
#endif

            // Update Host
            x_engine->callback(CALLBACK_MIDI_PROGRAM_CHANGED, m_id, event->value1, 0, 0.0, nullptr);
            break;

        case PluginPostEventNoteOn:
            // Update UI
            uiNoteOn(event->value1, event->value2, rint(event->value3));

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (x_engine->isOscControlRegisted())
                x_engine->osc_send_control_note_on(m_id, event->value1, event->value2, rint(event->value3));
#endif

            // Update Host
            x_engine->callback(CALLBACK_NOTE_ON, m_id, event->value1, event->value2, rint(event->value3), nullptr);
            break;

        case PluginPostEventNoteOff:
            // Update UI
            uiNoteOff(event->value1, event->value2);

#ifndef BUILD_BRIDGE
            // Update OSC control client
            if (x_engine->isOscControlRegisted())
                x_engine->osc_send_control_note_off(m_id, event->value1, event->value2);
#endif

            // Update Host
            x_engine->callback(CALLBACK_NOTE_OFF, m_id, event->value1, event->value2, 0.0, nullptr);
            break;

        case PluginPostEventCustom:
            // Handle custom event
            postEventHandleCustom(event->value1, event->value2, event->value3, event->cdata);
            break;
        }
    }
}

void CarlaPlugin::postEventHandleCustom(const int32_t value1, const int32_t value2, const double value3, const void* const cdata)
{
    Q_UNUSED(value1);
    Q_UNUSED(value2);
    Q_UNUSED(value3);
    Q_UNUSED(cdata);
}

void CarlaPlugin::uiParameterChange(const uint32_t index, const double value)
{
    CARLA_ASSERT(index < param.count);

    Q_UNUSED(value);
}

void CarlaPlugin::uiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < prog.count);
}

void CarlaPlugin::uiMidiProgramChange(const uint32_t index)
{
    CARLA_ASSERT(index < midiprog.count);
}

void CarlaPlugin::uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
{
    CARLA_ASSERT(channel < 16);
    CARLA_ASSERT(note < 128);
    CARLA_ASSERT(velo > 0 && velo < 128);
}

void CarlaPlugin::uiNoteOff(const uint8_t channel, const uint8_t note)
{
    CARLA_ASSERT(channel < 16);
    CARLA_ASSERT(note < 128);
}

// -------------------------------------------------------------------
// Cleanup

void CarlaPlugin::removeClientPorts()
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

void CarlaPlugin::initBuffers()
{
    for (uint32_t i=0; i < aIn.count; i++)
    {
        if (aIn.ports[i])
            aIn.ports[i]->initBuffer(x_engine);
    }

    for (uint32_t i=0; i < aOut.count; i++)
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

void CarlaPlugin::deleteBuffers()
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

bool CarlaPlugin::libOpen(const char* const filename)
{
    m_lib = lib_open(filename);
    return bool(m_lib);
}

bool CarlaPlugin::libClose()
{
    if (m_lib)
        return lib_close(m_lib);
    return false;
}

void* CarlaPlugin::libSymbol(const char* const symbol)
{
    if (m_lib)
        return lib_symbol(m_lib, symbol);
    return nullptr;
}

const char* CarlaPlugin::libError(const char* const filename)
{
    return lib_error(filename);
}

// -------------------------------------------------------------------
// Locks

void CarlaPlugin::engineProcessLock()
{
    x_engine->processLock();
}

void CarlaPlugin::engineProcessUnlock()
{
    x_engine->processUnlock();
}

void CarlaPlugin::engineMidiLock()
{
    x_engine->midiLock();
}

void CarlaPlugin::engineMidiUnlock()
{
    x_engine->midiUnlock();
}

CARLA_BACKEND_END_NAMESPACE

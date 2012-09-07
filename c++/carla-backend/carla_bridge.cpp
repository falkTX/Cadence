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

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#define CARLA_BRIDGE_CHECK_OSC_TYPES(/* argc, types, */ argcToCompare, typesToCompare)                                    \
    /* check argument count */                                                                                            \
    if (argc != argcToCompare)                                                                                            \
    {                                                                                                                     \
        qCritical("BridgePlugin::%s() - argument count mismatch: %i != %i", __FUNCTION__, argc, argcToCompare);           \
        return 1;                                                                                                         \
    }                                                                                                                     \
    if (argc > 0)                                                                                                         \
    {                                                                                                                     \
        /* check for nullness */                                                                                          \
        if (! (types && typesToCompare))                                                                                  \
        {                                                                                                                 \
            qCritical("BridgePlugin::%s() - argument types are null", __FUNCTION__);                                      \
            return 1;                                                                                                     \
        }                                                                                                                 \
        /* check argument types */                                                                                        \
        if (strcmp(types, typesToCompare) != 0)                                                                           \
        {                                                                                                                 \
            qCritical("BridgePlugin::%s() - argument types mismatch: '%s' != '%s'", __FUNCTION__, types, typesToCompare); \
            return 1;                                                                                                     \
        }                                                                                                                 \
    }

CARLA_BACKEND_START_NAMESPACE

struct BridgeParamInfo {
    double value;
    QString name;
    QString unit;

    BridgeParamInfo()
        : value(0.0) {}
};

class BridgePlugin : public CarlaPlugin
{
public:
    BridgePlugin(CarlaEngine* const engine, const unsigned short id, const BinaryType btype, const PluginType ptype)
        : CarlaPlugin(engine, id),
          m_binary(btype)
    {
        qDebug("BridgePlugin::BridgePlugin()");

        m_type   = ptype;
        m_hints  = PLUGIN_IS_BRIDGE;

        m_initiated = false;
        m_saved = false;

        info.aIns  = 0;
        info.aOuts = 0;
        info.mIns  = 0;
        info.mOuts = 0;

        info.category = PLUGIN_CATEGORY_NONE;
        info.uniqueId = 0;

        info.name  = nullptr;
        info.label = nullptr;
        info.maker = nullptr;
        info.copyright = nullptr;

        params = nullptr;

        osc.thread = new CarlaPluginThread(engine, this, CarlaPluginThread::PLUGIN_THREAD_BRIDGE);
    }

    ~BridgePlugin()
    {
        qDebug("BridgePlugin::~BridgePlugin()");

        if (osc.data.target)
        {
            osc_send_hide(&osc.data);
            osc_send_quit(&osc.data);
            osc_clear_data(&osc.data);
        }

        if (osc.thread)
        {
            // Wait a bit first, try safe quit, then force kill
            if (osc.thread->isRunning() && ! osc.thread->wait(3000))
            {
                qWarning("Failed to properly stop Plugin Bridge thread");
                osc.thread->terminate();
            }

            delete osc.thread;
        }

        if (info.name)
            free((void*)info.name);

        if (info.label)
            free((void*)info.label);

        if (info.maker)
            free((void*)info.maker);

        if (info.copyright)
            free((void*)info.copyright);

        info.chunk.clear();
    }

    // -------------------------------------------------------------------
    // Information (base)

    PluginCategory category()
    {
        return info.category;
    }

    long uniqueId()
    {
        return info.uniqueId;
    }

    // -------------------------------------------------------------------
    // Information (count)

    uint32_t audioInCount()
    {
        return info.aIns;
    }

    uint32_t audioOutCount()
    {
        return info.aOuts;
    }

    uint32_t midiInCount()
    {
        return info.mIns;
    }

    uint32_t midiOutCount()
    {
        return info.mOuts;
    }

    // -------------------------------------------------------------------
    // Information (current data)

    int32_t chunkData(void** const dataPtr)
    {
        Q_ASSERT(dataPtr);

        if (! info.chunk.isEmpty())
        {
            *dataPtr = info.chunk.data();
            return info.chunk.size();
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    double getParameterValue(const uint32_t parameterId)
    {
        Q_ASSERT(parameterId < param.count);

        return params[parameterId].value;
    }

    void getLabel(char* const strBuf)
    {
        if (info.label)
            strncpy(strBuf, info.label, STR_MAX);
        else
            CarlaPlugin::getLabel(strBuf);
    }

    void getMaker(char* const strBuf)
    {
        if (info.maker)
            strncpy(strBuf, info.maker, STR_MAX);
        else
            CarlaPlugin::getMaker(strBuf);
    }

    void getCopyright(char* const strBuf)
    {
        if (info.copyright)
            strncpy(strBuf, info.copyright, STR_MAX);
        else
            CarlaPlugin::getCopyright(strBuf);
    }

    void getRealName(char* const strBuf)
    {
        if (info.name)
            strncpy(strBuf, info.name, STR_MAX);
        else
            CarlaPlugin::getRealName(strBuf);
    }

    void getParameterName(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        strncpy(strBuf, params[parameterId].name.toUtf8().constData(), STR_MAX);
    }

    void getParameterUnit(const uint32_t parameterId, char* const strBuf)
    {
        Q_ASSERT(parameterId < param.count);

        strncpy(strBuf, params[parameterId].unit.toUtf8().constData(), STR_MAX);
    }

    void getGuiInfo(GuiType* const type, bool* const resizable)
    {
        if (m_hints & PLUGIN_HAS_GUI)
            *type = GUI_EXTERNAL_OSC;
        else
            *type = GUI_NONE;
        *resizable = false;
    }

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    int setOscBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types)
    {
        qDebug("setOscBridgeInfo(%i, %i, %p, \"%s\")", type, argc, argv, types);

        switch (type)
        {
        case PluginBridgeAudioCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t aIns   = argv[0]->i;
            const int32_t aOuts  = argv[1]->i;
            const int32_t aTotal = argv[2]->i;

            info.aIns  = aIns;
            info.aOuts = aOuts;

            break;
            Q_UNUSED(aTotal);
        }

        case PluginBridgeMidiCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t mIns   = argv[0]->i;
            const int32_t mOuts  = argv[1]->i;
            const int32_t mTotal = argv[2]->i;

            info.mIns  = mIns;
            info.mOuts = mOuts;

            break;
            Q_UNUSED(mTotal);
        }

        case PluginBridgeParameterCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iii");

            const int32_t pIns   = argv[0]->i;
            const int32_t pOuts  = argv[1]->i;
            const int32_t pTotal = argv[2]->i;

            // delete old data
            if (param.count > 0)
            {
                delete[] param.data;
                delete[] param.ranges;
                delete[] params;
            }

            // create new if needed
            param.count = (pTotal < (int32_t)carlaOptions.maxParameters) ? pTotal : 0;

            if (param.count > 0)
            {
                param.data   = new ParameterData[param.count];
                param.ranges = new ParameterRanges[param.count];
                params       = new BridgeParamInfo[param.count];
            }
            else
            {
                param.data = nullptr;
                param.ranges = nullptr;
                params = nullptr;
            }

            break;
            Q_UNUSED(pIns);
            Q_UNUSED(pOuts);
        }

        case PluginBridgeProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            // Delete old programs
            if (prog.count > 0)
            {
                for (uint32_t i=0; i < prog.count; i++)
                {
                    if (prog.names[i])
                        free((void*)prog.names[i]);
                }

                delete[] prog.names;
            }

            prog.count = 0;
            prog.names = nullptr;

            // Query new programs
            prog.count = count;

            if (prog.count > 0)
                prog.names = new const char* [prog.count];

            // Update names (NULL)
            for (uint32_t i=0; i < prog.count; i++)
                prog.names[i] = nullptr;

            break;
        }

        case PluginBridgeMidiProgramCount:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t count = argv[0]->i;

            // Delete old programs
            if (midiprog.count > 0)
            {
                for (uint32_t i=0; i < midiprog.count; i++)
                {
                    if (midiprog.data[i].name)
                        free((void*)midiprog.data[i].name);
                }

                delete[] midiprog.data;
            }

            midiprog.count = 0;
            midiprog.data  = nullptr;

            // Query new programs
            midiprog.count = count;

            if (midiprog.count > 0)
                midiprog.data = new midi_program_t [midiprog.count];

            break;
        }

        case PluginBridgePluginInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "iissssh");

            const int32_t category  = argv[0]->i;
            const int32_t hints     = argv[1]->i;
            const char* const name  = (const char*)&argv[2]->s;
            const char* const label = (const char*)&argv[3]->s;
            const char* const maker = (const char*)&argv[4]->s;
            const char* const copyright = (const char*)&argv[5]->s;
            const int64_t uniqueId      = argv[6]->i;

            Q_ASSERT(name);
            Q_ASSERT(label);
            Q_ASSERT(maker);
            Q_ASSERT(copyright);

            m_hints = hints | PLUGIN_IS_BRIDGE;
            info.category = (PluginCategory)category;
            info.uniqueId = uniqueId;

            info.name  = strdup(name);
            info.label = strdup(label);
            info.maker = strdup(maker);
            info.copyright = strdup(copyright);

            if (! m_name)
                m_name = x_engine->getUniqueName(name);

            break;
        }

        case PluginBridgeParameterInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(3, "iss");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;
            const char* const unit = (const char*)&argv[2]->s;

            Q_ASSERT(index >= 0 && index < (int32_t)param.count);
            Q_ASSERT(name);
            Q_ASSERT(unit);

            if (index >= 0 && index < (int32_t)param.count)
            {
                params[index].name = QString(name);
                params[index].unit = QString(unit);
            }

            break;
        }

        case PluginBridgeParameterData:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(6, "iiiiii");

            const int32_t index   = argv[0]->i;
            const int32_t type    = argv[1]->i;
            const int32_t rindex  = argv[2]->i;
            const int32_t hints   = argv[3]->i;
            const int32_t channel = argv[4]->i;
            const int32_t cc      = argv[5]->i;

            Q_ASSERT(index >= 0 && index < (int32_t)param.count);

            if (index >= 0 && index < (int32_t)param.count)
            {
                param.data[index].type    = (ParameterType)type;
                param.data[index].index   = index;
                param.data[index].rindex  = rindex;
                param.data[index].hints   = hints;
                param.data[index].midiChannel = channel;
                param.data[index].midiCC  = cc;
            }

            break;
        }

        case PluginBridgeParameterRanges:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(7, "idddddd");

            const int32_t index = argv[0]->i;
            const double def    = argv[1]->d;
            const double min    = argv[2]->d;
            const double max    = argv[3]->d;
            const double step   = argv[4]->d;
            const double stepSmall = argv[5]->d;
            const double stepLarge = argv[6]->d;

            Q_ASSERT(index >= 0 && index < (int32_t)param.count);

            if (index >= 0 && index < (int32_t)param.count)
            {
                param.ranges[index].def  = def;
                param.ranges[index].min  = min;
                param.ranges[index].max  = max;
                param.ranges[index].step = step;
                param.ranges[index].stepSmall = stepSmall;
                param.ranges[index].stepLarge = stepLarge;
                params[index].value = def;
            }

            break;
        }

        case PluginBridgeProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "is");

            const int32_t index    = argv[0]->i;
            const char* const name = (const char*)&argv[1]->s;

            Q_ASSERT(name);

            if (index >= 0 && index < (int32_t)prog.count)
            {
                if (prog.names[index])
                    free((void*)prog.names[index]);

                prog.names[index] = strdup(name);
            }

            break;
        }

        case PluginBridgeMidiProgramInfo:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(4, "iiis");

            const int32_t index    = argv[0]->i;
            const int32_t bank     = argv[1]->i;
            const int32_t program  = argv[2]->i;
            const char* const name = (const char*)&argv[3]->s;

            Q_ASSERT(name);

            if (index >= 0 && index < (int32_t)midiprog.count)
            {
                if (midiprog.data[index].name)
                    free((void*)midiprog.data[index].name);

                midiprog.data[index].bank    = bank;
                midiprog.data[index].program = program;
                midiprog.data[index].name    = strdup(name);
            }

            break;
        }

        case PluginBridgeSetParameterValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "id");

            const int32_t index = argv[0]->i;
            const double  value = argv[1]->d;

            setParameterValueByRIndex(index, value, false, true, true);

            break;
        }

        case PluginBridgeSetDefaultValue:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(2, "id");

            const int32_t index = argv[0]->i;
            const double  value = argv[1]->d;


            Q_ASSERT(index >= 0 && index < (int32_t)param.count);

            if (index >= 0 && index < (int32_t)param.count)
                param.ranges[index].def = value;

            break;
        }

        case PluginBridgeSetProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            setProgram(index, false, true, true, true);

            break;
        }

        case PluginBridgeSetMidiProgram:
        {
            CARLA_BRIDGE_CHECK_OSC_TYPES(1, "i");

            const int32_t index = argv[0]->i;

            setMidiProgram(index, false, true, true, true);

            break;
        }

        case PluginBridgeSetCustomData:
        {
//            const char* stype = (const char*)&argv[0]->s;
//            const char* key   = (const char*)&argv[1]->s;
//            const char* value = (const char*)&argv[2]->s;

//            setCustomData(getCustomDataStringType(stype), key, value, false);

            break;
        }

        case PluginBridgeSetChunkData:
        {
//            const char* const filePath = (const char*)&argv[0]->s;
//            QFile file(filePath);

//            if (file.open(QIODevice::ReadOnly))
//            {
//                info.chunk = file.readAll();
//                file.remove();
//            }

            break;
        }

        case PluginBridgeUpdateNow:
            m_initiated = true;
            break;

        case PluginBridgeSaved:
            m_saved = true;
            break;
        }

        return 0;
    }

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback)
    {
        Q_ASSERT(parameterId < param.count);

        params[parameterId].value = fixParameterValue(value, param.ranges[parameterId]);

        CarlaPlugin::setParameterValue(parameterId, value, sendGui, sendOsc, sendCallback);
    }

    void setCustomData(const CustomDataType type, const char* const key, const char* const value, const bool sendGui)
    {
        Q_ASSERT(key);
        Q_ASSERT(value);

        if (sendGui)
        {
            QString cData;
            cData += getCustomDataTypeString(type);
            cData += "·";
            cData += key;
            cData += "·";
            cData += value;
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CUSTOM, cData.toUtf8().constData());
        }

        CarlaPlugin::setCustomData(type, key, value, sendGui);
    }

    void setChunkData(const char* const stringData)
    {
        Q_ASSERT(m_hints & PLUGIN_USES_CHUNKS);
        Q_ASSERT(stringData);

        QString filePath;
        filePath += "/tmp/.CarlaChunk_"; // FIXME - cross-platform
        filePath += m_name;

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << stringData;
            file.close();
            osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SET_CHUNK, filePath.toUtf8().constData());
        }
    }

    // -------------------------------------------------------------------
    // Set gui stuff

    void showGui(const bool yesNo)
    {
        if (yesNo)
            osc_send_show(&osc.data);
        else
            osc_send_hide(&osc.data);
    }

    // -------------------------------------------------------------------
    // Plugin state

    void prepareForSave()
    {
        m_saved = false;
        osc_send_configure(&osc.data, CARLA_BRIDGE_MSG_SAVE_NOW, "");

        for (int i=0; i < 200; i++)
        {
            if (m_saved)
                break;
            carla_msleep(50);
        }

        if (! m_saved)
            qWarning("BridgePlugin::prepareForSave() - Timeout while requesting save state");
        else
            qDebug("BridgePlugin::prepareForSave() - success!");
    }

    // -------------------------------------------------------------------
    // Post-poned events

    void uiParameterChange(const uint32_t index, const double value)
    {
        Q_ASSERT(index < param.count);

        if (index >= param.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_control(&osc.data, param.data[index].rindex, value);
    }

    void uiProgramChange(const uint32_t index)
    {
        Q_ASSERT(index < prog.count);

        if (index >= prog.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_program(&osc.data, index);
    }

    void uiMidiProgramChange(const uint32_t index)
    {
        Q_ASSERT(index < midiprog.count);

        if (index >= midiprog.count)
            return;
        if (! osc.data.target)
            return;

        osc_send_midi_program(&osc.data, index);
    }

    void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);
        Q_ASSERT(velo > 0 && velo < 128);

        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_ON + channel;
        midiData[2] = note;
        midiData[3] = velo;
        osc_send_midi(&osc.data, midiData);
    }

    void uiNoteOff(const uint8_t channel, const uint8_t note)
    {
        Q_ASSERT(channel < 16);
        Q_ASSERT(note < 128);

        if (! osc.data.target)
            return;

        uint8_t midiData[4] = { 0 };
        midiData[1] = MIDI_STATUS_NOTE_OFF + channel;
        midiData[2] = note;
        osc_send_midi(&osc.data, midiData);
    }

    // -------------------------------------------------------------------
    // Cleanup

    void deleteBuffers()
    {
        qDebug("BridgePlugin::delete_buffers() - start");

        if (param.count > 0)
            delete[] params;

        params = nullptr;

        qDebug("BridgePlugin::delete_buffers() - end");
    }

    // -------------------------------------------------------------------

    bool init(const char* const filename, const char* const name, const char* const label)
    {
        const char* const bridgeBinary = getBinaryBidgePath(m_binary);

        if (! bridgeBinary)
        {
            setLastError("Bridge not possible, bridge-binary not found");
            return false;
        }

        m_filename = strdup(filename);

        if (name)
            m_name = x_engine->getUniqueName(name);

        // register plugin now so we can receive OSC (and wait for it)
        x_engine->__bridgePluginRegister(m_id, this);

        osc.thread->setOscData(bridgeBinary, label, getPluginTypeString(m_type));
        osc.thread->start();

        for (int i=0; i < 200; i++)
        {
            if (m_initiated)
                break;
            carla_msleep(50);
        }

        if (! m_initiated)
        {
            // unregister so it gets handled properly
            x_engine->__bridgePluginRegister(m_id, nullptr);

            osc.thread->terminate();
            setLastError("Timeout while waiting for a response from plugin-bridge");
            return false;
        }

        return true;
    }

private:
    const BinaryType m_binary;

    bool m_initiated;
    bool m_saved;

    struct {
        uint32_t aIns, aOuts;
        uint32_t mIns, mOuts;
        PluginCategory category;
        long uniqueId;
        const char* name;
        const char* label;
        const char* maker;
        const char* copyright;
        QByteArray chunk;
    } info;

    BridgeParamInfo* params;
};

CarlaPlugin* CarlaPlugin::newBridge(const initializer& init, BinaryType btype, PluginType ptype)
{
    qDebug("CarlaPlugin::newBridge(%p, \"%s\", \"%s\", \"%s\", %s, %s)", init.engine, init.filename, init.name, init.label, BinaryType2str(btype), PluginType2str(ptype));

    short id = init.engine->getNewPluginId();

    if (id < 0 || id > CarlaEngine::maxPluginNumber())
    {
        setLastError("Maximum number of plugins reached");
        return nullptr;
    }

    BridgePlugin* const plugin = new BridgePlugin(init.engine, id, btype, ptype);

    if (! plugin->init(init.filename, init.name, init.label))
    {
        delete plugin;
        return nullptr;
    }

    //plugin->reload();
    plugin->registerToOsc();

    return plugin;
}

CARLA_BACKEND_END_NAMESPACE

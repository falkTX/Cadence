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

#include <cstdio>

#ifndef __WINE__
#include <QtGui/QDialog>
#endif

#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"

#if VESTIGE_HEADER
#warning Using vestige header
#define effFlagsProgramChunks (1 << 5)
#define effGetParamLabel 6
#define effGetParamDisplay 7
#define effGetChunk 23
#define effSetChunk 24
#define effCanBeAutomated 26
#define effGetProgramNameIndexed 29
#define effGetPlugCategory 35
#define effSetBlockSizeAndSampleRate 43
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#define kVstProcessPrecision32 0
#define kVstTransportChanged 1
#define kVstVersion 2400
struct VstTimeInfo_R {
    double samplePos, sampleRate, nanoSeconds, ppqPos, tempo, barStartPos, cycleStartPos, cycleEndPos;
    int32_t timeSigNumerator, timeSigDenominator, smpteOffset, smpteFrameRate, samplesToNextClock, flags;
};
#else
typedef VstTimeInfo VstTimeInfo_R;
#endif

typedef AEffect* (*VST_Function)(audioMasterCallback);

bool VstPluginCanDo(AEffect* effect, const char* feature)
{
    return (effect->dispatcher(effect, effCanDo, 0, 0, (void*)feature, 0.0f) == 1);
}

const char* VstOpcode2str(int32_t opcode)
{
    switch (opcode)
    {
    case audioMasterAutomate:
        return "audioMasterAutomate";
    case audioMasterVersion:
        return "audioMasterVersion";
    case audioMasterCurrentId:
        return "audioMasterCurrentId";
    case audioMasterIdle:
        return "audioMasterIdle";
#if ! VST_FORCE_DEPRECATED
    case audioMasterPinConnected:
        return "audioMasterPinConnected";
    case audioMasterWantMidi:
        return "audioMasterWantMidi";
#endif
    case audioMasterGetTime:
        return "audioMasterGetTime";
    case audioMasterProcessEvents:
        return "audioMasterProcessEvents";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetTime:
        return "audioMasterSetTime";
    case audioMasterTempoAt:
        return "audioMasterTempoAt";
    case audioMasterGetNumAutomatableParameters:
        return "audioMasterGetNumAutomatableParameters";
    case audioMasterGetParameterQuantization:
        return "audioMasterGetParameterQuantization";
#endif
    case audioMasterIOChanged:
        return "audioMasterIOChanged";
#if ! VST_FORCE_DEPRECATED
    case audioMasterNeedIdle:
        return "audioMasterNeedIdle";
#endif
    case audioMasterSizeWindow:
        return "audioMasterSizeWindow";
    case audioMasterGetSampleRate:
        return "audioMasterGetSampleRate";
    case audioMasterGetBlockSize:
        return "audioMasterGetBlockSize";
    case audioMasterGetInputLatency:
        return "audioMasterGetInputLatency";
    case audioMasterGetOutputLatency:
        return "audioMasterGetOutputLatency";
#if ! VST_FORCE_DEPRECATED
    case audioMasterGetPreviousPlug:
        return "audioMasterGetPreviousPlug";
    case audioMasterGetNextPlug:
        return "audioMasterGetNextPlug";
    case audioMasterWillReplaceOrAccumulate:
        return "audioMasterWillReplaceOrAccumulate";
#endif
    case audioMasterGetCurrentProcessLevel:
        return "audioMasterGetCurrentProcessLevel";
    case audioMasterGetAutomationState:
        return "audioMasterGetAutomationState";
    case audioMasterOfflineStart:
        return "audioMasterOfflineStart";
    case audioMasterOfflineRead:
        return "audioMasterOfflineRead";
    case audioMasterOfflineWrite:
        return "audioMasterOfflineWrite";
    case audioMasterOfflineGetCurrentPass:
        return "audioMasterOfflineGetCurrentPass";
    case audioMasterOfflineGetCurrentMetaPass:
        return "audioMasterOfflineGetCurrentMetaPass";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetOutputSampleRate:
        return "audioMasterSetOutputSampleRate";
#ifdef VESTIGE_HEADER
    case audioMasterGetSpeakerArrangement:
#else
    case audioMasterGetOutputSpeakerArrangement:
#endif
        return "audioMasterGetOutputSpeakerArrangement";
#endif
    case audioMasterGetVendorString:
        return "audioMasterGetVendorString";
    case audioMasterGetProductString:
        return "audioMasterGetProductString";
    case audioMasterGetVendorVersion:
        return "audioMasterGetVendorVersion";
    case audioMasterVendorSpecific:
        return "audioMasterVendorSpecific";
#if ! VST_FORCE_DEPRECATED
    case audioMasterSetIcon:
        return "audioMasterSetIcon";
#endif
    case audioMasterCanDo:
        return "audioMasterCanDo";
    case audioMasterGetLanguage:
        return "audioMasterGetLanguage";
#if ! VST_FORCE_DEPRECATED
    case audioMasterOpenWindow:
        return "audioMasterOpenWindow";
    case audioMasterCloseWindow:
        return "audioMasterCloseWindow";
#endif
    case audioMasterGetDirectory:
        return "audioMasterGetDirectory";
    case audioMasterUpdateDisplay:
        return "audioMasterUpdateDisplay";
    case audioMasterBeginEdit:
        return "audioMasterBeginEdit";
    case audioMasterEndEdit:
        return "audioMasterEndEdit";
    case audioMasterOpenFileSelector:
        return "audioMasterOpenFileSelector";
    case audioMasterCloseFileSelector:
        return "audioMasterCloseFileSelector";
#if ! VST_FORCE_DEPRECATED
    case audioMasterEditFile:
        return "audioMasterEditFile";
    case audioMasterGetChunkFile:
        return "audioMasterGetChunkFile";
    case audioMasterGetInputSpeakerArrangement:
        return "audioMasterGetInputSpeakerArrangement";
#endif
    default:
        return "unknown";
    }
}

#if 1
short add_plugin_vst(const char* filename, const char* label);

int main()
{
    short id = add_plugin_vst("/usr/lib/vst/PingPongPan.so", "eq5");
    set_active(id, true);
    carla_sleep(3);
    remove_plugin(id);
    return 0;
}
#endif

class VstPlugin : public CarlaPlugin
{
public:
    VstPlugin() : CarlaPlugin()
    {
        qDebug("VstPlugin::VstPlugin()");
        m_type = PLUGIN_VST;

        effect = nullptr;
        events.numEvents = 0;
        events.reserved  = 0;

        gui.visible = false;
        gui.width  = 0;
        gui.height = 0;

        memset(midi_events, 0, sizeof(VstMidiEvent)*MAX_MIDI_EVENTS*2);

        for (unsigned short i=0; i < MAX_MIDI_EVENTS*2; i++)
            events.data[i] = (VstEvent*)&midi_events[i];

        // make plugin valid
        unique1 = unique2 = random();
    }

    ~VstPlugin()
    {
        qDebug("VstPlugin::~VstPlugin()");

        // plugin is no longer valid
        unique1 = 0;
        unique2 = 1;

        if (effect)
        {
            // close UI
            if (m_hints & PLUGIN_HAS_GUI)
                effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);

            if (m_active_before)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            }

            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        }
    }

    PluginCategory category()
    {
        intptr_t VstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        switch (VstCategory)
        {
        case kPlugCategSynth:
            return PLUGIN_CATEGORY_SYNTH;
        case kPlugCategAnalysis:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategMastering:
            return PLUGIN_CATEGORY_DYNAMICS;
        case kPlugCategRoomFx:
            return PLUGIN_CATEGORY_DELAY;
        case kPlugCategRestoration:
            return PLUGIN_CATEGORY_UTILITY;
        case kPlugCategGenerator:
            return PLUGIN_CATEGORY_SYNTH;
        }

        if (effect->flags & effFlagsIsSynth)
            return PLUGIN_CATEGORY_SYNTH;

        return get_category_from_name(m_name);
    }

    long unique_id()
    {
        return effect->uniqueID;
    }

    int32_t chunk_data(void** data_ptr)
    {
        return effect->dispatcher(effect, effGetChunk, 0 /* bank */, 0, data_ptr, 0.0f);
    }

    double get_parameter_value(uint32_t param_id)
    {
        return effect->getParameter(effect, param_id);
    }

    void get_label(char* buf_str)
    {
        effect->dispatcher(effect, effGetProductString, 0, 0, buf_str, 0.0f);
    }

    void get_maker(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    void get_copyright(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    void get_real_name(char* buf_str)
    {
        effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
    }

    void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        effect->dispatcher(effect, effGetParamName, param_id, 0, buf_str, 0.0f);
    }

    void get_parameter_unit(uint32_t param_id, char* buf_str)
    {
        effect->dispatcher(effect, effGetParamLabel, param_id, 0, buf_str, 0.0f);
    }

    void get_parameter_text(uint32_t param_id, char* buf_str)
    {
        effect->dispatcher(effect, effGetParamDisplay, param_id, 0, buf_str, 0.0f);
    }

    void get_gui_info(GuiInfo* info)
    {
#ifndef __WINE__
        if (effect->flags & effFlagsHasEditor)
            info->type = GUI_INTERNAL_QT4;
        else
#endif
            info->type = GUI_NONE;
        info->resizable = false;
    }

    void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        fix_parameter_value(value, param.ranges[param_id]);
        effect->setParameter(effect, param_id, value);
        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_chunk_data(const char* string_data)
    {
        static QByteArray chunk;
        chunk = QByteArray::fromBase64(string_data);
        effect->dispatcher(effect, effSetChunk, 0 /* bank */, chunk.size(), chunk.data(), 0.0f);
    }

    void set_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        // TODO - don't block when freewheeling
        if (index >= 0)
        {
            // block plugin so we don't change programs during processReplacing()
            short _id = m_id;

            if (block)
            {
                carla_proc_lock();
                m_id = -1;
                carla_proc_unlock();
            }

            effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);

            if (block)
            {
                carla_proc_lock();
                m_id = _id;
                carla_proc_unlock();
            }
        }

        CarlaPlugin::set_program(index, gui_send, osc_send, callback_send, block);
    }

    void set_gui_data(int data, void* ptr)
    {
#ifdef __WINE__
        if (effect->dispatcher(effect, effEditOpen, 0, data, ptr, 0.0f) == 1)
#else
        if (effect->dispatcher(effect, effEditOpen, 0, data, (void*)((QDialog*)ptr)->winId(), 0.0f) == 1)
#endif
        {
#ifndef ERect
            struct ERect {
                short top;
                short left;
                short bottom;
                short right;
            };
#endif
            ERect* vst_rect;

            if (effect->dispatcher(effect, effEditGetRect, 0, 0, &vst_rect, 0.0f))
            {
                int width  = vst_rect->right  - vst_rect->left;
                int height = vst_rect->bottom - vst_rect->top;

                if (width <= 0 || height <= 0)
                {
                    qCritical("Failed to get proper Plugin Window size");
                    return;
                }

                gui.width  = width;
                gui.height = height;
            }
            else
                qCritical("Failed to get Plugin Window size");
        }
        else
        {
            // failed to open UI
            m_hints &= ~PLUGIN_HAS_GUI;
            callback_action(CALLBACK_SHOW_GUI, m_id, -1, 0, 0.0);

            effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
        }
    }

    void show_gui(bool yesno)
    {
        gui.visible = yesno;

        if (gui.visible && gui.width > 0 && gui.height > 0)
            callback_action(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);
    }

    void idle_gui()
    {
        //effect->dispatcher(effect, effIdle, 0, 0, nullptr, 0.0f);

        // FIXME
        if (gui.visible)
            effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
    }

    void reload()
    {
        qDebug("VstPlugin::reload() - start");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        remove_from_jack(bool(_id >= 0));

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, mins, mouts, params, j;
        ains = aouts = mins = mouts = params = 0;

        ains   = effect->numInputs;
        aouts  = effect->numOutputs;
        params = effect->numParams;

        if (VstPluginCanDo(effect, "receiveVstEvents") || VstPluginCanDo(effect, "receiveVstMidiEvent") || (effect->flags & effFlagsIsSynth) > 0)
            mins = 1;

        if (VstPluginCanDo(effect, "sendVstEvents") || VstPluginCanDo(effect, "sendVstMidiEvent"))
            mouts = 1;

        if (ains > 0)
        {
            ain.ports    = new jack_port_t*[ains];
            ain.rindexes = new uint32_t[ains];
        }

        if (aouts > 0)
        {
            aout.ports    = new jack_port_t*[aouts];
            aout.rindexes = new uint32_t[aouts];
        }

        if (params > 0)
        {
            param.data    = new ParameterData[params];
            param.ranges  = new ParameterRanges[params];
        }

        const int port_name_size = jack_port_name_size() - 1;
        char port_name[port_name_size];
        bool needs_cin = (aouts > 0 || params > 0);

        for (j=0; j<ains; j++)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
                sprintf(port_name, "%s:input_%02i", m_name, j+1);
            else
#endif
                sprintf(port_name, "input_%02i", j+1);

            ain.ports[j]    = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            ain.rindexes[j] = j;
        }

        for (j=0; j<aouts; j++)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
                sprintf(port_name, "%s:output_%02i", m_name, j+1);
            else
#endif
                sprintf(port_name, "output_%02i", j+1);

            aout.ports[j]    = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
            aout.rindexes[j] = j;
        }

        for (j=0; j<params; j++)
        {
            param.data[j].type   = PARAMETER_INPUT;
            param.data[j].index  = j;
            param.data[j].rindex = j;
            param.data[j].hints  = 0;
            param.data[j].midi_channel = 0;
            param.data[j].midi_cc = -1;

            double min, max, def, step, step_small, step_large;

            VstParameterProperties prop;
            prop.flags = 0;

            if (effect->dispatcher(effect, effGetParameterProperties, j, 0, &prop, 0))
            {
                if (prop.flags & kVstParameterUsesIntegerMinMax)
                {
                    min = prop.minInteger;
                    max = prop.maxInteger;
                }
                else
                {
                    min = 0.0;
                    max = 1.0;
                }

                if (prop.flags & kVstParameterIsSwitch)
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                    param.data[j].hints |= PARAMETER_IS_BOOLEAN;
                }
                else if (prop.flags & kVstParameterUsesIntStep)
                {
                    step = prop.stepInteger;
                    step_small = prop.stepInteger;
                    step_large = prop.largeStepInteger;
                    param.data[j].hints |= PARAMETER_IS_INTEGER;
                }
                else if (prop.flags & kVstParameterUsesFloatStep)
                {
                    step = prop.stepFloat;
                    step_small = prop.smallStepFloat;
                    step_large = prop.largeStepFloat;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }

                if (prop.flags & kVstParameterCanRamp)
                    param.data[j].hints |= PARAMETER_IS_LOGARITHMIC;
            }
            else
            {
                min = 0.0;
                max = 1.0;
                step = 0.001;
                step_small = 0.0001;
                step_large = 0.1;
            }

            if (min > max)
                max = min;
            else if (max < min)
                min = max;

            if (max - min == 0.0)
            {
                qWarning("Broken plugin parameter: max - min == 0");
                max = min + 0.1;
            }

            // no such thing as VST default parameters
            def = effect->getParameter(effect, j);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            param.ranges[j].min = min;
            param.ranges[j].max = max;
            param.ranges[j].def = def;
            param.ranges[j].step = step;
            param.ranges[j].step_small = step_small;
            param.ranges[j].step_large = step_large;

            param.data[j].hints |= PARAMETER_IS_ENABLED;
            param.data[j].hints |= PARAMETER_USES_CUSTOM_TEXT;

            if (effect->dispatcher(effect, effCanBeAutomated, j, 0, nullptr, 0.0f) != 0)
                param.data[j].hints |= PARAMETER_IS_AUTOMABLE;
        }

        if (needs_cin)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":control-in");
            }
            else
#endif
                strcpy(port_name, "control-in");

            param.port_cin = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        if (mins == 1)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":midi-in");
            }
            else
#endif
                strcpy(port_name, "midi-in");

            midi.port_min = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        }

        if (mouts == 1)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
            {
                strcpy(port_name, m_name);
                strcat(port_name, ":midi-out");
            }
            else
#endif
                strcpy(port_name, "midi-out");

            midi.port_mout = jack_port_register(jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
        }

        ain.count   = ains;
        aout.count  = aouts;
        param.count = params;

        reload_programs(true);

        // plugin checks
        m_hints &= ~(PLUGIN_IS_SYNTH | PLUGIN_USES_CHUNKS | PLUGIN_CAN_DRYWET | PLUGIN_CAN_VOLUME | PLUGIN_CAN_BALANCE);

        intptr_t VstCategory = effect->dispatcher(effect, effGetPlugCategory, 0, 0, nullptr, 0.0f);

        if (VstCategory == kPlugCategSynth || VstCategory == kPlugCategGenerator)
            m_hints |= PLUGIN_IS_SYNTH;

        if (effect->flags & effFlagsProgramChunks)
            m_hints |= PLUGIN_USES_CHUNKS;

        if (aouts > 0 && (ains == aouts || ains == 1))
            m_hints |= PLUGIN_CAN_DRYWET;

        if (aouts > 0)
            m_hints |= PLUGIN_CAN_VOLUME;

        if (aouts >= 2 && aouts%2 == 0)
            m_hints |= PLUGIN_CAN_BALANCE;

        carla_proc_lock();
        m_id = _id;
        carla_proc_unlock();

#ifndef BUILD_BRIDGE
        if (carla_options.global_jack_client == false)
#endif
            jack_activate(jack_client);

        qDebug("VstPlugin::reload() - end");
    }

    void reload_programs(bool init)
    {
        qDebug("VstPlugin::reload_programs(%s)", bool2str(init));
        uint32_t i, old_count = prog.count;

        // Delete old programs
        if (prog.count > 0)
        {
            for (uint32_t i=0; i < prog.count; i++)
                free((void*)prog.names[i]);

            delete[] prog.names;
        }

        prog.count = 0;
        prog.names = nullptr;

        // Query new programs
        prog.count = effect->numPrograms;

        if (prog.count > 0)
            prog.names = new const char* [prog.count];

        // Update names
        for (i=0; i < prog.count; i++)
        {
            char buf_str[STR_MAX] = { 0 };
            if (effect->dispatcher(effect, effGetProgramNameIndexed, i, 0, buf_str, 0.0f) != 1)
            {
                // program will be [re-]changed later
                effect->dispatcher(effect, effSetProgram, 0, i, nullptr, 0.0f);
                effect->dispatcher(effect, effGetProgramName, 0, 0, buf_str, 0.0f);
            }
            prog.names[i] = strdup(buf_str);
        }

#ifndef BUILD_BRIDGE
        // Update OSC Names
        osc_global_send_set_program_count(m_id, prog.count);

        for (i=0; i < prog.count; i++)
            osc_global_send_set_program_name(m_id, i, prog.names[i]);

        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);
#endif

        if (init)
        {
            if (prog.count > 0)
                set_program(0, false, false, false, true);
        }
        else
        {
            callback_action(CALLBACK_UPDATE, m_id, 0, 0, 0.0);

            // Check if current program is invalid
            bool program_changed = false;

            if (prog.count == old_count+1)
            {
                // one program added, probably created by user
                prog.current = old_count;
                program_changed = true;
            }
            else if (prog.current >= (int32_t)prog.count)
            {
                // current program > count
                prog.current = 0;
                program_changed = true;
            }
            else if (prog.current < 0 && prog.count > 0)
            {
                // programs exist now, but not before
                prog.current = 0;
                program_changed = true;
            }
            else if (prog.current >= 0 && prog.count == 0)
            {
                // programs existed before, but not anymore
                prog.current = -1;
                program_changed = true;
            }

            if (program_changed)
            {
                set_program(prog.current, true, true, true, true);
            }
            else
            {
                // Program was changed during update, re-set it
                if (prog.current >= 0)
                    effect->dispatcher(effect, effSetProgram, 0, prog.current, nullptr, 0.0f);
            }
        }
    }

    void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        uint32_t midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_audio_sample_t* ains_buffer[ain.count];
        jack_audio_sample_t* aouts_buffer[aout.count];
        void* min_buffer  = nullptr;
        void* mout_buffer = nullptr;

        // reset MIDI
        events.numEvents = 0;
        midi_events[0].type = 0;

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        if (midi.port_min > 0)
            min_buffer = jack_port_get_buffer(midi.port_min, nframes);

        if (midi.port_mout > 0)
            mout_buffer = jack_port_get_buffer(midi.port_mout, nframes);

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            if (ain.count == 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                }
            }
            else if (ain.count >= 1)
            {
                for (k=0; k < nframes; k++)
                {
                    if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                        ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);

                    if (abs_d(ains_buffer[1][k]) > ains_peak_tmp[1])
                        ains_peak_tmp[1] = abs_d(ains_buffer[1][k]);
                }
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin)
        {
            void* pin_buffer = jack_port_get_buffer(param.port_cin, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            for (i=0; i < n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                jack_midi_data_t status  = pin_event.buffer[0];
                jack_midi_data_t channel = status & 0x0F;

                // Control change
                if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
                {
                    jack_midi_data_t control = pin_event.buffer[1];
                    jack_midi_data_t c_value = pin_event.buffer[2];

                    double value;

                    // Control GUI stuff (channel 0 only)
                    if (channel == cin_channel)
                    {
                        if (MIDI_IS_CONTROL_BREATH_CONTROLLER(control) && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            value = double(c_value)/127;
                            set_drywet(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_CHANNEL_VOLUME(control) && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            value = double(c_value)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                            continue;
                        }
                        else if (MIDI_IS_CONTROL_BALANCE(control) && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            double left, right;
                            value = (double(c_value)-63.5)/63.5;

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

                            set_balance_left(left, false, false);
                            set_balance_right(right, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_LEFT, left);
                            postpone_event(PostEventParameterChange, PARAMETER_BALANCE_RIGHT, right);
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();

                            if (m_active && m_active_before)
                            {
                                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);

                                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
                            }
                            continue;
                        }
                        else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                        {
                            if (midi.port_min)
                                send_midi_all_notes_off();
                            continue;
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].midi_channel == channel && param.data[k].midi_cc == control && param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0)
                        {
                            if (param.data[k].hints & PARAMETER_IS_BOOLEAN)
                            {
                                value = c_value <= 63 ? param.ranges[k].min : param.ranges[k].max;
                            }
                            else
                            {
                                value = (double(c_value) / 127 * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;

                                if (param.data[k].hints & PARAMETER_IS_INTEGER)
                                    value = rint(value);
                            }

                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(status))
                {
                    uint32_t prog_id = pin_event.buffer[1]; // & 0x7F;

                    if (prog_id < prog.count)
                    {
                        set_program(prog_id, false, false, false, false);
                        postpone_event(PostEventMidiProgramChange, prog_id, 0.0);
                    }
                }
            }
        } // End of Parameters Input

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (External)

        if (midi.port_min)
        {
            carla_midi_lock();

            for (i=0; i < MAX_MIDI_EVENTS && midi_event_count < MAX_MIDI_EVENTS; i++)
            {
                if (ext_midi_notes[i].valid)
                {
                    VstMidiEvent* midi_event = &midi_events[midi_event_count];
                    memset(midi_event, 0, sizeof(VstMidiEvent));

                    midi_event->type = kVstMidiType;
                    midi_event->byteSize = sizeof(VstMidiEvent);
                    midi_event->midiData[0] = ext_midi_notes[i].onoff ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF;
                    midi_event->midiData[1] = ext_midi_notes[i].note;
                    midi_event->midiData[2] = ext_midi_notes[i].velo;

                    ext_midi_notes[i].valid = false;
                    midi_event_count += 1;
                }
                else
                    break;
            }

            carla_midi_unlock();

        } // End of MIDI Input (External)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Input (JACK)

        if (midi.port_min)
        {
            jack_midi_event_t min_event;
            uint32_t n_min_events = jack_midi_get_event_count(min_buffer);

            for (k=0; k < n_min_events && midi_event_count < MAX_MIDI_EVENTS; k++)
            {
                if (jack_midi_event_get(&min_event, min_buffer, k) != 0)
                    break;

                jack_midi_data_t status = min_event.buffer[0];

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(status) && min_event.buffer[2] == 0)
                {
                    min_event.buffer[0] -= 0x10;
                    status = min_event.buffer[0];
                }

                VstMidiEvent* midi_event = &midi_events[midi_event_count];
                memset(midi_event, 0, sizeof(VstMidiEvent));

                midi_event->type = kVstMidiType;
                midi_event->byteSize = sizeof(VstMidiEvent);
                midi_event->deltaFrames = min_event.time;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    jack_midi_data_t note = min_event.buffer[1];

                    midi_event->midiData[0] = status;
                    midi_event->midiData[1] = note;
                    postpone_event(PostEventNoteOff, note, 0.0);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    jack_midi_data_t note = min_event.buffer[1];
                    jack_midi_data_t velo = min_event.buffer[2];

                    midi_event->midiData[0] = status;
                    midi_event->midiData[1] = note;
                    midi_event->midiData[2] = velo;
                    postpone_event(PostEventNoteOn, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    jack_midi_data_t note     = min_event.buffer[1];
                    jack_midi_data_t pressure = min_event.buffer[2];

                    midi_event->midiData[0] = status;
                    midi_event->midiData[1] = note;
                    midi_event->midiData[2] = pressure;
                }
                else if (MIDI_IS_STATUS_AFTERTOUCH(status))
                {
                    jack_midi_data_t pressure = min_event.buffer[1];

                    midi_event->midiData[0] = status;
                    midi_event->midiData[1] = pressure;
                }
                else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
                {
                    jack_midi_data_t lsb = min_event.buffer[1];
                    jack_midi_data_t msb = min_event.buffer[2];

                    midi_event->midiData[0] = status;
                    midi_event->midiData[1] = lsb;
                    midi_event->midiData[2] = msb;
                }
                else
                    continue;

                midi_event_count += 1;
            }
        } // End of MIDI Input (JACK)

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // VST Events

        if (midi_event_count > 0)
        {
            events.numEvents = midi_event_count;
            events.reserved  = 0;
            effect->dispatcher(effect, effProcessEvents, 0, 0, &events, 0.0f);
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Plugin processing

        if (m_active)
        {
            if (m_active_before == false)
            {
                effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
            }

            // FIXME - make this a global option
            // don't process if not needed
            //if ((effect->flags & effFlagsNoSoundInStop) > 0 && ains_peak_tmp[0] == 0 && ains_peak_tmp[1] == 0 && midi_event_count == 0 && ! midi.port_mout)
            //{
            if (effect->flags & effFlagsCanReplacing)
            {
                effect->processReplacing(effect, ains_buffer, aouts_buffer, nframes);
            }
            else
            {
                for (i=0; i < aout.count; i++)
                    memset(aouts_buffer[i], 0, sizeof(jack_audio_sample_t)*nframes);

#if ! VST_FORCE_DEPRECATED
                effect->process(effect, ains_buffer, aouts_buffer, nframes);
#endif
            }
            //}
        }
        else
        {
            if (m_active_before)
            {
                effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
                effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
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
            jack_audio_sample_t old_bal_left[do_balance ? nframes : 0];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                if (do_drywet || do_volume)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (do_drywet)
                        {
                            if (aout.count == 1)
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                            else
                                aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                        }

                        if (do_volume)
                            aouts_buffer[i][k] *= x_vol;
                    }
                }

                // Balance
                if (do_balance)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_audio_sample_t)*nframes);

                    bal_rangeL = (x_bal_left+1.0)/2;
                    bal_rangeR = (x_bal_right+1.0)/2;

                    for (k=0; k<nframes; k++)
                    {
                        if (i%2 == 0)
                        {
                            // left output
                            aouts_buffer[i][k]  = old_bal_left[k]*(1.0-bal_rangeL);
                            aouts_buffer[i][k] += aouts_buffer[i+1][k]*(1.0-bal_rangeR);
                        }
                        else
                        {
                            // right
                            aouts_buffer[i][k]  = aouts_buffer[i][k]*bal_rangeR;
                            aouts_buffer[i][k] += old_bal_left[k]*bal_rangeL;
                        }
                    }
                }

                // Output VU
                for (k=0; k < nframes && i < 2; k++)
                {
                    if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                        aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output

        if (midi.port_mout)
        {
            jack_midi_clear_buffer(mout_buffer);

            for (i = events.numEvents; i < MAX_MIDI_EVENTS*2; i++)
            {
                if (midi_events[i].type != kVstMidiType)
                    break;

                // Fix bad note-off
                if (MIDI_IS_STATUS_NOTE_ON(midi_events[i].midiData[0]) && midi_events[i].midiData[2] == 0)
                {
                    midi_events[i].midiData[0] -= 0x10;
                }

                jack_midi_event_write(mout_buffer, midi_events[i].deltaFrames, (const jack_midi_data_t*)midi_events[i].midiData, 3);
            }

        } // End of MIDI Output

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Peak Values

        ains_peak[(plugin_id*2)+0]  = ains_peak_tmp[0];
        ains_peak[(plugin_id*2)+1]  = ains_peak_tmp[1];
        aouts_peak[(plugin_id*2)+0] = aouts_peak_tmp[0];
        aouts_peak[(plugin_id*2)+1] = aouts_peak_tmp[1];

        m_active_before = m_active;
    }

    void buffer_size_changed(jack_nframes_t new_buffer_size)
    {
        if (m_active)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
        }

#if ! VST_FORCE_DEPRECATED
        effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, new_buffer_size, nullptr, get_sample_rate());
#endif
        effect->dispatcher(effect, effSetBlockSize, 0, new_buffer_size, nullptr, 0.0f);

        if (m_active)
        {
            effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f);
            effect->dispatcher(effect, effStartProcess, 0, 0, nullptr, 0.0f);
        }
    }

    bool init(const char* filename, const char* label)
    {
        if (lib_open(filename))
        {
            VST_Function vstfn = (VST_Function)lib_symbol("VSTPluginMain");

            if (vstfn == nullptr)
            {
                if (vstfn == nullptr)
                {
#ifdef TARGET_API_MAC_CARBON

                    vstfn = (VST_Function)lib_symbol("main_macho");
                    if (vstfn == nullptr)
#endif
                        vstfn = (VST_Function)lib_symbol("main");
                }
            }

            if (vstfn)
            {
                effect = vstfn(VstHostCallback);

                if (effect && effect->magic == kEffectMagic)
                {
                    m_filename = strdup(filename);

                    char buf_str[STR_MAX] = { 0 };
                    effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);

                    if (buf_str[0] != 0)
                        m_name = get_unique_name(buf_str);
                    else
                        m_name = get_unique_name(label);

                    // Init plugin
                    effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
#if ! VST_FORCE_DEPRECATED
                    effect->dispatcher(effect, effSetBlockSizeAndSampleRate, 0, get_buffer_size(), nullptr, get_sample_rate());
#endif
                    effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, get_sample_rate());
                    effect->dispatcher(effect, effSetBlockSize, 0, get_buffer_size(), nullptr, 0.0f);
                    effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, nullptr, 0.0f);
                    effect->user = this;

                    if (carla_jack_register_plugin(this, &jack_client))
                    {
                        // GUI Stuff
                        if (effect->flags & effFlagsHasEditor)
                            m_hints |= PLUGIN_HAS_GUI;

                        return true;
                    }
                    else
                        set_last_error("Failed to register plugin in JACK");
                }
                else
                    set_last_error("Plugin failed to initialize");
            }
            else
                set_last_error("Could not find the VST main entry in the plugin library");
        }
        else
            set_last_error(lib_error());

        return false;
    }

    static intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
    {
        qDebug("VstHostCallback() - code: %s, index: %i, value: " P_INTPTR ", opt: %f", VstOpcode2str(opcode), index, value, opt);

        // Check if 'user' points to this plugin
        VstPlugin* self = nullptr;

        if (effect && effect->user)
        {
            self = (VstPlugin*)effect->user;
            if (self->unique1 != self->unique2)
                self = nullptr;
        }

        switch (opcode)
        {
        case audioMasterAutomate:
            if (self)
                self->set_parameter_value(index, opt, false, true, true);
            break;

        case audioMasterVersion:
            return kVstVersion;

        case audioMasterCurrentId:
            return 0; // TODO

        case audioMasterIdle:
            if (effect)
                effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterWantMidi: // TODO
            // Deprecated in VST SDK 2.4
#if 0
            if (plugin && plugin->jack_client && plugin->min.count == 0)
            {
                i = plugin->id;
                bool unlock = carla_proc_trylock();
                plugin->id = -1;
                if (unlock) carla_proc_unlock();

                const int port_name_size = jack_port_name_size();
                char port_name[port_name_size];

#ifndef BUILD_BRIDGE
                if (carla_options.global_jack_client)
                {
                    strncpy(port_name, plugin->name, (port_name_size/2)-2);
                    strcat(port_name, ":midi-in");
                }
                else
#endif
                    strcpy(port_name, "midi-in");

                plugin->min.count    = 1;
                plugin->min.ports    = new jack_port_t*[1];
                plugin->min.ports[0] = jack_port_register(plugin->jack_client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

                plugin->id = i;
#ifndef BRIDGE_WINVST
                callback_action(CALLBACK_RELOAD_INFO, plugin->id, 0, 0, 0.0f);
#endif
            }
#endif
            break;
#endif

        case audioMasterGetTime:
        {
            static VstTimeInfo_R timeInfo;
            memset(&timeInfo, 0, sizeof(VstTimeInfo_R));

            jack_position_t* jack_pos;
            bool playing = carla_jack_transport_query(&jack_pos);

            timeInfo.flags |= kVstTransportChanged;

            if (playing)
                timeInfo.flags |= kVstTransportPlaying;

            if (jack_pos->unique_1 == jack_pos->unique_2)
            {
                timeInfo.samplePos  = jack_pos->frame;
                timeInfo.sampleRate = jack_pos->frame_rate;

                if (jack_pos->valid & JackPositionBBT)
                {
                    // Tempo
                    timeInfo.tempo  = jack_pos->beats_per_minute;
                    timeInfo.flags |= kVstTempoValid;

                    // Time Signature
                    timeInfo.timeSigNumerator   = jack_pos->beats_per_bar;
                    timeInfo.timeSigDenominator = jack_pos->beat_type;
                    timeInfo.flags |= kVstTimeSigValid;

                    // Position
                    double dPos = timeInfo.samplePos / timeInfo.sampleRate;
                    timeInfo.barStartPos = 0;
                    timeInfo.nanoSeconds = dPos * 1000.0;
                    timeInfo.ppqPos = dPos * timeInfo.tempo / 60.0;
                    timeInfo.flags |= kVstBarsValid|kVstNanosValid|kVstPpqPosValid;
                }
            }
            else
                timeInfo.sampleRate = get_sample_rate();

            return (intptr_t)&timeInfo;
        }

        case audioMasterProcessEvents:
            if (self && self->midi.port_mout && ptr)
            {
                int32_t i;
                const VstEvents* const events = (VstEvents*)ptr;

                for (i=0; i < events->numEvents && self->events.numEvents+i < MAX_MIDI_EVENTS*2; i++)
                {
                    const VstMidiEvent* const midi_event = (VstMidiEvent*)events->events[i];

                    if (midi_event && midi_event->type == kVstMidiType)
                        memcpy(&self->midi_events[self->events.numEvents+i], midi_event, sizeof(VstMidiEvent));
                }

                self->midi_events[self->events.numEvents+i].type = 0;
            }
            else
                qDebug("VstHostCallback:audioMasterProcessEvents - Some MIDI Out events were ignored");

            break;

#if ! VST_FORCE_DEPRECATED
#if 0
        case audioMasterSetTime:
            // Deprecated in VST SDK 2.4
            break;
#endif
#endif

#if ! VST_FORCE_DEPRECATED
        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            jack_position_t* jack_pos;
            carla_jack_transport_query(&jack_pos);

            if (jack_pos->unique_1 == jack_pos->unique_2 && (jack_pos->valid & JackPositionBBT) > 0)
                return jack_pos->beats_per_minute * 10000;

            return 120.0 * 10000;

        case audioMasterGetNumAutomatableParameters:
            // Deprecated in VST SDK 2.4
            return MAX_PARAMETERS;

#if 0
        case audioMasterGetParameterQuantization:
            // Deprecated in VST SDK 2.4
            break;
#endif
#endif

#if 0
        case audioMasterIOChanged:
            if (self && self->m_id >= 0)
            {
                short _id = self->m_id;

                carla_proc_lock();
                plugin->m_id = -1;
                carla_proc_unlock();

                if (self->m_active)
                {
                    self->effect->dispatcher(self->effect, effStopProcess, 0, 0, nullptr, 0.0f);
                    self->effect->dispatcher(self->effect, effMainsChanged, 0, 0, nullptr, 0.0f);
                }

                self->reload();

                if (self->m_active)
                {
                    self->effect->dispatcher(self->effect, effMainsChanged, 0, 1, nullptr, 0.0f);
                    self->effect->dispatcher(self->effect, effStartProcess, 0, 0, nullptr, 0.0f);
                }

                self->m_id = _id;

#ifndef BUILD_BRIDGE
                callback_action(CALLBACK_RELOAD_ALL, plugin->id, 0, 0, 0.0);
#endif
            }
            break;

        case audioMasterNeedIdle:
            // Deprecated in VST SDK 2.4
            break;
#endif

        case audioMasterSizeWindow:
            if (self)
            {
                self->gui.width  = index;
                self->gui.height = value;
#ifndef BUILD_BRIDGE
                callback_action(CALLBACK_RESIZE_GUI, self->id(), index, value, 0.0);
#endif
            }
            return 1;

        case audioMasterGetSampleRate:
            return get_sample_rate();

        case audioMasterGetBlockSize:
            return get_buffer_size();

#if 0
        case audioMasterGetInputLatency:
            return 0;

        case audioMasterGetOutputLatency:
            return 0;

        case audioMasterGetPreviousPlug:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetNextPlug:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterWillReplaceOrAccumulate:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetCurrentProcessLevel:
            // TODO
            return 0;

        case audioMasterGetAutomationState:
            // TODO
            return 0;

        case audioMasterOfflineStart:
        case audioMasterOfflineRead:
        case audioMasterOfflineWrite:
        case audioMasterOfflineGetCurrentPass:
        case audioMasterOfflineGetCurrentMetaPass:
            // TODO
            break;

        case audioMasterSetOutputSampleRate:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetOutputSpeakerArrangement:
            // Deprecated in VST SDK 2.4
            break;
#endif

        case audioMasterGetVendorString:
            strcpy((char*)ptr, "falkTX");
            break;

        case audioMasterGetProductString:
            strcpy((char*)ptr, "Carla");
            break;

        case audioMasterGetVendorVersion:
            return 0x05; // 0.5

        case audioMasterVendorSpecific:
            break;

#if ! VST_FORCE_DEPRECATED
        case audioMasterSetIcon:
            // Deprecated in VST SDK 2.4
            break;
#endif

        case audioMasterCanDo:
#if DEBUG
            qDebug("VstHostCallback:audioMasterCanDo - %s", (char*)ptr);
#endif

            if (strcmp((char*)ptr, "supplyIdle") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstEvents") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
                return 1;
            if (strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime") == 0)
                return -1;
            if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstEvents") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
                return 1;
            if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
                return -1;
            if (strcmp((char*)ptr, "reportConnectionChanges") == 0)
                return -1;
            if (strcmp((char*)ptr, "acceptIOChanges") == 0)
                return 1;
            if (strcmp((char*)ptr, "sizeWindow") == 0)
                return 1;
            if (strcmp((char*)ptr, "offline") == 0)
                return -1;
            if (strcmp((char*)ptr, "openFileSelector") == 0)
                return -1;
            if (strcmp((char*)ptr, "closeFileSelector") == 0)
                return -1;
            if (strcmp((char*)ptr, "startStopProcess") == 0)
                return 1;
            if (strcmp((char*)ptr, "supportShell") == 0)
                return -1;
            if (strcmp((char*)ptr, "shellCategory") == 0)
                return -1;

            // unimplemented
            qWarning("VstHostCallback:audioMasterCanDo - Got uninplemented feature request '%s'", (char*)ptr);
            return 0;

        case audioMasterGetLanguage:
            return kVstLangEnglish;

#if 0
        case audioMasterOpenWindow:
        case audioMasterCloseWindow:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetDirectory:
            // TODO
            break;
#endif

        case audioMasterUpdateDisplay:
            if (self)
            {
                // Update current program name
                if (self->prog.count > 0 && self->prog.current >= 0)
                {
                    char buf_str[STR_MAX] = { 0 };
                    self->effect->dispatcher(self->effect, effGetProgramName, 0, 0, buf_str, 0.0f);

                    if (buf_str[0] != 0 && !(self->prog.names[self->prog.current] && strcmp(buf_str, self->prog.names[self->prog.current]) == 0))
                    {
                        if (self->prog.names[self->prog.current])
                            free((void*)self->prog.names[self->prog.current]);

                        self->prog.names[self->prog.current] = strdup(buf_str);
                    }
                }

#ifndef BUILD_BRIDGE
                // Tell backend to update
                callback_action(CALLBACK_UPDATE, self->id(), 0, 0, 0.0);
#endif
            }
            break;

#if 0
        case audioMasterBeginEdit:
        case audioMasterEndEdit:
            // TODO
            break;

        case audioMasterOpenFileSelector:
        case audioMasterCloseFileSelector:
            // TODO
            break;

        case audioMasterEditFile:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetChunkFile:
            // Deprecated in VST SDK 2.4
            break;

        case audioMasterGetInputSpeakerArrangement:
            // Deprecated in VST SDK 2.4
            break;
#endif

        default:
            //qDebug("VstHostCallback() - code: %s, index: %i, value: " P_INTPTR ", opt: %f", VstOpcode2str(opcode), index, value, opt);
            break;
        }

        return 0;
    }

private:
    long unique1;
    AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_MIDI_EVENTS*2];
    } events;
    VstMidiEvent midi_events[MAX_MIDI_EVENTS*2];

    struct {
        bool visible;
        int width;
        int height;
    } gui;
    long unique2;
};

short add_plugin_vst(const char* filename, const char* label)
{
    qDebug("add_plugin_vst(%s, %s)", filename, label);

    short id = get_new_plugin_id();

    if (id >= 0)
    {
        VstPlugin* plugin = new VstPlugin;

        if (plugin->init(filename, label))
        {
            plugin->reload();
            plugin->set_id(id);

            unique_names[id] = plugin->name();
            CarlaPlugins[id] = plugin;

#ifndef BUILD_BRIDGE
            plugin->osc_global_register_new();
#endif
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

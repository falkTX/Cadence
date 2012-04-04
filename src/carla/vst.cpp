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
#include <QtGui/QDialog>

#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"

#if VESTIGE_HEADER
#warning Using vestige header
#define kVstVersion 2400
#define effGetPlugCategory 35
#define effSetBlockSizeAndSampleRate 43
#define effShellGetNextPlugin 70
#define effStartProcess 71
#define effStopProcess 72
#define effSetProcessPrecision 77
#define kVstProcessPrecision32 0
#define kPlugCategSynth 2
#define kPlugCategAnalysis 3
#define kPlugCategMastering 4
#define kPlugCategRoomFx 6
#define kPlugCategRestoration 8
#define kPlugCategShell 10
#define kPlugCategGenerator 11
#endif

typedef AEffect* (*VST_Function)(audioMasterCallback);

bool VstPluginCanDo(AEffect* effect, const char* feature)
{
    return (effect->dispatcher(effect, effCanDo, 0, 0, (void*)feature, 0.0f) == 1);
}

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

        // FIXME?
        memset(midi_events, 0, sizeof(VstMidiEvent)*MAX_MIDI_EVENTS);

        for (unsigned short i=0; i<MAX_MIDI_EVENTS; i++)
            events.data[i] = (VstEvent*)&midi_events[i];
    }

    virtual ~VstPlugin()
    {
        qDebug("VstPlugin::~VstPlugin()");

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

    virtual PluginCategory category()
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

        // TODO - try to get category from label
        return PLUGIN_CATEGORY_NONE;
    }

    virtual long unique_id()
    {
        return effect->uniqueID;
    }

    virtual int32_t chunk_data(void** data_ptr)
    {
        return effect->dispatcher(effect, effGetChunk, 0 /* bank */, 0, data_ptr, 0.0f);
    }

    virtual double get_parameter_value(uint32_t param_id)
    {
        return effect->getParameter(effect, param_id);
    }

    virtual void get_label(char* buf_str)
    {
        effect->dispatcher(effect, effGetProductString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_maker(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_copyright(char* buf_str)
    {
        effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    }

    virtual void get_real_name(char* buf_str)
    {
        effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);
    }

    virtual void get_parameter_name(uint32_t param_id, char* buf_str)
    {
        effect->dispatcher(effect, effGetParamName, param_id, 0, buf_str, 0.0f);
    }

    virtual void get_parameter_label(uint32_t param_id, char* buf_str)
    {
        effect->dispatcher(effect, effGetParamLabel, param_id, 0, buf_str, 0.0f);
    }

    virtual void get_gui_info(GuiInfo* info)
    {
        if (effect->flags & effFlagsHasEditor)
            info->type = GUI_INTERNAL_QT4;
        else
            info->type = GUI_NONE;
    }

    virtual void set_parameter_value(uint32_t param_id, double value, bool gui_send, bool osc_send, bool callback_send)
    {
        effect->setParameter(effect, param_id, value);
        CarlaPlugin::set_parameter_value(param_id, value, gui_send, osc_send, callback_send);
    }

    void set_chunk_data(const char* string_data)
    {
        QByteArray chunk = QByteArray::fromBase64(string_data);
        effect->dispatcher(effect, effSetChunk, 0 /* bank */, chunk.size(), chunk.data(), 0.0f);
    }

    virtual void set_program(int32_t index, bool gui_send, bool osc_send, bool callback_send, bool block)
    {
        // TODO - go for id -1 so we don't block audio
        if (index >= 0)
        {
            if (block) carla_proc_lock();
            effect->dispatcher(effect, effSetProgram, 0, index, nullptr, 0.0f);
            if (block) carla_proc_unlock();
        }

        CarlaPlugin::set_program(index, gui_send, osc_send, callback_send, block);
    }

    virtual void set_gui_data(int data, void* ptr)
    {
        if (effect->dispatcher(effect, effEditOpen, 0, data, (void*)((QDialog*)ptr)->winId(), 0.0f) == 1)
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

    virtual void show_gui(bool yesno)
    {
        gui.visible = yesno;

        if (gui.visible && gui.width > 0 && gui.height > 0)
            callback_action(CALLBACK_RESIZE_GUI, m_id, gui.width, gui.height, 0.0);
    }

    virtual void idle_gui()
    {
        //effect->dispatcher(effect, effIdle, 0, 0, nullptr, 0.0f);

        // FIXME
        if (gui.visible)
            effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
    }

    virtual void reload()
    {
        qDebug("DssiPlugin::reload()");
        short _id = m_id;

        // Safely disable plugin for reload
        carla_proc_lock();
        m_id = -1;
        carla_proc_unlock();

        // Unregister previous jack ports if needed
        if (_id >= 0)
            remove_from_jack();

        // Delete old data
        delete_buffers();

        uint32_t ains, aouts, mins, mouts, params, j;
        ains = aouts = mins = mouts = params = 0;

        ains   = effect->numInputs;
        aouts  = effect->numOutputs;
        params = effect->numParams;

        if (VstPluginCanDo(effect, "receiveVstEvents") || VstPluginCanDo(effect, "receiveVstMidiEvent") || effect->flags & effFlagsIsSynth)
            mins = 1;

        if (VstPluginCanDo(effect, "sendVstEvents") || VstPluginCanDo(effect, "sendVstMidiEvent"))
            mouts = 1;

        if (ains > 0)
            ain.ports = new jack_port_t*[ains];

        if (aouts > 0)
            aout.ports = new jack_port_t*[aouts];

        if (params > 0)
        {
            param.data    = new ParameterData[params];
            param.ranges  = new ParameterRanges[params];
        }

        const int port_name_size = jack_port_name_size();
        char port_name[port_name_size];
        bool needs_cin = (aouts > 0 || params > 0); // TODO - try to apply this <- to others

        for (j=0; j<ains; j++)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
                sprintf(port_name, "%s:input_%02i", m_name, j+1);
            else
#endif
                sprintf(port_name, "input_%02i", j+1);

            ain.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }

        for (j=0; j<aouts; j++)
        {
#ifndef BUILD_BRIDGE
            if (carla_options.global_jack_client)
                sprintf(port_name, "%s:output_%02i", m_name, j+1);
            else
#endif
                sprintf(port_name, "output_%02i", j+1);

            aout.ports[j] = jack_port_register(jack_client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }

        for (j=0; j<params; j++)
        {
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

                if (prop.flags & kVstParameterUsesIntStep)
                {
                    step = prop.stepInteger;
                    step_small = prop.stepInteger;
                    step_large = prop.largeStepInteger;
                }
                else if (prop.flags & kVstParameterUsesFloatStep)
                {
                    step = prop.stepFloat;
                    step_small = prop.smallStepFloat;
                    step_large = prop.largeStepFloat;
                }
                else if (prop.flags & kVstParameterIsSwitch)
                {
                    step = max - min;
                    step_small = step;
                    step_large = step;
                }
                else
                {
                    double range = max - min;
                    step = range/100.0;
                    step_small = range/1000.0;
                    step_large = range/10.0;
                }
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

            // no such thing as VST default parameters
            def = effect->getParameter(effect, j);

            if (def < min)
                def = min;
            else if (def > max)
                def = max;

            if (max - min == 0.0)
            {
                qWarning("Broken plugin parameter -> max - min == 0");
                max = min + 0.1;
            }

            param.data[j].type = PARAMETER_INPUT;
            param.ranges[j].min = min;
            param.ranges[j].max = max;
            param.ranges[j].def = def;
            param.ranges[j].step = step;
            param.ranges[j].step_small = step_small;
            param.ranges[j].step_large = step_large;

            // hints
            param.data[j].hints |= PARAMETER_IS_ENABLED;

            if (effect->dispatcher(effect, effCanBeAutomated, j, 0, nullptr, 0.0f) == 1)
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
                strcat(port_name, ":midi-in");
            }
            else
#endif
                strcpy(port_name, "midi-in");

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

        if (carla_options.global_jack_client == false)
            jack_activate(jack_client);
    }

    virtual void reload_programs(bool init)
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

        // Update OSC Names
        // etc etc

        callback_action(CALLBACK_RELOAD_PROGRAMS, m_id, 0, 0, 0.0);

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
                set_program(prog.current, true, true, true, true);
            else
            {
                // Program was changed during update, re-set it
                if (prog.current >= 0)
                    effect->dispatcher(effect, effSetProgram, 0, prog.current, nullptr, 0.0f);
            }
        }
    }

    virtual void process(jack_nframes_t nframes)
    {
        uint32_t i, k;
        unsigned short plugin_id = m_id;
        uint32_t midi_event_count = 0;

        double ains_peak_tmp[2]  = { 0.0 };
        double aouts_peak_tmp[2] = { 0.0 };

        jack_default_audio_sample_t* ains_buffer[ain.count];
        jack_default_audio_sample_t* aouts_buffer[aout.count];
        void* min_buffer  = nullptr;
        void* mout_buffer = nullptr;

        for (i=0; i < ain.count; i++)
            ains_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(ain.ports[i], nframes);

        for (i=0; i < aout.count; i++)
            aouts_buffer[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(aout.ports[i], nframes);

        if (midi.port_min > 0)
            min_buffer = jack_port_get_buffer(midi.port_min, nframes);

        if (midi.port_mout > 0)
            mout_buffer = jack_port_get_buffer(midi.port_mout, nframes);

        // --------------------------------------------------------------------------------------------------------
        // Input VU

        if (ain.count > 0)
        {
            short j2 = (ain.count == 1) ? 0 : 1;

            for (k=0; k<nframes; k++)
            {
                if (abs_d(ains_buffer[0][k]) > ains_peak_tmp[0])
                    ains_peak_tmp[0] = abs_d(ains_buffer[0][k]);
                if (abs_d(ains_buffer[j2][k]) > ains_peak_tmp[1])
                    ains_peak_tmp[1] = abs_d(ains_buffer[j2][k]);
            }
        }

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // Parameters Input [Automation]

        if (param.port_cin)
        {
            jack_default_audio_sample_t* pin_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(param.port_cin, nframes);

            jack_midi_event_t pin_event;
            uint32_t n_pin_events = jack_midi_get_event_count(pin_buffer);

            for (i=0; i<n_pin_events; i++)
            {
                if (jack_midi_event_get(&pin_event, pin_buffer, i) != 0)
                    break;

                unsigned char channel = pin_event.buffer[0] & 0x0F;
                unsigned char mode    = pin_event.buffer[0] & 0xF0;

                // Status change
                if (mode == 0xB0)
                {
                    unsigned char status  = pin_event.buffer[1] & 0x7F;
                    unsigned char velo    = pin_event.buffer[2] & 0x7F;
                    double value, velo_per = double(velo)/127;

                    // Control GUI stuff (channel 0 only)
                    if (channel == 0)
                    {
                        if (status == 0x78)
                        {
                            // All Sound Off
                            set_active(false, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_ACTIVE, 0.0);
                            break;
                        }
                        else if (status == 0x09 && (m_hints & PLUGIN_CAN_DRYWET) > 0)
                        {
                            // Dry/Wet (using '0x09', undefined)
                            set_drywet(velo_per, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_DRYWET, velo_per);
                        }
                        else if (status == 0x07 && (m_hints & PLUGIN_CAN_VOLUME) > 0)
                        {
                            // Volume
                            value = double(velo)/100;
                            set_volume(value, false, false);
                            postpone_event(PostEventParameterChange, PARAMETER_VOLUME, value);
                        }
                        else if (status == 0x08 && (m_hints & PLUGIN_CAN_BALANCE) > 0)
                        {
                            // Balance
                            double left, right;
                            value = (double(velo)-63.5)/63.5;

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
                        }
                    }

                    // Control plugin parameters
                    for (k=0; k < param.count; k++)
                    {
                        if (param.data[k].type == PARAMETER_INPUT && (param.data[k].hints & PARAMETER_IS_AUTOMABLE) > 0 &&
                                param.data[k].midi_channel == channel && param.data[k].midi_cc == status)
                        {
                            value = (velo_per * (param.ranges[k].max - param.ranges[k].min)) + param.ranges[k].min;
                            set_parameter_value(k, value, false, false, false);
                            postpone_event(PostEventParameterChange, k, value);
                        }
                    }
                }
                // Program change
                else if (mode == 0xC0)
                {
                    uint32_t prog_id = pin_event.buffer[1] & 0x7F;

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
                    ExternalMidiNote* enote = &ext_midi_notes[i];
                    enote->valid = false;

                    VstMidiEvent* midi_event = &midi_events[midi_event_count];
                    memset(midi_event, 0, sizeof(VstMidiEvent));

                    midi_event->type = kVstMidiType;
                    midi_event->byteSize = sizeof(VstMidiEvent);
                    midi_event->midiData[0] = enote->onoff ? 0x90 : 0x80;
                    midi_event->midiData[1] = enote->note;
                    midi_event->midiData[2] = enote->velo;

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

                if (min_event.size != 3)
                    continue;

                unsigned char channel = min_event.buffer[0] & 0x0F;
                unsigned char mode = min_event.buffer[0] & 0xF0;
                unsigned char note = min_event.buffer[1] & 0x7F;
                unsigned char velo = min_event.buffer[2] & 0x7F;

                // fix bad note off
                if (mode == 0x90 && velo == 0)
                {
                    mode = 0x80;
                    velo = 64;
                }

                VstMidiEvent* midi_event = &midi_events[midi_event_count];
                memset(midi_event, 0, sizeof(VstMidiEvent));

                if (mode == 0x80 || mode == 0x90)
                {
                    midi_event->type = kVstMidiType;
                    midi_event->byteSize = sizeof(VstMidiEvent);
                    midi_event->deltaFrames = min_event.time;
                    midi_event->midiData[0] = mode+channel;
                    midi_event->midiData[1] = note;
                    midi_event->midiData[2] = velo;

                    if (mode == 0x90)
                        postpone_event(PostEventNoteOn, note, velo);
                    else
                        postpone_event(PostEventNoteOff, note, 0.0);
                }
                // TODO - more types, but not status

                midi_event_count += 1;
            }
        } // End of MIDI Input (JACK)

        // VST Events
        if (midi_event_count > 0)
        {
            events.numEvents = midi_event_count;
            events.reserved = 0;
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

            if (effect->flags & effFlagsCanReplacing)
            {
                effect->processReplacing(effect, ains_buffer, aouts_buffer, nframes);
            }
            else
            {
                for (i=0; i < aout.count; i++)
                    memset(aouts_buffer[i], 0, sizeof(jack_default_audio_sample_t)*nframes);

                // FIXME - missing macro check
                effect->process(effect, ains_buffer, aouts_buffer, nframes);
            }
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
            double bal_rangeL, bal_rangeR;
            jack_default_audio_sample_t old_bal_left[nframes];

            for (i=0; i < aout.count; i++)
            {
                // Dry/Wet and Volume
                for (k=0; k<nframes; k++)
                {
                    if ((m_hints & PLUGIN_CAN_DRYWET) > 0 && x_drywet != 1.0)
                    {
                        if (aout.count == 1)
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[0][k]*(1.0-x_drywet));
                        else
                            aouts_buffer[i][k] = (aouts_buffer[i][k]*x_drywet)+(ains_buffer[i][k]*(1.0-x_drywet));
                    }

                    if (m_hints & PLUGIN_CAN_VOLUME)
                        aouts_buffer[i][k] *= x_vol;
                }

                // Balance
                if (m_hints & PLUGIN_CAN_BALANCE)
                {
                    if (i%2 == 0)
                        memcpy(&old_bal_left, aouts_buffer[i], sizeof(jack_default_audio_sample_t)*nframes);

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
                if (i < 2)
                {
                    for (k=0; k<nframes; k++)
                    {
                        if (abs_d(aouts_buffer[i][k]) > aouts_peak_tmp[i])
                            aouts_peak_tmp[i] = abs_d(aouts_buffer[i][k]);
                    }
                }
            }
        }
        else
        {
            // disable any output sound if not active
            for (i=0; i < aout.count; i++)
                memset(aouts_buffer[i], 0.0f, sizeof(jack_default_audio_sample_t)*nframes);

            aouts_peak_tmp[0] = 0.0;
            aouts_peak_tmp[1] = 0.0;

        } // End of Post-processing

        CARLA_PROCESS_CONTINUE_CHECK;

        // --------------------------------------------------------------------------------------------------------
        // MIDI Output
        if (midi.port_mout)
        {
            jack_midi_clear_buffer(mout_buffer);

            // TODO - read jack ringuffer events
            //jack_midi_event_write(mout_buffer, midi_event->deltaFrames, (unsigned char*)midi_event->midiData, midi_event->byteSize);

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

    virtual void buffer_size_changed(jack_nframes_t new_buffer_size)
    {
        if (m_active)
        {
            effect->dispatcher(effect, effStopProcess, 0, 0, nullptr, 0.0f);
            effect->dispatcher(effect, effMainsChanged, 0, 0, nullptr, 0.0f);
        }

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
#if !VST_FORCE_DEPRECATED
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
        // TODO - completely verify all calls

        qDebug("VstHostCallback() - code: %02i, index: %02i, value: " P_INTPTR ", opt: %03f", opcode, index, value, opt);

        VstPlugin* plugin = (effect && effect->user) ? (VstPlugin*)effect->user : nullptr;

        switch (opcode)
        {
        case audioMasterAutomate:
            if (plugin)
                plugin->set_parameter_value(index, opt, false, true, true);
            else if (effect)
                effect->setParameter(effect, index, opt);
            break;

        case audioMasterVersion:
            return kVstVersion;

            //case audioMasterCurrentId:
            //    return VstCurrentUniqueId;

        case audioMasterIdle:
            if (effect)
                effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0.0f);
            return 1; // FIXME?

        case audioMasterWantMidi:
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

#ifndef BRIDGE_WINVST
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
            return 1;

        case audioMasterGetTime:
            static VstTimeInfo timeInfo;
            memset(&timeInfo, 0, sizeof(VstTimeInfo));

            timeInfo.sampleRate = get_sample_rate();
            timeInfo.tempo = 120.0;

            if (plugin && plugin->jack_client)
            {
                static jack_position_t jack_pos;
                static jack_transport_state_t jack_state;

                jack_state = jack_transport_query(plugin->jack_client, &jack_pos);

                if (jack_state == JackTransportRolling)
                    timeInfo.flags |= kVstTransportChanged|kVstTransportPlaying;
                else
                    timeInfo.flags |= kVstTransportChanged;

                if (jack_pos.unique_1 == jack_pos.unique_2)
                {
                    timeInfo.samplePos  = jack_pos.frame;
                    timeInfo.sampleRate = jack_pos.frame_rate;

                    if (jack_pos.valid & JackPositionBBT)
                    {
                        // Tempo
                        timeInfo.tempo = jack_pos.beats_per_minute;
                        timeInfo.timeSigNumerator = jack_pos.beats_per_bar;
                        timeInfo.timeSigDenominator = jack_pos.beat_type;
                        timeInfo.flags |= kVstTempoValid|kVstTimeSigValid;

                        // Position
                        double dPos = timeInfo.samplePos / timeInfo.sampleRate;
                        timeInfo.nanoSeconds = dPos * 1000.0;
                        timeInfo.flags |= kVstNanosValid;

                        // Position
                        timeInfo.barStartPos = 0;
                        timeInfo.ppqPos = dPos * timeInfo.tempo / 60.0;
                        timeInfo.flags |= kVstBarsValid|kVstPpqPosValid;
                    }
                }
            }

            return (intptr_t)&timeInfo;

        case audioMasterProcessEvents:
#if 0
            if (plugin && plugin->mout.count > 0 && ptr)
            {
                VstEvents* events = (VstEvents*)ptr;

                jack_default_audio_sample_t* mout_buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(plugin->mout.ports[0], get_buffer_size());
                jack_midi_clear_buffer(mout_buffer);

                for (int32_t i=0; i < events->numEvents; i++)
                {
                    VstMidiEvent* midi_event = (VstMidiEvent*)events->events[i];

                    if (midi_event && midi_event->type == kVstMidiType)
                    {
                        // TODO - send event over jack rinbuffer
                    }
                }
                return 1;
            }
            else
                qDebug("Some MIDI Out events were ignored");
#endif
            return 0;

        case audioMasterTempoAt:
            // Deprecated in VST SDK 2.4
            if (plugin && plugin->jack_client)
            {
                static jack_position_t jack_pos;
                jack_transport_query(plugin->jack_client, &jack_pos);

                if (jack_pos.valid & JackPositionBBT)
                    return jack_pos.beats_per_minute;
            }
            return 120.0;

        case audioMasterGetNumAutomatableParameters:
            // Deprecated in VST SDK 2.4
            return MAX_PARAMETERS; // FIXME - what exacly is this for?

        case audioMasterIOChanged:
//            if (plugin && plugin->jack_client)
//            {
//                i = plugin->id;
//                bool unlock = carla_proc_trylock();
//                plugin->id = -1;
//                if (unlock) carla_proc_unlock();

//                if (plugin->active)
//                {
//                    plugin->effect->dispatcher(plugin->effect, 72 /* effStopProcess */, 0, 0, nullptr, 0.0f);
//                    plugin->effect->dispatcher(plugin->effect, effMainsChanged, 0, 0, nullptr, 0.0f);
//                }

//                plugin->reload();

//                if (plugin->active)
//                {
//                    plugin->effect->dispatcher(plugin->effect, effMainsChanged, 0, 1, nullptr, 0.0f);
//                    plugin->effect->dispatcher(plugin->effect, 71 /* effStartProcess */, 0, 0, nullptr, 0.0f);
//                }

//                plugin->id = i;
//#ifndef BRIDGE_WINVST
//                callback_action(CALLBACK_RELOAD_ALL, plugin->id, 0, 0, 0.0);
//#endif
//            }
            return 1;

        case audioMasterNeedIdle:
            // Deprecated in VST SDK 2.4
            effect->dispatcher(effect, 53 /* effIdle */, 0, 0, nullptr, 0.0f);
            return 1;

        case audioMasterSizeWindow:
            if (plugin)
            {
                plugin->gui.width  = index;
                plugin->gui.height = value;
                callback_action(CALLBACK_RESIZE_GUI, plugin->id(), index, value, 0.0);
            }
            return 1;

        case audioMasterGetSampleRate:
            return get_sample_rate();

        case audioMasterGetBlockSize:
            return get_buffer_size();

        case audioMasterGetVendorString:
            if (ptr)
                strcpy((char*)ptr, "falkTX");
            break;

        case audioMasterGetProductString:
            if (ptr)
                strcpy((char*)ptr, "Carla-Discovery");
            break;

        case audioMasterGetVendorVersion:
            return 0x05; // 0.5

        case audioMasterCanDo:
#if DEBUG
            qDebug("VstHostCallback:audioMasterCanDo - %s", (char*)ptr);
#endif

            if (strcmp((char*)ptr, "sendVstEvents") == 0)
                return 1;
            else if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
                return 1;
            else if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
                return 1;
            else if (strcmp((char*)ptr, "receiveVstEvents") == 0)
                return 1;
            else if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
                return 1;
#if !VST_FORCE_DEPRECATED
            else if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
                return -1;
#endif
            else if (strcmp((char*)ptr, "reportConnectionChanges") == 0)
                return 1;
            else if (strcmp((char*)ptr, "acceptIOChanges") == 0)
                return 1;
            else if (strcmp((char*)ptr, "sizeWindow") == 0)
                return 1;
            else if (strcmp((char*)ptr, "offline") == 0)
                return -1;
            else if (strcmp((char*)ptr, "openFileSelector") == 0)
                return -1;
            else if (strcmp((char*)ptr, "closeFileSelector") == 0)
                return -1;
            else if (strcmp((char*)ptr, "startStopProcess") == 0)
                return 1;
            else if (strcmp((char*)ptr, "shellCategory") == 0)
                return -1;
            else if (strcmp((char*)ptr, "sendVstMidiEventFlagIsRealtime") == 0)
                return -1;
            else
            {
                qWarning("VstHostCallback:audioMasterCanDo - Got uninplemented feature request '%s'", (char*)ptr);
                return 0;
            }

        case audioMasterGetLanguage:
            return kVstLangEnglish;

        case audioMasterUpdateDisplay:
            if (plugin)
            {
                // Update current program name
                if (plugin->prog.current >= 0 && plugin->prog.count > 0)
                {
                    char buf_str[STR_MAX] = { 0 };

                    if (plugin->effect->dispatcher(plugin->effect, effGetProgramNameIndexed, plugin->prog.current, 0, buf_str, 0.0f) != 1)
                        plugin->effect->dispatcher(plugin->effect, effGetProgramName, 0, 0, buf_str, 0.0f);

                    if (plugin->prog.names[plugin->prog.current])
                        free((void*)plugin->prog.names[plugin->prog.current]);

                    plugin->prog.names[plugin->prog.current] = strdup(buf_str);
                }
                callback_action(CALLBACK_UPDATE, plugin->id(), 0, 0, 0.0);
            }
            return 1;

        default:
            break;
        }

        return 0;
    }

private:
    AEffect* effect;
    struct {
        int32_t numEvents;
        intptr_t reserved;
        VstEvent* data[MAX_MIDI_EVENTS];
    } events;
    VstMidiEvent midi_events[MAX_MIDI_EVENTS];

    struct {
        bool visible;
        int width;
        int height;
    } gui;
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
            //osc_new_plugin(plugin);
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

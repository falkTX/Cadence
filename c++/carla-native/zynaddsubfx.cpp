/*
 * Carla Native Plugins
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

#include "carla_midi.h"
#include "carla_native.hpp"

#define NTK_GUI 1
#define VSTAUDIOOUT 1

#define PIXMAP_PATH "/usr/share/zynaddsubfx/pixmaps/"
#define SOURCE_DIR  "/usr/share/zynaddsubfx/pixmaps/nothing-here"

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Util.h"
#include "zynaddsubfx/Nio/Nio.h"

#include <climits>

#ifdef WANT_ZYNADDSUBFX_GUI
//# ifdef Q_WS_X11
//#  include <QtGui/QX11Info>
//# endif
# include <FL/Fl.H>
# include <FL/Fl_Shared_Image.H>
# include <FL/Fl_Tiled_Image.H>
# include <FL/Fl_Dial.H>
# include "zynaddsubfx/UI/MasterUI.h"

// this is used to know wherever gui stuff is initialized
static Fl_Tiled_Image* s_moduleBackdrop = nullptr;

void set_module_parameters(Fl_Widget* o)
{
    o->box(FL_DOWN_FRAME);
    o->align(o->align() | FL_ALIGN_IMAGE_BACKDROP);
    o->color(FL_BLACK );
    o->image(s_moduleBackdrop);
    o->labeltype(FL_SHADOW_LABEL);
}
#endif

SYNTH_T* synth = nullptr;

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    enum Parameters {
        PARAMETER_MASTER,
        PARAMETER_MAX
    };

    ZynAddSubFxPlugin(const HostDescriptor* host)
        : PluginDescriptorClass(host)
    {
        qDebug("ZynAddSubFxPlugin::ZynAddSubFxPlugin(), s_instanceCount=%i", s_instanceCount);

        m_master   = new Master;
#ifdef WANT_ZYNADDSUBFX_GUI
        m_masterUI = nullptr;
        m_uiClosed = 0;
#endif

        // refresh banks
        m_master->bank.rescanforbanks();

        for (size_t i=0, size = m_master->bank.banks.size(); i < size; i++)
        {
            if (m_master->bank.banks[i].dir.empty())
                continue;

            m_master->bank.loadbank(m_master->bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; instrument++)
            {
                const std::string insName = m_master->bank.getname(instrument);

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                ProgramInfo pInfo;
                pInfo.bank = i;
                pInfo.prog = instrument;
                pInfo.name = insName;
                m_programs.push_back(pInfo);
            }
        }
    }

    ~ZynAddSubFxPlugin()
    {
        qDebug("ZynAddSubFxPlugin::~ZynAddSubFxPlugin(), s_instanceCount=%i", s_instanceCount);

        //ensure that everything has stopped with the mutex wait
        pthread_mutex_lock(&m_master->mutex);
        pthread_mutex_unlock(&m_master->mutex);

        m_programs.clear();

#ifdef WANT_ZYNADDSUBFX_GUI
        if (m_masterUI)
            delete m_masterUI;
#endif

        delete m_master;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount()
    {
        return PARAMETER_MAX;
    }

    const Parameter* getParameterInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getParameterCount());

        if (index >= PARAMETER_MAX)
            return nullptr;

        static Parameter param;

        param.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case PARAMETER_MASTER:
            param.hints = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
            param.name  = "Master Volume";
            param.unit  = nullptr;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.ranges.def = 100.0f;
            break;
        }

        return &param;
    }

    float getParameterValue(uint32_t index)
    {
        switch (index)
        {
        case PARAMETER_MASTER:
            return m_master->Pvolume;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount()
    {
        return m_programs.size();
    }

    const MidiProgram* getMidiProgramInfo(uint32_t index)
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= m_programs.size())
            return nullptr;

        const ProgramInfo pInfo(m_programs[index]);

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo.bank;
        midiProgram.program = pInfo.prog;
        midiProgram.name    = pInfo.name.c_str();

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(uint32_t index, float value)
    {
        switch (index)
        {
        case PARAMETER_MASTER:
            m_master->setPvolume((char)rint(value));
            break;
        }
    }

    void setMidiProgram(uint32_t bank, uint32_t program)
    {
        if (bank >= m_master->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        const std::string bankdir = m_master->bank.banks[bank].dir;

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&m_master->mutex);

            m_master->bank.loadbank(bankdir);
            m_master->bank.loadfromslot(program, m_master->part[0]);

            pthread_mutex_unlock(&m_master->mutex);
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    static void cb_simplemasterwindow(Fl_Double_Window*, void* ptr)
    {
        qWarning("CLOSED");
        ((ZynAddSubFxPlugin*)ptr)->uiClosed();
    }

#ifdef WANT_ZYNADDSUBFX_GUI
    void uiShow(bool show)
    {
        if (! m_masterUI)
        {
            if (! s_moduleBackdrop)
            {
                //Fl::visual();
                //fltk::xdisplay = _disp_ptr;
                //fltk::xscreen = _screenID;
                //fltk::xvisual = _vis_ptr;
                //fltk::xcolormap = _colorMap;

                //fl_display  = QX11Info::display();
                //fl_screen   = QX11Info::appScreen();
                //fl_visual   = (XVisualInfo*)QX11Info::appVisual(fl_screen);
                //fl_colormap = QX11Info::appColormap(fl_screen);
//                //fl_gc       =
                //fl_window   = QX11Info::appRootWindow(fl_screen);

                //Fl::own_colormap();

                fl_register_images();

                Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

                if (Fl_Shared_Image* img = Fl_Shared_Image::get(PIXMAP_PATH "/knob.png"))
                    Fl_Dial::default_image(img);
                else
                    Fl_Dial::default_image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/knob.png"));

                if (Fl_Shared_Image* img = Fl_Shared_Image::get(PIXMAP_PATH "/window_backdrop.png"))
                    Fl::scheme_bg(new Fl_Tiled_Image(img));
                else
                    Fl::scheme_bg(new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/window_backdrop.png")));

                if (Fl_Shared_Image* img = Fl_Shared_Image::get(PIXMAP_PATH "/module_backdrop.png"))
                    s_moduleBackdrop = new Fl_Tiled_Image(img);
                else
                    s_moduleBackdrop = new Fl_Tiled_Image(Fl_Shared_Image::get(SOURCE_DIR "/../pixmaps/module_backdrop.png"));

                Fl::background(50, 50, 50);
                Fl::background2(70, 70, 70);
                Fl::foreground(255, 255, 255);
            }

            m_uiClosed = 0;
            qWarning("TEST - master: %p", m_master);
            m_masterUI = new MasterUI(m_master, &m_uiClosed);

            m_masterUI->simplemasterwindow->callback((Fl_Callback*)cb_simplemasterwindow, this);
        }

        if (show)
        {
            m_masterUI->showUI();
        }
        else
        {
            // same as showUI
            switch (config.cfg.UserInterfaceMode)
            {
            case 0:
                m_masterUI->selectuiwindow->hide();
                break;
            case 1:
                m_masterUI->masterwindow->hide();
                break;
            case 2:
                m_masterUI->simplemasterwindow->hide();
                break;
            };
        }
    }

    void uiIdle()
    {
        if (m_uiClosed)
        {
            qWarning("Closed!!");
        }
        else if (m_masterUI)
        {
            pthread_mutex_lock(&m_master->mutex);
            Fl::check();
            pthread_mutex_unlock(&m_master->mutex);
        }
    }
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate()
    {
        m_master->setController(0, MIDI_CONTROL_ALL_SOUND_OFF, 0);
    }

    void process(float**, float** outBuffer, uint32_t frames, uint32_t midiEventCount, MidiEvent* midiEvents)
    {
        unsigned long from_frame       = 0;
        unsigned long event_index      = 0;
        unsigned long next_event_frame = 0;
        unsigned long to_frame = 0;
        pthread_mutex_lock(&m_master->mutex);

        do {
            /* Find the time of the next event, if any */
            if (event_index >= midiEventCount)
                next_event_frame = ULONG_MAX;
            else
                next_event_frame = midiEvents[event_index].time;

            /* find the end of the sub-sample to be processed this time round... */
            /* if the next event falls within the desired sample interval... */
            if ((next_event_frame < frames) && (next_event_frame >= to_frame))
                /* set the end to be at that event */
                to_frame = next_event_frame;
            else
                /* ...else go for the whole remaining sample */
                to_frame = frames;

            if (from_frame < to_frame)
            {
                // call master to fill from `from_frame` to `to_frame`:
                m_master->GetAudioOutSamples(to_frame - from_frame, (int)getSampleRate(), &outBuffer[0][from_frame], &outBuffer[1][from_frame]);
                // next sub-sample please...
                from_frame = to_frame;
            }

            // Now process any event(s) at the current timing point
            while (event_index < midiEventCount && midiEvents[event_index].time == to_frame)
            {
                uint8_t status  = midiEvents[event_index].data[0];
                uint8_t channel = status & 0x0F;

                if (MIDI_IS_STATUS_NOTE_OFF(status))
                {
                    uint8_t note = midiEvents[event_index].data[1];

                    m_master->noteOff(channel, note);
                }
                else if (MIDI_IS_STATUS_NOTE_ON(status))
                {
                    uint8_t note = midiEvents[event_index].data[1];
                    uint8_t velo = midiEvents[event_index].data[2];

                    m_master->noteOn(channel, note, velo);
                }
                else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
                {
                    uint8_t note     = midiEvents[event_index].data[1];
                    uint8_t pressure = midiEvents[event_index].data[2];

                    m_master->polyphonicAftertouch(channel, note, pressure);
                }

                event_index++;
            }

        // Keep going until we have the desired total length of sample...
        } while (to_frame < frames);

        pthread_mutex_unlock(&m_master->mutex);
    }

    // -------------------------------------------------------------------

private:
    struct ProgramInfo {
        uint32_t bank;
        uint32_t prog;
        std::string name;
    };
    std::vector<ProgramInfo> m_programs;

    Master*   m_master;
#ifdef WANT_ZYNADDSUBFX_GUI
    MasterUI* m_masterUI;
    int m_uiClosed;
#endif

public:
    static int s_instanceCount;

    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        if (s_instanceCount++ == 0)
        {
            synth = new SYNTH_T;
            synth->buffersize = host->get_buffer_size(host->handle);
            synth->samplerate = host->get_sample_rate(host->handle);
            synth->alias();

            config.init();
            config.cfg.SoundBufferSize = synth->buffersize;
            config.cfg.SampleRate      = synth->samplerate;
            config.cfg.GzipCompression = 0;

            sprng(time(NULL));
            denormalkillbuf = new float [synth->buffersize];
            for (int i=0; i < synth->buffersize; i++)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16;

            Master::getInstance();

            Nio::start();
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;

        if (--s_instanceCount == 0)
        {
#ifdef WANT_ZYNADDSUBFX_GUI
            if (s_moduleBackdrop)
            {
                delete s_moduleBackdrop;
                s_moduleBackdrop = nullptr;
            }
#endif

            Nio::stop();

            Master::deleteInstance();

            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }
};

int ZynAddSubFxPlugin::s_instanceCount = 0;

// -----------------------------------------------------------------------

static PluginDescriptor zynAddSubFxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
#ifdef WANT_ZYNADDSUBFX_GUI
    /* hints     */ PLUGIN_IS_SYNTH | PLUGIN_HAS_GUI | PLUGIN_USES_SINGLE_THREAD,
#else
    /* hints     */ PLUGIN_IS_SYNTH | PLUGIN_USES_SINGLE_THREAD,
#endif
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::PARAMETER_MAX,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_zynaddsubfx()
{
    carla_register_native_plugin(&zynAddSubFxDesc);
}

// -----------------------------------------------------------------------

/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#ifdef QTCREATOR_TEST
# include "carla_utils.hpp"
# include "carla_native.h"
# include <vector>
//# include <QApplication>
//# include <QDialog>
#endif

#if defined(__WIN32__) || defined(__WIN64__)
# include "src/pugl/pugl_win.cpp"
#elif defined(__APPLE__)
# include "src/pugl/pugl_osx.m"
#elif defined(__linux__)
# include "src/pugl/pugl_x11.c"
#endif

#ifdef QTCREATOR_TEST

static std::vector<const PluginDescriptor*> descs;

static uint32_t get_buffer_size(HostHandle)
{
    return 512;
}

static double get_sample_rate(HostHandle)
{
    return 44100.0;
}

static const TimeInfo* get_time_info(HostHandle)
{
    static TimeInfo timeInfo;
    timeInfo.playing = false;
    timeInfo.frame = 0;
    timeInfo.time  = 0;
    timeInfo.valid = 0x0;
    return &timeInfo;
}

static bool write_midi_event(HostHandle, MidiEvent*)
{
    return false;
}

static void ui_parameter_changed(PluginHandle, uint32_t, float) {}
static void ui_custom_data_changed(PluginHandle, const char*, const char*) {}

static bool uiClosed = false;
static void ui_closed(PluginHandle)
{
    uiClosed = true;
}

void carla_register_native_plugin(const PluginDescriptor* desc)
{
    descs.push_back(desc);
}

#include <FL/Fl.H>

#if 0
static int qargc = 0;
static char* qargv[0] = {};

class TestApplication : public QApplication
{
public:
    TestApplication(int argc, char** argv)
        : QApplication(qargc, qargv, true)
    {
        msgTimer  = 0;

        zynDesc   = nullptr;
        zynHandle = nullptr;
    }

    void startNow()
    {
        msgTimer = startTimer(50);
    }

    void setHandle(const PluginDescriptor* desc, PluginHandle handle)
    {
        zynDesc   = desc;
        zynHandle = handle;
    }

protected:
    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == msgTimer)
        {
            if (zynDesc && zynHandle)
                zynDesc->ui_idle(zynHandle);
            //Fl::check();
        }

        QApplication::timerEvent(event);
    }

private:
    int msgTimer;

    const PluginDescriptor* zynDesc;
    PluginHandle zynHandle;
};
#endif

int main(int argc, char* argv[])
{
    //TestApplication app(argc, argv);
    //QApplication app(argc, argv);

    Fl::args(argc, argv);

    //QDialog guiTest;
    //guiTest.setFixedSize(150, 150);

    // Available plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_midiSplit();
    carla_register_native_plugin_zynaddsubfx();

    // DISTRHO based plugins
    carla_register_native_plugin_3BandEQ();
    carla_register_native_plugin_3BandSplitter();

    CARLA_ASSERT(descs.size() == 5);

    HostDescriptor host = {
        nullptr,
        get_buffer_size, get_sample_rate, get_time_info, write_midi_event,
        ui_parameter_changed, ui_custom_data_changed, ui_closed
    };

#if 0
    // test fast init & cleanup
    for (auto it = descs.begin(); it != descs.end(); it++)
    {
        const PluginDescriptor* desc(*it);

        PluginHandle h1, h2, h3, h4, h5;

        h1 = desc->instantiate(desc, &host);
        h2 = desc->instantiate(desc, &host);
        h3 = desc->instantiate(desc, &host);
        CARLA_ASSERT(h1 && h2 && h3);

        if (desc->cleanup)
            desc->cleanup(h1);

        h4 = desc->instantiate(desc, &host);
        h5 = desc->instantiate(desc, &host);
        CARLA_ASSERT(h4 && h5);

        if (desc->cleanup)
        {
            desc->cleanup(h4);
            desc->cleanup(h5);
            desc->cleanup(h2);
            desc->cleanup(h3);
        }
    }

    // test process of 0
    for (auto it = descs.begin(); it != descs.end(); it++)
    {
        const PluginDescriptor* desc(*it);
        PluginHandle handle = desc->instantiate(desc, &host);
        CARLA_ASSERT(handle);

        float* aIns[desc->audioIns];
        float* aOuts[desc->audioOuts];

        if (desc->activate)
            desc->activate(handle);

        desc->process(handle, aIns, aOuts, 0, 0, nullptr);

        if (desc->deactivate)
            desc->deactivate(handle);

        if (desc->cleanup)
            desc->cleanup(handle);
    }
#endif

    // close app when this dialog is closed
    //guiTest.show();

    const PluginDescriptor* zynDesc = descs[2];
    PluginHandle zynHandle = zynDesc->instantiate(zynDesc, &host);
    zynDesc->activate(zynHandle);
    zynDesc->ui_show(zynHandle, true);

    //app.setHandle(zynDesc, zynHandle);
    //app.startNow();

    //fl_display
    //int ret = app.exec();

    //app.setHandle(nullptr, nullptr);

    while (! uiClosed)
    {
        Fl::wait(0.02f);
        carla_msleep(100);
    }

    zynDesc->ui_show(zynHandle, false);
    zynDesc->activate(zynHandle);
    zynDesc->cleanup(zynHandle);

    // test 3BandEQ GUI
    // TODO
}
#endif

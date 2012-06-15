/*
 * Carla Plugin bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#include "carla_backend.h"
#include "carla_plugin.h"

#ifndef __WINE__
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#endif

#define CARLA_PLUGIN CarlaBackend::CarlaPlugins[0]

// -------------------------------------------------------------------------
// backend stuff

CARLA_BACKEND_START_NAMESPACE

short add_plugin_ladspa(const char* filename, const char* label, const void* extra_stuff);
short add_plugin_dssi(const char* filename, const char* label, const void* extra_stuff);
short add_plugin_lv2(const char* filename, const char* label);
short add_plugin_vst(const char* filename, const char* label);

CARLA_BACKEND_END_NAMESPACE

// -------------------------------------------------------------------------
// plugin bridge stuff

using namespace CarlaBackend;

#ifdef __WINE__
static bool close_now = false;
static HINSTANCE hInst = nullptr;
static HWND gui = nullptr;
#else
static QApplication* app = nullptr;
static QDialog* gui = nullptr;
#endif

static int nextWidth  = 0;
static int nextHeight = 0;
static int nextGuiMsg = 0;

void plugin_bridge_show_gui(bool yesno)
{
    nextGuiMsg = int(yesno)+1;
}

void plugin_bridge_quit()
{
#ifdef __WINE__
    close_now = true;
#else
    if (app)
        app->quit();
#endif
}

void plugin_bridge_idle()
{
    if (nextWidth != 0 && nextHeight != 0)
    {
        if (gui)
        {
#ifdef __WINE__
            SetWindowPos(gui, 0, 0, 0, nextWidth + 6, nextHeight + 25, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
            gui->setFixedSize(nextWidth, nextHeight);
#endif
        }
        nextWidth = nextHeight = 0;
    }

    if (nextGuiMsg)
    {
        bool yesno = nextGuiMsg -1;

        CARLA_PLUGIN->show_gui(yesno);

        if (gui)
        {
#ifdef __WINE__
            ShowWindow(gui, yesno ? SW_SHOWNORMAL : SW_HIDE);
            UpdateWindow(gui);
#else
            gui->setVisible(yesno);
#endif
        }

        nextGuiMsg = 0;
    }

    CARLA_PLUGIN->idle_gui();

    if (CARLA_PLUGIN->ain_count() > 0)
    {
        osc_send_bridge_ains_peak(1, ains_peak[0]);
        osc_send_bridge_ains_peak(2, ains_peak[1]);
    }

    if (CARLA_PLUGIN->aout_count() > 0)
    {
        osc_send_bridge_aouts_peak(1, aouts_peak[0]);
        osc_send_bridge_aouts_peak(2, aouts_peak[1]);
    }

    const ParameterData* param_data;

    for (uint32_t i=0; i < CARLA_PLUGIN->param_count(); i++)
    {
        param_data = CARLA_PLUGIN->param_data(i);

        if (param_data->type == PARAMETER_OUTPUT && (param_data->hints & PARAMETER_IS_AUTOMABLE) > 0)
            osc_send_control(param_data->rindex, CARLA_PLUGIN->get_parameter_value(i));
    }
}

// -------------------------------------------------------------------------

#ifdef __WINE__
LRESULT WINAPI MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        plugin_bridge_show_gui(false);
        osc_send_configure("CarlaBridgeHideGUI", "");
        return TRUE;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#else
class PluginIdleTimer : public QTimer
{
public:
    PluginIdleTimer() {}

    void timerEvent(QTimerEvent*)
    {
        plugin_bridge_idle();
    }
};
#endif

void plugin_bridge_callback(CallbackType action, unsigned short, int value1, int value2, double value3)
{
    switch (action)
    {
    case CALLBACK_PARAMETER_CHANGED:
        osc_send_control(value1, value3);
        break;
    case CALLBACK_PROGRAM_CHANGED:
        osc_send_program(value1);
        break;
    case CALLBACK_MIDI_PROGRAM_CHANGED:
        osc_send_midi_program(value1, value2, false);
        break;
    case CALLBACK_NOTE_ON:
    case CALLBACK_NOTE_OFF:
        //osc_send_midi(value1, value2);
        break;
    case CALLBACK_RESIZE_GUI:
        nextWidth  = value1;
        nextHeight = value2;
        break;
    case CALLBACK_QUIT:
        plugin_bridge_quit();
        break;
    default:
        break;
    }

    Q_UNUSED(value3);
}

// -------------------------------------------------------------------------

#ifdef __WINE__
int WINAPI WinMain(HINSTANCE hInst_, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{
#define MAXCMDTOKENS 128
    hInst = hInst_;
    int argc;
    LPSTR argv[MAXCMDTOKENS];
    LPSTR p = GetCommandLine();

    char command[256];
    char *args;
    char *d, *e;

    argc = 0;
    args = (char *)malloc(lstrlen(p)+1);
    if (args == (char *)NULL) {
        fprintf(stdout, "Insufficient memory in WinMain()\n");
        return 1;
    }

    // Parse command line handling quotes.
    d = args;
    while (*p)
    {
        // for each argument
        if (argc >= MAXCMDTOKENS - 1)
            break;

        e = d;
        while ((*p) && (*p != ' ')) {
            if (*p == '\042') {
                // Remove quotes, skipping over embedded spaces.
                // Doesn't handle embedded quotes.
                p++;
                while ((*p) && (*p != '\042'))
                    *d++ =*p++;
            }
            else
                *d++ = *p;
            if (*p)
                p++;
        }
        *d++ = '\0';
        argv[argc++] = e;

        while ((*p) && (*p == ' '))
            p++;        // Skip over trailing spaces
    }
    argv[argc] = 0;

    if (strlen(argv[0]) == 0)
    {
        GetModuleFileName(hInst, command, sizeof(command)-1);
        argv[0] = command;
    }
#else
int main(int argc, char* argv[])
{
#endif
    if (argc != 5)
    {
        qWarning("%s :: bad arguments", argv[0]);
        return 1;
    }

    const char* osc_url  = argv[1];
    const char* stype    = argv[2];
    const char* filename = argv[3];
    const char* label    = argv[4];

    short id;
    PluginType itype;

    if (strcmp(stype, "LADSPA") == 0)
        itype = PLUGIN_LADSPA;
    else if (strcmp(stype, "DSSI") == 0)
        itype = PLUGIN_DSSI;
    else if (strcmp(stype, "LV2") == 0)
        itype = PLUGIN_LV2;
    else if (strcmp(stype, "VST") == 0)
        itype = PLUGIN_VST;
    else
    {
        itype = PLUGIN_NONE;
        qWarning("Invalid plugin type '%s'", stype);
        return 1;
    }

    QString engName = QString("%1 (master)").arg(label);
    engName.truncate(CarlaEngine::maxClientNameSize());

    CarlaEngine engine;
    engine.init(engName.toUtf8().constData());

#ifdef __WINE__
#else
    app = new QApplication(argc, argv);
#endif

    set_callback_function(plugin_bridge_callback);
    set_last_error("no error");
    osc_init(osc_url);
    osc_send_update();

    switch (itype)
    {
    case PLUGIN_LADSPA:
        id = add_plugin_ladspa(filename, label, nullptr);
        break;
    case PLUGIN_DSSI:
        id = add_plugin_dssi(filename, label, nullptr);
        break;
    case PLUGIN_LV2:
        id = add_plugin_lv2(filename, label);
        break;
    case PLUGIN_VST:
        id = add_plugin_vst(filename, label);
        break;
    default:
        id = -1;
        break;
    }

    if (id == 0 && CARLA_PLUGIN)
    {
        GuiInfo guiInfo;
        CARLA_PLUGIN->get_gui_info(&guiInfo);

        QString guiTitle = QString("%1 (GUI)").arg(CARLA_PLUGIN->name());

#ifdef __WINE__
        if (guiInfo.type == GUI_INTERNAL_HWND)
        {
            WNDCLASSEX wclass;
            wclass.cbSize = sizeof(WNDCLASSEX);
            wclass.style  = 0;
            wclass.lpfnWndProc = MainProc;
            wclass.cbClsExtra = 0;
            wclass.cbWndExtra = 0;
            wclass.hInstance = hInst;
            wclass.hIcon   = LoadIcon(hInst, "carla");
            wclass.hCursor = LoadCursor(0, IDI_APPLICATION);
            wclass.lpszMenuName  = "MENU_CARLA_BRIDGE";
            wclass.lpszClassName = "CLASS_CARLA_BRIDGE";
            wclass.hIconSm = 0;

            if (! RegisterClassEx(&wclass))
            {
                qCritical("Failed to register Wine application");
                return 1;
            }

            gui = CreateWindow("CLASS_CARLA_BRIDGE", guiTitle.toUtf8().constData(),
                               WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               0, 0, hInst, 0);
            SetWindowPos(gui, 0, 0, 0, 6, 25, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
        if (guiInfo.type == GUI_INTERNAL_QT4 || guiInfo.type == GUI_INTERNAL_X11)
        {
            gui = new QDialog(nullptr);
            gui->setWindowTitle(guiTitle);
#endif
            CARLA_PLUGIN->set_gui_data(0, gui);
        }

        osc_send_bridge_update();

        {
            // FIXME
            //CARLA_PLUGIN->set_active(true, false, false);
            //plugin_bridge_show_gui(true);

#ifdef __WINE__
            MSG msg;

            while (! close_now)
            {
                while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                    DispatchMessage(&msg);

                if (close_now)
                    break;

                plugin_bridge_idle();

                if (close_now)
                    break;

                carla_msleep(50);
            }
#else
            PluginIdleTimer timer;
            timer.start(50);

            app->setQuitOnLastWindowClosed(false);
            app->exec();
#endif
        }

        carla_proc_lock();
        CARLA_PLUGIN->set_enabled(false);
        carla_proc_unlock();

        delete CARLA_PLUGIN;

#ifndef __WINE__
        if (gui)
        {
            gui->close();
            delete gui;
        }
        delete app;
#endif
    }
    else
    {
        qWarning("Plugin failed to load, error was:\n%s", get_last_error());
        return 1;
    }

    engine.close();

    osc_send_exiting();
    osc_close();

    return 0;
}

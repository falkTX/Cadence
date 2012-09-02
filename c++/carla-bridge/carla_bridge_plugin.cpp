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

#include "carla_bridge_client.h"
#include "carla_plugin.h"

//#include "carla_plugin.h"

#include <QtCore/QFile>

//#ifndef __WINE__
//#include <QtCore/QTimer>
//#include <QtGui/QApplication>
//#include <QtGui/QDialog>
//#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------
// client

class CarlaBridgePluginClient : public CarlaClient
{
public:
    CarlaBridgePluginClient(CarlaToolkit* const toolkit)
        : CarlaClient(toolkit)
    {
        engine = nullptr;
        plugin = nullptr;
    }

    ~CarlaBridgePluginClient()
    {
    }

    void setStuff(CarlaBackend::CarlaEngine* const engine_, CarlaBackend::CarlaPlugin* const plugin_)
    {
        engine = engine_;
        plugin = plugin_;
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const double value)
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setParameterValueByRIndex(rindex, value, true, true, false);
    }

    void setProgram(const uint32_t index)
    {
        Q_ASSERT(plugin && index < plugin->programCount());

        if (! plugin)
            return;
        if (index >= plugin->programCount())
            return;

        plugin->setProgram(index, true, true, false, true);
    }

    void setMidiProgram(const uint32_t bank, const uint32_t program)
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setMidiProgramById(bank, program, true, true, false, true);
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        Q_ASSERT(plugin);
        Q_ASSERT(velo != 0);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, velo, true, true, false);
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, 0, true, true, false);
    }

    // ---------------------------------------------------------------------
    // plugin

    void saveNow()
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->prepareForSave();

#if 0
        for (uint32_t i=0; i < CARLA_PLUGIN->customDataCount(); i++)
        {
            const CustomData* const cdata = CARLA_PLUGIN->customData(i);
            osc_send_bridge_custom_data(customdatatype2str(cdata->type), cdata->key, cdata->value);
        }

        if (CARLA_PLUGIN->hints() & PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            int32_t dataSize = CARLA_PLUGIN->chunkData(&data);

            if (data && dataSize >= 4)
            {
                QString filePath;
                filePath += "/tmp/.CarlaChunk_";
                filePath += CARLA_PLUGIN->name();

                QFile file(filePath);

                if (file.open(QIODevice::WriteOnly))
                {
                    QByteArray chunk((const char*)data, dataSize);
                    file.write(chunk);
                    file.close();
                    osc_send_bridge_chunk_data(filePath.toUtf8().constData());
                }
            }
        }

        osc_send_configure(CARLA_BRIDGE_MSG_SAVED, "");
#endif
    }

    void setCustomData(const char* const type, const char* const key, const char* const value)
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setCustomData(CarlaBackend::getCustomDataStringType(type), key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

#if 0
        nextChunkFilePath = strdup(filePath);

        while (nextChunkFilePath)
            carla_msleep(25);
#endif
    }

    // ---------------------------------------------------------------------
    // ...

    void handleCallback(const CarlaBackend::CallbackType action, const int value1, const int value2, const double value3)
    {
        switch (action)
        {
        case CarlaBackend::CALLBACK_PARAMETER_VALUE_CHANGED:
            //osc_send_control(value1, value3);
            break;
        case CarlaBackend::CALLBACK_PROGRAM_CHANGED:
            //osc_send_program(value1);
            break;
        case CarlaBackend::CALLBACK_MIDI_PROGRAM_CHANGED:
            //osc_send_midi_program(value1, value2, false);
            break;
        case CarlaBackend::CALLBACK_NOTE_ON:
        {
            //uint8_t mdata[4] = { 0, MIDI_STATUS_NOTE_ON, (uint8_t)value1, (uint8_t)value2 };
            //osc_send_midi(mdata);
            break;
        }
        case CarlaBackend::CALLBACK_NOTE_OFF:
        {
            //uint8_t mdata[4] = { 0, MIDI_STATUS_NOTE_OFF, (uint8_t)value1, (uint8_t)value2 };
            //osc_send_midi(mdata);
            break;
        }
        case CarlaBackend::CALLBACK_SHOW_GUI:
            //if (value1 == 0)
            //    osc_send_configure(CARLA_BRIDGE_MSG_HIDE_GUI, "");
            break;
        case CarlaBackend::CALLBACK_RESIZE_GUI:
            //if (client)
            //    client->queque_message(BRIDGE_MESSAGE_RESIZE_GUI, value1, value2, 0.0);
            break;
        case CarlaBackend::CALLBACK_RELOAD_PARAMETERS:
            //if (CARLA_PLUGIN)
            //{
            //    for (uint32_t i=0; i < CARLA_PLUGIN->parameterCount(); i++)
            //    {
            //        osc_send_control(i, CARLA_PLUGIN->getParameterValue(i));
            //    }
            //}
            break;
        case CarlaBackend::CALLBACK_QUIT:
            quequeMessage(MESSAGE_QUIT, 0, 0, 0.0);
            break;
        }
    }

    // ---------------------------------------------------------------------

    static void callback(void* const ptr, CarlaBackend::CallbackType const action, const unsigned short, const int value1, const int value2, const double value3)
    {
        Q_ASSERT(ptr);

        if (! ptr)
            return;

        CarlaBridgePluginClient* const client = (CarlaBridgePluginClient*)ptr;
        client->handleCallback(action, value1, value2, value3);
    }

private:
    CarlaBackend::CarlaEngine* engine;
    CarlaBackend::CarlaPlugin* plugin;
};

// -------------------------------------------------------------------------
// toolkit

class CarlaBridgeToolkitPlugin : public CarlaToolkit
{
public:
    CarlaBridgeToolkitPlugin(const char* const title)
        : CarlaToolkit(title)
    {
        qDebug("CarlaBridgeToolkitPlugin::CarlaBridgeToolkitPlugin(%s)", title);
    }

    ~CarlaBridgeToolkitPlugin()
    {
        qDebug("CarlaBridgeToolkitPlugin::~CarlaBridgeToolkitPlugin()");
    }

    void init()
    {
    }

    void exec(CarlaClient* const client)
    {
        m_client = client;
    }

    void quit()
    {
    }

    void show()
    {
    }

    void hide()
    {
    }

    void resize(int width, int height)
    {
    }
};

CarlaToolkit* CarlaToolkit::createNew(const char* const title)
{
    return new CarlaBridgeToolkitPlugin(title);
}

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        qWarning("%s :: bad arguments", argv[0]);
        return 1;
    }

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char* const label    = argv[5];

    if (strcmp(name, "(none)") == 0)
        name = nullptr;

    CarlaBackend::PluginType itype;

    if (strcmp(stype, "LADSPA") == 0)
        itype = CarlaBackend::PLUGIN_LADSPA;
    else if (strcmp(stype, "DSSI") == 0)
        itype = CarlaBackend::PLUGIN_DSSI;
    else if (strcmp(stype, "LV2") == 0)
        itype = CarlaBackend::PLUGIN_LV2;
    else if (strcmp(stype, "VST") == 0)
        itype = CarlaBackend::PLUGIN_VST;
    else
    {
        itype = CarlaBackend::PLUGIN_NONE;
        qWarning("Invalid plugin type '%s'", stype);
        return 1;
    }

    // Init toolkit
    CarlaBridge::CarlaBridgeToolkitPlugin toolkit(name);
    toolkit.init();

    // Init client
    CarlaBridge::CarlaBridgePluginClient client(&toolkit);

    // Init OSC
    if (! client.oscInit(oscUrl))
    {
        toolkit.quit();
        return -1;
    }

    // Init backend engine
    CarlaBackend::CarlaEngineJack engine;
    engine.setCallback(client.callback, &client);

    // bridge client <-> engine
    client.registerOscEngine(&engine);

    /// Init plugin
    short id = engine.addPlugin(itype, filename, name, label);

    if (id >= 0 && id < CarlaBackend::MAX_PLUGINS)
    {
        CarlaBackend::CarlaPlugin* const plugin = engine.getPlugin(id);
        client.setStuff(&engine, plugin);
    }
    else
    {
        qWarning("Plugin failed to load, error was:\n%s", CarlaBackend::getLastError());
        return 1;
    }

    // Init engine
    //QString engName = QString("%1 (master)").arg(label);
    //engName.truncate(CarlaEngine::maxClientNameSize());

    //CarlaEngine engine;
    //engine.init(engName.toUtf8().constData());

    // Init toolkit
    //toolkit_init();

    // Init plugin client
    //client = new PluginData;

    // Init OSC
    //osc_init(osc_url);
    //osc_send_update();

    toolkit.exec(&client);

    // Close OSC
    client.sendOscExiting();
    client.oscClose();

    // Close client
    //client.close();

    // Close toolkit
    toolkit.quit();

    return 0;
}

#if 0
#define CARLA_PLUGIN CarlaBackend::CarlaPlugins[0]

void toolkit_plugin_idle();

ClientData* client = nullptr;

// -------------------------------------------------------------------------
// backend stuff

// -------------------------------------------------------------------------
// toolkit classes

#ifdef __WINE__
LRESULT WINAPI MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        if (client)
            client->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 0, 0, 0.0);
        osc_send_configure(CARLA_BRIDGE_MSG_HIDE_GUI, "");
        return TRUE;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
static bool close_now = false;
static HINSTANCE hInst = nullptr;
static HWND gui = nullptr;
#else
class PluginIdleTimer : public QTimer
{
public:
    PluginIdleTimer() {}

    void timerEvent(QTimerEvent*)
    {
        if (client)
            client->run_messages();

        toolkit_plugin_idle();
    }

    Q_SLOT void guiClosed()
    {
        //if (client)
        //    client->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 0, 0, 0.0);
        osc_send_configure(CARLA_BRIDGE_MSG_HIDE_GUI, "");
    }
};
static QApplication* app = nullptr;
static QDialog* gui = nullptr;
#endif

#define nextShowMsgNULL  0
#define nextShowMsgFALSE 1
#define nextShowMsgTRUE  2
static int nextShowMsg = nextShowMsgNULL;
static const char* nextChunkFilePath = nullptr;

// -------------------------------------------------------------------------
// toolkit calls

void toolkit_init()
{
#ifdef __WINE__
#else
    static int argc = 0;
    static char* argv[] = { nullptr };
    app = new QApplication(argc, argv, true);
#endif
}

void toolkit_plugin_idle()
{
    if (nextShowMsg)
    {
        bool yesno = nextShowMsg - 1;

        CARLA_PLUGIN->showGui(yesno);

        if (gui)
        {
#ifdef __WINE__
            ShowWindow(gui, yesno ? SW_SHOWNORMAL : SW_HIDE);
            UpdateWindow(gui);
#else
            gui->setVisible(yesno);
#endif
        }

        nextShowMsg = nextShowMsgNULL;
    }

    if (nextChunkFilePath)
    {
        QFile file(nextChunkFilePath);

        free((void*)nextChunkFilePath);
        nextChunkFilePath = nullptr;

        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString stringData = file.readAll();
            file.remove();

            CARLA_PLUGIN->setChunkData(stringData.toUtf8().constData());
        }
    }

    CARLA_PLUGIN->idleGui();

    static PluginPostEvent postEvents[MAX_POST_EVENTS];
    CARLA_PLUGIN->postEventsCopy(postEvents);

    for (uint32_t i=0; i < MAX_POST_EVENTS; i++)
    {
        if (postEvents[i].type == PluginPostEventNull)
            break;

        switch (postEvents[i].type)
        {
        case PluginPostEventParameterChange:
            callback_action(CALLBACK_PARAMETER_CHANGED, 0, postEvents[i].index, 0, postEvents[i].value);
            break;
        case PluginPostEventProgramChange:
            callback_action(CALLBACK_PROGRAM_CHANGED, 0, postEvents[i].index, 0, 0.0);
            break;
        case PluginPostEventMidiProgramChange:
            callback_action(CALLBACK_MIDI_PROGRAM_CHANGED, 0, postEvents[i].index, 0, 0.0);
            break;
        case PluginPostEventNoteOn:
            callback_action(CALLBACK_NOTE_ON, 0, postEvents[i].index, postEvents[i].value, 0.0);
            break;
        case PluginPostEventNoteOff:
            callback_action(CALLBACK_NOTE_OFF, 0, postEvents[i].index, 0, 0.0);
            break;
        default:
            break;
        }
    }

    const ParameterData* paramData;

    for (uint32_t i=0; i < CARLA_PLUGIN->parameterCount(); i++)
    {
        paramData = CARLA_PLUGIN->parameterData(i);

        if (paramData->type == PARAMETER_OUTPUT && (paramData->hints & PARAMETER_IS_AUTOMABLE) > 0)
            osc_send_control(paramData->rindex, CARLA_PLUGIN->getParameterValue(i));
    }

    if (CARLA_PLUGIN->audioInCount() > 0)
    {
        osc_send_bridge_ains_peak(1, ains_peak[0]);
        osc_send_bridge_ains_peak(2, ains_peak[1]);
    }

    if (CARLA_PLUGIN->audioOutCount() > 0)
    {
        osc_send_bridge_aouts_peak(1, aouts_peak[0]);
        osc_send_bridge_aouts_peak(2, aouts_peak[1]);
    }
}

void toolkit_loop()
{
#ifdef __WINE__
    MSG msg;

    while (! close_now)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);

        client->run_messages();

        toolkit_plugin_idle();

        carla_msleep(50);
    }
#else
    PluginIdleTimer timer;
    timer.start(50);

    if (gui)
        timer.connect(gui, SIGNAL(finished(int)), &timer, SLOT(guiClosed()));

    app->setQuitOnLastWindowClosed(false);
    app->exec();
#endif
}

void toolkit_quit()
{
#ifdef __WINE__
    close_now = true;
#else
    if (app)
        app->quit();
#endif
}

void toolkit_window_show()
{
    nextShowMsg = nextShowMsgTRUE;
}

void toolkit_window_hide()
{
    nextShowMsg = nextShowMsgFALSE;
}

void toolkit_window_resize(int width, int height)
{
    if (gui)
    {
#ifdef __WINE__
        SetWindowPos(gui, 0, 0, 0, width + 6, height + 25, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
        gui->setFixedSize(width, height);
#endif
    }
}

// -------------------------------------------------------------------------

// -------------------------------------------------------------------------

#ifdef __WINE__
int WINAPI WinMain(HINSTANCE hInstX, HINSTANCE, LPSTR, int)
{
    hInst = hInstX;

#define MAXCMDTOKENS 128
    int argc;
    LPSTR argv[MAXCMDTOKENS];
    LPSTR p = GetCommandLine();

    char command[256];
    char *args;
    char *d, *e;

    argc = 0;
    args = (char *)malloc(lstrlen(p)+1);
    if (args == (char *)NULL) {
        qCritical("Insufficient memory in WinMain()");
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
    if (argc != 6)
    {
        qWarning("%s :: bad arguments", argv[0]);
        return 1;
    }

    const char* const osc_url  = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char* const label    = argv[5];

    if (strcmp(name, "(none)") == 0)
        name = nullptr;

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

    // Init backend
    set_callback_function(plugin_bridge_callback);
    set_last_error("no error");

    // Init engine
    QString engName = QString("%1 (master)").arg(label);
    engName.truncate(CarlaEngine::maxClientNameSize());

    CarlaEngine engine;
    engine.init(engName.toUtf8().constData());

    // Init toolkit
    toolkit_init();

    // Init plugin client
    client = new PluginData;

    // Init OSC
    osc_init(osc_url);
    osc_send_update();

    // Get plugin type
    switch (itype)
    {
    case PLUGIN_LADSPA:
        id = add_plugin_ladspa(filename, name, label, nullptr);
        break;
    case PLUGIN_DSSI:
        id = add_plugin_dssi(filename, name, label, nullptr);
        break;
    case PLUGIN_LV2:
        id = add_plugin_lv2(filename, name, label);
        break;
    case PLUGIN_VST:
        id = add_plugin_vst(filename, name, label);
        break;
    default:
        id = -1;
        break;
    }

    // Init plugin
    if (id == 0 && CARLA_PLUGIN)
    {
        // Create gui if needed
        GuiInfo guiInfo;
        CARLA_PLUGIN->getGuiInfo(&guiInfo);

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

            qDebug("Wine GUI created");
#else
        if (guiInfo.type == GUI_INTERNAL_QT4 || guiInfo.type == GUI_INTERNAL_X11)
        {
            gui = new QDialog(nullptr);
            gui->resize(10, 10);
            gui->setWindowTitle(guiTitle);
#endif
            CARLA_PLUGIN->setGuiData(0, gui);
        }

        // Report OK to backend
        osc_send_bridge_update();

        // Main loop
        toolkit_loop();

        // Remove & delete plugin
        carla_proc_lock();
        CARLA_PLUGIN->setEnabled(false);
        carla_proc_unlock();

        delete CARLA_PLUGIN;

        // Cleanup
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

    // delete old data
    if (nextChunkFilePath)
    {
        free((void*)nextChunkFilePath);
        nextChunkFilePath = nullptr;
    }

    // Close plugin client
    delete client;
    client = nullptr;

    // Close engine
    engine.close();

    // Close OSC
    osc_send_exiting();
    osc_close();

    return 0;
}

#endif

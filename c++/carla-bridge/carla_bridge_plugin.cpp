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

#ifdef BUILD_BRIDGE_PLUGIN

#include "carla_bridge_client.h"
#include "carla_plugin.h"

#include <QtCore/QFile>

#ifndef __WINE__
#include <QtCore/QTimerEvent>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#endif

#ifdef __WINE__
static HINSTANCE hInstGlobal = nullptr;
#else
static int qargc = 0;
static char* qargv[] = { nullptr };
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------
// client

class CarlaPluginClient : public CarlaClient
{
public:
    CarlaPluginClient(CarlaToolkit* const toolkit)
        : CarlaClient(toolkit)
    {
        engine = nullptr;
        plugin = nullptr;
    }

    ~CarlaPluginClient()
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
        Q_ASSERT(velo > 0);

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

        Q_UNUSED(filePath);

#if 0
        nextChunkFilePath = strdup(filePath);

        while (nextChunkFilePath)
            carla_msleep(25);
#endif
    }

    // ---------------------------------------------------------------------
    // idle

    void idle()
    {
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->idleGui();
        //plugin->showGui(true);
    }

    // ---------------------------------------------------------------------
    // callback

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
                //sendOscConfigure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");
                //quequeMessage(MESSAGE_QUIT, 0, 0, 0.0);
            break;
        case CarlaBackend::CALLBACK_RESIZE_GUI:
            qDebug("resize callback-------------------------------------------------------------------------------");
            quequeMessage(MESSAGE_RESIZE_GUI, value1, value2, 0.0);
            //if (m_toolkit)
            //    m_toolkit->resize(value1, value2);
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
            //quequeMessage(MESSAGE_QUIT, 0, 0, 0.0);
            break;
        default:
            break;
        }
        Q_UNUSED(value1);
        Q_UNUSED(value2);
        Q_UNUSED(value3);
    }

    // ---------------------------------------------------------------------

    static void callback(void* const ptr, CarlaBackend::CallbackType const action, const unsigned short, const int value1, const int value2, const double value3)
    {
        Q_ASSERT(ptr);

        if (! ptr)
            return;

        CarlaPluginClient* const client = (CarlaPluginClient*)ptr;
        client->handleCallback(action, value1, value2, value3);
    }

private:
    CarlaBackend::CarlaEngine* engine;
    CarlaBackend::CarlaPlugin* plugin;
};

// -------------------------------------------------------------------------
// toolkit

#ifndef __WINE__
class BridgeApplication : public QApplication
{
public:
    BridgeApplication()
        : QApplication(qargc, qargv)
    {
        msgTimer = 0;
        m_client = nullptr;
    }

    void exec(CarlaPluginClient* const client)
    {
        m_client = client;
        msgTimer = startTimer(50);

        QApplication::exec();
    }

protected:
    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == msgTimer)
        {
            if (m_client)
            {
                m_client->idle();

                if (! m_client->runMessages())
                    killTimer(msgTimer);
            }
        }

        QApplication::timerEvent(event);
    }

private:
    int msgTimer;
    CarlaPluginClient* m_client;
};
#endif

class CarlaToolkitPlugin : public CarlaToolkit
{
public:
    CarlaToolkitPlugin()
        : CarlaToolkit("carla-bridge-plugin")
    {
        qDebug("CarlaToolkitPlugin::CarlaToolkitPlugin()");
#ifdef __WINE__
        closeNow = false;
        hwnd = nullptr;
#else
        app = nullptr;
        dialog = nullptr;
#endif
    }

    ~CarlaToolkitPlugin()
    {
        qDebug("CarlaToolkitPlugin::~CarlaToolkitPlugin()");
#ifdef __WINE__
        Q_ASSERT(! closeNow);
#else
        Q_ASSERT(! app);
#endif
    }

    void init()
    {
        qDebug("CarlaToolkitPlugin::init()");
#ifdef __WINE__
        Q_ASSERT(! closeNow);

        WNDCLASSEXA wc;
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.style         = 0;
        wc.lpfnWndProc   = windowProcA;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstGlobal;
        wc.hIcon         = LoadIconA(nullptr, IDI_APPLICATION);
        wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
        wc.lpszMenuName  = "MENU_CARLA_BRIDGE";
        wc.lpszClassName = "CLASS_CARLA_BRIDGE";
        wc.hIconSm       = nullptr;
        RegisterClassExA(&wc);
#else
        Q_ASSERT(! app);

        app = new BridgeApplication;
#endif
    }

    void exec(CarlaClient* const client, const bool showGui)
    {
        qDebug("CarlaToolkitPlugin::exec(%p)", client);
        Q_ASSERT(client);

        m_client = client;
        m_client->sendOscUpdate();
        m_client->sendOscBridgeUpdate();

        if (showGui)
            show();

#ifdef __WINE__
        Q_ASSERT(! closeNow);

        MSG msg;
        CarlaPluginClient* const pluginClient = (CarlaPluginClient*)client;

        while (! closeNow)
        {
            //pluginClient->runMessages();
            //pluginClient->idle();

            //if (closeNow)
            //    break;

            while (GetMessageA(&msg, hwnd, 0, 0) > 0)
            {
            //if (PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE))
            //{
                //TranslateMessage(&msg);
                DispatchMessageA(&msg);

                pluginClient->runMessages();
                pluginClient->idle();
            }

            //if (closeNow)
            //    break;

            //carla_msleep(50);
        }
#else
        Q_ASSERT(app);

        app->exec((CarlaPluginClient*)client);
#endif
    }

    void quit()
    {
        qDebug("CarlaToolkitPlugin::quit()");
#ifdef __WINE__
        if (closeNow && hwnd)
        {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }

        closeNow = true;
#else
        Q_ASSERT(app);

        if (dialog)
        {
            dialog->close();

            delete dialog;
            dialog = nullptr;
        }

        if (app)
        {
            if (! app->closingDown())
                app->quit();

            delete app;
            app = nullptr;
        }
#endif
    }

    void show()
    {
        qDebug("CarlaToolkitPlugin::show()");
#ifdef __WINE__
        Q_ASSERT(hwnd);

        if (hwnd)
        {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
        }
#else
        Q_ASSERT(dialog);

        if (dialog)
            dialog->show();
#endif
    }

    void hide()
    {
        qDebug("CarlaToolkitPlugin::hide()");
#ifdef __WINE__
        Q_ASSERT(hwnd);

        if (hwnd)
            ShowWindow(hwnd, SW_HIDE);
#else
        Q_ASSERT(dialog);

        if (dialog)
            dialog->show();
#endif
    }

    void resize(int width, int height)
    {
        qDebug("CarlaToolkitPlugin::resize(%i, %i)", width, height);
#ifdef __WINE__
        Q_ASSERT(hwnd);

        if (hwnd)
            SetWindowPos(hwnd, 0, 0, 0, width + 6, height + 25, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
        Q_ASSERT(dialog);

        if (dialog)
            dialog->setFixedSize(width, height);
#endif
    }

    // ---------------------------------------------------------------------

    void createWindow(const char* const pluginName)
    {
#ifdef __WINE__
        hwnd = CreateWindowA("CLASS_CARLA_BRIDGE", pluginName, WS_OVERLAPPEDWINDOW &~ WS_THICKFRAME &~ WS_MAXIMIZEBOX,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              HWND_DESKTOP, nullptr, hInstGlobal, nullptr);

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowPos(hwnd, 0, 0, 0, 1100 + 6, 600 + 25, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
        dialog = new QDialog(nullptr);
        dialog->resize(10, 10);
        //window->setLayout(new QVBoxLayout(dialog);
        dialog->setWindowTitle(QString("%1 (GUI)").arg(pluginName));
#endif
    }

    CarlaBackend::GuiDataHandle getWindowHandle() const
    {
#ifdef __WINE__
        return hwnd;
#else
        return dialog;
#endif
    }

    // ---------------------------------------------------------------------

private:
#ifdef __WINE__
    bool closeNow;
    HWND hwnd;

    void handleWindowCloseMessageA()
    {
        //m_client->quequeMessage(MESSAGE_QUIT, 0, 0, 0.0);
        //m_client->quequeMessage(MESSAGE_SHOW_GUI, 0, 0, 0.0);
        closeNow = true;
        m_client->sendOscConfigure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");
    }


    static LRESULT CALLBACK windowProcA(HWND _hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        qDebug("windowProcA(%p, %i, %li, %li)", _hwnd, message, wParam, lParam);

        if (message == WM_CLOSE || message == WM_DESTROY)
        {
            CarlaToolkitPlugin* const toolkit = (CarlaToolkitPlugin*)GetWindowLongPtrA(_hwnd, GWLP_USERDATA);
            Q_ASSERT(toolkit);

            if (toolkit)
            {
                toolkit->handleWindowCloseMessageA();
                return TRUE;
            }
        }

        return DefWindowProcA(_hwnd, message, wParam, lParam);
    }
#else
    BridgeApplication* app;
    QDialog* dialog;
#endif
};

CarlaToolkit* CarlaToolkit::createNew(const char* const)
{
    return new CarlaToolkitPlugin;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#ifdef __WINE__
int WINAPI WinMain(HINSTANCE hInstX, HINSTANCE, LPSTR, int)
{
    hInstGlobal = hInstX;

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
        GetModuleFileName(hInstGlobal, command, sizeof(command)-1);
        argv[0] = command;
    }
#else
int main(int argc, char* argv[])
{
#endif
    if (argc != 6)
    {
        qWarning("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label>", argv[0]);
        return 1;
    }

    const char* const oscUrl   = argv[1];
    const char* const stype    = argv[2];
    const char* const filename = argv[3];
    const char*       name     = argv[4];
    const char* const label    = argv[5];

    const bool useOsc = strcmp(oscUrl, "null");

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
    CarlaBridge::CarlaToolkitPlugin toolkit;
    toolkit.init();

    // Init client
    CarlaBridge::CarlaPluginClient client(&toolkit);

    // Init OSC
    if (useOsc && ! client.oscInit(oscUrl))
    {
       toolkit.quit();
       return -1;
    }

    // Init backend engine
    CarlaBackend::CarlaEngineJack engine;
    engine.setCallback(client.callback, &client);

    // bridge client <-> engine
    client.registerOscEngine(&engine);

    // Init engine
    QString engName = QString("%1 (master)").arg(label);
    engName.truncate(engine.maxClientNameSize());
    engine.init(engName.toUtf8().constData());

    /// Init plugin
    short id = engine.addPlugin(itype, filename, name, label);

    if (id >= 0 && id < CarlaBackend::MAX_PLUGINS)
    {
        CarlaBackend::CarlaPlugin* const plugin = engine.getPlugin(id);
        client.setStuff(&engine, plugin);

        {
            // create window if needed
            toolkit.createWindow(plugin->name());
            plugin->setGuiData(toolkit.getWindowHandle());
        }

        if (! useOsc)
        {
            plugin->setActive(true, false, false);
            plugin->showGui(true);
        }
    }
    else
    {
        qWarning("Plugin failed to load, error was:\n%s", CarlaBackend::getLastError());
        return 1;
    }

    toolkit.exec(&client, !useOsc);

    engine.removeAllPlugins();
    engine.close();

    // Close OSC
    if (useOsc)
    {
        client.sendOscExiting();
        client.oscClose();
    }

    // Close toolkit
    toolkit.quit();

    return 0;
}

#endif // BUILD_BRIDGE_PLUGIN

/*
 * Carla Plugin bridge code
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

#ifdef BUILD_BRIDGE_PLUGIN

#include "carla_bridge_client.h"
#include "carla_plugin.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimerEvent>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

static int    qargc = 0;
static char** qargv = nullptr;

bool qcloseNow = false;

#if defined(Q_OS_UNIX)
void closeSignalHandler(int)
{
    qcloseNow = true;
}
#elif defined(Q_OS_WIN)
BOOL WINAPI closeSignalHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        qcloseNow = true;
        return TRUE;
    }

    return FALSE;
}
#endif

void initSignalHandler()
{
#if defined(Q_OS_UNIX)
    struct sigaction sint, sterm;

    sint.sa_handler = closeSignalHandler;
    sigemptyset(&sint.sa_mask);
    sint.sa_flags |= SA_RESTART;
    sigaction(SIGINT, &sint, 0);

    sterm.sa_handler = closeSignalHandler;
    sigemptyset(&sterm.sa_mask);
    sterm.sa_flags |= SA_RESTART;
    sigaction(SIGTERM, &sterm, 0);
#elif defined(Q_OS_WIN)
    SetConsoleCtrlHandler(closeSignalHandler, TRUE);
#endif
}

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
        Q_ASSERT(engine_);
        Q_ASSERT(plugin_);

        engine = engine_;
        plugin = plugin_;
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const double value)
    {
        qDebug("CarlaPluginClient::setParameter(%i, %g)", rindex, value);
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setParameterValueByRIndex(rindex, value, true, true, false);
    }

    void setProgram(const uint32_t index)
    {
        qDebug("CarlaPluginClient::setProgram(%i)", index);
        Q_ASSERT(engine);
        Q_ASSERT(plugin);
        Q_ASSERT(index < plugin->programCount());

        if (! (plugin && engine))
            return;
        if (index >= plugin->programCount())
            return;

        plugin->setProgram(index, true, true, false, true);

        double value;
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
        {
            value = plugin->getParameterValue(i);
            engine->osc_send_bridge_set_parameter_value(i, value);
            engine->osc_send_bridge_set_default_value(i, value);
        }
    }

    void setMidiProgram(const uint32_t index)
    {
        qDebug("CarlaPluginClient::setMidiProgram(%i)", index);
        Q_ASSERT(engine);
        Q_ASSERT(plugin);

        if (! (plugin && engine))
            return;

        plugin->setMidiProgram(index, true, true, false, true);

        double value;
        for (uint32_t i=0; i < plugin->parameterCount(); i++)
        {
            value = plugin->getParameterValue(i);
            engine->osc_send_bridge_set_parameter_value(i, value);
            engine->osc_send_bridge_set_default_value(i, value);
        }
    }

    void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo)
    {
        qDebug("CarlaPluginClient::noteOn(%i, %i, %i)", channel, note, velo);
        Q_ASSERT(plugin);
        Q_ASSERT(velo > 0);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, velo, true, true, false);
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        qDebug("CarlaPluginClient::noteOff(%i, %i)", channel, note);
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, 0, true, true, false);
    }

    // ---------------------------------------------------------------------
    // plugin

    void saveNow()
    {
        qDebug("CarlaPluginClient::saveNow()");
        Q_ASSERT(plugin);
        Q_ASSERT(engine);

        if (! (plugin && engine))
            return;

        plugin->prepareForSave();

        for (uint32_t i=0; i < plugin->customDataCount(); i++)
        {
            const CarlaBackend::CustomData* const cdata = plugin->customData(i);
            engine->osc_send_bridge_set_custom_data(CarlaBackend::getCustomDataTypeString(cdata->type), cdata->key, cdata->value);
        }

        if (plugin->hints() & CarlaBackend::PLUGIN_USES_CHUNKS)
        {
            void* data = nullptr;
            int32_t dataSize = plugin->chunkData(&data);

            if (data && dataSize >= 4)
            {
                QString filePath;
                filePath = QDir::tempPath();
#ifdef Q_OS_WIN
                filePath += "\\.CarlaChunk_";
#else
                filePath += "/.CarlaChunk_";
#endif
                filePath += plugin->name();

                QFile file(filePath);

                if (file.open(QIODevice::WriteOnly))
                {
                    QByteArray chunk((const char*)data, dataSize);
                    file.write(chunk);
                    file.close();
                    engine->osc_send_bridge_set_chunk_data(filePath.toUtf8().constData());
                }
            }
        }

        engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_SAVED, "");
    }

    void setCustomData(const char* const type, const char* const key, const char* const value)
    {
        qDebug("CarlaPluginClient::setCustomData(\"%s\", \"%s\", \"%s\")", type, key, value);
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setCustomData(CarlaBackend::getCustomDataStringType(type), key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        qDebug("CarlaPluginClient::setChunkData(\"%s\")", filePath);
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
    }

    void showGui(const bool yesNo)
    {
        qDebug("CarlaPluginClient::showGui(%s)", bool2str(yesNo));
        Q_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->showGui(yesNo);
    }

    // ---------------------------------------------------------------------
    // callback

    void handleCallback(const CarlaBackend::CallbackType action, const int value1, const int value2, const double value3)
    {
        qDebug("CarlaPluginClient::handleCallback(%s, %i, %i, %g)", CarlaBackend::CallbackType2str(action), value1, value2, value3);

        if (! engine)
            return;

        switch (action)
        {
        case CarlaBackend::CALLBACK_PARAMETER_VALUE_CHANGED:
            engine->osc_send_bridge_set_parameter_value(value1, value3);
            break;

        case CarlaBackend::CALLBACK_PROGRAM_CHANGED:
            engine->osc_send_bridge_set_program(value1);
            break;

        case CarlaBackend::CALLBACK_MIDI_PROGRAM_CHANGED:
            engine->osc_send_bridge_set_midi_program(value1);
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
            if (value1 == 0)
                engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");
            break;

        case CarlaBackend::CALLBACK_RESIZE_GUI:
            if (m_toolkit)
                m_toolkit->resize(value1, value2);
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

    void killMsgTimer()
    {
        Q_ASSERT(msgTimer != 0);

        killTimer(msgTimer);
        msgTimer = 0;
    }

protected:
    void timerEvent(QTimerEvent* const event)
    {
        if (qcloseNow)
            return quit();

        if (event->timerId() == msgTimer)
        {
            if (m_client)
            {
                m_client->idle();

                if (! m_client->runMessages())
                {
                    msgTimer = 0;
                    return;
                }
            }
        }

        QApplication::timerEvent(event);
    }

private:
    int msgTimer;
    CarlaPluginClient* m_client;
};

class CarlaToolkitPlugin : public CarlaToolkit
{
public:
    CarlaToolkitPlugin()
        : CarlaToolkit("carla-bridge-plugin")
    {
        qDebug("CarlaToolkitPlugin::CarlaToolkitPlugin()");
        app = nullptr;
        dialog = nullptr;
        m_resizable = false;
    }

    ~CarlaToolkitPlugin()
    {
        qDebug("CarlaToolkitPlugin::~CarlaToolkitPlugin()");
        Q_ASSERT(! app);
    }

    void init()
    {
        qDebug("CarlaToolkitPlugin::init()");
        Q_ASSERT(! app);

        app = new BridgeApplication;
    }

    void exec(CarlaClient* const client, const bool showGui)
    {
        qDebug("CarlaToolkitPlugin::exec(%p)", client);
        Q_ASSERT(app);
        Q_ASSERT(client);

        m_client = client;

        if (showGui)
        {
            show();
        }
        else
        {
            m_client->sendOscUpdate();
            m_client->sendOscBridgeUpdate();
            app->setQuitOnLastWindowClosed(false);
        }

        app->exec((CarlaPluginClient*)client);
    }

    void quit()
    {
        qDebug("CarlaToolkitPlugin::quit()");
        Q_ASSERT(app);

        if (dialog)
        {
            dialog->close();

            delete dialog;
            dialog = nullptr;
        }

        if (app)
        {
            app->killMsgTimer();

            if (! app->closingDown())
                app->quit();

            delete app;
            app = nullptr;
        }
    }

    void show()
    {
        qDebug("CarlaToolkitPlugin::show()");

        if (m_client)
            ((CarlaPluginClient*)m_client)->showGui(true);

        if (dialog)
            dialog->show();
    }

    void hide()
    {
        qDebug("CarlaToolkitPlugin::hide()");

        if (dialog)
            dialog->hide();

        if (m_client)
            ((CarlaPluginClient*)m_client)->showGui(false);
    }

    void resize(int width, int height)
    {
        qDebug("CarlaToolkitPlugin::resize(%i, %i)", width, height);
        Q_ASSERT(dialog);

        if (! dialog)
            return;

        if (m_resizable)
            dialog->resize(width, height);
        else
            dialog->setFixedSize(width, height);
    }

    // ---------------------------------------------------------------------

    void createWindow(const char* const pluginName, const bool createLayout, const bool resizable)
    {
        qDebug("CarlaToolkitPlugin::createWindow(%s, %s, %s)", pluginName, bool2str(createLayout), bool2str(resizable));
        Q_ASSERT(pluginName);

        m_resizable = resizable;

        dialog = new QDialog(nullptr);
        resize(10, 10);

        if (createLayout)
        {
            QVBoxLayout* const layout = new QVBoxLayout(dialog);
            dialog->setContentsMargins(0, 0, 0, 0);
            dialog->setLayout(layout);
        }

        dialog->setWindowTitle(QString("%1 (GUI)").arg(pluginName));

#ifdef Q_OS_WIN
        if (! resizable)
            dialog->setWindowFlags(dialog->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
    }

    QDialog* getWindowHandle() const
    {
        return dialog;
    }

    // ---------------------------------------------------------------------

private:
    BridgeApplication* app;
    QDialog* dialog;
    bool m_resizable;
};

CarlaToolkit* CarlaToolkit::createNew(const char* const)
{
    return new CarlaToolkitPlugin;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
        qWarning("usage: %s <osc-url|\"null\"> <type> <filename> <name|\"(none)\"> <label>", argv[0]);
        return 1;
    }

    qargc = argc;
    qargv = argv;

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
    QString engName = QString("%1 (master)").arg(name ? name : label);
    engName.truncate(engine.maxClientNameSize());

    if (! engine.init(engName.toUtf8().constData()))
    {
        qWarning("Bridge engine failed to start, error was:\n%s", CarlaBackend::getLastError());
        engine.close();
        toolkit.quit();
        return 2;
    }

    /// Init plugin
    short id = engine.addPlugin(itype, filename, name, label);
    int ret;

    if (id >= 0 && id < CarlaBackend::MAX_PLUGINS)
    {
        CarlaBackend::CarlaPlugin* const plugin = engine.getPlugin(id);
        client.setStuff(&engine, plugin);

        // create window if needed
        bool guiResizable;
        CarlaBackend::GuiType guiType;
        plugin->getGuiInfo(&guiType, &guiResizable);

        if (guiType == CarlaBackend::GUI_INTERNAL_QT4 || guiType == CarlaBackend::GUI_INTERNAL_COCOA || guiType == CarlaBackend::GUI_INTERNAL_HWND || guiType == CarlaBackend::GUI_INTERNAL_X11)
        {
            toolkit.createWindow(plugin->name(), (guiType == CarlaBackend::GUI_INTERNAL_QT4), guiResizable);
            plugin->setGuiData(toolkit.getWindowHandle());
        }

        if (! useOsc)
            plugin->setActive(true, false, false);

        ret = 0;
    }
    else
    {
        qWarning("Plugin failed to load, error was:\n%s", CarlaBackend::getLastError());
        ret = 1;
    }

    if (ret == 0)
    {
        initSignalHandler();
        toolkit.exec(&client, !useOsc);
    }

    engine.removeAllPlugins();
    engine.close();

    if (useOsc)
    {
        // Close OSC
        client.sendOscExiting();
        client.oscClose();
        // toolkit can't be closed manually, only by host
    }
    else
    {
        // Close toolkit
        toolkit.quit();
    }

    return ret;
}

#endif // BUILD_BRIDGE_PLUGIN

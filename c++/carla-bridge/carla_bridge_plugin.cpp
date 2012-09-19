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
#include <QtGui/QtEvents>

#ifdef Q_OS_UNIX
#include <signal.h>
#endif

static int    qargc     = 0;
static char** qargv     = nullptr;
static bool   qCloseNow = false;

#if defined(Q_OS_UNIX)
void closeSignalHandler(int)
{
    qCloseNow = true;
}
#elif defined(Q_OS_WIN)
BOOL WINAPI closeSignalHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT)
    {
        qCloseNow = true;
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

class BridgePluginGUI : public QDialog
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    BridgePluginGUI(QWidget* const parent, Callback* const callback_, const char* const pluginName, const bool resizable)
        : QDialog(parent),
          callback(callback_)
    {
        qDebug("BridgePluginGUI::BridgePluginGUI(%p, %p, \"%s\", %s", parent, callback, pluginName, bool2str(resizable));
        Q_ASSERT(callback);
        Q_ASSERT(pluginName);

        m_firstShow = true;
        m_resizable = resizable;

        vbLayout = new QVBoxLayout(this);
        vbLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vbLayout);

        container = new GuiContainer(this);
        vbLayout->addWidget(container);

        setNewSize(50, 50);
        setWindowTitle(QString("%1 (GUI)").arg(pluginName));

#ifdef Q_OS_WIN
        if (! resizable)
            setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
    }

    ~BridgePluginGUI()
    {
        qDebug("BridgePluginGUI::~BridgePluginGUI()");
        Q_ASSERT(container);
        Q_ASSERT(vbLayout);

        delete container;
        delete vbLayout;
    }

    GuiContainer* getContainer()
    {
        return container;
    }

    void setNewSize(int width, int height)
    {
        qDebug("BridgePluginGUI::setNewSize(%i, %i)", width, height);

        if (width < 30)
            width = 30;
        if (height < 30)
            height = 30;

        if (m_resizable)
        {
            resize(width, height);
        }
        else
        {
            setFixedSize(width, height);
            container->setFixedSize(width, height);
        }
    }

    void setVisible(const bool yesNo)
    {
        qDebug("BridgePluginGUI::setVisible(%s)", bool2str(yesNo));

        if (yesNo)
        {
            if (m_firstShow)
            {
                m_firstShow = false;
                restoreGeometry(QByteArray());
            }
            else if (! m_geometry.isNull())
                restoreGeometry(m_geometry);
        }
        else
            m_geometry = saveGeometry();

        QDialog::setVisible(yesNo);
    }

protected:
    void hideEvent(QHideEvent* const event)
    {
        qDebug("BridgePluginGUI::hideEvent(%p)", event);

        event->accept();
        close();
    }

    void closeEvent(QCloseEvent* const event)
    {
        qDebug("BridgePluginGUI::closeEvent(%p)", event);

        if (event->spontaneous())
            callback->guiClosedCallback();

        QDialog::closeEvent(event);
    }

    void done(int r)
    {
        QDialog::done(r);
        close();
    }

private:
    Callback* const callback;

    QVBoxLayout*  vbLayout;
    GuiContainer* container;

    bool m_firstShow;
    bool m_resizable;
    QByteArray m_geometry;
};

// -------------------------------------------------------------------------

class BridgePluginClient : public CarlaToolkit,
                           public CarlaClient,
                           public BridgePluginGUI::Callback,
                           public QApplication
{
public:
    BridgePluginClient()
        : CarlaToolkit("carla-bridge-plugin"),
          CarlaClient(this),
          QApplication(qargc, qargv)
    {
        qDebug("BridgePluginClient::BridgePluginClient()");

        msgTimer   = 0;
        nextWidth  = 0;
        nextHeight = 0;

        engine     = nullptr;
        plugin     = nullptr;
        pluginGui  = nullptr;

        m_client   = this;
    }

    ~BridgePluginClient()
    {
        qDebug("BridgePluginClient::~BridgePluginClient()");
        Q_ASSERT(msgTimer == 0);
        Q_ASSERT(! pluginGui);
    }

    void setStuff(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin)
    {
        qDebug("BridgePluginClient::setStuff(%p, %p)", engine, plugin);
        Q_ASSERT(engine);
        Q_ASSERT(plugin);

        this->engine = engine;
        this->plugin = plugin;
    }

    // ---------------------------------------------------------------------
    // toolkit

    void init()
    {
        qDebug("BridgePluginClient::init()");
    }

    void exec(CarlaClient* const, const bool showGui)
    {
        qDebug("BridgePluginClient::exec()");

        if (showGui)
        {
            show();
        }
        else
        {
            CarlaClient::sendOscUpdate();
            CarlaClient::sendOscBridgeUpdate();
            QApplication::setQuitOnLastWindowClosed(false);
        }

        msgTimer = startTimer(50);

        QApplication::exec();
    }

    void quit()
    {
        qDebug("BridgePluginClient::quit()");

        if (msgTimer != 0)
        {
            QApplication::killTimer(msgTimer);
            msgTimer = 0;
        }

        if (pluginGui)
        {
            if (pluginGui->isVisible())
                hide();

            pluginGui->close();

            delete pluginGui;
            pluginGui = nullptr;
        }

        if (! QApplication::closingDown())
            QApplication::quit();
    }

    void show()
    {
        qDebug("BridgePluginClient::show()");
        Q_ASSERT(pluginGui);

        if (plugin)
            plugin->showGui(true);

        if (pluginGui)
            pluginGui->show();
    }

    void hide()
    {
        qDebug("BridgePluginClient::hide()");
        Q_ASSERT(pluginGui);

        if (pluginGui)
            pluginGui->hide();

        if (plugin)
            plugin->showGui(false);
    }

    void resize(int width, int height)
    {
        qDebug("BridgePluginClient::resize(%i, %i)", width, height);
        Q_ASSERT(pluginGui);

        if (pluginGui)
            pluginGui->setNewSize(width, height);
    }

    // ---------------------------------------------------------------------

    void createWindow(const bool resizable)
    {
        qDebug("BridgePluginClient::createWindow(%s)", bool2str(resizable));
        Q_ASSERT(plugin);

        pluginGui = new BridgePluginGUI(nullptr, this, plugin->name(), resizable);
        plugin->setGuiContainer(pluginGui->getContainer());
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
            Q_ASSERT(value1 > 0 && value2 > 0);
            nextWidth  = value1;
            nextHeight = value2;
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
            //QApplication::quit();
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

        BridgePluginClient* const _this_ = (BridgePluginClient*)ptr;
        _this_->handleCallback(action, value1, value2, value3);
    }

protected:
    void guiClosedCallback()
    {
    }

    void timerEvent(QTimerEvent* const event)
    {
        if (qCloseNow)
            return quit();

        if (event->timerId() == msgTimer)
        {
            if (nextWidth > 0 && nextHeight > 0 && pluginGui)
            {
                pluginGui->setNewSize(nextWidth, nextHeight);
                nextWidth  = 0;
                nextHeight = 0;
            }

            if (plugin)
                plugin->idleGui();

            if (! CarlaClient::runMessages())
            {
                Q_ASSERT(msgTimer == 0);
                msgTimer = 0;
                return;
            }
        }

        QApplication::timerEvent(event);
    }

    // ---------------------------------------------------------------------

private:
    int msgTimer;
    int nextWidth, nextHeight;

    CarlaBackend::CarlaEngine* engine;
    CarlaBackend::CarlaPlugin* plugin;

    BridgePluginGUI* pluginGui;
};

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

    // Init bridge client
    CarlaBridge::BridgePluginClient client;
    client.init();

    // Init OSC
    if (useOsc && ! client.oscInit(oscUrl))
    {
        client.quit();
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
        const char* const lastError = CarlaBackend::getLastError();
        qWarning("Bridge engine failed to start, error was:\n%s", lastError);
        engine.close();
        client.sendOscBridgeError(lastError);
        client.quit();
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
            client.createWindow(guiResizable);
        }

        if (! useOsc)
            plugin->setActive(true, false, false);

        ret = 0;
    }
    else
    {
        const char* const lastError = CarlaBackend::getLastError();
        qWarning("Plugin failed to load, error was:\n%s", lastError);
        client.sendOscBridgeError(lastError);
        ret = 1;
    }

    if (ret == 0)
    {
        initSignalHandler();
        client.exec(nullptr, !useOsc);
    }

    engine.removeAllPlugins();
    engine.close();

    if (useOsc)
    {
        // Close OSC
        client.sendOscExiting();
        client.oscClose();
        // bridge client can't be closed manually, only by host
    }
    else
    {
        // Close bridge client
        client.quit();
    }

    return ret;
}

#endif // BUILD_BRIDGE_PLUGIN

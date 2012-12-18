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

#include "carla_bridge_client.hpp"
#include "carla_backend_utils.hpp"
#include "carla_plugin.hpp"

#include <set>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimerEvent>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QtEvents>

#ifdef Q_OS_UNIX
# include <signal.h>
#endif

static int    qargc     = 0;
static char** qargv     = nullptr;
static bool   qCloseNow = false;
static bool   qSaveNow  = false;

#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
void closeSignalHandler(int)
{
    qCloseNow = true;
}
void saveSignalHandler(int)
{
    qSaveNow = true;
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
#if defined(Q_OS_HAIKU) || defined(Q_OS_UNIX)
    struct sigaction sint;
    struct sigaction sterm;
    struct sigaction susr1;

    sint.sa_handler  = closeSignalHandler;
    sint.sa_flags    = SA_RESTART;
    sint.sa_restorer = nullptr;
    sigemptyset(&sint.sa_mask);
    sigaction(SIGINT, &sint, nullptr);

    sterm.sa_handler  = closeSignalHandler;
    sterm.sa_flags    = SA_RESTART;
    sterm.sa_restorer = nullptr;
    sigemptyset(&sterm.sa_mask);
    sigaction(SIGTERM, &sterm, nullptr);

    susr1.sa_handler  = saveSignalHandler;
    susr1.sa_flags    = SA_RESTART;
    susr1.sa_restorer = nullptr;
    sigemptyset(&susr1.sa_mask);
    sigaction(SIGUSR1, &susr1, nullptr);
#elif defined(Q_OS_WIN)
    SetConsoleCtrlHandler(closeSignalHandler, TRUE);
#endif
}

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class BridgePluginGUI : public QMainWindow
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    BridgePluginGUI(QWidget* const parent, Callback* const callback_)
        : QMainWindow(parent),
          callback(callback_)
    {
        qDebug("BridgePluginGUI::BridgePluginGUI(%p, %p", parent, callback);
        CARLA_ASSERT(callback);

        m_firstShow = true;
        m_resizable = true;

        container = new GuiContainer(this);
        setCentralWidget(container);

        setNewSize(50, 50);
    }

    ~BridgePluginGUI()
    {
        qDebug("BridgePluginGUI::~BridgePluginGUI()");
        CARLA_ASSERT(container);

        delete container;
    }

    GuiContainer* getContainer()
    {
        return container;
    }

    void setResizable(bool resizable)
    {
        m_resizable = resizable;
        setNewSize(width(), height());

#ifdef Q_OS_WIN
        if (! resizable)
            setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
    }

    void setTitle(const char* title)
    {
        CARLA_ASSERT(title);
        setWindowTitle(QString("%1 (GUI)").arg(title));
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

        QMainWindow::setVisible(yesNo);
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
        {
            callback->guiClosedCallback();
            QMainWindow::closeEvent(event);
            return;
        }

        event->ignore();
    }

private:
    Callback* const callback;

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
          QApplication(qargc, qargv, true)
    {
        qDebug("BridgePluginClient::BridgePluginClient()");

        msgTimerGUI = 0;
        msgTimerOSC = 0;

        engine    = nullptr;
        plugin    = nullptr;
        pluginGui = nullptr;

        m_client = this;
        m_doQuit = false;
        m_hasUI  = false;
        m_qt4UI  = false;

        m_needsResize = false;
        m_nextSize[0] = 0;
        m_nextSize[1] = 0;
    }

    ~BridgePluginClient()
    {
        qDebug("BridgePluginClient::~BridgePluginClient()");
        CARLA_ASSERT(msgTimerGUI == 0);
        CARLA_ASSERT(msgTimerOSC == 0);
        CARLA_ASSERT(! pluginGui);
    }

    void setStuff(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin)
    {
        qDebug("BridgePluginClient::setStuff(%p, %p)", engine, plugin);
        CARLA_ASSERT(engine);
        CARLA_ASSERT(plugin);

        this->engine = engine;
        this->plugin = plugin;
    }

    // ---------------------------------------------------------------------
    // toolkit

    void init()
    {
        qDebug("BridgePluginClient::init()");

        pluginGui = new BridgePluginGUI(nullptr, this);
        pluginGui->hide();
    }

    void exec(CarlaClient* const, const bool showGui)
    {
        qDebug("BridgePluginClient::exec()");

        if (showGui)
        {
            if (m_hasUI)
                show();

            m_doQuit = true;
        }
        else
        {
            CarlaClient::sendOscUpdate();
            CarlaClient::sendOscBridgeUpdate();
            QApplication::setQuitOnLastWindowClosed(false);
        }

        msgTimerGUI = startTimer(50);
        msgTimerOSC = startTimer(25);

        QApplication::exec();
    }

    void quit()
    {
        qDebug("BridgePluginClient::quit()");

        if (msgTimerGUI != 0)
        {
            QApplication::killTimer(msgTimerGUI);
            msgTimerGUI = 0;
        }

        if (msgTimerOSC != 0)
        {
            QApplication::killTimer(msgTimerOSC);
            msgTimerOSC = 0;
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
        CARLA_ASSERT(pluginGui);

        if (plugin)
            plugin->showGui(true);

        if (pluginGui && m_qt4UI)
            pluginGui->show();
    }

    void hide()
    {
        qDebug("BridgePluginClient::hide()");
        CARLA_ASSERT(pluginGui);

        if (pluginGui && m_qt4UI)
            pluginGui->hide();

        if (plugin)
            plugin->showGui(false);
    }

    void resize(int width, int height)
    {
        qDebug("BridgePluginClient::resize(%i, %i)", width, height);
        CARLA_ASSERT(pluginGui);

        if (pluginGui)
            pluginGui->setNewSize(width, height);
    }

    // ---------------------------------------------------------------------

    void createWindow(const bool resizable)
    {
        qDebug("BridgePluginClient::createWindow(%s)", bool2str(resizable));
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(pluginGui);

        if (! (plugin && pluginGui))
            return;

        m_hasUI = true;
        m_qt4UI = true;

        pluginGui->setResizable(resizable);
        pluginGui->setTitle(plugin->name());

        plugin->setGuiContainer(pluginGui->getContainer());
    }

    void enableUI()
    {
        m_hasUI = true;
    }

    // ---------------------------------------------------------------------
    // processing

    void setParameter(const int32_t rindex, const double value)
    {
        qDebug("CarlaPluginClient::setParameter(%i, %g)", rindex, value);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setParameterValueByRIndex(rindex, value, true, true, false);
    }

    void setProgram(const uint32_t index)
    {
        qDebug("CarlaPluginClient::setProgram(%i)", index);
        CARLA_ASSERT(engine);
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(index < plugin->programCount());

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
        CARLA_ASSERT(engine);
        CARLA_ASSERT(plugin);

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
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(velo > 0);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, velo, true, true, false);
    }

    void noteOff(const uint8_t channel, const uint8_t note)
    {
        qDebug("CarlaPluginClient::noteOff(%i, %i)", channel, note);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->sendMidiSingleNote(channel, note, 0, true, true, false);
    }

    // ---------------------------------------------------------------------
    // plugin

    void saveNow()
    {
        qDebug("CarlaPluginClient::saveNow()");
        CARLA_ASSERT(plugin);
        CARLA_ASSERT(engine);

        if (! (plugin && engine))
            return;

        plugin->prepareForSave();

        for (uint32_t i=0; i < plugin->customDataCount(); i++)
        {
            const CarlaBackend::CustomData* const cdata = plugin->customData(i);
            engine->osc_send_bridge_set_custom_data(cdata->type, cdata->key, cdata->value);
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
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        plugin->setCustomData(type, key, value, true);
    }

    void setChunkData(const char* const filePath)
    {
        qDebug("CarlaPluginClient::setChunkData(\"%s\")", filePath);
        CARLA_ASSERT(plugin);

        if (! plugin)
            return;

        QString chunkFilePath(filePath);

#ifdef Q_OS_WIN
        if (chunkFilePath.startsWith("/"))
        {
            // running under Wine, posix host
            chunkFilePath = chunkFilePath.replace(0, 1, "Z:/");
            chunkFilePath = QDir::toNativeSeparators(chunkFilePath);
        }
#endif
        QFile chunkFile(chunkFilePath);

        if (plugin && chunkFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&chunkFile);
            QString stringData(in.readAll());
            chunkFile.close();
            chunkFile.remove();

            plugin->setChunkData(stringData.toUtf8().constData());
        }
    }

    // ---------------------------------------------------------------------
    // callback

    void handleCallback(const CarlaBackend::CallbackType action, const int value1, const int value2, const double value3, const char* const valueStr)
    {
        qDebug("CarlaPluginClient::handleCallback(%s, %i, %i, %g \"%s\")", CarlaBackend::CallbackType2Str(action), value1, value2, value3, valueStr);

        if (! engine)
            return;

        switch (action)
        {
        case CarlaBackend::CALLBACK_PARAMETER_VALUE_CHANGED:
#if 0
            parametersToUpdate.insert(value1);
#endif
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
            {
                engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");

                if (m_doQuit)
                    qCloseNow = true;
            }
            break;

        case CarlaBackend::CALLBACK_RESIZE_GUI:
            CARLA_ASSERT(value1 > 0 && value2 > 0);
            CARLA_ASSERT(pluginGui);

            if (value1 > 0 && value2 > 0 && pluginGui)
            {
                m_needsResize = true;
                m_nextSize[0] = value1;
                m_nextSize[1] = value2;
            }

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
    }

    // ---------------------------------------------------------------------

    static void callback(void* const ptr, CarlaBackend::CallbackType const action, const unsigned short, const int value1, const int value2, const double value3, const char* const valueStr)
    {
        CARLA_ASSERT(ptr);

        if (! ptr)
            return;

        BridgePluginClient* const _this_ = (BridgePluginClient*)ptr;
        _this_->handleCallback(action, value1, value2, value3, valueStr);
    }

protected:
    void guiClosedCallback()
    {
        if (engine)
            engine->osc_send_bridge_configure(CarlaBackend::CARLA_BRIDGE_MSG_HIDE_GUI, "");
    }

    void timerEvent(QTimerEvent* const event)
    {
        if (qCloseNow)
            return quit();

        if (qSaveNow)
        {
            // TODO
            qSaveNow = false;
        }

        if (event->timerId() == msgTimerGUI)
        {
#if 0
            if (parametersToUpdate.size() > 0)
            {
                for (auto it = parametersToUpdate.begin(); it != parametersToUpdate.end(); it++)
                {
                    const int32_t paramId(*it);
                    engine->osc_send_bridge_set_parameter_value(paramId, plugin->getParameterValue(paramId));
                }

                parametersToUpdate.clear();
            }
#endif

            if (plugin)
                plugin->idleGui();

            if (pluginGui && m_needsResize)
            {
                pluginGui->setNewSize(m_nextSize[0], m_nextSize[1]);
                m_needsResize = false;
            }
        }
        else if (event->timerId() == msgTimerOSC)
        {
            if (! CarlaClient::oscIdle())
            {
                CARLA_ASSERT(msgTimerOSC == 0);
                msgTimerOSC = 0;
                return;
            }
        }

        QApplication::timerEvent(event);
    }

    // ---------------------------------------------------------------------

private:
    int msgTimerGUI, msgTimerOSC;
#if 0
    std::set<int32_t> parametersToUpdate;
#endif

    bool m_doQuit;
    bool m_hasUI;
    bool m_qt4UI;
    bool m_needsResize;
    int  m_nextSize[2];

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

    // Listen for ctrl+c or sigint/sigterm events
    initSignalHandler();

    // Init backend engine
    CarlaBackend::CarlaEngine* engine = CarlaBackend::CarlaEngine::newDriverByName("JACK");
    engine->setCallback(client.callback, &client);

    // bridge client <-> engine
    client.registerOscEngine(engine);

    // Init engine
    QString engName = QString("%1 (master)").arg(name ? name : label);
    engName.truncate(engine->maxClientNameSize());

    if (! engine->init(engName.toUtf8().constData()))
    {
        const char* const lastError = engine->getLastError();
        qWarning("Bridge engine failed to start, error was:\n%s", lastError);
        engine->close();
        delete engine;
        client.sendOscBridgeError(lastError);
        client.quit();
        return 2;
    }

    void* extraStuff = nullptr;

#if 1 // TESTING
    static const char* const dssiGUI = "/usr/lib/dssi/calf/calf_gtk";
    extraStuff = (void*)dssiGUI;
#endif

    // Init plugin
    short id = engine->addPlugin(itype, filename, name, label, extraStuff);
    int ret;

    if (id >= 0 && id < CarlaBackend::MAX_PLUGINS)
    {
        CarlaBackend::CarlaPlugin* const plugin = engine->getPlugin(id);
        client.setStuff(engine, plugin);

        // create window if needed
        bool guiResizable;
        CarlaBackend::GuiType guiType;
        plugin->getGuiInfo(&guiType, &guiResizable);

        if (guiType == CarlaBackend::GUI_INTERNAL_QT4 || guiType == CarlaBackend::GUI_INTERNAL_COCOA || guiType == CarlaBackend::GUI_INTERNAL_HWND || guiType == CarlaBackend::GUI_INTERNAL_X11)
            client.createWindow(guiResizable);
        else if (guiType == CarlaBackend::GUI_EXTERNAL_LV2 || guiType == CarlaBackend::GUI_EXTERNAL_OSC)
            client.enableUI();

        if (! useOsc)
            plugin->setActive(true, false, false);

        client.exec(nullptr, !useOsc);

        ret = 0;
    }
    else
    {
        const char* const lastError = engine->getLastError();
        qWarning("Plugin failed to load, error was:\n%s", lastError);

        if (useOsc)
            client.sendOscBridgeError(lastError);

        ret = 1;
    }

    engine->removeAllPlugins();
    engine->close();
    delete engine;

    if (useOsc)
    {
        // Close OSC
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

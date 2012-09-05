/*
 * Carla UI bridge code
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

#include "carla_bridge_client.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtCore/QTimerEvent>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>

CARLA_BRIDGE_START_NAMESPACE

static int qargc = 0;
static char* qargv[] = { nullptr };

class BridgeApplication : public QApplication
{
public:
    BridgeApplication()
        : QApplication(qargc, qargv, true)
    {
        msgTimer = 0;
        m_client = nullptr;
    }

    void exec(CarlaClient* const client)
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
            if (m_client && ! m_client->runMessages())
                killTimer(msgTimer);
        }

        QApplication::timerEvent(event);
    }

private:
    int msgTimer;
    CarlaClient* m_client;
};

// -------------------------------------------------------------------------

class CarlaToolkitQt4: public CarlaToolkit
{
public:
    CarlaToolkitQt4(const char* const title)
        : CarlaToolkit(title),
#if defined(BRIDGE_LV2_QT4)
          settings("Cadence", "Carla-Qt4UIs")
#elif defined(BRIDGE_LV2_X11) || defined(BRIDGE_VST_X11)
          settings("Cadence", "Carla-X11UIs")
#elif defined(BRIDGE_LV2_HWND) || defined(BRIDGE_VST_HWND)
          settings("Cadence", "Carla-HWNDUIs")
#else
          settings("Cadence", "Carla-UIs")
#endif
    {
        qDebug("CarlaToolkitQt4::CarlaToolkitQt4(%s)", title);

        app = nullptr;
        window = nullptr;
    }

    ~CarlaToolkitQt4()
    {
        qDebug("CarlaToolkitQt4::~CarlaToolkitQt4()");
        Q_ASSERT(! app);
    }

    void init()
    {
        qDebug("CarlaToolkitQt4::init()");
        Q_ASSERT(! app);

        app = new BridgeApplication;
    }

    void exec(CarlaClient* const client, const bool showGui)
    {
        qDebug("CarlaToolkitQt4::exec(%p)", client);
        Q_ASSERT(app);
        Q_ASSERT(client);

        m_client = client;

        if (client->needsReparent())
        {
            window = (QDialog*)client->getWidget();
            window->resize(10, 10);
        }
        else
        {
            // TODO - window->setCentralWidget(widget); or other simpler method
            window = new QDialog(nullptr);
            window->resize(10, 10);
            window->setLayout(new QVBoxLayout(window));

            QWidget* const widget = (QWidget*)client->getWidget();
            window->layout()->addWidget(widget);
            window->layout()->setContentsMargins(0, 0, 0, 0);
            window->adjustSize();
            widget->setParent(window);
            widget->show();
        }

        if (! client->isResizable())
            window->setFixedSize(window->width(), window->height());

        window->setWindowTitle(m_title);

        if (settings.contains(QString("%1/pos_x").arg(m_title)))
        {
            int posX = settings.value(QString("%1/pos_x").arg(m_title), window->x()).toInt();
            int posY = settings.value(QString("%1/pos_y").arg(m_title), window->y()).toInt();
            window->move(posX, posY);

            if (client->isResizable())
            {
                int width  = settings.value(QString("%1/width").arg(m_title), window->width()).toInt();
                int height = settings.value(QString("%1/height").arg(m_title), window->height()).toInt();
                window->resize(width, height);
            }
        }

        app->connect(window, SIGNAL(finished(int)), app, SLOT(quit()));

        if (showGui)
            show();
        else
            m_client->sendOscUpdate();

        // Main loop
        app->exec(client);
    }

    void quit()
    {
        qDebug("CarlaToolkitQt4::quit()");
        Q_ASSERT(app);

        if (window)
        {
            if (m_client)
            {
                settings.setValue(QString("%1/pos_x").arg(m_title), window->x());
                settings.setValue(QString("%1/pos_y").arg(m_title), window->y());
                settings.setValue(QString("%1/width").arg(m_title), window->width());
                settings.setValue(QString("%1/height").arg(m_title), window->height());
                settings.sync();
            }

            window->close();

            delete window;
            window = nullptr;
        }

        m_client = nullptr;

        if (app)
        {
            if (! app->closingDown())
                app->quit();

            delete app;
            app = nullptr;
        }
    }

    void show()
    {
        qDebug("CarlaToolkitQt4::show()");
        Q_ASSERT(window);

        if (window)
            window->show();
    }

    void hide()
    {
        qDebug("CarlaToolkitQt4::hide()");
        Q_ASSERT(window);

        if (window)
            window->hide();
    }

    void resize(int width, int height)
    {
        qDebug("CarlaToolkitQt4::resize(%i, %i)", width, height);
        Q_ASSERT(window);

        if (window)
            window->setFixedSize(width, height);
    }

private:
    BridgeApplication* app;
    QDialog* window;
    QSettings settings;
};

// -------------------------------------------------------------------------

CarlaToolkit* CarlaToolkit::createNew(const char* const title)
{
    return new CarlaToolkitQt4(title);
}

CARLA_BRIDGE_END_NAMESPACE

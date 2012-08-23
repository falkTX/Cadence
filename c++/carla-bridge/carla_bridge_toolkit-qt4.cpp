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

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>

//CARLA_BRIDGE_START_NAMESPACE
namespace CarlaBridge {

// -------------------------------------------------------------------------

class MessageChecker : public QTimer
{
public:
    MessageChecker(CarlaBridgeClient* const client_)
        : client(client_)
    {
        Q_ASSERT(client);
    }

    void timerEvent(QTimerEvent*)
    {
        if (! client->runMessages())
            stop();
    }

private:
    CarlaBridgeClient* const client;
};

class CarlaBridgeToolkitQt4: public CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkitQt4(const char* const title) :
        CarlaBridgeToolkit(title),
    #ifdef BRIDGE_LV2_X11
        settings("Cadence", "Carla-X11UIs")
  #else
        settings("Cadence", "Carla-Qt4UIs")
  #endif
    {
        qDebug("CarlaBridgeToolkitQt4::CarlaBridgeToolkitQt4(%s)", title);

        app = nullptr;
        window = nullptr;
    }

    ~CarlaBridgeToolkitQt4()
    {
        qDebug("CarlaBridgeToolkitQt4::~CarlaBridgeToolkitQt4()");
    }

    void init()
    {
        qDebug("CarlaBridgeToolkitQt4::init()");
        Q_ASSERT(! app);

        static int argc = 0;
        static char* argv[] = { nullptr };
        app = new QApplication(argc, argv, true);
    }

    void exec(CarlaBridgeClient* const client)
    {
        qDebug("CarlaBridgeToolkitQt4::exec(%p)", client);
        Q_ASSERT(app);
        Q_ASSERT(client);

        if (client->needsReparent())
        {
            window = (QDialog*)client->getWidget();
            window->resize(10, 10);
        }
        else
        {
            // TODO - window->setCentralWidget(widget); or other simpler method
            window = new QDialog();
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

        MessageChecker messageChecker(client);
        messageChecker.start(50);

        QObject::connect(window, SIGNAL(finished(int)), app, SLOT(quit()));

        m_client = client;
        m_client->sendOscUpdate();

#ifdef QTCREATOR_TEST
        show();
#endif

        // Main loop
        app->exec();
    }

    void quit()
    {
        qDebug("CarlaBridgeToolkitQt4::quit()");
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
        }
    }

    void show()
    {
        qDebug("CarlaBridgeToolkitQt4::show()");
        Q_ASSERT(window);

        if (window)
            window->show();
    }

    void hide()
    {
        qDebug("CarlaBridgeToolkitQt4::hide()");
        Q_ASSERT(window);

        if (window)
            window->hide();
    }

    void resize(int width, int height)
    {
        qDebug("CarlaBridgeToolkitQt4::resize(%i, %i)", width, height);
        Q_ASSERT(window);

        if (window)
            window->setFixedSize(width, height);
    }

private:
    QApplication* app;
    QDialog* window;
    QSettings settings;
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(const char* const title)
{
    return new CarlaBridgeToolkitQt4(title);
}

CARLA_BRIDGE_END_NAMESPACE

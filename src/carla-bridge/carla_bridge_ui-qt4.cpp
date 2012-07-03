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

#include "carla_bridge.h"
#include "carla_bridge_osc.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>

static QDialog* window = nullptr;
#ifdef BRIDGE_LV2_X11
static QSettings settings("Cadence", "Carla-X11UIs");
#else
static QSettings settings("Cadence", "Carla-Qt4UIs");
#endif

// -------------------------------------------------------------------------

// Static QApplication
static QApplication* app = nullptr;

class MessageChecker : public QTimer
{
public:
    MessageChecker() {}

    void timerEvent(QTimerEvent*)
    {
        if (client)
            client->run_messages();
    }
};

// -------------------------------------------------------------------------

void toolkit_init()
{
    static int argc = 0;
    static char* argv[] = { nullptr };
    app = new QApplication(argc, argv, true);
}

void toolkit_loop()
{
    if (client->needs_reparent())
    {
        window = (QDialog*)client->get_widget();
        window->resize(10, 10);
    }
    else
    {
        // TODO - window->setCentralWidget(widget); or other simpler method
        window = new QDialog();
        window->resize(10, 10);
        window->setLayout(new QVBoxLayout(window));

        QWidget* widget = (QWidget*)client->get_widget();
        window->layout()->addWidget(widget);
        window->layout()->setContentsMargins(0, 0, 0, 0);
        window->adjustSize();
        widget->setParent(window);
        widget->show();
    }

    MessageChecker checker;
    checker.start(50);
    QObject::connect(window, SIGNAL(finished(int)), app, SLOT(quit()));

    if (! client->is_resizable())
        window->setFixedSize(window->width(), window->height());

    window->setWindowTitle(client->get_title());

    if (settings.contains(QString("%1/pos_x").arg(client->get_title())))
    {
        int pos_x = settings.value(QString("%1/pos_x").arg(client->get_title()), window->x()).toInt();
        int pos_y = settings.value(QString("%1/pos_y").arg(client->get_title()), window->y()).toInt();
        window->move(pos_x, pos_y);

        if (client->is_resizable())
        {
            int width  = settings.value(QString("%1/width").arg(client->get_title()), window->width()).toInt();
            int height = settings.value(QString("%1/height").arg(client->get_title()), window->height()).toInt();
            window->resize(width, height);
        }
    }

    osc_send_update();

    // Main loop
    app->exec();
}

void toolkit_quit()
{
    if (window)
    {
        if (client)
        {
            settings.setValue(QString("%1/pos_x").arg(client->get_title()), window->x());
            settings.setValue(QString("%1/pos_y").arg(client->get_title()), window->y());
            settings.setValue(QString("%1/width").arg(client->get_title()), window->width());
            settings.setValue(QString("%1/height").arg(client->get_title()), window->height());
            settings.sync();
        }

        window->close();

        delete window;
        window = nullptr;
    }

    if (app)
    {
        if (! app->closingDown())
            app->quit();

        delete app;
        app = nullptr;
    }
}

void toolkit_window_show()
{
    if (window)
        window->show();
}

void toolkit_window_hide()
{
    if (window)
        window->hide();
}

void toolkit_window_resize(int width, int height)
{
    if (client && window)
    {
        if (client->is_resizable())
            window->resize(width, height);
        else
            window->setFixedSize(width, height);
    }
}

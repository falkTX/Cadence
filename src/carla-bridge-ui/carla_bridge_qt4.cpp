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

#include "carla_bridge_ui.h"
#include "carla_osc.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QVBoxLayout>

static QDialog* window;

// -------------------------------------------------------------------------

static QApplication* app = nullptr;

class MessageChecker : public QTimer
{
public:
    MessageChecker() {}

    void timerEvent(QTimerEvent*)
    {
        if (ui)
            ui->run_messages();
    }
};

// -------------------------------------------------------------------------

void toolkit_init()
{
    int argc = 0;
    char** argv = nullptr;
    //char* argv[] = { nullptr };
    //static QApplication* app = nullptr;
    app = new QApplication(argc, argv, true);
    //static QApplication* app = new QApplication(argc, argv, true);
}

void toolkit_loop(const char* ui_title, bool reparent)
{
    if (reparent)
    {
        window = new QDialog();
        window->setLayout(new QVBoxLayout(window));

        QWidget* widget = (QWidget*)ui->get_widget();
        window->layout()->addWidget(widget);
        widget->setParent(window);
        widget->show();
    }
    else
    {
        window = (QDialog*)ui->get_widget();
    }

    MessageChecker checker;
    checker.start(50);
    QObject::connect(window, SIGNAL(finished(int)), app, SLOT(quit()));

    QSettings settings("Cadence", "CarlaBridges");
    window->restoreGeometry(settings.value(ui_title).toByteArray());

    if (ui->is_resizable() == false)
        window->setFixedSize(window->width(), window->height());

    window->setWindowTitle(ui_title);

    osc_send_update();

    // Main loop
    app->exec();
}

void toolkit_quit()
{
    //QSettings settings("Cadence", "CarlaBridges");
    //settings.setValue(plugin_name, window->saveGeometry());
    //settings.sync();
    app->quit();
}

void toolkit_window_show()
{
    window->show();
}

void toolkit_window_hide()
{
    window->hide();
}

void toolkit_window_resize(int width, int height)
{
    if (ui->is_resizable())
        window->resize(width, height);
    else
        window->setFixedSize(width, height);
}

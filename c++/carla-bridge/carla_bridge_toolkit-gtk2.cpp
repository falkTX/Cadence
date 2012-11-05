/*
 * Carla UI bridge code
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#include "carla_bridge_client.hpp"

#ifdef BRIDGE_LV2_X11
# error X11 UI uses Qt4
#endif

#include <gtk/gtk.h>
#include <QtCore/QSettings>

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaToolkitGtk2 : public CarlaToolkit
{
public:
    CarlaToolkitGtk2(const char* const title)
        : CarlaToolkit(title),
          settings("Cadence", "Carla-Gtk2UIs")
    {
        qDebug("CarlaToolkitGtk2::CarlaToolkitGtk2(%s)", title);

        window = nullptr;

        lastX = lastY = 0;
        lastWidth = lastHeight = 0;
    }

    ~CarlaToolkitGtk2()
    {
        qDebug("CarlaToolkitGtk2::~CarlaToolkitGtk2()");
    }

    void init()
    {
        qDebug("CarlaToolkitGtk2::init()");

        static int argc = 0;
        static char** argv = { nullptr };
        gtk_init(&argc, &argv);
    }

    void exec(CarlaClient* const client, const bool showGui)
    {
        qDebug("CarlaToolkitGtk2::exec(%p)", client);
        CARLA_ASSERT(client);

        m_client = client;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_container_add(GTK_CONTAINER(window), (GtkWidget*)client->getWidget());

        gtk_window_set_resizable(GTK_WINDOW(window), client->isResizable());
        gtk_window_set_title(GTK_WINDOW(window), m_title);

        gtk_window_get_position(GTK_WINDOW(window), &lastX, &lastY);
        gtk_window_get_size(GTK_WINDOW(window), &lastWidth, &lastHeight);

        if (settings.contains(QString("%1/pos_x").arg(m_title)))
        {
            lastX = settings.value(QString("%1/pos_x").arg(m_title), lastX).toInt();
            lastY = settings.value(QString("%1/pos_y").arg(m_title), lastY).toInt();
            gtk_window_move(GTK_WINDOW(window), lastX, lastY);

            if (client->isResizable())
            {
                lastWidth  = settings.value(QString("%1/width").arg(m_title), lastWidth).toInt();
                lastHeight = settings.value(QString("%1/height").arg(m_title), lastHeight).toInt();
                gtk_window_resize(GTK_WINDOW(window), lastWidth, lastHeight);
            }
        }

        g_timeout_add(50, gtk_ui_timeout, this);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_ui_destroy), this);

        if (showGui)
            show();
        else
            m_client->sendOscUpdate();

        // Main loop
        gtk_main();
    }

    void quit()
    {
        qDebug("CarlaToolkitGtk2::quit()");

        if (window)
        {
            gtk_widget_destroy(window);
            gtk_main_quit();

            window = nullptr;
        }

        m_client = nullptr;
    }

    void show()
    {
        qDebug("CarlaToolkitGtk2::show()");
        CARLA_ASSERT(window);

        if (window)
            gtk_widget_show_all(window);
    }

    void hide()
    {
        qDebug("CarlaToolkitGtk2::hide()");
        CARLA_ASSERT(window);

        if (window)
            gtk_widget_hide_all(window);
    }

    void resize(int width, int height)
    {
        qDebug("CarlaToolkitGtk2::resize(%i, %i)", width, height);
        CARLA_ASSERT(window);

        if (window)
            gtk_window_resize(GTK_WINDOW(window), width, height);
    }

    // ---------------------------------------------------------------------

protected:
    void handleDestroy()
    {
        qDebug("CarlaToolkitGtk2::handleDestroy()");

        window = nullptr;
        m_client = nullptr;

        settings.setValue(QString("%1/pos_x").arg(m_title), lastX);
        settings.setValue(QString("%1/pos_y").arg(m_title), lastY);
        settings.setValue(QString("%1/width").arg(m_title), lastWidth);
        settings.setValue(QString("%1/height").arg(m_title), lastHeight);
        settings.sync();
    }

    gboolean handleTimeout()
    {
        if (window)
        {
            gtk_window_get_position(GTK_WINDOW(window), &lastX, &lastY);
            gtk_window_get_size(GTK_WINDOW(window), &lastWidth, &lastHeight);
        }

        return m_client ? m_client->oscIdle() : false;
    }

    // ---------------------------------------------------------------------

private:
    GtkWidget* window;
    QSettings settings;

    gint lastX, lastY, lastWidth, lastHeight;

    static void gtk_ui_destroy(GtkWidget*, gpointer data)
    {
        CarlaToolkitGtk2* const _this_ = (CarlaToolkitGtk2*)data;
        _this_->handleDestroy();

        gtk_main_quit();
    }

    static gboolean gtk_ui_timeout(gpointer data)
    {
        CarlaToolkitGtk2* const _this_ = (CarlaToolkitGtk2*)data;
        return _this_->handleTimeout();
    }
};

// -------------------------------------------------------------------------

CarlaToolkit* CarlaToolkit::createNew(const char* const title)
{
    return new CarlaToolkitGtk2(title);
}

CARLA_BRIDGE_END_NAMESPACE

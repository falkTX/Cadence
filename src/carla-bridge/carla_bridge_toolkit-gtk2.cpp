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

#ifdef BRIDGE_LV2_X11
#error X11 UI uses Qt4
#endif

#include <gtk/gtk.h>
#include <QtCore/QSettings>

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

class CarlaBridgeToolkitGtk2: public CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkitGtk2(const char* const title) :
        CarlaBridgeToolkit(title),
        settings("Cadence", "Carla-Gtk2UIs")
    {
        qDebug("CarlaBridgeToolkitGtk2::CarlaBridgeToolkitGtk2(%s)", title);

        window = nullptr;

        last_x = last_y = 0;
        last_width = last_height = 0;
    }

    ~CarlaBridgeToolkitGtk2()
    {
        qDebug("CarlaBridgeToolkitGtk2::~CarlaBridgeToolkitGtk2()");
    }

    void init()
    {
        qDebug("CarlaBridgeToolkitGtk2::init()");

        static int argc = 0;
        static char** argv = { nullptr };
        gtk_init(&argc, &argv);
    }

    void exec(CarlaBridgeClient* const client)
    {
        qDebug("CarlaBridgeToolkitGtk2::exec(%p)", client);
        assert(client);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_container_add(GTK_CONTAINER(window), (GtkWidget*)client->getWidget());

        gtk_window_set_resizable(GTK_WINDOW(window), client->isResizable());
        gtk_window_set_title(GTK_WINDOW(window), m_title);

        gtk_window_get_position(GTK_WINDOW(window), &last_x, &last_y);
        gtk_window_get_size(GTK_WINDOW(window), &last_width, &last_height);

        if (settings.contains(QString("%1/pos_x").arg(m_title)))
        {
            last_x = settings.value(QString("%1/pos_x").arg(m_title), last_x).toInt();
            last_y = settings.value(QString("%1/pos_y").arg(m_title), last_y).toInt();
            gtk_window_move(GTK_WINDOW(window), last_x, last_y);

            if (client->isResizable())
            {
                last_width  = settings.value(QString("%1/width").arg(m_title), last_width).toInt();
                last_height = settings.value(QString("%1/height").arg(m_title), last_height).toInt();
                gtk_window_resize(GTK_WINDOW(window), last_width, last_height);
            }
        }

        g_timeout_add(50, gtk_ui_timeout, this);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_ui_destroy), this);

        m_client = client;
        client->oscSendUpdate();

        // Main loop
        gtk_main();
    }

    void quit()
    {
        qDebug("CarlaBridgeToolkitGtk2::quit()");

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
        qDebug("CarlaBridgeToolkitGtk2::show()");
        assert(window);

        if (window)
            gtk_widget_show_all(window);
    }

    void hide()
    {
        qDebug("CarlaBridgeToolkitGtk2::hide()");
        assert(window);

        if (window)
            gtk_widget_hide_all(window);
    }

    void resize(int width, int height)
    {
        qDebug("CarlaBridgeToolkitGtk2::resize(%i, %i)", width, height);
        assert(window);

        if (window)
            gtk_window_resize(GTK_WINDOW(window), width, height);
    }

private:
    GtkWidget* window;
    QSettings settings;

    gint last_x, last_y, last_width, last_height;

    static void gtk_ui_destroy(GtkWidget*, gpointer data)
    {
        CarlaBridgeToolkitGtk2* const _this_ = (CarlaBridgeToolkitGtk2*)data;
        _this_->handleDestroy();

        gtk_main_quit();
    }

    static gboolean gtk_ui_timeout(gpointer data)
    {
        CarlaBridgeToolkitGtk2* const _this_ = (CarlaBridgeToolkitGtk2*)data;
        return _this_->handleTimeout();
    }

    // ---------------------------------------------------------------------

    void handleDestroy()
    {
        window = nullptr;
        m_client = nullptr;

        settings.setValue(QString("%1/pos_x").arg(m_title), last_x);
        settings.setValue(QString("%1/pos_y").arg(m_title), last_y);
        settings.setValue(QString("%1/width").arg(m_title), last_width);
        settings.setValue(QString("%1/height").arg(m_title), last_height);
        settings.sync();
    }

    gboolean handleTimeout()
    {
        if (window)
        {
            gtk_window_get_position(GTK_WINDOW(window), &last_x, &last_y);
            gtk_window_get_size(GTK_WINDOW(window), &last_width, &last_height);
        }

        return m_client ? m_client->runMessages() : false;
    }
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(const char* const title)
{
    return new CarlaBridgeToolkitGtk2(title);
}

CARLA_BRIDGE_END_NAMESPACE

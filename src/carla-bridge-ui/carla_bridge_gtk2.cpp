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
#include "../carla-bridge/carla_osc.h"

#include <gtk/gtk.h>

static GtkWidget* window;

// -------------------------------------------------------------------------

gboolean gtk_ui_recheck(void*)
{
    return ui->run_messages();
}

void gtk_ui_destroy(GtkWidget*, void*)
{
    window = nullptr;
    gtk_main_quit();
}

// -------------------------------------------------------------------------

void toolkit_init()
{
    int argc = 0;
    char** argv = nullptr;
    gtk_init(&argc, &argv);
}

void toolkit_loop(const char* plugin_name, bool reparent)
{
    if (reparent)
    {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_container_add(GTK_CONTAINER(window), (GtkWidget*)ui->get_widget());
    }
    else
    {
        window = (GtkWidget*)ui->get_widget();
    }

    g_timeout_add(50, gtk_ui_recheck, nullptr);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_ui_destroy), nullptr);

    gtk_window_set_resizable(GTK_WINDOW(window), ui->is_resizable());
    gtk_window_set_title(GTK_WINDOW(window), plugin_name);

    osc_send_update();

    // Main loop
    gtk_main();
}

void toolkit_quit()
{
    if (window)
    {
        gtk_widget_destroy(window);
        gtk_main_quit();
    }
}

void toolkit_window_show()
{
    gtk_widget_show_all(window);
}

void toolkit_window_hide()
{
    gtk_widget_hide_all(window);
}

void toolkit_window_resize(int width, int height)
{
    gtk_window_resize(GTK_WINDOW(window), width, height);
}

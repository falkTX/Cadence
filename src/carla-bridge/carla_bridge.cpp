/*
 * Carla Plugin bridge code
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
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

#include "carla_backend.h"
#include "carla_osc.h"
#include "carla_plugin.h"

#include <QtGui/QApplication>

// Global variables
CallbackFunc Callback = nullptr;
const char* last_error = nullptr;

QMutex carla_proc_lock_var;
QMutex carla_midi_lock_var;

// Global variables (shared)
const char* unique_names[MAX_PLUGINS]  = { nullptr };
CarlaPlugin* CarlaPlugins[MAX_PLUGINS] = { nullptr };

volatile double ains_peak[MAX_PLUGINS*2]  = { 0.0 };
volatile double aouts_peak[MAX_PLUGINS*2] = { 0.0 };

// Global OSC stuff
lo_server_thread global_osc_server_thread = nullptr;
const char* global_osc_server_path = nullptr;
OscData global_osc_data = { nullptr, nullptr, nullptr };

// Global JACK stuff
jack_client_t* carla_jack_client = nullptr;
jack_nframes_t carla_buffer_size = 512;
jack_nframes_t carla_sample_rate = 44100;

// Global options
carla_options_t carla_options = {
  /* initiated */          false,
  /* global_jack_client */ false,
  /* use_dssi_chunks    */ false,
  /* prefer_ui_bridges  */ true
};

// plugin specific
short add_plugin_ladspa(const char* filename, const char* label, void* extra_stuff);

// TODO - make these shared:

short get_new_plugin_id()
{
    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        if (CarlaPlugins[i] == nullptr)
            return i;
    }

    return -1;
}

const char* get_unique_name(const char* name)
{
    int max = jack_port_name_size()/2 - 5;
    //if (carla_options.global_jack_client)
    //    max -= strlen(carla_client_name);

    qDebug("get_unique_name(%s) - truncated to %i", name, max);

    QString qname(name);

    if (qname.isEmpty())
        qname = "(No name)";

    qname.truncate(max);
    //qname.replace(":", "."); // ":" is used in JACK to split client/port names

    for (unsigned short i=0; i<MAX_PLUGINS; i++)
    {
        // Check if unique name already exists
        if (unique_names[i] && qname == unique_names[i])
        {
            // Check if string has already been modified
            uint len = qname.size();

            if (qname.at(len-3) == QChar('(') && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                int number = qname.at(len-2).toAscii()-'0';

                if (number == 9)
                    // next number is 10, 2 digits
                    qname.replace(" (9)", " (10)");
                else
                    qname[len-2] = QChar('0'+number+1);

                continue;
            }
            else if (qname.at(len-4) == QChar('(') && qname.at(len-3).isDigit() && qname.at(len-2).isDigit() && qname.at(len-1) == QChar(')'))
            {
                QChar n2 = qname.at(len-2); // (1x)
                QChar n3 = qname.at(len-3); // (x0)

                if (n2 == QChar('9'))
                {
                    n2 = QChar('0');
                    n3 = QChar(n3.toAscii()+1);
                }
                else
                    n2 = QChar(n2.toAscii()+1);

                qname[len-2] = n2;
                qname[len-3] = n3;

                continue;
            }

            // Modify string if not
            qname += " (2)";
        }
    }

    return strdup(qname.toUtf8().constData());
}

void set_last_error(const char* error)
{
    if (last_error)
        free((void*)last_error);

    last_error = strdup(error);
}

void carla_proc_lock()
{
    carla_proc_lock_var.lock();
}

void carla_proc_unlock()
{
    carla_proc_lock_var.unlock();
}

void carla_midi_lock()
{
    carla_midi_lock_var.lock();
}

void carla_midi_unlock()
{
    carla_midi_lock_var.unlock();
}

void callback_action(CallbackType action, unsigned short plugin_id, int value1, int value2, double value3)
{
    if (Callback)
        Callback(action, plugin_id, value1, value2, value3);
}

uint32_t get_buffer_size()
{
    qDebug("get_buffer_size()");
    return carla_buffer_size;
}

double get_sample_rate()
{
    qDebug("get_sample_rate()");
    return carla_sample_rate;
}

int main(int argc, char* argv[])
{
    if (argc != 6)
    {
       qWarning("%s :: bad arguments", argv[0]);
       return 1;
    }

    const char* stype    = argv[1];
    const char* filename = argv[2];
    const char* label    = argv[3];
    const char* pname    = argv[4];
    const char* osc_url  = argv[5];

    short id;
    PluginType itype;

    if (strcmp(stype, "LADSPA") == 0)
        itype = PLUGIN_LADSPA;
    else if (strcmp(stype, "DSSI") == 0)
        itype = PLUGIN_DSSI;
    else if (strcmp(stype, "VST") == 0)
        itype = PLUGIN_VST;
    else
    {
        itype = PLUGIN_NONE;
        qWarning("Invalid plugin type '%s'", stype);
        return 1;
    }

    osc_init(pname, osc_url);
    set_last_error("no error");

    QApplication app(argc, argv);

    switch (itype)
    {
    case PLUGIN_LADSPA:
        id = add_plugin_ladspa(filename, label, nullptr);
        break;
    default:
        break;
    }

     if (id >= 0)
     {
        app.exec();
     }
     else
     {
       qWarning("Plugin failed to load, error was:\n%s", last_error);
       return 1;
     }

    //remove_plugin(id);
    osc_close();

    return 0;
}

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

#include "carla_osc.h"

#ifdef BUILD_BRIDGE_UI
#include "carla_bridge_ui.h"
#else
#include "carla_plugin.h"
extern void plugin_bridge_show_gui(bool yesno);
extern void plugin_bridge_quit();
#endif

#include <cstring>

size_t plugin_name_len = 0;
const char* plugin_name = nullptr;

const char* global_osc_server_path = nullptr;
lo_server_thread global_osc_server_thread = nullptr;
OscData global_osc_data = { nullptr, nullptr, nullptr };

// -------------------------------------------------------------------------

void osc_init(const char* osc_name, const char* osc_url)
{
    qDebug("osc_init(%s, %s)", osc_name, osc_url);
    plugin_name = osc_name;
    plugin_name_len = strlen(plugin_name);

    const char* host = lo_url_get_hostname(osc_url);
    const char* port = lo_url_get_port(osc_url);

    global_osc_data.path   = lo_url_get_path(osc_url);
    global_osc_data.target = lo_address_new(host, port);

    free((void*)host);
    free((void*)port);

    // create new OSC thread
    global_osc_server_thread = lo_server_thread_new(nullptr, osc_error_handler);

    // get our full OSC server path
    char* osc_thread_path = lo_server_thread_get_url(global_osc_server_thread);

    char osc_path_tmp[strlen(osc_thread_path) + plugin_name_len + 1];
    strcpy(osc_path_tmp, osc_thread_path);
    strcat(osc_path_tmp, plugin_name);
    free(osc_thread_path);

    global_osc_server_path = strdup(osc_path_tmp);

    // register message handler and start OSC thread
    lo_server_thread_add_method(global_osc_server_thread, nullptr, nullptr, osc_message_handler, nullptr);
    lo_server_thread_start(global_osc_server_thread);

    // debug our server path just to make sure everything is ok
    qDebug("carla-bridge OSC -> %s\n", global_osc_server_path);
}

void osc_close()
{
    qDebug("osc_close()");

    osc_clear_data(&global_osc_data);

    lo_server_thread_stop(global_osc_server_thread);
    lo_server_thread_del_method(global_osc_server_thread, nullptr, nullptr);
    lo_server_thread_free(global_osc_server_thread);

    free((void*)global_osc_server_path);
    global_osc_server_path = nullptr;
}

void osc_clear_data(OscData* osc_data)
{
    qDebug("osc_clear_data(%p)", osc_data);

    if (global_osc_data.path)
        free((void*)global_osc_data.path);

    if (global_osc_data.target)
        lo_address_free(global_osc_data.target);

    global_osc_data.path = nullptr;
    global_osc_data.target = nullptr;
}

// -------------------------------------------------------------------------

void osc_error_handler(int num, const char* msg, const char* path)
{
    qCritical("osc_error_handler(%i, %s, %s)", num, msg, path);
}

int osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
    qDebug("osc_message_handler(%s, %s, %p, %i, %p, %p)", path, types, argv, argc, data, user_data);

    char method[32];
    memset(method, 0, sizeof(char)*24);

    unsigned int mindex = strlen(plugin_name)+2;

    for (unsigned int i=mindex; i < strlen(path) && i-mindex < 24; i++)
        method[i-mindex] = path[i];

    if (strcmp(method, "control") == 0)
        return osc_handle_control(argv);
    else if (strcmp(method, "program") == 0)
        return osc_handle_program(argv);
    else if (strcmp(method, "midi_program") == 0)
        return osc_handle_midi_program(argv);
//    else if (strcmp(method, "note_on") == 0)
//        return osc_note_on_handler(argv);
//    else if (strcmp(method, "note_off") == 0)
//        return osc_note_off_handler(argv);
    else if (strcmp(method, "show") == 0)
        return osc_handle_show();
    else if (strcmp(method, "hide") == 0)
        return osc_handle_hide();
    else if (strcmp(method, "quit") == 0)
        return osc_handle_quit();
#if 0
    else if (strcmp(method, "set_parameter_midi_channel") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
    else if (strcmp(method, "set_parameter_midi_cc") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
#endif
    else
        qWarning("Got unsupported OSC method '%s' on '%s'", method, path);

    return 1;
}

// -------------------------------------------------------------------------

int osc_handle_control(lo_arg** argv)
{
    int index    = argv[0]->i;
    double value = argv[1]->f;

#ifdef BUILD_BRIDGE_UI
    if (ui)
        ui->queque_message(BRIDGE_MESSAGE_PARAMETER, index, 0, value);
#else
    if (CarlaPlugins[0])
        CarlaPlugins[0]->set_parameter_value_rindex(index, value, false, true, true);
#endif

    return 0;
}

int osc_handle_program(lo_arg** argv)
{
    int index = argv[0]->i;

#ifdef BUILD_BRIDGE_UI
    if (ui && index >= 0)
        ui->queque_message(BRIDGE_MESSAGE_PROGRAM, index, 0, 0.0);
#else
    if (CarlaPlugins[0])
        CarlaPlugins[0]->set_program(index, false, true, true, true);
#endif

    return 0;
}

int osc_handle_midi_program(lo_arg** argv)
{
    int bank    = argv[0]->i;
    int program = argv[1]->i;

#ifdef BUILD_BRIDGE_UI
    //if (ui && index >= 0)
    //    ui->queque_message(BRIDGE_MESSAGE_MIDI_PROGRAM, index, 0, 0.0);
    (void)bank;
    (void)program;
#else
    if (CarlaPlugins[0])
        CarlaPlugins[0]->set_midi_program_full(bank, program, false, true, true, true);
#endif

    return 0;
}

int osc_handle_show()
{
#ifdef BUILD_BRIDGE_UI
    if (ui)
        ui->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 1, 0, 0.0);
#else
    plugin_bridge_show_gui(true);
#endif

    return 0;
}

int osc_handle_hide()
{
#ifdef BUILD_BRIDGE_UI
    if (ui)
        ui->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 0, 0, 0.0);
#else
    plugin_bridge_show_gui(false);
#endif

    return 0;
}

int osc_handle_quit()
{
#ifdef BUILD_BRIDGE_UI
    if (ui)
        ui->queque_message(BRIDGE_MESSAGE_QUIT, 0, 0, 0.0);
#else
    plugin_bridge_quit();
#endif

    return 0;
}

// -------------------------------------------------------------------------

void osc_send_update()
{
    qDebug("osc_send_update()");
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+8];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/update");
        lo_send(global_osc_data.target, target_path, "s", global_osc_server_path);
    }
}

void osc_send_exiting()
{
    qDebug("osc_send_exiting()");
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/exiting");
        lo_send(global_osc_data.target, target_path, "");
    }
}

void osc_send_bridge_ains_peak(int index, double value)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+18];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_ains_peak");
        lo_send(global_osc_data.target, target_path, "if", index, value);
    }
}

void osc_send_bridge_aouts_peak(int index, double value)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+19];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_aouts_peak");
        lo_send(global_osc_data.target, target_path, "if", index, value);
    }
}

void osc_send_bridge_audio_count(int ins, int outs, int total)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+20];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_audio_count");
        lo_send(global_osc_data.target, target_path, "iii", ins, outs, total);
    }
}

void osc_send_bridge_midi_count(int ins, int outs, int total)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+19];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_midi_count");
        lo_send(global_osc_data.target, target_path, "iii", ins, outs, total);
    }
}

void osc_send_bridge_param_count(int ins, int outs, int total)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+20];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_param_count");
        lo_send(global_osc_data.target, target_path, "iii", ins, outs, total);
    }
}

void osc_send_bridge_plugin_info(int type, int category, int hints, const char* name, const char* label, const char* maker, const char* copyright, long unique_id)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+20];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_plugin_info");
        lo_send(global_osc_data.target, target_path, "iiissssh", type, category, hints, name, label, maker, copyright, unique_id);
    }
}

void osc_send_bridge_param_info(int index, const char* name, const char* unit)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+19];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_param_info");
        lo_send(global_osc_data.target, target_path, "iss", index, name, unit);
    }
}

void osc_send_bridge_param_data(int type, int index, int rindex, int hints, int midi_channel, int midi_cc)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+19];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_param_data");
        lo_send(global_osc_data.target, target_path, "iiiiii", type, index, rindex, hints, midi_channel, midi_cc);
    }
}

void osc_send_bridge_param_ranges(int index, double def, double min, double max, double step, double step_small, double step_large)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+21];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_param_ranges");
        lo_send(global_osc_data.target, target_path, "iffffff", index, def, min, max, step, step_small, step_large);
    }
}

void osc_send_bridge_update()
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+14];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_update");
        lo_send(global_osc_data.target, target_path, "");
    }
}

// -------------------------------------------------------------------------

void osc_send_configure(OscData*, const char* key, const char* value)
{
    qDebug("osc_send_configure(%s, %s)", key, value);
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+11];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/configure");
        lo_send(global_osc_data.target, target_path, "ss", key, value);
    }
}

void osc_send_control(OscData*, int param, double value)
{
    qDebug("osc_send_control(%i, %f)", param, value);
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/control");
        lo_send(global_osc_data.target, target_path, "if", param, value);
    }
}

void osc_send_program(OscData*, int program_id)
{
    qDebug("osc_send_program(%i)", program_id);
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/program");
        lo_send(global_osc_data.target, target_path, "i", program_id);
    }
}

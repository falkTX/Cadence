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

#include "carla_bridge.h"
#include "carla_bridge_osc.h"
#include "carla_midi.h"

#include <QtCore/QString>

#ifdef BUILD_BRIDGE_PLUGIN
static const size_t client_name_len  = 13;
static const char* const client_name = "plugin-bridge";
#else
static const size_t client_name_len  = 13;
static const char* const client_name = "lv2-ui-bridge";
#endif

const char* global_osc_server_path = nullptr;
lo_server_thread global_osc_server_thread = nullptr;
OscData global_osc_data = { nullptr, nullptr, nullptr };

// -------------------------------------------------------------------------

void osc_init(const char* osc_url)
{
    qDebug("osc_init(%s)", osc_url);

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
    global_osc_server_path = strdup(QString("%1%2").arg(osc_thread_path).arg(client_name).toUtf8().constData());
    free(osc_thread_path);

    // register message handler and start OSC thread
    lo_server_thread_add_method(global_osc_server_thread, nullptr, nullptr, osc_message_handler, nullptr);
    lo_server_thread_start(global_osc_server_thread);
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

    if (osc_data->path)
        free((void*)osc_data->path);

    if (osc_data->source)
        lo_address_free(osc_data->source);

    if (osc_data->target)
        lo_address_free(osc_data->target);

    osc_data->path = nullptr;
    osc_data->source = nullptr;
    osc_data->target = nullptr;
}

// -------------------------------------------------------------------------

void osc_error_handler(int num, const char* msg, const char* path)
{
    qCritical("osc_error_handler(%i, %s, %s)", num, msg, path);
}

int osc_message_handler(const char* path, const char* types, lo_arg** argv, int argc, void* data, void* user_data)
{
#if DEBUG
    qDebug("osc_message_handler(%s, %s, %p, %i, %p, %p)", path, types, argv, argc, data, user_data);
#endif

    // Check if message is for this client
    if (strlen(path) <= client_name_len || strncmp(path+1, client_name, client_name_len) != 0)
    {
        qWarning("osc_message_handler() - message not for this client -> '%s' != '/%s/'", path, client_name);
        return 1;
    }

    char method[32] = { 0 };
    memcpy(method, path + client_name_len + 1, 32);

    if (strcmp(method, "/configure") == 0)
        return osc_handle_configure(argv);
    else if (strcmp(method, "/control") == 0)
        return osc_handle_control(argv);
    else if (strcmp(method, "/program") == 0)
        return osc_handle_program(argv);
    else if (strcmp(method, "/midi_program") == 0)
        return osc_handle_midi_program(argv);
    else if (strcmp(method, "/midi") == 0)
        return osc_handle_midi(argv);
    else if (strcmp(method, "/show") == 0)
        return osc_handle_show();
    else if (strcmp(method, "/hide") == 0)
        return osc_handle_hide();
    else if (strcmp(method, "/quit") == 0)
        return osc_handle_quit();
#if BRIDGE_LV2_GTK2 || BRIDGE_LV2_QT4 || BRIDGE_LV2_X11
    //else if (strcmp(method, "/lv2_atom_transfer") == 0)
    //    return osc_handle_lv2_atom_transfer(argv);
    else if (strcmp(method, "/lv2_event_transfer") == 0)
        return osc_handle_lv2_event_transfer(argv);
#endif
#if 0
    else if (strcmp(method, "set_parameter_midi_channel") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
    else if (strcmp(method, "set_parameter_midi_cc") == 0)
        return osc_set_parameter_midi_channel_handler(argv);
#endif
    else
        qWarning("Got unsupported OSC method '%s' on '%s'", method, path);

    return 1;

    Q_UNUSED(types);
    Q_UNUSED(argc);
    Q_UNUSED(data);
    Q_UNUSED(user_data);
}

// -------------------------------------------------------------------------

int osc_handle_configure(lo_arg** argv)
{
#ifdef BUILD_BRIDGE_PLUGIN
    const char* key   = (const char*)&argv[0]->s;
    const char* value = (const char*)&argv[1]->s;

    if (client && strcmp(key, "CarlaBridgeSaveNow") == 0)
        client->save_now(value);
#else
    Q_UNUSED(argv);
#endif

    return 0;
}

int osc_handle_control(lo_arg** argv)
{
    int rindex  = argv[0]->i;
    float value = argv[1]->f;

    if (client)
        client->queque_message(BRIDGE_MESSAGE_PARAMETER, rindex, 0, value);

    return 0;
}

int osc_handle_program(lo_arg** argv)
{
    int index = argv[0]->i;

    if (client)
        client->queque_message(BRIDGE_MESSAGE_PROGRAM, index, 0, 0.0);

    return 0;
}

int osc_handle_midi_program(lo_arg** argv)
{
    int bank    = argv[0]->i;
    int program = argv[1]->i;

    if (client)
        client->queque_message(BRIDGE_MESSAGE_MIDI_PROGRAM, bank, program, 0.0);

    return 0;
}

int osc_handle_midi(lo_arg** argv)
{
    uint8_t* data  = argv[0]->m;
    uint8_t status = data[1];

    // Fix bad note-off
    if (MIDI_IS_STATUS_NOTE_ON(status) && data[3] == 0)
        status -= 0x10;

    if (MIDI_IS_STATUS_NOTE_OFF(status))
    {
        uint8_t note = data[2];

        if (client)
            client->queque_message(BRIDGE_MESSAGE_NOTE_OFF, note, 0, 0.0);
    }
    else if (MIDI_IS_STATUS_NOTE_ON(status))
    {
        uint8_t note = data[2];
        uint8_t velo = data[3];

        if (client)
            client->queque_message(BRIDGE_MESSAGE_NOTE_ON, note, velo, 0.0);
    }

    return 0;
}

int osc_handle_show()
{
    if (client)
        client->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 1, 0, 0.0);

    return 0;
}

int osc_handle_hide()
{
    if (client)
        client->queque_message(BRIDGE_MESSAGE_SHOW_GUI, 0, 0, 0.0);

    return 0;
}

int osc_handle_quit()
{
    if (client)
        client->queque_message(BRIDGE_MESSAGE_QUIT, 0, 0, 0.0);

    return 0;
}

// -------------------------------------------------------------------------

void osc_send_configure(const char* key, const char* value)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+11];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/configure");
        lo_send(global_osc_data.target, target_path, "ss", key, value);
    }
}

void osc_send_control(int param, double value)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/control");
        lo_send(global_osc_data.target, target_path, "if", param, value);
    }
}

void osc_send_program(int program)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/program");
        lo_send(global_osc_data.target, target_path, "i", program);
    }
}

void osc_send_midi_program(int bank, int program, bool)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+14];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/midi_program");
        lo_send(global_osc_data.target, target_path, "ii", bank, program);
    }
}

void osc_send_midi(uint8_t buf[4])
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+6];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/midi");
        lo_send(global_osc_data.target, target_path, "m", buf);
    }
}

// -------------------------------------------------------------------------

void osc_send_update()
{
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
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+9];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/exiting");
        lo_send(global_osc_data.target, target_path, "");
    }
}

#ifdef BUILD_BRIDGE_PLUGIN
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

void osc_send_bridge_program_count(int count)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+22];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_program_count");
        lo_send(global_osc_data.target, target_path, "i", count);
    }
}

void osc_send_bridge_midi_program_count(int count)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+27];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_midi_program_count");
        lo_send(global_osc_data.target, target_path, "i", count);
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

void osc_send_bridge_param_data(int index, int type, int rindex, int hints, int midi_channel, int midi_cc)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+19];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_param_data");
        lo_send(global_osc_data.target, target_path, "iiiiii", index, type, rindex, hints, midi_channel, midi_cc);
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

void osc_send_bridge_program_info(int index, const char* name)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+21];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_program_info");
        lo_send(global_osc_data.target, target_path, "is", index, name);
    }
}

void osc_send_bridge_midi_program_info(int index, int bank, int program, const char* label)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+26];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_midi_program_info");
        lo_send(global_osc_data.target, target_path, "iiis", index, bank, program, label);
    }
}

void osc_send_bridge_update()
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+15];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/bridge_update");
        lo_send(global_osc_data.target, target_path, "");
    }
}
#else
void osc_send_lv2_event_transfer(const char* type, const char* key, const char* value)
{
    if (global_osc_data.target)
    {
        char target_path[strlen(global_osc_data.path)+20];
        strcpy(target_path, global_osc_data.path);
        strcat(target_path, "/lv2_event_transfer");
        lo_send(global_osc_data.target, target_path, "sss", type, key, value);
    }
}
#endif

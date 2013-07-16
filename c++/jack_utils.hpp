/*
 * Common JACK code
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef __JACK_UTILS_HPP__
#define __JACK_UTILS_HPP__

#include "jackbridge/JackBridge.cpp"

#include <cstring>
#include <string>
#include <vector>

static inline
std::vector<char*> jackbridge_port_get_all_connections_as_vector(jack_client_t* const client, jack_port_t* const port)
{
    std::vector<char*> connectionsVector;
    const char** connections = jackbridge_port_get_all_connections(client, port);

    if (connections)
    {
        for (int i=0; connections[i]; i++)
            connectionsVector.push_back(strdup(connections[i]));

        jackbridge_free(connections);
    }

    return connectionsVector;
}

static inline
std::string jackbridge_status_get_error_string(const jack_status_t& status)
{
    std::string errorString;

    if (status & JackFailure)
        errorString += "Overall operation failed;\n";
    if (status & JackInvalidOption)
        errorString += "The operation contained an invalid or unsupported option;\n";
    if (status & JackNameNotUnique)
        errorString += "The desired client name was not unique;\n";
    if (status & JackServerStarted)
        errorString += "The JACK server was started as a result of this operation;\n";
    if (status & JackServerFailed)
        errorString += "Unable to connect to the JACK server;\n";
    if (status & JackServerError)
        errorString += "Communication error with the JACK server;\n";
    if (status & JackNoSuchClient)
        errorString += "Requested client does not exist;\n";
    if (status & JackLoadFailure)
        errorString += "Unable to load internal client;\n";
    if (status & JackInitFailure)
        errorString += "Unable to initialize client;\n";
    if (status & JackShmFailure)
        errorString += "Unable to access shared memory;\n";
    if (status & JackVersionError)
        errorString += "Client's protocol version does not match;\n";
    if (status & JackBackendError)
        errorString += "Backend Error;\n";
    if (status & JackClientZombie)
        errorString += "Client is being shutdown against its will;\n";

    if (errorString.size() > 2)
        errorString.replace(errorString.size()-2, 2, ".");

    return errorString;
}

#endif // __JACK_UTILS_HPP__

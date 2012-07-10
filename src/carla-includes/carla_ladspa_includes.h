/*
 * Carla common LADSPA code
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

#ifndef CARLA_LADSPA_INCLUDES_H
#define CARLA_LADSPA_INCLUDES_H

#include "ladspa/ladspa.h"
#include "ladspa_rdf.h"

// ------------------------------------------------------------------------------------------------

// Copy RDF object
static inline
const LADSPA_RDF_Descriptor* ladspa_rdf_dup(const LADSPA_RDF_Descriptor* const rdf_descriptor)
{
    LADSPA_RDF_Descriptor* const new_descriptor = new LADSPA_RDF_Descriptor;

    new_descriptor->Type = rdf_descriptor->Type;
    new_descriptor->UniqueID = rdf_descriptor->UniqueID;
    new_descriptor->PortCount = rdf_descriptor->PortCount;

    if (rdf_descriptor->Title)
        new_descriptor->Title = strdup(rdf_descriptor->Title);

    if (rdf_descriptor->Creator)
        new_descriptor->Creator = strdup(rdf_descriptor->Creator);

    if (new_descriptor->PortCount > 0)
    {
        new_descriptor->Ports = new LADSPA_RDF_Port[new_descriptor->PortCount];

        for (unsigned long i=0; i < new_descriptor->PortCount; i++)
        {
            LADSPA_RDF_Port* const Port = &new_descriptor->Ports[i];
            Port->Type    = rdf_descriptor->Ports[i].Type;
            Port->Hints   = rdf_descriptor->Ports[i].Hints;
            Port->Default = rdf_descriptor->Ports[i].Default;
            Port->Unit    = rdf_descriptor->Ports[i].Unit;
            Port->ScalePointCount = rdf_descriptor->Ports[i].ScalePointCount;

            if (rdf_descriptor->Ports[i].Label)
                Port->Label = strdup(rdf_descriptor->Ports[i].Label);

            if (Port->ScalePointCount > 0)
            {
                Port->ScalePoints = new LADSPA_RDF_ScalePoint[Port->ScalePointCount];

                for (unsigned long j=0; j < Port->ScalePointCount; j++)
                {
                    LADSPA_RDF_ScalePoint* const ScalePoint = &Port->ScalePoints[j];
                    ScalePoint->Value = rdf_descriptor->Ports[i].ScalePoints[j].Value;

                    if (rdf_descriptor->Ports[i].ScalePoints[j].Label)
                        ScalePoint->Label = strdup(rdf_descriptor->Ports[i].ScalePoints[j].Label);
                }
            }
        }
    }

    return new_descriptor;
}

// Delete object
static inline
void ladspa_rdf_free(const LADSPA_RDF_Descriptor* const rdf_descriptor)
{
    if (rdf_descriptor->Title)
        free((void*)rdf_descriptor->Title);

    if (rdf_descriptor->Creator)
        free((void*)rdf_descriptor->Creator);

    if (rdf_descriptor->PortCount > 0)
    {
        for (unsigned long i=0; i < rdf_descriptor->PortCount; i++)
        {
            const LADSPA_RDF_Port* const Port = &rdf_descriptor->Ports[i];

            if (Port->Label)
                free((void*)Port->Label);

            if (Port->ScalePointCount > 0)
            {
                for (unsigned long j=0; j < Port->ScalePointCount; j++)
                {
                    const LADSPA_RDF_ScalePoint* const ScalePoint = &Port->ScalePoints[j];

                    if (ScalePoint->Label)
                        free((void*)ScalePoint->Label);
                }
                delete[] Port->ScalePoints;
            }
        }
        delete[] rdf_descriptor->Ports;
    }

    delete rdf_descriptor;
}

// ------------------------------------------------------------------------------------------------

static inline
bool is_rdf_port_good(int Type1, int Type2)
{
    if (LADSPA_IS_PORT_INPUT(Type1) && ! LADSPA_IS_PORT_INPUT(Type2))
        return false;
    if (LADSPA_IS_PORT_OUTPUT(Type1) && ! LADSPA_IS_PORT_OUTPUT(Type2))
        return false;
    if (LADSPA_IS_PORT_CONTROL(Type1) && ! LADSPA_IS_PORT_CONTROL(Type2))
        return false;
    if (LADSPA_IS_PORT_AUDIO(Type1) && ! LADSPA_IS_PORT_AUDIO(Type2))
        return false;
    return true;
}

static inline
bool is_ladspa_rdf_descriptor_valid(const LADSPA_RDF_Descriptor* const rdf_descriptor, const LADSPA_Descriptor* const descriptor)
{
    if (! rdf_descriptor)
        return false;

    if (rdf_descriptor->UniqueID != descriptor->UniqueID)
    {
        qWarning("WARNING - Plugin has wrong UniqueID: %li != %li", rdf_descriptor->UniqueID, descriptor->UniqueID);
        return false;
    }

    if (rdf_descriptor->PortCount > descriptor->PortCount)
    {
        qWarning("WARNING - Plugin has RDF data, but invalid PortCount: %li > %li", rdf_descriptor->PortCount, descriptor->PortCount);
        return false;
    }

    for (unsigned long i=0; i < rdf_descriptor->PortCount; i++)
    {
        if (! is_rdf_port_good(rdf_descriptor->Ports[i].Type, descriptor->PortDescriptors[i]))
        {
            qWarning("WARNING - Plugin has RDF data, but invalid PortTypes: %i != %i", rdf_descriptor->Ports[i].Type, descriptor->PortDescriptors[i]);
            return false;
        }
    }

    return true;
}

#endif // CARLA_LADSPA_INCLUDES_H

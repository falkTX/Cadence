/*
 * Custom types to store LADSPA RDF information
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

#ifndef LADSPA_RDF_INCLUDED
#define LADSPA_RDF_INCLUDED

// Base Types
typedef float LADSPA_Data;
typedef int LADSPA_Property;
typedef unsigned long long LADSPA_PluginType;

// Unit Types
#define LADSPA_UNIT_DB                    0x01
#define LADSPA_UNIT_COEF                  0x02
#define LADSPA_UNIT_HZ                    0x04
#define LADSPA_UNIT_S                     0x08
#define LADSPA_UNIT_MS                    0x10
#define LADSPA_UNIT_MIN                   0x20

#define LADSPA_UNIT_CLASS_AMPLITUDE       (LADSPA_UNIT_DB|LADSPA_UNIT_COEF)
#define LADSPA_UNIT_CLASS_FREQUENCY       (LADSPA_UNIT_HZ)
#define LADSPA_UNIT_CLASS_TIME            (LADSPA_UNIT_S|LADSPA_UNIT_MS|LADSPA_UNIT_MIN)

#define LADSPA_IS_UNIT_DB(x)              ((x) == LADSPA_UNIT_DB)
#define LADSPA_IS_UNIT_COEF(x)            ((x) == LADSPA_UNIT_COEF)
#define LADSPA_IS_UNIT_HZ(x)              ((x) == LADSPA_UNIT_HZ)
#define LADSPA_IS_UNIT_S(x)               ((x) == LADSPA_UNIT_S)
#define LADSPA_IS_UNIT_MS(x)              ((x) == LADSPA_UNIT_MS)
#define LADSPA_IS_UNIT_MIN(x)             ((x) == LADSPA_UNIT_MIN)

#define LADSPA_IS_UNIT_CLASS_AMPLITUDE(x) ((x) & LADSPA_UNIT_CLASS_AMPLITUDE)
#define LADSPA_IS_UNIT_CLASS_FREQUENCY(x) ((x) & LADSPA_UNIT_CLASS_FREQUENCY)
#define LADSPA_IS_UNIT_CLASS_TIME(x)      ((x) & LADSPA_UNIT_CLASS_TIME)

// Port Types (Official API)
#define LADSPA_PORT_INPUT                 0x1
#define LADSPA_PORT_OUTPUT                0x2
#define LADSPA_PORT_CONTROL               0x4
#define LADSPA_PORT_AUDIO                 0x8

// Port Hints
#define LADSPA_PORT_UNIT                  0x1
#define LADSPA_PORT_DEFAULT               0x2
#define LADSPA_PORT_LABEL                 0x4

#define LADSPA_PORT_HAS_UNIT(x)           ((x) & LADSPA_PORT_UNIT)
#define LADSPA_PORT_HAS_DEFAULT(x)        ((x) & LADSPA_PORT_DEFAULT)
#define LADSPA_PORT_HAS_LABEL(x)          ((x) & LADSPA_PORT_LABEL)

// Plugin Types
#define LADSPA_CLASS_UTILITY              0x000000001
#define LADSPA_CLASS_GENERATOR            0x000000002
#define LADSPA_CLASS_SIMULATOR            0x000000004
#define LADSPA_CLASS_OSCILLATOR           0x000000008
#define LADSPA_CLASS_TIME                 0x000000010
#define LADSPA_CLASS_DELAY                0x000000020
#define LADSPA_CLASS_PHASER               0x000000040
#define LADSPA_CLASS_FLANGER              0x000000080
#define LADSPA_CLASS_CHORUS               0x000000100
#define LADSPA_CLASS_REVERB               0x000000200
#define LADSPA_CLASS_FREQUENCY            0x000000400
#define LADSPA_CLASS_FREQUENCY_METER      0x000000800
#define LADSPA_CLASS_FILTER               0x000001000
#define LADSPA_CLASS_LOWPASS              0x000002000
#define LADSPA_CLASS_HIGHPASS             0x000004000
#define LADSPA_CLASS_BANDPASS             0x000008000
#define LADSPA_CLASS_COMB                 0x000010000
#define LADSPA_CLASS_ALLPASS              0x000020000
#define LADSPA_CLASS_EQ                   0x000040000
#define LADSPA_CLASS_PARAEQ               0x000080000
#define LADSPA_CLASS_MULTIEQ              0x000100000
#define LADSPA_CLASS_AMPLITUDE            0x000200000
#define LADSPA_CLASS_PITCH                0x000400000
#define LADSPA_CLASS_AMPLIFIER            0x000800000
#define LADSPA_CLASS_WAVESHAPER           0x001000000
#define LADSPA_CLASS_MODULATOR            0x002000000
#define LADSPA_CLASS_DISTORTION           0x004000000
#define LADSPA_CLASS_DYNAMICS             0x008000000
#define LADSPA_CLASS_COMPRESSOR           0x010000000
#define LADSPA_CLASS_EXPANDER             0x020000000
#define LADSPA_CLASS_LIMITER              0x040000000
#define LADSPA_CLASS_GATE                 0x080000000
#define LADSPA_CLASS_SPECTRAL             0x100000000LL
#define LADSPA_CLASS_NOTCH                0x200000000LL

#define LADSPA_GROUP_DYNAMICS             (LADSPA_CLASS_DYNAMICS|LADSPA_CLASS_COMPRESSOR|LADSPA_CLASS_EXPANDER|LADSPA_CLASS_LIMITER|LADSPA_CLASS_GATE)
#define LADSPA_GROUP_AMPLITUDE            (LADSPA_CLASS_AMPLITUDE|LADSPA_CLASS_AMPLIFIER|LADSPA_CLASS_WAVESHAPER|LADSPA_CLASS_MODULATOR|LADSPA_CLASS_DISTORTION|LADSPA_GROUP_DYNAMICS)
#define LADSPA_GROUP_EQ                   (LADSPA_CLASS_EQ|LADSPA_CLASS_PARAEQ|LADSPA_CLASS_MULTIEQ)
#define LADSPA_GROUP_FILTER               (LADSPA_CLASS_FILTER|LADSPA_CLASS_LOWPASS|LADSPA_CLASS_HIGHPASS|LADSPA_CLASS_BANDPASS|LADSPA_CLASS_COMB|LADSPA_CLASS_ALLPASS|LADSPA_CLASS_NOTCH)
#define LADSPA_GROUP_FREQUENCY            (LADSPA_CLASS_FREQUENCY|LADSPA_CLASS_FREQUENCY_METER|LADSPA_GROUP_FILTER|LADSPA_GROUP_EQ|LADSPA_CLASS_PITCH)
#define LADSPA_GROUP_SIMULATOR            (LADSPA_CLASS_SIMULATOR|LADSPA_CLASS_REVERB)
#define LADSPA_GROUP_TIME                 (LADSPA_CLASS_TIME|LADSPA_CLASS_DELAY|LADSPA_CLASS_PHASER|LADSPA_CLASS_FLANGER|LADSPA_CLASS_CHORUS|LADSPA_CLASS_REVERB)
#define LADSPA_GROUP_GENERATOR            (LADSPA_CLASS_GENERATOR|LADSPA_CLASS_OSCILLATOR)

#define LADSPA_IS_PLUGIN_DYNAMICS(x)      ((x) & LADSPA_GROUP_DYNAMICS)
#define LADSPA_IS_PLUGIN_AMPLITUDE(x)     ((x) & LADSPA_GROUP_AMPLITUDE)
#define LADSPA_IS_PLUGIN_EQ(x)            ((x) & LADSPA_GROUP_EQ)
#define LADSPA_IS_PLUGIN_FILTER(x)        ((x) & LADSPA_GROUP_FILTER)
#define LADSPA_IS_PLUGIN_FREQUENCY(x)     ((x) & LADSPA_GROUP_FREQUENCY)
#define LADSPA_IS_PLUGIN_SIMULATOR(x)     ((x) & LADSPA_GROUP_SIMULATOR)
#define LADSPA_IS_PLUGIN_TIME(x)          ((x) & LADSPA_GROUP_TIME)
#define LADSPA_IS_PLUGIN_GENERATOR(x)     ((x) & LADSPA_GROUP_GENERATOR)

// A Scale Point
struct LADSPA_RDF_ScalePoint {
    LADSPA_Data Value;
    const char* Label;
};

// A Port
struct LADSPA_RDF_Port {
    LADSPA_Property Type;
    LADSPA_Property Hints;
    const char* Label;
    LADSPA_Data Default;
    LADSPA_Property Unit;

    unsigned long ScalePointCount;
    LADSPA_RDF_ScalePoint* ScalePoints;
};

// The actual plugin descriptor
struct LADSPA_RDF_Descriptor {
    LADSPA_PluginType Type;
    unsigned long UniqueID;
    const char* Title;
    const char* Creator;

    unsigned long PortCount;
    LADSPA_RDF_Port* Ports;
};


// Copy RDF object
inline const LADSPA_RDF_Descriptor* ladspa_rdf_dup(const LADSPA_RDF_Descriptor* rdf_descriptor)
{
    unsigned long i, j;
    LADSPA_RDF_Descriptor* new_descriptor = new LADSPA_RDF_Descriptor;

    new_descriptor->Type = rdf_descriptor->Type;
    new_descriptor->UniqueID = rdf_descriptor->UniqueID;
    new_descriptor->PortCount = rdf_descriptor->PortCount;

    new_descriptor->Title = strdup(rdf_descriptor->Title);
    new_descriptor->Creator = strdup(rdf_descriptor->Creator);

    if (new_descriptor->PortCount > 0)
    {
        new_descriptor->Ports = new LADSPA_RDF_Port[new_descriptor->PortCount];

        for (i=0; i < new_descriptor->PortCount; i++)
        {
            LADSPA_RDF_Port* Port = &new_descriptor->Ports[i];
            Port->Type = rdf_descriptor->Ports[i].Type;
            Port->Hints = rdf_descriptor->Ports[i].Hints;
            Port->Default = rdf_descriptor->Ports[i].Default;
            Port->Unit = rdf_descriptor->Ports[i].Unit;
            Port->ScalePointCount = rdf_descriptor->Ports[i].ScalePointCount;

            Port->Label = strdup(rdf_descriptor->Ports[i].Label);

            if (Port->ScalePointCount > 0)
            {
                Port->ScalePoints = new LADSPA_RDF_ScalePoint[Port->ScalePointCount];

                for (j=0; j < Port->ScalePointCount; j++)
                {
                    Port->ScalePoints[j].Value = rdf_descriptor->Ports[i].ScalePoints[j].Value;
                    Port->ScalePoints[j].Label = strdup(rdf_descriptor->Ports[i].ScalePoints[j].Label);
                }
            }
            else
                Port->ScalePoints = nullptr;
        }
    }
    else
        new_descriptor->Ports = nullptr;

    return new_descriptor;
}

// Delete copied object
inline void ladspa_rdf_free(const LADSPA_RDF_Descriptor* rdf_descriptor)
{
    unsigned long i, j;

    free((void*)rdf_descriptor->Title);
    free((void*)rdf_descriptor->Creator);

    if (rdf_descriptor->PortCount > 0)
    {
        for (i=0; i < rdf_descriptor->PortCount; i++)
        {
            LADSPA_RDF_Port* Port = &rdf_descriptor->Ports[i];
            free((void*)Port->Label);

            if (Port->ScalePointCount > 0)
            {
                for (j=0; j < Port->ScalePointCount; j++)
                    free((void*)Port->ScalePoints[j].Label);

                delete[] Port->ScalePoints;
            }
        }
        delete[] rdf_descriptor->Ports;
    }
    delete rdf_descriptor;
}

#endif // LADSPA_RDF_INCLUDED

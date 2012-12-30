/*
 * Carla (frontend)
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

#ifndef SHARED_HPP
#define SHARED_HPP

#include "carla_backend.hpp"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtXml/QDomNode>

CARLA_BACKEND_USE_NAMESPACE

#define VERSION "0.5.0"

// ------------------------------------------------------------------------------------------------
// Carla Host object

struct CarlaHostObject {
    void* host;
    void* gui;
    bool  isControl;
    ProcessMode  processMode;
    unsigned int maxParameters;

    CarlaHostObject()
        : host(nullptr),
          gui(nullptr),
          isControl(false),
          processMode(PROCESS_MODE_CONTINUOUS_RACK),
          maxParameters(MAX_PARAMETERS) {}
};

extern CarlaHostObject Carla;

// ------------------------------------------------------------------------------------------------
// Carla GUI stuff

#define ICON_STATE_NULL 0
#define ICON_STATE_WAIT 1
#define ICON_STATE_OFF  2
#define ICON_STATE_ON   3

#define PALETTE_COLOR_NONE   0
#define PALETTE_COLOR_WHITE  1
#define PALETTE_COLOR_RED    2
#define PALETTE_COLOR_GREEN  3
#define PALETTE_COLOR_BLUE   4
#define PALETTE_COLOR_YELLOW 5
#define PALETTE_COLOR_ORANGE 6
#define PALETTE_COLOR_BROWN  7
#define PALETTE_COLOR_PINK   8

struct CarlaStateParameter {
    uint32_t index;
    QString name;
    QString symbol;
    double  value;
    uint8_t midiChannel;
    int16_t midiCC;

    CarlaStateParameter()
        : index(0),
          value(0.0),
          midiChannel(1),
          midiCC(-1) {}
};

struct CarlaStateCustomData {
    QString type;
    QString key;
    QString value;

    CarlaStateCustomData() {}
};

struct CarlaSaveState {
    QString type;
    QString name;
    QString label;
    QString binary;
    long uniqueID;
    bool active;
    double dryWet;
    double volume;
    double balanceLeft;
    double balanceRight;
    QVector<CarlaStateParameter> parameters;
    int32_t currentProgramIndex;
    QString currentProgramName;
    int32_t currentMidiBank;
    int32_t currentMidiProgram;
    QVector<CarlaStateCustomData> customData;
    QString chunk;

    CarlaSaveState()
        : uniqueID(0),
          active(false),
          dryWet(1.0),
          volume(1.0),
          balanceLeft(-1.0),
          balanceRight(1.0),
          currentProgramIndex(-1),
          currentMidiBank(-1),
          currentMidiProgram(-1) {}

    void reset()
    {
        type.clear();
        name.clear();
        label.clear();
        binary.clear();
        uniqueID = 0;
        active = false;
        dryWet = 1.0;
        volume = 1.0;
        balanceLeft = -1.0;
        balanceRight = 1.0;
        parameters.clear();
        currentProgramIndex = -1;
        currentProgramName.clear();
        currentMidiBank = -1;
        currentMidiProgram = -1;
        customData.clear();
        chunk.clear();
    }
};

// ------------------------------------------------------------------------------------------------------------
// Carla XML helpers

const CarlaSaveState* getSaveStateDictFromXML(const QDomNode& xmlNode);

QString xmlSafeString(QString string, const bool toXml);

// ------------------------------------------------------------------------------------------------------------
// Plugin Query (helper functions)

// needs free() afterwars if valid
char* findDSSIGUI(const char* const filename, const char* const name, const char* const label);

// ------------------------------------------------------------------------------------------------------------

#endif // SHARED_HPP

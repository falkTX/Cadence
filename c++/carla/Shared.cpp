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

#include "Shared.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

//#define __STDC_LIMIT_MACROS
//#include <cstdint>

CARLA_BACKEND_USE_NAMESPACE

// ------------------------------------------------------------------------------------------------
// Carla Host object

CarlaHostObject Carla;

// ------------------------------------------------------------------------------------------------------------
// Carla XML helpers

const CarlaSaveState* getSaveStateDictFromXML(const QDomNode& xmlNode)
{
    static CarlaSaveState saveState;
    saveState.reset();

    QDomNode node(xmlNode.firstChild());

    while (! node.isNull())
    {
        // ------------------------------------------------------
        // Info

        if (node.toElement().tagName() == "Info")
        {
            QDomNode xmlInfo(node.toElement().firstChild());

            while (! xmlInfo.isNull())
            {
                const QString tag  = xmlInfo.toElement().tagName();
                const QString text = xmlInfo.toElement().text(); //.strip();

                if (tag == "Type")
                    saveState.type = text;
                else if (tag == "Name")
                    saveState.name = xmlSafeString(text, false);
                else if (tag == "Label" || tag == "URI")
                    saveState.label = xmlSafeString(text, false);
                else if (tag == "Binary")
                    saveState.binary = xmlSafeString(text, false);
                else if (tag == "UniqueID")
                {
                    bool ok;
                    long uniqueID = text.toLong(&ok);
                    if (ok) saveState.uniqueID = uniqueID;
                }

                xmlInfo = xmlInfo.nextSibling();
            }
        }

        // ------------------------------------------------------
        // Data

        else if (node.toElement().tagName() == "Data")
        {
            QDomNode xmlData(node.toElement().firstChild());

            while (! xmlData.isNull())
            {
                const QString tag  = xmlData.toElement().tagName();
                const QString text = xmlData.toElement().text(); //.strip();

                // ----------------------------------------------
                // Internal Data

                if (tag == "Active")
                {
                    saveState.active = bool(text == "Yes");
                }
                else if (tag == "DryWet")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.dryWet = value;
                }
                else if (tag == "Volume")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.volume = value;
                }
                else if (tag == "Balance-Left")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.balanceLeft = value;
                }
                else if (tag == "Balance-Right")
                {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) saveState.balanceRight = value;
                }

                // ----------------------------------------------
                // Program (current)

                else if (tag == "CurrentProgramIndex")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentProgramIndex = value;
                }
                else if (tag == "CurrentProgramName")
                {
                    saveState.currentProgramName = xmlSafeString(text, false);
                }

                // ----------------------------------------------
                // Midi Program (current)

                else if (tag == "CurrentMidiBank")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiBank = value;
                }
                else if (tag == "CurrentMidiProgram")
                {
                    bool ok;
                    int value = text.toInt(&ok);
                    if (ok) saveState.currentMidiProgram = value;
                }

                // ----------------------------------------------
                // Parameters

                else if (tag == "Parameter")
                {
                    CarlaStateParameter stateParameter;

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString pTag  = xmlSubData.toElement().tagName();
                        const QString pText = xmlSubData.toElement().text(); //.strip();

                        if (pTag == "index")
                        {
                            bool ok;
                            uint index = pText.toUInt(&ok);
                            if (ok) stateParameter.index = index;
                        }
                        else if (pTag == "name")
                        {
                            stateParameter.name = xmlSafeString(pText, false);
                        }
                        else if (pTag == "symbol")
                        {
                            stateParameter.symbol = xmlSafeString(pText, false);
                        }
                        else if (pTag == "value")
                        {
                            bool ok;
                            double value = pText.toDouble(&ok);
                            if (ok) stateParameter.value = value;
                        }
                        else if (pTag == "midiChannel")
                        {
                            bool ok;
                            uint channel = pText.toUInt(&ok);
                            if (ok && channel < 16)
                                stateParameter.midiChannel = channel;
                        }
                        else if (pTag == "midiCC")
                        {
                            bool ok;
                            int cc = pText.toInt(&ok);
                            if (ok && cc < INT16_MAX)
                                stateParameter.midiCC = cc;
                        }

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.parameters.append(stateParameter);
                }

                // ----------------------------------------------
                // Custom Data

                else if (tag == "CustomData")
                {
                    CarlaStateCustomData stateCustomData;

                    QDomNode xmlSubData(xmlData.toElement().firstChild());

                    while (! xmlSubData.isNull())
                    {
                        const QString cTag  = xmlSubData.toElement().tagName();
                        const QString cText = xmlSubData.toElement().text(); //.strip();

                        if (cTag == "type")
                            stateCustomData.type = xmlSafeString(cText, false);
                        else if (cTag == "key")
                            stateCustomData.key = xmlSafeString(cText, false);
                        else if (cTag == "value")
                            stateCustomData.value = xmlSafeString(cText, false);

                        xmlSubData = xmlSubData.nextSibling();
                    }

                    saveState.customData.append(stateCustomData);
                }

                // ----------------------------------------------
                // Chunk

                else if (tag == "Chunk")
                {
                    saveState.chunk = xmlSafeString(text, false);
                }

                // ----------------------------------------------

                xmlData = xmlData.nextSibling();
            }
        }

        // ------------------------------------------------------

        node = node.nextSibling();
    }

    return &saveState;
}

QString xmlSafeString(QString string, const bool toXml)
{
    if (toXml)
        return string.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;");
    else
        return string.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"");
}

// ------------------------------------------------------------------------------------------------------------
// Plugin Query (helper functions)

char* findDSSIGUI(const char* const filename, const char* const name, const char* const label)
{
    static QString guiFilename;
    guiFilename.clear();

    QString pluginDir(filename);
    pluginDir.resize(pluginDir.lastIndexOf("."));

    QString shortName = QFileInfo(pluginDir).baseName();

    QString checkName  = QString(name).replace(" ", "_");
    QString checkLabel = QString(label);
    QString checkSName = shortName;

    if (! checkName.endsWith("_"))  checkName  += "_";
    if (! checkLabel.endsWith("_")) checkLabel += "_";
    if (! checkSName.endsWith("_")) checkSName += "_";

    QStringList guiFiles = QDir(pluginDir).entryList();

    foreach (const QString& gui, guiFiles)
    {
        if (gui.startsWith(checkName) || gui.startsWith(checkLabel) || gui.startsWith(checkSName))
        {
            QFileInfo finalname(pluginDir + QDir::separator() + gui);
            guiFilename = finalname.absoluteFilePath();
            break;
        }
    }

    if (guiFilename.isEmpty())
        return nullptr;

    return strdup(guiFilename.toUtf8().constData());
}

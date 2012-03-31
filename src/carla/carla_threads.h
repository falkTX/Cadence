/*
 * JACK Backend code for Carla
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

#ifndef CARLA_THREADS_H
#define CARLA_THREADS_H

#include <QtCore/QThread>

class QProcess;
class CarlaPlugin;

// --------------------------------------------------------------------------------------------------------
// CarlaCheckThread

class CarlaCheckThread : public QThread
{
public:
    CarlaCheckThread(QObject* parent = 0);
    void run();
};

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

class CarlaPluginThread : public QThread
{
public:
    enum PluginThreadMode {
        PLUGIN_THREAD_DSSI_GUI,
        PLUGIN_THREAD_LV2_GUI,
        PLUGIN_THREAD_BRIDGE
    };

    CarlaPluginThread(CarlaPlugin* plugin, PluginThreadMode mode);
    ~CarlaPluginThread();

    void setOscData(const char* binary, const char* label);

protected:
    virtual void run();

private:
    CarlaPlugin* m_plugin;
    PluginThreadMode m_mode;

    QString m_binary;
    QString m_label;

    QProcess* m_process;
};

#endif // CARLA_THREADS_H

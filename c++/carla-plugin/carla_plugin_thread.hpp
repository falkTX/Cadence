/*
 * Carla Plugin
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

#ifndef CARLA_PLUGIN_THREAD_HPP
#define CARLA_PLUGIN_THREAD_HPP

#include "carla_backend_utils.hpp"

#include <QtCore/QThread>

class QProcess;

CARLA_BACKEND_START_NAMESPACE

class CarlaPluginThread : public QThread
{
public:
    enum PluginThreadMode {
        PLUGIN_THREAD_DSSI_GUI,
        PLUGIN_THREAD_LV2_GUI,
        PLUGIN_THREAD_VST_GUI,
        PLUGIN_THREAD_BRIDGE
    };

    CarlaPluginThread(CarlaEngine* const engine, CarlaPlugin* const plugin, const PluginThreadMode mode, QObject* const parent = nullptr);
    ~CarlaPluginThread();

    void setOscData(const char* const binary, const char* const label, const char* const data1="");

protected:
    void run();

private:
    CarlaEngine* const engine;
    CarlaPlugin* const plugin;
    const PluginThreadMode mode;

    QString m_binary;
    QString m_label;
    QString m_data1;

    QProcess* m_process;
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_THREAD_HPP

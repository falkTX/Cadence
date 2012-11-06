/*
 * Carla Backend
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#include "carla_backend.hpp"

#include <QtCore/QMutex>
#include <QtCore/QThread>

CARLA_BACKEND_START_NAMESPACE
class CarlaEngine;
class CarlaPlugin;
CARLA_BACKEND_END_NAMESPACE

// --------------------------------------------------------------------------------------------------------
// CarlaCheckThread

class CarlaCheckThread : public QThread
{
public:
    CarlaCheckThread(CarlaBackend::CarlaEngine* const engine, QObject* const parent = nullptr);
    ~CarlaCheckThread();

    void startNow();
    void stopNow();

protected:
    void run();

private:
    CarlaBackend::CarlaEngine* const engine;
    QMutex mutex;
    bool m_stopNow;

    // ----------------------------------------------

    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaCheckThread* const thread)
            : m_thread(thread)
        {
            m_thread->mutex.lock();
        }

        ~ScopedLocker()
        {
            m_thread->mutex.unlock();
        }

    private:
        CarlaCheckThread* const m_thread;
    };
};

// --------------------------------------------------------------------------------------------------------
// CarlaPluginThread

#ifndef BUILD_BRIDGE

class QProcess;

class CarlaPluginThread : public QThread
{
public:
    enum PluginThreadMode {
        PLUGIN_THREAD_DSSI_GUI,
        PLUGIN_THREAD_LV2_GUI,
        PLUGIN_THREAD_VST_GUI,
        PLUGIN_THREAD_BRIDGE
    };

    CarlaPluginThread(CarlaBackend::CarlaEngine* const engine, CarlaBackend::CarlaPlugin* const plugin, const PluginThreadMode mode, QObject* const parent = nullptr);
    ~CarlaPluginThread();

    void setOscData(const char* const binary, const char* const label, const char* const data1="");

protected:
    void run();

private:
    CarlaBackend::CarlaEngine* const engine;
    CarlaBackend::CarlaPlugin* const plugin;
    const PluginThreadMode mode;

    QString m_binary;
    QString m_label;
    QString m_data1;

    QProcess* m_process;
};

#endif

#endif // CARLA_THREADS_H

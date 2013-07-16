/*
 * Simple JACK Audio Meter
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

#include <QtCore/Qt>

#ifndef Q_COMPILER_LAMBDA
# define nullptr (0)
#endif

#define VERSION "0.8.1"

#include "../jack_utils.hpp"
#include "../widgets/digitalpeakmeter.hpp"

#include <cmath>
#include <QtGui/QApplication>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>

// -------------------------------

volatile double x_portValue1 = 0.0;
volatile double x_portValue2 = 0.0;
volatile bool x_needReconnect = false;
volatile bool x_quitNow = false;

jack_client_t* jClient = nullptr;
jack_port_t* jPort1 = nullptr;
jack_port_t* jPort2 = nullptr;

bool gIsOutput = true;

// -------------------------------
// JACK callbacks

int process_callback(const jack_nframes_t nframes, void*)
{
    float* const jOut1 = (float*)jackbridge_port_get_buffer(jPort1, nframes);
    float* const jOut2 = (float*)jackbridge_port_get_buffer(jPort2, nframes);

    for (jack_nframes_t i = 0; i < nframes; i++)
    {
        if (std::abs(jOut1[i]) > x_portValue1)
            x_portValue1 = std::abs(jOut1[i]);

        if (std::abs(jOut2[i]) > x_portValue2)
            x_portValue2 = std::abs(jOut2[i]);
    }

    return 0;
}

void port_callback(jack_port_id_t, jack_port_id_t, int, void*)
{
    x_needReconnect = true;
}

#ifdef HAVE_JACKSESSION
void session_callback(jack_session_event_t* const event, void* const arg)
{
#ifdef Q_OS_LINUX
    QString filepath("cadence-jackmeter");
    Q_UNUSED(arg);
#else
    QString filepath((char*)arg);
#endif

    event->command_line = strdup(filepath.toUtf8().constData());

    jackbridge_session_reply(jClient, event);

    if (event->type == JackSessionSaveAndQuit)
        x_quitNow = true;

    jackbridge_session_event_free(event);
}
#endif

// -------------------------------
// helpers

void reconnect_ports()
{
    x_needReconnect = false;

    jack_port_t* const jPlayPort1 = jackbridge_port_by_name(jClient, gIsOutput ? "system:playback_1" : "system:capture_1");
    jack_port_t* const jPlayPort2 = jackbridge_port_by_name(jClient, gIsOutput ? "system:playback_2" : "system:capture_2");
    std::vector<char*> jPortList1(jackbridge_port_get_all_connections_as_vector(jClient, jPlayPort1));
    std::vector<char*> jPortList2(jackbridge_port_get_all_connections_as_vector(jClient, jPlayPort2));

    foreach (char* const& thisPortName, jPortList1)
    {
        jack_port_t* const thisPort = jackbridge_port_by_name(jClient, thisPortName);

        if (! (jackbridge_port_is_mine(jClient, thisPort) || jackbridge_port_connected_to(jPort1, thisPortName)))
        {
            if (gIsOutput)
                jackbridge_connect(jClient, thisPortName, "M:in1");
            else
                jackbridge_connect(jClient, "Mi:in1", thisPortName);
        }

        free(thisPortName);
    }

    foreach (char* const& thisPortName, jPortList2)
    {
        jack_port_t* const thisPort = jackbridge_port_by_name(jClient, thisPortName);

        if (! (jackbridge_port_is_mine(jClient, thisPort) || jackbridge_port_connected_to(jPort2, thisPortName)))
        {
            if (gIsOutput)
                jackbridge_connect(jClient, thisPortName, "M:in2");
            else
                jackbridge_connect(jClient, "Mi:in2", thisPortName);
        }

        free(thisPortName);
    }

    jPortList1.clear();
    jPortList2.clear();
}

// -------------------------------
// Meter class

class MeterW : public DigitalPeakMeter
{
public:
    MeterW() : DigitalPeakMeter(nullptr)
    {
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
        setWindowTitle(gIsOutput ? "M" : "Mi");

        if (gIsOutput)
            setColor(Color::GREEN);
        else
            setColor(Color::BLUE);

        setChannels(2);
        setOrientation(VERTICAL);
        setSmoothRelease(1);

        displayMeter(1, 0.0f);
        displayMeter(2, 0.0f);

        int refresh = float(jackbridge_get_buffer_size(jClient)) / jackbridge_get_sample_rate(jClient) * 1000;

        m_peakTimerId = startTimer(refresh > 50 ? refresh : 50);
    }

protected:
    void timerEvent(QTimerEvent* event)
    {
        if (x_quitNow)
        {
            close();
            x_quitNow = false;
            return;
        }

        if (event->timerId() == m_peakTimerId)
        {
            displayMeter(1, x_portValue1);
            displayMeter(2, x_portValue2);
            x_portValue1 = 0.0;
            x_portValue2 = 0.0;

            if (x_needReconnect)
                reconnect_ports();
        }

        QWidget::timerEvent(event);
    }

private:
    int m_peakTimerId;
};

// -------------------------------

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("JackMeter");
    app.setApplicationVersion(VERSION);
    app.setOrganizationName("Cadence");
    app.setWindowIcon(QIcon(":/scalable/cadence.svg"));

    if (app.arguments().contains("-in"))
        gIsOutput = false;

    // JACK initialization
    jack_status_t jStatus;
#ifdef HAVE_JACKSESSION
    jack_options_t jOptions = static_cast<jack_options_t>(JackNoStartServer|JackUseExactName|JackSessionID);
#else
    jack_options_t jOptions = static_cast<jack_options_t>(JackNoStartServer|JackUseExactName);
#endif
    jClient = jackbridge_client_open(gIsOutput ? "M" : "Mi", jOptions, &jStatus);

    if (! jClient)
    {
        std::string errorString(jackbridge_status_get_error_string(jStatus));
        QMessageBox::critical(nullptr, app.translate("MeterW", "Error"), app.translate("MeterW",
                                                                                       "Could not connect to JACK, possible reasons:\n"
                                                                                       "%1").arg(QString::fromStdString(errorString)));
        return 1;
    }

    jPort1 = jackbridge_port_register(jClient, "in1", JACK_DEFAULT_AUDIO_TYPE, gIsOutput ? JackPortIsInput : JackPortIsOutput, 0);
    jPort2 = jackbridge_port_register(jClient, "in2", JACK_DEFAULT_AUDIO_TYPE, gIsOutput ? JackPortIsInput : JackPortIsOutput, 0);

    jackbridge_set_process_callback(jClient, process_callback, nullptr);
    jackbridge_set_port_connect_callback(jClient, port_callback, nullptr);
#ifdef HAVE_JACKSESSION
    jackbridge_set_session_callback(jClient, session_callback, argv[0]);
#endif
    jackbridge_activate(jClient);

    reconnect_ports();

    // Show GUI
    MeterW gui;
    gui.resize(70, 600);
    gui.show();
    gui.setAttribute(Qt::WA_QuitOnClose);

    // App-Loop
    int ret = app.exec();

    jackbridge_deactivate(jClient);
    jackbridge_client_close(jClient);

    return ret;
}

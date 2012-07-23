/*
 * PatchCanvas test app
 */

#ifndef CANVASTESTAPP_H
#define CANVASTESTAPP_H

#include <QMainWindow>

#include <jack/jack.h>

#include "patchcanvas.h"

namespace Ui {
    class CanvasTestApp;
}

class QSettings;

class CanvasTestApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit CanvasTestApp(QWidget *parent = 0);
    ~CanvasTestApp();

    static void client_register_callback(const char* name, int register_, void *arg);
    static void port_register_callback(jack_port_id_t port_id_jack, int register_, void *arg);
    static void port_connect_callback(jack_port_id_t port_a, jack_port_id_t port_b, int connect, void* arg);

signals:
    void clientRegisterCallback(QString name, bool yesno);
    void portRegisterCallback(int port, bool yesno);
    void connectionCallback(int port_a, int port_b, bool yesno);

private slots:
    void handle_clientRegisterCallback(QString name, bool yesno);
    void handle_portRegisterCallback(int port, bool yesno);
    void handle_connectionCallback(int port_a, int port_b, bool yesno);

private:
    Ui::CanvasTestApp* ui;
    PatchScene* scene;
    QSettings* settings;

    virtual void closeEvent(QCloseEvent* event);
};

#endif // CANVASTESTAPP_H

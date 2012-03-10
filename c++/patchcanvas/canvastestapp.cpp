#include "canvastestapp.h"
#include "ui_canvastestapp.h"

#include <QMessageBox>
#include <QSettings>
#include <QVariant>
#include <QGLWidget>

struct group_name_to_id_t {
    int id;
    QString name;
};

struct port_name_to_id_t {
    int group_id;
    int port_id;
    QString name;
};

struct connection_to_id_t {
    int id;
    int port_out;
    int port_in;
};

static int last_group_id = 0;
static int last_port_id = 0;
static int last_connection_id = 0;

static CanvasTestApp* main_gui = 0;
static jack_client_t* jack_client = 0;

static QList<group_name_to_id_t> used_group_names;
static QList<port_name_to_id_t>  used_port_names;
static QList<connection_to_id_t> used_connections;

int get_group_id(QString group_name)
{
    for (int i=0; i < used_group_names.count(); i++)
    {
        if (used_group_names[i].name == group_name)
        {
            return used_group_names[i].id;
        }
    }
    return -1;
}

int get_port_id(QString full_port_name)
{
    QString group_name = full_port_name.split(":").at(0);
    QString port_name = full_port_name.replace(group_name+":", "");
    int group_id = get_group_id(group_name);

    for (int i=0; i < used_port_names.count(); i++)
    {
        if (used_port_names[i].group_id == group_id && used_port_names[i].name == port_name)
        {
            return used_port_names[i].port_id;
        }
    }

    return -1;
}

QString get_full_port_name(int port_id)
{
    int group_id = -1;
    QString group_name;
    QString port_name;

    for (int i=0; i < used_port_names.count(); i++)
    {
        if (used_port_names[i].port_id == port_id)
        {
            group_id = used_port_names[i].group_id;
            port_name = used_port_names[i].name;
        }
    }

    for (int i=0; i < used_group_names.count(); i++)
    {
        if (used_group_names[i].id == group_id)
        {
            group_name = used_group_names[i].name;
        }
    }

    return group_name+":"+port_name;
}

void canvas_callback(PatchCanvas::CallbackAction action, int value1, int value2, QString value_str)
{
    qDebug("--------------------------- Callback called %i|%i|%i|%s", action, value1, value2, value_str.toStdString().data());

    switch (action)
    {
    case PatchCanvas::ACTION_PORT_INFO:
        QMessageBox::information(main_gui, "port info dialog", "dummy text here");
        break;
    case PatchCanvas::ACTION_PORT_RENAME:
        // Unused
        break;
    case PatchCanvas::ACTION_PORTS_CONNECT:
        jack_connect(jack_client, get_full_port_name(value1).toStdString().data(), get_full_port_name(value2).toStdString().data());
        break;
    case PatchCanvas::ACTION_PORTS_DISCONNECT:
        for (int i=0; i < used_connections.count(); i++)
        {
            if (used_connections[i].id == value1)
            {
                jack_disconnect(jack_client, get_full_port_name(used_connections[i].port_out).toStdString().data(), get_full_port_name(used_connections[i].port_in).toStdString().data());
                break;
            }
        }
        break;
    case PatchCanvas::ACTION_GROUP_INFO:
        QMessageBox::information(main_gui, "group info dialog", "dummy text here");
        break;
    case PatchCanvas::ACTION_GROUP_RENAME:
        // Unused
        break;
    case PatchCanvas::ACTION_GROUP_SPLIT:
        PatchCanvas::splitGroup(value1);
        break;
    case PatchCanvas::ACTION_GROUP_JOIN:
        PatchCanvas::joinGroup(value1);
        break;
    default:
        break;
    }
}

CanvasTestApp::CanvasTestApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CanvasTestApp)
{
    ui->setupUi(this);

    settings = new QSettings("PatchCanvas", "Canvas-test-app");
    restoreGeometry(settings->value("Geometry").toByteArray());

    main_gui = this;
    used_group_names.clear();
    used_port_names.clear();
    used_connections.clear();

    scene = new PatchScene(this, ui->graphicsView);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::TextAntialiasing, true);
    //ui->graphicsView->setRenderHint(QPainter::HighQualityAntialiasing, true);
    //ui->graphicsView->setViewport(new QGLWidget(ui->graphicsView));

    PatchCanvas::options_t options;
    options.auto_hide_groups = false;
    options.use_bezier_lines = true;
    options.antialiasing = PatchCanvas::ANTIALIASING_SMALL;
    options.eyecandy = PatchCanvas::EYECANDY_FULL;
    options.theme_name = PatchCanvas::getDefaultThemeName();

    PatchCanvas::features_t features;
    features.group_info       = false;
    features.group_rename     = false;
    features.port_info        = false;
    features.port_rename      = false;
    features.handle_group_pos = true;

    PatchCanvas::setOptions(&options);
    PatchCanvas::setFeatures(&features);
    PatchCanvas::init(scene, canvas_callback, true);

    connect(this, SIGNAL(clientRegisterCallback(QString,bool)), SLOT(handle_clientRegisterCallback(QString,bool)));
    connect(this, SIGNAL(portRegisterCallback(int,bool)), SLOT(handle_portRegisterCallback(int,bool)));
    connect(this, SIGNAL(connectionCallback(int,int,bool)), SLOT(handle_connectionCallback(int,int,bool)));

    jack_client = jack_client_open("canvas-test-app", JackNullOption, 0);
    jack_set_client_registration_callback(jack_client, client_register_callback, 0);
    jack_set_port_registration_callback(jack_client, port_register_callback, 0);
    jack_set_port_connect_callback(jack_client, port_connect_callback, 0);
    jack_activate(jack_client);

    // query initial jack ports
    QList<QString> parsed_groups;
    const char** ports = jack_get_ports(jack_client, 0, 0, 0);
    if (ports) {
        for (int i=0; ports[i]; i++) {
            QString full_name(ports[i]);
            QString group_name = full_name.split(":").at(0);
            QString port_name = full_name.replace(group_name+":", "");
            int group_id = -1;

            if (parsed_groups.contains(group_name))
            {
                group_id = get_group_id(group_name);
            }
            else
            {
                group_id = last_group_id;

                group_name_to_id_t group_name_to_id;
                group_name_to_id.id = group_id;
                group_name_to_id.name = group_name;
                used_group_names.append(group_name_to_id);

                parsed_groups.append(group_name);
                PatchCanvas::addGroup(group_id, group_name);
                last_group_id++;
            }

            PatchCanvas::PortMode port_mode;
            PatchCanvas::PortType port_type;
            jack_port_t* jack_port = jack_port_by_name(jack_client, ports[i]);

            if (jack_port_flags(jack_port) & JackPortIsInput)
                port_mode = PatchCanvas::PORT_MODE_INPUT;
            else
                port_mode = PatchCanvas::PORT_MODE_OUTPUT;

            if (strcmp(jack_port_type(jack_port), JACK_DEFAULT_AUDIO_TYPE) == 0)
                port_type = PatchCanvas::PORT_TYPE_AUDIO_JACK;
            else
                port_type = PatchCanvas::PORT_TYPE_MIDI_JACK;

            port_name_to_id_t port_name_to_id;
            port_name_to_id.group_id = group_id;
            port_name_to_id.port_id = last_port_id;
            port_name_to_id.name = port_name;
            used_port_names.append(port_name_to_id);
            PatchCanvas::addPort(group_id, last_port_id, port_name, port_mode, port_type);
            last_port_id++;
        }

        jack_free(ports);
    }

    // query connections, after all ports are in place
    ports = jack_get_ports(jack_client, 0, 0, JackPortIsOutput);
    if (ports) {
        for (int i=0; ports[i]; i++) {
            QString this_full_name(ports[i]);
            int this_port_id = get_port_id(this_full_name);

            jack_port_t* jack_port = jack_port_by_name(jack_client, ports[i]);
            const char** connections = jack_port_get_connections(jack_port);

            if (connections) {
                for (int j=0; connections[j]; j++) {
                    QString target_full_name(connections[j]);
                    int target_port_id = get_port_id(target_full_name);

                    connection_to_id_t connection;
                    connection.id = last_connection_id;
                    connection.port_out = this_port_id;
                    connection.port_in = target_port_id;
                    used_connections.append(connection);
                    PatchCanvas::connectPorts(last_connection_id, this_port_id, target_port_id);
                    last_connection_id++;
                }

                jack_free(connections);
            }
        }

        jack_free(ports);
    }
}

CanvasTestApp::~CanvasTestApp()
{
    delete settings;
    delete scene;
    delete ui;
}

void CanvasTestApp::client_register_callback(const char* name, int register_, void*)
{
    main_gui->emit clientRegisterCallback(QString(name), bool(register_));
}

void CanvasTestApp::port_register_callback(jack_port_id_t port_id_jack, int register_, void*)
{
    main_gui->emit portRegisterCallback(port_id_jack, bool(register_));
}

void CanvasTestApp::port_connect_callback(jack_port_id_t port_a, jack_port_id_t port_b, int connect, void*)
{
    main_gui->emit connectionCallback(port_a, port_b, bool(connect));
}

void CanvasTestApp::handle_clientRegisterCallback(QString name, bool yesno)
{
    QString qname(name);

    if (yesno)
    {
        group_name_to_id_t group_name_to_id;
        group_name_to_id.id = last_group_id;
        group_name_to_id.name = qname;
        used_group_names.append(group_name_to_id);
        PatchCanvas::addGroup(last_group_id, qname);
        last_group_id++;
    }
    else
    {
        for (int i=0; i < used_group_names.count(); i++)
        {
            if (used_group_names[i].name == qname)
            {
                PatchCanvas::removeGroup(used_group_names[i].id);
                used_group_names.takeAt(i);
                break;
            }
        }
    }
}

void CanvasTestApp::handle_portRegisterCallback(int port, bool yesno)
{
    jack_port_t* jack_port = jack_port_by_id(jack_client, port);

    QString full_name(jack_port_name(jack_port));
    QString group_name = full_name.split(":").at(0);
    QString port_name = full_name.replace(group_name+":", "");
    int group_id = get_group_id(group_name);

    if (yesno)
    {
        PatchCanvas::PortMode port_mode;
        PatchCanvas::PortType port_type;

        if (jack_port_flags(jack_port) & JackPortIsInput)
            port_mode = PatchCanvas::PORT_MODE_INPUT;
        else
            port_mode = PatchCanvas::PORT_MODE_OUTPUT;

        if (strcmp(jack_port_type(jack_port), JACK_DEFAULT_AUDIO_TYPE) == 0)
            port_type = PatchCanvas::PORT_TYPE_AUDIO_JACK;
        else
            port_type = PatchCanvas::PORT_TYPE_MIDI_JACK;

        port_name_to_id_t port_name_to_id;
        port_name_to_id.group_id = group_id;
        port_name_to_id.port_id = last_port_id;
        port_name_to_id.name = port_name;
        used_port_names.append(port_name_to_id);
        PatchCanvas::addPort(group_id, last_port_id, port_name, port_mode, port_type);
        last_port_id++;
    }
    else
    {
        for (int i=0; i < used_port_names.count(); i++)
        {
            if (used_port_names[i].group_id == group_id && used_port_names[i].name == port_name)
            {
                PatchCanvas::removePort(used_port_names[i].port_id);
                used_port_names.takeAt(i);
                break;
            }
        }
    }
}

void CanvasTestApp::handle_connectionCallback(int port_a, int port_b, bool yesno)
{
    jack_port_t* jack_port_a = jack_port_by_id(jack_client, port_a);
    jack_port_t* jack_port_b = jack_port_by_id(jack_client, port_b);
    int port_id_a = get_port_id(QString(jack_port_name(jack_port_a)));
    int port_id_b = get_port_id(QString(jack_port_name(jack_port_b)));

    if (yesno)
    {
        connection_to_id_t connection;
        connection.id = last_connection_id;
        connection.port_out = port_id_a;
        connection.port_in = port_id_b;
        used_connections.append(connection);
        PatchCanvas::connectPorts(last_connection_id, port_id_a, port_id_b);
        last_connection_id++;
    }
    else
    {
        for (int i=0; i < used_connections.count(); i++)
        {
            if (used_connections[i].port_out == port_id_a && used_connections[i].port_in == port_id_b)
            {
                PatchCanvas::disconnectPorts(used_connections[i].id);
                used_connections.takeAt(i);
                break;
            }
        }
    }
}

void CanvasTestApp::closeEvent(QCloseEvent* event)
{
    jack_deactivate(jack_client);
    jack_client_close(jack_client);

    PatchCanvas::clear();

    settings->setValue("Geometry", QVariant(saveGeometry()));

    QMainWindow::closeEvent(event);
}

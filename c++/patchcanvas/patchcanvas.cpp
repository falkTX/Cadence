/*
 * Patchbay Canvas engine using QGraphicsView/Scene
 * Copyright (C) 2010-2012 Filipe Coelho <falktx@gmail.com>
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

#include "patchcanvas.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QAction>

#include "canvasfadeanimation.h"
#include "canvasline.h"
#include "canvasbezierline.h"
#include "canvasport.h"
#include "canvasbox.h"

CanvasObject::CanvasObject(QObject* parent) : QObject(parent) {}

void CanvasObject::AnimationIdle()
{
    PatchCanvas::CanvasFadeAnimation* animation = (PatchCanvas::CanvasFadeAnimation*)sender();
    if (animation)
        PatchCanvas::CanvasRemoveAnimation(animation);
}

void CanvasObject::AnimationHide()
{
    PatchCanvas::CanvasFadeAnimation* animation = (PatchCanvas::CanvasFadeAnimation*)sender();
    if (animation)
    {
        if (animation->item())
            animation->item()->hide();
        PatchCanvas::CanvasRemoveAnimation(animation);
    }
}

void CanvasObject::AnimationDestroy()
{
    PatchCanvas::CanvasFadeAnimation* animation = (PatchCanvas::CanvasFadeAnimation*)sender();
    if (animation)
    {
        if (animation->item())
            PatchCanvas::CanvasRemoveItemFX(animation->item());
        PatchCanvas::CanvasRemoveAnimation(animation);
    }
}

void CanvasObject::CanvasPostponedGroups()
{
    PatchCanvas::CanvasPostponedGroups();
}

void CanvasObject::PortContextMenuDisconnect()
{
    bool ok;
    int connection_id = ((QAction*)sender())->data().toInt(&ok);
    if (ok)
        PatchCanvas::CanvasCallback(PatchCanvas::ACTION_PORTS_DISCONNECT, connection_id, 0, "");
}

START_NAMESPACE_PATCHCANVAS

/* contructor and destructor */
Canvas::Canvas()
{
    qobject   = 0;
    settings  = 0;
    theme     = 0;
    initiated = false;
}

Canvas::~Canvas()
{
    if (qobject)
        delete qobject;
    if (settings)
        delete settings;
    if (theme)
        delete theme;
}

/* Global objects */
Canvas canvas;

options_t options = {
    /* theme_name */       getDefaultThemeName(),
    /* auto_hide_groups */ false,
    /* use_bezier_lines */ true,
    /* antialiasing */     ANTIALIASING_SMALL,
    /* eyecandy */         EYECANDY_SMALL
};

features_t features = {
    /* group_info */       false,
    /* group_rename */     false,
    /* port_info */        false,
    /* port_rename */      false,
    /* handle_group_pos */ false
};

/* Internal functions */
const char* bool2str(bool check)
{
    return check ? "true" : "false";
}

const char* port_mode2str(PortMode port_mode)
{
    if (port_mode == PORT_MODE_NULL)
        return "PORT_MODE_NULL";
    else if (port_mode == PORT_MODE_INPUT)
        return "PORT_MODE_INPUT";
    else if (port_mode == PORT_MODE_OUTPUT)
        return "PORT_MODE_OUTPUT";
    else
        return "PORT_MODE_???";
}

const char* port_type2str(PortType port_type)
{
    if (port_type == PORT_TYPE_NULL)
        return "PORT_TYPE_NULL";
    else if (port_type == PORT_TYPE_AUDIO_JACK)
        return "PORT_TYPE_AUDIO_JACK";
    else if (port_type == PORT_TYPE_MIDI_JACK)
        return "PORT_TYPE_MIDI_JACK";
    else if (port_type == PORT_TYPE_MIDI_A2J)
        return "PORT_TYPE_MIDI_A2J";
    else if (port_type == PORT_TYPE_MIDI_ALSA)
        return "PORT_TYPE_MIDI_ALSA";
    else
        return "PORT_TYPE_???";
}

const char* icon2str(Icon icon)
{
    if (icon == ICON_HARDWARE)
        return "ICON_HARDWARE";
    else if (ICON_APPLICATION)
        return "ICON_APPLICATION";
    else if (ICON_LADISH_ROOM)
        return "ICON_LADISH_ROOM";
    else
        return "ICON_???";
}

const char* split2str(SplitOption split)
{
    if (split == SPLIT_UNDEF)
        return "SPLIT_UNDEF";
    else if (split == SPLIT_NO)
        return "SPLIT_NO";
    else if (split == SPLIT_YES)
        return "SPLIT_YES";
    else
        return "SPLIT_???";
}

/* PatchCanvas API */
void setOptions(options_t* new_options)
{
    if (canvas.initiated) return;
    options.theme_name        = new_options->theme_name;
    options.auto_hide_groups  = new_options->auto_hide_groups;
    options.use_bezier_lines  = new_options->use_bezier_lines;
    options.antialiasing      = new_options->antialiasing;
    options.eyecandy          = new_options->eyecandy;
}

void setFeatures(features_t* new_features)
{
    if (canvas.initiated) return;
    features.group_info       = new_features->group_info;
    features.group_rename     = new_features->group_rename;
    features.port_info        = new_features->port_info;
    features.port_rename      = new_features->port_rename;
    features.handle_group_pos = new_features->handle_group_pos;
}

void init(PatchScene* scene, Callback callback, bool debug)
{
    if (debug)
        qDebug("PatchCanvas::init(%p, %p, %s)", scene, callback, bool2str(debug));

    if (canvas.initiated)
    {
        qCritical("PatchCanvas::init() - already initiated");
        return;
    }

    if (!callback)
    {
        qFatal("PatchCanvas::init() - fatal error: callback not set");
        return;
    }

    canvas.scene = scene;
    canvas.callback = callback;
    canvas.debug = debug;

    canvas.last_z_value = 0;
    canvas.last_connection_id = 0;
    canvas.initial_pos = QPointF(0, 0);
    canvas.size_rect = QRectF();

    canvas.group_list.clear();
    canvas.port_list.clear();
    canvas.connection_list.clear();
    canvas.animation_list.clear();

    if (!canvas.qobject) canvas.qobject = new CanvasObject();
    if (!canvas.settings) canvas.settings = new QSettings(PATCHCANVAS_ORGANISATION_NAME, "PatchCanvas");

    if (canvas.theme)
    {
        delete canvas.theme;
        canvas.theme = 0;
    }

    for (int i=0; i<Theme::THEME_MAX; i++)
    {
        QString this_theme_name = getThemeName(static_cast<Theme::List>(i));
        if (this_theme_name == options.theme_name)
        {
            canvas.theme = new Theme(static_cast<Theme::List>(i));
            break;
        }
    }

    if (!canvas.theme)
        canvas.theme = new Theme(getDefaultTheme());

    canvas.scene->updateTheme();

    canvas.initiated = true;
}

void clear()
{
    if (canvas.debug)
        qDebug("PatchCanvas::clear()");

    QList<int> group_list_ids;
    QList<int> port_list_ids;
    QList<int> connection_list_ids;

    foreach (const group_dict_t& group, canvas.group_list)
        group_list_ids.append(group.group_id);

    foreach (const port_dict_t& port, canvas.port_list)
        port_list_ids.append(port.port_id);

    foreach (const connection_dict_t& connection, canvas.connection_list)
        connection_list_ids.append(connection.connection_id);

    foreach (const int& idx, connection_list_ids)
        disconnectPorts(idx);

    foreach (const int& idx, port_list_ids)
        removePort(idx);

    foreach (const int& idx, group_list_ids)
        removeGroup(idx);

    canvas.last_z_value = 0;
    canvas.last_connection_id = 0;

    canvas.group_list.clear();
    canvas.port_list.clear();
    canvas.connection_list.clear();

    canvas.initiated = false;
}

void setInitialPos(int x, int y)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setInitialPos(%i, %i)", x, y);

    canvas.initial_pos.setX(x);
    canvas.initial_pos.setY(y);
}

void setCanvasSize(int x, int y, int width, int height)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setCanvasSize(%i, %i, %i, %i)", x, y, width, height);

    canvas.size_rect.setX(x);
    canvas.size_rect.setY(y);
    canvas.size_rect.setWidth(width);
    canvas.size_rect.setHeight(height);
}

void addGroup(int group_id, QString group_name, SplitOption split, Icon icon)
{
    if (canvas.debug)
        qDebug("PatchCanvas::addGroup(%i, %s, %s, %s)", group_id, group_name.toUtf8().constData(), split2str(split), icon2str(icon));

    if (split == SPLIT_UNDEF && features.handle_group_pos)
        split = static_cast<SplitOption>(canvas.settings->value(QString("CanvasPositions/%1_SPLIT").arg(group_name), split).toInt());

    CanvasBox* group_box = new CanvasBox(group_id, group_name, icon);

    group_dict_t group_dict;
    group_dict.group_id   = group_id;
    group_dict.group_name = group_name;
    group_dict.split = (split == SPLIT_YES);
    group_dict.icon  = icon;
    group_dict.widgets[0] = group_box;
    group_dict.widgets[1] = 0;

    if (split == SPLIT_YES)
    {
        group_box->setSplit(true, PORT_MODE_OUTPUT);

        if (features.handle_group_pos)
            group_box->setPos(canvas.settings->value(QString("CanvasPositions/%1_OUTPUT").arg(group_name), CanvasGetNewGroupPos()).toPointF());
        else
            group_box->setPos(CanvasGetNewGroupPos());

        CanvasBox* group_sbox = new CanvasBox(group_id, group_name, icon);
        group_sbox->setSplit(true, PORT_MODE_INPUT);

        group_dict.widgets[1] = group_sbox;

        if (features.handle_group_pos)
            group_sbox->setPos(canvas.settings->value(QString("CanvasPositions/%1_INPUT").arg(group_name), CanvasGetNewGroupPos(true)).toPointF());
        else
            group_sbox->setPos(CanvasGetNewGroupPos(true));

        canvas.last_z_value += 1;
        group_sbox->setZValue(canvas.last_z_value);

        if (options.auto_hide_groups == false && options.eyecandy)
            CanvasItemFX(group_sbox, true);
    }
    else
    {
        group_box->setSplit(false);

        if (features.handle_group_pos)
            group_box->setPos(canvas.settings->value(QString("CanvasPositions/%1").arg(group_name), CanvasGetNewGroupPos()).toPointF());
        else
        {
            // Special ladish fake-split groups
            bool horizontal = (icon == ICON_HARDWARE || icon == ICON_LADISH_ROOM);
            group_box->setPos(CanvasGetNewGroupPos(horizontal));
        }
    }

    canvas.last_z_value += 1;
    group_box->setZValue(canvas.last_z_value);

    canvas.group_list.append(group_dict);

    if (options.auto_hide_groups == false && options.eyecandy)
        CanvasItemFX(group_box, true);

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

void removeGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::removeGroup(%i)", group_id);

    foreach2 (const group_dict_t& group, canvas.group_list)
        if (group.group_id == group_id)
        {
            CanvasBox* item = group.widgets[0];
            QString group_name = group.group_name;

            if (group.split)
            {
                CanvasBox* s_item = group.widgets[1];
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1_OUTPUT").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_INPUT").arg(group_name), s_item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_SPLIT").arg(group_name), SPLIT_YES);
                }

                if (options.eyecandy)
                {
                    CanvasItemFX(s_item, false, true);
                }
                else
                {
                    s_item->removeIconFromScene();
                    canvas.scene->removeItem(s_item);
                    delete s_item;
                }
            }
            else
            {
                if (features.handle_group_pos)
                {
                    canvas.settings->setValue(QString("CanvasPositions/%1").arg(group_name), item->pos());
                    canvas.settings->setValue(QString("CanvasPositions/%1_SPLIT").arg(group_name), SPLIT_NO);
                }
            }

            if (options.eyecandy)
            {
                CanvasItemFX(item, false, true);
            }
            else
            {
                item->removeIconFromScene();
                canvas.scene->removeItem(item);
                delete item;
            }

            canvas.group_list.takeAt(i);

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::removeGroup(%i) - unable to find group to remove", group_id);
}

void renameGroup(int group_id, QString new_group_name)
{
    if (canvas.debug)
        qDebug("PatchCanvas::renameGroup(%i, %s)", group_id, new_group_name.toUtf8().constData());

    foreach2 (group_dict_t& group, canvas.group_list)
        if (group.group_id == group_id)
        {
            group.group_name = new_group_name;
            group.widgets[0]->setGroupName(new_group_name);

            if (group.split && group.widgets[1])
                group.widgets[1]->setGroupName(new_group_name);

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::renameGroup(%i, %s) - unable to find group to rename", group_id, new_group_name.toUtf8().constData());
}

void splitGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::splitGroup(%i)", group_id);

    CanvasBox* item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
        {
            if (group.split)
            {
                qCritical("PatchCanvas::splitGroup(%i) - group is already splitted", group_id);
                return;
            }

            item = group.widgets[0];
            group_name = group.group_name;
            group_icon = group.icon;
            break;
        }
    }

    if (!item)
    {
        qCritical("PatchCanvas::splitGroup(%i) - unable to find group to split", group_id);
        return;
    }

    QList<int> port_list_ids = QList<int>(item->getPortList());

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port_list_ids.contains(port.port_id))
        {
            port_dict_t port_dict;
            port_dict.group_id  = port.group_id;
            port_dict.port_id   = port.port_id;
            port_dict.port_name = port.port_name;
            port_dict.port_mode = port.port_mode;
            port_dict.port_type = port.port_type;
            port_dict.widget    = 0;
            ports_data.append(port_dict);
        }
    }

    foreach (const connection_dict_t& connection, canvas.connection_list)
    {
        if (port_list_ids.contains(connection.port_out_id) || port_list_ids.contains(connection.port_in_id))
        {
            connection_dict_t connection_dict;
            connection_dict.connection_id = connection.connection_id;
            connection_dict.port_in_id    = connection.port_in_id;
            connection_dict.port_out_id   = connection.port_out_id;
            connection_dict.widget        = 0;
            conns_data.append(connection_dict);
        }
    }

    // Step 2 - Remove Item and Children
    foreach (const connection_dict_t& conn, conns_data)
        disconnectPorts(conn.connection_id);

    foreach (const int& port_id, port_list_ids)
        removePort(port_id);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now splitted
    addGroup(group_id, group_name, SPLIT_YES, group_icon);

    foreach (const port_dict_t& port, ports_data)
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type);

    foreach (const connection_dict_t& conn, conns_data)
        connectPorts(conn.connection_id, conn.port_out_id, conn.port_in_id);

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

void joinGroup(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::joinGroup(%i)", group_id);

    CanvasBox* item = 0;
    CanvasBox* s_item = 0;
    QString group_name;
    Icon group_icon = ICON_APPLICATION;
    QList<port_dict_t> ports_data;
    QList<connection_dict_t> conns_data;

    // Step 1 - Store all Item data
    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
        {
            if (group.split == false)
            {
                qCritical("PatchCanvas::joinGroup(%i) - group is not splitted", group_id);
                return;
            }

            item   = group.widgets[0];
            s_item = group.widgets[1];
            group_name = group.group_name;
            group_icon = group.icon;
            break;
        }
    }

    if (!item || !s_item)
    {
        qCritical("PatchCanvas::joinGroup(%i) - Unable to find groups to join", group_id);
        return;
    }

    QList<int> port_list_ids  = QList<int>(item->getPortList());
    QList<int> port_list_idss = s_item->getPortList();

    foreach (const int& port_id, port_list_idss)
    {
        if (port_list_ids.contains(port_id) == false)
            port_list_ids.append(port_id);
    }

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port_list_ids.contains(port.port_id))
        {
            port_dict_t port_dict;
            port_dict.group_id  = port.group_id;
            port_dict.port_id   = port.port_id;
            port_dict.port_name = port.port_name;
            port_dict.port_mode = port.port_mode;
            port_dict.port_type = port.port_type;
            port_dict.widget    = 0;
            ports_data.append(port_dict);
        }
    }

    foreach (const connection_dict_t& connection, canvas.connection_list)
    {
        if (port_list_ids.contains(connection.port_out_id) || port_list_ids.contains(connection.port_in_id))
        {
            connection_dict_t connection_dict;
            connection_dict.connection_id = connection.connection_id;
            connection_dict.port_in_id    = connection.port_in_id;
            connection_dict.port_out_id   = connection.port_out_id;
            connection_dict.widget        = 0;
            conns_data.append(connection_dict);
        }
    }

    // Step 2 - Remove Item and Children
    foreach (const connection_dict_t& conn, conns_data)
        disconnectPorts(conn.connection_id);

    foreach (const int& port_id, port_list_ids)
        removePort(port_id);

    removeGroup(group_id);

    // Step 3 - Re-create Item, now together
    addGroup(group_id, group_name, SPLIT_NO, group_icon);

    foreach (const port_dict_t& port, ports_data)
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type);

    foreach (const connection_dict_t& conn, conns_data)
        connectPorts(conn.connection_id, conn.port_out_id, conn.port_in_id);

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

QPointF getGroupPos(int group_id, PortMode port_mode)
{
    if (canvas.debug)
        qDebug("PatchCanvas::getGroupPos(%i, %s)", group_id, port_mode2str(port_mode));

    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
        {
            if (group.split)
            {
                if (port_mode == PORT_MODE_OUTPUT)
                    return group.widgets[0]->pos();
                else if (port_mode == PORT_MODE_INPUT)
                    return group.widgets[1]->pos();
                else
                    return QPointF(0, 0);
            }
            else
                return group.widgets[0]->pos();
        }
    }

    qCritical("PatchCanvas::getGroupPos(%i, %s) - unable to find group", group_id, port_mode2str(port_mode));
    return QPointF(0,0);
}

void setGroupPos(int group_id, int group_pos_x, int group_pos_y)
{
    setGroupPos(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y);
}

void setGroupPos(int group_id, int group_pos_x, int group_pos_y, int group_pos_xs, int group_pos_ys)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)", group_id, group_pos_x, group_pos_y, group_pos_xs, group_pos_ys);

    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
        {
            group.widgets[0]->setPos(group_pos_x, group_pos_y);

            if (group.split && group.widgets[1])
            {
                group.widgets[1]->setPos(group_pos_xs, group_pos_ys);
            }

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i) - unable to find group to reposition", group_id, group_pos_x, group_pos_y, group_pos_xs, group_pos_ys);
}

void setGroupIcon(int group_id, Icon icon)
{
    if (canvas.debug)
        qDebug("PatchCanvas::setGroupIcon(%i, %s)", group_id, icon2str(icon));

    foreach2 (group_dict_t& group, canvas.group_list)
        if (group.group_id == group_id)
        {
            group.icon = icon;
            group.widgets[0]->setIcon(icon);

            if (group.split && group.widgets[1])
                group.widgets[1]->setIcon(icon);

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::setGroupIcon(%i, %s) - unable to find group to change icon", group_id, icon2str(icon));
}

void addPort(int group_id, int port_id, QString port_name, PortMode port_mode, PortType port_type)
{
    if (canvas.debug)
        qDebug("PatchCanvas::addPort(%i, %i, %s, %s, %s)", group_id, port_id, port_name.toUtf8().constData(), port_mode2str(port_mode), port_type2str(port_type));

    CanvasBox* box_widget = 0;
    CanvasPort* port_widget = 0;

    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
        {
            int n;
            if (group.split && group.widgets[0]->getSplittedMode() != port_mode && group.widgets[1])
                n = 1;
            else
                n = 0;
            box_widget = group.widgets[n];
            port_widget = box_widget->addPortFromGroup(port_id, port_name, port_mode, port_type);
            break;
        }
    }

    if (!box_widget || !port_widget)
    {
        qCritical("PatchCanvas::addPort(%i, %i, %s, %s, %s) - unable to find parent group", group_id, port_id, port_name.toUtf8().constData(), port_mode2str(port_mode), port_type2str(port_type));
        return;
    }

    if (options.eyecandy)
        CanvasItemFX(port_widget, true);

    port_dict_t port_dict;
    port_dict.group_id  = group_id;
    port_dict.port_id   = port_id;
    port_dict.port_name = port_name;
    port_dict.port_mode = port_mode;
    port_dict.port_type = port_type;
    port_dict.widget    = port_widget;
    canvas.port_list.append(port_dict);

    box_widget->updatePositions();

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

void removePort(int port_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::removePort(%i)", port_id);

    foreach2 (const port_dict_t& port, canvas.port_list)
        if (port.port_id == port_id)
        {
            CanvasPort* item = port.widget;
            ((CanvasBox*)item->parentItem())->removePortFromGroup(port_id);
            canvas.scene->removeItem(item);
            delete item;

            canvas.port_list.takeAt(i);

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::removePort(%i) - unable to find port to remove", port_id);
}

void renamePort(int port_id, QString new_port_name)
{
    if (canvas.debug)
        qDebug("PatchCanvas::renamePort(%i, %s)", port_id, new_port_name.toUtf8().constData());

    foreach2 (port_dict_t& port, canvas.port_list)
        if (port.port_id == port_id)
        {
            port.port_name = new_port_name;
            port.widget->setPortName(new_port_name);
            ((CanvasBox*)port.widget->parentItem())->updatePositions();

            QTimer::singleShot(0, canvas.scene, SLOT(update()));
            return;
        }
    }

    qCritical("PatchCanvas::renamePort(%i, %s) - unable to find port to rename", port_id, new_port_name.toUtf8().constData());
}

void connectPorts(int connection_id, int port_out_id, int port_in_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::connectPorts(%i, %i, %i)", connection_id, port_out_id, port_in_id);

    CanvasPort* port_out = 0;
    CanvasPort* port_in  = 0;
    CanvasBox* port_out_parent = 0;
    CanvasBox* port_in_parent  = 0;

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port.port_id == port_out_id)
        {
            port_out = port.widget;
            port_out_parent = (CanvasBox*)port_out->parentItem();
        }
        else if (port.port_id == port_in_id)
        {
            port_in = port.widget;
            port_in_parent = (CanvasBox*)port_in->parentItem();
        }
    }

    if (!port_out || !port_in)
    {
        qCritical("PatchCanvas::connectPorts(%i, %i, %i) - Unable to find ports to connect", connection_id, port_out_id, port_in_id);
        return;
    }

    connection_dict_t connection_dict;
    connection_dict.connection_id = connection_id;
    connection_dict.port_out_id = port_out_id;
    connection_dict.port_in_id  = port_in_id;

    if (options.use_bezier_lines)
        connection_dict.widget = new CanvasBezierLine(port_out, port_in, 0);
    else
        connection_dict.widget = new CanvasLine(port_out, port_in, 0);

    port_out_parent->addLineFromGroup(connection_dict.widget, connection_id);
    port_in_parent->addLineFromGroup(connection_dict.widget, connection_id);

    canvas.last_z_value += 1;
    port_out_parent->setZValue(canvas.last_z_value);
    port_in_parent->setZValue(canvas.last_z_value);

    canvas.last_z_value += 1;
    connection_dict.widget->setZValue(canvas.last_z_value);

    canvas.connection_list.append(connection_dict);

    if (options.eyecandy)
    {
        QGraphicsItem* item = (options.use_bezier_lines) ? (QGraphicsItem*)(CanvasBezierLine*)connection_dict.widget : (QGraphicsItem*)(CanvasLine*)connection_dict.widget;
        CanvasItemFX(item, true);
    }

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

void disconnectPorts(int connection_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::disconnectPorts(%i)", connection_id);

    int port_1_id, port_2_id;
    AbstractCanvasLine* line = 0;
    QGraphicsItem* item1 = 0;
    QGraphicsItem* item2 = 0;

    foreach2 (const connection_dict_t& connection, canvas.connection_list)
        if (connection.connection_id == connection_id)
        {
            port_1_id = connection.port_out_id;
            port_2_id = connection.port_in_id;
            line = connection.widget;
            canvas.connection_list.takeAt(i);
            break;
        }
    }

    if (!line)
    {
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find connection ports", connection_id);
        return;
    }

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port.port_id == port_1_id)
        {
            item1 = port.widget;
            break;
        }
    }

    if (!item1)
    {
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find output port", connection_id);
        return;
    }

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port.port_id == port_2_id)
        {
            item2 = port.widget;
            break;
        }
    }

    if (!item2)
    {
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find input port", connection_id);
        return;
    }

    ((CanvasBox*)item1->parentItem())->removeLineFromGroup(connection_id);
    ((CanvasBox*)item2->parentItem())->removeLineFromGroup(connection_id);

    if (options.eyecandy)
    {
        QGraphicsItem* item = (options.use_bezier_lines) ? (QGraphicsItem*)(CanvasBezierLine*)line : (QGraphicsItem*)(CanvasLine*)line;
        CanvasItemFX(item, false, true);
    }
    else
        line->deleteFromScene();

    QTimer::singleShot(0, canvas.scene, SLOT(update()));
}

void Arrange()
{
    if (canvas.debug)
        qDebug("PatchCanvas::Arrange()");
}

/* Extra Internal functions */

QString CanvasGetGroupName(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetGroupName(%i)", group_id);

    foreach (const group_dict_t& group, canvas.group_list)
    {
        if (group.group_id == group_id)
            return group.group_name;
    }

    qCritical("PatchCanvas::CanvasGetGroupName(%i) - unable to find group", group_id);
    return "";
}

int CanvasGetGroupPortCount(int group_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetGroupPortCount(%i)", group_id);

    int port_count = 0;
    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port.group_id == group_id)
            port_count += 1;
    }

    return port_count;
}

QPointF CanvasGetNewGroupPos(bool horizontal)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetNewGroupPos(%s)", bool2str(horizontal));

    QPointF new_pos(canvas.initial_pos.x(), canvas.initial_pos.y());
    QList<QGraphicsItem*> items = canvas.scene->items();

    bool break_loop = false;
    while (break_loop == false)
    {
        bool break_for = false;
        for (int i=0; i < items.count(); i++)
        {
            QGraphicsItem* item = items[i];
            if (item && item->type() == CanvasBoxType)
            {
                if (item->sceneBoundingRect().contains(new_pos))
                {
                    if (horizontal)
                        new_pos += QPointF(item->boundingRect().width()+15, 0);
                    else
                        new_pos += QPointF(0, item->boundingRect().height()+15);
                    break;
                }
            }
            if (i >= items.count()-1 && break_for == false)
                break_loop = true;
        }
    }

    return new_pos;
}

QString CanvasGetFullPortName(int port_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetFullPortName(%i)", port_id);

    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (port.port_id == port_id)
        {
            int group_id = port.group_id;
            foreach (const group_dict_t& group, canvas.group_list)
            {
                if (group.group_id == group_id)
                    return group.group_name + ":" + port.port_name;
            }
            break;
        }
    }

    qCritical("PatchCanvas::CanvasGetFullPortName(%i) - unable to find port", port_id);
    return "";
}

QList<int> CanvasGetPortConnectionList(int port_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetPortConnectionList(%i)", port_id);

    QList<int> port_con_list;

    foreach (const connection_dict_t& connection, canvas.connection_list)
    {
        if (connection.port_out_id == port_id || connection.port_in_id == port_id)
            port_con_list.append(connection.connection_id);
    }

    return port_con_list;
}

int CanvasGetConnectedPort(int connection_id, int port_id)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasGetConnectedPort(%i, %i)", connection_id, port_id);

    foreach (const connection_dict_t& connection, canvas.connection_list)
    {
        if (connection.connection_id == connection_id)
        {
            if (connection.port_out_id == port_id)
                return connection.port_in_id;
            else
                return connection.port_out_id;
        }
    }

    qCritical("PatchCanvas::CanvasGetConnectedPort(%i, %i) - unable to find connection", connection_id, port_id);
    return 0;
}

void CanvasRemoveAnimation(CanvasFadeAnimation* f_animation)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasRemoveAnimation(%p)", f_animation);

    foreach2 (const animation_dict_t& animation, canvas.animation_list)
        if (animation.animation == f_animation)
        {
            delete animation.animation;
            canvas.animation_list.takeAt(i);
            break;
        }
    }
}

void CanvasPostponedGroups()
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasPostponedGroups()");
}

void CanvasCallback(CallbackAction action, int value1, int value2, QString value_str)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasCallback(%i, %i, %i, %s)", action, value1, value2, value_str.toStdString().data());

    canvas.callback(action, value1, value2, value_str);
}

void CanvasItemFX(QGraphicsItem* item, bool show, bool destroy)
{
    if (canvas.debug)
        qDebug("PatchCanvas::CanvasItemFX(%p, %s, %s)", item, bool2str(show), bool2str(destroy));

    // Check if item already has an animationItemFX
    foreach2 (const animation_dict_t& animation, canvas.animation_list)
        if (animation.item == item)
        {
            if (animation.animation)
            {
                animation.animation->stop();
                delete animation.animation;
            }
            canvas.animation_list.takeAt(i);
            break;
        }
    }

    CanvasFadeAnimation* animation = new CanvasFadeAnimation(item, show);
    animation->setDuration(show ? 750 : 500);

    animation_dict_t animation_dict;
    animation_dict.animation = animation;
    animation_dict.item = item;
    canvas.animation_list.append(animation_dict);

    if (show)
    {
        QObject::connect(animation, SIGNAL(finished()), canvas.qobject, SLOT(AnimationIdle()));
    }
    else
    {
        if (destroy)
            QObject::connect(animation, SIGNAL(finished()), canvas.qobject, SLOT(AnimationDestroy()));
        else
            QObject::connect(animation, SIGNAL(finished()), canvas.qobject, SLOT(AnimationHide()));
    }

    animation->start();
}

void CanvasRemoveItemFX(QGraphicsItem* item)
{
    if (canvas.debug)
      qDebug("PatchCanvas::CanvasRemoveItemFX(%p)", item);

    switch (item->type())
    {
    case CanvasBoxType:
    {
        CanvasBox* box = (CanvasBox*)item;
        box->removeIconFromScene();
        canvas.scene->removeItem(box);
        delete box;
    }
    case CanvasPortType:
    {
        CanvasPort* port = (CanvasPort*)item;
        canvas.scene->removeItem(port);
        delete port;
    }
    case CanvasLineType:
    case CanvasBezierLineType:
    {
        AbstractCanvasLine* line = (AbstractCanvasLine*)item;
        line->deleteFromScene();
    }
    default:
        break;
    }
}

END_NAMESPACE_PATCHCANVAS

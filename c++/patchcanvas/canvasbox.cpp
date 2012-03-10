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

#include "canvasbox.h"

#include <QtCore/QTimer>
#include <QtGui/QCursor>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QPainter>

#include "canvasline.h"
#include "canvasbezierline.h"
#include "canvasport.h"
#include "canvasboxshadow.h"
#include "canvasicon.h"

START_NAMESPACE_PATCHCANVAS

CanvasBox::CanvasBox(int group_id, QString group_name, Icon icon, QGraphicsItem* parent) :
    QGraphicsItem(parent, canvas.scene)
{
    // Save Variables, useful for later
    m_group_id   = group_id;
    m_group_name = group_name;

    // Base Variables
    p_width  = 50;
    p_height = 25;

    m_last_pos = QPointF();
    m_splitted = false;
    m_splitted_mode = PORT_MODE_NULL;

    m_cursor_moving = false;
    m_forced_split  = false;
    m_mouse_down    = false;

    m_port_list_ids.clear();
    m_connection_lines.clear();

    // Set Font
    m_font_name = QFont(canvas.theme->box_font_name, canvas.theme->box_font_size, canvas.theme->box_font_state);
    m_font_port = QFont(canvas.theme->port_font_name, canvas.theme->port_font_size, canvas.theme->port_font_state);

    // Icon
    icon_svg = new CanvasIcon(icon, group_name, this);

    // Shadow
    if (options.eyecandy)
    {
        shadow = new CanvasBoxShadow(toGraphicsObject());
        shadow->setFakeParent(this);
        setGraphicsEffect(shadow);
    }
    else
        shadow = 0;

    // Final touches
    setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);

    // Wait for at least 1 port
    if (options.auto_hide_groups)
        setVisible(false);

    updatePositions();
}

CanvasBox::~CanvasBox()
{
    if (shadow)
        delete shadow;
    delete icon_svg;
}

int CanvasBox::getGroupId()
{
    return m_group_id;
}

QString CanvasBox::getGroupName()
{
    return m_group_name;
}

bool CanvasBox::isSplitted()
{
    return m_splitted;
}

PortMode CanvasBox::getSplittedMode()
{
    return m_splitted_mode;
}

int CanvasBox::getPortCount()
{
    return m_port_list_ids.count();
}

QList<int> CanvasBox::getPortList()
{
    return m_port_list_ids;
}

void CanvasBox::setIcon(Icon icon)
{
    icon_svg->setIcon(icon, m_group_name);
}

void CanvasBox::setSplit(bool split, PortMode mode)
{
    m_splitted = split;
    m_splitted_mode = mode;
}

void CanvasBox::setGroupName(QString group_name)
{
    m_group_name = group_name;
    updatePositions();
}

CanvasPort* CanvasBox::addPortFromGroup(int port_id, QString port_name, PortMode port_mode, PortType port_type)
{
    if (m_port_list_ids.count() == 0)
    {
        if (options.auto_hide_groups)
            setVisible(true);
    }

    CanvasPort* new_widget = new CanvasPort(port_id, port_name, port_mode, port_type, this);

    port_dict_t port_dict;
    port_dict.group_id  = m_group_id;
    port_dict.port_id   = port_id;
    port_dict.port_name = port_name;
    port_dict.port_mode = port_mode;
    port_dict.port_type = port_type;
    port_dict.widget    = new_widget;

    m_port_list_ids.append(port_id);

    return new_widget;
}

void CanvasBox::removePortFromGroup(int port_id)
{
    if (m_port_list_ids.contains(port_id))
    {
        m_port_list_ids.removeOne(port_id);
    }
    else
    {
        qCritical("PatchCanvas::CanvasBox->removePort(%i) - unable to find port to remove", port_id);
        return;
    }

    if (m_port_list_ids.count() > 0)
    {
        updatePositions();
    }
    else if (isVisible())
    {
        if (options.auto_hide_groups)
            setVisible(false);
    }
}

void CanvasBox::addLineFromGroup(AbstractCanvasLine* line, int connection_id)
{
    cb_line_t new_cbline;
    new_cbline.line = line;
    new_cbline.connection_id = connection_id;
    m_connection_lines.append(new_cbline);
}

void CanvasBox::removeLineFromGroup(int connection_id)
{
    foreach2 (const cb_line_t& connection, m_connection_lines)
        if (connection.connection_id == connection_id)
        {
            m_connection_lines.takeAt(i);
            return;
        }
    }

    qCritical("PatchCanvas::CanvasBox->removeLineFromGroup(%i) - unable to find line to remove", connection_id);
}

void CanvasBox::checkItemPos()
{
    if (canvas.size_rect.isNull() == false)
    {
        QPointF pos = scenePos();
        if (canvas.size_rect.contains(pos) == false || canvas.size_rect.contains(pos+QPointF(p_width, p_height)) == false)
        {
            if (pos.x() < canvas.size_rect.x())
                setPos(canvas.size_rect.x(), pos.y());
            else if (pos.x()+p_width > canvas.size_rect.width())
                setPos(canvas.size_rect.width()-p_width, pos.y());

            pos = scenePos();
            if (pos.y() < canvas.size_rect.y())
                setPos(pos.x(), canvas.size_rect.y());
            else if (pos.y()+p_height > canvas.size_rect.height())
                setPos(pos.x(), canvas.size_rect.height()-p_height);
        }
    }
}

void CanvasBox::removeIconFromScene()
{
    canvas.scene->removeItem(icon_svg);
}

void CanvasBox::updatePositions()
{
    prepareGeometryChange();

    int max_in_width   = 0;
    int max_in_height  = 24;
    int max_out_width  = 0;
    int max_out_height = 24;
    bool have_audio_jack_in, have_audio_jack_out, have_midi_jack_in, have_midi_jack_out;
    bool have_midi_a2j_in,  have_midi_a2j_out, have_midi_alsa_in,  have_midi_alsa_out;
    have_audio_jack_in  = have_midi_jack_in  = have_midi_a2j_in  = have_midi_alsa_in  = false;
    have_audio_jack_out = have_midi_jack_out = have_midi_a2j_out = have_midi_alsa_out = false;

    // reset box size
    p_width  = 50;
    p_height = 25;

    // Check Text Name size
    int app_name_size = QFontMetrics(m_font_name).width(m_group_name)+30;
    if (app_name_size > p_width)
        p_width = app_name_size;

    // Get Port List
    QList<port_dict_t> port_list;
    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (m_port_list_ids.contains(port.port_id))
            port_list.append(port);
    }

    // Get Max Box Width/Height
    foreach (const port_dict_t& port, port_list)
    {
        if (port.port_mode == PORT_MODE_INPUT)
        {
            max_in_height += 18;

            int size = QFontMetrics(m_font_port).width(port.port_name);
            if (size > max_in_width)
                max_in_width = size;

            if (port.port_type == PORT_TYPE_AUDIO_JACK && have_audio_jack_in == false)
            {
                have_audio_jack_in = true;
                max_in_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_JACK && have_midi_jack_in == false)
            {
                have_midi_jack_in = true;
                max_in_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_A2J && have_midi_a2j_in == false)
            {
                have_midi_a2j_in = true;
                max_in_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_ALSA && have_midi_alsa_in == false)
            {
                have_midi_alsa_in = true;
                max_in_height += 2;
            }
        }
        else if (port.port_mode == PORT_MODE_OUTPUT)
        {
            max_out_height += 18;

            int size = QFontMetrics(m_font_port).width(port.port_name);
            if (size > max_out_width)
                max_out_width = size;

            if (port.port_type == PORT_TYPE_AUDIO_JACK && have_audio_jack_out == false)
            {
                have_audio_jack_out = true;
                max_out_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_JACK && have_midi_jack_out == false)
            {
                have_midi_jack_out = true;
                max_out_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_A2J && have_midi_a2j_out == false)
            {
                have_midi_a2j_out = true;
                max_out_height += 2;
            }
            else if (port.port_type == PORT_TYPE_MIDI_ALSA && have_midi_alsa_out == false)
            {
                have_midi_alsa_out = true;
                max_out_height += 2;
            }
        }
    }

    int final_width = 30 + max_in_width + max_out_width;
    if (final_width > p_width)
        p_width = final_width;

    if (max_in_height > p_height)
        p_height = max_in_height;

    if (max_out_height > p_height)
        p_height = max_out_height;

    // Remove bottom space
    p_height -= 2;

    int last_in_pos  = 24;
    int last_out_pos = 24;
    PortType last_in_type  = PORT_TYPE_NULL;
    PortType last_out_type = PORT_TYPE_NULL;

    // Re-position ports, AUDIO_JACK
    foreach (const port_dict_t& port, port_list)
    {
        if (port.port_type == PORT_TYPE_AUDIO_JACK)
        {
            if (port.port_mode == PORT_MODE_INPUT)
            {
                port.widget->setPos(QPointF(1, last_in_pos));
                port.widget->setPortWidth(max_in_width);

                last_in_pos += 18;
                last_in_type = port.port_type;
            }
            else if (port.port_mode == PORT_MODE_OUTPUT)
            {
                port.widget->setPos(QPointF(p_width-max_out_width-13, last_out_pos));
                port.widget->setPortWidth(max_out_width);

                last_out_pos += 18;
                last_out_type = port.port_type;
            }
        }
    }

    // Re-position ports, MIDI_JACK
    foreach (const port_dict_t& port, port_list)
    {
        if (port.port_type == PORT_TYPE_MIDI_JACK)
        {
            if (port.port_mode == PORT_MODE_INPUT)
            {
                if (last_in_type != PORT_TYPE_NULL && port.port_type != last_in_type)
                    last_in_pos += 2;

                port.widget->setPos(QPointF(1, last_in_pos));
                port.widget->setPortWidth(max_in_width);

                last_in_pos += 18;
                last_in_type = port.port_type;
            }
            else if (port.port_mode == PORT_MODE_OUTPUT)
            {
                if (last_out_type != PORT_TYPE_NULL && port.port_type != last_out_type)
                    last_out_pos += 2;

                port.widget->setPos(QPointF(p_width-max_out_width-13, last_out_pos));
                port.widget->setPortWidth(max_out_width);

                last_out_pos += 18;
                last_out_type = port.port_type;
            }
        }
    }

    // Re-position ports, MIDI_A2J
    foreach (const port_dict_t& port, port_list)
    {
        if (port.port_type == PORT_TYPE_MIDI_A2J)
        {
            if (port.port_mode == PORT_MODE_INPUT)
            {
                if (last_in_type != PORT_TYPE_NULL && port.port_type != last_in_type)
                    last_in_pos += 2;

                port.widget->setPos(QPointF(1, last_in_pos));
                port.widget->setPortWidth(max_in_width);

                last_in_pos += 18;
                last_in_type = port.port_type;
            }
            else if (port.port_mode == PORT_MODE_OUTPUT)
            {
                if (last_out_type != PORT_TYPE_NULL && port.port_type != last_out_type)
                    last_out_pos += 2;

                port.widget->setPos(QPointF(p_width-max_out_width-13, last_out_pos));
                port.widget->setPortWidth(max_out_width);

                last_out_pos += 18;
                last_out_type = port.port_type;
            }
        }
    }

    // Re-position ports, MIDI_ALSA
    foreach (const port_dict_t& port, port_list)
    {
        if (port.port_type == PORT_TYPE_MIDI_ALSA)
        {
            if (port.port_mode == PORT_MODE_INPUT)
            {
                if (last_in_type != PORT_TYPE_NULL && port.port_type != last_in_type)
                    last_in_pos += 2;

                port.widget->setPos(QPointF(1, last_in_pos));
                port.widget->setPortWidth(max_in_width);

                last_in_pos += 18;
                last_in_type = port.port_type;
            }
            else if (port.port_mode == PORT_MODE_OUTPUT)
            {
                if (last_out_type != PORT_TYPE_NULL && port.port_type != last_out_type)
                    last_out_pos += 2;

                port.widget->setPos(QPointF(p_width-max_out_width-13, last_out_pos));
                port.widget->setPortWidth(max_out_width);

                last_out_pos += 18;
                last_out_type = port.port_type;
            }
        }
    }

    repaintLines(true);
    update();
}

void CanvasBox::repaintLines(bool forced)
{
    if (pos() != m_last_pos || forced)
    {
        foreach (const cb_line_t& connection, m_connection_lines)
            connection.line->updateLinePos();
    }

    m_last_pos = pos();
}

void CanvasBox::resetLinesZValue()
{
    foreach (const connection_dict_t& connection, canvas.connection_list)
    {
        int z_value;
        if (m_port_list_ids.contains(connection.port_out_id) && m_port_list_ids.contains(connection.port_in_id))
            z_value = canvas.last_z_value;
        else
            z_value = canvas.last_z_value-1;

        connection.widget->setZValue(z_value);
    }
}

int CanvasBox::type() const
{
    return CanvasBoxType;
}

void CanvasBox::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    QMenu discMenu("Disconnect", &menu);

    QList<int> port_con_list;
    QList<int> port_con_list_ids;

    foreach (const int& port_id, m_port_list_ids)
    {
        QList<int> tmp_port_con_list = CanvasGetPortConnectionList(port_id);
        foreach (const int& port_con_id, tmp_port_con_list)
        {
            if (port_con_list.contains(port_con_id) == false)
            {
                port_con_list.append(port_con_id);
                port_con_list_ids.append(port_id);
            }
        }
    }

    if (port_con_list.count() > 0)
    {
        for (int i=0; i < port_con_list.count(); i++)
        {
            int port_con_id = CanvasGetConnectedPort(port_con_list[i], port_con_list_ids[i]);
            QAction* act_x_disc = discMenu.addAction(CanvasGetFullPortName(port_con_id));
            act_x_disc->setData(port_con_list[i]);
            QObject::connect(act_x_disc, SIGNAL(triggered()), canvas.qobject, SLOT(PortContextMenuDisconnect()));
        }
    }
    else
    {
        QAction* act_x_disc = discMenu.addAction("No connections");
        act_x_disc->setEnabled(false);
    }

    menu.addMenu(&discMenu);
    QAction* act_x_disc_all   = menu.addAction("Disconnect &All");
    QAction* act_x_sep1       = menu.addSeparator();
    QAction* act_x_info       = menu.addAction("&Info");
    QAction* act_x_rename     = menu.addAction("&Rename");
    QAction* act_x_sep2       = menu.addSeparator();
    QAction* act_x_split_join = menu.addAction(m_splitted ? "Join" : "Split");

    if (features.group_info == false)
        act_x_info->setVisible(false);

    if (features.group_rename == false)
        act_x_rename->setVisible(false);

    if (features.group_info == false && features.group_rename == false)
        act_x_sep1->setVisible(false);

    bool haveIns, haveOuts;
    haveIns = haveOuts = false;
    foreach (const port_dict_t& port, canvas.port_list)
    {
        if (m_port_list_ids.contains(port.port_id))
        {
            if (port.port_mode == PORT_MODE_INPUT)
                haveIns = true;
            else if (port.port_mode == PORT_MODE_OUTPUT)
                haveOuts = true;
        }
    }

    if (m_splitted == false && (haveIns && haveOuts) == false)
    {
        act_x_sep2->setVisible(false);
        act_x_split_join->setVisible(false);
    }

    QAction* act_selected = menu.exec(event->screenPos());

    if (act_selected == act_x_disc_all)
    {
        foreach (const int& port_id, port_con_list)
            canvas.callback(ACTION_PORTS_DISCONNECT, port_id, 0, "");
    }
    else if (act_selected == act_x_info)
    {
        canvas.callback(ACTION_GROUP_INFO, m_group_id, 0, "");
    }
    else if (act_selected == act_x_rename)
    {
        bool ok_check;
        QString new_name = QInputDialog::getText(0, "Rename Group", "New name:", QLineEdit::Normal, m_group_name, &ok_check);
        if (ok_check and !new_name.isEmpty())
        {
            canvas.callback(ACTION_GROUP_RENAME, m_group_id, 0, new_name);
        }
    }
    else if (act_selected == act_x_split_join)
    {
        if (m_splitted)
            canvas.callback(ACTION_GROUP_JOIN, m_group_id, 0, "");
        else
            canvas.callback(ACTION_GROUP_SPLIT, m_group_id, 0, "");

    }

    event->accept();
}

void CanvasBox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    canvas.last_z_value += 1;
    setZValue(canvas.last_z_value);
    resetLinesZValue();
    m_cursor_moving = false;

    if (event->button() == Qt::RightButton)
    {
        canvas.scene->clearSelection();
        setSelected(true);
        m_mouse_down = false;
        return event->accept();
    }
    else if (event->button() == Qt::LeftButton)
    {
        if (sceneBoundingRect().contains(event->scenePos()))
            m_mouse_down = true;
        else
        {
            // Fixes a weird Qt behaviour with right-click mouseMove
            m_mouse_down = false;
            return event->ignore();
        }
    }
    else
        m_mouse_down = false;

    QGraphicsItem::mousePressEvent(event);
}

void CanvasBox::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_mouse_down)
    {
        if (m_cursor_moving == false)
        {
            setCursor(QCursor(Qt::SizeAllCursor));
            m_cursor_moving = true;
        }
        repaintLines();
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void CanvasBox::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_cursor_moving)
        setCursor(QCursor(Qt::ArrowCursor));
    m_mouse_down = false;
    m_cursor_moving = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

QRectF CanvasBox::boundingRect() const
{
    return QRectF(0, 0, p_width, p_height);
}

void CanvasBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (isSelected())
        painter->setPen(canvas.theme->box_pen_sel);
    else
        painter->setPen(canvas.theme->box_pen);

    QLinearGradient box_gradient(0, 0, 0, p_height);
    box_gradient.setColorAt(0, canvas.theme->box_bg_1);
    box_gradient.setColorAt(1, canvas.theme->box_bg_2);

    painter->setBrush(box_gradient);
    painter->drawRect(0, 0, p_width, p_height);

    QPointF text_pos(25, 16);

    painter->setFont(m_font_name);
    painter->setPen(canvas.theme->box_text);
    painter->drawText(text_pos, m_group_name);

    repaintLines();
}

END_NAMESPACE_PATCHCANVAS

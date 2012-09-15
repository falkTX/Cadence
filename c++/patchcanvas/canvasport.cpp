/*
 * Patchbay Canvas engine using QGraphicsView/Scene
 * Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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

#include "canvasport.h"

#include <QtCore/QTimer>
#include <QtGui/QCursor>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>
#include <QtGui/QPainter>

#include "canvaslinemov.h"
#include "canvasbezierlinemov.h"
#include "canvasbox.h"

START_NAMESPACE_PATCHCANVAS

CanvasPort::CanvasPort(int port_id, QString port_name, PortMode port_mode, PortType port_type, QGraphicsItem* parent) :
        QGraphicsItem(parent, canvas.scene)
{
    // Save Variables, useful for later
    m_port_id   = port_id;
    m_port_mode = port_mode;
    m_port_type = port_type;
    m_port_name = port_name;

    // Base Variables
    m_port_width  = 15;
    m_port_height = 15;
    m_port_font   = QFont(canvas.theme->port_font_name, canvas.theme->port_font_size, canvas.theme->port_font_state);

    m_line_mov   = 0;
    m_hover_item = 0;
    m_last_selected_state = false;

    m_mouse_down    = false;
    m_cursor_moving = false;

    setFlags(QGraphicsItem::ItemIsSelectable);
}

int CanvasPort::getPortId()
{
    return m_port_id;
}

PortMode CanvasPort::getPortMode()
{
    return m_port_mode;
}

PortType CanvasPort::getPortType()
{
    return m_port_type;
}

QString CanvasPort::getPortName()
{
    return m_port_name;
}

QString CanvasPort::getFullPortName()
{
    return ((CanvasBox*)parentItem())->getGroupName()+":"+m_port_name;
}

int CanvasPort::getPortWidth()
{
    return m_port_width;
}

int CanvasPort::getPortHeight()
{
    return m_port_height;
}

void CanvasPort::setPortMode(PortMode port_mode)
{
    m_port_mode = port_mode;
    update();
}

void CanvasPort::setPortType(PortType port_type)
{
    m_port_type = port_type;
    update();
}

void CanvasPort::setPortName(QString port_name)
{
    if (QFontMetrics(m_port_font).width(port_name) < QFontMetrics(m_port_font).width(m_port_name))
        QTimer::singleShot(0, canvas.scene, SLOT(update()));

    m_port_name = port_name;
    update();
}

void CanvasPort::setPortWidth(int port_width)
{
    if (port_width < m_port_width)
        QTimer::singleShot(0, canvas.scene, SLOT(update()));

    m_port_width = port_width;
    update();
}

int CanvasPort::type() const
{
    return CanvasPortType;
}

void CanvasPort::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_hover_item = 0;
    m_mouse_down = (event->button() == Qt::LeftButton);
    m_cursor_moving = false;
    QGraphicsItem::mousePressEvent(event);
}

void CanvasPort::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_mouse_down)
    {
        if (m_cursor_moving == false)
        {
            setCursor(QCursor(Qt::CrossCursor));
            m_cursor_moving = true;

            foreach (const connection_dict_t& connection, canvas.connection_list)
            {
                if (connection.port_out_id == m_port_id || connection.port_in_id == m_port_id)
                    connection.widget->setLocked(true);
            }
        }

        if (! m_line_mov)
        {
            if (options.use_bezier_lines)
                m_line_mov = new CanvasBezierLineMov(m_port_mode, m_port_type, this);
            else
                m_line_mov = new CanvasLineMov(m_port_mode, m_port_type, this);

            canvas.last_z_value += 1;
            m_line_mov->setZValue(canvas.last_z_value);
            canvas.last_z_value += 1;
            parentItem()->setZValue(canvas.last_z_value);
        }

        CanvasPort* item = 0;
        QList<QGraphicsItem*> items = canvas.scene->items(event->scenePos(), Qt::ContainsItemShape, Qt::AscendingOrder);
        for (int i=0; i < items.count(); i++)
        {
            if (items[i]->type() == CanvasPortType)
            {
                if (items[i] != this)
                {
                    if (! item)
                        item = (CanvasPort*)items[i];
                    else if (items[i]->parentItem()->zValue() > item->parentItem()->zValue())
                        item = (CanvasPort*)items[i];
                }
            }
        }

        if (m_hover_item and m_hover_item != item)
            m_hover_item->setSelected(false);

        if (item)
        {
            bool a2j_connection = (item->getPortType() == PORT_TYPE_MIDI_JACK && m_port_type == PORT_TYPE_MIDI_A2J) || (item->getPortType() == PORT_TYPE_MIDI_A2J && m_port_type == PORT_TYPE_MIDI_JACK);
            if (item->getPortMode() != m_port_mode && (item->getPortType() == m_port_type || a2j_connection))
            {
                item->setSelected(true);
                m_hover_item = item;
            }
            else
                m_hover_item = 0;
        }
        else
            m_hover_item = 0;

        m_line_mov->updateLinePos(event->scenePos());
        return event->accept();
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void CanvasPort::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_mouse_down)
    {
        if (m_line_mov)
        {
            m_line_mov->deleteFromScene();
            m_line_mov = 0;
        }

        foreach (const connection_dict_t& connection, canvas.connection_list)
        {
            if (connection.port_out_id == m_port_id || connection.port_in_id == m_port_id)
                connection.widget->setLocked(false);
        }

        if (m_hover_item)
        {
            bool check = false;
            foreach (const connection_dict_t& connection, canvas.connection_list)
            {
                if ( (connection.port_out_id == m_port_id && connection.port_in_id == m_hover_item->getPortId()) ||
                     (connection.port_out_id == m_hover_item->getPortId() && connection.port_in_id == m_port_id) )
                {
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "");
                    check = true;
                    break;
                }
            }

            if (check == false)
            {
                if (m_port_mode == PORT_MODE_OUTPUT)
                    canvas.callback(ACTION_PORTS_CONNECT, m_port_id, m_hover_item->getPortId(), "");
                else
                    canvas.callback(ACTION_PORTS_CONNECT, m_hover_item->getPortId(), m_port_id, "");
            }

            canvas.scene->clearSelection();
        }
    }

    if (m_cursor_moving)
        setCursor(QCursor(Qt::ArrowCursor));

    m_hover_item = 0;
    m_mouse_down = false;
    m_cursor_moving = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

void CanvasPort::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    canvas.scene->clearSelection();
    setSelected(true);

    QMenu menu;
    QMenu discMenu("Disconnect", &menu);

    QList<int> port_con_list = CanvasGetPortConnectionList(m_port_id);

    if (port_con_list.count() > 0)
    {
        foreach (int port_id, port_con_list)
        {
            int port_con_id = CanvasGetConnectedPort(port_id, m_port_id);
            QAction* act_x_disc = discMenu.addAction(CanvasGetFullPortName(port_con_id));
            act_x_disc->setData(port_id);
            QObject::connect(act_x_disc, SIGNAL(triggered()), canvas.qobject, SLOT(PortContextMenuDisconnect()));
        }
    }
    else
    {
        QAction* act_x_disc = discMenu.addAction("No connections");
        act_x_disc->setEnabled(false);
    }

    menu.addMenu(&discMenu);
    QAction* act_x_disc_all = menu.addAction("Disconnect &All");
    QAction* act_x_sep_1    = menu.addSeparator();
    QAction* act_x_info     = menu.addAction("Get &Info");
    QAction* act_x_rename   = menu.addAction("&Rename");

    if (features.port_info == false)
        act_x_info->setVisible(false);

    if (features.port_rename == false)
        act_x_rename->setVisible(false);

    if (features.port_info == false && features.port_rename == false)
        act_x_sep_1->setVisible(false);

    QAction* act_selected = menu.exec(event->screenPos());

    if (act_selected == act_x_disc_all)
    {
        foreach (int port_id, port_con_list)
            canvas.callback(ACTION_PORTS_DISCONNECT, port_id, 0, "");
    }
    else if (act_selected == act_x_info)
    {
        canvas.callback(ACTION_PORT_INFO, m_port_id, 0, "");
    }
    else if (act_selected == act_x_rename)
    {
        bool ok_check;
        QString new_name = QInputDialog::getText(0, "Rename Port", "New name:", QLineEdit::Normal, m_port_name, &ok_check);
        if (ok_check and new_name.isEmpty() == false)
        {
            canvas.callback(ACTION_PORT_RENAME, m_port_id, 0, new_name);
        }
    }

    event->accept();
}

QRectF CanvasPort::boundingRect() const
{
    return QRectF(0, 0, m_port_width+12, m_port_height);
}

void CanvasPort::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing, (options.antialiasing == ANTIALIASING_FULL));

    QPointF text_pos;
    int poly_locx[5] = { 0 };

    if (m_port_mode == PORT_MODE_INPUT)
    {
        text_pos = QPointF(3, 12);

        if (canvas.theme->port_mode == Theme::THEME_PORT_POLYGON)
        {
            poly_locx[0] = 0;
            poly_locx[1] = m_port_width+5;
            poly_locx[2] = m_port_width+12;
            poly_locx[3] = m_port_width+5;
            poly_locx[4] = 0;
        }
        else if (canvas.theme->port_mode == Theme::THEME_PORT_SQUARE)
        {
            poly_locx[0] = 0;
            poly_locx[1] = m_port_width+5;
            poly_locx[2] = m_port_width+5;
            poly_locx[3] = m_port_width+5;
            poly_locx[4] = 0;
        }
        else
        {
            qCritical("PatchCanvas::CanvasPort->paint() - invalid theme port mode '%i'", canvas.theme->port_mode);
            return;
        }
    }
    else if (m_port_mode == PORT_MODE_OUTPUT)
    {
        text_pos = QPointF(9, 12);

        if (canvas.theme->port_mode == Theme::THEME_PORT_POLYGON)
        {
            poly_locx[0] = m_port_width+12;
            poly_locx[1] = 7;
            poly_locx[2] = 0;
            poly_locx[3] = 7;
            poly_locx[4] = m_port_width+12;
        }
        else if (canvas.theme->port_mode == Theme::THEME_PORT_SQUARE)
        {
            poly_locx[0] = m_port_width+12;
            poly_locx[1] = 5;
            poly_locx[2] = 5;
            poly_locx[3] = 5;
            poly_locx[4] = m_port_width+12;
        }
        else
        {
            qCritical("PatchCanvas::CanvasPort->paint() - invalid theme port mode '%i'", canvas.theme->port_mode);
            return;
        }
    }
    else
    {
        qCritical("PatchCanvas::CanvasPort->paint() - invalid port mode '%s'", port_mode2str(m_port_mode));
        return;
    }

    QColor poly_color;
    QPen poly_pen;

    if (m_port_type == PORT_TYPE_AUDIO_JACK)
    {
        poly_color = isSelected() ? canvas.theme->port_audio_jack_bg_sel : canvas.theme->port_audio_jack_bg;
        poly_pen = isSelected() ? canvas.theme->port_audio_jack_pen_sel : canvas.theme->port_audio_jack_pen;
    }
    else if (m_port_type == PORT_TYPE_MIDI_JACK)
    {
        poly_color = isSelected() ? canvas.theme->port_midi_jack_bg_sel : canvas.theme->port_midi_jack_bg;
        poly_pen = isSelected() ? canvas.theme->port_midi_jack_pen_sel : canvas.theme->port_midi_jack_pen;
    }
    else if (m_port_type == PORT_TYPE_MIDI_A2J)
    {
        poly_color = isSelected() ? canvas.theme->port_midi_a2j_bg_sel : canvas.theme->port_midi_a2j_bg;
        poly_pen = isSelected() ? canvas.theme->port_midi_a2j_pen_sel : canvas.theme->port_midi_a2j_pen;
    }
    else if (m_port_type == PORT_TYPE_MIDI_ALSA)
    {
        poly_color = isSelected() ? canvas.theme->port_midi_alsa_bg_sel : canvas.theme->port_midi_alsa_bg;
        poly_pen = isSelected() ? canvas.theme->port_midi_alsa_pen_sel : canvas.theme->port_midi_alsa_pen;
    }
    else
    {
        qCritical("PatchCanvas::CanvasPort->paint() - invalid port type '%s'", port_type2str(m_port_type));
        return;
    }

    QPolygonF polygon;
    polygon += QPointF(poly_locx[0], 0);
    polygon += QPointF(poly_locx[1], 0);
    polygon += QPointF(poly_locx[2], 7.5);
    polygon += QPointF(poly_locx[3], 15);
    polygon += QPointF(poly_locx[4], 15);

    painter->setBrush(poly_color);
    painter->setPen(poly_pen);
    painter->drawPolygon(polygon);

    painter->setPen(canvas.theme->port_text);
    painter->setFont(m_port_font);
    painter->drawText(text_pos, m_port_name);

    if (isSelected() != m_last_selected_state)
    {
        foreach (const connection_dict_t& connection, canvas.connection_list)
        {
            if (connection.port_out_id == m_port_id || connection.port_in_id == m_port_id)
                connection.widget->setLineSelected(isSelected());
        }
    }

    m_last_selected_state = isSelected();
}

END_NAMESPACE_PATCHCANVAS

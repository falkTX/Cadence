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

#ifndef CANVASBOX_H
#define CANVASBOX_H

#include "patchcanvas.h"

class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneMouseEvent;
class QPainter;

START_NAMESPACE_PATCHCANVAS

class AbstractCanvasLine;
class CanvasBoxShadow;
class CanvasPort;
class CanvasIcon;

struct cb_line_t {
    AbstractCanvasLine* line;
    int connection_id;
};

class CanvasBox : public QGraphicsItem
{
public:
    CanvasBox(int group_id, QString group_name, Icon icon, QGraphicsItem* parent=0);
    virtual ~CanvasBox();

    int getGroupId();
    QString getGroupName();
    bool isSplitted();
    PortMode getSplittedMode();

    int getPortCount();
    QList<int> getPortList();

    void setIcon(Icon icon);
    void setSplit(bool split, PortMode mode=PORT_MODE_NULL);
    void setGroupName(QString group_name);

    void setShadowOpacity(float opacity);

    CanvasPort* addPortFromGroup(int port_id, QString port_name, PortMode port_mode, PortType port_type);
    void removePortFromGroup(int port_id);
    void addLineFromGroup(AbstractCanvasLine* line, int connection_id);
    void removeLineFromGroup(int connection_id);

    void checkItemPos();
    void removeIconFromScene();

    void updatePositions();
    void repaintLines(bool forced=false);
    void resetLinesZValue();

    virtual int type() const;

private:
    int m_group_id;
    QString m_group_name;

    int p_width;
    int p_height;

    QList<int> m_port_list_ids;
    QList<cb_line_t> m_connection_lines;

    QPointF m_last_pos;
    bool m_splitted;
    PortMode m_splitted_mode;

    bool m_forced_split;
    bool m_cursor_moving;
    bool m_mouse_down;

    QFont m_font_name;
    QFont m_font_port;

    CanvasIcon* icon_svg;
    CanvasBoxShadow* shadow;

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASBOX_H

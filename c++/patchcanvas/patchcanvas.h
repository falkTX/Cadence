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

#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include <QtGui/QGraphicsItem>

#include "patchcanvas-api.h"

#define foreach2(var, list) \
    for (int i=0; i < list.count(); i++) { var = list[i];

class QSettings;
class QTimer;

class CanvasObject : public QObject {
    Q_OBJECT

public:
    CanvasObject(QObject* parent=0);

public slots:
    void AnimationIdle();
    void AnimationHide();
    void AnimationDestroy();
    void CanvasPostponedGroups();
    void PortContextMenuDisconnect();
};

START_NAMESPACE_PATCHCANVAS

class AbstractCanvasLine;
class CanvasFadeAnimation;
class CanvasBox;
class CanvasPort;
class Theme;

// object types
enum CanvasType {
    CanvasBoxType           = QGraphicsItem::UserType + 1,
    CanvasIconType          = QGraphicsItem::UserType + 2,
    CanvasPortType          = QGraphicsItem::UserType + 3,
    CanvasLineType          = QGraphicsItem::UserType + 4,
    CanvasBezierLineType    = QGraphicsItem::UserType + 5,
    CanvasLineMovType       = QGraphicsItem::UserType + 6,
    CanvasBezierLineMovType = QGraphicsItem::UserType + 7
};

// object lists
struct group_dict_t {
    int group_id;
    QString group_name;
    bool split;
    Icon icon;
    CanvasBox* widgets[2];
};

struct port_dict_t {
    int group_id;
    int port_id;
    QString port_name;
    PortMode port_mode;
    PortType port_type;
    CanvasPort* widget;
};

struct connection_dict_t {
    int connection_id;
    int port_in_id;
    int port_out_id;
    AbstractCanvasLine* widget;
};

struct animation_dict_t {
    CanvasFadeAnimation* animation;
    QGraphicsItem* item;
};

// Main Canvas object
class Canvas {
public:
    Canvas();
    ~Canvas();

    PatchScene* scene;
    Callback callback;
    bool debug;
    unsigned long last_z_value;
    int last_connection_id;
    QPointF initial_pos;
    QRectF size_rect;
    QList<group_dict_t> group_list;
    QList<port_dict_t> port_list;
    QList<connection_dict_t> connection_list;
    QList<animation_dict_t> animation_list;
    CanvasObject* qobject;
    QSettings* settings;
    Theme* theme;
    bool initiated;
};

const char* bool2str(bool check);
const char* port_mode2str(PortMode port_mode);
const char* port_type2str(PortType port_type);
const char* icon2str(Icon icon);
const char* split2str(SplitOption split);

QString CanvasGetGroupName(int group_id);
int CanvasGetGroupPortCount(int group_id);
QPointF CanvasGetNewGroupPos(bool horizontal=false);
QString CanvasGetFullPortName(int port_id);
QList<int> CanvasGetPortConnectionList(int port_id);
int CanvasGetConnectedPort(int connection_id, int port_id);
void CanvasRemoveAnimation(CanvasFadeAnimation* f_animation);
void CanvasPostponedGroups();
void CanvasCallback(CallbackAction action, int value1, int value2, QString value_str);
void CanvasItemFX(QGraphicsItem* item, bool show, bool destroy=false);
void CanvasRemoveItemFX(QGraphicsItem* item);

// global objects
extern Canvas canvas;
extern options_t options;
extern features_t features;

END_NAMESPACE_PATCHCANVAS

#endif // PATCHCANVAS_H

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

#ifndef PATCHSCENE_H
#define PATCHSCENE_H

#include <QtGui/QGraphicsScene>

class QKeyEvent;
class QGraphicsRectItem;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QGraphicsView;

class PatchScene : public QGraphicsScene
{
    Q_OBJECT

public:
    PatchScene(QObject* parent, QGraphicsView* view);

    void fixScaleFactor();
    void updateTheme();

    void zoom_fit();
    void zoom_in();
    void zoom_out();
    void zoom_reset();

signals:
    void scaleChanged(double);
    void sceneGroupMoved(int, int, QPointF);

private:
    bool m_ctrl_down;
    bool m_mouse_down_init;
    bool m_mouse_rubberband;

    QGraphicsRectItem* m_rubberband;
    bool m_rubberband_selection;
    QPointF m_rubberband_orig_point;

    QGraphicsView* m_view;

    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    virtual void wheelEvent(QGraphicsSceneWheelEvent* event);
};

#endif // PATCHSCENE_H

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

#include "patchscene.h"

#include <cmath>
#include <QtGui/QKeyEvent>
#include <QtGui/QGraphicsRectItem>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsSceneWheelEvent>
#include <QtGui/QGraphicsView>

#include "patchcanvas/patchcanvas.h"
#include "patchcanvas/canvasbox.h"

using namespace PatchCanvas;

PatchScene::PatchScene(QObject* parent, QGraphicsView* view) :
        QGraphicsScene(parent)
{
    m_ctrl_down  = false;
    m_mouse_down_init  = false;
    m_mouse_rubberband = false;

    m_rubberband = addRect(QRectF(0, 0, 0, 0));
    m_rubberband->setZValue(-1);
    m_rubberband->hide();
    m_rubberband_selection  = false;
    m_rubberband_orig_point = QPointF(0, 0);

    m_view = view;
    if (! m_view)
        qFatal("PatchCanvas::PatchScene() - invalid view");
}

void PatchScene::fixScaleFactor()
{
    qreal scale = m_view->transform().m11();
    if (scale > 3.0)
    {
      m_view->resetTransform();
      m_view->scale(3.0, 3.0);
    }
    else if (scale < 0.2)
    {
      m_view->resetTransform();
      m_view->scale(0.2, 0.2);
    }
    emit scaleChanged(m_view->transform().m11());
}

void PatchScene::updateTheme()
{
    setBackgroundBrush(canvas.theme->canvas_bg);
    m_rubberband->setPen(canvas.theme->rubberband_pen);
    m_rubberband->setBrush(canvas.theme->rubberband_brush);
}

void PatchScene::zoom_fit()
{
    qreal min_x, min_y, max_x, max_y;
    bool first_value = true;

    QList<QGraphicsItem*> items_list = items();

    if (items_list.count() > 0)
    {
        foreach (const QGraphicsItem* item, items_list)
        {
            if (item && item->isVisible() and item->type() == CanvasBoxType)
            {
                QPointF pos = item->scenePos();
                QRectF rect = item->boundingRect();

                if (first_value)
                    min_x = pos.x();
                else if (pos.x() < min_x)
                    min_x = pos.x();

                if (first_value)
                    min_y = pos.y();
                else if (pos.y() < min_y)
                    min_y = pos.y();

                if (first_value)
                    max_x = pos.x()+rect.width();
                else if (pos.x()+rect.width() > max_x)
                    max_x = pos.x()+rect.width();

                if (first_value)
                    max_y = pos.y()+rect.height();
                else if (pos.y()+rect.height() > max_y)
                    max_y = pos.y()+rect.height();

                first_value = false;
            }
        }

        if (first_value == false)
        {
            m_view->fitInView(min_x, min_y, abs(max_x-min_x), abs(max_y-min_y), Qt::KeepAspectRatio);
            fixScaleFactor();
        }
    }
}

void PatchScene::zoom_in()
{
    if (m_view->transform().m11() < 3.0)
        m_view->scale(1.2, 1.2);
    emit scaleChanged(m_view->transform().m11());
}

void PatchScene::zoom_out()
{
    if (m_view->transform().m11() > 0.2)
        m_view->scale(0.8, 0.8);
    emit scaleChanged(m_view->transform().m11());
}

void PatchScene::zoom_reset()
{
    m_view->resetTransform();
    emit scaleChanged(1.0);
}

void PatchScene::keyPressEvent(QKeyEvent* event)
{
    if (! m_view)
        return event->ignore();

    if (event->key() == Qt::Key_Control)
    {
        m_ctrl_down = true;
    }
    else if (event->key() == Qt::Key_Home)
    {
        zoom_fit();
        return event->accept();
    }
    else if (m_ctrl_down)
    {
        if (event->key() == Qt::Key_Plus)
        {
            zoom_in();
            return event->accept();
        }
        else if (event->key() == Qt::Key_Minus)
        {
            zoom_out();
            return event->accept();
        }
        else if (event->key() == Qt::Key_1)
        {
            zoom_reset();
            return event->accept();
        }
    }

    QGraphicsScene::keyPressEvent(event);
}

void PatchScene::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
        m_ctrl_down = false;
    QGraphicsScene::keyReleaseEvent(event);
}

void PatchScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_mouse_down_init  = (event->button() == Qt::LeftButton);
    m_mouse_rubberband = false;
    QGraphicsScene::mousePressEvent(event);
}

void PatchScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_mouse_down_init)
    {
        m_mouse_down_init  = false;
        m_mouse_rubberband = (selectedItems().count() == 0);
    }

    if (m_mouse_rubberband)
    {
        if (m_rubberband_selection == false)
        {
            m_rubberband->show();
            m_rubberband_selection  = true;
            m_rubberband_orig_point = event->scenePos();
        }

        int x, y;
        QPointF pos = event->scenePos();

        if (pos.x() > m_rubberband_orig_point.x())
            x = m_rubberband_orig_point.x();
        else
            x = pos.x();

        if (pos.y() > m_rubberband_orig_point.y())
            y = m_rubberband_orig_point.y();
        else
            y = pos.y();

        m_rubberband->setRect(x, y, abs(pos.x()-m_rubberband_orig_point.x()), abs(pos.y()-m_rubberband_orig_point.y()));
        return event->accept();
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void PatchScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_rubberband_selection)
    {
        QList<QGraphicsItem*> items_list = items();
        if (items_list.count() > 0)
        {
            foreach (QGraphicsItem* item, items_list)
            {
                if (item && item->isVisible() && item->type() == CanvasBoxType)
                {
                    QRectF item_rect = item->sceneBoundingRect();
                    QPointF item_top_left     = QPointF(item_rect.x(), item_rect.y());
                    QPointF item_bottom_right = QPointF(item_rect.x()+item_rect.width(), item_rect.y()+item_rect.height());

                    if (m_rubberband->contains(item_top_left) && m_rubberband->contains(item_bottom_right))
                        item->setSelected(true);
                }
            }

            m_rubberband->hide();
            m_rubberband->setRect(0, 0, 0, 0);
            m_rubberband_selection = false;
        }
    }
    else
    {
        QList<QGraphicsItem*> items_list = selectedItems();
        foreach (QGraphicsItem* item, items_list)
        {
            if (item && item->isVisible() && item->type() == CanvasBoxType)
            {
                CanvasBox* citem = (CanvasBox*)item;
                citem->checkItemPos();
                emit sceneGroupMoved(citem->getGroupId(), citem->getSplittedMode(), citem->scenePos());
            }
        }

        if (items_list.count() > 1)
            canvas.scene->update();
    }

    m_mouse_down_init  = false;
    m_mouse_rubberband = false;
    QGraphicsScene::mouseReleaseEvent(event);
}

void PatchScene::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (! m_view)
        return event->ignore();

    if (m_ctrl_down)
    {
        double factor = std::pow(1.41, (event->delta()/240.0));
        m_view->scale(factor, factor);

        fixScaleFactor();
        return event->accept();
    }

    QGraphicsScene::wheelEvent(event);
}

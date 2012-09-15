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

#ifndef CANVASPORTGLOW_H
#define CANVASPORTGLOW_H

#include <QGraphicsDropShadowEffect>

#include "patchcanvas.h"

START_NAMESPACE_PATCHCANVAS

class CanvasPortGlow : public QGraphicsDropShadowEffect
{
public:
    CanvasPortGlow(PortType port_type, QObject* parent);
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASPORTGLOW_H

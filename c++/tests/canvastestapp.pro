#-------------------------------------------------
#
# Project created by QtCreator 2011-10-17T21:44:58
#
#-------------------------------------------------

QT       = core gui svg opengl
TEMPLATE = app
CONFIG  += debug

TARGET = CanvasTestApp

SOURCES = \
    canvastestapp.cpp \
    ../patchcanvas.cpp

HEADERS = \
    canvastestapp.h \
    ../patchcanvas.h \
    ../patchcanvas/patchcanvas.h \
    ../patchcanvas/patchscene.h

INCLUDEPATH = ..

FORMS = canvastestapp.ui

LIBS = -ljack

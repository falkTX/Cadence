#-------------------------------------------------
#
# Project created by QtCreator 2011-10-17T21:44:58
#
#-------------------------------------------------

QT       = core gui svg opengl
TEMPLATE = app
CONFIG  += debug

TARGET = PatchCanvas

SOURCES = main.cpp canvastestapp.cpp \
    patchcanvas.cpp \
    patchcanvas-theme.cpp \
    patchscene.cpp \
    canvasfadeanimation.cpp \
    canvasline.cpp \
    canvasbezierline.cpp \
    canvaslinemov.cpp \
    canvasbezierlinemov.cpp \
    canvasport.cpp \
    canvasbox.cpp \
    canvasicon.cpp \
    canvasboxshadow.cpp \
    canvasportglow.cpp

HEADERS = canvastestapp.h \
    patchcanvas.h \
    patchcanvas-api.h \
    patchcanvas-theme.h \
    patchscene.h \
    canvasfadeanimation.h \
    abstractcanvasline.h \
    canvasline.h \
    canvasbezierline.h \
    canvaslinemov.h \
    canvasbezierlinemov.h \
    canvasport.h \
    canvasbox.h \
    canvasicon.h \
    canvasboxshadow.h \
    canvasportglow.h

FORMS = canvastestapp.ui

LIBS = -ljack

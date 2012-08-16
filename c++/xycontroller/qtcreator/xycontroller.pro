# QtCreator project file

QT = core gui

CONFIG    = debug link_pkgconfig qt resources uic warn_on
PKGCONFIG = jack

TARGET   = xycontroller
TEMPLATE = app
VERSION  = 0.5.0

DEFINES  = HAVE_JACKSESSION

SOURCES  = \
    ../xycontroller.cpp \
    ../../widgets/pixmapdial.cpp \
    ../../widgets/pixmapkeyboard.cpp

HEADERS  = \
    ../../jack_utils.h \
    ../../widgets/pixmapdial.h \
    ../../widgets/pixmapkeyboard.h

FORMS = \
    ../../../src/ui/xycontroller.ui

RESOURCES = \
    ../../../resources/resources.qrc

INCLUDEPATH = \
    ../../widgets

QMAKE_CXXFLAGS *= -std=c++0x

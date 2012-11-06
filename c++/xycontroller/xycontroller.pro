# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += link_pkgconfig qt resources uic warn_on

DEFINES   = DEBUG
DEFINES  += HAVE_JACKSESSION
PKGCONFIG = jack

TARGET   = cadence-xycontroller
TEMPLATE = app
VERSION  = 0.5.0

SOURCES  = \
    xycontroller.cpp \
    ../widgets/pixmapdial.cpp \
    ../widgets/pixmapkeyboard.cpp

HEADERS  = \
    ../jack_utils.hpp \
    ../midi_queue.hpp \
    ../widgets/pixmapdial.hpp \
    ../widgets/pixmapkeyboard.hpp

INCLUDEPATH = \
    ../widgets

FORMS = \
    ../../resources/ui/xycontroller.ui

RESOURCES = \
    ../../resources/resources-xycontroller.qrc

QMAKE_CXXFLAGS *= -std=c++0x

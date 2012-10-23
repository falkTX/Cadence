# QtCreator project file

QT = core gui

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = liblo

TARGET   = carla-bridge-vst-x11
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_osc.cpp \
    ../carla_bridge_ui-vst.cpp \
    ../carla_bridge_toolkit-qt4.cpp

HEADERS = \
    ../carla_bridge.h \
    ../carla_bridge_client.h \
    ../carla_bridge_osc.h \
    ../carla_bridge_toolkit.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_vst.h \
    ../../carla-includes/carla_midi.h

INCLUDEPATH = .. \
    ../../carla-includes

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG
DEFINES += BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_VST BRIDGE_VST_X11

QMAKE_CXXFLAGS *= -std=c++0x

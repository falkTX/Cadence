# QtCreator project file

QT = core gui

CONFIG    = warn_on qt link_pkgconfig debug
PKGCONFIG = liblo

TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_osc.cpp \
    ../carla_bridge_ui-lv2.cpp \
    ../carla_bridge_ui-qt4.cpp

HEADERS = \
    ../carla_bridge_osc.h \
    ../carla_bridge_ui.h \
    ../../carla/carla_midi.h \
    ../../carla/lv2_rdf.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_osc_includes.h

INCLUDEPATH = .. \
    ../../carla \
    ../../carla-includes

TARGET  = carla-bridge-lv2-x11

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2_X11

LIBS    = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

# QtCreator project file

QT = core gui

CONFIG    = debug link_pkgconfig qt warn_on
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
    ../../carla-backend/carla_midi.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-backend \
    ../../carla-includes

TARGET  = carla-bridge-lv2-qt4

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2_QT4

LIBS    = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

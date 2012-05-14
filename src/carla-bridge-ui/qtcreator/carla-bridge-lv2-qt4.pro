# QtCreator project file

QT = core gui

CONFIG   += warn_on qt debug link_pkgconfig
PKGCONFIG = liblo

TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_lv2.cpp \
    ../carla_bridge_qt4.cpp \
    ../../carla-bridge/carla_bridge_osc.cpp

HEADERS = \
    ../carla_bridge_ui.h \
    ../../carla/carla_midi.h \
    ../../carla/lv2_rdf.h \
    ../../carla-bridge/carla_bridge_osc.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_osc_includes.h

INCLUDEPATH = .. \
    ../../carla \
    ../../carla-bridge \
    ../../carla-includes

TARGET  = carla-bridge-lv2-qt4

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2_QT4

LIBS    = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

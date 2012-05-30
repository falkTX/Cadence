# QtCreator project file

QT = core gui

CONFIG   += warn_on qt debug link_pkgconfig
PKGCONFIG = liblo

TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge_vst.cpp \
    ../carla_bridge_qt4.cpp \
    ../../carla-bridge/carla_bridge_osc.cpp

HEADERS = \
    ../carla_bridge_ui.h \
    ../../carla/carla_midi.h \
    ../../carla/lv2_rdf.h \
    ../../carla-bridge/carla_bridge_osc.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_vst_includes.h

INCLUDEPATH = .. \
    ../../carla \
    ../../carla-bridge \
    ../../carla-includes

TARGET  = carla-bridge-vst-x11

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI

LIBS    = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

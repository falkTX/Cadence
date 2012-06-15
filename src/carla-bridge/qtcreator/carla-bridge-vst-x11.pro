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
    ../carla_bridge_ui-qt4.cpp

HEADERS = \
    ../carla_bridge.h \
    ../carla_bridge_osc.h \
    ../../carla-backend/carla_midi.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_vst_includes.h

INCLUDEPATH = .. \
    ../../carla-backend \
    ../../carla-includes \
    ../../carla-includes/vst

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_VST_X11

LIBS    = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

# QtCreator project file

QT = core gui

CONFIG   += warn_on qt debug link_pkgconfig
PKGCONFIG = jack liblo fluidsynth

TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_bridge.cpp \
    ../carla_osc.cpp \
    ../../carla/carla_jack.cpp \
    ../../carla/carla_shared.cpp \
    ../../carla/ladspa.cpp \
    ../../carla/dssi.cpp \
    ../../carla/lv2.cpp \
    ../../carla/vst.cpp \
    ../../carla/lv2-rtmempool/rtmempool.c

HEADERS = \
    ../carla_osc.h \
    ../../carla/carla_backend.h \
    ../../carla/carla_jack.h \
    ../../carla/carla_plugin.h \
    ../../carla/carla_shared.h \
    ../../carla-includes/carla_includes.h

INCLUDEPATH = .. \
    ../../carla-includes \
#    ../../carla-includes/vestige \
    ../../carla-includes/vst \
    ../../carla

TARGET  = carla-bridge-qtcreator

DEFINES = BUILD_BRIDGE

LIBS    = ../../carla-lilv/carla_lilv.a

QMAKE_CXXFLAGS *= -std=c++0x

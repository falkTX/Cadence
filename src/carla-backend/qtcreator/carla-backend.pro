# QtCreator project file

QT = core gui

CONFIG   += warn_on qt debug shared dll plugin link_pkgconfig
PKGCONFIG = jack liblo fluidsynth

TEMPLATE = app #lib
VERSION  = 0.5.0

SOURCES = \
    ../carla_backend.cpp \
    ../carla_bridge.cpp \
    ../carla_jack.cpp \
    ../carla_osc.cpp \
    ../carla_shared.cpp \
    ../carla_threads.cpp \
    ../ladspa.cpp \
    ../dssi.cpp \
    ../lv2.cpp \
    ../vst.cpp \
    ../sf2.cpp \
    ../lv2-rtmempool/rtmempool.c

HEADERS = \
    ../carla_backend.h \
    ../carla_jack.h \
    ../carla_midi.h \
    ../carla_osc.h \
    ../carla_plugin.h \
    ../carla_shared.h \
    ../carla_threads.h \
    ../ladspa_rdf.h \
    ../lv2_rdf.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_osc_includes.h

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-includes/vst
#    ../../carla-includes/vestige

TARGET  = carla_backend

#DEFINES = VESTIGE_HEADER

LIBS    = ../../carla-lilv/carla_lilv.a

QMAKE_CXXFLAGS *= -std=c++0x

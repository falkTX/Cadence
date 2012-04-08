# QtCreator project file

CONFIG = warn_on qt release shared dll plugin

TEMPLATE = lib
VERSION = 0.5.0

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
    ../sf2.cpp

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
    ../../carla-includes/carla_includes.h

TARGET = carla_backend

INCLUDEPATH = \
    ../../carla-includes
#   ../../carla-includes/vestige

LIBS += -ljack -llo

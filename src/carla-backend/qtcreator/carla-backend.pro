# QtCreator project file

QT = core gui

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = jack liblo fluidsynth linuxsampler

TARGET   = carla_backend
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_backend.cpp \
    ../carla_engine_jack.cpp \
    ../carla_engine_rtaudio.cpp \
    ../carla_bridge.cpp \
    ../carla_osc.cpp \
    ../carla_shared.cpp \
    ../carla_threads.cpp \
    ../ladspa.cpp \
    ../dssi.cpp \
    ../lv2.cpp \
    ../vst.cpp \
    ../fluidsynth.cpp \
    ../linuxsampler.cpp \
    ../lv2-rtmempool/rtmempool.c

HEADERS = \
    ../carla_backend.h \
    ../carla_engine.h \
    ../carla_midi.h \
    ../carla_osc.h \
    ../carla_plugin.h \
    ../carla_shared.h \
    ../carla_threads.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_vst_includes.h \
    ../../carla-includes/ladspa_rdf.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-includes/vst

DEFINES = QTCREATOR_TEST __LINUX_JACK__ CARLA_ENGINE_JACK
LIBS    = ../../carla-lilv/carla_lilv.a -ldl #-lrtaudio

QMAKE_CXXFLAGS *= -std=c++0x

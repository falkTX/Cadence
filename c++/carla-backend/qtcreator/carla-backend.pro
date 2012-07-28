# QtCreator project file

QT = core gui

CONFIG     = debug link_pkgconfig qt warn_on #plugin shared
PKGCONFIG  = liblo
PKGCONFIG += jack
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += suil-0

TARGET   = carla_backend
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla_backend_standalone.cpp \
    ../carla_bridge.cpp \
    ../carla_engine.cpp \
    ../carla_engine_jack.cpp \
    ../carla_engine_rtaudio.cpp \
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
    ../carla_backend_standalone.h \
    ../carla_engine.h \
    ../carla_osc.h \
    ../carla_plugin.h \
    ../carla_shared.h \
    ../carla_threads.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_ladspa.h \
    ../../carla-includes/carla_dssi.h \
    ../../carla-includes/carla_lv2.h \
    ../../carla-includes/carla_vst.h \
    ../../carla-includes/carla_fluidsynth.h \
    ../../carla-includes/carla_linuxsampler.h \
    ../../carla-includes/carla_midi.h \
    ../../carla-includes/ladspa_rdf.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-includes

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG # NDEBUG
DEFINES += CARLA_ENGINE_JACK
DEFINES += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY __LINUX_ALSA__ __LINUX_ALSASEQ__ __LINUX_PULSE__ __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__
DEFINES += HAVE_SUIL
DEFINES += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
LIBS    = ../../carla-lilv/carla_lilv.a -ldl

INCLUDEPATH += ../rtaudio-4.0.11
INCLUDEPATH += ../rtmidi-2.0.0
SOURCES += ../rtaudio-4.0.11/RtAudio.cpp
SOURCES += ../rtmidi-2.0.0/RtMidi.cpp

QMAKE_CXXFLAGS *= -fPIC -std=c++0x

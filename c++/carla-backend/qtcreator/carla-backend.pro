# QtCreator project file

QT = core gui

CONFIG     = debug link_pkgconfig qt warn_on plugin shared
PKGCONFIG  = liblo
PKGCONFIG += jack
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += fftw3 mxml
PKGCONFIG += suil-0

TARGET   = carla_backend
TEMPLATE = app
VERSION  = 0.5.0

SOURCES  = \
    ../carla_backend_plugin.cpp \
    ../carla_backend_standalone.cpp \
    ../carla_bridge.cpp \
    ../carla_engine.cpp \
    ../carla_engine_jack.cpp \
    ../carla_engine_rtaudio.cpp \
    ../carla_native.cpp \
    ../carla_osc.cpp \
    ../carla_shared.cpp \
    ../carla_threads.cpp \
    ../ladspa.cpp \
    ../dssi.cpp \
    ../lv2.cpp \
    ../vst.cpp \
    ../fluidsynth.cpp \
    ../linuxsampler.cpp

SOURCES += \
    ../plugins/bypass.c \
    ../plugins/midi-split.cpp \
    ../plugins/zynaddsubfx.cpp \
    ../plugins/zynaddsubfx-src.cpp

HEADERS = \
    ../carla_backend.h \
    ../carla_backend_standalone.h \
    ../carla_engine.h \
    ../carla_osc.h \
    ../carla_plugin.h \
    ../carla_shared.h \
    ../carla_threads.h \
    ../plugins/carla_native.h \
    ../plugins/carla_nativemm.h \
    ../../carla-includes/carla_defines.hpp \
    ../../carla-includes/carla_midi.h \
    ../../carla-includes/ladspa_rdf.hpp \
    ../../carla-includes/lv2_rdf.hpp \
    ../../carla-utils/carla_utils.hpp \
    ../../carla-utils/carla_lib_utils.hpp \
    ../../carla-utils/carla_osc_utils.hpp \
    ../../carla-utils/carla_ladspa_utils.hpp \
    ../../carla-utils/carla_lv2_utils.hpp \
    ../../carla-utils/carla_vst_utils.hpp \
    ../../carla-utils/carla_linuxsampler_utils.hpp

INCLUDEPATH = .. \
    ../../carla-jackbridge \
    ../../carla-includes \
    ../../carla-utils \
    ../distrho-plugin-toolkit

LIBS     =  -ldl \
    ../../carla-lilv/carla_lilv.a \
    ../../carla-rtmempool/carla_rtmempool.a

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG NDEBUG
DEFINES += CARLA_ENGINE_JACK
DEFINES += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY __LINUX_ALSA__ __LINUX_ALSASEQ__ __LINUX_PULSE__ __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__
DEFINES += CARLA_ENGINE_LV2
DEFINES += CARLA_ENGINE_VST
DEFINES += DISTRHO_PLUGIN_TARGET_DSSI
DEFINES += HAVE_SUIL
DEFINES += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
DEFINES += WANT_ZYNADDSUBFX

#LIBS    += -L../../carla-jackbridge -lcarla-jackbridge-native

INCLUDEPATH += ../rtaudio-4.0.11
INCLUDEPATH += ../rtmidi-2.0.1
SOURCES += ../rtaudio-4.0.11/RtAudio.cpp
SOURCES += ../rtmidi-2.0.1/RtMidi.cpp

QMAKE_CFLAGS   *= -fPIC -std=c99
QMAKE_CXXFLAGS *= -fPIC -std=c++0x

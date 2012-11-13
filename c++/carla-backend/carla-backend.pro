# QtCreator project file

QT = core gui

CONFIG     = debug
CONFIG    += link_pkgconfig qt warn_on # plugin shared

DEFINES    = DEBUG
DEFINES   += QTCREATOR_TEST

DEFINES   += CARLA_ENGINE_RTAUDIO
DEFINES   += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES   += WANT_FLUIDSYNTH WANT_LINUXSAMPLER

PKGCONFIG  = liblo
PKGCONFIG += jack
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += suil-0
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += fftw3 mxml

TARGET   = carla_backend
TEMPLATE = app # lib
VERSION  = 0.5.0

SOURCES  = \
     carla_backend_standalone.cpp

HEADERS = \
    carla_backend.hpp \
    carla_backend_utils.hpp \
    carla_backend_standalone.hpp

INCLUDEPATH = . \
    ../carla-engine \
    ../carla-includes \
    ../carla-native \
    ../carla-plugin \
    ../carla-utils

LIBS = \
    ../carla-engine/carla_engine.a \
    ../carla-plugin/carla_plugin.a \
    ../carla-native/carla_native.a

LIBS += \
    ../carla-lilv/carla_lilv.a \
    ../carla-rtmempool/carla_rtmempool.a

QMAKE_CXXFLAGS *= -std=c++0x

# NTK
QMAKE_CXXFLAGS *= `ntk-config --cxxflags`
QMAKE_LFLAGS *= `ntk-config --ldflags`
LIBS += `ntk-config --libs`

unix {
LIBS += -ldl -lm
}

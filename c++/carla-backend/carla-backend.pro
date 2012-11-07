# QtCreator project file

QT = core gui

CONFIG     = debug
CONFIG    += link_pkgconfig qt warn_on # plugin shared

DEFINES    = DEBUG
DEFINES   += QTCREATOR_TEST

PKGCONFIG  = liblo
PKGCONFIG += jack
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += suil-0
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += fftw3 mxml ntk

TARGET   = carla_backend
TEMPLATE = app # lib
VERSION  = 0.5.0

SOURCES  = \
     carla_backend_standalone.cpp

HEADERS = \
    carla_backend.hpp \
    carla_backend_standalone.hpp

INCLUDEPATH = . \
    ../carla-engine \
    ../carla-includes \
    ../carla-plugin \
    ../carla-utils

LIBS = \
    ../carla-engine/carla_engine.a \
    ../carla-native/carla_native.a \
    ../carla-plugin/carla_plugin.a

LIBS = \
    ../carla-lilv/carla_lilv.a \
    ../carla-rtmempool/carla_rtmempool.a

QMAKE_CXXFLAGS *= -std=c++0x

unix {
LIBS += -ldl -lm
}

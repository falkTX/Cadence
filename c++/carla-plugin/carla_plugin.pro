# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += static
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES  += WANT_ZYNADDSUBFX
DEFINES  += WANT_SUIL
DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER

PKGCONFIG = liblo suil-0 fluidsynth linuxsampler

TARGET   = carla_plugin
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES = \
     carla_bridge.cpp \
     native.cpp \
     ladspa.cpp \
     dssi.cpp \
     lv2.cpp \
     vst.cpp \
     fluidsynth.cpp \
     linuxsampler.cpp

HEADERS = \
    carla_plugin.hpp

INCLUDEPATH = . \
    ../carla-backend \
    ../carla-engine \
    ../carla-includes \
    ../carla-native \
    ../carla-utils

QMAKE_CXXFLAGS *= -std=c++0x

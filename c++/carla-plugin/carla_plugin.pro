# QtCreator project file

QT = core gui

#CONFIG    = debug
CONFIG   += link_pkgconfig qt warn_on
CONFIG   += dll shared

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# Plugins
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST

# Samplers
DEFINES  += WANT_FLUIDSYNTH
# WANT_LINUXSAMPLER

# ZynAddSubFX
DEFINES  += WANT_ZYNADDSUBFX

# Misc
#DEFINES  += WANT_SUIL

PKGCONFIG = liblo fluidsynth
#suil-0 linuxsampler

TARGET   = carla_plugin
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES = \
    carla_plugin.cpp \
    carla_plugin_thread.cpp \
    carla_bridge.cpp \
    native.cpp \
    ladspa.cpp \
    dssi.cpp \
    lv2.cpp \
    vst.cpp \
    fluidsynth.cpp \
    linuxsampler.cpp

HEADERS = \
    carla_plugin.hpp \
    carla_plugin_thread.hpp

INCLUDEPATH = . \
    ../carla-backend \
    ../carla-engine \
    ../carla-includes \
    ../carla-native \
    ../carla-utils

QMAKE_CXXFLAGS *= -std=c++0x

# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += static
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
PKGCONFIG = fftw3 mxml

TARGET   = carla_native
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES = \
    bypass.c \
    midi-split.cpp \
    zynaddsubfx.cpp \
    zynaddsubfx-src.cpp

HEADERS = \
    carla_native.h \
    carla_native.hpp

INCLUDEPATH = . \
    ../carla-includes \
    ../carla-utils

LIBS = \
    ../carla-lilv/carla_lilv.a

QMAKE_CXXFLAGS *= -std=c++0x

# QtCreator project file

QT = core # gui

CONFIG    = debug
CONFIG   += static
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

DEFINES  += WANT_ZYNADDSUBFX WANT_ZYNADDSUBFX_GUI

PKGCONFIG = fftw3 mxml ntk ntk_images

TARGET   = carla_native
TEMPLATE = app #lib
VERSION  = 0.5.0

SOURCES  = \
    bypass.c \
    midi-split.cpp \
    zynaddsubfx.cpp \
    zynaddsubfx-src.cpp

SOURCES += \
    3bandeq.cpp \
    3bandeq-src.cpp \
    3bandsplitter.cpp \
    3bandsplitter-src.cpp \
    distrho/pugl.cpp

HEADERS  = \
    carla_native.h \
    carla_native.hpp

HEADERS += \
    distrho/DistrhoPluginCarla.cpp

INCLUDEPATH = . distrho \
    ../carla-includes \
    ../carla-utils \
    ../distrho-plugin-toolkit

# FIX
INCLUDEPATH += /usr/include/ntk

LIBS = -lGL

QMAKE_CFLAGS   *= -std=c99
QMAKE_CXXFLAGS *= -std=c++0x

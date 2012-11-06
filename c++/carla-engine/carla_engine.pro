# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += static
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

DEFINES  += CARLA_ENGINE_JACK
DEFINES  += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY __LINUX_ALSA__ __LINUX_ALSASEQ__ __LINUX_PULSE__
DEFINES  += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__

PKGCONFIG = alsa jack liblo libpulse-simple

TARGET   = carla_engine
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES = \
     carla_engine.cpp \
     carla_engine_osc.cpp \
     jack.cpp \
     rtaudio.cpp

HEADERS = \
    carla_engine.hpp \
    carla_engine_osc.hpp

INCLUDEPATH = . \
    ../carla-backend \
    ../carla-includes \
    ../carla-jackbridge \
    ../carla-plugin \
    ../carla-utils

# RtAudio/RtMidi
INCLUDEPATH += rtaudio-4.0.11
INCLUDEPATH += rtmidi-2.0.1
SOURCES     += rtaudio-4.0.11/RtAudio.cpp
SOURCES     += rtmidi-2.0.1/RtMidi.cpp

QMAKE_CXXFLAGS *= -std=c++0x

# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
PKGCONFIG = fluidsynth linuxsampler

TARGET   = carla-discovery-native
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    carla-discovery.cpp

INCLUDEPATH = \
    ../carla-backend \
    ../carla-includes \
    ../carla-utils

LIBS = \
    ../carla-lilv/carla_lilv.a

unix {
LIBS += -ldl -lpthread
}
win {
LIBS += -static -mwindows
}

QMAKE_CXXFLAGS *= -std=c++0x

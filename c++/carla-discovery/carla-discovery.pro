# QtCreator project file

QT = core

CONFIG    = debug
CONFIG   += link_pkgconfig qt warn_on

DEFINES   = DEBUG
DEFINES  += BUILD_NATIVE
DEFINES  += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
PKGCONFIG = fluidsynth linuxsampler

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    carla-discovery.cpp

HEADERS = \
    ../carla-includes/carla_defines.hpp \
    ../carla-includes/ladspa_rdf.hpp \
    ../carla-includes/lv2_rdf.hpp \
    ../carla-utils/carla_lib_utils.hpp \
    ../carla-utils/carla_ladspa_utils.hpp \
    ../carla-utils/carla_lv2_utils.hpp \
    ../carla-utils/carla_vst_utils.hpp

INCLUDEPATH = .. \
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

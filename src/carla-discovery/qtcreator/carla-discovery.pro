# QtCreator project file

QT = core

CONFIG    = debug link_pkgconfig qt warn_on
DEFINES   = DEBUG BUILD_NATIVE WANT_FLUIDSYNTH WANT_LINUXSAMPLER
PKGCONFIG = fluidsynth linuxsampler

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla-discovery.cpp

HEADERS = \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_ladspa.h \
    ../../carla-includes/carla_dssi.h \
    ../../carla-includes/carla_lv2.h \
    ../../carla-includes/carla_vst.h \
    ../../carla-includes/carla_fluidsynth.h \
    ../../carla-includes/carla_linuxsampler.h \
    ../../carla-includes/ladspa_rdf.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-backend \
    ../../carla-includes

LIBS = \
    ../../carla-lilv/carla_lilv.a

QMAKE_CXXFLAGS *= -std=c++0x

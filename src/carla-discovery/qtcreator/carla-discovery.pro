# QtCreator project file

QT = core

CONFIG    = link_pkgconfig qt warn_on debug
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
    ../../carla-includes/carla_vst_includes.h \
    ../../carla-includes/carla_ladspa_includes.h \
    ../../carla-includes/carla_lv2_includes.h \
    ../../carla-includes/carla_vst_includes.h \
    ../../carla-includes/carla_linuxsampler_includes.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-backend \
    ../../carla-includes \
    ../../carla-includes/vst

LIBS = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

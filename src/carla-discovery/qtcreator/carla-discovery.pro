# QtCreator project file

QT = core

CONFIG    = link_pkgconfig qt warn_on debug
DEFINES   = DEBUG VESTIGE_HEADER WANT_FLUIDSYNTH
PKGCONFIG = fluidsynth

TARGET   = carla-discovery-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

SOURCES = \
    ../carla-discovery.cpp

HEADERS = \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_vst_includes.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-includes/vestige

LIBS = ../../carla-lilv/carla_lilv.a -ldl

QMAKE_CXXFLAGS *= -std=c++0x

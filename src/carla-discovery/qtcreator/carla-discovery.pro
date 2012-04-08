# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5.0

SOURCES = \
    ../carla-discovery.cpp

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-includes/vestige

TARGET  = carla-discovery-qtcreator

DEFINES = VESTIGE_HEADER BUILD_UNIX64

LIBS += -ldl

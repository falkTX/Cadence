# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5

TARGET = carla-discovery-qtcreator

SOURCES = ../carla-discovery.cpp

INCLUDEPATH = ../../carla-includes
#../../carla-includes/vestige

LIBS += -ldl

DEFINES = BUILD_UNIX64

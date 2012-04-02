# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5

TARGET = carla-bridge-qtcreator

SOURCES = ../carla_bridge.cpp ../carla_osc.cpp \
  ../../carla/carla_jack.cpp ../../carla/carla_shared.cpp \
  ../../carla/ladspa.cpp ../../carla/dssi.cpp ../../carla/vst.cpp

HEADERS = ../carla_osc.h \
  ../../carla/carla_backend.h ../../carla/carla_includes.h \
  ../../carla/carla_jack.h ../../carla/carla_plugin.h ../../carla/carla_shared.h

INCLUDEPATH = ../ ../../carla-includes ../../carla
#../../carla-includes/vestige

LIBS += -ldl -ljack -llo

DEFINES = BUILD_BRIDGE

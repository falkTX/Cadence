# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5.0

SOURCES = \
    ../carla_bridge.cpp \
    ../carla_osc.cpp \
    ../../carla/carla_jack.cpp \
    ../../carla/carla_shared.cpp \
    ../../carla/ladspa.cpp \
    ../../carla/dssi.cpp \
    ../../carla/vst.cpp

HEADERS = \
    ../carla_osc.h \
    ../../carla/carla_backend.h \
    ../../carla/carla_jack.h \
    ../../carla/carla_plugin.h \
    ../../carla/carla_shared.h \
    ../../carla-includes/carla_includes.h

INCLUDEPATH = .. \
    ../../carla-includes \
    ../../carla-includes/vestige \
    ../../carla

TARGET  = carla-bridge-qtcreator

DEFINES = VESTIGE_HEADER BUILD_BRIDGE

LIBS += -ldl -ljack -llo

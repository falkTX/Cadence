# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5

TARGET = carla-bridge-lv2-gtk2

SOURCES = \
  ../carla_bridge_lv2.cpp \
  ../carla_bridge_gtk2.cpp \
  ../../carla-bridge/carla_osc.cpp

HEADERS = \
  ../carla_bridge_ui.h \
  ../../carla-bridge/carla_osc.h \
  ../../carla-includes/carla_includes.h

INCLUDEPATH = .. \
  ../../carla-bridge \
  ../../carla-includes

LIBS += -ldl -lpthread -llo

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += gtk+-2.0
}

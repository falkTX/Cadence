# QtCreator project file

CONFIG = warn_on qt release

TEMPLATE = app
VERSION = 0.5.0

SOURCES = \
    ../carla_bridge_lv2.cpp \
    ../carla_bridge_gtk2.cpp \
    ../../carla-bridge/carla_osc.cpp

HEADERS = \
    ../carla_bridge_ui.h \
    ../../carla-bridge/carla_osc.h

INCLUDEPATH = .. \
    ../../carla-bridge \
    ../../carla-includes

TARGET  = carla-bridge-lv2-gtk2

DEFINES = BUILD_BRIDGE BUILD_BRIDGE_UI BRIDGE_LV2_GTK2

LIBS += -ldl -lpthread -llo

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += gtk+-2.0
}

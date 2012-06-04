#!/usr/bin/make -f
# Makefile for carla-bridges #
# ---------------------------------------- #
# Created by falkTX
#

CC      ?= gcc
CXX     ?= g++
WINECC  ?= winegcc
WINECXX ?= wineg++

BUILD_CFLAGS = -O2 -ffast-math -fomit-frame-pointer -mtune=generic -msse -std=c99 -Wall -I. -I../carla -I../carla-includes $(CFLAGS)

BUILD_FLAGS  = -O2 -ffast-math -fomit-frame-pointer -mtune=generic -msse -std=c++0x -Wall $(CXXFLAGS)
BUILD_FLAGS += -I. -I../carla-backend -I../carla-includes $(shell pkg-config --cflags liblo QtCore)
BUILD_FLAGS += -DBUILD_BRIDGE -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
BUILD_FLAGS += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64
LINK_FLAGS   = $(shell pkg-config --libs liblo QtCore)

# --------------------------------------------------------------

BUILD_PLUGIN_FLAGS = $(BUILD_FLAGS) $(shell pkg-config --cflags QtGui)
LINK_PLUGIN_FLAGS  = $(LINK_FLAGS) $(shell pkg-config --libs QtGui)

UNIX_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS)
UNIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
UNIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
UNIX_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -ldl

WIN_BUILD_FLAGS  = $(BUILD_PLUGIN_FLAGS)
WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
WIN_LINK_FLAGS   = $(LINK_PLUGIN_FLAGS) -static

WINE_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS) # -fpermissive
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -ldl

# --------------------------------------------------------------

BUILD_UI_FLAGS = $(BUILD_FLAGS) -DBUILD_BRIDGE_UI
LINK_UI_FLAGS  = $(LINK_FLAGS)

BUILD_UI_LV2_GTK2_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_UI_LV2_GTK2_FLAGS  = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-2.0)

BUILD_UI_LV2_QT4_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtGui)
LINK_UI_LV2_QT4_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

BUILD_UI_LV2_X11_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_LV2_X11 $(shell pkg-config --cflags QtGui)
LINK_UI_LV2_X11_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

BUILD_UI_VST_X11_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_VST_X11 $(shell pkg-config --cflags QtGui)
LINK_UI_VST_X11_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

# --------------------------------------------------------------

all: ui

ui: ui_lv2-gtk2 ui_lv2-qt4 ui_lv2-x11 ui_vst-x11
ui_lv2-gtk2: carla-bridge-lv2-gtk2
ui_lv2-qt4:  carla-bridge-lv2-qt4
ui_lv2-x11:  carla-bridge-lv2-x11
ui_vst-x11:  carla-bridge-vst-x11

unix32: carla-bridge-unix32
unix64: carla-bridge-unix64
win32:  carla-bridge-win32.exe
win64:  carla-bridge-win64.exe
wine32: carla-bridge-win32.exe.so
wine64: carla-bridge-win64.exe.so

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_UI_LV2_GTK2 = carla_bridge_ui-lv2__lv2-gtk2.o carla_bridge_ui-gtk2__lv2-gtk2.o carla_bridge_osc__lv2-gtk2.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-gtk2: $(OBJS_UI_LV2_GTK2)
	$(CXX) $(OBJS_UI_LV2_GTK2) $(LINK_UI_LV2_GTK2_FLAGS) -o $@ && strip $@

carla_bridge_ui-lv2__lv2-gtk2.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

carla_bridge_ui-gtk2__lv2-gtk2.o: carla_bridge_ui-gtk2.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

carla_bridge_osc__lv2-gtk2.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_UI_LV2_QT4 = carla_bridge_ui-lv2__lv2-qt4.o carla_bridge_ui-qt4__lv2-qt4.o carla_bridge_osc__lv2-qt4.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-qt4: $(OBJS_UI_LV2_QT4)
	$(CXX) $(OBJS_UI_LV2_QT4) $(LINK_UI_LV2_QT4_FLAGS) -o $@ && strip $@

carla_bridge_ui-lv2__lv2-qt4.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

carla_bridge_ui-qt4__lv2-qt4.o: carla_bridge_ui-qt4.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

carla_bridge_osc__lv2-qt4.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_UI_LV2_X11 = carla_bridge_ui-lv2__lv2-x11.o carla_bridge_ui-qt4__lv2-x11.o carla_bridge_osc__lv2-x11.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-x11: $(OBJS_UI_LV2_X11)
	$(CXX) $(OBJS_UI_LV2_X11) $(LINK_UI_LV2_X11_FLAGS) -o $@ && strip $@

carla_bridge_ui-lv2__lv2-x11.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

carla_bridge_ui-qt4__lv2-x11.o: carla_bridge_ui-qt4.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

carla_bridge_osc__lv2-x11.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_UI_VST_X11 = carla_bridge_ui-vst__vst-x11.o carla_bridge_ui-qt4__vst-x11.o carla_bridge_osc__vst-x11.o

carla-bridge-vst-x11: $(OBJS_UI_VST_X11)
	$(CXX) $(OBJS_UI_VST_X11) $(LINK_UI_VST_X11_FLAGS) -o $@ && strip $@

carla_bridge_ui-vst__vst-x11.o: carla_bridge_ui-vst.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

carla_bridge_ui-qt4__vst-x11.o: carla_bridge_ui-qt4.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

carla_bridge_osc__vst-x11.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# unix32

OBJS_UNIX32 = carla_bridge_plugin__unix32.o carla_bridge_osc__unix32.o carla_jack__unix32.o carla_shared__unix32.o ladspa__unix32.o dssi__unix32.o lv2__unix32.o vst__unix32.o rtmempool__unix32.o ../carla-lilv/carla_lilv_32bit.a

carla-bridge-unix32: $(OBJS_UNIX32)

# carla_bridge_plugin__unix32.o: carla_bridge_plugin.cpp
# 	$(CXX) $< $(UNIX_BUILD_FLAGS) $(UNIX_32BIT_FLAGS) $(UNIX_LINK_FLAGS) -c -o $@

# carla_jack__unix32.o: ../carla/carla_jack.cpp
# 	$(CXX) $< $(UNIX_BUILD_FLAGS) $(UNIX_32BIT_FLAGS) $(UNIX_LINK_FLAGS) -c -o $@

rtmempool__unix32.o: ../carla/lv2-rtmempool/rtmempool.c
	$(CC) $< $(BUILD_CFLAGS) $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.o *.so *.exe
	rm -f carla-bridge-lv2-gtk2 carla-bridge-lv2-qt4 carla-bridge-lv2-x11 carla-bridge-vst-x11
	rm -f carla-bridge-unix32 carla-bridge-unix64

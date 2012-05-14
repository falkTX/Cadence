#!/usr/bin/make -f
# Makefile for Carla-Bridge-UI #
# -------------------------------------------- #
# Created by falkTX
#

CXX     ?= g++
WINECXX ?= wineg++

BUILD_FLAGS  = -DBUILD_BRIDGE -DBUILD_BRIDGE_UI -I. -I../carla -I../carla-bridge -I../carla-includes
BUILD_FLAGS += -Wall -std=c++0x `pkg-config --cflags liblo QtCore` $(CXXFLAGS)
BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT -O2 -ffast-math -fomit-frame-pointer -mtune=generic -msse
# BUILD_FLAGS += -DDEBUG -O0 -g
BUILD_FLAGS += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header
LINK_FLAGS   = `pkg-config --cflags --libs liblo QtCore` -ldl $(LDFLAGS)

BRIDGE_LV2_GTK2_FLAGS = $(BUILD_FLAGS) -DBRIDGE_LV2_GTK2 `pkg-config --cflags --libs gtk+-2.0` $(LINK_FLAGS)
BRIDGE_LV2_QT4_FLAGS  = $(BUILD_FLAGS) -DBRIDGE_LV2_QT4 `pkg-config --cflags --libs QtGui` $(LINK_FLAGS)
BRIDGE_LV2_X11_FLAGS  = $(BUILD_FLAGS) -DBRIDGE_LV2_X11 `pkg-config --cflags --libs QtGui` $(LINK_FLAGS)
BRIDGE_VST_X11_FLAGS  = $(BUILD_FLAGS) -DBRIDGE_VST_QT4 `pkg-config --cflags --libs QtGui` $(LINK_FLAGS)


all: carla-bridge-lv2-gtk2 carla-bridge-lv2-qt4 carla-bridge-lv2-x11 carla-bridge-vst-x11

carla-bridge-lv2-gtk2: carla_bridge_lv2.cpp carla_bridge_gtk2.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a
	$(CXX) carla_bridge_lv2.cpp carla_bridge_gtk2.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a $(BRIDGE_LV2_GTK2_FLAGS) -o $@

carla-bridge-lv2-qt4: carla_bridge_lv2.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a
	$(CXX) carla_bridge_lv2.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a $(BRIDGE_LV2_QT4_FLAGS) -o $@

carla-bridge-lv2-x11: carla_bridge_lv2.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a
	$(CXX) carla_bridge_lv2.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp ../carla-lilv/carla_lilv.a $(BRIDGE_LV2_X11_FLAGS) -o $@

carla-bridge-vst-x11: carla_bridge_vst.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp
	$(CXX) carla_bridge_vst.cpp carla_bridge_qt4.cpp ../carla-bridge/carla_bridge_osc.cpp $(BRIDGE_VST_X11_FLAGS) -o $@

clean:
	rm -f carla-bridge-*

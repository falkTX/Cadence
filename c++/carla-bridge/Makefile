#!/usr/bin/make -f
# Makefile for carla-bridges #
# ---------------------------------------- #
# Created by falkTX
#

CXX     ?= g++
WINECXX ?= wineg++
STRIP   ?= strip

BASE_FLAGS   = -O2 -ffast-math -fomit-frame-pointer -fPIC -mtune=generic -msse -mfpmath=sse -Wall

BUILD_FLAGS  = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
BUILD_FLAGS += -I. -I../carla-includes $(shell pkg-config --cflags liblo QtCore)
BUILD_FLAGS += -DBUILD_BRIDGE -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
BUILD_FLAGS += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64
LINK_FLAGS   = $(shell pkg-config --libs liblo QtCore) $(LDFLAGS)

# --------------------------------------------------------------

BUILD_PLUGIN_FLAGS = $(BUILD_FLAGS) -DBUILD_BRIDGE_PLUGIN -DCARLA_ENGINE_JACK $(shell pkg-config --cflags jack)
LINK_PLUGIN_FLAGS  = $(LINK_FLAGS) $(shell pkg-config --libs jack)

POSIX_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS) $(shell pkg-config --cflags QtGui)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs QtGui) -ldl

WIN_BUILD_FLAGS  = $(BUILD_PLUGIN_FLAGS) $(shell pkg-config --cflags QtGui)
WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
WIN_LINK_FLAGS   = $(LINK_PLUGIN_FLAGS) $(shell pkg-config --libs QtGui) -static

WINE_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS) # -fpermissive
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -ldl

# --------------------------------------------------------------

BUILD_UI_FLAGS = $(BUILD_FLAGS) -DBUILD_BRIDGE_UI
LINK_UI_FLAGS  = $(LINK_FLAGS) -ldl

BUILD_UI_LV2_GTK2_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_LV2 -DBRIDGE_LV2_GTK2 $(shell pkg-config --cflags gtk+-2.0)
LINK_UI_LV2_GTK2_FLAGS  = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-2.0)

BUILD_UI_LV2_GTK3_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_LV2 -DBRIDGE_LV2_GTK3 $(shell pkg-config --cflags gtk+-3.0)
LINK_UI_LV2_GTK3_FLAGS  = $(LINK_UI_FLAGS) $(shell pkg-config --libs gtk+-3.0)

BUILD_UI_LV2_QT4_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_LV2 -DBRIDGE_LV2_QT4 $(shell pkg-config --cflags QtGui)
LINK_UI_LV2_QT4_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

BUILD_UI_LV2_X11_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_LV2 -DBRIDGE_LV2_X11 $(shell pkg-config --cflags QtGui)
LINK_UI_LV2_X11_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

BUILD_UI_VST_HWND_FLAGS = $(BUILD_UI_FLAGS) -DBRIDGE_VST -DBRIDGE_VST_HWND $(shell pkg-config --cflags QtGui)
LINK_UI_VST_HWND_FLAGS  = $(LINK_FLAGS) $(shell pkg-config --libs QtGui) -mwindows -static

BUILD_UI_VST_X11_FLAGS  = $(BUILD_UI_FLAGS) -DBRIDGE_VST -DBRIDGE_VST_X11 $(shell pkg-config --cflags QtGui)
LINK_UI_VST_X11_FLAGS   = $(LINK_UI_FLAGS) $(shell pkg-config --libs QtGui)

# --------------------------------------------------------------

all: ui

ui: ui_lv2-gtk2 ui_lv2-qt4 ui_lv2-x11 ui_vst-x11
ui_lv2-gtk2: carla-bridge-lv2-gtk2
ui_lv2-gtk3: carla-bridge-lv2-gtk3
ui_lv2-qt4:  carla-bridge-lv2-qt4
ui_lv2-x11:  carla-bridge-lv2-x11
ui_vst-hwnd: carla-bridge-vst-hwnd.exe
ui_vst-x11:  carla-bridge-vst-x11

posix32: carla-bridge-posix32
posix64: carla-bridge-posix64
win32:   carla-bridge-win32.exe
win64:   carla-bridge-win64.exe
wine32:  carla-bridge-win32.exe.so
wine64:  carla-bridge-win64.exe.so

# --------------------------------------------------------------
# ui_lv2-gtk2

OBJS_UI_LV2_GTK2 = carla_bridge_ui-lv2__lv2-gtk2.o carla_bridge_toolkit-gtk2__lv2-gtk2.o carla_bridge_osc__lv2-gtk2.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-gtk2: $(OBJS_UI_LV2_GTK2)
	$(CXX) $^ $(LINK_UI_LV2_GTK2_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-lv2__lv2-gtk2.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

carla_bridge_toolkit-gtk2__lv2-gtk2.o: carla_bridge_toolkit-gtk2.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

carla_bridge_osc__lv2-gtk2.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK2_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-gtk3

OBJS_UI_LV2_GTK3 = carla_bridge_ui-lv2__lv2-gtk3.o carla_bridge_toolkit-gtk3__lv2-gtk3.o carla_bridge_osc__lv2-gtk3.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-gtk3: $(OBJS_UI_LV2_GTK3)
	$(CXX) $^ $(LINK_UI_LV2_GTK3_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-lv2__lv2-gtk3.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

carla_bridge_toolkit-gtk3__lv2-gtk3.o: carla_bridge_toolkit-gtk3.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

carla_bridge_osc__lv2-gtk3.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_GTK3_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-qt4

OBJS_UI_LV2_QT4 = carla_bridge_ui-lv2__lv2-qt4.o carla_bridge_toolkit-qt4__lv2-qt4.o carla_bridge_osc__lv2-qt4.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-qt4: $(OBJS_UI_LV2_QT4)
	$(CXX) $^ $(LINK_UI_LV2_QT4_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-lv2__lv2-qt4.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

carla_bridge_toolkit-qt4__lv2-qt4.o: carla_bridge_toolkit-qt4.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

carla_bridge_osc__lv2-qt4.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_QT4_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_lv2-x11

OBJS_UI_LV2_X11 = carla_bridge_ui-lv2__lv2-x11.o carla_bridge_toolkit-qt4__lv2-x11.o carla_bridge_osc__lv2-x11.o ../carla-lilv/carla_lilv.a

carla-bridge-lv2-x11: $(OBJS_UI_LV2_X11)
	$(CXX) $^ $(LINK_UI_LV2_X11_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-lv2__lv2-x11.o: carla_bridge_ui-lv2.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

carla_bridge_toolkit-qt4__lv2-x11.o: carla_bridge_toolkit-qt4.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

carla_bridge_osc__lv2-x11.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_LV2_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-hwnd

OBJS_UI_VST_HWND = carla_bridge_ui-vst__vst-hwnd.o carla_bridge_toolkit-qt4__vst-hwnd.o carla_bridge_osc__vst-hwnd.o

carla-bridge-vst-hwnd.exe: $(OBJS_UI_VST_HWND)
	$(CXX) $^ $(LINK_UI_VST_HWND_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-vst__vst-hwnd.o: carla_bridge_ui-vst.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

carla_bridge_toolkit-qt4__vst-hwnd.o: carla_bridge_toolkit-qt4.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

carla_bridge_osc__vst-hwnd.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_VST_HWND_FLAGS) -c -o $@

# --------------------------------------------------------------
# ui_vst-x11

OBJS_UI_VST_X11 = carla_bridge_ui-vst__vst-x11.o carla_bridge_toolkit-qt4__vst-x11.o carla_bridge_osc__vst-x11.o

carla-bridge-vst-x11: $(OBJS_UI_VST_X11)
	$(CXX) $^ $(LINK_UI_VST_X11_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_ui-vst__vst-x11.o: carla_bridge_ui-vst.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

carla_bridge_toolkit-qt4__vst-x11.o: carla_bridge_toolkit-qt4.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

carla_bridge_osc__vst-x11.o: carla_bridge_osc.cpp
	$(CXX) $< $(BUILD_UI_VST_X11_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix32

OBJS_POSIX32 = \
	carla_bridge_plugin__posix32.o carla_bridge_osc__posix32.o \
	carla_engine_jack__posix32.o carla_shared__posix32.o ladspa__posix32.o dssi__posix32.o lv2__posix32.o vst__posix32.o ../carla-lilv/carla_lilv_32bit.a

carla-bridge-posix32: $(OBJS_POSIX32)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_plugin__posix32.o: carla_bridge_plugin.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

carla_bridge_osc__posix32.o: carla_bridge_osc.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

carla_engine_jack__posix32.o: ../carla-backend/carla_engine_jack.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

carla_shared__posix32.o: ../carla-backend/carla_shared.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

ladspa__posix32.o: ../carla-backend/ladspa.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

dssi__posix32.o: ../carla-backend/dssi.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

lv2__posix32.o: ../carla-backend/lv2.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

vst__posix32.o: ../carla-backend/vst.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix64

OBJS_POSIX64 = \
	carla_bridge_plugin__posix64.o carla_bridge_osc__posix64.o \
	carla_engine_jack__posix64.o carla_shared__posix64.o ladspa__posix64.o dssi__posix64.o lv2__posix64.o vst__posix64.o ../carla-lilv/carla_lilv_64bit.a

carla-bridge-posix64: $(OBJS_POSIX64)
	$(CXX) $^ $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@ && $(STRIP) $@

carla_bridge_plugin__posix64.o: carla_bridge_plugin.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

carla_bridge_osc__posix64.o: carla_bridge_osc.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

carla_engine_jack__posix64.o: ../carla-backend/carla_engine_jack.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

carla_shared__posix64.o: ../carla-backend/carla_shared.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

ladspa__posix64.o: ../carla-backend/ladspa.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

dssi__posix64.o: ../carla-backend/dssi.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

lv2__posix64.o: ../carla-backend/lv2.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

vst__posix64.o: ../carla-backend/vst.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# wine32

OBJS_WINE32 = \
	carla_bridge_plugin__wine32.o carla_bridge_osc__wine32.o \
	carla_engine_jack__wine32.o carla_shared__wine32.o ladspa__wine32.o dssi__wine32.o lv2__wine32.o vst__wine32.o ../carla-lilv/carla_lilv_32bit.a

carla-bridge-win32.exe.so: $(OBJS_WINE32)
	$(WINECXX) $^ $(WINE_LINK_FLAGS) $(WINE_32BIT_FLAGS) -o carla-bridge-win32 && $(STRIP) $@

carla_bridge_plugin__wine32.o: carla_bridge_plugin.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

carla_bridge_osc__wine32.o: carla_bridge_osc.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

carla_engine_jack__wine32.o: ../carla-backend/carla_engine_jack.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

carla_shared__wine32.o: ../carla-backend/carla_shared.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

ladspa__wine32.o: ../carla-backend/ladspa.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

dssi__wine32.o: ../carla-backend/dssi.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

lv2__wine32.o: ../carla-backend/lv2.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

vst__wine32.o: ../carla-backend/vst.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# wine64

OBJS_WINE64 = \
	carla_bridge_plugin__wine64.o carla_bridge_osc__wine64.o \
	carla_engine_jack__wine64.o carla_shared__wine64.o ladspa__wine64.o dssi__wine64.o lv2__wine64.o vst__wine64.o ../carla-lilv/carla_lilv_64bit.a

carla-bridge-win64.exe.so: $(OBJS_WINE64)
	$(WINECXX) $^ $(WINE_LINK_FLAGS) $(WINE_64BIT_FLAGS) -o carla-bridge-win64 && $(STRIP) $@

carla_bridge_plugin__wine64.o: carla_bridge_plugin.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

carla_bridge_osc__wine64.o: carla_bridge_osc.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

carla_engine_jack__wine64.o: ../carla-backend/carla_engine_jack.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

carla_shared__wine64.o: ../carla-backend/carla_shared.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

ladspa__wine64.o: ../carla-backend/ladspa.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

dssi__wine64.o: ../carla-backend/dssi.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

lv2__wine64.o: ../carla-backend/lv2.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

vst__wine64.o: ../carla-backend/vst.cpp
	$(WINECXX) $< $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

../carla-lilv/carla_lilv_32bit.a:
	$(MAKE) -C ../carla-lilv 32bit

../carla-lilv/carla_lilv_64bit.a:
	$(MAKE) -C ../carla-lilv 64bit

# --------------------------------------------------------------

doc: carla_bridge.doxygen
	doxygen $<

clean:
	rm -f *.o *.so *.exe
	rm -f carla-bridge-lv2-gtk2 carla-bridge-lv2-gtk3 carla-bridge-lv2-qt4 carla-bridge-lv2-x11 carla-bridge-vst-x11
	rm -f carla-bridge-posix32 carla-bridge-posix64

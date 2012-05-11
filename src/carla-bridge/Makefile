#!/usr/bin/make -f
# Makefile for Carla-Bridge #
# ----------------------------------------- #
# Created by falkTX
#

CXX     ?= g++
WINECXX ?= wineg++

BUILD_FLAGS  = -DBUILD_BRIDGE -Wall -std=c++0x -I. -I../carla-includes -I../carla `pkg-config --cflags jack liblo QtCore QtGui` $(CXXFLAGS)
# BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT -O2 -ffast-math -fomit-frame-pointer -mtune=generic -msse
BUILD_FLAGS += -DDEBUG -O0 -g
BUILD_FLAGS += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header
LINK_FLAGS   = `pkg-config --libs jack liblo QtCore QtGui` $(LDFLAGS)

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64

UNIX_BUILD_FLAGS = $(BUILD_FLAGS)
UNIX_LINK_FLAGS  = -ldl $(LINK_FLAGS)
UNIX_32BIT_FLAGS = -L/usr/lib32 -L/usr/lib/i386-linux-gnu $(32BIT_FLAGS)
UNIX_64BIT_FLAGS = -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu $(64BIT_FLAGS)

WIN_BUILD_FLAGS = $(BUILD_FLAGS)
WIN_LINK_FLAGS  = -static $(LINK_FLAGS)
WIN_32BIT_FLAGS = $(32BIT_FLAGS)
WIN_64BIT_FLAGS = $(64BIT_FLAGS)

WINE_BUILD_FLAGS = -fpermissive $(BUILD_FLAGS)
WINE_LINK_FLAGS  = $(LINK_FLAGS)
WINE_32BIT_FLAGS = -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine $(32BIT_FLAGS)
WINE_64BIT_FLAGS = -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine $(64BIT_FLAGS)

SOURCES = carla_bridge.cpp carla_osc.cpp ../carla/carla_jack.cpp ../carla/carla_shared.cpp ../carla/ladspa.cpp ../carla/dssi.cpp ../carla/lv2.cpp ../carla/vst.cpp ../carla/lv2-rtmempool/rtmempool.o ../carla-lilv/carla_lilv.a


all:
	@echo "Build type must be unix32, unix64, win[e]32 or win[e]64"

unix32: carla-bridge-unix32

unix64: carla-bridge-unix64

win32: carla-bridge-win32.exe

win64: carla-bridge-win64.exe

wine32: carla-bridge-win32.exe.so

wine64: carla-bridge-win64.exe.so


carla-bridge-unix32: $(SOURCES)
	$(CXX) $(SOURCES) $(UNIX_BUILD_FLAGS) $(UNIX_32BIT_FLAGS) $(UNIX_LINK_FLAGS) -o $@

carla-bridge-unix64: $(SOURCES)
	$(CXX) $(SOURCES) $(UNIX_BUILD_FLAGS) $(UNIX_64BIT_FLAGS) $(UNIX_LINK_FLAGS) -o $@

carla-bridge-win32.exe: $(SOURCES)
	$(CXX) $(SOURCES) $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-bridge-win64.exe: $(SOURCES)
	$(CXX) $(SOURCES) $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-bridge-win32.exe.so: $(SOURCES)
	$(WINECXX) $(SOURCES) $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -o carla-bridge-win32.exe

carla-bridge-win64.exe.so: $(SOURCES)
	$(WINECXX) $(SOURCES) $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -o carla-bridge-win64.exe

clean:
	rm -f carla-bridge-*

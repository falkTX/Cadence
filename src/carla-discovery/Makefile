#!/usr/bin/make -f
# Makefile for Carla-Discovery #
# ----------------------------------------- #
# Created by falkTX
#

CXX     ?= g++
WINECXX ?= wineg++

BUILD_FLAGS  = -Wall -std=c++0x -ffast-math -fomit-frame-pointer -mtune=generic -msse -O2 -I../carla-includes $(CXXFLAGS)
BUILD_FLAGS += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header
LINK_FLAGS   = $(LDFLAGS)

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

ifeq ($(FLUIDSYNTH),)
else
BUILD_FLAGS += `pkg-config --cflags fluidsynth` -DWANT_FLUIDSYNTH
LINK_FLAGS  += `pkg-config --libs fluidsynth`
endif


all:
	@echo "Build type must be unix32, unix64, win32 or win64"

unix32: carla-discovery-unix32

unix64: carla-discovery-unix64

win32: carla-discovery-win32.exe

win64: carla-discovery-win64.exe

wine32: carla-discovery-win32.exe.so

wine64: carla-discovery-win64.exe.so


carla-discovery-unix32: carla-discovery.cpp
	$(CXX) carla-discovery.cpp $(UNIX_BUILD_FLAGS) $(UNIX_32BIT_FLAGS) $(UNIX_LINK_FLAGS) -DBUILD_UNIX32 -o $@

carla-discovery-unix64: carla-discovery.cpp
	$(CXX) carla-discovery.cpp $(UNIX_BUILD_FLAGS) $(UNIX_64BIT_FLAGS) $(UNIX_LINK_FLAGS) -DBUILD_UNIX64 -o $@

carla-discovery-win32.exe: carla-discovery.cpp
	$(CXX) carla-discovery.cpp $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -DBUILD_WIN32 -o $@

carla-discovery-win64.exe: carla-discovery.cpp
	$(CXX) carla-discovery.cpp $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -DBUILD_WIN64 -o $@

carla-discovery-win32.exe.so: carla-discovery.cpp
	$(WINECXX) carla-discovery.cpp $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -DBUILD_WIN32 -o carla-discovery-win32.exe

carla-discovery-win64.exe.so: carla-discovery.cpp
	$(WINECXX) carla-discovery.cpp $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -DBUILD_WIN64 -o carla-discovery-win64.exe


clean:
	rm -f carla-discovery-*

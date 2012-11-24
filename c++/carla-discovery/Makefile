#!/usr/bin/make -f
# Makefile for carla-discovery #
# ----------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../carla-backend -I../carla-includes -I../carla-utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifneq ($(NATIVE),)
ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth) -DWANT_FLUIDSYNTH
LINK_FLAGS      += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler) -DWANT_LINUXSAMPLER
LINK_FLAGS      += $(shell pkg-config --libs linuxsampler)
endif
endif

# --------------------------------------------------------------

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lpthread

WIN_BUILD_FLAGS   = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS   = $(32BIT_FLAGS)
WIN_64BIT_FLAGS   = $(64BIT_FLAGS)
WIN_LINK_FLAGS    = $(LINK_FLAGS) -static -mwindows

# --------------------------------------------------------------

all: carla-discovery-native

posix32: carla-discovery-posix32
posix64: carla-discovery-posix64
win32:   carla-discovery-win32.exe
win64:   carla-discovery-win64.exe

# --------------------------------------------------------------

OBJS = carla-discovery.cpp

carla-discovery-native: $(OBJS) ../carla-lilv/carla_lilv.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-posix32: $(OBJS) ../carla-lilv/carla_lilv_posix32.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-posix64: $(OBJS) ../carla-lilv/carla_lilv_posix64.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-win32.exe: $(OBJS) ../carla-lilv/carla_lilv_win32.a
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-win64.exe: $(OBJS) ../carla-lilv/carla_lilv_win64.a
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

../carla-lilv/carla_lilv.a:
	$(MAKE) -C ../carla-lilv

../carla-lilv/carla_lilv_posix32.a:
	$(MAKE) -C ../carla-lilv posix32

../carla-lilv/carla_lilv_posix64.a:
	$(MAKE) -C ../carla-lilv posix64

../carla-lilv/carla_lilv_win32.a:
	$(MAKE) -C ../carla-lilv win32

../carla-lilv/carla_lilv_win64.a:
	$(MAKE) -C ../carla-lilv win64

# --------------------------------------------------------------

clean:
	rm -f carla-discovery-*

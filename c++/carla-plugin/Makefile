#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC
BUILD_CXX_FLAGS += -I. -I../carla-backend -I../carla-engine -I../carla-includes -I../carla-native -I../carla-utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore QtGui)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
endif

ifeq ($(HAVE_SUIL),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags suil-0) -DWANT_SUIL
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth) -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler) -DWANT_LINUXSAMPLER
endif

OBJS = \
	carla_plugin.o \
	carla_plugin_thread.o \
	carla_bridge.o \
	native.o \
	ladspa.o \
	dssi.o \
	lv2.o \
	vst.o \
	fluidsynth.o \
	linuxsampler.o

# --------------------------------------------------------------

all: carla_plugin.a

doxygen: carla_plugin.doxygen
	doxygen $<

carla_plugin.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a *.o

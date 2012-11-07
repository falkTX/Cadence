#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_ZYN_DEPS    = $(shell pkg-config --exists fftw3 mxml && echo true)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
HAVE_SUIL        = $(shell pkg-config --exists suil-0 && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH  = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

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

ifeq ($(HAVE_LINUSAMPLER),true)
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

doc: carla_plugin.doxygen
	doxygen $<

carla_plugin.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a $(OBJS)

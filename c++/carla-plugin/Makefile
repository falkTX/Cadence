#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)

BUILD_CXX_FLAGS  += -I. -I../carla-includes -I../carla-engine -I../carla-utils -fPIC
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags QtCore)

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS  += -DWANT_ZYNADDSUBFX
endif

OBJS = \
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
	rm -f *.a *.o

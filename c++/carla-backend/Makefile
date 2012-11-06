#!/usr/bin/make -f
# Makefile for carla-backend #
# ------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
HAVE_SUIL         = $(shell pkg-config --exists suil-0 && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)
HAVE_ZYN_GUI_DEPS = $(shell pkg-config --exists ntk && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../carla-includes -I../carla-engine -I../carla-jackbridge -I../carla-native -I../carla-plugin -I../carla-utils
BUILD_CXX_FLAGS += -fPIC
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack liblo QtCore QtGui)

LINK_FLAGS      += -fPIC -shared -ldl -lm
LINK_FLAGS      += $(shell pkg-config --libs jack liblo QtCore QtGui)

ifeq ($(HAVE_SUIL),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags suil-0) -DWANT_SUIL
LINK_FLAGS      += $(shell pkg-config --libs suil-0)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth) -DWANT_FLUIDSYNTH
LINK_FLAGS      += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler) -DWANT_LINUXSAMPLER
LINK_FLAGS      += $(shell pkg-config --libs linuxsampler)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml)

ifeq ($(HAVE_ZYN_GUI_DEPS),true)
LINK_FLAGS       += $(shell pkg-config --libs ntk)
endif
endif

OBJS = \
	carla_backend_standalone.o \

OBJS += \
	../carla-engine/carla_engine.a \
	../carla-native/carla_native.a \
	../carla-plugin/carla_plugin.a

# others
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
OBJS += ../carla-lilv/carla_lilv.a
OBJS += ../carla-rtmempool/carla_rtmempool.a
endif

# --------------------------------------------------------------

all: carla_backend.so

doc: carla_backend.doxygen
	doxygen $<

carla_backend.so: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@ && strip $@

# carla_backend-dssi.so: $(OBJS) carla_backend_plugin.cpp
# 	$(CXX) $^ -DDISTRHO_PLUGIN_TARGET_DSSI -Idistrho-plugin-toolkit $(CARLA_CXX_FLAGS) $(CARLA_LD_FLAGS) -o $@ && strip $@

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

../carla-engine/carla_engine.a:
	$(MAKE) -C ../carla-engine

../carla-lilv/carla_lilv.a:
	$(MAKE) -C ../carla-lilv

../carla-native/carla_native.a:
	$(MAKE) -C ../carla-native

../carla-plugin/carla_plugin.a:
	$(MAKE) -C ../carla-plugin

../carla-rtmempool/carla_rtmempool.a:
	$(MAKE) -C ../carla-rtmempool

clean:
	rm -f $(OBJS) *.a *.so *.dll

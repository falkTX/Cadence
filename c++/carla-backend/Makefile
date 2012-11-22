#!/usr/bin/make -f
# Makefile for carla-backend #
# ------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

HAVE_JACK         = $(shell pkg-config --exists jack && echo true)

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
HAVE_SUIL         = $(shell pkg-config --exists suil-0 && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)
HAVE_ZYN_GUI_DEPS = $(shell pkg-config --exists ntk ntk_images && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS  += -I. -I../carla-engine -I../carla-includes -I../carla-native -I../carla-plugin -I../carla-utils
BUILD_CXX_FLAGS  += -fvisibility=hidden -fPIC
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags liblo QtCore)

LINK_FLAGS       += -fPIC -shared -ldl -lm -lGL
LINK_FLAGS       += $(shell pkg-config --libs liblo QtCore QtGui)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS  += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS  += -DCARLA_ENGINE_RTAUDIO
endif

ifeq ($(HAVE_JACK),true)
LINK_FLAGS       += $(shell pkg-config --libs jack)
endif

ifeq ($(HAVE_ALSA),true)
LINK_FLAGS       += $(shell pkg-config --libs alsa)
endif

ifeq ($(HAVE_PULSEAUDIO),true)
LINK_FLAGS       += $(shell pkg-config --libs libpulse-simple)
endif

ifeq ($(HAVE_SUIL),true)
LINK_FLAGS       += $(shell pkg-config --libs suil-0)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS  += -DWANT_FLUIDSYNTH
LINK_FLAGS       += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS  += -DWANT_LINUXSAMPLER
LINK_FLAGS       += $(shell pkg-config --libs linuxsampler)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS       += $(shell pkg-config --libs fftw3 mxml)

ifeq ($(HAVE_ZYN_GUI_DEPS),true)
LINK_FLAGS       += $(shell pkg-config --libs ntk ntk_images)
endif
endif

OBJS = \
	carla_backend_standalone.o \

OBJS += ../carla-engine/carla_engine.a
OBJS += ../carla-plugin/carla_plugin.a
OBJS += ../carla-native/carla_native.a

# others
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
OBJS += ../carla-lilv/carla_lilv.a
OBJS += ../carla-rtmempool/carla_rtmempool.a
endif

# --------------------------------------------------------------

all: carla_backend.so

doxygen: carla_backend.doxygen
	doxygen $<

carla_backend.so: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

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

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(OBJS) *.so *.dll

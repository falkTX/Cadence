#!/usr/bin/make -f
# Makefile for carla-backend #
# ------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

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

carla_backend.dll: $(OBJS)
carla_backend.so: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@ && $(STRIP) $@

carla_backend.dylib: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -bundle -o $@ && $(STRIP) $@

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
	rm -f $(OBJS) *.dll *.dylib *.so

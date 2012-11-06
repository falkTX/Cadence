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

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
endif

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)
HAVE_ZYN_GUI_DEPS = $(shell pkg-config --exists fftw3 mxml ntk && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../carla-includes -I../carla-engine -I../carla-jackbridge -I../carla-native -I../carla-plugin -I../carla-utils
BUILD_CXX_FLAGS += -fPIC -DCARLA_ENGINE_JACK
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack liblo QtCore QtGui)

LINK_FLAGS      += -fPIC -shared -ldl -lm
LINK_FLAGS      += $(shell pkg-config --libs jack liblo QtCore QtGui)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

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

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__ -D__LINUX_ALSASEQ__
LINK_FLAGS       += $(shell pkg-config --libs alsa)
WANT_RTAUDIO      = true
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
LINK_FLAGS       += $(shell pkg-config --libs libpulse-simple)
WANT_RTAUDIO      = true
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS       += $(shell pkg-config --libs fftw3 mxml)
endif

ifeq ($(HAVE_ZYN_GUI_DEPS),true)
LINK_FLAGS       += $(shell pkg-config --libs ntk)
endif

# backend
OBJS = \
	carla_backend_standalone.o

# engine
OBJS += \
     ../carla-engine/carla_engine.o \
     ../carla-engine/carla_engine_jack.o \
     ../carla-engine/carla_engine_rtaudio.o \
     ../carla-engine/carla_osc.o \
     ../carla-engine/carla_shared.o \
     ../carla-engine/carla_threads.o

# plugins
OBJS += \
     ../carla-plugin/carla_bridge.o \
     ../carla-plugin/native.o \
     ../carla-plugin/ladspa.o \
     ../carla-plugin/dssi.o \
     ../carla-plugin/lv2.o \
     ../carla-plugin/vst.o \
     ../carla-plugin/fluidsynth.o \
     ../carla-plugin/linuxsampler.o

# native
OBJS += ../carla-native/carla_native.a

# others
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
OBJS += ../carla-lilv/carla_lilv.a
OBJS += ../carla-rtmempool/carla_rtmempool.a
endif

ifeq ($(WANT_RTAUDIO),true)
BUILD_CXX_FLAGS += -DCARLA_ENGINE_RTAUDIO -DHAVE_GETTIMEOFDAY -D_FORTIFY_SOURCE=2
BUILD_CXX_FLAGS += -I../carla-engine/rtaudio-4.0.11 -I../carla-engine/rtmidi-2.0.1
OBJS += ../carla-engine/rtaudio-4.0.11/RtAudio.o
OBJS += ../carla-engine/rtmidi-2.0.1/RtMidi.o
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

../carla-lilv/carla_lilv.a:
	$(MAKE) -C ../carla-lilv

../carla-native/carla_native.a:
	$(MAKE) -C ../carla-native

../carla-rtmempool/carla_rtmempool.a:
	$(MAKE) -C ../carla-rtmempool

clean:
	rm -f $(OBJS) *.a *.so *.dll

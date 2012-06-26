#!/usr/bin/make -f
# Makefile for carla-backend #
# ------------------------------------- #
# Created by falkTX
#

CC  ?= gcc
CXX ?= g++

BASE_FLAGS = -O2 -ffast-math -fomit-frame-pointer -fPIC -mtune=generic -msse -Wall -I. -I../carla-includes

HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
HAVE_SUIL         = $(shell pkg-config --exists suil-0 && echo true)

CARLA_C_FLAGS     = $(BASE_FLAGS) $(CFLAGS)

CARLA_CXX_FLAGS   = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
CARLA_CXX_FLAGS  += $(shell pkg-config --cflags jack liblo QtCore QtGui) -DCARLA_BACKEND_NO_NAMESPACE -DCARLA_ENGINE_JACK
# CARLA_CXX_FLAGS  += $(shell pkg-config --cflags jack liblo QtCore QtGui) -DCARLA_BACKEND_NO_NAMESPACE -DCARLA_ENGINE_RTAUDIO -D_FORTIFY_SOURCE=2 -DHAVE_GETTIMEOFDAY -D__UNIX_JACK__ -D__LINUX_ALSA__ -D__LINUX_PULSE__
CARLA_CXX_FLAGS  += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
# CARLA_CXX_FLAGS  += -DVESTIGE_HEADER -I../carla-includes/vestige # Comment this line to not use vestige header

CARLA_LD_FLAGS    = -shared -ldl -fPIC $(LDFLAGS)
CARLA_LD_FLAGS   += $(shell pkg-config --libs jack liblo QtCore QtGui)

ifeq ($(HAVE_FLUIDSYNTH),true)
CARLA_CXX_FLAGS  += $(shell pkg-config --cflags fluidsynth) -DWANT_FLUIDSYNTH
CARLA_LD_FLAGS   += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
CARLA_CXX_FLAGS  += $(shell pkg-config --cflags linuxsampler) -DWANT_LINUXSAMPLER
CARLA_LD_FLAGS   += $(shell pkg-config --libs linuxsampler)
endif

ifeq ($(HAVE_SUIL),true)
CARLA_CXX_FLAGS  += $(shell pkg-config --cflags suil-0) -DHAVE_SUIL
CARLA_LD_FLAGS   += $(shell pkg-config --libs suil-0)
endif

OBJS = \
	carla_backend.o \
	carla_bridge.o \
	carla_engine_jack.o \
	carla_engine_rtaudio.o \
	carla_osc.o \
	carla_shared.o \
	carla_threads.o \
	ladspa.o dssi.o lv2.o vst.o fluidsynth.o linuxsampler.o \
	lv2-rtmempool/rtmempool.o \
	../carla-lilv/carla_lilv.a

# OBJS += rtaudio/rtaudio-4.0.11/RtAudio.o
# CARLA_CXX_FLAGS += -Irtaudio/rtaudio-4.0.11
# CARLA_LD_FLAGS  += -lasound -lpulse -lpulse-simple

# --------------------------------------------------------------

all: carla_backend.so

carla_backend.so: $(OBJS)
	$(CXX) $^ $(CARLA_LD_FLAGS) -o $@ && strip $@

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(CARLA_C_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(CARLA_CXX_FLAGS) -o $@

clean:
	rm -f *.o lv2-rtmempool/*.o *.so *.dll

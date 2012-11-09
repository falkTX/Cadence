#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_JACK        = $(shell pkg-config --exists jack && echo true)

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA        = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO  = $(shell pkg-config --exists libpulse-simple && echo true)
endif

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC
BUILD_CXX_FLAGS += -I. -I../carla-backend -I../carla-includes -I../carla-plugin -I../carla-utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# BUILD_CXX_FLAGS += -DWANT_LV2
endif

ifeq ($(HAVE_JACK),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack)
WANT_JACK        = true
endif

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__ -D__LINUX_ALSASEQ__
WANT_RTAUDIO     = true
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
WANT_RTAUDIO     = true
endif

OBJS = \
     carla_engine.o \
     carla_engine_osc.o \
     carla_engine_thread.o \
     jack.o \
     rtaudio.o

# --------------------------------------------------------------

ifeq ($(WANT_JACK),true)
BUILD_CXX_FLAGS += -DCARLA_ENGINE_JACK
BUILD_CXX_FLAGS += -I../carla-jackbridge
endif

ifeq ($(WANT_RTAUDIO),true)
BUILD_CXX_FLAGS += -DCARLA_ENGINE_RTAUDIO -DHAVE_GETTIMEOFDAY -D_FORTIFY_SOURCE=2
BUILD_CXX_FLAGS += -Irtaudio-4.0.11 -Irtmidi-2.0.1
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__
endif
OBJS += rtaudio-4.0.11/RtAudio.o
OBJS += rtmidi-2.0.1/RtMidi.o
endif

# --------------------------------------------------------------

all: carla_engine.a

doc: carla_engine.doxygen
	doxygen $<

carla_engine.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a $(OBJS)

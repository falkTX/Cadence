#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)
HAVE_ZYN_GUI_DEPS = $(shell pkg-config --exists ntk && echo true)

# --------------------------------------------------------------

BUILD_C_FLAGS    += -fvisibility=hidden -fPIC -I.
BUILD_CXX_FLAGS  += -fvisibility=hidden -fPIC -I. -Idistrho -I../carla-includes -I../carla-utils -I../distrho-plugin-toolkit
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags QtCore)

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags fftw3 mxml) -DWANT_ZYNADDSUBFX

ifeq ($(HAVE_ZYN_GUI_DEPS),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags ntk) -DWANT_ZYNADDSUBFX_GUI
endif
endif

OBJS  = \
	bypass.o \
	midi-split.o

OBJS += \
	3bandeq.o \
	3bandsplitter.o \
	distrho/pugl.o

# --------------------------------------------------------------

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.o \
	zynaddsubfx-src.o
endif

# --------------------------------------------------------------

all: carla_native.a

doxygen: carla_native.doxygen
	doxygen $<

carla_native.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(BUILD_C_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a $(OBJS)

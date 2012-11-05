#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_ZYN_DEPS    = $(shell pkg-config --exists fftw3 mxml && echo true)

BUILD_CXX_FLAGS += -I. -I../carla-includes -I../carla-utils -fPIC
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fftw3 mxml)
endif

OBJS = \
	bypass.o \
	midi-split.o

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.o \
	zynaddsubfx-src.o
endif

# --------------------------------------------------------------

all: carla_native.a

doc: carla_native.doxygen
	doxygen $<

carla_native.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(BUILD_C_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a *.o

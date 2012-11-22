#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)
HAVE_ZYN_GUI_DEPS = $(shell pkg-config --exists ntk ntk_images && echo true)
# --------------------------------------------------------------

BUILD_C_FLAGS    += -fvisibility=hidden -fPIC -I.
BUILD_CXX_FLAGS  += -fvisibility=hidden -fPIC -I. -I../carla-includes -I../carla-utils -I../distrho-plugin-toolkit
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags QtCore)

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags fftw3 mxml) -DWANT_ZYNADDSUBFX
ifeq ($(HAVE_ZYN_GUI_DEPS),true)
BUILD_CXX_FLAGS  += $(shell pkg-config --cflags ntk ntk_images) -DWANT_ZYNADDSUBFX_GUI
BUILD_CXX_FLAGS  += -DNTK_GUI=1 -DVSTAUDIOOUT=1
endif
endif

OBJS  = \
	bypass.o \
	midi-split.o

# OBJS += \
# 	3bandeq.o \
# 	3bandsplitter.o \
# 	distrho/pugl.o

# OBJS += \
# 	3bandeq.a \
# 	3bandsplitter.a

# --------------------------------------------------------------

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += \
	zynaddsubfx.o \
	zynaddsubfx.a
endif

# --------------------------------------------------------------

all: carla_native.a

doxygen: carla_native.doxygen
	doxygen $<

carla_native.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

3bandeq.a:
	$(MAKE) -C 3bandeq

3bandsplitter.a:
	$(MAKE) -C 3bandsplitter

zynaddsubfx.a:
	$(MAKE) -C zynaddsubfx

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(BUILD_C_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f *.a $(OBJS)
	$(MAKE) clean -C 3bandeq
	$(MAKE) clean -C 3bandsplitter
	$(MAKE) clean -C zynaddsubfx

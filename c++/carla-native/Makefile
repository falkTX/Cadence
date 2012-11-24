#!/usr/bin/make -f
# Makefile for carla-native #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += -fvisibility=hidden -fPIC -I.
BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I. -I../carla-includes -I../carla-utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)

ifeq ($(HAVE_ZYN_DEPS),true)
ZYN_CXX_FLAGS    = $(shell pkg-config --cflags fftw3 mxml)
endif

OBJS  = \
	bypass.o \
	midi-split.o

OBJS += \
	distrho-3bandeq.o \
	distrho-3bandsplitter.o \
	distrho-pugl.o

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS += zynaddsubfx.o zynaddsubfx-src.o
endif

# --------------------------------------------------------------

all: carla_native.a

doxygen: carla_native.doxygen
	doxygen $<

carla_native.a: $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

distrho-3bandeq.o: distrho-3bandeq.cpp
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -I../distrho-plugin-toolkit -I3bandeq -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ -o $@

distrho-3bandsplitter.o: distrho-3bandsplitter.cpp
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -I../distrho-plugin-toolkit -I3bandsplitter -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter -o $@

distrho-pugl.o: distrho/pugl.cpp
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -I../distrho-plugin-toolkit -o $@

zynaddsubfx.o: zynaddsubfx.cpp
	$(CXX) -c $< $(BUILD_CXX_FLAGS) $(ZYN_CXX_FLAGS) -o $@

zynaddsubfx-src.o: zynaddsubfx-src.cpp
	$(CXX) -c $< $(BUILD_CXX_FLAGS) $(ZYN_CXX_FLAGS) -o $@

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(BUILD_C_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	rm -f *.a $(OBJS)

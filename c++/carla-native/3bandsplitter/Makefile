#!/usr/bin/make -f
# Makefile for carla-native plugins #
# ----------------------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DDISTRHO_NAMESPACE=DISTRHO_3BandSplitter
BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I. -I../../distrho-plugin-toolkit

OBJS += \
	DistrhoArtwork3BandSplitter.o \
	DistrhoPlugin3BandSplitter.o \
	DistrhoUI3BandSplitter.o

TARGET = ../3bandsplitter.a

# --------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(TARGET) $(OBJS)

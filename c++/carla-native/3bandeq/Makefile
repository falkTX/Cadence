#!/usr/bin/make -f
# Makefile for carla-native plugins #
# ----------------------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DDISTRHO_NAMESPACE=DISTRHO_3BandEQ
BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I. -I../../distrho-plugin-toolkit

OBJS += \
	DistrhoArtwork3BandEQ.o \
	DistrhoPlugin3BandEQ.o \
	DistrhoUI3BandEQ.o

TARGET = ../3bandeq.a

# --------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(TARGET) $(OBJS)

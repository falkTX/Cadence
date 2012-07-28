#!/usr/bin/make -f
# Makefile for jackmeter #
# -------------------------------- #
# Created by falkTX
#

CXX   ?= g++
STRIP ?= strip
PKGCONFIG ?= pkg-config

BASE_FLAGS   = -O2 -ffast-math -fomit-frame-pointer -fPIC -mtune=generic -msse -Wall

BUILD_FLAGS  = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
BUILD_FLAGS += $(shell $(PKGCONFIG) --cflags QtCore QtGui jack)
BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT

LINK_FLAGS   = $(LDFLAGS)
LINK_FLAGS  += $(shell $(PKGCONFIG) --libs QtCore QtGui jack)

OBJS = \
	jackmeter.o \
	../widgets/digitalpeakmeter.o

# --------------------------------------------------------------

all: cadence_jackmeter

cadence_jackmeter: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@ && strip $@

cadence_jackmeter.exe: $(OBJS)
	$(CXX) $^ -static $(LINK_FLAGS) -o $@ && strip $@

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_FLAGS) -o $@

clean:
	rm -f $(OBJS) cadence_jackmeter cadence_jackmeter.exe

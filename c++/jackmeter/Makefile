#!/usr/bin/make -f
# Makefile for jackmeter #
# -------------------------------- #
# Created by falkTX
#

CXX   ?= g++
STRIP ?= strip

BASE_FLAGS   = -O2 -ffast-math -fomit-frame-pointer -fPIC -mtune=generic -msse -mfpmath=sse -Wall

BUILD_FLAGS  = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
BUILD_FLAGS += $(shell pkg-config --cflags QtCore QtGui jack)
BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT

LINK_FLAGS   = $(LDFLAGS)
LINK_FLAGS  += $(shell pkg-config --libs QtCore QtGui jack)

ifeq ($(shell pkg-config --atleast-version=0.121.0 jack && echo true),true)
BUILD_FLAGS += -DHAVE_JACKSESSION
endif

OBJS = jackmeter.o \
	../widgets/digitalpeakmeter.o

# --------------------------------------------------------------

all: cadence_jackmeter

cadence_jackmeter: $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@ && strip $@

cadence_jackmeter.exe: $(OBJS)
	$(CXX) $^ -static -mwindows $(LINK_FLAGS) -o $@ && strip $@

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_FLAGS) -o $@

clean:
	rm -f $(OBJS) cadence_jackmeter*

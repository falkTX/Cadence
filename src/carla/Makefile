#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

CC  ?= gcc
CXX ?= g++

CARLA_BUILD_FLAGS  = -Wall -std=c++0x -fPIC -I. -I../carla-includes `pkg-config --cflags jack fluidsynth liblo QtCore QtGui` $(CXXFLAGS)
CARLA_BUILD_FLAGS += -DDEBUG -O0 -g
# CARLA_BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT -O2 -fvisibility=hidden -ffast-math -fomit-frame-pointer -mtune=generic -msse
CARLA_LINK_FLAGS   = -shared -fPIC -ldl `pkg-config --libs jack fluidsynth liblo QtCore QtGui` $(LDFLAGS)

OBJS  = carla_backend.o jack.o ladspa.o
# osc.o misc.o dssi.o lv2.o vst.o winvst.o sf2.o lv2-rtmempool/rtmempool.o


all: carla_backend.so

carla_backend.so: $(OBJS)
	$(CXX) $(OBJS) $(CARLA_LINK_FLAGS) -o carla_backend.so

# carla_backend.dll: $(OBJS) windows.def
# 	$(CXX) $(OBJS) windows.def $(CARLA_LINK_FLAGS) -o carla_backend.dll

.c.o:
	$(CC) -c $< $(CARLA_BUILD_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(CARLA_BUILD_FLAGS) -o $@

clean:
	rm -f *.o *.so *.dll
# lv2-rtmempool/*.o


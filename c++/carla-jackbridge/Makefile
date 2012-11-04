#!/usr/bin/make -f
# Makefile for carla-jackbridge #
# ----------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

WINECXX ?= wineg++

BUILD_CXX_FLAGS += -I../carla-includes
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack QtCore)
LINK_FLAGS      += -shared

WIN_BUILD_FLAGS  = $(BUILD_CXX_FLAGS) -DJACKBRIDGE_DUMMY -I.
WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
WIN_LINK_FLAGS   = $(LINK_FLAGS)

WINE_BUILD_FLAGS = $(BUILD_CXX_FLAGS) -fPIC
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_FLAGS) $(shell pkg-config --libs jack) -ldl

# --------------------------------------------------------------

all:

win32:   libcarla-jackbridge-win32.dll
win64:   libcarla-jackbridge-win64.dll
wine32:  libcarla-jackbridge-win32.dll.so
wine64:  libcarla-jackbridge-win64.dll.so

# --------------------------------------------------------------

OBJS = carla_jackbridge.cpp

libcarla-jackbridge-win32.dll: $(OBJS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ && $(STRIP) $@

libcarla-jackbridge-win64.dll: $(OBJS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ && $(STRIP) $@

libcarla-jackbridge-win32.dll.so: $(OBJS)
	$(WINECXX) $^ libcarla-jackbridge-win32.dll.def $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -mno-cygwin -o $@ && $(STRIP) $@

libcarla-jackbridge-win64.dll.so: $(OBJS)
	$(WINECXX) $^ libcarla-jackbridge-win64.dll.def $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -mno-cygwin -o $@ && $(STRIP) $@

# --------------------------------------------------------------

clean:
	rm -f *.o *.exe *.so libcarla-jackbridge-*

#!/usr/bin/make -f
# Makefile for xycontroller #
# ------------------------------------ #
# Created by falkTX
#

CXX   ?= g++
MOC   ?= moc
RCC   ?= rcc
UIC   ?= uic
STRIP ?= strip
WINDRES ?= windres

BASE_FLAGS   = -O2 -ffast-math -fomit-frame-pointer -mtune=generic -msse -mfpmath=sse -Wall -I../widgets

BUILD_FLAGS  = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
BUILD_FLAGS += $(shell pkg-config --cflags QtCore QtGui jack)
BUILD_FLAGS += -DNDEBUG -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT

LINK_FLAGS   = $(LDFLAGS)
LINK_FLAGS  += $(shell pkg-config --libs QtCore QtGui jack)

ifeq ($(shell pkg-config --atleast-version=0.121.0 jack && echo true),true)
BUILD_FLAGS += -DHAVE_JACKSESSION
endif

FILES = \
	xycontroller.moc \
	ui_xycontroller.h \
	../widgets/moc_pixmapkeyboard.cpp \
	../../resources/qrc_resources.cpp

OBJS  = xycontroller.o \
	../widgets/pixmapdial.o \
	../widgets/pixmapkeyboard.o \
	../widgets/moc_pixmapkeyboard.o \
	../../resources/qrc_resources.o

# --------------------------------------------------------------

all: cadence_xycontroller

cadence_xycontroller: $(FILES) $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -o $@ && strip $@

cadence_xycontroller.exe: $(FILES) $(OBJS) icon.o
	$(CXX) $(OBJS) icon.o -static -mwindows $(LINK_FLAGS) -o $@ && strip $@

# --------------------------------------------------------------

xycontroller.moc: xycontroller.cpp
	$(MOC) $< -o $@

ui_xycontroller.h: ../../src/ui/xycontroller.ui
	$(UIC) $< -o $@

icon.o: ../../resources/ico/cadence.rc
	$(WINDRES) -i $< -o $@ -O coff

../widgets/moc_pixmapkeyboard.cpp: ../widgets/pixmapkeyboard.h
	$(MOC) $< -o $@

../../resources/qrc_resources.cpp: ../../resources/resources.qrc
	$(RCC) -name resources $< -o $@

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_FLAGS) -o $@

clean:
	rm -f $(FILES) $(OBJS) icon.o cadence_xycontroller*

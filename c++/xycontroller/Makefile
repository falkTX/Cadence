#!/usr/bin/make -f
# Makefile for xycontroller #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

BUILD_CXX_FLAGS += -I../widgets
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui jack)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui jack)

ifeq ($(shell pkg-config --atleast-version=0.121.0 jack && echo true),true)
BUILD_CXX_FLAGS += -DHAVE_JACKSESSION
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

all: cadence-xycontroller

cadence-xycontroller: $(FILES) $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -o $@ && $(STRIP) $@

cadence-xycontroller.exe: $(FILES) $(OBJS) icon.o
	$(CXX) $(OBJS) icon.o -static -mwindows $(LINK_FLAGS) -o $@ && $(STRIP) $@

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
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(FILES) $(OBJS) icon.o cadence-xycontroller*

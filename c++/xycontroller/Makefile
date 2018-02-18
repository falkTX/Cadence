#!/usr/bin/make -f
# Makefile for xycontroller #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../widgets
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)

ifeq ($(HAVE_JACKSESSION),true)
BUILD_CXX_FLAGS += -DHAVE_JACKSESSION
endif

# --------------------------------------------------------------

FILES = \
	xycontroller.moc \
	ui_xycontroller.h \
	qrc_resources-xycontroller.cpp \
	../widgets/moc_pixmapkeyboard.cpp

OBJS  = xycontroller.o \
	qrc_resources-xycontroller.o \
	../widgets/pixmapdial.o \
	../widgets/pixmapkeyboard.o \
	../widgets/moc_pixmapkeyboard.o

# --------------------------------------------------------------

all: cadence-xycontroller

cadence-xycontroller: $(FILES) $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -ldl -o $@ && $(STRIP) $@

cadence-xycontroller.exe: $(FILES) $(OBJS) icon.o
	$(CXX) $(OBJS) icon.o $(LINK_FLAGS) -limm32 -lole32 -luuid -lwinspool -lws2_32 -mwindows -o $@ && $(STRIP) $@

# --------------------------------------------------------------

xycontroller.moc: xycontroller.cpp
	$(MOC) $< -o $@

ui_xycontroller.h: ../../resources/ui/xycontroller.ui
	$(UIC) $< -o $@

qrc_resources-xycontroller.cpp: ../../resources/resources-xycontroller.qrc
	$(RCC) -name resources-xycontroller $< -o $@

icon.o: ../../resources/ico/cadence.rc
	$(WINDRES) -i $< -o $@ -O coff

../widgets/moc_pixmapkeyboard.cpp: ../widgets/pixmapkeyboard.hpp
	$(MOC) $< -o $@

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(FILES) $(OBJS) icon.o cadence-xycontroller*

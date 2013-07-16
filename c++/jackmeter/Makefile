#!/usr/bin/make -f
# Makefile for jackmeter #
# -------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../widgets
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui jack)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui jack)

ifeq ($(HAVE_JACKSESSION),true)
BUILD_CXX_FLAGS += -DHAVE_JACKSESSION
endif

# --------------------------------------------------------------

FILES = \
	qrc_resources-jackmeter.cpp

OBJS = \
	jackmeter.o \
	qrc_resources-jackmeter.o \
	../widgets/digitalpeakmeter.o

# --------------------------------------------------------------

all: cadence-jackmeter

cadence-jackmeter: $(FILES) $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -ldl -o $@ && $(STRIP) $@

cadence-jackmeter.exe: $(FILES) $(OBJS) icon.o
	$(CXX) $(OBJS) icon.o $(LINK_FLAGS) -limm32 -lole32 -luuid -lwinspool -lws2_32 -mwindows -o $@ && $(STRIP) $@

# --------------------------------------------------------------

qrc_resources-jackmeter.cpp: ../../resources/resources-jackmeter.qrc
	$(RCC) -name resources-jackmeter $< -o $@

icon.o: ../../resources/ico/cadence.rc
	$(WINDRES) -i $< -o $@ -O coff

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(FILES) $(OBJS) icon.o cadence-jackmeter*

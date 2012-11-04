#!/usr/bin/make -f
# Makefile for jackmeter #
# -------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

BUILD_CXX_FLAGS += -I../widgets
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui jack)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui jack)

ifeq ($(shell pkg-config --atleast-version=0.121.0 jack && echo true),true)
BUILD_CXX_FLAGS += -DHAVE_JACKSESSION
endif

OBJS = \
	jackmeter.o \
	../widgets/digitalpeakmeter.o

# --------------------------------------------------------------

all: cadence-jackmeter

cadence-jackmeter: $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -o $@ && $(STRIP) $@

cadence-jackmeter.exe: $(OBJS) icon.o
	$(CXX) $(OBJS) icon.o $(LINK_FLAGS) -static -mwindows -o $@ && $(STRIP) $@

icon.o: ../../resources/ico/cadence.rc
	$(WINDRES) -i $< -o $@ -O coff

# --------------------------------------------------------------

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(OBJS) cadence-jackmeter*

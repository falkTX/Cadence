#!/usr/bin/make -f
# Makefile for caitlib #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

BUILD_C_FLAGS += -D_GNU_SOURCE -I.
BUILD_C_FLAGS += -fvisibility=hidden -fPIC -std=gnu99 -Werror
BUILD_C_FLAGS += $(shell pkg-config --cflags jack)

LINK_FLAGS    += -fPIC -shared -lm -lpthread
LINK_FLAGS    += $(shell pkg-config --libs jack)

OBJS = \
	caitlib.o \
	memory_atomic.o \

# --------------------------------------------------------------

all: caitlib.so test

doxygen: caitlib.doxygen
	doxygen $<

caitlib.so: $(OBJS)
	$(CC) $^ $(LINK_FLAGS) -o $@ && $(STRIP) $@

test: $(OBJS) test.o
	$(CC) $^ -lm -lpthread -ljack -o $@ && $(STRIP) $@

# --------------------------------------------------------------

.c.o:
	$(CC) -c $< $(BUILD_C_FLAGS) -o $@

clean:
	rm -f $(OBJS) *.so *.dll

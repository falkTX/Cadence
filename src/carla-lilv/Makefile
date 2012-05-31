#!/usr/bin/make -f
# Makefile for static lilv #
# ------------------------------------- #
# Created by falkTX
#

CC ?= gcc
AR ?= ar

SERD_VERSION   = 0.14.0
SORD_VERSION   = 0.8.0
SRATOM_VERSION = 0.2.0
LILV_VERSION   = 0.14.2

BUILD_FLAGS  = -O2 -fvisibility=hidden -fPIC -mtune=generic -std=c99 -Wall $(CFLAGS)
BUILD_FLAGS += -Iconfig -I../carla-includes

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64

OBJS       = serd.o sord.o sratom.o lilv.o
OBJS_32bit = serd_32bit.o sord_32bit.o sratom_32bit.o lilv_32bit.o
OBJS_64bit = serd_64bit.o sord_64bit.o sratom_64bit.o lilv_64bit.o

# --------------------------------------------------------------

all: carla_lilv.a

carla_lilv.a: $(OBJS)
	$(AR) rs $@ $(OBJS)

serd.o: serd.c
	$(CC) $< $(BUILD_FLAGS) -c -o $@

sord.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src -c -o $@

sratom.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) -c -o $@

lilv.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src -c -o $@

# --------------------------------------------------------------

32bit: carla_lilv_32bit.a

carla_lilv_32bit.a: $(OBJS_32bit)
	$(AR) rs $@ $(OBJS_32bit)

serd_32bit.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

sord_32bit.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom_32bit.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

lilv_32bit.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

64bit: carla_lilv_64bit.a

carla_lilv_64bit.a: $(OBJS_64bit)
	$(AR) rs $@ $(OBJS_64bit)

serd_64bit.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

sord_64bit.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom_64bit.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

lilv_64bit.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.a *.o


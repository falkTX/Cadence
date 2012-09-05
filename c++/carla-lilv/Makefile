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

BUILD_FLAGS  = -O2 -ffast-math -fomit-frame-pointer -fvisibility=hidden -fPIC -mtune=generic -msse -mfpmath=sse -std=c99 -Wall $(CFLAGS)
BUILD_FLAGS += -Iconfig -I../carla-includes

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64

OBJS         = serd.o sord.o sratom.o lilv.o
OBJS_posix32 = serd_posix32.o sord_posix32.o sratom_posix32.o lilv_posix32.o
OBJS_posix64 = serd_posix64.o sord_posix64.o sratom_posix64.o lilv_posix64.o
OBJS_win32   = serd_win32.o sord_win32.o sratom_win32.o lilv_win32.o
OBJS_win64   = serd_win64.o sord_win64.o sratom_win64.o lilv_win64.o

# --------------------------------------------------------------

all: carla_lilv.a

carla_lilv.a: $(OBJS)
	$(AR) rs $@ $^

serd.o: serd.c
	$(CC) $< $(BUILD_FLAGS) -c -o $@

sord.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src -c -o $@

sratom.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) -c -o $@

lilv.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src -c -o $@

# --------------------------------------------------------------

posix32: carla_lilv_posix32.a

carla_lilv_posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

serd_posix32.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

sord_posix32.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom_posix32.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

lilv_posix32.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

posix64: carla_lilv_posix64.a

carla_lilv_posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

serd_posix64.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

sord_posix64.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom_posix64.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

lilv_posix64.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

win32: carla_lilv_win32.a

carla_lilv_win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

serd_win32.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

sord_win32.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

sratom_win32.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

lilv_win32.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

win64: carla_lilv_win64.a

carla_lilv_win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

serd_win64.o: serd.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

sord_win64.o: sord.c
	$(CC) $< $(BUILD_FLAGS) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

sratom_win64.o: sratom.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

lilv_win64.o: lilv.c
	$(CC) $< $(BUILD_FLAGS) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.a *.o


#!/usr/bin/make -f
# Makefile for static rtmempool #
# ----------------------------------------- #
# Created by falkTX
#

CC ?= gcc
AR ?= ar

BUILD_FLAGS  = -O2 -ffast-math -fomit-frame-pointer -fvisibility=hidden -fPIC -mtune=generic -msse -mfpmath=sse -std=c99 -Wall $(CFLAGS)
BUILD_FLAGS += -I../carla-includes

32BIT_FLAGS  = -m32
64BIT_FLAGS  = -m64

OBJS         = rtmempool.o
OBJS_posix32 = rtmempool_posix32.o
OBJS_posix64 = rtmempool_posix64.o
OBJS_win32   = rtmempool_win32.o
OBJS_win64   = rtmempool_win64.o

# --------------------------------------------------------------

all: carla_rtmempool.a

carla_rtmempool.a: $(OBJS)
	$(AR) rs $@ $^

rtmempool.o: rtmempool.c
	$(CC) $< $(BUILD_FLAGS) -c -o $@

# --------------------------------------------------------------

posix32: carla_rtmempool_posix32.a

carla_rtmempool_posix32.a: $(OBJS_posix32)
	$(AR) rs $@ $^

rtmempool_posix32.o: rtmempool.c
	$(CC) $< $(BUILD_FLAGS) $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

posix64: carla_rtmempool_posix64.a

carla_rtmempool_posix64.a: $(OBJS_posix64)
	$(AR) rs $@ $^

rtmempool_posix64.o: rtmempool.c
	$(CC) $< $(BUILD_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

win32: carla_rtmempool_win32.a

carla_rtmempool_win32.a: $(OBJS_win32)
	$(AR) rs $@ $^

rtmempool_win32.o: rtmempool.c
	$(CC) $< $(BUILD_FLAGS) -DPTW32_STATIC_LIB $(32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

win64: carla_rtmempool_win64.a

carla_rtmempool_win64.a: $(OBJS_win64)
	$(AR) rs $@ $^

rtmempool_win64.o: rtmempool.c
	$(CC) $< $(BUILD_FLAGS) -DPTW32_STATIC_LIB $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	rm -f *.a *.o

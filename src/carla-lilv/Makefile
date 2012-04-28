#!/usr/bin/make -f
# Makefile for static lilv #
# ------------------------------------- #
# Created by falkTX
#

CC  ?= gcc

SERD_VERSION   = 0.14.0
SORD_VERSION   = 0.8.0
SRATOM_VERSION = 0.2.0
LILV_VERSION   = 0.14.0

BUILD_FLAGS  = -O2 -fvisibility=hidden -fPIC -mtune=generic -std=c99
BUILD_FLAGS += -I. -I../carla-includes
BUILD_FLAGS += -Iserd-$(SERD_VERSION)
BUILD_FLAGS += -Isord-$(SORD_VERSION)
BUILD_FLAGS += -Isratom-$(SRATOM_VERSION)
BUILD_FLAGS += -Ililv-$(LILV_VERSION)
LINK_FLAGS   = -lm -static

SERD_OBJS    = serd-${SERD_VERSION}/src/env.o
SERD_OBJS   += serd-${SERD_VERSION}/src/node.o
SERD_OBJS   += serd-${SERD_VERSION}/src/reader.o
SERD_OBJS   += serd-${SERD_VERSION}/src/string.o
SERD_OBJS   += serd-${SERD_VERSION}/src/uri.o
SERD_OBJS   += serd-${SERD_VERSION}/src/writer.o

SORD_OBJS    = sord-${SORD_VERSION}/src/sord.o
SORD_OBJS   += sord-${SORD_VERSION}/src/syntax.o
SORD_OBJS   += sord-${SORD_VERSION}/src/zix/hash.o
SORD_OBJS   += sord-${SORD_VERSION}/src/zix/tree.o

SRATOM_OBJS  = sratom-${SRATOM_VERSION}/src/sratom.o

LILV_OBJS    = lilv-${LILV_VERSION}/src/collections.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/instance.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/lib.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/node.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/plugin.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/pluginclass.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/port.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/query.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/scalepoint.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/state.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/ui.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/util.o
LILV_OBJS   += lilv-${LILV_VERSION}/src/world.o

all: carla_lilv.a

carla_lilv.a: $(SERD_OBJS) $(SORD_OBJS) $(SRATOM_OBJS) $(LILV_OBJS)
	ar rs carla_lilv.a $(SERD_OBJS) $(SORD_OBJS) $(SRATOM_OBJS) $(LILV_OBJS)

clean:
	rm -f $(SERD_OBJS) $(SORD_OBJS) $(SRATOM_OBJS) $(LILV_OBJS) carla_lilv.a

.c.o:
	$(CC) $< $(BUILD_FLAGS) -c -o $@

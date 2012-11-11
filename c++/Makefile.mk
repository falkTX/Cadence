#!/usr/bin/make -f
# Makefile for Cadence C++ code #
# ------------------------------------ #
# Created by falkTX
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++
MOC ?= $(shell pkg-config QtCore --variable=moc_location)
RCC ?= $(shell pkg-config QtCore --variable=rcc_location)
UIC ?= $(shell pkg-config QtCore --variable=uic_location)
STRIP ?= strip
WINDRES ?= windres

DEBUG ?= false

ifeq ($(DEBUG),true)
BASE_FLAGS  = -O0 -g -Wall
BASE_FLAGS += -DDEBUG
STRIP       = true # FIXME
else
BASE_FLAGS  = -O2 -ffast-math -mtune=generic -msse -mfpmath=sse -Wall
BASE_FLAGS += -DNDEBUG
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)

ifneq ($(DEBUG),true)
BUILD_CXX_FLAGS += -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
endif

# Comment this line to not use vestige header
BUILD_CXX_FLAGS += -DVESTIGE_HEADER

# Modify to enable/disable specific features
CARLA_PLUGIN_SUPPORT   = true
CARLA_SAMPLERS_SUPPORT = true
CARLA_RTAUDIO_SUPPORT  = true

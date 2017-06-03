#!/usr/bin/make -f
# Makefile for Cadence C++ code #
# -------------------------------------------- #
# Created by falkTX
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++
MOC ?= $(shell pkg-config --variable=moc_location QtCore)
RCC ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC ?= $(shell pkg-config --variable=uic_location QtCore)
WINDRES ?= windres

# --------------------------------------------------------------

DEBUG ?= false

ifeq ($(DEBUG),true)
BASE_FLAGS  = -O0 -g -Wall -Wextra
BASE_FLAGS += -DDEBUG
else
BASE_FLAGS  = -O2 -ffast-math -mtune=generic -msse -mfpmath=sse -Wall -Wextra
BASE_FLAGS += -DNDEBUG
endif

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)

ifneq ($(DEBUG),true)
BUILD_CXX_FLAGS += -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
endif

ifneq ($(SKIP_STRIPPING),true)
LINK_FLAGS += -Wl,--strip-all
endif

# --------------------------------------------------------------

# Currently broken
# HAVE_JACKSESSION = $(shell pkg-config --atleast-version=0.121.0 jack && echo true)

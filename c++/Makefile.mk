#!/usr/bin/make -f
# Makefile for Cadence C++ code #
# -------------------------------------------- #
# Created by falkTX
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++
STRIP ?= strip
WINDRES ?= windres

HOSTBINS = $(shell pkg-config --variable=host_bins Qt5Core)
MOC ?= $(HOSTBINS)/moc
RCC ?= $(HOSTBINS)/rcc
UIC ?= $(HOSTBINS)/uic

# --------------------------------------------------------------

DEBUG ?= false

ifeq ($(DEBUG),true)
BASE_FLAGS  = -O0 -g -Wall -Wextra
BASE_FLAGS += -DDEBUG
else
BASE_FLAGS  = -O3 -ffast-math -Wall -Wextra
BASE_FLAGS += -DNDEBUG
endif

BASE_FLAGS += -fPIC

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

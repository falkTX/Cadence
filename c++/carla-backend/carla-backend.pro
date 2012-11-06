# QtCreator project file

QT = core gui

CONFIG     = debug
CONFIG    += link_pkgconfig qt warn_on # plugin shared

DEFINES    = DEBUG
DEFINES   += QTCREATOR_TEST

DEFINES   += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES   += WANT_SUIL
DEFINES   += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
DEFINES   += WANT_ZYNADDSUBFX WANT_ZYNADDSUBFX_GUI

PKGCONFIG  = jack liblo
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += fftw3 mxml
PKGCONFIG += suil-0

TARGET   = carla_backend
TEMPLATE = app
VERSION  = 0.5.0

# backend
SOURCES  = \
#     carla_backend_plugin.cpp \
     carla_backend_standalone.cpp

# engine
SOURCES += \
     ../carla-engine/carla_engine.cpp \
     ../carla-engine/carla_engine_jack.cpp \
     ../carla-engine/carla_engine_rtaudio.cpp \
     ../carla-engine/carla_osc.cpp \
     ../carla-engine/carla_shared.cpp \
     ../carla-engine/carla_threads.cpp

# plugins
SOURCES += \
     ../carla-plugin/carla_bridge.cpp \
     ../carla-plugin/native.cpp \
     ../carla-plugin/ladspa.cpp \
     ../carla-plugin/dssi.cpp \
     ../carla-plugin/lv2.cpp \
     ../carla-plugin/vst.cpp \
     ../carla-plugin/fluidsynth.cpp \
     ../carla-plugin/linuxsampler.cpp

# HEADERS = \
#     ../carla_backend.hpp \
#     ../carla_backend_standalone.hpp \
#     ../carla_engine.hpp \
#     ../carla_osc.hpp \
#     ../carla_plugin.hpp \
#     ../carla_shared.hpp \
#     ../carla_threads.hpp \
#     ../plugins/carla_native.h \
#     ../plugins/carla_nativemm.h \
#     ../../carla-includes/carla_defines.hpp \
#     ../../carla-includes/carla_midi.hpp \
#     ../../carla-includes/ladspa_rdf.hpp \
#     ../../carla-includes/lv2_rdf.hpp \
#     ../../carla-utils/carla_utils.hpp \
#     ../../carla-utils/carla_lib_utils.hpp \
#     ../../carla-utils/carla_osc_utils.hpp \
#     ../../carla-utils/carla_ladspa_utils.hpp \
#     ../../carla-utils/carla_lv2_utils.hpp \
#     ../../carla-utils/carla_vst_utils.hpp

INCLUDEPATH = . \
    ../carla-includes \
    ../carla-engine \
    ../carla-jackbridge \
    ../carla-native \
    ../carla-plugin \
    ../carla-utils

LIBS = \
    ../carla-lilv/carla_lilv.a \
    ../carla-native/carla_native.a \
    ../carla-rtmempool/carla_rtmempool.a

QMAKE_CFLAGS   *= -fPIC -std=c99
QMAKE_CXXFLAGS *= -fPIC -std=c++0x

unix {
LIBS += -ldl -lm
}

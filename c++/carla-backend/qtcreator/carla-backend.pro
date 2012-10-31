# QtCreator project file

QT = core gui

CONFIG     = debug link_pkgconfig qt warn_on plugin shared
PKGCONFIG  = liblo
PKGCONFIG += jack
PKGCONFIG += alsa libpulse-simple
PKGCONFIG += fluidsynth linuxsampler
PKGCONFIG += fftw3 mxml
PKGCONFIG += suil-0

TARGET   = carla_backend
TEMPLATE = app
VERSION  = 0.5.0

SOURCES  = \
    ../carla_backend_standalone.cpp \
    ../carla_backend_vst.cpp \
    ../carla_bridge.cpp \
    ../carla_engine.cpp \
    ../carla_engine_jack.cpp \
    ../carla_engine_rtaudio.cpp \
    ../carla_native.cpp \
    ../carla_osc.cpp \
    ../carla_shared.cpp \
    ../carla_threads.cpp \
    ../ladspa.cpp \
    ../dssi.cpp \
    ../lv2.cpp \
    ../vst.cpp \
    ../fluidsynth.cpp \
    ../linuxsampler.cpp

SOURCES += \
    ../plugins/bypass.c \
    ../plugins/midi-split.cpp \
    ../plugins/zynaddsubfx.cpp

#SOURCES += \
#    ../plugins/zynaddsubfx/DSP/AnalogFilter.cpp \
#    ../plugins/zynaddsubfx/DSP/FFTwrapper.cpp \
#    ../plugins/zynaddsubfx/DSP/Filter.cpp \
#    ../plugins/zynaddsubfx/DSP/FormantFilter.cpp \
#    ../plugins/zynaddsubfx/DSP/SVFilter.cpp \
#    ../plugins/zynaddsubfx/DSP/Unison.cpp \
#    ../plugins/zynaddsubfx/Effects/Alienwah.cpp \
#    ../plugins/zynaddsubfx/Effects/Chorus.cpp \
#    ../plugins/zynaddsubfx/Effects/Distorsion.cpp \
#    ../plugins/zynaddsubfx/Effects/DynamicFilter.cpp \
#    ../plugins/zynaddsubfx/Effects/Echo.cpp \
#    ../plugins/zynaddsubfx/Effects/Effect.cpp \
#    ../plugins/zynaddsubfx/Effects/EffectLFO.cpp \
#    ../plugins/zynaddsubfx/Effects/EffectMgr.cpp \
#    ../plugins/zynaddsubfx/Effects/EQ.cpp \
#    ../plugins/zynaddsubfx/Effects/Phaser.cpp \
#    ../plugins/zynaddsubfx/Effects/Reverb.cpp \
#    ../plugins/zynaddsubfx/Misc/Bank.cpp \
#    ../plugins/zynaddsubfx/Misc/Config.cpp \
#    ../plugins/zynaddsubfx/Misc/Dump.cpp \
#    ../plugins/zynaddsubfx/Misc/Master.cpp \
#    ../plugins/zynaddsubfx/Misc/Microtonal.cpp \
#    ../plugins/zynaddsubfx/Misc/Part.cpp \
#    ../plugins/zynaddsubfx/Misc/Recorder.cpp \
#    ../plugins/zynaddsubfx/Misc/Stereo.cpp \
#    ../plugins/zynaddsubfx/Misc/Util.cpp \
#    ../plugins/zynaddsubfx/Misc/WavFile.cpp \
#    ../plugins/zynaddsubfx/Misc/WaveShapeSmps.cpp \
#    ../plugins/zynaddsubfx/Misc/XMLwrapper.cpp \
#    ../plugins/zynaddsubfx/Nio/AudioOut.cpp \
#    ../plugins/zynaddsubfx/Nio/Engine.cpp \
#    ../plugins/zynaddsubfx/Nio/EngineMgr.cpp \
#    ../plugins/zynaddsubfx/Nio/MidiIn.cpp \
#    ../plugins/zynaddsubfx/Nio/Nio.cpp \
#    ../plugins/zynaddsubfx/Nio/NulEngine.cpp \
#    ../plugins/zynaddsubfx/Nio/InMgr.cpp \
#    ../plugins/zynaddsubfx/Nio/OutMgr.cpp \
#    ../plugins/zynaddsubfx/Nio/WavEngine.cpp \
#    ../plugins/zynaddsubfx/Params/ADnoteParameters.cpp \
#    ../plugins/zynaddsubfx/Params/Controller.cpp \
#    ../plugins/zynaddsubfx/Params/EnvelopeParams.cpp \
#    ../plugins/zynaddsubfx/Params/FilterParams.cpp \
#    ../plugins/zynaddsubfx/Params/LFOParams.cpp \
#    ../plugins/zynaddsubfx/Params/PADnoteParameters.cpp \
#    ../plugins/zynaddsubfx/Params/Presets.cpp \
#    ../plugins/zynaddsubfx/Params/PresetsArray.cpp \
#    ../plugins/zynaddsubfx/Params/PresetsStore.cpp \
#    ../plugins/zynaddsubfx/Params/SUBnoteParameters.cpp \
#    ../plugins/zynaddsubfx/Synth/ADnote.cpp \
#    ../plugins/zynaddsubfx/Synth/Envelope.cpp \
#    ../plugins/zynaddsubfx/Synth/LFO.cpp \
#    ../plugins/zynaddsubfx/Synth/OscilGen.cpp \
#    ../plugins/zynaddsubfx/Synth/PADnote.cpp \
#    ../plugins/zynaddsubfx/Synth/Resonance.cpp \
#    ../plugins/zynaddsubfx/Synth/SUBnote.cpp \
#    ../plugins/zynaddsubfx/Synth/SynthNote.cpp

#    ../plugins/zynaddsubfx/Effects/.cpp \
#    ../plugins/zynaddsubfx/Params/.cpp \
#    ../plugins/zynaddsubfx/Synth/.cpp \

HEADERS = \
    ../carla_backend.h \
    ../carla_backend_standalone.h \
    ../carla_engine.h \
    ../carla_osc.h \
    ../carla_plugin.h \
    ../carla_shared.h \
    ../carla_threads.h \
    ../plugins/carla_native.h \
    ../plugins/carla_nativemm.h \
    ../../carla-jackbridge/carla_jackbridge.h \
    ../../carla-includes/carla_includes.h \
    ../../carla-includes/carla_lib_includes.h \
    ../../carla-includes/carla_osc_includes.h \
    ../../carla-includes/carla_midi.h \
    ../../carla-includes/carla_ladspa.h \
    ../../carla-includes/carla_dssi.h \
    ../../carla-includes/carla_lv2.h \
    ../../carla-includes/carla_vst.h \
    ../../carla-includes/carla_fluidsynth.h \
    ../../carla-includes/carla_linuxsampler.h \
    ../../carla-includes/ladspa_rdf.h \
    ../../carla-includes/lv2_rdf.h

INCLUDEPATH = .. \
    ../../carla-jackbridge \
    ../../carla-includes

LIBS     =  -ldl \
    ../../carla-lilv/carla_lilv.a \
    ../../carla-rtmempool/carla_rtmempool.a

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG NDEBUG
DEFINES += CARLA_ENGINE_JACK
DEFINES += CARLA_ENGINE_RTAUDIO HAVE_GETTIMEOFDAY __LINUX_ALSA__ __LINUX_ALSASEQ__ __LINUX_PULSE__ __RTAUDIO_DEBUG__ __RTMIDI_DEBUG__
DEFINES += CARLA_ENGINE_LV2
DEFINES += CARLA_ENGINE_VST
DEFINES += HAVE_SUIL
DEFINES += WANT_LADSPA WANT_DSSI WANT_LV2 WANT_VST
DEFINES += WANT_FLUIDSYNTH WANT_LINUXSAMPLER
DEFINES += WANT_ZYNADDSUBFX

#LIBS    += -L../../carla-jackbridge -lcarla-jackbridge-native

INCLUDEPATH += ../rtaudio-4.0.11
INCLUDEPATH += ../rtmidi-2.0.1
SOURCES += ../rtaudio-4.0.11/RtAudio.cpp
SOURCES += ../rtmidi-2.0.1/RtMidi.cpp

QMAKE_CFLAGS   *= -fPIC -std=c99
QMAKE_CXXFLAGS *= -fPIC -std=c++0x

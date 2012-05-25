#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Package database

generic_audio_icon = "audio-x-generic"
generic_midi_icon  = "audio-midi"

LEVEL_0    = "Lv. 0"
LEVEL_1    = "Lv. 1"
LEVEL_LASH = "LASH"
LEVEL_JS   = "JACK-Session"
LEVEL_NSM  = "NSM"

TEMPLATE_YES = "Yes"
TEMPLATE_NO  = "No"

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# DAW

# (L, D, L, V, VST-Mode, T, M, MIDI-Mode) -> ( LADSPA, DSSI, LV2, VST, VST-Mode, Transport, MIDI, MIDI-Mode)

list_DAW = [
  # Package         AppName             Type              Binary              Icon              Template?     Level      Rel.-Model    (L, D, L, V, VST-Mode,  T, M, MIDI-Mode)      (doc-file,                                                         website)
  ( "ardour",       "Ardour 2.8",       "DAW",            "ardour2",          "ardour",         TEMPLATE_YES, LEVEL_1,   "OpenSource", (1, 0, 1, 0, "",        1, 0, "ALSA"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ),
  ( "ardour3",      "Ardour 3.0",       "DAW",            "ardour3",          "ardour",         TEMPLATE_YES, LEVEL_JS,  "OpenSource", (1, 0, 1, 0, "",        1, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ),

  ( "composite",    "Composite",        "Drum Sequencer", "composite-gui",    "composite32x32", TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/composite/data/doc/manual.html",               "http://gabe.is-a-geek.org/composite/") ),

  ( "energyxt2",    "EnergyXT2",        "DAW",            "energyxt2",        "energyxt2",      TEMPLATE_NO,  LEVEL_0,   "Demo",       (0, 0, 0, 1, "Native",  0, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/EnergyXT_Manual_EN.pdf",         "http://www.energy-xt.com/") ),

  ( "hydrogen",      "Hydrogen",        "Drum Sequencer", "hydrogen -d jack", "h2-icon",        TEMPLATE_NO,  LEVEL_JS,  "OpenSource", (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ),
  ( "hydrogen-svn",  "Hydrogen (SVN)",  "Drum Sequencer", "hydrogen -d jack", "h2-icon",        TEMPLATE_NO,  LEVEL_JS,  "OpenSource", (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ),

  ( "jacker",        "Jacker",          "MIDI Sequencer", "jacker",           "jacker",         TEMPLATE_NO,  LEVEL_1,   "OpenSource", (0, 0, 0, 0, "",        1, 1, "JACK"),        ("",                                                               "https://bitbucket.org/paniq/jacker/wiki/Home") ),

  ( "lmms",          "LMMS",            "DAW",            "lmms",             "lmms",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, 0, 1, "Windows", 0, 1, "ALSA"),        ("",                                                               "http://lmms.sourceforge.net/") ),

 #( "muse",          "MusE",            "DAW",            "muse",             "muse",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/doc/muse/html/window_ref.html",                "http://www.muse-sequencer.org/") ),
 #( "muse2",         "MusE 2",          "DAW",            "muse",             "muse",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/doc/muse/html/window_ref.html",                "http://www.muse-sequencer.org/") ),

  ( "musescore",     "MuseScore",       "MIDI Composer",  "mscore",           "mscore",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("file:///usr/share/kxstudio/docs/MuseScore-en.pdf",               "http://www.musescore.org/") ),

  ( "non-daw",       "Non-DAW",         "DAW",            "non-daw",          "non-daw",        TEMPLATE_NO,  LEVEL_NSM, "OpenSource", (0, 0, 0, 0, "",        1, 0, "CV"),          ("file:///usr/share/doc/non-daw/MANUAL.html",                      "http://non-daw.tuxfamily.org/") ),
  ( "non-sequencer", "Non-Sequencer",   "MIDI Sequencer", "non-sequencer",    "non-sequencer",  TEMPLATE_NO,  LEVEL_NSM, "OpenSource", (0, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/doc/non-sequencer/MANUAL.html",                "http://non-sequencer.tuxfamily.org/") ),

  ( "oomidi-2011",   "OpenOctave 2011", "DAW",            "oomidi-2011",      "oomidi-2011",    TEMPLATE_NO,  LEVEL_1,   "OpenSource", (1, 0, 1, 0, "",        1, 1, "ALSA + JACK"), ("",                                                               "http://www.openoctave.org/") ),
  ( "oomidi-2012",   "OpenOctave 2012", "DAW",            "oomidi-2012",      "oomidi-2012",    TEMPLATE_NO,  LEVEL_1,   "OpenSource", (1, 0, 1, 1, "Native",  1, 1, "ALSA + JACK"), ("",                                                               "http://www.openoctave.org/") ),

  ( "qtractor",      "Qtractor",        "DAW",            "qtractor",         "qtractor",       TEMPLATE_NO,  LEVEL_JS,  "OpenSource", (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.3.0-user-manual.pdf", "http://qtractor.sourceforge.net/") ),
  ( "qtractor-svn",  "Qtractor (SVN)",  "DAW",            "qtractor",         "qtractor",       TEMPLATE_NO,  LEVEL_JS,  "OpenSource", (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.3.0-user-manual.pdf", "http://qtractor.sourceforge.net/") ),

  ( "reaper",        "REAPER",          "DAW",            "reaper",           "reaper",         TEMPLATE_NO,  LEVEL_0,   "Demo",       (0, 0, 0, 1, "Windows", 1, 1, "ALSA"),        ("",                                                               "http://www.reaper.fm/") ),

  ( "renoise",       "Renoise",         "Tracker",        "renoise",          "renoise",        TEMPLATE_NO,  LEVEL_0,   "ShareWare",  (1, 1, 0, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/Renoise User Manual.pdf",        "http://www.renoise.com/") ),

  ( "rosegarden",    "Rosegarden",      "MIDI Sequencer", "rosegarden",       "rosegarden",     TEMPLATE_NO,  LEVEL_1,   "OpenSource", (1, 1, 0, 0, "",        1, 1, "ALSA"),        ("",                                                               "http://www.rosegardenmusic.com/") ),

  ( "seq24",         "Seq24",           "MIDI Sequencer", "seq24",            "seq24",          TEMPLATE_NO,  LEVEL_JS,  "OpenSource", (0, 0, 0, 0, "",        1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/SEQ24",                          "http://www.filter24.org/seq24/") ),

  ( "traverso",      "Traverso",        "DAW",            "traverso",         "traverso",       TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, 1, 0, "",        1, 0, ""),            ("file:///usr/share/kxstudio/docs/traverso-manual-0.49.0.pdf",     "http://traverso-daw.org/") ),
]

iDAW_Package, iDAW_AppName, iDAW_Type, iDAW_Binary, iDAW_Icon, iDAW_Template, iDAW_Level, iDAW_RelModel, iDAW_Features, iDAW_Docs = range(0, len(list_DAW[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Host

# (I, L, D, L, V, VST-Mode, MIDI-Mode) -> (Internal, LADSPA, DSSI, LV2, VST, VST-Mode, MIDI-Mode)

list_Host = [
  # Package             AppName                 Ins?   FX?    Binary           Icon         Template?     Level       Rel.-Model    (I, L, D, L, V, VST-Mode,  MIDI-Mode)      (doc-file,                               website)
  ( "calf-plugins",     "Calf Jack Host",       "Yes", "Yes", "calfjackhost",  "calf",      TEMPLATE_NO,  LEVEL_0,    "OpenSource", (1, 0, 0, 0, 0, "",        "JACK"),        ("",                                     "http://calf.sourceforge.net/") ),
  ( "calf-plugins-git", "Calf Jack Host (GIT)", "Yes", "Yes", "calfjackhost",  "calf",      TEMPLATE_YES, LEVEL_1,    "OpenSource", (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/Calf.html", "http://calf.sourceforge.net/") ),

  ( "carla",            "Carla",                "Yes", "Yes", "carla",         "carla",     TEMPLATE_NO,  LEVEL_1,    "OpenSource", (0, 1, 1, 1, 1, "Native",  "JACK"),        ("",                                     "http://kxstudio.sourceforge.net/KXStudio:Applications:Carla") ),

  ( "festige",          "FeSTige",              "Yes", "Yes", "festige",       "festige",   TEMPLATE_NO,  LEVEL_1,    "OpenSource", (0, 0, 0, 0, 1, "Windows", "ALSA | JACK"), ("",                                     "http://festige.sourceforge.net/") ),

 #( "ingen",            "Ingen",                "Yes", "Yes", "ingen -eg",     "ingen",     TEMPLATE_NO,  LEVEL_JS,   "OpenSource", (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                     "http://drobilla.net/blog/software/ingen/") ),
  ( "ingen-svn",        "Ingen (SVN)",          "Yes", "Yes", "ingen-svn -eg", "ingen",     TEMPLATE_NO,  LEVEL_JS,   "OpenSource", (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                     "http://drobilla.net/blog/software/ingen/") ),

  ( "jack-rack",        "Jack Rack",            "No",  "Yes", "jack-rack",     "jack-rack", TEMPLATE_NO,  LEVEL_0,    "OpenSource", (0, 1, 0, 0, 0, "",        "ALSA"),        ("",                                     "http://jack-rack.sourceforge.net/") ),

  ( "zynjacku",         "LV2 Rack",             "No",  "Yes", "lv2rack",       "zynjacku",  TEMPLATE_NO,  LEVEL_LASH, "OpenSource", (0, 0, 0, 1, 0, "",        "JACK"),        ("",                                     "http://home.gna.org/zynjacku/") ),
  ( "zynjacku",         "ZynJackU",             "Yes", "No",  "zynjacku",      "zynjacku",  TEMPLATE_NO,  LEVEL_LASH, "OpenSource", (0, 0, 0, 1, 0, "",        "JACK"),        ("",                                     "http://home.gna.org/zynjacku/") ),
]

iHost_Package, iHost_AppName, iHost_Ins, iHost_FX, iHost_Binary, iHost_Icon, iHost_Template, iHost_Level, iHost_RelModel, iHost_Features, iDAW_Docs = range(0, len(list_Host[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Instrument

# (F, I, MIDI-Mode) -> (Built-in FX, Audio Input, MIDI-Mode)

list_Instrument = [
  # Package                 AppName              Type                Binary                    Icon                Template?     Level    Rel.-Model    (F, I, MIDI-Mode)      (doc-file,                                                     website)
  ( "aeolus",               "Aeolus",            "Synth",            "aeolus",                 generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, 0, "ALSA | JACK"), ("",                                                           "http://www.kokkinizita.net/linuxaudio/aeolus/index.html") ),

  ( "amsynth",              "Amsynth",           "Synth",            "amsynth",                "amsynth",          TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 0, "ALSA"),        ("",                                                           "") ),

  ( "azr3-jack",            "AZR3",              "Synth",            "azr3",                   "azr3",             TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, 0, "JACK"),        ("",                                                           "http://ll-plugins.nongnu.org/azr3/") ),

  ( "distrho-plugin-ports", "Vex",               "Synth",            "vex",                    generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("",                                                           "") ),
  ( "highlife",             "HighLife",          "Sampler",          "highlife",               generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("",                                                           "http://www.discodsp.com/highlife/") ),
  ( "juced-plugins",        "Capsaicin",         "Synth",            "capsaicin",              "juced_plugins",    TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("",                                                           "") ),
  ( "juced-plugins",        "DrumSynth",         "Synth",            "drumsynth",              "juced_plugins",    TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("",                                                           "") ),
  ( "tal-plugins",          "TAL NoiseMaker",    "Synth",            "TAL-NoiseMaker",         "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("",                                                           "http://kunz.corrupt.ch/products/tal-noisemaker") ),
  ( "wolpertinger",         "Wolpertinger",      "Synth",            "Wolpertinger",           "wolpertinger",     TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 0, "ALSA"),        ("",                                                           "http://tumbetoene.tuxfamily.org") ),

  ( "foo-yc20",             "Foo YC20",          "Synth",            "foo-yc20",               "foo-yc20",         TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, 0, "JACK"),        ("",                                                           "http://code.google.com/p/foo-yc20/") ),

  ( "jsampler",             "JSampler Fantasia", "Sampler",          "jsampler-bin",           "jsampler",         TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, 0, "ALSA + JACK"), ("file:///usr/share/kxstudio/docs/jsampler/jsampler.html",     "http://www.linuxsampler.org/") ),

  ( "loomer-plugins",       "Aspect",            "Synth",            "Aspect",                 "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Aspect Manual.pdf.gz",  "http://www.loomer.co.uk/aspect.htm") ),
  ( "loomer-plugins",       "Sequent",           "Synth",            "Sequent",                "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Sequent Manual.pdf.gz", "http://www.loomer.co.uk/sequent.htm") ),
  ( "loomer-plugins",       "String",            "Synth",            "String",                 "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",  "http://www.loomer.co.uk/string.htm") ),

  ( "phasex",               "Phasex",            "Synth",            "phasex",                 "phasex",           TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 1, "ALSA"),        ("file:///usr/share/phasex/help/parameters.help",              "") ),

  ( "pianoteq",             "Pianoteq",          "Synth",            "Pianoteq",               "pianoteq",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, 0, "ALSA + JACK"), ("file:///usr/local/bin/Documentation/pianoteq-english.pdf",   "http://www.pianoteq.com/pianoteq3_standard") ),
  ( "pianoteq-play",        "Pianoteq Play",     "Synth",            "Pianoteq-PLAY",          "pianoteq",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, 0, "ALSA + JACK"), ("file:///usr/local/bin/Documentation/pianoteq-english.pdf",   "http://www.pianoteq.com/pianoteq3_play") ),

  ( "qsampler",             "Qsampler",          "Sampler",          "qsampler",               "qsampler",         TEMPLATE_NO,  LEVEL_1, "OpenSource", (0, 0, "ALSA + JACK"), ("",                                                           "http://qsampler.sourceforge.net/") ),

  ( "qsynth",               "Qsynth",            "SoundFont Player", "qsynth -a jack -m jack", "qsynth",           TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 0, "ALSA | JACK"), ("",                                                           "http://qsynth.sourceforge.net/") ),

  ( "yoshimi",              "Yoshimi",           "Synth",            "yoshimi -j -J",          "yoshimi",          TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 0, "ALSA | JACK"), ("",                                                           "http://yoshimi.sourceforge.net/") ),

  ( "zynaddsubfx",          "ZynAddSubFX",       "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, 0, "ALSA + JACK"), ("",                                                           "http://zynaddsubfx.sourceforge.net/") ),
]

iInstrument_Package, iInstrument_AppName, iInstrument_Type, iInstrument_Binary, iInstrument_Icon, iInstrument_Template, iInstrument_Level, iInstrument_RelModel, iInstrument_Features, iInstrument_Docs = range(0, len(list_Instrument[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Bristol

# Need name: bit99, bit100

list_Bristol = [
  # Package    AppName                           Type     Short-name    Icon                  Template?     Level    Rel.-Model    (F, I, MIDI-Mode)      (doc-file, website)
  ( "bristol", "Moog Voyager",                   "Synth", "explorer",   "bristol_explorer",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/explorer.html") ),
  ( "bristol", "Moog Mini",                      "Synth", "mini",       "bristol_mini",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mini.html") ),
  ( "bristol", "Sequential Circuits Prophet-52", "Synth", "prophet52",  "bristol_prophet52",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet52.html") ),

  ( "bristol", "Moog/Realistic MG-1",            "Synth", "realistic",  "bristol_realistic",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/realistic.html") ),
  ( "bristol", "Memory Moog",                    "Synth", "memoryMoog", "bristol_memoryMoog", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/memorymoog.html") ),
  ( "bristol", "Baumann BME-700",                "Synth", "BME700",     "bristol_BME700",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/bme700.shtml") ),
 #( "bristol", "Synthi Aks",                     "Synth", "aks",        "bristol_aks",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/aks.html") ),

  ( "bristol", "Moog Voyager Blue Ice",          "Synth", "voyager",    "bristol_voyager",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/voyager.html") ),
  ( "bristol", "Moog Sonic-6",                   "Synth", "sonic6",     "bristol_sonic6",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/sonic6.html") ),
  ( "bristol", "Hammond B3",                     "Synth", "hammondB3",  "bristol_hammondB3",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/hammond.html") ),
  ( "bristol", "Sequential Circuits Prophet-5",  "Synth", "prophet",    "bristol_prophet",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet5.html") ),
  ( "bristol", "Sequential Circuits Prophet-10", "Synth", "prophet10",  "bristol_prophet10",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet10.html") ),
  ( "bristol", "Sequential Circuits Pro-1",      "Synth", "pro1",       "bristol_pro1",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/pro1.html") ),
  ( "bristol", "Fender Rhodes Stage-73",         "Synth", "rhodes",     "bristol_rhodes",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/rhodes.html") ),
  ( "bristol", "Rhodes Bass Piano",              "Synth", "rhodesbass", "bristol_rhodesbass", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/rhodes.html") ),
  ( "bristol", "Crumar Roadrunner",              "Synth", "roadrunner", "bristol_roadrunner", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/roadrunner.html") ),
  ( "bristol", "Crumar Bit-1",                   "Synth", "bitone",     "bristol_bitone",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/bitone.html") ),
  ( "bristol", "Crumar Stratus",                 "Synth", "stratus",    "bristol_stratus",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/stratus.html") ),
  ( "bristol", "Crumar Trilogy",                 "Synth", "trilogy",    "bristol_trilogy",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/trilogy.html") ),
  ( "bristol", "Oberheim OB-X",                  "Synth", "obx",        "bristol_obx",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/obx.html") ),
  ( "bristol", "Oberheim OB-Xa",                 "Synth", "obxa",       "bristol_obxa",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/obxa.html") ),
  ( "bristol", "ARP Axxe",                       "Synth", "axxe",       "bristol_axxe",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/axxe.html") ),
  ( "bristol", "ARP Odyssey",                    "Synth", "odyssey",    "bristol_odyssey",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/odyssey.html") ),
  ( "bristol", "ARP 2600",                       "Synth", "arp2600",    "bristol_arp2600",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/arp2600.html") ),
  ( "bristol", "ARP Solina Strings",             "Synth", "solina",     "bristol_solina",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/solina.html") ),
  ( "bristol", "Korg Poly-800",                  "Synth", "poly800",    "bristol_poly800",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/poly800.shtml") ),
  ( "bristol", "Korg Mono/Poly",                 "Synth", "monopoly",   "bristol_monopoly",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mono.html") ),
  ( "bristol", "Korg Polysix",                   "Synth", "poly",       "bristol_poly",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/poly.html") ),
  ( "bristol", "Korg MS-20 (*)",                 "Synth", "ms20",       "bristol_ms20",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/ms20.html") ),
  ( "bristol", "VOX Continental",                "Synth", "vox",        "bristol_vox",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/vox.html") ),
  ( "bristol", "VOX Continental 300",            "Synth", "voxM2",      "bristol_voxM2",      TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/vox300.html") ),
  ( "bristol", "Roland Juno-6",                  "Synth", "juno",       "bristol_juno",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/juno.html") ),
  ( "bristol", "Roland Jupiter 8",               "Synth", "jupiter8",   "bristol_jupiter8",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/jupiter8.html") ),
 #( "bristol", "Bristol BassMaker",              "Synth", "bassmaker",  "bristol_bassmaker",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ),
  ( "bristol", "Yamaha DX",                      "Synth", "dx",         "bristol_dx",         TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/dx.html") ),
 #( "bristol", "Yamaha CS-80",                   "Synth", "cs80",       "bristol_cs80",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/cs80.html") ),
  ( "bristol", "Bristol SID Softsynth",          "Synth", "sidney",     "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/sidney.shtml") ),
 #( "bristol", "Commodore-64 SID polysynth",     "Synth", "melbourne",  "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ), #FIXME - needs icon
 #( "bristol", "Bristol Granular Synthesiser",   "Synth", "granular",   "bristol_granular",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ),
 #( "bristol", "Bristol Realtime Mixer",         "Synth", "mixer",      "bristol_mixer",      TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mixer.html") ),
]

iBristol_Package, iBristol_AppName, iBristol_Type, iBristol_ShortName, iBristol_Icon, iBristol_Template, iBristol_Level, iBristol_RelModel, iBristol_Features, iBristol_Docs = range(0, len(list_Bristol[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Effect

# (S, MIDI-Mode) -> (Stereo, MIDI-Mode)

list_Effect = [
  # Package                 AppName                          Type                   Binary                                          Icon                Template?     Level    Rel.-Model    (S, MIDI-Mode)      (doc,                                                                  website)
  ( "ambdec",               "AmbDec",                        "Ambisonic Decoder",   "ambdec",                                       generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "---"),         ("",                                                                   "") ),

  ( "arctican-plugins",     "The Function",                  "Delay",               "TheFunction",                                  "arctican_plugins", TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://arcticanaudio.webs.com/effects/thepilgrim.htm") ),
  ( "arctican-plugins",     "The Pilgrim",                   "Filter",              "ThePilgrim",                                   "arctican_plugins", TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://arcticanaudio.webs.com/effects/thefunction.htm") ),

  ( "distrho-plugins",      "3 BandEQ",                      "EQ",                  "3BandEQ",                                      "distrho_plugins",  TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "distrho-plugins",      "Ping Pong Pan",                 "Pan",                 "PingPongPan",                                  "distrho_plugins",  TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
 #( "distrho-plugins",      "Stereo Audio Gain",             "Amplifier",           "StereoAudioGain",                              "distrho_plugins",  TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),

  ( "distrho-plugin-ports", "Argotlunar",                    "Granularor",          "argotlunar",                                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://argotlunar.info/") ),
  ( "distrho-plugin-ports", "BitMangler",                    "Misc",                "bitmangler",                                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "distrho-plugin-ports", "Juce Pitcher",                  "Pitch-Shifter",       "juce_pitcher",                                 generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "distrho-plugin-ports", "sDelay",                        "Delay",               "sDelay",                                       generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),

  ( "drowaudio-plugins",    "dRowAudio Distortion",          "Distortion",          "drowaudio-distortion",                         generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "drowaudio-plugins",    "dRowAudio Distortion-Shaper",   "Distortion",          "drowaudio-distortionshaper",                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "drowaudio-plugins",    "dRowAudio Flanger",             "Flanger",             "drowaudio-flanger",                            generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "drowaudio-plugins",    "dRowAudio Reverb",              "Reverb",              "drowaudio-reverb",                             generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),
  ( "drowaudio-plugins",    "dRowAudio Tremolo",             "Tremolo",             "drowaudio-tremolo",                            generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),

  ( "guitarix",             "Guitarix",                      "Guitar FX",           "guitarix",                                     "gx_head",          TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, "JACK"),        ("",                                                                   "http://guitarix.sourceforge.net/") ),

  ( "hybridreverb2",        "HybridReverb2",                 "Reverb",              "HybridReverb2",                                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://www2.ika.rub.de/HybridReverb2/") ),

  ( "jamin",                "Jamin",                         "Mastering",           "jamin",                                        "jamin",            TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "---"),         ("",                                                                   "http://jamin.sourceforge.net/") ),

  ( "jcgui",                "Jc_Gui",                        "Convolver",           "Jc_Gui",                                       "Jc_Gui",           TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "---"),         ("",                                                                   "") ),

  ( "juced-plugins",        "EQinox",                        "EQ",                  "eqinox",                                       "juced_plugins",    TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ),

  ( "linuxdsp-plugins",     "linuxDSP Channel Equaliser",    "EQ",                  "ch-eq2b-x86-64 || ch-eq2b-i686",               "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/CH-EQ2B/manual.pdf.gz",       "http://www.linuxdsp.co.uk/download/lv2/download_ch_eqb/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Multiband Compressor", "Compressor",          "mbc2b-x86-64 || mbc2b-i686",                   "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/MBC2B/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_mbc2/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Graphical Equalizer",  "EQ",                  "mkii-graph-eq2-x86-64 || mkii-graph-eq2-i686", "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/MKII-GRAPH-EQ/manual.pdf.gz", "http://www.linuxdsp.co.uk/download/lv2/download_mkii_graph_eq/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Vintage Program EQ",   "EQ",                  "peq-2a-x86-64 || peq-2a-i686",                 "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/PEQ-2A/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_sr_2b/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Reverb",               "Reverb",              "sr2b-x86-64 || sr2b-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/SR2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_sr_2b/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Vintage Compressor",   "Compressor",          "vc2b-x86-64 || vc2b-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/VC2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_vc2b/index.html") ),

  ( "linuxdsp-plugins",     "linuxDSP Guitar Chorus",        "Chorus",              "cr1-x86-64 || cr1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/CR1/manual.pdf.gz",           "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Guitar Distortion",    "Distortion",          "dt1-x86-64 || dt1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/DT1/dt1_manual.pdf.gz",       "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Guitar Phaser",        "Phaser",              "ph1-x86-64 || ph1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/PH1/ph1_manual.pdf.gz",       "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Guitar WAH",           "Distortion",          "wah1-x86-64 || wah1-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/WAH1/wah1_manual.pdf.gz",     "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ),
  ( "linuxdsp-plugins",     "linuxDSP Valve Overdrive",      "Amplifier",           "odv2_x86-64 || odv2_i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "FreeWare",   (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/ODV2/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/jack/download_odv2_jack/index.html") ),

  ( "loomer-plugins",       "Manifold",                      "Enhancer",            "Manifold",                                     "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Manifold Manual.pdf.gz",        "http://www.loomer.co.uk/manifold.htm") ),
  ( "loomer-plugins",       "Resound",                       "Delay",               "Resound",                                      "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Resound Manual.pdf.gz",         "http://www.loomer.co.uk/resound.htm") ),
  ( "loomer-plugins",       "Shift2",                        "Pitch-Shifter/Delay", "Shift2",                                       "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Shift2 Manual.pdf.gz",          "http://www.loomer.co.uk/shift2.htm") ),
  ( "loomer-plugins",       "String (FX)",                   "Bundle",              "String_FX",                                    "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",          "http://www.loomer.co.uk/string.htm") ),

  ( "rakarrack",            "Rakarrack",                     "Guitar FX",           "rakarrack",                                    "rakarrack",        TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA + JACK"), ("file:///usr/share/doc/rakarrack/html/help.html",                     "http://rakarrack.sourceforge.net") ),

  ( "tal-plugins",          "TAL Dub 3",                     "Delay",               "TAL-Dub-3",                                    "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-noisemaker") ),
  ( "tal-plugins",          "TAL Filter",                    "Filter",              "TAL-Filter",                                   "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-filter") ),
  ( "tal-plugins",          "TAL Filter 2",                  "Filter",              "TAL-Filter-2",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-filter") ),
  ( "tal-plugins",          "TAL Reverb",                    "Reverb",              "TAL-Reverb",                                   "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ),
  ( "tal-plugins",          "TAL Reverb 2",                  "Reverb",              "TAL-Reverb-2",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ),
  ( "tal-plugins",          "TAL Reverb 3",                  "Reverb",              "TAL-Reverb-3",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ),
  ( "tal-plugins",          "TAL Vocoder 2",                 "Vocoder",             "TAL-Vocoder-2",                                "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-vocoder") ),
]

iEffect_Package, iEffect_AppName, iEffect_Type, iEffect_Binary, iEffect_Icon, iEffect_Template, iEffect_Level, iEffect_RelModel, iEffect_Features, iEffect_Docs = range(0, len(list_Effect[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Tool

# (MIDI-Mode, T) -> (MIDI-Mode, Transport)

list_Tool = [
  # Package              AppName                         Type                   Binary                    Icon                Template?     Level    Rel.-Model    (MIDI-Mode, T)      (doc,                                                             website)
  ( "a2jmidid",          "ALSA -> Jack MIDI",            "Bridge",              "a2j -e",                 generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "http://home.gna.org/a2jmidid/") ),

  ( "arpage",            "Arpage",                       "MIDI Arpeggiator",    "arpage",                 "arpage",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ),
  ( "arpage",            "Zonage",                       "MIDI Mapper",         "zonage",                 "zonage",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "") ),

  ( "audacity",          "Audacity",                     "Audio Editor",        "audacity",               "audacity",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "http://audacity.sourceforge.net/") ),

  ( "cadence",           "Cadence",                      "Multi-Feature",       "cadence",                "cadence",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ),
  ( "cadence-py3-tools", "JACK XY Controller",           "XY Controller",       "jack_xycontroller",      generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "") ),
  ( "catia",             "Catia",                        "Patch Bay",           "catia",                  "catia",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ),
  ( "claudia",           "Claudia",                      "Session Handler",     "claudia",                "claudia",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ),
  ( "carla-control",     "Carla OSC Control",            "OSC Control",         "carla-control",          "carla-control",    TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ),

  ( "drumstick-tools",   "Drumstick Virtual Piano",      "Virtual Keyboard",    "drumstick-vpiano",       "drumstick",        TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://drumstick.sourceforge.net/") ),

  ( "fmit",              "Music Instrument Tuner",       "Instrument Tuner",    "fmit",                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ),

  ( "gigedit",           "Gigedit",                      "Instrument Editor",   "gigedit",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("/usr/share/doc/gigedit/gigedit_quickstart.html",                "") ),

  ( "gjacktransport",    "GJackClock",                   "Transport Tool",      "gjackclock",             "gjackclock",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ),
  ( "gjacktransport",    "GJackTransport",               "Transport Tool",      "gjacktransport",         "gjacktransport",   TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ),

  ( "gladish",           "LADI Session Handler",         "Session Handler",     "gladish",                "gladish",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "http://www.ladish.org") ),

  ( "gninjam",           "Gtk NINJAM client",            "Music Collaboration", "gninjam",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ),

  ( "jack-capture",      "Jack Capture",                 "Recorder",            "jack_capture_gui2",      "media-record",     TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ),

  ( "jack-keyboard",     "Jack Keyboard",                "Virtual Keyboard",    "jack-keyboard",          "jack-keyboard",    TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("file:///usr/share/kxstudio/docs/jack-keyboard/manual.html",     "http://jack-keyboard.sourceforge.net/") ),

  ( "jack-mixer",        "Jack Mixer",                   "Mixer",               "jack_mixer",             "jack_mixer",       TEMPLATE_NO,  LEVEL_1, "OpenSource", ("JACK",        0), ("",                                                              "http://home.gna.org/jackmixer/") ),

  ( "jack-trans2midi",   "Jack-Transport -> MIDI Clock", "Bridge",              "trans2midi",             generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ),

  ( "kmetronome",        "KMetronome",                   "Metronome",           "kmetronome",             "kmetronome",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://kmetronome.sourceforge.net/kmetronome.shtml") ),
  ( "kmidimon",          "KMidimon",                     "Monitor",             "kmidimon",               "kmidimon",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://kmidimon.sourceforge.net/") ),

  ( "laditools",         "LADI Log",                     "Log Viewer",          "ladilog",                "ladilog",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ),
  ( "laditools",         "LADI Tray",                    "Session Handler",     "laditray",               "laditray",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ),

  ( "linuxdsp-plugins",  "linuxDSP JACK Patch Bay",      "Patch Bay",           "jp1_x86-64 || jp1_i686", "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "FreeWare",   ("JACK",        0), ("file:///usr/share/doc/linuxdsp-plugins/JP1/manual.pdf.gz",      "http://www.linuxdsp.co.uk/download/jack/download_jp1_jack/index.html") ),

  ( "lives",             "LiVES",                        "VJ / Video Editor",   "lives",                  "lives",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "http://lives.sourceforge.net/") ),

  ( "ltc2amidi",         "LTC -> MIDI Clock",            "Bridge",              "ltc2amidi",              generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ),

  ( "mhwaveedit",        "MhWaveEdit",                   "Audio Editor",        "mhwaveedit",             "mhwaveedit",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "http://gna.org/projects/mhwaveedit/") ),

  ( "mixxx",             "Mixxx",                        "DJ",                  "mixxx",                  "mixxx",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("file:///usr/share/kxstudio/docs/Mixxx-Manual.pdf",              "http://mixxx.sourceforge.net/") ),

  ( "non-mixer",         "Non-Mixer",                    "Mixer",               "non-mixer",              "non-mixer",        TEMPLATE_NO,  LEVEL_0, "OpenSource", ("CV",          0), ("file:///usr/share/doc/non-mixer/MANUAL.html",                   "http://non-daw.tuxfamily.org/") ),

  ( "openmovieeditor",   "OpenMovieEditor",              "Video Editor",        "openmovieeditor",        "openmovieeditor",  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("file:///usr/share/kxstudio/docs/openmovieeditor/tutorial.html", "http://www.openmovieeditor.org/") ),

  ( "oomidi-2011",       "OpenOctave Studio",            "Session Handler",     "oostudio-2011",          "oomidi-2011",      TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "https://github.com/ccherrett/oom/wiki/OOStudio") ),

  ( "patchage",          "Patchage",                     "Patch Bay",           "patchage",               "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ),
  ( "patchage",          "Patchage (ALSA Only)",         "Patch Bay",           "patchage -J",            "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ),
  ( "patchage-svn",      "Patchage (SVN)",               "Patch Bay",           "patchage-svn",           "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ),
  ( "patchage-svn",      "Patchage (SVN, ALSA Only)",    "Patch Bay",           "patchage-svn -J",        "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ),

  ( "qjackctl",          "QJackControl",                 "Jack Control",        "qjackctl",               "qjackctl",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 1), ("",                                                              "") ),

  ( "qamix",             "QAMix",                        "Mixer",               "qamix",                  "qamix",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ),
  ( "qarecord",          "QARecord",                     "Recorder",            "qarecord --jack",        "qarecord_48",      TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ),
  ( "qmidiarp",          "QMidiArp",                     "MIDI Arpeggiator",    "qmidiarp",               generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ),

  ( "timemachine",       "TimeMachine",                  "Recorder",            "timemachine",            "timemachine",      TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "http://plugin.org.uk/timemachine/") ),

  ( "vmpk",              "Virtual MIDI Piano Keyboard",  "Virtual Keyboard",    "vmpk",                   "vmpk",             TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("file:///usr/share/vmpk/help.html",                              "http://vmpk.sourceforge.net/") ),

  ( "xjadeo",            "XJadeo",                       "Video Player",        "qjadeo",                 "qjadeo",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "http://xjadeo.sourceforge.net/") ),
]

iTool_Package, iTool_AppName, iTool_Type, iTool_Binary, iTool_Icon, iTool_Template, iTool_Level, iTool_RelModel, iTool_Features, iTool_Docs = range(0, len(list_Tool[0]))


#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# TODO
# - add lv2 plugins
# - split linuxdsp plugins into :i386 and :amd64 (some of its plugins are 32bit only)

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

USING_KXSTUDIO = False

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# DAW

# (L, D, L, V, VST-Mode, T, M, MIDI-Mode) -> ( LADSPA, DSSI, LV2, VST, VST-Mode, Transport, MIDI, MIDI-Mode)

list_DAW = [
  # Package          AppName            Type              Binary              Icon                Template?     Level      Rel.-Model    (L, D, L, V, VST-Mode,  T, M, MIDI-Mode)      (doc-file,                                                         website)
  [ "ardour",        "Ardour 2.8",      "DAW",            "ardour2",          "ardour",           TEMPLATE_YES, LEVEL_0,   "OpenSource", (1, 0, 1, 0, "",        1, 0, "ALSA"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ],
  [ "ardour3",       "Ardour 3",        "DAW",            "ardour3",          "ardour",           TEMPLATE_YES, LEVEL_JS,  "OpenSource", (1, 0, 1, 1, "Native",  1, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ],

  [ "ariamaestosa",  "Aria Maestosa",   "MIDI Sequencer", "Aria",             "aria",             TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("",                                                               "http://ariamaestosa.sourceforge.net/") ],

  [ "composite",     "Composite",       "Drum Sequencer", "composite-gui",    "composite32x32",   TEMPLATE_YES, LEVEL_0,   "OpenSource", (1, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/composite/data/doc/manual.html",               "http://gabe.is-a-geek.org/composite/") ],

  [ "energyxt2",     "energyXT2",       "DAW",            "energyxt2",        "energyxt2",        TEMPLATE_NO,  LEVEL_0,   "Demo",       (0, 0, 0, 1, "Native",  0, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/EnergyXT_Manual_EN.pdf",         "http://www.energy-xt.com/") ],

  [ "giada",         "Giada",           "Audio Looper",   "giada",            generic_audio_icon, TEMPLATE_YES, LEVEL_0,   "OpenSource", (0, 0, 0, 0, "",        0, 0, ""),            ("",                                                               "http://www.monocasual.com/giada/") ],

  [ "hydrogen",      "Hydrogen",        "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  "OpenSource", (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],
  [ "hydrogen-git",  "Hydrogen (GIT)",  "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  "OpenSource", (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],
  [ "hydrogen-svn",  "Hydrogen (SVN)",  "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  "OpenSource", (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],

  [ "jacker",        "Jacker",          "MIDI Sequencer", "jacker",           "jacker",           TEMPLATE_YES, LEVEL_0,   "OpenSource", (0, 0, 0, 0, "",        1, 1, "JACK"),        ("",                                                               "https://bitbucket.org/paniq/jacker/wiki/Home") ],

  [ "lmms",          "LMMS",            "DAW",            "lmms",             "lmms",             TEMPLATE_YES, LEVEL_0,   "OpenSource", (1, 0, 0, 1, "Windows", 0, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/LMMS_UserManual_0.4.12.1.pdf",   "http://lmms.sourceforge.net/") ],

  [ "muse",          "MusE",            "DAW",            "muse",             "muse",             TEMPLATE_YES, LEVEL_0,   "OpenSource", (1, 1, 0, 0, "",        1, 1, "ALSA + JACK"), ("file:///usr/share/doc/muse/html/window_ref.html",                "http://www.muse-sequencer.org/") ],

  [ "musescore",     "MuseScore",       "MIDI Composer",  "mscore",           "mscore",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("file:///usr/share/kxstudio/docs/MuseScore-en.pdf",               "http://www.musescore.org/") ],

  [ "non-daw",       "Non-DAW",         "DAW",            "non-daw",          "non-daw",          TEMPLATE_YES, LEVEL_NSM, "OpenSource", (0, 0, 0, 0, "",        1, 0, "CV + OSC"),    ("file:///usr/share/doc/non-daw/MANUAL.html",                      "http://non-daw.tuxfamily.org/") ],
  [ "non-sequencer", "Non-Sequencer",   "MIDI Sequencer", "non-sequencer",    "non-sequencer",    TEMPLATE_YES, LEVEL_NSM, "OpenSource", (0, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/doc/non-sequencer/MANUAL.html",                "http://non-sequencer.tuxfamily.org/") ],

  [ "qtractor",      "Qtractor",        "DAW",            "qtractor",         "qtractor",         TEMPLATE_YES, LEVEL_1,   "OpenSource", (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.5.x-user-manual.pdf", "http://qtractor.sourceforge.net/") ],
  [ "qtractor-svn",  "Qtractor (SVN)",  "DAW",            "qtractor",         "qtractor",         TEMPLATE_YES, LEVEL_1,   "OpenSource", (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.5.x-user-manual.pdf", "http://qtractor.sourceforge.net/") ],

  [ "radium",        "Radium",          "Tracker",        "radium",           "radium",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, 0, 1, "Native",  0, 1, "ALSA"),        ("",                                                               "http://users.notam02.no/~kjetism/radium/") ],

  [ "reaper",        "REAPER",          "DAW",            "reaper",           "reaper",           TEMPLATE_NO,  LEVEL_0,   "Demo",       (0, 0, 0, 1, "Windows", 1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/ReaperUserGuide426C.pdf",        "http://www.reaper.fm/") ],
  [ "reaper:i386",   "REAPER",          "DAW",            "reaper",           "reaper",           TEMPLATE_NO,  LEVEL_0,   "Demo",       (0, 0, 0, 1, "Windows", 1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/ReaperUserGuide426C.pdf",        "http://www.reaper.fm/") ],

  [ "renoise",       "Renoise",         "Tracker",        "renoise",          "renoise",          TEMPLATE_YES, LEVEL_0,   "ShareWare",  (1, 1, 0, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/Renoise User Manual.pdf",        "http://www.renoise.com/") ],

  [ "rosegarden",    "Rosegarden",      "MIDI Sequencer", "rosegarden",       "rosegarden",       TEMPLATE_YES, LEVEL_1,   "OpenSource", (1, 1, 0, 0, "",        1, 1, "ALSA"),        ("",                                                               "http://www.rosegardenmusic.com/") ],

  [ "seq24",         "Seq24",           "MIDI Sequencer", "seq24",            "seq24",            TEMPLATE_YES, LEVEL_1,   "OpenSource", (0, 0, 0, 0, "",        1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/SEQ24",                          "http://www.filter24.org/seq24/") ],

  [ "sunvox",        "SunVox",          "Tracker",        "sunvox",           "sunvox",           TEMPLATE_NO,  LEVEL_0,   "FreeWare",   (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("file:///usr/share/sunvox/docs/manual/manual.html",               "http://www.warmplace.ru/soft/sunvox/") ],

  [ "traverso",      "Traverso",        "DAW",            "traverso",         "traverso",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, 1, 0, "",        1, 0, ""),            ("file:///usr/share/kxstudio/docs/traverso-manual-0.49.0.pdf",     "http://traverso-daw.org/") ]
]

iDAW_Package, iDAW_AppName, iDAW_Type, iDAW_Binary, iDAW_Icon, iDAW_Template, iDAW_Level, iDAW_RelModel, iDAW_Features, iDAW_Docs = range(0, len(list_DAW[0]))

if USING_KXSTUDIO:
    # Ardour 2.8
    list_DAW[0][iDAW_Level] = LEVEL_1
    # Jacker
    list_DAW[8][iDAW_Level] = LEVEL_1

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Host

# (I, L, D, L, V, VST-Mode, MIDI-Mode) -> (Internal, LADSPA, DSSI, LV2, VST, VST-Mode, MIDI-Mode)

list_Host = [
  # Package         AppName           Ins?   FX?    Binary           Icon         Template?     Level     Rel.-Model    (I, L, D, L, V, VST-Mode,  MIDI-Mode)      (doc-file,                                website)
  [ "calf-plugins", "Calf Jack Host", "Yes", "Yes", "calfjackhost",  "calf",      TEMPLATE_NO,  LEVEL_1,  "OpenSource", (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/index.html", "http://calf.sourceforge.net/") ],

  [ "carla",        "Carla",          "Yes", "Yes", "carla",         "carla",     TEMPLATE_YES, LEVEL_1,  "OpenSource", (1, 1, 1, 1, 1, "Both",    "ALSA | JACK"), ("",                                      "http://kxstudio.sourceforge.net/KXStudio:Applications:Carla") ],

  [ "festige",      "FeSTige",        "Yes", "Yes", "festige",       "festige",   TEMPLATE_NO,  LEVEL_1,  "OpenSource", (0, 0, 0, 0, 1, "Windows", "ALSA | JACK"), ("",                                      "http://festige.sourceforge.net/") ],

  [ "ingen",        "Ingen",          "Yes", "Yes", "ingen -eg",     "ingen",     TEMPLATE_NO,  LEVEL_0,  "OpenSource", (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://drobilla.net/blog/software/ingen/") ],
  [ "ingen-svn",    "Ingen (SVN)",    "Yes", "Yes", "ingen-svn -eg", "ingen",     TEMPLATE_NO,  LEVEL_0,  "OpenSource", (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://drobilla.net/blog/software/ingen/") ],

  [ "jack-rack",    "Jack Rack",      "No",  "Yes", "jack-rack",     "jack-rack", TEMPLATE_YES, LEVEL_0,  "OpenSource", (0, 1, 0, 0, 0, "",        "ALSA"),        ("",                                      "http://jack-rack.sourceforge.net/") ],

  [ "zynjacku",     "LV2 Rack",       "No",  "Yes", "lv2rack",       "zynjacku",  TEMPLATE_NO,  LEVEL_0,  "OpenSource", (0, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://home.gna.org/zynjacku/") ],
  [ "zynjacku",     "ZynJackU",       "Yes", "No",  "zynjacku",      "zynjacku",  TEMPLATE_NO,  LEVEL_0,  "OpenSource", (0, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://home.gna.org/zynjacku/") ]
]

iHost_Package, iHost_AppName, iHost_Ins, iHost_FX, iHost_Binary, iHost_Icon, iHost_Template, iHost_Level, iHost_RelModel, iHost_Features, iDAW_Docs = range(0, len(list_Host[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Instrument

# (F, I, MIDI-Mode) -> (Built-in FX, Audio Input, MIDI-Mode)

list_Instrument = [
  # Package                 AppName              Type                Binary                    Icon                Template?     Level      Rel.-Model    (F, I, MIDI-Mode)      (doc-file,                                                            website)
  [ "aeolus",               "Aeolus",            "Synth",            "aeolus -J",              generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, "ALSA | JACK"), ("",                                                                  "http://www.kokkinizita.net/linuxaudio/aeolus/index.html") ],

  [ "amsynth",              "amSynth",           "Synth",            "amsynth",                "amsynth",          TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "ALSA + JACK"), ("",                                                                  "") ],

  [ "azr3-jack",            "AZR3",              "Synth",            "azr3",                   "azr3",             TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "JACK"),        ("",                                                                  "http://ll-plugins.nongnu.org/azr3/") ],

  [ "distrho-plugin-ports", "Vex",               "Synth",            "vex",                    generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, "ALSA"),        ("",                                                                  "") ],
  [ "highlife",             "HighLife",          "Sampler",          "highlife",               generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, "ALSA"),        ("",                                                                  "http://www.discodsp.com/highlife/") ],
  [ "juced-plugins",        "DrumSynth",         "Synth",            "drumsynth",              "juced_plugins",    TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, "ALSA"),        ("",                                                                  "") ],
  [ "tal-plugins",          "TAL NoiseMaker",    "Synth",            "TAL-NoiseMaker",         "tal_plugins",      TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/TAL Noisemaker User Guide 1.0.pdf", "http://kunz.corrupt.ch/products/tal-noisemaker") ],
  [ "wolpertinger",         "Wolpertinger",      "Synth",            "Wolpertinger",           "wolpertinger",     TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "ALSA"),        ("",                                                                  "http://tumbetoene.tuxfamily.org") ],

  [ "foo-yc20",             "Foo YC20",          "Synth",            "foo-yc20",               "foo-yc20",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, "JACK"),        ("",                                                                  "http://code.google.com/p/foo-yc20/") ],

  [ "jsampler",             "JSampler Fantasia", "Sampler",          "jsampler-bin",           "jsampler",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, "ALSA + JACK"), ("file:///usr/share/kxstudio/docs/jsampler/jsampler.html",            "http://www.linuxsampler.org/") ],

  [ "loomer-plugins",       "Aspect",            "Synth",            "Aspect",                 "loomer",           TEMPLATE_NO,  LEVEL_0,   "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Aspect Manual.pdf.gz",         "http://www.loomer.co.uk/aspect.htm") ],
  [ "loomer-plugins",       "Sequent",           "Synth",            "Sequent",                "loomer",           TEMPLATE_NO,  LEVEL_0,   "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Sequent Manual.pdf.gz",        "http://www.loomer.co.uk/sequent.htm") ],
  [ "loomer-plugins",       "String",            "Synth",            "String",                 "loomer",           TEMPLATE_NO,  LEVEL_0,   "Demo",       (1, 1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",         "http://www.loomer.co.uk/string.htm") ],

  [ "petri-foo",            "Petri-Foo",         "Sampler",          "petri-foo",              "petri-foo",        TEMPLATE_NO,  LEVEL_NSM, "OpenSource", (0, 0, "ALSA + JACK"), ("",                                                                  "http://petri-foo.sourceforge.net/") ],

  [ "phasex",               "Phasex",            "Synth",            "phasex",                 "phasex",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 1, "ALSA"),        ("file:///usr/share/phasex/help/parameters.help",                     "") ],

  [ "pianoteq",             "Pianoteq",          "Synth",            "Pianoteq",               "pianoteq",         TEMPLATE_NO,  LEVEL_0,   "Demo",       (1, 0, "ALSA + JACK"), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq4") ],
  [ "pianoteq-stage",       "Pianoteq Stage",    "Synth",            "Pianoteq-STAGE",         "pianoteq",         TEMPLATE_NO,  LEVEL_0,   "Demo",       (1, 0, "ALSA + JACK"), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq_stage") ],

  [ "qsampler",             "Qsampler",          "Sampler",          "qsampler",               "qsampler",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (0, 0, "ALSA + JACK"), ("",                                                                  "http://qsampler.sourceforge.net/") ],

  [ "qsynth",               "Qsynth",            "SoundFont Player", "qsynth -a jack -m jack", "qsynth",           TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "ALSA | JACK"), ("",                                                                  "http://qsynth.sourceforge.net/") ],

  [ "setbfree",             "setBfree",          "Synth",            "setBfree-start",         "setBfree",         TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "JACK"),        ("",                                                                  "http://setbfree.org/") ],

  [ "yoshimi",              "Yoshimi",           "Synth",            "yoshimi -j -J",          "yoshimi",          TEMPLATE_NO,  LEVEL_1,   "OpenSource", (1, 0, "ALSA | JACK"), ("",                                                                  "http://yoshimi.sourceforge.net/") ],

  [ "zynaddsubfx",          "ZynAddSubFX",       "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_0,   "OpenSource", (1, 0, "ALSA + JACK"), ("",                                                                  "http://zynaddsubfx.sourceforge.net/") ]
]

iInstrument_Package, iInstrument_AppName, iInstrument_Type, iInstrument_Binary, iInstrument_Icon, iInstrument_Template, iInstrument_Level, iInstrument_RelModel, iInstrument_Features, iInstrument_Docs = range(0, len(list_Instrument[0]))

if USING_KXSTUDIO:
    # Qsampler
    list_Instrument[16][iInstrument_Level] = LEVEL_1

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Bristol

# Need name: bit99, bit100

list_Bristol = [
  # Package    AppName                           Type     Short-name    Icon                  Template?     Level    Rel.-Model    (F, I, MIDI-Mode)      (doc-file, website)
  [ "bristol", "Moog Voyager",                   "Synth", "explorer",   "bristol_explorer",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/explorer.html") ],
  [ "bristol", "Moog Mini",                      "Synth", "mini",       "bristol_mini",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mini.html") ],
  [ "bristol", "Sequential Circuits Prophet-52", "Synth", "prophet52",  "bristol_prophet52",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet52.html") ],

  [ "bristol", "Moog/Realistic MG-1",            "Synth", "realistic",  "bristol_realistic",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/realistic.html") ],
  [ "bristol", "Memory Moog",                    "Synth", "memoryMoog", "bristol_memoryMoog", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/memorymoog.html") ],
  [ "bristol", "Baumann BME-700",                "Synth", "BME700",     "bristol_BME700",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/bme700.shtml") ],
 #[ "bristol", "Synthi Aks",                     "Synth", "aks",        "bristol_aks",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/aks.html") ],

  [ "bristol", "Moog Voyager Blue Ice",          "Synth", "voyager",    "bristol_voyager",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/voyager.html") ],
  [ "bristol", "Moog Sonic-6",                   "Synth", "sonic6",     "bristol_sonic6",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/sonic6.html") ],
  [ "bristol", "Hammond B3",                     "Synth", "hammondB3",  "bristol_hammondB3",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/hammond.html") ],
  [ "bristol", "Sequential Circuits Prophet-5",  "Synth", "prophet",    "bristol_prophet",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet5.html") ],
  [ "bristol", "Sequential Circuits Prophet-10", "Synth", "prophet10",  "bristol_prophet10",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/prophet10.html") ],
  [ "bristol", "Sequential Circuits Pro-1",      "Synth", "pro1",       "bristol_pro1",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/pro1.html") ],
  [ "bristol", "Fender Rhodes Stage-73",         "Synth", "rhodes",     "bristol_rhodes",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/rhodes.html") ],
  [ "bristol", "Rhodes Bass Piano",              "Synth", "rhodesbass", "bristol_rhodesbass", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/rhodes.html") ],
  [ "bristol", "Crumar Roadrunner",              "Synth", "roadrunner", "bristol_roadrunner", TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/roadrunner.html") ],
  [ "bristol", "Crumar Bit-1",                   "Synth", "bitone",     "bristol_bitone",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/bitone.html") ],
  [ "bristol", "Crumar Stratus",                 "Synth", "stratus",    "bristol_stratus",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/stratus.html") ],
  [ "bristol", "Crumar Trilogy",                 "Synth", "trilogy",    "bristol_trilogy",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/trilogy.html") ],
  [ "bristol", "Oberheim OB-X",                  "Synth", "obx",        "bristol_obx",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/obx.html") ],
  [ "bristol", "Oberheim OB-Xa",                 "Synth", "obxa",       "bristol_obxa",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/obxa.html") ],
  [ "bristol", "ARP Axxe",                       "Synth", "axxe",       "bristol_axxe",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/axxe.html") ],
  [ "bristol", "ARP Odyssey",                    "Synth", "odyssey",    "bristol_odyssey",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/odyssey.html") ],
  [ "bristol", "ARP 2600",                       "Synth", "arp2600",    "bristol_arp2600",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/arp2600.html") ],
  [ "bristol", "ARP Solina Strings",             "Synth", "solina",     "bristol_solina",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/solina.html") ],
  [ "bristol", "Korg Poly-800",                  "Synth", "poly800",    "bristol_poly800",    TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/poly800.shtml") ],
  [ "bristol", "Korg Mono/Poly",                 "Synth", "monopoly",   "bristol_monopoly",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mono.html") ],
  [ "bristol", "Korg Polysix",                   "Synth", "poly",       "bristol_poly",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/poly.html") ],
  [ "bristol", "Korg MS-20 (*)",                 "Synth", "ms20",       "bristol_ms20",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/ms20.html") ],
  [ "bristol", "VOX Continental",                "Synth", "vox",        "bristol_vox",        TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/vox.html") ],
  [ "bristol", "VOX Continental 300",            "Synth", "voxM2",      "bristol_voxM2",      TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/vox300.html") ],
  [ "bristol", "Roland Juno-6",                  "Synth", "juno",       "bristol_juno",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/juno.html") ],
  [ "bristol", "Roland Jupiter 8",               "Synth", "jupiter8",   "bristol_jupiter8",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/jupiter8.html") ],
 #[ "bristol", "Bristol BassMaker",              "Synth", "bassmaker",  "bristol_bassmaker",  TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ],
  [ "bristol", "Yamaha DX",                      "Synth", "dx",         "bristol_dx",         TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/dx.html") ],
 #[ "bristol", "Yamaha CS-80",                   "Synth", "cs80",       "bristol_cs80",       TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/cs80.html") ],
  [ "bristol", "Bristol SID Softsynth",          "Synth", "sidney",     "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/sidney.shtml") ]
 #[ "bristol", "Commodore-64 SID polysynth",     "Synth", "melbourne",  "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ], #FIXME - needs icon
 #[ "bristol", "Bristol Granular Synthesiser",   "Synth", "granular",   "bristol_granular",   TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "") ],
 #[ "bristol", "Bristol Realtime Mixer",         "Synth", "mixer",      "bristol_mixer",      TEMPLATE_NO,  LEVEL_1, "OpenSource", (1, 1, "ALSA | JACK"), ("", "http://bristol.sourceforge.net/mixer.html") ]
]

iBristol_Package, iBristol_AppName, iBristol_Type, iBristol_ShortName, iBristol_Icon, iBristol_Template, iBristol_Level, iBristol_RelModel, iBristol_Features, iBristol_Docs = range(0, len(list_Bristol[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Plugin, TODO

# (S, M, P) -> (Stereo, MIDI-In, factory Presets)

list_Plugin = [
  # Package                 PluginName                Spec    Type          Filename                               Label/URI          Icon                Rel.-Model    (S, M, P)  (doc,                                     website)

  # DSSI
  [ "amsynth",              "amSynth DSSI",           "DSSI", "Synth",      "/usr/lib/dssi/amsynth_dssi.so",       "amsynth",         "amsynth",          "OpenSource", (1, 1, 1), ("", "") ],

  [ "dssi-example-plugins", "Less Trivial synth",     "DSSI", "Synth",      "/usr/lib/dssi/less_trivial_synth.so", "LTS",             generic_audio_icon, "OpenSource", (0, 1, 0), ("", "") ],
  [ "dssi-example-plugins", "Simple Mono Sampler",    "DSSI", "Sampler",    "/usr/lib/dssi/trivial_sampler.so",    "mono_sampler",    generic_audio_icon, "OpenSource", (0, 1, 0), ("", "") ],
  [ "dssi-example-plugins", "Simple Stereo Sampler",  "DSSI", "Sampler",    "/usr/lib/dssi/trivial_sampler.so",    "stereo_sampler",  generic_audio_icon, "OpenSource", (1, 1, 0), ("", "") ],

  [ "fluidsynth-dssi",      "FluidSynth DSSI",        "DSSI", "Sampler",    "/usr/lib/dssi/fluidsynth-dssi.so",    "FluidSynth-DSSI", generic_audio_icon, "OpenSource", (1, 1, 0), ("", "") ],

  [ "hexter",               "Hexter DX7 emulation",   "DSSI", "Synth",      "/usr/lib/dssi/hexter.so",             "hexter",          generic_audio_icon, "OpenSource", (0, 1, 1), ("", "") ],

  [ "holap",                "Horgand",                "DSSI", "Synth",      "/usr/lib/dssi/horgand.so",            "horgand",         generic_audio_icon, "OpenSource", (1, 1, 1), ("", "") ],

  [ "ls16",                 "LinuxSampler 16Ch Rack", "DSSI", "Sampler",    "/usr/lib/dssi/ls16.so",               "ls16",            generic_audio_icon, "OpenSource", (1, 1, 0), ("", "") ],

  [ "ll-scope",             "Oscilloscope",           "DSSI", "Util",       "/usr/lib/dssi/ll-scope.so",           "ll-scope",        "/usr/lib/dssi/ll-scope/icon.png", "OpenSource", (1, 0, 0), ("", "") ],
  [ "nekobee",              "Nekobee DSSI",           "DSSI", "Synth",      "/usr/lib/dssi/nekobee.so",            "nekobee",         generic_audio_icon, "OpenSource", (0, 1, 1), ("", "") ],
  [ "sineshaper",           "Sineshaper",             "DSSI", "Synth",      "/usr/lib/dssi/sineshaper.so",         "ll-sineshaper",   "sineshaper32x32",  "OpenSource", (0, 1, 1), ("", "") ],

  [ "whysynth",             "WhySynth",               "DSSI", "Synth",      "/usr/lib/dssi/whysynth.so",           "WhySynth",        generic_audio_icon, "OpenSource", (1, 1, 1), ("", "") ],
  [ "wsynth-dssi",          "Wsynth DSSI",            "DSSI", "Synth",      "/usr/lib/dssi/wsynth-dssi.so",        "Wsynth",          generic_audio_icon, "OpenSource", (0, 1, 1), ("", "") ],
  [ "xsynth-dssi",          "Xsynth DSSI",            "DSSI", "Synth",      "/usr/lib/dssi/xsynth-dssi.so",        "Xsynth",          generic_audio_icon, "OpenSource", (0, 1, 1), ("", "") ],

  # LV2
  [ "calf-plugins",         "Calf Compressor",        "LV2",  "Compressor", "/usr/lib/lv2/calf.lv2/",              "http://calf.sourceforge.net/plugins/Compressor", "calf", "OpenSource", (1, 0, 0), ("file:///usr/share/doc/calf/Compressor.html", "http://calf.sourceforge.net/") ],

  # VST
  [ "arctican-plugins-vst",     "The Function [VST]",                "VST", "Delay",      "/usr/lib/vst/TheFunction.so",                "The Function",               "arctican_plugins", "OpenSource", (1, 0, 1), ("", "http://arcticanaudio.com/plugins/") ],
  [ "arctican-plugins-vst",     "The Pilgrim [VST]",                 "VST", "Filter",     "/usr/lib/vst/ThePilgrim.so",                 "The Pilgrim",                "arctican_plugins", "OpenSource", (1, 1, 0), ("", "http://arcticanaudio.com/plugins/") ],

  [ "distrho-plugins-vst",      "3 Band EQ [VST]",                   "VST", "EQ",         "/usr/lib/vst/3BandEQ.so",                    "3BandEQ",                    "distrho_plugins",  "OpenSource", (1, 0, 0), ("", "") ],
  [ "distrho-plugins-vst",      "3 Band Splitter [VST]",             "VST", "EQ",         "/usr/lib/vst/3BandSplitter.so",              "3BandSplitter",              "distrho_plugins",  "OpenSource", (0, 0, 0), ("", "") ],
  [ "distrho-plugins-vst",      "Ping Pong Pan [VST]",               "VST", "Pan",        "/usr/lib/vst/PingPongPan.so",                "Ping Pong Pan",              "distrho_plugins",  "OpenSource", (1, 0, 0), ("", "") ],

  [ "distrho-plugin-ports-vst", "Vex [VST]",                         "VST", "Synth",      "/usr/lib/vst/vex.so",                        "Vex",                        generic_audio_icon, "OpenSource", (1, 1, 1), ("", "") ],

  [ "drowaudio-plugins-vst",    "dRowAudio Distortion-Shaper [VST]", "VST", "Distortion", "/usr/lib/vst/drowaudio-distortionshaper.so", "dRowAudio DistortionShaper", generic_audio_icon, "OpenSource", (1, 0, 0), ("", "") ],
  [ "drowaudio-plugins-vst",    "dRowAudio Distortion [VST]",        "VST", "Distortion", "/usr/lib/vst/drowaudio-distortion.so",       "dRowAudio Distortion",       generic_audio_icon, "OpenSource", (1, 0, 0), ("", "") ],
  [ "drowaudio-plugins-vst",    "dRowAudio Flanger [VST]",           "VST", "Flanger",    "/usr/lib/vst/drowaudio-flanger.so",          "dRowAudio Flanger",          generic_audio_icon, "OpenSource", (1, 0, 0), ("", "") ],
  [ "drowaudio-plugins-vst",    "dRowAudio Reverb [VST]",            "VST", "Reverb",     "/usr/lib/vst/drowaudio-reverb.so",           "dRowAudio Reverb",           generic_audio_icon, "OpenSource", (1, 1, 0), ("", "") ],
  [ "drowaudio-plugins-vst",    "dRowAudio Tremolo [VST]",           "VST", "Tremolo",    "/usr/lib/vst/drowaudio-tremolo.so",          "dRowAudio Tremolo",          generic_audio_icon, "OpenSource", (1, 0, 0), ("", "") ],

  [ "linuxdsp-plugins-vst",     "linuxDSP Black Equalizer [VST, Mono]",      "VST", "EQ",         "/usr/lib/vst/black-eq1.so", "BLACK-EQ1", "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/BLACK-EQ/manual.pdf.gz",      "http://www.linuxdsp.co.uk/download/lv2/download_black_eq/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP Multiband Compressor [VST]",       "VST", "Compressor", "/usr/lib/vst/mbc2b.so",     "MBC2B",     "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/MBC2B/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_mbc2/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP Vintage Program EQ [VST, Mono]",   "VST", "EQ",         "/usr/lib/vst/peq-1a.so",    "PEQ-1A",    "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/PEQ-2A/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_peq/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP Vintage Program EQ [VST, Stereo]", "VST", "EQ",         "/usr/lib/vst/peq-2a.so",    "PEQ-2A",    "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/PEQ-2A/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_peq/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP Vintage Compressor [VST, Mono]",   "VST", "Compressor", "/usr/lib/vst/vc1b.so",      "VC1B",      "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/VC2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_vc2b/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP Vintage Compressor [VST, Stereo]", "VST", "Compressor", "/usr/lib/vst/vc2b.so",      "VC2B",      "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/VC2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_vc2b/index.html") ],

  [ "linuxdsp-plugins-vst",     "linuxDSP DYN500 [VST, Mono]",               "VST", "Dynamics",   "/usr/lib/vst/DYN500-1.so",  "DYN500",    "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/DYN500/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP DYN500 [VST, Stereo]",             "VST", "Dynamics",   "/usr/lib/vst/DYN500-2.so",  "DYN500",    "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/DYN500/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP EQ500 [VST, Mono]",                "VST", "EQ",         "/usr/lib/vst/EQ500-1.so",   "EQ500",     "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/EQ500/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP EQ500 [VST, Stereo]",              "VST", "EQ",         "/usr/lib/vst/EQ500-2.so",   "EQ500",     "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/EQ500/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP GT500 [VST, Mono]",                "VST", "Gate",       "/usr/lib/vst/GT500-1.so",   "GT500",     "linuxdsp", "Demo", (0, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/GT500/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],
  [ "linuxdsp-plugins-vst",     "linuxDSP GT500 [VST, Stereo]",              "VST", "Gate",       "/usr/lib/vst/GT500-2.so",   "GT500",     "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/GT500/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_500/index.html") ],

  # FIXME - plugin UI broken upstream
  #[ "linuxdsp-plugins-vst",     "linuxDSP Black Equalizer [VST, Stereo]",    "VST", "EQ",         "/usr/lib/vst/black-eq2.so", "BLACK-EQ2", "linuxdsp", "Demo", (1, 0, 1), ("file:///usr/share/doc/linuxdsp-plugins/BLACK-EQ/manual.pdf.gz",      "http://www.linuxdsp.co.uk/download/lv2/download_black_eq/index.html") ],

  [ "loomer-plugins-vst",       "Aspect [VST]",                     "VST", "Synth",               "/usr/lib/vst/AspectVST.so",              "Aspect",         "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/Aspect Manual.pdf.gz",         "http://www.loomer.co.uk/aspect.htm") ],
  [ "loomer-plugins-vst",       "Manifold [VST]",                   "VST", "Enhancer",            "/usr/lib/vst/ManifoldVST.so",            "Manifold",       "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/Manifold Manual.pdf.gz",       "http://www.loomer.co.uk/manifold.htm") ],
  [ "loomer-plugins-vst",       "Resound [VST]",                    "VST", "Delay",               "/usr/lib/vst/ResoundVST.so",             "Resound",        "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/Resound Manual.pdf.gz",        "http://www.loomer.co.uk/resound.htm") ],
  [ "loomer-plugins-vst",       "Sequent [VST]",                    "VST", "Synth",               "/usr/lib/vst/SequentVST.so",             "Sequent",        "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/Sequent Manual.pdf.gz",        "http://www.loomer.co.uk/sequent.htm") ],
  [ "loomer-plugins-vst",       "Shift2 [VST]",                     "VST", "Pitch-Shifter/Delay", "/usr/lib/vst/Shift2VST.so",              "Shift2",         "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/Shift2 Manual.pdf.gz",         "http://www.loomer.co.uk/shift2.htm") ],
  [ "loomer-plugins-vst",       "String [VST]",                     "VST", "Synth",               "/usr/lib/vst/StringVST.so",              "String",         "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",         "http://www.loomer.co.uk/string.htm") ],
  [ "loomer-plugins-vst",       "String (FX) [VST]",                "VST", "Bundle",              "/usr/lib/vst/String_FXVST.so",           "String_FX",      "loomer",       "Demo",       (1, 1, 1), ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",         "http://www.loomer.co.uk/string.htm") ],

  [ "pianoteq-vst",             "Pianoteq [VST]",                   "VST", "Synth",               "/usr/lib/vst/Pianoteq 4.so",             "Pianoteq",       "pianoteq",     "Demo",       (0, 1, 1), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq4") ],
  [ "pianoteq-vst",             "Pianoteq [VST, Stereo]",           "VST", "Synth",               "/usr/lib/vst/Pianoteq 4_2chan.so",       "Pianoteq",       "pianoteq",     "Demo",       (1, 1, 1), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq4") ],
  [ "pianoteq-stage-vst",       "Pianoteq STAGE [VST]",             "VST", "Synth",               "/usr/lib/vst/Pianoteq 4 STAGE.so",       "Pianoteq STAGE", "pianoteq",     "Demo",       (0, 1, 1), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq_stage") ],
  [ "pianoteq-stage-vst",       "Pianoteq STAGE [VST, Stereo]",     "VST", "Synth",               "/usr/lib/vst/Pianoteq 4 STAGE_2chan.so", "Pianoteq STAGE", "pianoteq",     "Demo",       (1, 1, 1), ("file:///opt/Pianoteq/Documentation/pianoteq-english.pdf",           "http://www.pianoteq.com/pianoteq_stage") ],

  [ "tal-plugins-vst",          "TAL Dub 3 [VST]",                  "VST", "Delay",               "/usr/lib/vst/TAL-Dub-3.so",              "TAL Dub 2",      "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-dub") ],
  [ "tal-plugins-vst",          "TAL Filter [VST]",                 "VST", "Filter",              "/usr/lib/vst/TAL-Filter.so",             "TAL Filter",     "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-filter") ],
  [ "tal-plugins-vst",          "TAL Filter 2 [VST]",               "VST", "Filter",              "/usr/lib/vst/TAL-Filter-2.so",           "TAL Filter 2",   "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-filter") ],
  [ "tal-plugins-vst",          "TAL NoiseMaker [VST]",             "VST", "Synth",               "/usr/lib/vst/TAL-NoiseMaker.so",         "TAL NoiseMaker", "tal_plugins",  "OpenSource", (1, 1, 1), ("file:///usr/share/kxstudio/docs/TAL Noisemaker User Guide 1.0.pdf", "http://kunz.corrupt.ch/products/tal-noisemaker") ],
  [ "tal-plugins-vst",          "TAL Reverb [VST]",                 "VST", "Reverb",              "/usr/lib/vst/TAL-Reverb.so",             "TAL Reverb",     "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins-vst",          "TAL Reverb 2 [VST]",               "VST", "Reverb",              "/usr/lib/vst/TAL-Reverb-2.so",           "TAL Reverb 2",   "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins-vst",          "TAL Reverb 3 [VST]",               "VST", "Reverb",              "/usr/lib/vst/TAL-Reverb-3.so",           "TAL Reverb 3",   "tal_plugins",  "OpenSource", (1, 1, 1), ("",                                                                  "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins-vst",          "TAL Vocoder 2 [VST]",              "VST", "Vocoder",             "/usr/lib/vst/TAL-Vocoder-2.so",          "TAL Vocoder 2",  "tal_plugins",  "OpenSource", (1, 1, 1), ("file:///usr/share/kxstudio/docs/TAL-Vocoder-UserManual.pdf",        "http://kunz.corrupt.ch/products/tal-vocoder") ],

  [ "wolpertinger-vst",         "Wolpertinger [VST]",               "VST", "Synth",               "/usr/lib/vst/Wolpertinger.so",           "Wolpertinger",   "wolpertinger", "OpenSource", (1, 1, 1), ("",                                                                  "http://tumbetoene.tuxfamily.org") ],
]

iPlugin_Package, iPlugin_Name, iPlugin_Spec, iPlugin_Type, iPlugin_Filename, iPlugin_Label, iPlugin_Icon, iPlugin_RelModel, iPlugin_Features, iPlugin_Docs = range(0, len(list_Plugin[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Effect

# (S, MIDI-Mode) -> (Stereo, MIDI-Mode)

list_Effect = [
  # Package                 AppName                          Type                   Binary                                          Icon                Template?     Level    Rel.-Model    (S, MIDI-Mode)      (doc,                                                                  website)
  [ "ambdec",               "AmbDec",                        "Ambisonic Decoder",   "ambdec",                                       generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "---"),         ("",                                                                   "") ],

  [ "arctican-plugins",     "The Function",                  "Delay",               "TheFunction",                                  "arctican_plugins", TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://arcticanaudio.com/plugins/") ],
  [ "arctican-plugins",     "The Pilgrim",                   "Filter",              "ThePilgrim",                                   "arctican_plugins", TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://arcticanaudio.com/plugins/") ],

  [ "distrho-plugins",      "3 Band EQ",                     "EQ",                  "3BandEQ",                                      "distrho_plugins",  TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],
  [ "distrho-plugins",      "Ping Pong Pan",                 "Pan",                 "PingPongPan",                                  "distrho_plugins",  TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],

  [ "distrho-plugin-ports", "Argotlunar",                    "Granulator",          "argotlunar",                                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://argotlunar.info/") ],
  [ "distrho-plugin-ports", "BitMangler",                    "Misc",                "bitmangler",                                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],

  [ "drowaudio-plugins",    "dRowAudio Distortion",          "Distortion",          "drowaudio-distortion",                         generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],
  [ "drowaudio-plugins",    "dRowAudio Distortion-Shaper",   "Distortion",          "drowaudio-distortionshaper",                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],
  [ "drowaudio-plugins",    "dRowAudio Flanger",             "Flanger",             "drowaudio-flanger",                            generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],
  [ "drowaudio-plugins",    "dRowAudio Reverb",              "Reverb",              "drowaudio-reverb",                             generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],
  [ "drowaudio-plugins",    "dRowAudio Tremolo",             "Tremolo",             "drowaudio-tremolo",                            generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],

  [ "guitarix",             "Guitarix",                      "Guitar FX",           "guitarix",                                     "gx_head",          TEMPLATE_NO,  LEVEL_0, "OpenSource", (0, "JACK"),        ("",                                                                   "http://guitarix.sourceforge.net/") ],

  [ "hybridreverb2",        "HybridReverb2",                 "Reverb",              "HybridReverb2",                                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://www2.ika.rub.de/HybridReverb2/") ],

  [ "jamin",                "Jamin",                         "Mastering",           "jamin",                                        "jamin",            TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "---"),         ("",                                                                   "http://jamin.sourceforge.net/") ],

  [ "juced-plugins",        "EQinox",                        "EQ",                  "eqinox",                                       "juced_plugins",    TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "") ],

  [ "linuxdsp-plugins",     "linuxDSP Black Equalizer",      "EQ",                  "black-eq2-x86-64 || black-eq2-i686",           "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/BLACK-EQ/manual.pdf.gz",      "http://www.linuxdsp.co.uk/download/lv2/download_black_eq/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Channel Equaliser",    "EQ",                  "ch-eq2b-x86-64 || ch-eq2b-i686",               "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/CH-EQ2B/manual.pdf.gz",       "") ],
  [ "linuxdsp-plugins",     "linuxDSP Multiband Compressor", "Compressor",          "mbc2b-x86-64 || mbc2b-i686",                   "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/MBC2B/manual.pdf.gz",         "http://www.linuxdsp.co.uk/download/lv2/download_mbc2/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP MKII Graph Equalizer", "EQ",                  "mkii-graph-eq2-x86-64 || mkii-graph-eq2-i686", "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/MKII-GRAPH-EQ/manual.pdf.gz", "http://www.linuxdsp.co.uk/download/lv2/download_mkii_graph_eq/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Vintage Program EQ",   "EQ",                  "peq-2a-x86-64 || peq-2a-i686",                 "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/PEQ-2A/manual.pdf.gz",        "http://www.linuxdsp.co.uk/download/lv2/download_peq/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Reverb",               "Reverb",              "sr2b-x86-64 || sr2b-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/SR2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_sr_2b/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Vintage Compressor",   "Compressor",          "vc2b-x86-64 || vc2b-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/VC2B/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/lv2/download_vc2b/index.html") ],

  [ "linuxdsp-plugins",     "linuxDSP Guitar Chorus",        "Chorus",              "cr1-x86-64 || cr1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/CR1/manual.pdf.gz",           "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Guitar Distortion",    "Distortion",          "dt1-x86-64 || dt1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/DT1/dt1_manual.pdf.gz",       "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Guitar Phaser",        "Phaser",              "ph1-x86-64 || ph1-i686",                       "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/PH1/ph1_manual.pdf.gz",       "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Guitar WAH",           "Distortion",          "wah1-x86-64 || wah1-i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "Demo",       (0, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/WAH1/wah1_manual.pdf.gz",     "http://www.linuxdsp.co.uk/download/lv2/download_guitar_fx/index.html") ],
  [ "linuxdsp-plugins",     "linuxDSP Valve Overdrive",      "Amplifier",           "odv2_x86-64 || odv2_i686",                     "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "FreeWare",   (1, "---"),         ("file:///usr/share/doc/linuxdsp-plugins/ODV2/manual.pdf.gz",          "http://www.linuxdsp.co.uk/download/jack/download_odv2_jack/index.html") ],

  [ "loomer-plugins",       "Manifold",                      "Enhancer",            "Manifold",                                     "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Manifold Manual.pdf.gz",        "http://www.loomer.co.uk/manifold.htm") ],
  [ "loomer-plugins",       "Resound",                       "Delay",               "Resound",                                      "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Resound Manual.pdf.gz",         "http://www.loomer.co.uk/resound.htm") ],
  [ "loomer-plugins",       "Shift2",                        "Pitch-Shifter/Delay", "Shift2",                                       "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/Shift2 Manual.pdf.gz",          "http://www.loomer.co.uk/shift2.htm") ],
  [ "loomer-plugins",       "String (FX)",                   "Bundle",              "String_FX",                                    "loomer",           TEMPLATE_NO,  LEVEL_0, "Demo",       (1, "ALSA"),        ("file:///usr/share/doc/loomer-plugins/String Manual.pdf.gz",          "http://www.loomer.co.uk/string.htm") ],

  [ "rakarrack",            "Rakarrack",                     "Guitar FX",           "rakarrack",                                    "rakarrack",        TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA + JACK"), ("file:///usr/share/doc/rakarrack/html/help.html",                     "http://rakarrack.sourceforge.net") ],

  [ "tal-plugins",          "TAL Dub 3",                     "Delay",               "TAL-Dub-3",                                    "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-dub") ],
  [ "tal-plugins",          "TAL Filter",                    "Filter",              "TAL-Filter",                                   "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-filter") ],
  [ "tal-plugins",          "TAL Filter 2",                  "Filter",              "TAL-Filter-2",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-filter") ],
  [ "tal-plugins",          "TAL Reverb",                    "Reverb",              "TAL-Reverb",                                   "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins",          "TAL Reverb 2",                  "Reverb",              "TAL-Reverb-2",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins",          "TAL Reverb 3",                  "Reverb",              "TAL-Reverb-3",                                 "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("",                                                                   "http://kunz.corrupt.ch/products/tal-reverb") ],
  [ "tal-plugins",          "TAL Vocoder 2",                 "Vocoder",             "TAL-Vocoder-2",                                "tal_plugins",      TEMPLATE_NO,  LEVEL_0, "OpenSource", (1, "ALSA"),        ("file:///usr/share/kxstudio/docs/TAL-Vocoder-UserManual.pdf",         "http://kunz.corrupt.ch/products/tal-vocoder") ]
]

iEffect_Package, iEffect_AppName, iEffect_Type, iEffect_Binary, iEffect_Icon, iEffect_Template, iEffect_Level, iEffect_RelModel, iEffect_Features, iEffect_Docs = range(0, len(list_Effect[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Tool

# (MIDI-Mode, T) -> (MIDI-Mode, Transport)

list_Tool = [
  # Package              AppName                         Type                   Binary                    Icon                Template?     Level    Rel.-Model    (MIDI-Mode, T)      (doc,                                                             website)
  [ "arpage",            "Arpage",                       "MIDI Arpeggiator",    "arpage",                 "arpage",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ],
  [ "arpage",            "Zonage",                       "MIDI Mapper",         "zonage",                 "zonage",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "") ],

  [ "audacity",          "Audacity",                     "Audio Editor",        "audacity",               "audacity",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "http://audacity.sourceforge.net/") ],

  [ "cadence",           "Cadence",                      "JACK Toolbox",        "cadence",                "cadence",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ],
  [ "cadence-tools",     "Cadence XY-Controller",        "XY Controller",       "cadence-xycontroller",   "cadence",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "") ],
  [ "catia",             "Catia",                        "Patch Bay",           "catia",                  "catia",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ],
  [ "claudia",           "Claudia",                      "Session Handler",     "claudia",                "claudia",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ],
  [ "carla-control",     "Carla OSC Control",            "OSC Control",         "carla-control",          "carla-control",    TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        1), ("",                                                              "") ],

  [ "drumstick-tools",   "Drumstick Virtual Piano",      "Virtual Keyboard",    "drumstick-vpiano",       "drumstick",        TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://drumstick.sourceforge.net/") ],

  [ "fmit",              "Music Instrument Tuner",       "Instrument Tuner",    "fmit",                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ],

  [ "gigedit",           "Gigedit",                      "Instrument Editor",   "gigedit",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("/usr/share/doc/gigedit/gigedit_quickstart.html",                "") ],

  [ "gjacktransport",    "GJackClock",                   "Transport Tool",      "gjackclock",             "gjackclock",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ],
  [ "gjacktransport",    "GJackTransport",               "Transport Tool",      "gjacktransport",         "gjacktransport",   TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ],

  [ "gladish",           "LADI Session Handler",         "Session Handler",     "gladish",                "gladish",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "http://www.ladish.org") ],

  [ "gninjam",           "Gtk NINJAM client",            "Music Collaboration", "gninjam",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "") ],

  [ "jack-keyboard",     "Jack Keyboard",                "Virtual Keyboard",    "jack-keyboard",          "jack-keyboard",    TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("file:///usr/share/kxstudio/docs/jack-keyboard/manual.html",     "http://jack-keyboard.sourceforge.net/") ],

  [ "jack-mixer",        "Jack Mixer",                   "Mixer",               "jack_mixer",             "jack_mixer",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("",                                                              "http://home.gna.org/jackmixer/") ],

  [ "kmetronome",        "KMetronome",                   "Metronome",           "kmetronome",             "kmetronome",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://kmetronome.sourceforge.net/kmetronome.shtml") ],
  [ "kmidimon",          "KMidimon",                     "Monitor",             "kmidimon",               "kmidimon",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://kmidimon.sourceforge.net/") ],

  [ "laditools",         "LADI Log",                     "Log Viewer",          "ladilog",                "ladilog",          TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ],
  [ "laditools",         "LADI Tray",                    "Session Handler",     "laditray",               "laditray",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "") ],

  [ "linuxdsp-plugins",  "linuxDSP JACK Patch Bay",      "Patch Bay",           "jp1_x86-64 || jp1_i686", "linuxdsp",         TEMPLATE_NO,  LEVEL_0, "FreeWare",   ("JACK",        0), ("file:///usr/share/doc/linuxdsp-plugins/JP1/manual.pdf.gz",      "http://www.linuxdsp.co.uk/download/jack/download_jp1_jack/index.html") ],

  [ "lives",             "LiVES",                        "VJ / Video Editor",   "lives",                  "lives",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "http://lives.sourceforge.net/") ],

  [ "meterbridge",       "MeterBridge Classic VU",              "VU / Peak Analyzer", "meterbridge -t vu :",         "meterbridge32x32", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge PPM Meter",               "VU / Peak Analyzer", "meterbridge -t ppm :",        "meterbridge32x32", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge Digital Peak Meter",      "VU / Peak Analyzer", "meterbridge -t dpm -c 2 : :", "meterbridge32x32", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge 'Jellyfish' Phase Meter", "VU / Peak Analyzer", "meterbridge -t jf -c 2 : :",  "meterbridge32x32", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge Oscilloscope Meter",      "VU / Peak Analyzer", "meterbridge -t sco :",        "meterbridge32x32", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],

  [ "mhwaveedit",        "MhWaveEdit",                   "Audio Editor",        "mhwaveedit",             "mhwaveedit",       TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         0), ("",                                                              "http://gna.org/projects/mhwaveedit/") ],

  [ "mixxx",             "Mixxx",                        "DJ",                  "mixxx",                  "mixxx",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("file:///usr/share/kxstudio/docs/Mixxx-Manual.pdf",              "http://mixxx.sourceforge.net/") ],

  [ "non-mixer",         "Non-Mixer",                    "Mixer",               "non-mixer",              "non-mixer",        TEMPLATE_NO,  LEVEL_0, "OpenSource", ("CV",          0), ("file:///usr/share/doc/non-mixer/MANUAL.html",                   "http://non-daw.tuxfamily.org/") ],

  [ "patchage",          "Patchage",                     "Patch Bay",           "patchage",               "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage",          "Patchage (ALSA Only)",         "Patch Bay",           "patchage -J",            "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage-svn",      "Patchage (SVN)",               "Patch Bay",           "patchage-svn",           "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage-svn",      "Patchage (SVN, ALSA Only)",    "Patch Bay",           "patchage-svn -J",        "patchage",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],

  [ "qjackctl",          "QJackControl",                 "JACK Control",        "qjackctl",               "qjackctl",         TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA + JACK", 1), ("",                                                              "") ],

  [ "qamix",             "QAMix",                        "Mixer",               "qamix",                  "qamix",            TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ],
  [ "qarecord",          "QARecord",                     "Recorder",            "qarecord --jack",        "qarecord_48",      TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ],
  [ "qmidiarp",          "QMidiArp",                     "MIDI Arpeggiator",    "qmidiarp",               generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("",                                                              "") ],

  [ "timemachine",       "TimeMachine",                  "Recorder",            "timemachine",            "/usr/share/timemachine/pixmaps/timemachine-icon.png", TEMPLATE_NO, LEVEL_0, "OpenSource", ("---", 0), ("",                                    "http://plugin.org.uk/timemachine/") ],

  [ "vmpk",              "Virtual MIDI Piano Keyboard (ALSA)","Virtual Keyboard","vmpk",                  "vmpk",             TEMPLATE_NO,  LEVEL_0, "OpenSource", ("ALSA",        0), ("file:///usr/share/vmpk/help.html",                              "http://vmpk.sourceforge.net/") ],
  [ "vmpk-jack",         "Virtual MIDI Piano Keyboard (JACK)","Virtual Keyboard","vmpk-jack",             "vmpk",             TEMPLATE_NO,  LEVEL_0, "OpenSource", ("JACK",        0), ("file:///usr/share/vmpk/help.html",                              "http://vmpk.sourceforge.net/") ],

  [ "xjadeo",            "XJadeo",                       "Video Player",        "qjadeo",                 "qjadeo",           TEMPLATE_NO,  LEVEL_0, "OpenSource", ("---",         1), ("",                                                              "http://xjadeo.sourceforge.net/") ]
]

iTool_Package, iTool_AppName, iTool_Type, iTool_Binary, iTool_Icon, iTool_Template, iTool_Level, iTool_RelModel, iTool_Features, iTool_Docs = range(0, len(list_Tool[0]))


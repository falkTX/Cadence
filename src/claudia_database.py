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

USING_KXSTUDIO = False

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# DAW

# (L, D, L, V, VST-Mode, T, M, MIDI-Mode) -> (LADSPA, DSSI, LV2, VST, VST-Mode, Transport, MIDI, MIDI-Mode)

# TODO:
# luppp
# protrekkr

list_DAW = [
  # Package          AppName            Type              Binary              Icon                Template?     Level      (L, D, L, V, VST-Mode,  T, M, MIDI-Mode)      (doc-file,                                                         website)
  [ "ardour",        "Ardour 2.8",      "DAW",            "ardour2",          "ardour",           TEMPLATE_YES, LEVEL_0,   (1, 0, 1, 0, "",        1, 0, "ALSA"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ],
  [ "ardour3",       "Ardour 3",        "DAW",            "ardour3",          "ardour",           TEMPLATE_YES, LEVEL_JS,  (1, 0, 1, 1, "Native",  1, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ],
  [ "ardour4",       "Ardour 4",        "DAW",            "ardour4",          "ardour",           TEMPLATE_NO,  LEVEL_JS,  (1, 0, 1, 1, "Native",  1, 1, "JACK"),        ("file:///usr/share/kxstudio/docs/ardour.pdf",                     "http://www.ardour.org/") ],

  [ "ariamaestosa",  "Aria Maestosa",   "MIDI Sequencer", "Aria",             "aria",             TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("",                                                               "http://ariamaestosa.sf.net/") ],

  [ "giada",         "Giada",           "Audio Looper",   "giada",            generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 1, "Native",  0, 0, ""),            ("",                                                               "http://www.monocasual.com/giada/") ],

  [ "hydrogen",      "Hydrogen",        "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],
  [ "hydrogen-git",  "Hydrogen (GIT)",  "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],
  [ "hydrogen-svn",  "Hydrogen (SVN)",  "Drum Sequencer", "hydrogen -d jack", "h2-icon",          TEMPLATE_YES, LEVEL_JS,  (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html.upstream",    "http://www.hydrogen-music.org/") ],

  [ "jacker",        "Jacker",          "MIDI Sequencer", "jacker",           "jacker",           TEMPLATE_YES, LEVEL_0,   (0, 0, 0, 0, "",        1, 1, "JACK"),        ("",                                                               "https://bitbucket.org/paniq/jacker/wiki/Home") ],

  [ "lmms",          "LMMS",            "DAW",            "lmms",             "lmms",             TEMPLATE_YES, LEVEL_0,   (1, 0, 0, 1, "Windows", 0, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/LMMS_UserManual_0.4.12.1.pdf",   "http://lmms.sf.net/") ],

  [ "muse",          "MusE",            "DAW",            "muse",             "muse",             TEMPLATE_YES, LEVEL_0,   (1, 1, 0, 1, "Native",  1, 1, "ALSA + JACK"), ("file:///usr/share/doc/muse/html/window_ref.html",                "http://www.muse-sequencer.org/") ],

  [ "musescore",     "MuseScore",       "MIDI Composer",  "mscore",           "mscore",           TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("file:///usr/share/kxstudio/docs/MuseScore-en.pdf",               "http://www.musescore.org/") ],

  [ "non-sequencer", "Non-Sequencer",   "MIDI Sequencer", "non-sequencer",    "non-sequencer",    TEMPLATE_YES, LEVEL_NSM, (0, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/doc/non-sequencer/MANUAL.html",                "http://non.tuxfamily.org/wiki/Non%20Sequencer") ],
  [ "non-timeline",  "Non-Timeline",    "DAW",            "non-timeline",     "non-timeline",     TEMPLATE_YES, LEVEL_NSM, (0, 0, 0, 0, "",        1, 0, "CV + OSC"),    ("file:///usr/share/doc/non-timeline/MANUAL.html",                 "http://non.tuxfamily.org/wiki/Non%20Timeline") ],

  [ "qtractor",      "Qtractor",        "DAW",            "qtractor",         "qtractor",         TEMPLATE_YES, LEVEL_1,   (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.5.x-user-manual.pdf", "http://qtractor.sf.net/") ],
  [ "qtractor-svn",  "Qtractor (SVN)",  "DAW",            "qtractor",         "qtractor",         TEMPLATE_YES, LEVEL_1,   (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/qtractor-0.5.x-user-manual.pdf", "http://qtractor.sf.net/") ],

  [ "rosegarden",    "Rosegarden",      "MIDI Sequencer", "rosegarden",       "rosegarden",       TEMPLATE_YES, LEVEL_1,   (1, 1, 0, 0, "",        1, 1, "ALSA"),        ("",                                                               "http://www.rosegardenmusic.com/") ],

  [ "seq24",         "Seq24",           "MIDI Sequencer", "seq24",            "seq24",            TEMPLATE_YES, LEVEL_1,   (0, 0, 0, 0, "",        1, 1, "ALSA"),        ("file:///usr/share/kxstudio/docs/SEQ24",                          "http://www.filter24.org/seq24/") ],

  [ "traverso",      "Traverso",        "DAW",            "traverso",         "traverso",         TEMPLATE_NO,  LEVEL_0,   (1, 0, 1, 0, "",        1, 0, ""),            ("file:///usr/share/kxstudio/docs/traverso-manual-0.49.0.pdf",     "http://traverso-daw.org/") ],
]

iDAW_Package, iDAW_AppName, iDAW_Type, iDAW_Binary, iDAW_Icon, iDAW_Template, iDAW_Level, iDAW_Features, iDAW_Docs = range(0, len(list_DAW[0]))

if USING_KXSTUDIO:
    # Jacker
    list_DAW[10][iDAW_Level] = LEVEL_1

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Host

# (I, L, D, L, V, VST-Mode, MIDI-Mode) -> (Internal, LADSPA, DSSI, LV2, VST, VST-Mode, MIDI-Mode)

# TODO:
# ams
# mod-app
# spiralsynthmodular

list_Host = [
  # Package             AppName                 Ins?   FX?    Binary           Icon         Template?     Level      (I, L, D, L, V, VST-Mode,  MIDI-Mode)      (doc-file,                                website)
  [ "calf-plugins",     "Calf Jack Host",       "Yes", "Yes", "calfjackhost",  "calf",      TEMPLATE_NO,  LEVEL_1,   (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/index.html", "http://calf.sf.net/") ],
  [ "calf-plugins-git", "Calf Jack Host (GIT)", "Yes", "Yes", "calfjackhost",  "calf",      TEMPLATE_NO,  LEVEL_1,   (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/index.html", "http://calf.sf.net/") ],

  [ "carla",            "Carla",                "Yes", "Yes", "carla",         "carla",     TEMPLATE_YES, LEVEL_1,   (1, 1, 1, 1, 1, "Both",    "ALSA | JACK"), ("",                                      "http://kxstudio.sf.net/Applications:Carla") ],
  [ "carla-git",        "Carla (GIT)",          "Yes", "Yes", "carla",         "carla",     TEMPLATE_YES, LEVEL_NSM, (1, 1, 1, 1, 1, "Both",    "ALSA | JACK"), ("",                                      "http://kxstudio.sf.net/Applications:Carla") ],

  [ "festige",          "FeSTige",              "Yes", "Yes", "festige",       "festige",   TEMPLATE_NO,  LEVEL_1,   (0, 0, 0, 0, 1, "Windows", "ALSA | JACK"), ("",                                      "http://festige.sf.net/") ],

  [ "ingen",            "Ingen",                "Yes", "Yes", "ingen -eg",     "ingen",     TEMPLATE_NO,  LEVEL_0,   (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://drobilla.net/blog/software/ingen/") ],
  [ "ingen-svn",        "Ingen (SVN)",          "Yes", "Yes", "ingen-svn -eg", "ingen",     TEMPLATE_NO,  LEVEL_0,   (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://drobilla.net/blog/software/ingen/") ],

  [ "jack-rack",        "Jack Rack",            "No",  "Yes", "jack-rack",     "jack-rack", TEMPLATE_YES, LEVEL_0,   (0, 1, 0, 0, 0, "",        "ALSA"),        ("",                                      "http://jack-rack.sf.net/") ],
]

iHost_Package, iHost_AppName, iHost_Ins, iHost_FX, iHost_Binary, iHost_Icon, iHost_Template, iHost_Level, iHost_Features, iDAW_Docs = range(0, len(list_Host[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Instrument

# (F, I, MIDI-Mode) -> (Built-in FX, Audio Input, MIDI-Mode)

# TODO:
# add64
# cursynth

list_Instrument = [
  # Package                 AppName              Type                Binary                    Icon                Template?     Level      (F, I, MIDI-Mode)      (doc-file,                                                            website)
  [ "aeolus",               "Aeolus",            "Synth",            "aeolus -J",              generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA | JACK"), ("",                                                                  "http://www.kokkinizita.net/linuxaudio/aeolus/index.html") ],

  [ "amsynth",              "amSynth",           "Synth",            "amsynth",                "amsynth",          TEMPLATE_NO,  LEVEL_0,   (1, 0, "ALSA + JACK"), ("",                                                                  "") ],

  [ "azr3-jack",            "AZR3",              "Synth",            "azr3",                   "azr3",             TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                  "http://ll-plugins.nongnu.org/azr3/") ],

  [ "foo-yc20",             "Foo YC20",          "Synth",            "foo-yc20",               "foo-yc20",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "http://code.google.com/p/foo-yc20/") ],

  [ "jsampler",             "JSampler Fantasia", "Sampler",          "jsampler-bin",           "jsampler",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA + JACK"), ("file:///usr/share/kxstudio/docs/jsampler/jsampler.html",            "http://www.linuxsampler.org/") ],

  [ "petri-foo",            "Petri-Foo",         "Sampler",          "petri-foo",              "petri-foo",        TEMPLATE_NO,  LEVEL_NSM, (0, 0, "ALSA + JACK"), ("",                                                                  "http://petri-foo.sf.net/") ],

  [ "phasex",               "Phasex",            "Synth",            "phasex",                 "phasex",           TEMPLATE_NO,  LEVEL_0,   (1, 1, "ALSA"),        ("file:///usr/share/phasex/help/parameters.help",                     "") ],

  [ "qsampler",             "Qsampler",          "Sampler",          "qsampler",               "qsampler",         TEMPLATE_YES, LEVEL_0,   (0, 0, "ALSA + JACK"), ("",                                                                  "http://qsampler.sf.net/") ],

  [ "qsynth",               "Qsynth",            "SoundFont Player", "qsynth -a jack -m jack", "qsynth",           TEMPLATE_NO,  LEVEL_0,   (1, 0, "ALSA | JACK"), ("",                                                                  "http://qsynth.sf.net/") ],

  [ "setbfree",             "setBfree",          "Synth",            "setBfree-start",         "setBfree",         TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                  "http://setbfree.org/") ],

  [ "yoshimi",              "Yoshimi",           "Synth",            "yoshimi -j -J",          "yoshimi",          TEMPLATE_NO,  LEVEL_1,   (1, 0, "ALSA | JACK"), ("",                                                                  "http://yoshimi.sf.net/") ],

  [ "zynaddsubfx",          "ZynAddSubFX",       "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_NSM, (1, 0, "ALSA + JACK"), ("",                                                                  "http://zynaddsubfx.sf.net/") ],
  [ "zynaddsubfx-git",      "ZynAddSubFX (GIT)", "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_NSM, (1, 0, "ALSA + JACK"), ("",                                                                  "http://zynaddsubfx.sf.net/") ],
]

iInstrument_Package, iInstrument_AppName, iInstrument_Type, iInstrument_Binary, iInstrument_Icon, iInstrument_Template, iInstrument_Level, iInstrument_Features, iInstrument_Docs = range(0, len(list_Instrument[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Bristol

# Need name: bit99, bit100

list_Bristol = [
  # Package    AppName                           Type     Short-name    Icon                  Template?     Level    (F, I, MIDI-Mode)      (doc-file, website)
  [ "bristol", "Moog Voyager",                   "Synth", "explorer",   "bristol_explorer",   TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/explorer.html") ],
  [ "bristol", "Moog Mini",                      "Synth", "mini",       "bristol_mini",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/mini.html") ],
  [ "bristol", "Sequential Circuits Prophet-52", "Synth", "prophet52",  "bristol_prophet52",  TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/prophet52.html") ],

  [ "bristol", "Moog/Realistic MG-1",            "Synth", "realistic",  "bristol_realistic",  TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/realistic.html") ],
  [ "bristol", "Memory Moog",                    "Synth", "memoryMoog", "bristol_memoryMoog", TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/memorymoog.html") ],
  [ "bristol", "Baumann BME-700",                "Synth", "BME700",     "bristol_BME700",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/bme700.shtml") ],
 #[ "bristol", "Synthi Aks",                     "Synth", "aks",        "bristol_aks",        TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/aks.html") ],

  [ "bristol", "Moog Voyager Blue Ice",          "Synth", "voyager",    "bristol_voyager",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/voyager.html") ],
  [ "bristol", "Moog Sonic-6",                   "Synth", "sonic6",     "bristol_sonic6",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/sonic6.html") ],
  [ "bristol", "Hammond B3",                     "Synth", "hammondB3",  "bristol_hammondB3",  TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/hammond.html") ],
  [ "bristol", "Sequential Circuits Prophet-5",  "Synth", "prophet",    "bristol_prophet",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/prophet5.html") ],
  [ "bristol", "Sequential Circuits Prophet-10", "Synth", "prophet10",  "bristol_prophet10",  TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/prophet10.html") ],
  [ "bristol", "Sequential Circuits Pro-1",      "Synth", "pro1",       "bristol_pro1",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/pro1.html") ],
  [ "bristol", "Fender Rhodes Stage-73",         "Synth", "rhodes",     "bristol_rhodes",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/rhodes.html") ],
  [ "bristol", "Rhodes Bass Piano",              "Synth", "rhodesbass", "bristol_rhodesbass", TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/rhodes.html") ],
  [ "bristol", "Crumar Roadrunner",              "Synth", "roadrunner", "bristol_roadrunner", TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/roadrunner.html") ],
  [ "bristol", "Crumar Bit-1",                   "Synth", "bitone",     "bristol_bitone",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/bitone.html") ],
  [ "bristol", "Crumar Stratus",                 "Synth", "stratus",    "bristol_stratus",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/stratus.html") ],
  [ "bristol", "Crumar Trilogy",                 "Synth", "trilogy",    "bristol_trilogy",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/trilogy.html") ],
  [ "bristol", "Oberheim OB-X",                  "Synth", "obx",        "bristol_obx",        TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/obx.html") ],
  [ "bristol", "Oberheim OB-Xa",                 "Synth", "obxa",       "bristol_obxa",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/obxa.html") ],
  [ "bristol", "ARP Axxe",                       "Synth", "axxe",       "bristol_axxe",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/axxe.html") ],
  [ "bristol", "ARP Odyssey",                    "Synth", "odyssey",    "bristol_odyssey",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/odyssey.html") ],
  [ "bristol", "ARP 2600",                       "Synth", "arp2600",    "bristol_arp2600",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/arp2600.html") ],
  [ "bristol", "ARP Solina Strings",             "Synth", "solina",     "bristol_solina",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/solina.html") ],
  [ "bristol", "Korg Poly-800",                  "Synth", "poly800",    "bristol_poly800",    TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/poly800.shtml") ],
  [ "bristol", "Korg Mono/Poly",                 "Synth", "monopoly",   "bristol_monopoly",   TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/mono.html") ],
  [ "bristol", "Korg Polysix",                   "Synth", "poly",       "bristol_poly",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/poly.html") ],
  [ "bristol", "Korg MS-20 (*)",                 "Synth", "ms20",       "bristol_ms20",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/ms20.html") ],
  [ "bristol", "VOX Continental",                "Synth", "vox",        "bristol_vox",        TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/vox.html") ],
  [ "bristol", "VOX Continental 300",            "Synth", "voxM2",      "bristol_voxM2",      TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/vox300.html") ],
  [ "bristol", "Roland Juno-6",                  "Synth", "juno",       "bristol_juno",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/juno.html") ],
  [ "bristol", "Roland Jupiter 8",               "Synth", "jupiter8",   "bristol_jupiter8",   TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/jupiter8.html") ],
 #[ "bristol", "Bristol BassMaker",              "Synth", "bassmaker",  "bristol_bassmaker",  TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "") ],
  [ "bristol", "Yamaha DX",                      "Synth", "dx",         "bristol_dx",         TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/dx.html") ],
 #[ "bristol", "Yamaha CS-80",                   "Synth", "cs80",       "bristol_cs80",       TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/cs80.html") ],
  [ "bristol", "Bristol SID Softsynth",          "Synth", "sidney",     "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/sidney.shtml") ],
 #[ "bristol", "Commodore-64 SID polysynth",     "Synth", "melbourne",  "bristol_sidney",     TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "") ], #FIXME - needs icon
 #[ "bristol", "Bristol Granular Synthesiser",   "Synth", "granular",   "bristol_granular",   TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "") ],
 #[ "bristol", "Bristol Realtime Mixer",         "Synth", "mixer",      "bristol_mixer",      TEMPLATE_NO,  LEVEL_1, (1, 1, "ALSA | JACK"), ("", "http://bristol.sf.net/mixer.html") ],
]

iBristol_Package, iBristol_AppName, iBristol_Type, iBristol_ShortName, iBristol_Icon, iBristol_Template, iBristol_Level, iBristol_Features, iBristol_Docs = range(0, len(list_Bristol[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Effect

# (S, MIDI-Mode) -> (Stereo, MIDI-Mode)

list_Effect = [
  # Package                 AppName                          Type                   Binary                                          Icon                Template?     Level    (S, MIDI-Mode)      (doc,                                                                  website)
  [ "ambdec",               "AmbDec",                        "Ambisonic Decoder",   "ambdec",                                       generic_audio_icon, TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "") ],

  [ "guitarix",             "Guitarix",                      "Guitar FX",           "guitarix",                                     "gx_head",          TEMPLATE_NO,  LEVEL_0, (0, "JACK"),        ("",                                                                   "http://guitarix.sf.net/") ],

  [ "jamin",                "Jamin",                         "Mastering",           "jamin",                                        "jamin",            TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "http://jamin.sf.net/") ],

  [ "rakarrack",            "Rakarrack",                     "Guitar FX",           "rakarrack",                                    "rakarrack",        TEMPLATE_NO,  LEVEL_0, (1, "ALSA + JACK"), ("file:///usr/share/doc/rakarrack/html/help.html",                     "http://rakarrack.sf.net") ],
]

iEffect_Package, iEffect_AppName, iEffect_Type, iEffect_Binary, iEffect_Icon, iEffect_Template, iEffect_Level, iEffect_Features, iEffect_Docs = range(0, len(list_Effect[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Tool

# (MIDI-Mode, T) -> (MIDI-Mode, Transport)

# TODO:
# paulstretch

list_Tool = [
  # Package              AppName                         Type                   Binary                    Icon                Template?     Level    (MIDI-Mode, T)      (doc,                                                             website)
  [ "arpage",            "Arpage",                       "MIDI Arpeggiator",    "arpage",                 "arpage",           TEMPLATE_NO,  LEVEL_0, ("JACK",        1), ("",                                                              "") ],
  [ "arpage",            "Zonage",                       "MIDI Mapper",         "zonage",                 "zonage",           TEMPLATE_NO,  LEVEL_0, ("JACK",        0), ("",                                                              "") ],

  [ "audacity",          "Audacity",                     "Audio Editor",        "audacity",               "audacity",         TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "http://audacity.sf.net/") ],

  [ "cadence",           "Cadence",                      "JACK Toolbox",        "cadence",                "cadence",          TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "") ],
  [ "cadence-tools",     "Cadence XY-Controller",        "XY Controller",       "cadence-xycontroller",   "cadence",          TEMPLATE_NO,  LEVEL_0, ("JACK",        0), ("",                                                              "") ],
  [ "catia",             "Catia",                        "Patch Bay",           "catia",                  "catia",            TEMPLATE_NO,  LEVEL_0, ("JACK",        1), ("",                                                              "") ],
  [ "carla-control",     "Carla OSC Control",            "OSC Control",         "carla-control",          "carla-control",    TEMPLATE_NO,  LEVEL_0, ("JACK",        1), ("",                                                              "") ],

  [ "drumstick-tools",   "Drumstick Virtual Piano",      "Virtual Keyboard",    "drumstick-vpiano",       "drumstick",        TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "http://drumstick.sf.net/") ],

  [ "fmit",              "Music Instrument Tuner",       "Instrument Tuner",    "fmit",                   generic_audio_icon, TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "") ],

  [ "gigedit",           "Gigedit",                      "Instrument Editor",   "gigedit",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, ("---",         0), ("/usr/share/doc/gigedit/gigedit_quickstart.html",                "") ],

  [ "gjacktransport",    "GJackClock",                   "Transport Tool",      "gjackclock",             "gjackclock",       TEMPLATE_NO,  LEVEL_0, ("---",         1), ("",                                                              "") ],
  [ "gjacktransport",    "GJackTransport",               "Transport Tool",      "gjacktransport",         "gjacktransport",   TEMPLATE_NO,  LEVEL_0, ("---",         1), ("",                                                              "") ],

  [ "gninjam",           "Gtk NINJAM client",            "Music Collaboration", "gninjam",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, ("---",         1), ("",                                                              "") ],

  [ "jack-keyboard",     "Jack Keyboard",                "Virtual Keyboard",    "jack-keyboard",          "jack-keyboard",    TEMPLATE_NO,  LEVEL_0, ("JACK",        0), ("file:///usr/share/kxstudio/docs/jack-keyboard/manual.html",     "http://jack-keyboard.sf.net/") ],

  [ "jack-mixer",        "Jack Mixer",                   "Mixer",               "jack_mixer",             "jack_mixer",       TEMPLATE_NO,  LEVEL_0, ("JACK",        0), ("",                                                              "http://home.gna.org/jackmixer/") ],

  [ "kmetronome",        "KMetronome",                   "Metronome",           "kmetronome",             "kmetronome",       TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "http://kmetronome.sf.net/kmetronome.shtml") ],
  [ "kmidimon",          "KMidimon",                     "Monitor",             "kmidimon",               "kmidimon",         TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "http://kmidimon.sf.net/") ],

  [ "laditools",         "LADI Log",                     "Log Viewer",          "ladilog",                "ladilog",          TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "") ],
  [ "laditools",         "LADI Tray",                    "Session Handler",     "laditray",               "laditray",         TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "") ],

  [ "lives",             "LiVES",                        "VJ / Video Editor",   "lives",                  "lives",            TEMPLATE_NO,  LEVEL_0, ("---",         1), ("",                                                              "http://lives.sf.net/") ],

  [ "meterbridge",       "MeterBridge Classic VU",              "VU / Peak Analyzer", "meterbridge -t vu :",         "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge PPM Meter",               "VU / Peak Analyzer", "meterbridge -t ppm :",        "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge Digital Peak Meter",      "VU / Peak Analyzer", "meterbridge -t dpm -c 2 : :", "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge 'Jellyfish' Phase Meter", "VU / Peak Analyzer", "meterbridge -t jf -c 2 : :",  "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge Oscilloscope Meter",      "VU / Peak Analyzer", "meterbridge -t sco :",        "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                            "http://plugin.org.uk/meterbridge/") ],

  [ "mhwaveedit",        "MhWaveEdit",                   "Audio Editor",        "mhwaveedit",             "mhwaveedit",       TEMPLATE_NO,  LEVEL_0, ("---",         0), ("",                                                              "http://gna.org/projects/mhwaveedit/") ],

  [ "mixxx",             "Mixxx",                        "DJ",                  "mixxx",                  "mixxx",            TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("file:///usr/share/kxstudio/docs/Mixxx-Manual.pdf",              "http://mixxx.sf.net/") ],

  [ "non-mixer",         "Non-Mixer",                    "Mixer",               "non-mixer",              "non-mixer",        TEMPLATE_NO,  LEVEL_0, ("CV",          0), ("file:///usr/share/doc/non-mixer/MANUAL.html",                   "http://non.tuxfamily.org/wiki/Non%20Mixer") ],

  [ "patchage",          "Patchage",                     "Patch Bay",           "patchage",               "patchage",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage",          "Patchage (ALSA Only)",         "Patch Bay",           "patchage -J",            "patchage",         TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage-svn",      "Patchage (SVN)",               "Patch Bay",           "patchage-svn",           "patchage",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],
  [ "patchage-svn",      "Patchage (SVN, ALSA Only)",    "Patch Bay",           "patchage-svn -J",        "patchage",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 0), ("",                                                              "http://drobilla.net/blog/software/patchage/") ],

  [ "qjackctl",          "QJackControl",                 "JACK Control",        "qjackctl",               "qjackctl",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 1), ("",                                                              "") ],

  [ "qamix",             "QAMix",                        "Mixer",               "qamix",                  "qamix",            TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "") ],
  [ "qarecord",          "QARecord",                     "Recorder",            "qarecord --jack",        "qarecord_48",      TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "") ],
  [ "qmidiarp",          "QMidiArp",                     "MIDI Arpeggiator",    "qmidiarp",               generic_midi_icon,  TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("",                                                              "") ],

  [ "timemachine",       "TimeMachine",                  "Recorder",            "timemachine",            "/usr/share/timemachine/pixmaps/timemachine-icon.png", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                    "http://plugin.org.uk/timemachine/") ],

  [ "vmpk",              "Virtual MIDI Piano Keyboard (ALSA)","Virtual Keyboard","vmpk",                  "vmpk",             TEMPLATE_NO,  LEVEL_0, ("ALSA",        0), ("file:///usr/share/vmpk/help.html",                              "http://vmpk.sf.net/") ],
  [ "vmpk-jack",         "Virtual MIDI Piano Keyboard (JACK)","Virtual Keyboard","vmpk-jack",             "vmpk",             TEMPLATE_NO,  LEVEL_0, ("JACK",        0), ("file:///usr/share/vmpk/help.html",                              "http://vmpk.sf.net/") ],

  [ "xjadeo",            "XJadeo",                       "Video Player",        "qjadeo",                 "qjadeo",           TEMPLATE_NO,  LEVEL_0, ("---",         1), ("",                                                              "http://xjadeo.sf.net/") ],
]

iTool_Package, iTool_AppName, iTool_Type, iTool_Binary, iTool_Icon, iTool_Template, iTool_Level, iTool_Features, iTool_Docs = range(0, len(list_Tool[0]))

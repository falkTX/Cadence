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

list_DAW = [
  # Package          AppName            Type              Binary              Icon                Template?     Level      (L, D, L, V, VST-Mode,  T, M, MIDI-Mode)      (doc-file,                                                         website)
  [ "ardour",        "Ardour 6",        "DAW",     "ardour",  "/usr/share/pixmaps/ardour.svg",    TEMPLATE_NO,  LEVEL_NSM, (1, 0, 1, 0, "Native",  1, 1, "JACK"),        ("",                                                               "http://www.ardour.org/") ],

  [ "ariamaestosa",  "Aria Maestosa",   "MIDI Sequencer", "Aria",  "/usr/share/Aria/aria64.png",  TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        0, 1, "ALSA | JACK"), ("",                                                               "http://ariamaestosa.sf.net/") ],

  [ "giada",         "Giada",           "Audio Looper",   "giada",            "giada",            TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        0, 0, ""),            ("",                                                               "http://www.monocasual.com/giada/") ],

  [ "hydrogen",      "Hydrogen",        "Drum Sequencer", "hydrogen -d jack", "/usr/share/icons/hicolor/scalable/apps/org.hydrogenmusic.Hydrogen.svg",          TEMPLATE_YES, LEVEL_NSM, (1, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("file:///usr/share/hydrogen/data/doc/manual_en.html",             "http://www.hydrogen-music.org/") ],

  [ "jacker",        "Jacker",          "MIDI Tracker",   "jacker",           "jacker",           TEMPLATE_YES, LEVEL_0,   (0, 0, 0, 0, "",        1, 1, "JACK"),        ("",                                                               "https://bitbucket.org/paniq/jacker/wiki/Home") ],

  [ "klystrack",     "Klystrack",       "Chiptune Tracker", "klystrack",      "klystrack",        TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",       0, 0, "----"),        ("",                                                               "http://kometbomb.github.io/klystrack/") ],

  [ "lmms",          "LMMS",            "DAW",            "lmms",             "lmms",             TEMPLATE_YES, LEVEL_0,   (1, 0, 0, 1, "Windows", 0, 1, "ALSA"),        ("",                                                               "http://lmms.sf.net/") ],

  [ "milkytracker",  "MilkyTracker",    "Tracker",        "milkytracker",     "milkytracker",     TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        0, 1, "ALSA"),        ("",                                                               "https://milkytracker.org/") ],

  [ "muse",          "MusE",            "DAW",            "muse",             "muse_icon",        TEMPLATE_YES, LEVEL_0,   (1, 1, 0, 1, "Native",  1, 1, "ALSA + JACK"), ("",                                                               "http://www.muse-sequencer.org/") ],

  [ "musescore3",    "MuseScore 3",     "MIDI Composer",  "mscore3",          "mscore3",          TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",        1, 1, "ALSA | JACK"), ("",                                                               "http://www.musescore.org/") ],

  [ "non-sequencer", "Non-Sequencer",   "MIDI Sequencer", "non-sequencer",    "non-sequencer",    TEMPLATE_YES, LEVEL_NSM, (0, 0, 0, 0, "",        1, 1, "JACK"),        ("file:///usr/share/doc/non-sequencer/MANUAL.html",                "http://non.tuxfamily.org/wiki/Non%20Sequencer") ],
  [ "non-timeline",  "Non-Timeline",    "DAW",            "non-timeline",     "non-timeline",     TEMPLATE_YES, LEVEL_NSM, (0, 0, 0, 0, "",        1, 0, "CV + OSC"),    ("file:///usr/share/doc/non-timeline/MANUAL.html",                 "http://non.tuxfamily.org/wiki/Non%20Timeline") ],

  [ "protrekkr",     "ProTrekkr",       "Tracker",        "protrekkr",        "protrekkr",        TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 1, "",        0, 1, "ALSA"),        ("",                                                               "https://code.google.com/p/protrekkr/") ],

  [ "qtractor",      "Qtractor",        "DAW",            "qtractor",         "qtractor",         TEMPLATE_YES, LEVEL_1,   (1, 1, 1, 1, "Native",  1, 1, "ALSA"),        ("",                                                               "http://qtractor.sf.net/") ],

  [ "rosegarden",    "Rosegarden",      "MIDI Sequencer", "rosegarden",       "rosegarden",       TEMPLATE_YES, LEVEL_1,   (1, 1, 0, 0, "",        1, 1, "ALSA"),        ("",                                                               "http://www.rosegardenmusic.com/") ],

  [ "schism",        "Schism",  "Impulse Tracker clone",  "schismtracker",    "schism-icon-128",  TEMPLATE_YES, LEVEL_0,   (0, 0, 0, 0, "",        0, 1, "ALSA"),        ("",                                                               "http://schismtracker.org/") ],

  [ "seq24",         "Seq24",           "MIDI Sequencer", "seq24",            "seq24",            TEMPLATE_YES, LEVEL_1,   (0, 0, 0, 0, "",        1, 1, "ALSA"),        ("",                                                               "http://www.filter24.org/seq24/") ],

  [ "sooperlooper",  "SooperLooper",    "Audio Looper",   "slgui",            "sooperlooper",     TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",       0, 1, "JACK"),        ("",                                                               "http://essej.net/sooperlooper/download.html") ],

  [ "tutka",         "Tutka",           "MIDI Tracker",   "tutka",            "tutka",            TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 0, "",       0, 1, "JACK"),        ("",                                                               "http://www.nongnu.org/tutka/") ],
]

iDAW_Package, iDAW_AppName, iDAW_Type, iDAW_Binary, iDAW_Icon, iDAW_Template, iDAW_Level, iDAW_Features, iDAW_Docs = range(0, len(list_DAW[0]))

if USING_KXSTUDIO:
    # Jacker
    list_DAW[4][iDAW_Level] = LEVEL_1

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Host

# (I, L, D, L, V, VST-Mode, MIDI-Mode) -> (Internal, LADSPA, DSSI, LV2, VST, VST-Mode, MIDI-Mode)

list_Host = [
  # Package               AppName                 Ins?   FX?    Binary                Icon                Template?     Level      (I, L, D, L, V, VST-Mode,  MIDI-Mode)      (doc-file,                                website)
  [ "ams",                "AlsaModularSynth",     "Yes", "Yes", "ams",                "ams_32",           TEMPLATE_NO,  LEVEL_0,   (1, 1, 0, 0, 0, "",        "ALSA"),        ("",                                      "http://alsamodular.sourceforge.net/") ],

  [ "calf-plugins",       "Calf Jack Host",       "Yes", "Yes", "calfjackhost",       "calf",             TEMPLATE_YES, LEVEL_1,   (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/index.html", "http://calf.sf.net/") ],
  [ "calf-plugins-git",   "Calf Jack Host (GIT)", "Yes", "Yes", "calfjackhost",       "calf",             TEMPLATE_YES, LEVEL_1,   (1, 0, 0, 0, 0, "",        "JACK"),        ("file:///usr/share/doc/calf/index.html", "http://calf.sf.net/") ],

  [ "carla",              "Carla",                "Yes", "Yes", "carla",              "carla",            TEMPLATE_YES, LEVEL_1,   (1, 1, 1, 1, 1, "Both",    "ALSA | JACK"), ("",                                      "http://kxstudio.sf.net/Applications:Carla") ],
  [ "carla-git",          "Carla (GIT)",          "Yes", "Yes", "carla",              "carla",            TEMPLATE_YES, LEVEL_NSM, (1, 1, 1, 1, 1, "Both",    "ALSA | JACK"), ("",                                      "http://kxstudio.sf.net/Applications:Carla") ],

  [ "festige",            "FeSTige",              "Yes", "Yes", "festige",            "festige",          TEMPLATE_NO,  LEVEL_1,   (0, 0, 0, 0, 1, "Windows", "ALSA | JACK"), ("",                                      "http://festige.sf.net/") ],

  [ "ingen",              "Ingen",                "Yes", "Yes", "ingen -eg",          "ingen",            TEMPLATE_NO,  LEVEL_0,   (1, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://drobilla.net/blog/software/ingen/") ],

  [ "jack-rack",        "Jack Rack",      "No",  "Yes", "jack-rack",     "/usr/share/pixmaps/jack-rack-icon.png",   TEMPLATE_YES, LEVEL_0,   (0, 1, 0, 0, 0, "",        "ALSA"),        ("",                                      "http://jack-rack.sf.net/") ],

  [ "jalv.select",      "Jalv.select",    "No",  "No",  "jalv.select",   "/usr/share/pixmaps/lv2.png",              TEMPLATE_YES, LEVEL_0,   (0, 0, 0, 1, 0, "",        "---"),         ("",                                      "https://github.com/brummer10/jalv_select") ],

  [ "mod-app",            "MOD App",              "Yes", "Yes", "mod-app",            "mod",              TEMPLATE_NO,  LEVEL_0,   (0, 0, 0, 1, 0, "",        "JACK"),        ("",                                      "http://moddevices.com/") ],

  [ "spiralsynthmodular", "SpiralSynthModular",   "Yes", "Yes", "spiralsynthmodular", generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   (1, 0, 0, 0, 0, "",        "ALSA"),        ("",                                      "http://jack-rack.sf.net/") ],
]

iHost_Package, iHost_AppName, iHost_Ins, iHost_FX, iHost_Binary, iHost_Icon, iHost_Template, iHost_Level, iHost_Features, iDAW_Docs = range(0, len(list_Host[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Instrument

# (F, I, MIDI-Mode) -> (Built-in FX, Audio Input, MIDI-Mode)

list_Instrument = [
  # Package                 AppName              Type                Binary                    Icon                Template?     Level      (F, I, MIDI-Mode)      (doc-file,                                                            website)
  [ "6pm",                  "6 PM",              "Synth",            "6pm",                    "6PM-icon",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://sourceforge.net/projects/mv-6pm/") ],

  [ "add64",                "Add64",             "Synth",            "Add64",                  "add64",            TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                  "https://sourceforge.net/projects/add64/") ],

  [ "aeolus",               "Aeolus",            "Synth",            "aeolus -J",              "audio-x-generic",  TEMPLATE_NO,  LEVEL_0,   (0, 0,       "JACK"),        ("",                                                                  "http://www.kokkinizita.net/linuxaudio/aeolus/index.html") ],

  [ "amsynth",              "AmSynth",           "Synth",            "amsynth",                "amsynth",          TEMPLATE_NO,  LEVEL_0,   (0, 0,   "ALSA | JACK"), ("",                                                                  "http://amsynth.github.io/") ],

  [ "azr3-jack",            "AZR3",              "Synth",            "azr3",                   "/usr/share/pixmaps/azr3-jack.xpm",        TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                  "http://ll-plugins.nongnu.org/azr3/") ],

  [ "borderlands",          "Borderlands",       "Sampler",          "Borderlands",            "borderlands",      TEMPLATE_NO,  LEVEL_0,   (0, 1, "JACK"),        ("",                                                                  "https://ccrma.stanford.edu/~carlsonc/256a/Borderlands/") ],

  [ "coldgaze",             "Coldgaze",          "Synth",            "microSynth",             "coldgaze",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA"),        ("",                                                                  "https://sourceforge.net/projects/coldgaze/") ],

  [ "cursynth",             "cursynth",          "Synth",   "x-terminal-emulator -e cursynth", generic_audio_icon, TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA"),        ("",                                                                  "http://www.gnu.org/software/cursynth/") ],

  [ "distrho-lv2",          "Dexed",             "Yamaha DX7 Emulator",  "Dexed",              "dexed",            TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://distrho.sourceforge.io/ports.php") ],

  [ "din",                  "DIN",               "Musical Instrument",   "din",                "din",              TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://dinisnoise.org/") ],

  [ "drumkv1",              "Drumk-V1",          "Drum Sampler",     "drumkv1_jack",           "drumkv1",          TEMPLATE_NO,  LEVEL_JS,   (0, 0, "ALSA | JACK"),       ("",                                                                  "https://drumkv1.sourceforge.io/") ],

  [ "distrho-lv2",          "DrumSynth",         "Drum synth",       "DrumSynth",              "drumsynth",        TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://distrho.sourceforge.io/ports.php") ],

  [ "fabla",                "Fabla",             "Drum Sampler",     "fabla",                  "fabla",            TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "http://openavproductions.com/fabla/") ],

  [ "fabla2",               "Fabla 2",           "Sampler",          "fabla2",                 "fabla2",           TEMPLATE_NO,  LEVEL_0,   (0, 1, "JACK"),        ("",                                                                  "http://openavproductions.com/fabla2/") ],

  [ "foo-yc20",          "Foo YC20",      "Organ Synth",      "foo-yc20", "/usr/share/foo-yc20/graphics/icon.png", TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://foo-yc20.codeforcode.com/") ],

  [ "grandorgue",       "GrandOrgue",   "Grand Organ Sampler", "GrandOrgue",  "/usr/share/pixmaps/GrandOrgue.png", TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://sourceforge.net/p/ourorgan/wiki/Home/") ],

  [ "helm",                 "Helm",              "Synth",            "helm",                   "helm_icon_128_1x", TEMPLATE_NO,  LEVEL_0,   (1, 1, "JACK"),        ("",                                                                  "https://tytel.org/helm/") ],

  [ "hexter",               "Hexter",            "Synth",         "jack-dssi-host hexter.so",  "hexter",           TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA"),        ("",                                                                  "http://smbolton.com/hexter.html") ],

  [ "horgand",              "Horgand",           "Synth",            "horgand",                "horgand128",       TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                  "https://sourceforge.net/projects/horgand.berlios/") ],

  [ "infamous-plugins",  "Infamous Cellular Automaton Synth",  "Synth",  "infamous-casynth",  "infamous-casynth",  TEMPLATE_NO,  LEVEL_0,   (0, 1, "JACK"),        ("",                                                                   "http://ssj71.github.io/infamousPlugins/") ],

  [ "jsampler",             "JSampler Fantasia", "Sampler",          "jsampler-bin",           "jsampler",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "ALSA + JACK"),       ("file:///usr/share/kxstudio/docs/jsampler/jsampler.html",             "http://www.linuxsampler.org/") ],

  [ "distrho-lv2",          "Juce Demo Plugin",  "Synth",            "JuceDemoPlugin",         "jucedemoplugin",   TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "dpf-plugins",          "Kars",            "Virtual Instrument", "Kars",                   "kars",             TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/plugins.php") ],

  [ "dpf-plugins",          "Nekobi",  "Roland TB-303 Emulator",     "Nekobi",                 "nekobi",           TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/plugins.php") ],

  [ "distrho-lv2",          "TAL Noize Mak3r",   "Synth",            "NoizeMak3r",             "noizemak3r",       TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "distrho-lv2",          "Obxd",  "Ob-x, ob-xa & ob8 Emulators",  "Obxd",                   "obxd",             TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "padthv1",              "Padth-V1",          "Synth",            "padthv1_jack",           "padthv1",          TEMPLATE_NO,  LEVEL_NSM, (0, 0, "ALSA | JACK"),       ("",                                                                   "https://padthv1.sourceforge.io/") ],

  [ "petri-foo",            "Petri-Foo",         "Sampler",          "petri-foo",              "petri-foo",        TEMPLATE_NO,  LEVEL_NSM, (0, 0, "ALSA + JACK"),       ("",                                                                   "http://petri-foo.sf.net/") ],

  [ "phasex",               "Phasex",            "Synth",            "phasex",                 "phasex",           TEMPLATE_NO,  LEVEL_0,   (1, 1, "ALSA"),        ("file:///usr/share/phasex/help/parameters.help",                      "") ],

  [ "polyphone",            "Polyphone",         "Sampler",          "polyphone",              "polyphone",        TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                   "http://www.polyphone-soundfonts.com/") ],

  [ "qsampler",             "Qsampler",          "Sampler",          "qsampler",               "qsampler",         TEMPLATE_YES, LEVEL_0,   (0, 0, "ALSA + JACK"),       ("",                                                                   "http://qsampler.sf.net/") ],

  [ "qsynth",               "Qsynth",            "SoundFont Player", "qsynth -a jack -m jack", "qsynth",           TEMPLATE_NO,  LEVEL_0,   (1, 0, "ALSA | JACK"),       ("",                                                                   "http://qsynth.sf.net/") ],

  [ "samplv1",              "Sampl-V1",          "Sampler",          "samplv1_jack",           "samplv1",          TEMPLATE_NO,  LEVEL_NSM, (0, 0, "ALSA | JACK"),       ("",                                                                   "https://samplv1.sourceforge.io/") ],

  [ "setbfree",             "SetBfree",          "Organ Emulator",   "setBfreeUI",             "setBfree",         TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "http://setbfree.org/") ],

  [ "sorcer",               "Sorcer",            "Polyphonic Synth", "sorcer",                 "sorcer",           TEMPLATE_NO,  LEVEL_NSM, (0, 0, "JACK"),        ("",                                                                   "http://openavproductions.com/sorcer/") ],

  [ "spectmorph",           "Spectmorph",        "Morph Synth",      "smjack",                 "smjack",           TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "http://www.spectmorph.org/") ],

  [ "stegosaurus",          "Stegosaurus",    "Analog Drum Synth",   "stegosaurus",            "stegosaurus",      TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "http://www.thunderox.com/") ],

  [ "synthv1",              "Synth-V1",          "Synth",            "synthv1_jack",           "synthv1",          TEMPLATE_NO,  LEVEL_NSM, (0, 0, "ALSA | JACK"),       ("",                                                                   "https://synthv1.sourceforge.io/") ],

  [ "tapeutape",            "Tapeutape",         "Sampler",          "tapeutape",              "tapeutape",        TEMPLATE_NO,  LEVEL_0,   (0, 1, "ALSA"),        ("",                                                                   "http://hitmuri.net/index.php/Software/Tapeutape") ],

  [ "triceratops-lv2",      "Triceratops",       "Polyphonic Synth", "triceratops",            "triceratops",      TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "http://www.thunderox.com/triceratops.html") ],

  [ "vcvrack",              "VCV Rack",          "Modular Synth",    "vcvrack",                "vcvrack",          TEMPLATE_NO,  LEVEL_0,   (0, 0, "---"),         ("",                                                                   "https://vcvrack.com/") ],

  [ "distrho-lv2",          "Vex",               "Synth",            "Vex",                    "vex",              TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "whysynth",             "Whysynth",          "Synth",            "whysynth",               "whysynth",         TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                   "http://smbolton.com/whysynth.html") ],

  [ "distrho-lv2",          "Wolpertinger",   "Synth",   "Wolpertinger",   "/usr/share/pixmaps/wolpertinger.xpm",  TEMPLATE_NO,  LEVEL_0,   (0, 0, "JACK"),        ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "wsynth-dssi",          "Wsynth",          "WaveTable Synth",  "jack-dssi-host wsynth-dssi.so", "wsynth-dssi", TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                   "http://www.linuxsynths.com/WsynthBanksDemos/wsynth.html") ],

  [ "xsynth-dssi",          "Xsynth",          "Analog Synth",     "jack-dssi-host xsynth-dssi.so", "xsynth-dssi", TEMPLATE_NO,  LEVEL_0,   (1, 0, "JACK"),        ("",                                                                   "http://dssi.sourceforge.net/download.html#Xsynth-DSSI") ],

  [ "yoshimi",              "Yoshimi",    "Synth",    "yoshimi -j -J",  "/usr/share/pixmaps/yoshimi.png",          TEMPLATE_NO,  LEVEL_1,   (1, 0, "ALSA | JACK"),       ("",                                                                   "http://yoshimi.sf.net/") ],

  [ "zynaddsubfx",          "ZynAddSubFX",       "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_NSM, (1, 0, "ALSA | JACK"),       ("",                                                                   "http://zynaddsubfx.sf.net/") ],
  [ "zynaddsubfx-git",      "ZynAddSubFX (GIT)", "Synth",            "zynaddsubfx",            "zynaddsubfx",      TEMPLATE_NO,  LEVEL_NSM, (1, 0, "ALSA | JACK"),       ("",                                                                   "http://zynaddsubfx.sf.net/") ],
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
  [ "ambdec",               "AmbDec",                        "Ambisonic Decoder",   "ambdec",                                       "ambdec", TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "guitarix",             "Guitarix",                      "Guitar FX",           "guitarix",                                     "gx_head",          TEMPLATE_NO,  LEVEL_0, (0, "JACK"),        ("",                                                                   "http://guitarix.sf.net/") ],

  [ "fogpad-port",          "FogPad",                        "Reverb",              "fogpad",                                generic_audio_icon,          TEMPLATE_NO,  LEVEL_0, (0, "JACK"),        ("",                                                                   "https://github.com/linuxmao-org/fogpad-port") ],

  [ "freqtweak",            "Freqtweak",              "FFT-based realtime audio spectral manipulation",        "freqtweak",         "freqtweak_logo32x18", TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "https://freqtweak.sourceforge.io/") ],

  [ "jamin",                "Jamin",                         "Mastering",           "jamin",                                        "jamin",            TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "http://jamin.sf.net/") ],

  [ "lsp-plugins",          "LSP comp-delay-mono",           "Compressor",          "lsp-plugins-comp-delay-mono",                  generic_audio_icon, TEMPLATE_NO, LEVEL_0, (0, "---"),          ("",                                                                   "https://lsp-plug.in/") ],

  [ "lsp-plugins",          "LSP comp-delay-stereo",         "Compressor",          "lsp-plugins-comp-delay-stereo",                generic_audio_icon, TEMPLATE_NO, LEVEL_0, (1, "---"),          ("",                                                                   "https://lsp-plug.in/") ],

  [ "lsp-plugins",          "LSP comp-delay-x2-stereo",      "Compressor",          "lsp-plugins-comp-delay-x2-stereo",             generic_audio_icon, TEMPLATE_NO, LEVEL_0, (1, "---"),          ("",                                                                   "https://lsp-plug.in/") ],

  [ "lsp-plugins",          "LSP phase-detector",            "Phase Detector",      "lsp-plugins-phase-detector",                   generic_audio_icon, TEMPLATE_NO, LEVEL_0, (1, "---"),          ("",                                                                   "https://lsp-plug.in/") ],

  [ "dpf-plugins",          "MVerb",                         "Reverb Effect",       "MVerb",                                        "mverb",            TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "https://distrho.sourceforge.io/ports.php") ],

  [ "paulstretch",          "PaulStretch",                   "Extreme Stretch",     "paulstretch",                                  "paulstretch",      TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                                                                   "http://hypermammut.sourceforge.net/paulstretch/") ],

  [ "radium-compressor",    "Radium Compressor",             "Compressor",          "radium_compressor",     "/usr/share/pixmaps/radium-compressor.xpm", TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                   "http://users.notam02.no/~kjetism/radium/compressor_plugin.php") ],

  [ "rakarrack",            "Rakarrack",                     "Guitar FX",           "rakarrack",                          "icono_rakarrack_128x128",        TEMPLATE_NO,  LEVEL_0, (1, "ALSA + JACK"), ("file:///usr/share/doc/rakarrack/html/help.html",                     "http://rakarrack.sf.net") ],

  [ "tap-reverbed",         "Reverbed",                      "Reverb",              "reverbed",                                     "reverbed",        TEMPLATE_NO,  LEVEL_0, (1, "---"),         ("",                     "http://tap-plugins.sourceforge.net/reverbed.html") ],

  [ "snokoder",             "SnoKoder",                      "Vocoder",             "snokoder",                                     "snokoder",         TEMPLATE_NO,  LEVEL_0, (0, "ALSA"),         ("",                                                                  "https://www.transformate.de/x/pawfaliki.php?page=DownLoads") ],

  [ "x42-plugins",          "X42 - Darc",                    "Compressor",          "x42-darc",                                     "x42-darc",         TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "https://x42-plugins.com/x42/x42-compressor") ],

  [ "x42-plugins",          "X42 - Fil4 mono",               "4 Bands EQ",          "x42-fil4",                                     "x42-fil4",         TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "https://x42-plugins.com/x42/x42-eq") ],

  [ "x42-plugins",          "X42 - Fil4 stéréo",             "4 Bands EQ",          "x42-fil4 1",                                   "x42-fil4",         TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "https://x42-plugins.com/x42/x42-eq") ],

  [ "x42-plugins",          "X42 - Fat1",                    "Auto-tuner",          "x42-fat1",                                     "x42-fat1",         TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "https://x42-plugins.com/x42/x42-autotune") ],

  [ "setbfree",             "X42 - Whirl",                   "Leslie Emulator",     "x42-whirl",                                    "x42-whirl",        TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "https://x42-plugins.com/x42/x42-whirl") ],

  [ "zam-plugins",          "ZamAutoSat",                    "Saturator",           "ZamAutoSat",                                   "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamComp",                       "Mono Compressor",     "ZamComp",                                      "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamCompX2",                     "Stereo Compressor",   "ZamCompX2",                                    "zam",              TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamDelay",                      "Delay",               "ZamDelay",                                     "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamDynamicEQ",                  "Dynamic Mono EQ",     "ZamDynamicEQ",                                 "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamEQ2",                        "2 Bands EQ",          "ZamEQ2",                                       "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamGate",                       "Mono Gate",           "ZamGate",                                      "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamGateX2",                     "Stereo Gate",         "ZamGateX2",                                    "zam",              TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamGEQ31",                      "31 Bands EQ",         "ZamGEQ31",                                     "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamHeadX2",                     "Stereo Enhancer",     "ZamHeadX2",                                    "zam",              TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZaMaximX2",                     "Maximiser",           "ZaMaximX2",                                    "zam",              TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZaMultiComp",                   "Mo Multi-Compressor", "ZaMultiComp",                                  "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZaMultiCompX2",                 "St Multi-Compressor", "ZaMultiCompX2",                                "zam",              TEMPLATE_NO,  LEVEL_0, (1, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamPhono",                      "Phono Filters",       "ZamPhono",                                     "zam",              TEMPLATE_NO,  LEVEL_0, (0, "---"),          ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamTube",                       "Tube Amp Emulator",   "ZamTube",                                      "zam",              TEMPLATE_NO,  LEVEL_0, (0, "JACK"),         ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zam-plugins",          "ZamVerb",                       "Reverb",              "ZamVerb",                                      "zam",              TEMPLATE_NO,  LEVEL_0, (1, "JACK"),         ("",                                                                  "http://www.zamaudio.com/?p=976") ],

  [ "zita-at1",             "Zita-at1",                      "Auto-tuner",          "zita-at1",                                     "zita-at1",         TEMPLATE_NO,  LEVEL_0, (0, "JACK"),         ("",                                                                  "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "zita-rev1",            "Zita-rev1",                     "Reverb",              "zita-rev1",                                    "zita-rev1",        TEMPLATE_NO,  LEVEL_0, (1, "JACK"),         ("",                                                                  "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "zita-rev1",            "Zita-rev1",                     "Reverb - Ambisonic",  "zita-rev1 -B",                                 "zita-rev1",        TEMPLATE_NO,  LEVEL_0, (1, "JACK"),         ("",                                                                  "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

]

iEffect_Package, iEffect_AppName, iEffect_Type, iEffect_Binary, iEffect_Icon, iEffect_Template, iEffect_Level, iEffect_Features, iEffect_Docs = range(0, len(list_Effect[0]))

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Tool

# (MIDI-M, T) -> (MIDI-Mode, Transport)

list_Tool = [
  # Package              AppName                         Type                   Binary                    Icon                Template?     Level    (MIDI-M, T)  (doc,                                                         website)
  [ "aliki",             "Aliki (ALSA)",  "Impulse Response Measurements Tool", "aliki -a",               "aliki_32x32",      TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "aliki",             "Aliki (JACK)",  "Impulse Response Measurements Tool", "aliki -j",               "aliki_32x32",      TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "arpage",            "Arpage",                       "MIDI Arpeggiator",    "arpage",                 "arpage",           TEMPLATE_NO,  LEVEL_0, ("JACK", 1), ("",                                                          "") ],
  [ "arpage",            "Zonage",                       "MIDI Mapper",         "zonage",                 "zonage",           TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "") ],

  [ "audacity",          "Audacity",                     "Audio Editor",        "audacity",               "audacity",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.audacityteam.org/") ],

  [ "bitmeter",          "Bitmeter",                     "JACK Diagnotic Tool", "bitmeter",               "bitmeter",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "cadence",           "Cadence",                      "JACK Toolbox",        "cadence",                "cadence",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],
  [ "cadence-tools",     "Cadence XY-Controller",        "XY Controller",       "cadence-xycontroller",   "cadence",          TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "") ],
  [ "catia",             "Catia",                        "Patch Bay",           "catia",                  "catia",            TEMPLATE_NO,  LEVEL_0, ("JACK", 1), ("",                                                          "") ],
  [ "carla-control",     "Carla OSC Control",            "OSC Control",         "carla-control",          "carla-control",    TEMPLATE_NO,  LEVEL_0, ("JACK", 1), ("",                                                          "") ],

  [ "denemo",            "Denemo",                    "Music Notation Editor",  "denemo",                 "denemo",           TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://denemo.org/") ],

  [ "digitalscratch",   "Digital-Scratch",   "DJ with time-coded vinyls",   "digitalscratch",   "digitalscratch-icon_2decks", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://www.digital-scratch.org/") ],

  [ "drumstick-tools",   "Drumstick Virtual Piano",      "Virtual Keyboard",    "drumstick-vpiano",       "drumstick",        TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "https://drumstick.sourceforge.io/") ],

  [ "easytag",           "EasyTAG",      "Audio File Metadata Management",      "easytag",                "easytag",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://wiki.gnome.org/Apps/EasyTAG") ],

  [ "etktab",            "eTktab",                    "Music Notation Editor",  "eTktab",                 "eTktab",           TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://etktab.sourceforge.net/") ],

  [ "ebumeter",          "Ebumeter",             "EBU-r128 Loudness Indicator", "ebumeter",               "ebumeter32x32",    TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "http://kokkinizita.linuxaudio.org/linuxaudio/") ],

  [ "alsa-tools-gui",    "Echomixer",                    "Echoaudio Mixer",     "echomixer",              "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "") ],

  [ "elektroid",         "Elektroid",           "Soft for Elektron Devices",    "elektroid",              "elektroid",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "https://github.com/dagargo/elektroid/") ],

  [ "alsa-tools-gui",    "Envy24Control",                "Ice1712 Mixer",       "envy24control",          "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "https://alsa.opensrc.org/Envy24control") ],

  [ "audio-recorder",    "Audio-recorder",               "Audio Recorder",      "audio-recorder",         "audio-recorder",   TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "https://launchpad.net/audio-recorder") ],

  [ "ffado-mixer-qt4",   "FFADO-mixer",      "FireWire Audio Interfaces Mixer", "ffado-mixer",            "hi64-apps-ffado",  TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                         "http://ffado.org/") ],

  [ "fmit",              "Music Instrument Tuner",       "Instrument Tuner",    "fmit",                   "fmit",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://gillesdegottex.github.io/fmit/") ],

  [ "frescobaldi",       "Frescobaldi",         "Music Notation Editor",   "frescobaldi",   "org.frescobaldi.Frescobaldi",    TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                               "https://www.frescobaldi.org/") ],

  [ "gdigi",             "Gdigi",                  "Digitech Pedals Control",   "gdigi",                  "gdigi",            TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://desowin.org/gdigi/") ],

  [ "gigedit",           "Gigedit",                      "Instrument Editor",   "gigedit",                generic_audio_icon, TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("file:///usr/share/doc/gigedit/gigedit_quickstart.html",     "") ],

  [ "gjacktransport",    "GJackClock",                   "Transport Tool",      "gjackclock",             "gjackclock",       TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                          "http://gjacktransport.sourceforge.net/") ],
  [ "gjacktransport",    "GJackTransport",               "Transport Tool",      "gjacktransport",         "gjacktransport",   TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                          "http://gjacktransport.sourceforge.net/") ],

  [ "gladish",           "Gladish",                 "LADISH Studio Manager",    "gladish",                "gladish",          TEMPLATE_NO,  LEVEL_0, ("JACK | ALSA", 0), ("",                                                   "http://ladish.org/") ],

  [ "dpf-plugins",       "glBars",                       "Audio Visualizer",    "glBars",                 "glbars",           TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "https://distrho.sourceforge.io/plugins.php") ],

  [ "gmidimonitor",      "Gmidimonitor (ALSA)",          "MIDI-ALSA Monitor",   "gmidimonitor --alsa",  "gmidimonitor_32x32", TEMPLATE_NO,  LEVEL_0, ("JACK | ALSA", 0), ("",                                                   "") ],
  [ "gmidimonitor",      "Gmidimonitor (JACK)",          "MIDI-JACK Monitor",   "gmidimonitor --jack",  "gmidimonitor_32x32", TEMPLATE_NO,  LEVEL_0, ("JACK | ALSA", 0), ("",                                                   "") ],

  [ "gwc",               "Gtk Wave Cleaner",             "Audio File Cleaner",  "gtk-wave-cleaner",       "gtk-wave-cleaner", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://sourceforge.net/projects/gwc/") ],

  [ "gninjam",           "Gtk NINJAM client",            "Music Collaboration", "gninjam",                "gninjam",          TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                          "") ],

  [ "gtklick",           "Gtklick",                      "Metronome",           "gtklick",                "gtklick",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://das.nasophon.de/gtklick/") ],

  [ "gxtuner",           "Gxtuner",                      "Instrument Tuner",    "gxtuner",                "gxtuner",          TEMPLATE_NO,  LEVEL_JS, ("---", 0),  ("",                                                          "") ],

  [ "handbrake",    "Handbrake",    "DVD, Bluray, and other Medias Transcoder", "ghb",                    "hb-icon",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://handbrake.fr/") ],

  [ "alsa-tools-gui",    "HdaJackRetask",                "Intel HDA Control",   "hdajackretask",          "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],
  [ "alsa-tools-gui",    "HDSPconf",                     "HDSP Control",        "hdspconf",               "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],
  [ "alsa-tools-gui",    "HDSPmixer",                    "HDSP Mixer",          "hdspmixer",              "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "jaaa",              "Jaaa-ALSA",    "Analyzes/Generates an Audio Signal",  "jaaa -A",                "jaaa",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],
  [ "jaaa",              "Jaaa-JACK",    "Analyzes/Generates an Audio Signal",  "jaaa -J",                "jaaa",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "jack-capture",      "Jack-Capture-GUI",     "JACK-Audio Capture GUI",      "jack_capture_gui",      "jack_capture_gui",  TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/kmatheussen/jack_capture") ],

  [ "jackeq",            "JackEQ",                  "DJ Console GUI",      "jackeq",  "/usr/share/pixmaps/logo-jackeq-s.png", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://djcj.org/jackeq/") ],

  [ "jack-keyboard",     "Jack Keyboard",                "Virtual Keyboard",    "jack-keyboard",          "jack-keyboard",    TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("file:///usr/share/kxstudio/docs/jack-keyboard/manual.html", "http://jack-keyboard.sf.net/") ],

  [ "jack-mixer",        "Jack Mixer",                   "Mixer",               "jack_mixer",             "jack_mixer",       TEMPLATE_NO,  LEVEL_NSM, ("JACK", 0), ("",                                                          "http://home.gna.org/jackmixer/") ],

  [ "jamulus",           "Jamulus",                 "Music Collaboration",      "jamulus",                "jamulus",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://llcon.sourceforge.net/") ],

  [ "japa",              "Japa-ALSA",               "Audio Signal Analysis",    "japa -A",                "japa",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],
  [ "japa",              "Japa-JACK",               "Audio Signal Analysis",    "japa -J",                "japa",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "jkmeter",           "Jkmeter",                  "Audio Level Measurement", "jkmeter",                "jkmeter32x32",     TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "jmeters",           "Jmeters",    "Multi Canal Audio Level Measurement",   "jmeters",                "jmeters_32x32",    TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

#  [ "jnoise",            "Jnoise",            "White & Pink Noise Generation", "jnoise",                 generic_audio_icon, TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "jnoisemeter",       "Jnoisemeter",              "Test Audio Signal Meter", "jnoisemeter",           "jnoisemeter_32x32", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://librazik.tuxfamily.org/doc2/logiciels/jnoisemeter") ],

  [ "kmetronome",        "KMetronome",                   "Metronome",           "kmetronome",             "kmetronome",       TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "http://kmetronome.sf.net/kmetronome.shtml") ],
  [ "kmidimon",          "KMidimon",                     "Monitor",             "kmidimon",               "kmidimon",         TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "http://kmidimon.sf.net/") ],

  [ "laditools",         "LADI Log",                     "Log Viewer",          "ladi-system-log",        "ladi-system-log",  TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],
  [ "laditools",         "LADI Tray",                    "Session Handler",     "ladi-system-tray",       "laditools",        TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "lenmus-all",      "Lenmus",                "Music Theory Studying",        "lenmus",                 "lenmus",           TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://www.lenmus.org/en/noticias") ],

  [ "lingot",            "Lingot",                       "Instrument Tuner",    "lingot",              "org.nongnu.lingot",   TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://www.nongnu.org/lingot/") ],

  [ "linux-show-player", "Linux Show Player",            "Sample player",       "linux-show-player",      "linuxshowplayer",  TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "https://www.linux-show-player.org/") ],

  [ "lives",             "LiVES",                        "VJ / Video Editor",   "lives",                  "lives",            TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                          "http://lives.sf.net/") ],

  [ "luppp",             "Luppp",                        "Audio Looper",        "luppp",                  "luppp",            TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://openavproductions.com/luppp/") ],

  [ "mamba",             "Mamba",                        "Virtual Keyboard",    "mamba",                  "Mamba",            TEMPLATE_NO,  LEVEL_NSM, ("ALSA", 0), ("",                                                          "https://github.com/brummer10/Mamba") ],

  [ "mcpdisp",         "MCP Disp",           "Mackie Display Emulator",         "mcpdisp",                "mcpdisp",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/ovenwerks/mcpdisp") ],

  [ "meterbridge",       "MeterBridge Classic VU",       "VU / Peak Analyzer", "meterbridge -t vu :",     "meterbridge32x32", TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",       "MeterBridge PPM Meter",        "VU / Peak Analyzer", "meterbridge -t ppm :",    "meterbridge32x32", TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge", "MeterBridge Digital Peak Meter", "VU / Peak Analyzer", "meterbridge -t dpm -c 2 : :", "meterbridge32x32", TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge", "MeterBridge 'Jellyfish' Phase Meter", "VU / Peak Analyzer", "meterbridge -t jf -c 2 : :", "meterbridge32x32", TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/meterbridge/") ],
  [ "meterbridge",     "MeterBridge Oscilloscope Meter", "VU / Peak Analyzer",  "meterbridge -t sco :",   "meterbridge32x32", TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/meterbridge/") ],

  [ "mhwaveedit",        "MhWaveEdit",                   "Audio Editor",        "mhwaveedit",             "mhwaveedit",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://gna.org/projects/mhwaveedit/") ],

  [ "mixxx",             "Mixxx",                        "DJ",                  "mixxx",                  "mixxx_icon",       TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "http://mixxx.sf.net/") ],

  [ "mpk-m2-editor",     "MPK M2 Editor",                "Editor for MPK M2",   "mpk_m2-editor",          "mpk-m2-editor",    TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/PiOverFour/MPK-M2-editor") ],

  [ "mudita24",          "Mudita24",                     "Ice1712 Mixer",       "mudita24",               "mudita24",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/NielsMayer/mudita24") ],

  [ "nano-basket",       "Nano-Basket",           "Nano Hardware Management",   "nano-basket",            "nano-basket",      TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/royvegard/Nano-Basket") ],

  [ "non-mixer",         "Non-Mixer",                    "Mixer",               "non-mixer",              "non-mixer",        TEMPLATE_NO,  LEVEL_0, ("CV",   0), ("file:///usr/share/doc/non-mixer/MANUAL.html",               "http://non.tuxfamily.org/wiki/Non%20Mixer") ],

  [ "nootka",            "Nootka",     "Learn music notation on sheet music",   "nootka",                 "nootka",           TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://nootka.sourceforge.io/") ],

  [ "patchage",          "Patchage",                     "Patch Bay",           "patchage",               "patchage",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 0), ("",                                                   "http://drobilla.net/blog/software/patchage/") ],

  [ "pd-l2ork", "Pd-L2Ork", "Pure-Data Environnement", "pd-l2ork -rt -audiobuf 20 -inchannels 2 -outchannels 2 -alsamidi -mididev 0 %U", "pd-l2ork", TEMPLATE_NO,  LEVEL_0, ("ALSA", 0),        ("",                            "http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/") ],

  [ "pianobooster",      "Piano Booster",                "Piano Teacher",       "pianobooster",           "pianobooster",     TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "http://pianobooster.sourceforge.net/") ],

  [ "playitslowly",      "Play it Slowly",               "Strech-Player",       "playitslowly",       "ch.x29a.playitslowly", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://29a.ch/playitslowly/") ],

  [ "dpf-plugins",       "ProM",                         "Music Visualizer",    "ProM",                   "prom",             TEMPLATE_NO,  LEVEL_0, ("JACK", 0),  ("",                                                         "https://distrho.sourceforge.io/plugins.php") ],

  [ "qamix",             "QAMix",                        "Mixer",               "qamix",                  "qamix",            TEMPLATE_NO,  LEVEL_0, ("ALSA", 0),  ("",                                                         "") ],
  [ "qarecord",          "QARecord",                     "Recorder",            "qarecord --jack",        "qarecord_48",      TEMPLATE_NO,  LEVEL_0, ("ALSA", 0),  ("",                                                         "") ],

  [ "qjackctl",          "QJackControl",                 "JACK Control",        "qjackctl",               "qjackctl",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 1), ("",                                                   "https://qjackctl.sourceforge.io/") ],

  [ "qlcplus",           "QLCplus",                    "Light-Show Controller", "qlcplus",                "qlcplus",          TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "https://www.qlcplus.org/") ],

  [ "qlcplus",     "Fixture editor for QLC+",  "Fixture editor for QLC+",  "qlcplus-fixtureeditor",  "qlcplus-fixtureeditor", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.qlcplus.org/") ],

  [ "qloud",             "Qloud",       "Audio measurement tool for JACK",      "qloud",                  "qloud",            TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://gaydenko.com/qloud/") ],

  [ "qmidiarp",          "QMidiArp",                     "MIDI Arpeggiator",    "qmidiarp",               "qmidiarp",         TEMPLATE_NO,  LEVEL_0, ("ALSA + JACK", 0), ("",                                                          "http://qmidiarp.sourceforge.net/") ],

  [ "qmidiroute",    "QMidiRoute",   "MIDI router & event processor",    "qmidiroute", "/usr/share/pixmaps/qmidiroute_32x32.xpm", TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "") ],

  [ "qmidictl",          "QmidiCtl",     "MIDI controller over the network",    "qmidictl",               "qmidictl",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://qmidictl.sourceforge.io/") ],

  [ "qmidinet",        "QmidiNet",                "MIDI Network Gateway",       "qmidinet",               "qmidinet",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://qmidinet.sourceforge.io/") ],

  [ "qrest",           "Qrest",              "Calculator for delay, LFO,...",   "qrest",                  "qrest",            TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "qxgedit",         "QXGedit",                      "XG HardWare Controler", "qxgedit",                "qxgedit",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://qxgedit.sourceforge.io/") ],

  [ "raysession",      "Ray Session",                    "NSM Session Manager", "raysession",             "raysession",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://github.com/Houston4444/RaySession") ],

  [ "recjack",         "RecJack",                        "Recorder",            "recjack",                "recjack",          TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                  "http://mein-neues-blog.de/2015/02/07/mein-neues-blog-deb-repository/#recjack") ],

  [ "rezound",         "ReZound",                        "Audio Editor",        "rezound",                "rezound32",        TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://rezound.sourceforge.net/") ],

  [ "alsa-tools-gui",  "RME Digi Control",               "RME Digi Mixer",      "rmedigicontrol",         "alsa-tools",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "showq",           "ShowQ",                          "Sampler Player",      "showq",                  "showq_32x32",      TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "") ],

  [ "shuriken",        "Shuriken",                       "Beat Slicer",         "shuriken",               "shuriken",         TEMPLATE_NO,  LEVEL_1, ("JACK+ALSA", 1), ("",                                                     "https://rock-hopper.github.io/shuriken/") ],

  [ "simplescreenrecorder", "Simple Screen Recorder",  "ScreenCast Recorder", "simplescreenrecorder", "simplescreenrecorder", TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.maartenbaert.be/simplescreenrecorder/") ],

  [ "smplayer",        "Smplayer",                       "Multimédia Player",   "smplayer",               "smplayer",         TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.smplayer.info/") ],

  [ "solfege",         "Solfege",                        "Ear Training",        "solfege",                "solfege",          TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.gnu.org/software/solfege/") ],

  [ "songwrite",       "Songwrite 3",   "Guitar tablature reader and editor",   "songwrite",              "songwrite3",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                  "http://www.lesfleursdunormal.fr/static/informatique/songwrite/index_en.html") ],

  [ "sonic-visualiser", "Sonic Visualiser",              "Signal Visualization and Analysis", "sonic-visualiser",  "sv-icon", TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "https://sonicvisualiser.org/") ],

  [ "spek",            "Spek",                           "Spectrum Analyzer",   "spek",                   "spek",             TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://spek.cc/") ],

  [ "stretchplayer",   "Stretch Player",                 "Stretch Player",      "stretchplayer",          "stretchplayer",    TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://www.teuton.org/~gabriel/stretchplayer/") ],

  [ "swami",           "Swami",                     "SF2 instruments editor",   "swami",                  "swami",            TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "http://www.swamiproject.org/") ],

  [ "tinyeartrainer",  "Tiny Ear Trainer",               "Ear Training",        "tinyeartrainer",         "tinyeartrainer",   TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://29a.ch/tinyeartrainer/") ],

  [ "timemachine",   "TimeMachine",   "Recorder",   "timemachine",   "/usr/share/timemachine/pixmaps/timemachine-icon.png",   TEMPLATE_NO, LEVEL_0, ("---", 0), ("",                                                           "http://plugin.org.uk/timemachine/") ],

  [ "tmlauncher",      "TM Launcher",                    "Recorder",            "TMLauncher.py",          "TMLauncher",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "") ],

  [ "tuxguitar",       "TuxGuitar",                      "Guitar Score",        "tuxguitar",              "tuxguitar",        TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "http://www.tuxguitar.com.ar/") ],

  [ "vkeybd",          "Virtual Keyboard",               "Virtual Keyboard",    "vkeybd",                 "vkeybd",           TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("",                                                          "https://github.com/tiwai/vkeybd") ],

  [ "vmpk",            "Virtual MIDI Piano Keyboard (ALSA)","Virtual Keyboard", "vmpk",                   "vmpk",             TEMPLATE_NO,  LEVEL_0, ("ALSA", 0), ("file:///usr/share/doc/vmpk/help.html",                      "http://vmpk.sf.net/") ],
  [ "vmpk-jack",       "Virtual MIDI Piano Keyboard (JACK)","Virtual Keyboard", "vmpk-jack",              "vmpk",             TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("file:///usr/share/doc/vmpk/help.html",                      "http://vmpk.sf.net/") ],

  [ "x42-plugins",     "X42 - StepSeq",           "Step By Step Sequencer",     "x42-stepseq",            "x42-stepseq",      TEMPLATE_NO,  LEVEL_0, ("JACK", 0), ("",                                                          "https://x42-plugins.com/x42/x42-stepseq-8x8") ],

  [ "x42-plugins",     "X42-Meter - EBU R128",    "Meter",                      "x42-meter",              "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - K20",         "Meter",                      "x42-meter 1",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - K14",         "Meter",                      "x42-meter 2",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - K12",         "Meter",                      "x42-meter 3",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0),  ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - BBC",         "Meter",                      "x42-meter 4",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - BBC M-6",     "Meter",                      "x42-meter 5",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - DIN",         "Meter",                      "x42-meter 6",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - EBU",         "Meter",                      "x42-meter 7",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Nordic",      "Meter",                      "x42-meter 8",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - VU",          "Meter",                      "x42-meter 9",            "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - True Peak et RMS",            "Meter",      "x42-meter 10",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Dynamic Range",               "Meter",      "x42-meter 11",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Phase Correlation ",          "Meter",      "x42-meter 12",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Goniometer",                  "Meter",      "x42-meter 13",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Phase Frequency Wheel",       "Meter",      "x42-meter 14",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - 1/3 Octave Spectrum",         "Meter",      "x42-meter 15",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Frequency Scope",             "Meter",      "x42-meter 16",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Distribution Histogram",      "Meter",      "x42-meter 17",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Bitmeter",                    "Meter",      "x42-meter 18",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42-Meter - Surround Analysor",           "Meter",      "x42-meter 19",           "x42-meters",       TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",                                                          "https://x42-plugins.com/x42/x42-meters") ],

  [ "x42-plugins",     "X42 - Mixtri",            "Mixer/Trigger Preprocessor", "x42-mixtri",             "x42-mixtri",       TEMPLATE_NO,  LEVEL_0, ("---", 0),        ("", "https://x42-plugins.com/x42/x42-mixtrix") ],

  [ "x42-plugins",     "X42 - Oscilloscope",      "Oscilloscope",               "x42-scope",              "x42-scope",        TEMPLATE_NO,  LEVEL_0, ("---", 0),        ("", "https://x42-plugins.com/x42/x42-scope") ],

  [ "x42-plugins",     "X42 - Tuna",              "Instrument Tuner",           "x42-tuna",               "x42-tuna",         TEMPLATE_NO,  LEVEL_0, ("---", 0),        ("", "https://x42-plugins.com/x42/x42-tuner") ],

  [ "x42-plugins",     "X42 - Tuna - spectral",  "Instrument Tuner - spectrum", "x42-tuna 1",             "x42-tuna",         TEMPLATE_NO,  LEVEL_0, ("---", 0),        ("", "https://x42-plugins.com/x42/x42-tuner") ],

  [ "xjadeo",          "XJadeo",                 "Video Player",                "qjadeo",                 "qjadeo",           TEMPLATE_NO,  LEVEL_0, ("---", 1),  ("",                                                          "http://xjadeo.sf.net/") ],

  [ "zita-bls1",       "Zita-bls1",    "Binaural -> stereo signal converter",   "zita-bls1",              "zita-bls1",        TEMPLATE_NO,  LEVEL_0, ("---", 0), ("",  "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

  [ "zita-mu1",        "Zita-mu1",        "Stereo Monitoring Organizer",        "zita-mu1",               "zita-mu1",         TEMPLATE_NO,  LEVEL_0, ("---", 0),       ("",  "http://kokkinizita.linuxaudio.org/linuxaudio/index.html") ],

]

iTool_Package, iTool_AppName, iTool_Type, iTool_Binary, iTool_Icon, iTool_Template, iTool_Level, iTool_Features, iTool_Docs = range(0, len(list_Tool[0]))

if USING_KXSTUDIO:
    # Non-Mixer
    list_Tool[10][iDAW_Level] = LEVEL_1

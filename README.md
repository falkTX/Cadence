=========================
=  README for Cadence  =
=======================

Cadence is a set of tools useful for audio production.
It's being developed by falkTX, using Python and Qt (and some C++ where needed).

The tools share the same base source code, so most of them look quite similar (which is intentional).

The basic modules are:
  cadence_logs -> Show JACK, A2J, LASH and LADISH logs (just like the 'ladilog' app)
  cadence_render -> Record (or 'render') a JACK project using jack-capture, controlled by JACK Transport; supports realtime and freewheel modes
  cadence_settings -> Configure JACK (requires jackdbus, either JACK2 or JACK1 + DBus patch)

Also, lots of custom widgets were coded when needed (like pixmapdial, pixmapkeyboard, patchcanvas and systray).
They can be re-used in other projects. Contact the author if you need help on that.

Note that the main focus goes to JACK2 (or JACK1+DBus); JACK1 (raw) and tschack are untested and may not work properly at this point.


DESCRIPTIONS
------------------------
Here's a brief description of the main tools:

---> Cadence
The main app. It performs system checks, manages JACK, calls other tools and make system tweaks.
Currently under development, may change at anytime.

---> Catarina
A Patchbay test app, created while the patchcanvas module was being developed.
It allows the user to experiment with the patchbay, without using ALSA, JACK or LADISH.
You can save & load patchbay configurations too.

---> Catia
JACK Patchbay, with some neat features like A2J bridge support and JACK Transport.
It's supposed to be as simple as possible (there's Claudia for advanced things), so it can work nicely on Windows and Mac too.
Currently has ALSA-MIDI support in experimental stage (it doesn't automatically refresh the canvas when changes happen externally).

---> Claudia
LADISH frontend; just like Catia, but focused at session management through LADISH.
It has a bit more features than the official LADISH GUI, with a pretty preview of the main canvas in the bottom-left.
It also implements the 'Claudia-Launcher' add-application style.

---> Claudia
A multimedia application launcher with LADISH support.
It searches for installed packages (not binaries), and displays the respective content as a launcher.
The content is got through an hardcoded database, created and/or modified to suit the target distribution.
Currently supports Debian and ArchLinux based distros.

---> Carla
Multi-plugin host for JACK.
It has some nice features like automation of parameters via MIDI CCs (and send control outputs back to MIDI too) and full OSC control.
Currently supports LADSPA (including LRDF), DSSI, LV2, and VST plugin formats, with additional GIG, SF2 and SFZ file support via FluidSynth and LinuxSampler.
This application is still under development and may change/break at anytime.

---> Carla Control
An OSC Control GUI for Carla (you get the OSC address from the Carla's about dialog, and connect to it).
Supports controlling main UI components (Dry/Wet, Vol and Balance), and all plugins parameters.
Peak values and control outputs are displayed as well.
Custom Plugin GUIs are not supported at this point.
NOTE: This application is not yet ported to this python3 branch

---> JACK-Meter
Simple JACK audio peak meter.
In the future it will be available in a plugin version, based on the DISTRHO project.

---> XY-Controller
Simple XY widget that sends and receives data from Jack MIDI.
It can send data through specific channels and has a MIDI Keyboard too.
This is more of a concept of my jacklib python module usage then anything else, may not work properly under very low JACK buffer sizes.
In the future it will be available in a plugin version, based on the DISTRHO project.

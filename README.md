# ---  README for Cadence  ---

Cadence is a set of tools useful for audio production.
It's being developed by falkTX, using Python and Qt (and some C++ where needed).

The tools share the same base source code, so most of them look quite similar (which is intentional).

The basic modules are:

-   cadence_logs -> Show JACK, A2J, LASH and LADISH logs (just like the 'ladilog' app).
-   cadence_render -> Record (or 'render') a JACK project using jack-capture, controlled by JACK Transport; supports realtime and freewheel modes.
-   cadence_jacksettings -> Configure JACK (requires jackdbus, either JACK2 or JACK1 + DBus patch).

Also, lots of custom widgets were coded when needed (like pixmapdial, pixmapkeyboard, patchcanvas and systray).<br/>
They can be re-used in other projects. Contact the author if you need help on that.

Note that the main focus goes to JACK2 (or JACK1+DBus); JACK1 (raw) and tschack are untested and may not work properly at this point.

<br/>

===== DESCRIPTIONS =====
------------------------
Here's a brief description of the main tools:
(Click on the tool name to open its respective documentation)

### ![alt text](./Cadence/blob/master/src/icons/48x48/cadence.png) [Cadence](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence)
The main app. It performs system checks, manages JACK, calls other tools and make system tweaks.<br/>
Currently under development, may change at anytime.

### ![alt text](./Cadence/blob/master/src/icons/48x48/catarina.png) [Catarina](http://kxstudio.sourceforge.net/KXStudio:Applications:Catarina)
A Patchbay test app, created while the patchcanvas module was being developed.<br/>
It allows the user to experiment with the patchbay, without using ALSA, JACK or LADISH.<br/>
You can save & load patchbay configurations too.

### ![alt text](./Cadence/blob/master/src/icons/48x48/catia.png) [Catia](http://kxstudio.sourceforge.net/KXStudio:Applications:Catia)
JACK Patchbay, with some neat features like A2J bridge support and JACK Transport.<br/>
It's supposed to be as simple as possible (there's Claudia for advanced things), so it can work nicely on Windows and Mac too.<br/>
Currently has ALSA-MIDI support in experimental stage (it doesn't automatically refresh the canvas when changes happen externally).

### ![alt text](./Cadence/blob/master/src/icons/48x48/claudia.png) [Claudia](http://kxstudio.sourceforge.net/KXStudio:Applications:Claudia)
LADISH frontend; just like Catia, but focused at session management through LADISH.<br/>
It has a bit more features than the official LADISH GUI, with a pretty preview of the main canvas in the bottom-left.<br/>
It also implements the 'Claudia-Launcher' add-application style.

### ![alt text](./Cadence/blob/master/src/icons/48x48/claudia-launcher.png) [Claudia-Launcher](http://kxstudio.sourceforge.net/KXStudio:Applications:Claudia-Launcher)
A multimedia application launcher with LADISH support.<br/>
It searches for installed packages (not binaries), and displays the respective content as a launcher.<br/>
The content is got through an hardcoded database, created and/or modified to suit the target distribution.<br/>
Currently supports Debian and ArchLinux based distros.

### ![alt text](./Cadence/blob/master/src/icons/48x48/carla.png) [Carla](http://kxstudio.sourceforge.net/KXStudio:Applications:Carla)
Multi-plugin host for JACK.<br/>
It has some nice features like automation of parameters via MIDI CCs (and send control outputs back to MIDI too) and full OSC control.<br/>
Currently supports LADSPA (including LRDF), DSSI, LV2, and VST plugin formats, with additional GIG, SF2 and SFZ file support via FluidSynth and LinuxSampler.<br/>
This application is still under development and may change/break at anytime.

### ![alt text](./Cadence/blob/master/src/icons/48x48/carla-control.png) [Carla-Control](http://kxstudio.sourceforge.net/KXStudio:Applications:Carla-Control)
An OSC Control GUI for Carla (you get the OSC address from the Carla's about dialog, and connect to it).<br/>
Supports controlling main UI components (Dry/Wet, Vol and Balance), and all plugins parameters.<br/>
Peak values and control outputs are displayed as well.<br/>
Custom Plugin GUIs are not supported at this point.<br/>
NOTE: This application is not yet ported to this python3 branch

### [JACK-Meter](http://kxstudio.sourceforge.net/KXStudio:Applications:JACK-Meter)
Simple JACK audio peak meter.<br/>
In the future it will be available in a plugin version, based on the DISTRHO project.

### [XY-Controller](http://kxstudio.sourceforge.net/KXStudio:Applications:XY-Controller)
Simple XY widget that sends and receives data from Jack MIDI.<br/>
It can send data through specific channels and has a MIDI Keyboard too.<br/>
This is more of a concept of my jacklib python module usage then anything else, may not work properly under very low JACK buffer sizes.<br/>
In the future it will be available in a plugin version, based on the DISTRHO project.

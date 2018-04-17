# ---  README for Cadence  ---

Cadence is a set of tools useful for audio production. <br/>
It's being developed by falkTX, using Python3 and Qt5 (and some C++ where needed).

The tools share the same base source code, so most of them look quite similar (which is intentional). <br/>
Also, lots of custom widgets were coded when needed (like pixmapdial, pixmapkeyboard, patchcanvas and systray). <br/>
They can be re-used in other projects. Contact the author if you need help on that.


===== DESCRIPTIONS =====
------------------------
Here's a brief description of the main tools:

### [Cadence](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence)
The main app. It performs system checks, manages JACK, calls other tools and make system tweaks.

### [Cadence-JackMeter](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence-JackMeter)
Digital peak meter for JACK. <br/>
It automatically connects itself to all application JACK output ports that are also connected to the system output.

### [Cadence-JackSettings](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence-JackSettings)
Simple and easy-to-use configure dialog for jackdbus. <br/>
It can configure JACK's driver and engine parameters, and it also supports LADISH studios.

### [Cadence-Logs](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence-Logs)
Small tool that shows JACK, A2J, LASH and LADISH logs in a multi-tab window. <br/>
The logs are viewed in a text box, making it easy to browse and extract status messages using copy and paste commands.

### [Cadence-Render](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence-Render)
Tool to record (or 'render') a JACK project using jack-capture, controlled by JACK Transport. <br/>
It supports a vast number of file types and can render in both realtime and freewheel modes.

### [Cadence-XY Controller](http://kxstudio.sourceforge.net/KXStudio:Applications:Cadence-XYController)
Simple XY widget that sends and receives data from Jack MIDI. <br/>
It can send data through specific channels and has a MIDI Keyboard too.

### [Catarina](http://kxstudio.sourceforge.net/KXStudio:Applications:Catarina)
A Patchbay test app, created while the patchcanvas module was being developed. <br/>
It allows the user to experiment with the patchbay, without using ALSA, JACK or LADISH. <br/>
You can save & load patchbay configurations too.

### [Catia](http://kxstudio.sourceforge.net/KXStudio:Applications:Catia)
JACK Patchbay, with some neat features like A2J bridge support and JACK Transport. <br/>
It's supposed to be as simple as possible (there's Claudia for advanced things), so it can work nicely on Windows and Mac too. <br/>
Currently has ALSA-MIDI support in experimental stage (it doesn't automatically refresh the canvas when changes happen externally).

### [Claudia](http://kxstudio.sourceforge.net/KXStudio:Applications:Claudia)
LADISH frontend; just like Catia, but focused at session management through LADISH. <br/>
It has a bit more features than the official LADISH GUI, with a nice preview of the main canvas in the bottom-left. <br/>
It also implements the 'Claudia-Launcher' add-application style for LADISH.

### [Claudia-Launcher](http://kxstudio.sourceforge.net/KXStudio:Applications:Claudia-Launcher)
A multimedia application launcher with LADISH support. <br/>
It searches for installed packages (not binaries), and displays the respective content as a launcher. <br/>
The content is got through an hardcoded database, created and/or modified to suit the target distribution. <br/>
Currently supports Debian and ArchLinux based distros.

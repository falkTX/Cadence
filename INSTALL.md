# ---  INSTALL for Cadence  ---

To install Cadence and its tools, simply run as usual: <br/>
`$ make` <br/>
`$ [sudo] make install`

You can also run the tools without installing them, by using instead: <br/>
<i>(Replace '&lt;tool&gt;' by a proper name, in lowercase. Some distros may need to use 'python' here)</i> <br/>
`$ make` <br/>
`$ python3 src/<tool>.py`

Packagers can make use of the 'PREFIX' and 'DESTDIR' variable during install, like this: <br/>
`$ make install PREFIX=/usr DESTDIR=./test-dir`

<br/>

===== BUILD DEPENDENCIES =====
--------------------------------
The required build dependencies are: <i>(devel packages of these)</i>

 - JACK
 - liblo
 - Gtk2
 - Qt4
 - PyQt4

Optional but recommended:

 - FluidSynth
 - LinuxSampler

On Debian and Ubuntu, use this command to install all dependencies: <br/>
`$ sudo apt-get install libjack-dev liblo-dev libgtk2.0-dev libqt4-dev libfluidsynth-dev` <br/>
`$ sudo apt-get install qt4-dev-tools python-qt4-dev pyqt4-dev-tools`

NOTE: linuxsampler is not packaged in either Debian or Ubuntu, but it's available in KXStudio. <br/>
<br/>

To run all the apps/tools, you'll additionally need:

 - python3-dbus
 - python3-liblo

Optional but recommended:

 - a2jmidid
 - jack-capture
 - python3-rdflib

The 'Cadence' and 'Catia' apps rely on a jackdbus version to work properly (either JACK2 or JACK1 + DBus patch). <br/>
Claudia is a LADISH frontend, so it will obviously require LADISH to run.

The python version used is python3. <br/>
After install, the app/tools will still work on distros with python2 as default, without any additional work.

<br/>

===== RUNTIME DEPENDENCIES =====
----------------------------------
All tools require Python3 and Qt4, some of them work on Windows and Mac. <br/>
Here's the required run-time dependencies of each of the main tools:

### Cadence
Recommends a2jmidid and jackdbus <br/>
Suggests pulseaudio <br/>
<br/>

### Catarina
No special requirements <br/>
<br/>

### Catia
Recommends a2jmidid and jackdbus <br/>
<br/>

### Claudia [Linux only] <br/>
Requires jackdbus and ladish <br/>
Recommends a2jmidid <br/>
<br/>

### Carla
Requires liblo and Gtk2 <br/>
Recommends python3-rdflib (for LADSPA RDF support) <br/>
<br/>

### Carla-Control
Requires python3-liblo <br/>
<br/>

### JACK-Meter
No special requirements <br/>
<br/>

### XY-Controller
No special requirements <br/>
<br/>

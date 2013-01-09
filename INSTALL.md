# ---  INSTALL for Cadence  ---

To install Cadence and its tools, simply run as usual: <br/>
`$ make` <br/>
`$ [sudo] make install`

You can run the tools without installing them, by using instead: <br/>
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
 - Qt4
 - PyQt4 (Py3 version)

On Debian and Ubuntu, use these commands to install all build dependencies: <br/>
`$ sudo apt-get install libjack-dev libqt4-dev qt4-dev-tools` <br/>
`$ sudo apt-get install python-qt4-dev python3-pyqt4 pyqt4-dev-tools`

To run all the apps/tools, you'll additionally need:

 - python3-dbus
 - python3-dbus.mainloop.qt

Optional but recommended:

 - a2jmidid
 - jack-capture
 - pulseaudio[-module-jack]

The 'Cadence' app relies on jackdbus to work properly (either JACK2 or JACK1+DBus patch). <br/>
Claudia is a LADISH frontend, so it will obviously require LADISH to run.

The python version used and tested is python3.2. Older versions won't work! <br/>
After install, the app/tools will still work on distributions with python2 as default, without any additional work.

<br/>

===== RUNTIME DEPENDENCIES =====
----------------------------------
All tools require Python3 and Qt4, some of them work on Windows and Mac. <br/>
Here's the required run-time dependencies of each of the main tools:

### Cadence
Recommends a2jmidid (>= 7) and jackdbus <br/>
Suggests pulseaudio[-module-jack] <br/>

### Cadence-JackMeter
Requires jack <br/>

### Cadence-JackSettings
Requires jackdbus <br/>

### Cadence-Logs
No special requirements <br/>

### Cadence-Render
Requires jack-capture <br/>

### Cadence-XY Controller
Requires jack <br/>

### Catarina
No special requirements <br/>

### Catia
Recommends a2jmidid (>= 7) and jackdbus <br/>

### Claudia [Linux only] <br/>
Requires jackdbus and ladish <br/>
Recommends a2jmidid <br/>

<br/>

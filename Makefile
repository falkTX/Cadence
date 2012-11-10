#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PREFIX  = /usr/local
DESTDIR =

SED_PREFIX = $(shell echo $(PREFIX) | sed "s/\//\\\\\\\\\//g")

PYUIC = pyuic4
PYRCC = pyrcc4 -py3

# Detect X11 rules dir
ifeq "$(wildcard /etc/X11/xinit/xinitrc.d/ )" ""
	X11_RC_DIR = $(DESTDIR)/etc/X11/Xsession.d/
else
	X11_RC_DIR = $(DESTDIR)/etc/X11/xinit/xinitrc.d/
endif


all: UI RES CPP

# ------------------------------------------------------------------------------------------------------------------------------------------------------

UI: cadence catarina catia claudia carla tools

cadence: src/ui_cadence.py \
	src/ui_cadence_tb_jack.py src/ui_cadence_tb_alsa.py src/ui_cadence_tb_a2j.py src/ui_cadence_tb_pa.py \
	src/ui_cadence_rwait.py

catarina: src/ui_catarina.py \
	src/ui_catarina_addgroup.py src/ui_catarina_removegroup.py src/ui_catarina_renamegroup.py \
	src/ui_catarina_addport.py src/ui_catarina_removeport.py src/ui_catarina_renameport.py \
	src/ui_catarina_connectports.py src/ui_catarina_disconnectports.py

catia: src/ui_catia.py

claudia: src/ui_claudia.py \
	src/ui_claudia_studioname.py src/ui_claudia_studiolist.py \
	src/ui_claudia_createroom.py src/ui_claudia_projectname.py src/ui_claudia_projectproperties.py \
	src/ui_claudia_runcustom.py src/ui_claudia_launcher.py src/ui_claudia_launcher_app.py

carla: src/ui_carla.py src/ui_carla_control.py \
	src/ui_carla_about.py src/ui_carla_database.py src/ui_carla_edit.py src/ui_carla_parameter.py src/ui_carla_plugin.py src/ui_carla_refresh.py \
	src/ui_inputdialog_value.py

tools: \
	src/ui_logs.py src/ui_render.py \
	src/ui_settings_app.py src/ui_settings_jack.py

src/ui_cadence.py: resources/ui/cadence.ui
	$(PYUIC) $< -o $@

src/ui_cadence_tb_jack.py: resources/ui/cadence_tb_jack.ui
	$(PYUIC) $< -o $@

src/ui_cadence_tb_alsa.py: resources/ui/cadence_tb_alsa.ui
	$(PYUIC) $< -o $@

src/ui_cadence_tb_a2j.py: resources/ui/cadence_tb_a2j.ui
	$(PYUIC) $< -o $@

src/ui_cadence_tb_pa.py: resources/ui/cadence_tb_pa.ui
	$(PYUIC) $< -o $@

src/ui_cadence_rwait.py: resources/ui/cadence_rwait.ui
	$(PYUIC) $< -o $@

src/ui_catarina.py: resources/ui/catarina.ui
	$(PYUIC) $< -o $@

src/ui_catarina_addgroup.py: resources/ui/catarina_addgroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_removegroup.py: resources/ui/catarina_removegroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_renamegroup.py: resources/ui/catarina_renamegroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_addport.py: resources/ui/catarina_addport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_removeport.py: resources/ui/catarina_removeport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_renameport.py: resources/ui/catarina_renameport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_connectports.py: resources/ui/catarina_connectports.ui
	$(PYUIC) $< -o $@

src/ui_catarina_disconnectports.py: resources/ui/catarina_disconnectports.ui
	$(PYUIC) $< -o $@

src/ui_catia.py: resources/ui/catia.ui
	$(PYUIC) $< -o $@

src/ui_claudia.py: resources/ui/claudia.ui
	$(PYUIC) $< -o $@

src/ui_claudia_studioname.py: resources/ui/claudia_studioname.ui
	$(PYUIC) $< -o $@

src/ui_claudia_studiolist.py: resources/ui/claudia_studiolist.ui
	$(PYUIC) $< -o $@

src/ui_claudia_createroom.py: resources/ui/claudia_createroom.ui
	$(PYUIC) $< -o $@

src/ui_claudia_projectname.py: resources/ui/claudia_projectname.ui
	$(PYUIC) $< -o $@

src/ui_claudia_projectproperties.py: resources/ui/claudia_projectproperties.ui
	$(PYUIC) $< -o $@

src/ui_claudia_runcustom.py: resources/ui/claudia_runcustom.ui
	$(PYUIC) $< -o $@

src/ui_claudia_launcher.py: resources/ui/claudia_launcher.ui
	$(PYUIC) $< -o $@

src/ui_claudia_launcher_app.py: resources/ui/claudia_launcher_app.ui
	$(PYUIC) $< -o $@

src/ui_carla.py: resources/ui/carla.ui
	$(PYUIC) $< -o $@

src/ui_carla_control.py: resources/ui/carla_control.ui
	$(PYUIC) $< -o $@

src/ui_carla_about.py: resources/ui/carla_about.ui
	$(PYUIC) $< -o $@

src/ui_carla_database.py: resources/ui/carla_database.ui
	$(PYUIC) $< -o $@

src/ui_carla_edit.py: resources/ui/carla_edit.ui
	$(PYUIC) $< -o $@

src/ui_carla_parameter.py: resources/ui/carla_parameter.ui
	$(PYUIC) $< -o $@

src/ui_carla_plugin.py: resources/ui/carla_plugin.ui
	$(PYUIC) $< -o $@

src/ui_carla_refresh.py: resources/ui/carla_refresh.ui
	$(PYUIC) $< -o $@

src/ui_logs.py: resources/ui/logs.ui
	$(PYUIC) $< -o $@

src/ui_render.py: resources/ui/render.ui
	$(PYUIC) $< -o $@

src/ui_settings_app.py: resources/ui/settings_app.ui
	$(PYUIC) $< -o $@

src/ui_settings_jack.py: resources/ui/settings_jack.ui
	$(PYUIC) $< -o $@

src/ui_inputdialog_value.py: resources/ui/inputdialog_value.ui
	$(PYUIC) $< -o $@

# ------------------------------------------------------------------------------------------------------------------------------------------------------

RES: src/resources_rc.py

src/resources_rc.py: resources/resources.qrc
	$(PYRCC) $< -o $@

# ------------------------------------------------------------------------------------------------------------------------------------------------------

CPP: carla-backend carla-bridge carla-discovery jackmeter xycontroller

carla-backend: carla-engine carla-native carla-plugin
	$(MAKE) -C c++/carla-backend

carla-bridge:
	$(MAKE) -C c++/carla-bridge

carla-discovery:
	$(MAKE) -C c++/carla-discovery NATIVE=1

carla-engine:
	$(MAKE) -C c++/carla-engine

carla-lilv:
	$(MAKE) -C c++/carla-lilv

carla-native:
	$(MAKE) -C c++/carla-native

carla-plugin:
	$(MAKE) -C c++/carla-plugin

carla-rtmempool:
	$(MAKE) -C c++/carla-rtmempool

jackmeter:
	$(MAKE) -C c++/jackmeter

xycontroller:
	$(MAKE) -C c++/xycontroller

# ------------------------------------------------------------------------------------------------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true

doxygen:
	$(MAKE) doxygen -C c++/carla-backend
	$(MAKE) doxygen -C c++/carla-bridge
	$(MAKE) doxygen -C c++/carla-engine
	$(MAKE) doxygen -C c++/carla-native
	$(MAKE) doxygen -C c++/carla-plugin

# ------------------------------------------------------------------------------------------------------------------------------------------------------

posix32:
	$(MAKE) -C c++/carla-bridge posix32
	$(MAKE) -C c++/carla-discovery posix32

posix64:
	$(MAKE) -C c++/carla-bridge posix64
	$(MAKE) -C c++/carla-discovery posix64

win32:
	$(MAKE) -C c++/carla-bridge win32
	$(MAKE) -C c++/carla-discovery win32

win64:
	$(MAKE) -C c++/carla-bridge win64
	$(MAKE) -C c++/carla-discovery win64

wine32:
	$(MAKE) -C c++/carla-jackbridge wine32
	cp c++/carla-jackbridge/libcarla-jackbridge-win32.dll.so c++/carla-bridge/libcarla-jackbridge-win32.dll

wine64:
	$(MAKE) -C c++/carla-jackbridge wine64
	cp c++/carla-jackbridge/libcarla-jackbridge-win64.dll.so c++/carla-bridge/libcarla-jackbridge-win64.dll

# ------------------------------------------------------------------------------------------------------------------------------------------------------

clean:
	$(MAKE) clean -C c++/carla-backend
	$(MAKE) clean -C c++/carla-bridge
	$(MAKE) clean -C c++/carla-discovery
	$(MAKE) clean -C c++/carla-engine
	$(MAKE) clean -C c++/carla-jackbridge
	$(MAKE) clean -C c++/carla-lilv
	$(MAKE) clean -C c++/carla-native
	$(MAKE) clean -C c++/carla-plugin
	$(MAKE) clean -C c++/carla-rtmempool
	$(MAKE) clean -C c++/jackmeter
	$(MAKE) clean -C c++/xycontroller
	rm -f *~ src/*~ src/*.pyc src/ui_*.py src/resources_rc.py
	rm -rf c++/*/doxygen

install:
	# Create directories
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -d $(DESTDIR)$(PREFIX)/share/cadence/
	install -d $(DESTDIR)$(PREFIX)/share/cadence/src/
	install -d $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
	install -d $(DESTDIR)$(PREFIX)/share/cadence/icons/
	install -d $(DESTDIR)$(PREFIX)/share/cadence/templates/
	install -d $(X11_RC_DIR)

	# Install script files and binaries
	install -m 755 \
		data/cadence \
		data/cadence-aloop-daemon \
		data/cadence-jacksettings \
		data/cadence-logs \
		data/cadence-pulse2jack \
		data/cadence-render \
		data/cadence-session-start \
		data/catarina \
		data/catia \
		data/claudia \
		data/claudia-launcher \
		data/carla \
		data/carla-control \
		c++/jackmeter/cadence-jackmeter \
		c++/xycontroller/cadence-xycontroller \
		$(DESTDIR)$(PREFIX)/bin/

	# Install desktop files
	install -m 644 data/*.desktop $(DESTDIR)$(PREFIX)/share/applications/

	# Install icons, 16x16
	install -m 644 resources/16x16/cadence.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/catarina.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/catia.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/claudia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/claudia-launcher.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/carla.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/carla-control.png       $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/

	# Install icons, 48x48
	install -m 644 resources/48x48/cadence.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/catarina.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/catia.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/claudia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/claudia-launcher.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/carla.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/carla-control.png       $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/

	# Install icons, 128x128
	install -m 644 resources/128x128/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/carla.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/carla-control.png     $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/

	# Install icons, 256x256
	install -m 644 resources/256x256/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/carla.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/carla-control.png     $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/

	# Install icons, scalable
	install -m 644 resources/scalable/cadence.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/catarina.svg         $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/catia.svg            $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/claudia.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/claudia-launcher.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/carla.svg            $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/carla-control.svg    $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/

	# Install main code
	install -m 755 src/*.py $(DESTDIR)$(PREFIX)/share/cadence/src/

	install -m 755 \
		c++/carla-backend/*.so \
		c++/carla-bridge/carla-bridge-* \
		c++/carla-discovery/carla-discovery-* \
		$(DESTDIR)$(PREFIX)/lib/carla/

	# Install addtional stuff for Cadence
	install -m 644 data/pulse2jack/* $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
	install -m 644 data/99cadence-session-start $(X11_RC_DIR)

	# Install addtional stuff for Claudia
	cp -r data/icons/*     $(DESTDIR)$(PREFIX)/share/cadence/icons/
	cp -r data/templates/* $(DESTDIR)$(PREFIX)/share/cadence/templates/

	# Adjust PREFIX value in script files
	sed -i "s/X-PREFIX-X/$(SED_PREFIX)/" \
		$(DESTDIR)$(PREFIX)/bin/cadence \
		$(DESTDIR)$(PREFIX)/bin/cadence-aloop-daemon \
		$(DESTDIR)$(PREFIX)/bin/cadence-jacksettings \
		$(DESTDIR)$(PREFIX)/bin/cadence-logs \
		$(DESTDIR)$(PREFIX)/bin/cadence-pulse2jack \
		$(DESTDIR)$(PREFIX)/bin/cadence-render \
		$(DESTDIR)$(PREFIX)/bin/cadence-session-start \
		$(DESTDIR)$(PREFIX)/bin/catarina \
		$(DESTDIR)$(PREFIX)/bin/catia \
		$(DESTDIR)$(PREFIX)/bin/claudia \
		$(DESTDIR)$(PREFIX)/bin/claudia-launcher \
		$(DESTDIR)$(PREFIX)/bin/carla \
		$(DESTDIR)$(PREFIX)/bin/carla-control \
		$(X11_RC_DIR)/99cadence-session-start

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cadence*
	rm -f $(DESTDIR)$(PREFIX)/bin/catarina
	rm -f $(DESTDIR)$(PREFIX)/bin/catia
	rm -f $(DESTDIR)$(PREFIX)/bin/claudia*
	rm -f $(DESTDIR)$(PREFIX)/bin/carla*
	rm -f $(DESTDIR)$(PREFIX)/share/applications/cadence.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catarina.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia-launcher.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla-control.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla-control.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/cadence.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catarina.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia-launcher.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla-control.svg
	rm -f $(X11_RC_DIR)/99cadence-session-start
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/share/cadence/

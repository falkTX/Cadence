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


all: UI RES CPP

# ------------------------------------------------------------------------------------------------------------------------------------------------------

UI: cadence catarina catia claudia carla tools

cadence: src/ui_cadence.py

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

src/ui_cadence.py: src/ui/cadence.ui
	$(PYUIC) $< -o $@

src/ui_catarina.py: src/ui/catarina.ui
	$(PYUIC) $< -o $@

src/ui_catarina_addgroup.py: src/ui/catarina_addgroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_removegroup.py: src/ui/catarina_removegroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_renamegroup.py: src/ui/catarina_renamegroup.ui
	$(PYUIC) $< -o $@

src/ui_catarina_addport.py: src/ui/catarina_addport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_removeport.py: src/ui/catarina_removeport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_renameport.py: src/ui/catarina_renameport.ui
	$(PYUIC) $< -o $@

src/ui_catarina_connectports.py: src/ui/catarina_connectports.ui
	$(PYUIC) $< -o $@

src/ui_catarina_disconnectports.py: src/ui/catarina_disconnectports.ui
	$(PYUIC) $< -o $@

src/ui_catia.py: src/ui/catia.ui
	$(PYUIC) $< -o $@

src/ui_claudia.py: src/ui/claudia.ui
	$(PYUIC) $< -o $@

src/ui_claudia_studioname.py: src/ui/claudia_studioname.ui
	$(PYUIC) $< -o $@

src/ui_claudia_studiolist.py: src/ui/claudia_studiolist.ui
	$(PYUIC) $< -o $@

src/ui_claudia_createroom.py: src/ui/claudia_createroom.ui
	$(PYUIC) $< -o $@

src/ui_claudia_projectname.py: src/ui/claudia_projectname.ui
	$(PYUIC) $< -o $@

src/ui_claudia_projectproperties.py: src/ui/claudia_projectproperties.ui
	$(PYUIC) $< -o $@

src/ui_claudia_runcustom.py: src/ui/claudia_runcustom.ui
	$(PYUIC) $< -o $@

src/ui_claudia_launcher.py: src/ui/claudia_launcher.ui
	$(PYUIC) $< -o $@

src/ui_claudia_launcher_app.py: src/ui/claudia_launcher_app.ui
	$(PYUIC) $< -o $@

src/ui_carla.py: src/ui/carla.ui
	$(PYUIC) $< -o $@

src/ui_carla_control.py: src/ui/carla_control.ui
	$(PYUIC) $< -o $@

src/ui_carla_about.py: src/ui/carla_about.ui
	$(PYUIC) $< -o $@

src/ui_carla_database.py: src/ui/carla_database.ui
	$(PYUIC) $< -o $@

src/ui_carla_edit.py: src/ui/carla_edit.ui
	$(PYUIC) $< -o $@

src/ui_carla_parameter.py: src/ui/carla_parameter.ui
	$(PYUIC) $< -o $@

src/ui_carla_plugin.py: src/ui/carla_plugin.ui
	$(PYUIC) $< -o $@

src/ui_carla_refresh.py: src/ui/carla_refresh.ui
	$(PYUIC) $< -o $@

src/ui_logs.py: src/ui/logs.ui
	$(PYUIC) $< -o $@

src/ui_render.py: src/ui/render.ui
	$(PYUIC) $< -o $@

src/ui_settings_app.py: src/ui/settings_app.ui
	$(PYUIC) $< -o $@

src/ui_settings_jack.py: src/ui/settings_jack.ui
	$(PYUIC) $< -o $@

src/ui_inputdialog_value.py: src/ui/inputdialog_value.ui
	$(PYUIC) $< -o $@

# ------------------------------------------------------------------------------------------------------------------------------------------------------

RES: src/resources_rc.py

src/resources_rc.py: resources/resources.qrc
	$(PYRCC) $< -o $@

# ------------------------------------------------------------------------------------------------------------------------------------------------------

CPP: carla-backend carla-bridge carla-discovery carla-lilv jackmeter xycontroller

carla-backend: carla-lilv
	$(MAKE) -C c++/carla-backend

carla-bridge: carla-lilv
	$(MAKE) -C c++/carla-bridge

carla-discovery:
	$(MAKE) -C c++/carla-discovery NATIVE=1

carla-lilv:
	$(MAKE) -C c++/carla-lilv

jackmeter:
	$(MAKE) -C c++/jackmeter

xycontroller:
	$(MAKE) -C c++/xycontroller

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
	$(MAKE) clean -C c++/carla-lilv
	$(MAKE) clean -C c++/carla-jackbridge
	$(MAKE) clean -C c++/jackmeter
	$(MAKE) clean -C c++/xycontroller
	rm -f *~ src/*~ src/*.pyc src/ui_*.py src/resources_rc.py

doc:
	$(MAKE) doc -C c++/carla-backend
	$(MAKE) doc -C c++/carla-bridge

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

	# Install script files and binaries
	install -m 755 \
		data/cadence \
		data/cadence_jacksettings \
		data/cadence_logs \
		data/cadence_pulse2jack \
		data/cadence_render \
		data/catarina \
		data/catia \
		data/claudia \
		data/claudia-launcher \
		data/carla \
		data/carla-control \
		c++/carla-bridge/carla-bridge-* \
		c++/carla-discovery/carla-discovery-* \
		c++/jackmeter/cadence_jackmeter \
		c++/xycontroller/cadence_xycontroller \
		src/cadence-aloop-daemon \
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
	install -m 755 c++/carla-backend/*.so $(DESTDIR)$(PREFIX)/lib/carla/

	# Install addtional stuff
	install -m 644 data/pulse2jack/* $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
	cp -r data/icons/*     $(DESTDIR)$(PREFIX)/share/cadence/icons/
	cp -r data/templates/* $(DESTDIR)$(PREFIX)/share/cadence/templates/

	# Adjust PREFIX value in script files
	sed -i "s/X-PREFIX-X/$(SED_PREFIX)/" \
		$(DESTDIR)$(PREFIX)/bin/cadence \
		$(DESTDIR)$(PREFIX)/bin/cadence_jacksettings \
		$(DESTDIR)$(PREFIX)/bin/cadence_logs \
		$(DESTDIR)$(PREFIX)/bin/cadence_pulse2jack \
		$(DESTDIR)$(PREFIX)/bin/cadence_render \
		$(DESTDIR)$(PREFIX)/bin/catarina \
		$(DESTDIR)$(PREFIX)/bin/catia \
		$(DESTDIR)$(PREFIX)/bin/claudia \
		$(DESTDIR)$(PREFIX)/bin/claudia-launcher \
		$(DESTDIR)$(PREFIX)/bin/carla \
		$(DESTDIR)$(PREFIX)/bin/carla-control

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
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/share/cadence/

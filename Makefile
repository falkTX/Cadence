#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PREFIX = /usr/local
DESTDIR =

SED_PREFIX = $(shell echo $(PREFIX) | sed "s/\//\\\\\\\\\//g")

PYUIC = pyuic4
PYRCC = pyrcc4 -py3

# Detect architecture
ifndef _arch_n
  ARCH = $(shell uname -m)
  ifeq ("$(ARCH)", "x86_64")
    _arch_n = 64
  else
    _arch_n = 32
  endif
endif


all: UI RES CPP


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
	src/ui_logs.py src/ui_render.py src/ui_xycontroller.py \
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

src/ui_xycontroller.py: src/ui/xycontroller.ui
	$(PYUIC) $< -o $@

src/ui_settings_app.py: src/ui/settings_app.ui
	$(PYUIC) $< -o $@

src/ui_settings_jack.py: src/ui/settings_jack.ui
	$(PYUIC) $< -o $@

src/ui_inputdialog_value.py: src/ui/inputdialog_value.ui
	$(PYUIC) $< -o $@


RES: src/icons_rc.py

src/icons_rc.py: src/icons/icons.qrc
	$(PYRCC) $< -o $@


CPP: carla_backend carla_bridge carla_discovery

carla_backend: carla_lilv
	$(MAKE) -C c++/carla-backend

carla_bridge: carla_lilv
	$(MAKE) -C c++/carla-bridge

carla_discovery:
	$(MAKE) -C c++/carla-discovery unix$(_arch_n) NATIVE=1

carla_lilv:
	$(MAKE) -C c++/carla-lilv

unix32:
	$(MAKE) -C c++/carla-bridge unix32
	$(MAKE) -C c++/carla-discovery unix32

unix64:
	$(MAKE) -C c++/carla-bridge unix64
	$(MAKE) -C c++/carla-discovery unix64

wine32:
	$(MAKE) -C c++/carla-bridge wine32
	$(MAKE) -C c++/carla-discovery wine32

wine64:
	$(MAKE) -C c++/carla-bridge wine64
	$(MAKE) -C c++/carla-discovery wine64


clean:
	$(MAKE) clean -C c++/carla-backend
	$(MAKE) clean -C c++/carla-bridge
	$(MAKE) clean -C c++/carla-discovery
	$(MAKE) clean -C c++/carla-lilv
	rm -f *~ src/*~ src/*.pyc src/*.dll src/*.so src/ui_*.py src/icons_rc.py


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
		data/cadence_* \
		data/cadence-aloop-daemon \
		data/catarina \
		data/catia \
		data/claudia \
		data/claudia-launcher \
		data/carla \
		c++/carla-bridge/carla-bridge-lv2-gtk2 \
		c++/carla-bridge/carla-bridge-lv2-qt4 \
		c++/carla-bridge/carla-bridge-lv2-x11 \
		c++/carla-bridge/carla-bridge-vst-x11 \
		c++/carla-discovery/carla-discovery-* \
		$(DESTDIR)$(PREFIX)/bin/

	# Install desktop files
	install -m 644 data/*.desktop $(DESTDIR)$(PREFIX)/share/applications/

	# Install icons, 16x16
	install -m 644 src/icons/16x16/cadence.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/catarina.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/catia.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/claudia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/claudia-launcher.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/carla.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/

	# Install icons, 48x48
	install -m 644 src/icons/48x48/cadence.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/catarina.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/catia.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/claudia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/claudia-launcher.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/carla.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/

	# Install icons, 128x128
	install -m 644 src/icons/128x128/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/carla.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/

	# Install icons, 256x256
	install -m 644 src/icons/256x256/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/carla.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/

	# Install icons, scalable
	install -m 644 src/icons/scalable/cadence.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/scalable/catarina.svg         $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/scalable/catia.svg            $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/scalable/claudia.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/scalable/claudia-launcher.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/scalable/carla.svg            $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/

	# Install main code
	install -m 755 src/*.py $(DESTDIR)$(PREFIX)/share/cadence/src/
	install -m 755 c++/carla-backend/*.so $(DESTDIR)$(PREFIX)/lib/carla/

	# Install addtional stuff
	install -m 644 data/pulse2jack-data/* $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
	cp -r data/icons/*     $(DESTDIR)$(PREFIX)/share/cadence/icons/
	cp -r data/templates/* $(DESTDIR)$(PREFIX)/share/cadence/templates/

	# Adjust PREFIX value in script files
	sed -i "s/X-PREFIX-X/$(SED_PREFIX)/" \
		$(DESTDIR)$(PREFIX)/bin/cadence \
		$(DESTDIR)$(PREFIX)/bin/catarina \
		$(DESTDIR)$(PREFIX)/bin/catia \
		$(DESTDIR)$(PREFIX)/bin/claudia \
		$(DESTDIR)$(PREFIX)/bin/claudia-launcher \
		$(DESTDIR)$(PREFIX)/bin/carla \
		$(DESTDIR)$(PREFIX)/bin/cadence_*

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cadence
	rm -f $(DESTDIR)$(PREFIX)/bin/cadence_*
	rm -f $(DESTDIR)$(PREFIX)/bin/cadence-aloop-daemon
	rm -f $(DESTDIR)$(PREFIX)/bin/catarina
	rm -f $(DESTDIR)$(PREFIX)/bin/catia
	rm -f $(DESTDIR)$(PREFIX)/bin/claudia
	rm -f $(DESTDIR)$(PREFIX)/bin/claudia-launcher
	rm -f $(DESTDIR)$(PREFIX)/bin/carla
	rm -f $(DESTDIR)$(PREFIX)/bin/carla-bridge-*
	rm -f $(DESTDIR)$(PREFIX)/bin/carla-discovery-*
	rm -f $(DESTDIR)$(PREFIX)/share/applications/cadence.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catarina.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia-launcher.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/cadence.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catarina.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia-launcher.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla.svg
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/share/cadence/

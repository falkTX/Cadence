#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PREFIX  = /usr/local
DESTDIR =

SED_PREFIX = $(shell echo $(PREFIX) | sed "s/\//\\\\\\\\\//g")

LINK   = ln -s
PYUIC ?= pyuic4
PYRCC ?= pyrcc4 -py3

# Detect X11 rules dir
ifeq "$(wildcard /etc/X11/xinit/xinitrc.d/ )" ""
	X11_RC_DIR = $(DESTDIR)/etc/X11/Xsession.d/
else
	X11_RC_DIR = $(DESTDIR)/etc/X11/xinit/xinitrc.d/
endif

# -----------------------------------------------------------------------------------------------------------------------------------------

all: CPP RES UI

# -----------------------------------------------------------------------------------------------------------------------------------------
# C++ code

CPP: jackmeter xycontroller

jackmeter:
	$(MAKE) -C c++/jackmeter

xycontroller:
	$(MAKE) -C c++/xycontroller

# -----------------------------------------------------------------------------------------------------------------------------------------
# Resources

RES: src/resources_rc.py

src/resources_rc.py: resources/resources.qrc
	$(PYRCC) $< -o $@

# -----------------------------------------------------------------------------------------------------------------------------------------
# UI code

UI: cadence catarina catia claudia tools

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

tools: \
	src/ui_logs.py src/ui_render.py \
	src/ui_settings_app.py src/ui_settings_jack.py

src/ui_%.py: resources/ui/%.ui
	$(PYUIC) $< -o $@

# -----------------------------------------------------------------------------------------------------------------------------------------

clean:
	$(MAKE) clean -C c++/jackmeter
	$(MAKE) clean -C c++/xycontroller
	rm -f *~ src/*~ src/*.pyc src/ui_*.py src/resources_rc.py

# -----------------------------------------------------------------------------------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true

# -----------------------------------------------------------------------------------------------------------------------------------------

install:
	# Create directories
	install -d $(DESTDIR)$(PREFIX)/bin/
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

	# Install icons, 48x48
	install -m 644 resources/48x48/cadence.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/catarina.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/catia.png               $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/claudia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/claudia-launcher.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/

	# Install icons, 128x128
	install -m 644 resources/128x128/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/

	# Install icons, 256x256
	install -m 644 resources/256x256/cadence.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/catarina.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/catia.png             $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/claudia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/claudia-launcher.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/

	# Install icons, scalable
	install -m 644 resources/scalable/cadence.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/catarina.svg         $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/catia.svg            $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/claudia.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/claudia-launcher.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/

	# Install main code
	install -m 755 src/*.py $(DESTDIR)$(PREFIX)/share/cadence/src/

	# Install addtional stuff for Cadence
	install -m 644 data/pulse2jack/* $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
	install -m 644 data/70cadence-plugin-paths  $(X11_RC_DIR)
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
		$(X11_RC_DIR)/70cadence-plugin-paths \
		$(X11_RC_DIR)/99cadence-session-start

# -----------------------------------------------------------------------------------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cadence*
	rm -f $(DESTDIR)$(PREFIX)/bin/catarina
	rm -f $(DESTDIR)$(PREFIX)/bin/catia
	rm -f $(DESTDIR)$(PREFIX)/bin/claudia*
	rm -f $(DESTDIR)$(PREFIX)/share/applications/cadence.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catarina.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia-launcher.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/cadence.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/claudia-launcher.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/cadence.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catarina.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia-launcher.svg
	rm -f $(X11_RC_DIR)/70cadence-plugin-paths
	rm -f $(X11_RC_DIR)/99cadence-session-start
	rm -rf $(DESTDIR)$(PREFIX)/share/cadence/

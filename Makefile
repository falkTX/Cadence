#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PYUIC = pyuic4 --pyqt3-wrapper
PYRCC = pyrcc4 -py3


all: UI RES


UI: catarina catia claudia carla tools \
	carla_backend

catarina: src/ui_catarina.py \
	src/ui_catarina_addgroup.py src/ui_catarina_removegroup.py src/ui_catarina_renamegroup.py \
	src/ui_catarina_addport.py src/ui_catarina_removeport.py src/ui_catarina_renameport.py \
	src/ui_catarina_connectports.py src/ui_catarina_disconnectports.py

catia: src/ui_catia.py

claudia: src/ui_claudia.py \
	src/ui_claudia_studioname.py src/ui_claudia_studiolist.py \
	src/ui_claudia_createroom.py src/ui_claudia_projectname.py src/ui_claudia_projectproperties.py \
	src/ui_claudia_runcustom.py
# 	src/ui_claudia_addnew.py src/ui_claudia_addnew_klaudia.py

carla: src/ui_carla.py src/ui_carla_control.py \
	src/ui_carla_about.py src/ui_carla_database.py src/ui_carla_edit.py src/ui_carla_parameter.py src/ui_carla_plugin.py src/ui_carla_refresh.py \
	src/ui_inputdialog_value.py

tools: \
	src/ui_logs.py src/ui_render.py src/ui_xycontroller.py \
	src/ui_settings_app.py src/ui_settings_jack.py

carla_backend:
	# Build static lilv first
	$(MAKE) -C src/carla-lilv

	$(MAKE) -C src/carla
	$(MAKE) -C src/carla-bridge-ui

src/ui_catarina.py: src/ui/catarina.ui
	$(PYUIC) -o src/ui_catarina.py $<

src/ui_catarina_addgroup.py: src/ui/catarina_addgroup.ui
	$(PYUIC) -o src/ui_catarina_addgroup.py $<

src/ui_catarina_removegroup.py: src/ui/catarina_removegroup.ui
	$(PYUIC) -o src/ui_catarina_removegroup.py $<

src/ui_catarina_renamegroup.py: src/ui/catarina_renamegroup.ui
	$(PYUIC) -o src/ui_catarina_renamegroup.py $<

src/ui_catarina_addport.py: src/ui/catarina_addport.ui
	$(PYUIC) -o src/ui_catarina_addport.py $<

src/ui_catarina_removeport.py: src/ui/catarina_removeport.ui
	$(PYUIC) -o src/ui_catarina_removeport.py $<

src/ui_catarina_renameport.py: src/ui/catarina_renameport.ui
	$(PYUIC) -o src/ui_catarina_renameport.py $<

src/ui_catarina_connectports.py: src/ui/catarina_connectports.ui
	$(PYUIC) -o src/ui_catarina_connectports.py $<

src/ui_catarina_disconnectports.py: src/ui/catarina_disconnectports.ui
	$(PYUIC) -o src/ui_catarina_disconnectports.py $<

src/ui_catia.py: src/ui/catia.ui
	$(PYUIC) -o src/ui_catia.py $<

src/ui_claudia.py: src/ui/claudia.ui
	$(PYUIC) -o src/ui_claudia.py $<

src/ui_claudia_studioname.py: src/ui/claudia_studioname.ui
	$(PYUIC) -o src/ui_claudia_studioname.py $<

src/ui_claudia_studiolist.py: src/ui/claudia_studiolist.ui
	$(PYUIC) -o src/ui_claudia_studiolist.py $<

src/ui_claudia_createroom.py: src/ui/claudia_createroom.ui
	$(PYUIC) -o src/ui_claudia_createroom.py $<

src/ui_claudia_projectname.py: src/ui/claudia_projectname.ui
	$(PYUIC) -o src/ui_claudia_projectname.py $<

src/ui_claudia_projectproperties.py: src/ui/claudia_projectproperties.ui
	$(PYUIC) -o src/ui_claudia_projectproperties.py $<

src/ui_claudia_runcustom.py: src/ui/claudia_runcustom.ui
	$(PYUIC) -o src/ui_claudia_runcustom.py $<

src/ui_carla.py: src/ui/carla.ui
	$(PYUIC) -o src/ui_carla.py $<

src/ui_carla_control.py: src/ui/carla_control.ui
	$(PYUIC) -o src/ui_carla_control.py $<

src/ui_carla_about.py: src/ui/carla_about.ui
	$(PYUIC) -o src/ui_carla_about.py $<

src/ui_carla_database.py: src/ui/carla_database.ui
	$(PYUIC) -o src/ui_carla_database.py $<

src/ui_carla_edit.py: src/ui/carla_edit.ui
	$(PYUIC) -o src/ui_carla_edit.py $<

src/ui_carla_parameter.py: src/ui/carla_parameter.ui
	$(PYUIC) -o src/ui_carla_parameter.py $<

src/ui_carla_plugin.py: src/ui/carla_plugin.ui
	$(PYUIC) -o src/ui_carla_plugin.py $<

src/ui_carla_refresh.py: src/ui/carla_refresh.ui
	$(PYUIC) -o src/ui_carla_refresh.py $<

src/ui_logs.py: src/ui/logs.ui
	$(PYUIC) -o src/ui_logs.py $<

src/ui_render.py: src/ui/render.ui
	$(PYUIC) -o src/ui_render.py $<

src/ui_xycontroller.py: src/ui/xycontroller.ui
	$(PYUIC) -o src/ui_xycontroller.py $<

src/ui_settings_app.py: src/ui/settings_app.ui
	$(PYUIC) -o src/ui_settings_app.py $<

src/ui_settings_jack.py: src/ui/settings_jack.ui
	$(PYUIC) -o src/ui_settings_jack.py $<

src/ui_inputdialog_value.py: src/ui/inputdialog_value.ui
	$(PYUIC) -o src/ui_inputdialog_value.py $<


RES: src/icons_rc.py

src/icons_rc.py: src/icons/icons.qrc
	$(PYRCC) -o src/icons_rc.py $<


clean:
	$(MAKE) clean -C src/carla
	$(MAKE) clean -C src/carla-bridge
	$(MAKE) clean -C src/carla-bridge-ui
	$(MAKE) clean -C src/carla-discovery
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

# 	# Install script files and binaries
# 	install -m 755 \
# 		data/catarina \
# 		data/catia \
# 		data/claudia \
# 		data/carla \
# 		data/cadence_* \
# 		data/pulse2jack \
# 		src/carla-bridges/carla-bridge-lv2-gtk2 \
# 		src/carla-bridges/carla-bridge-lv2-qt4 \
# 		src/carla-bridges/carla-bridge-lv2-x11 \
# 		$(DESTDIR)$(PREFIX)/bin/
# 
# 	# Install desktop files
# 	install -m 644 data/desktop/*.desktop $(DESTDIR)$(PREFIX)/share/applications/

	# Install icons, 16x16
	install -m 644 src/icons/16x16/carla.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/catarina.png        $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/catia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 src/icons/16x16/claudia.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/

	# Install icons, 48x48
	install -m 644 src/icons/48x48/carla.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/catarina.png        $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/catia.png           $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 src/icons/48x48/claudia.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/

	# Install icons, 128x128
	install -m 644 src/icons/128x128/carla.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/catarina.png      $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/catia.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 src/icons/128x128/claudia.png       $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/

	# Install icons, 256x256
	install -m 644 src/icons/256x256/carla.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/catarina.png      $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/catia.png         $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 src/icons/256x256/claudia.png       $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/

	# Install icons, scalable
	install -m 644 src/icons/svg/carla.svg             $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/svg/catarina.svg          $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/svg/catia.svg             $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 src/icons/svg/claudia.svg           $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/

# 	# Install main code
# 	install -m 755 src/*.py $(DESTDIR)$(PREFIX)/share/cadence/src/
# 	install -m 755 src/carla/*.so $(DESTDIR)$(PREFIX)/share/cadence/src/carla/
# 
# 	# Install addtional stuff for Cadence
# 	install -m 644 pulse2jack/* $(DESTDIR)$(PREFIX)/share/cadence/pulse2jack/
# 	install -m 644 data/99cadence-session-start $(X11_RC_DIR)
# 
# 	# Install additional stuff for Claudia
# 	cp -r icons/* $(DESTDIR)$(PREFIX)/share/cadence/icons/
# 	cp -r templates/* $(DESTDIR)$(PREFIX)/share/cadence/templates/
# 
# 	# Install addtional stuff for Carla
# 	install -m 644 data/carla.lv2/* $(DESTDIR)$(PREFIX)/lib/lv2/carla.lv2/
# 
# 	# Adjust PREFIX value in script files
# 	sed -i "s/X-PREFIX-X/$(SED_PREFIX)/" \
# 		$(DESTDIR)$(PREFIX)/bin/cadence \
# 		$(DESTDIR)$(PREFIX)/bin/catarina \
# 		$(DESTDIR)$(PREFIX)/bin/catia \
# 		$(DESTDIR)$(PREFIX)/bin/claudia \
# 		$(DESTDIR)$(PREFIX)/bin/carla \
# 		$(DESTDIR)$(PREFIX)/bin/carla-control \
# 		$(DESTDIR)$(PREFIX)/bin/jack_logs \
# 		$(DESTDIR)$(PREFIX)/bin/jack_meter2 \
# 		$(DESTDIR)$(PREFIX)/bin/jack_render \
# 		$(DESTDIR)$(PREFIX)/bin/jack_settings \
# 		$(DESTDIR)$(PREFIX)/bin/jack_xycontroller \
# 		$(DESTDIR)$(PREFIX)/bin/pulse2jack \
# 		$(X11_RC_DIR)/99cadence-session-start
# 
# 	# For Ubuntu there are PPAs
# 	rm -f $(DESTDIR)$(PREFIX)/share/applications/cadence-unity-support.desktop

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/catarina
	rm -f $(DESTDIR)$(PREFIX)/bin/catia
	rm -f $(DESTDIR)$(PREFIX)/bin/claudia
	rm -f $(DESTDIR)$(PREFIX)/bin/carla
	rm -f $(DESTDIR)$(PREFIX)/bin/carla-bridge-*
	rm -f $(DESTDIR)$(PREFIX)/bin/carla-discovery-*
# 	rm -f $(DESTDIR)$(PREFIX)/bin/jack_logs
# 	rm -f $(DESTDIR)$(PREFIX)/bin/jack_meter2
# 	rm -f $(DESTDIR)$(PREFIX)/bin/jack_render
# 	rm -f $(DESTDIR)$(PREFIX)/bin/jack_settings
# 	rm -f $(DESTDIR)$(PREFIX)/bin/jack_xycontroller
# 	rm -f $(DESTDIR)$(PREFIX)/bin/pulse2jack
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catarina.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/catia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/claudia.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/catarina.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/catia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/claudia.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catarina.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/catia.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claudia.svg
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/share/cadence/

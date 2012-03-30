#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PYUIC = pyuic4 --pyqt3-wrapper
PYRCC = pyrcc4 -py3

all: build

build: UI RES LANG

UI: catarina catia claudia carla tools

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

carla: carla_backend carla_bridges carla_discovery carla_gui

carla_backend:
	$(MAKE) -C src/carla

carla_bridges:
# 	$(MAKE) native$(_arch_n) -C src/carla-bridges

carla_discovery:
# 	$(MAKE) native$(_arch_n) -C src/carla-discovery

carla_gui: src/ui_carla.py src/ui_carla_control.py \
	src/ui_carla_about.py src/ui_carla_database.py src/ui_carla_edit.py src/ui_carla_parameter.py src/ui_carla_plugin.py src/ui_carla_refresh.py \
	src/ui_inputdialog_value.py

tools: \
	src/ui_logs.py src/ui_render.py src/ui_xycontroller.py \
	src/ui_settings_app.py src/ui_settings_jack.py

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

LANG:
#	pylupdate4 -verbose src/lang/lang.pro
#	lrelease src/lang/lang.pro

clean:
	rm -f *~ src/*~ src/*.pyc src/*.dll src/*.so src/ui_*.py src/icons_rc.py

distclean: clean

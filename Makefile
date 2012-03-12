#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PYUIC = pyuic4 --pyqt3-wrapper
PYRCC = pyrcc4 -py3

all: build

build: UI RES LANG

UI: catarina catia tools

catarina: src/ui_catarina.py \
	src/ui_catarina_addgroup.py src/ui_catarina_removegroup.py src/ui_catarina_renamegroup.py \
	src/ui_catarina_addport.py src/ui_catarina_removeport.py src/ui_catarina_renameport.py \
	src/ui_catarina_connectports.py src/ui_catarina_disconnectports.py

catia: src/ui_catia.py

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

RES: src/icons_rc.py

src/icons_rc.py: src/icons/icons.qrc
	$(PYRCC) -o src/icons_rc.py $<

LANG:
#	pylupdate4 -verbose src/lang/lang.pro
#	lrelease src/lang/lang.pro

clean:
	rm -f *~ src/*~ src/*.pyc src/*.dll src/*.so src/ui_*.py src/icons_rc.py

distclean: clean

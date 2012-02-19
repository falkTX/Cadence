#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PYUIC = pyuic4 --pyqt3-wrapper
PYRCC = pyrcc4 -py3

all: build

build: UI RES LANG

UI: tools

tools: \
	src/ui_logs.py src/ui_render.py

src/ui_logs.py: src/ui/logs.ui
	$(PYUIC) -o src/ui_logs.py $<

src/ui_render.py: src/ui/render.ui
	$(PYUIC) -o src/ui_render.py $<

RES: src/icons_rc.py

src/icons_rc.py: src/icons/icons.qrc
	$(PYRCC) -o src/icons_rc.py $<

LANG:
#	pylupdate4 -verbose src/lang/lang.pro
#	lrelease src/lang/lang.pro

clean:
	rm -f *~ src/*~ src/*.pyc src/*.dll src/*.so src/ui_*.py src/icons_rc.py

distclean: clean

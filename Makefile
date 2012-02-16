#!/usr/bin/make -f
# Makefile for Cadence #
# ---------------------- #
# Created by falkTX
#

PYUIC = pyuic4
PYRCC = pyrcc4

all: build

build: UI RES LANG

UI: tools

tools: \
	src/ui_logs.py

src/ui_logs.py: src/ui/logs.ui
	$(PYUIC) -w -o src/ui_logs.py $<

RES: src/icons_rc.py

src/icons_rc.py: src/icons/icons.qrc
	$(PYRCC) -py3 -o src/icons_rc.py $<

LANG:
#	pylupdate4 -verbose src/lang/lang.pro
#	lrelease src/lang/lang.pro

clean:
	rm -f *~ src/*~  src/*.pyc src/*.so src/ui_*.py src/icons_rc.py

distclean: clean

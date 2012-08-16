# QtCreator project file

QT = core gui

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = jack

TARGET   = jackmeter
TEMPLATE = app
VERSION  = 0.5.0

DEFINES  = HAVE_JACKSESSION

SOURCES  = \
    ../jackmeter.cpp \
    ../../widgets/digitalpeakmeter.cpp

HEADERS  = \
    ../../jack_utils.h \
    ../../widgets/digitalpeakmeter.h

QMAKE_CXXFLAGS *= -std=c++0x

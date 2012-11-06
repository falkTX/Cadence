# QtCreator project file

QT = core gui

CONFIG    = debug
CONFIG   += link_pkgconfig qt resources warn_on

DEFINES   = DEBUG
DEFINES  += HAVE_JACKSESSION
PKGCONFIG = jack

TARGET   = cadence-jackmeter
TEMPLATE = app
VERSION  = 0.5.0

SOURCES  = \
    jackmeter.cpp \
    ../widgets/digitalpeakmeter.cpp

HEADERS  = \
    ../jack_utils.hpp \
    ../widgets/digitalpeakmeter.hpp

INCLUDEPATH = \
    ../widgets

RESOURCES = \
    ../../resources/resources-jackmeter.qrc

QMAKE_CXXFLAGS *= -std=c++0x

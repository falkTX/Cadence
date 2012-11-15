# QtCreator project file

CONFIG    = debug
CONFIG   += link_pkgconfig warn_on # plugin shared

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

DEFINES  += _GNU_SOURCE

PKGCONFIG = jack

TARGET   = caitlib
TEMPLATE = app # lib
VERSION  = 0.0.1

SOURCES  = \
     caitlib.c \
     memory_atomic.c

SOURCES += \
    test.c

HEADERS  = \
    caitlib.h

INCLUDEPATH = .

QMAKE_CFLAGS *= -fvisibility=hidden -fPIC -Wextra -Werror -std=gnu99

unix {
LIBS = -lm -lpthread
}

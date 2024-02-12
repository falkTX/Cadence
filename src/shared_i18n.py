#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common/Shared code related to Internationalization
# Copyright (C) 2019 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os, sys
if True:
    from PyQt5.QtCore import QCoreApplication, QTranslator, QLocale, QLibraryInfo
else:
    from PyQt4.QtCore import QCoreApplication, QTranslator, QLocale, QLibraryInfo

def setup_i18n():
    app = QCoreApplication.instance()
    locale = QLocale()

    syspath = sys.path[0]
    qmpath = os.path.join(syspath, "..", "translations")

    # Load translations from Cadence resources
    translator = QTranslator()
    if not translator.load(locale, "cadence", "_", qmpath):
        return False
    app.installTranslator(translator)
    app.fAppTranslator = translator

    # Load translations from Qt libraries
    translator = QTranslator()
    if not translator.load(locale, "qt", "_", qmpath):
        translator.load(locale, "qt", "_", QLibraryInfo.location(QLibraryInfo.TranslationsPath))
    app.installTranslator(translator)
    app.fSysTranslator = translator

    return True

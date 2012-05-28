#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# KDE, App-Indicator or Qt Systray
# Copyright (C) 2011-2012 Filipe Coelho <falktx@gmail.com>
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

# TESTING - remove if python3
from sip import setapi
setapi("QString", 2)

# Imports (Global)
from os import getenv
from PyQt4.QtCore import SIGNAL
from PyQt4.QtGui import QAction, QIcon, QMenu, QSystemTrayIcon

try:
    if getenv("DESKTOP_SESSION") in ("ubuntu", "ubuntu-2d") and not os.path.exists("/var/kxstudio/no_appindicators"):
        from gi.repository import AppIndicator3, Gtk
        TrayEngine = "AppIndicator"
    elif getenv("KDE_FULL_SESSION"):
        from PyKDE4.kdeui import KAction, KIcon, KMenu, KStatusNotifierItem
        TrayEngine = "KDE"
    else:
        TrayEngine = "Qt"
except:
    TrayEngine = "Qt"

print("Using Tray Engine '%s'" % TrayEngine)

iActNameId = 0
iActWidget = 1
iActParentMenuId = 2
iActFunc   = 3

iSepNameId = 0
iSepWidget = 1
iSepParentMenuId = 2

iMenuNameId = 0
iMenuWidget = 1
iMenuParentMenuId = 2

# Get Icon from user theme, using our own as backup (Oxygen)
def getIcon(icon, size=16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# Global Systray class
class GlobalSysTray(object):
    def __init__(self, parent, name, icon):
        object.__init__(self)

        self._parent = parent
        self._gtk_running = False
        self._quit_added  = False

        self.act_indexes  = []
        self.sep_indexes  = []
        self.menu_indexes = []

        if TrayEngine == "KDE":
            self.menu = KMenu(parent)
            self.menu.setTitle(name)
            self.tray = KStatusNotifierItem()
            self.tray.setAssociatedWidget(parent)
            self.tray.setCategory(KStatusNotifierItem.ApplicationStatus)
            self.tray.setContextMenu(self.menu)
            self.tray.setIconByPixmap(getIcon(icon))
            self.tray.setTitle(name)
            self.tray.setToolTipTitle(" ")
            self.tray.setToolTipIconByPixmap(getIcon(icon))
            # Double-click is managed by KDE

        elif TrayEngine == "AppIndicator":
            self.menu = Gtk.Menu()
            self.tray = AppIndicator3.Indicator.new(name, icon, AppIndicator3.IndicatorCategory.APPLICATION_STATUS)
            self.tray.set_menu(self.menu)
            # Double-click is not possible with App-Indicators

        elif TrayEngine == "Qt":
            self.menu = QMenu(parent)
            self.tray = QSystemTrayIcon(getIcon(icon))
            self.tray.setContextMenu(self.menu)
            self.tray.setParent(parent)
            self.tray.connect(self.tray, SIGNAL("activated(QSystemTrayIcon::ActivationReason)"), self.qt_systray_clicked)

    # -------------------------------------------------------------------------------------------

    def addAction(self, act_name_id, act_name_string, is_check=False):
        if TrayEngine == "KDE":
            act_widget = KAction(act_name_string, self.menu)
            act_widget.setCheckable(is_check)
            self.menu.addAction(act_widget)

        elif TrayEngine == "AppIndicator":
            if is_check:
                act_widget = Gtk.CheckMenuItem(act_name_string)
            else:
                act_widget = Gtk.ImageMenuItem(act_name_string)
                act_widget.set_image(None)
            act_widget.show()
            self.menu.append(act_widget)

        elif TrayEngine == "Qt":
            act_widget = QAction(act_name_string, self.menu)
            act_widget.setCheckable(is_check)
            self.menu.addAction(act_widget)

        else:
            act_widget = None

        act_obj = [None, None, None, None]
        act_obj[iActNameId] = act_name_id
        act_obj[iActWidget] = act_widget

        self.act_indexes.append(act_obj)

    def addSeparator(self, sep_name_id):
        if TrayEngine == "KDE":
            sep_widget = self.menu.addSeparator()

        elif TrayEngine == "AppIndicator":
            sep_widget = Gtk.SeparatorMenuItem()
            sep_widget.show()
            self.menu.append(sep_widget)

        elif TrayEngine == "Qt":
            sep_widget = self.menu.addSeparator()

        else:
            sep_widget = None

        sep_obj = [None, None, None]
        sep_obj[iSepNameId] = sep_name_id
        sep_obj[iSepWidget] = sep_widget

        self.sep_indexes.append(sep_obj)

    def addMenu(self, menu_name_id, menu_name_string):
        if TrayEngine == "KDE":
            menu_widget = KMenu(menu_name_string, self.menu)
            self.menu.addMenu(menu_widget)

        elif TrayEngine == "AppIndicator":
            menu_widget = Gtk.MenuItem(menu_name_string)
            menu_parent = Gtk.Menu()
            menu_widget.set_submenu(menu_parent)
            menu_widget.show()
            self.menu.append(menu_widget)

        elif TrayEngine == "Qt":
            menu_widget = QMenu(menu_name_string, self.menu)
            self.menu.addMenu(menu_widget)

        else:
            menu_widget = None

        menu_obj = [None, None, None]
        menu_obj[iMenuNameId] = menu_name_id
        menu_obj[iMenuWidget] = menu_widget

        self.menu_indexes.append(menu_obj)

    # -------------------------------------------------------------------------------------------

    def addMenuAction(self, menu_name_id, act_name_id, act_name_string, is_check=False):
        i = self.get_menu_index(menu_name_id)
        if i < 0: return

        menu_widget = self.menu_indexes[i][iMenuWidget]

        if TrayEngine == "KDE":
            act_widget = KAction(act_name_string, menu_widget)
            act_widget.setCheckable(is_check)
            menu_widget.addAction(act_widget)

        elif TrayEngine == "AppIndicator":
            menu_widget = menu_widget.get_submenu()
            if is_check:
                act_widget = Gtk.CheckMenuItem(act_name_string)
            else:
                act_widget = Gtk.ImageMenuItem(act_name_string)
                act_widget.set_image(None)
            act_widget.show()
            menu_widget.append(act_widget)

        elif TrayEngine == "Qt":
            act_widget = QAction(act_name_string, menu_widget)
            act_widget.setCheckable(is_check)
            menu_widget.addAction(act_widget)

        else:
            act_widget = None

        act_obj = [None, None, None, None]
        act_obj[iActNameId] = act_name_id
        act_obj[iActWidget] = act_widget
        act_obj[iActParentMenuId] = menu_name_id

        self.act_indexes.append(act_obj)

    def addMenuSeparator(self, menu_name_id, sep_name_id):
        i = self.get_menu_index(menu_name_id)
        if i < 0: return

        menu_widget = self.menu_indexes[i][iMenuWidget]

        if TrayEngine == "KDE":
            sep_widget = menu_widget.addSeparator()

        elif TrayEngine == "AppIndicator":
            menu_widget = menu_widget.get_submenu()
            sep_widget = Gtk.SeparatorMenuItem()
            sep_widget.show()
            menu_widget.append(sep_widget)

        elif TrayEngine == "Qt":
            sep_widget = menu_widget.addSeparator()

        else:
            sep_widget = None

        sep_obj = [None, None, None]
        sep_obj[iSepNameId] = sep_name_id
        sep_obj[iSepWidget] = sep_widget
        sep_obj[iSepParentMenuId] = menu_name_id

        self.sep_indexes.append(sep_obj)

    #def addSubMenu(self, menu_name_id, new_menu_name_id, new_menu_name_string):
        #menu_index = self.get_menu_index(menu_name_id)
        #if menu_index < 0: return
        #menu_widget = self.menu_indexes[menu_index][1]
        ##if TrayEngine == "KDE":
            ##new_menu_widget = KMenu(new_menu_name_string, self.menu)
            ##menu_widget.addMenu(new_menu_widget)
        ##elif TrayEngine == "AppIndicator":
            ##new_menu_widget = Gtk.MenuItem(new_menu_name_string)
            ##new_menu_widget.show()
            ##menu_widget.get_submenu().append(new_menu_widget)
            ##parent_menu_widget = Gtk.Menu()
            ##new_menu_widget.set_submenu(parent_menu_widget)
        ##else:
        #if (1):
            #new_menu_widget = QMenu(new_menu_name_string, self.menu)
            #menu_widget.addMenu(new_menu_widget)
        #self.menu_indexes.append([new_menu_name_id, new_menu_widget, menu_name_id])

    # -------------------------------------------------------------------------------------------

    def connect(self, act_name_id, act_func):
        i = self.get_act_index(act_name_id)
        if i < 0: return

        act_widget = self.act_indexes[i][iActWidget]

        if TrayEngine == "KDE":
            self.tray.connect(act_widget, SIGNAL("triggered()"), act_func)

        elif TrayEngine == "AppIndicator":
            act_widget.connect("activate", self.gtk_call_func, act_name_id)

        elif TrayEngine == "Qt":
            self.tray.connect(act_widget, SIGNAL("triggered()"), act_func)

        self.act_indexes[i][iActFunc] = act_func

    # -------------------------------------------------------------------------------------------

    #def setActionChecked(self, act_name_id, yesno):
        #index = self.get_act_index(act_name_id)
        #if index < 0: return
        #act_widget = self.act_indexes[index][1]
        ##if TrayEngine == "KDE":
            ##act_widget.setChecked(yesno)
        ##elif TrayEngine == "AppIndicator":
            ##if type(act_widget) != Gtk.CheckMenuItem:
                ##return # Cannot continue
            ##act_widget.set_active(yesno)
        ##else:
        #if (1):
            #act_widget.setChecked(yesno)

    def setActionEnabled(self, act_name_id, yesno):
        i = self.get_act_index(act_name_id)
        if i < 0: return

        act_widget = self.act_indexes[i][iActWidget]

        if TrayEngine == "KDE":
            act_widget.setEnabled(yesno)

        elif TrayEngine == "AppIndicator":
            act_widget.set_sensitive(yesno)

        elif TrayEngine == "Qt":
            act_widget.setEnabled(yesno)

    def setActionIcon(self, act_name_id, icon):
        i = self.get_act_index(act_name_id)
        if i < 0: return

        act_widget = self.act_indexes[i][iActWidget]

        if TrayEngine == "KDE":
            act_widget.setIcon(KIcon(icon))

        elif TrayEngine == "AppIndicator":
            if not isinstance(act_widget, Gtk.ImageMenuItem):
                # Cannot use icons here
                return

            act_widget.set_image(Gtk.Image.new_from_icon_name(icon, Gtk.IconSize.MENU))
            #act_widget.set_always_show_image(True)

        elif TrayEngine == "Qt":
            act_widget.setIcon(getIcon(icon))

    def setActionText(self, act_name_id, text):
        i = self.get_act_index(act_name_id)
        if i < 0: return

        act_widget = self.act_indexes[i][iActWidget]

        if TrayEngine == "KDE":
            act_widget.setText(text)

        elif TrayEngine == "AppIndicator":
            if isinstance(act_widget, Gtk.ImageMenuItem):
                # Fix icon reset
                last_icon = act_widget.get_image()
                act_widget.set_label(text)
                act_widget.set_image(last_icon)
            else:
                act_widget.set_label(text)

        elif TrayEngine == "Qt":
            act_widget.setText(text)

    def setIcon(self, icon):
        if TrayEngine == "KDE":
            self.tray.setIconByPixmap(getIcon(icon))
            #self.tray.setToolTipIconByPixmap(getIcon(icon))

        elif TrayEngine == "AppIndicator":
            self.tray.set_icon(icon)

        elif TrayEngine == "Qt":
            self.tray.setIcon(getIcon(icon))

    def setToolTip(self, text):
        if TrayEngine == "KDE":
            self.tray.setToolTipSubTitle(text)

        elif TrayEngine == "AppIndicator":
            # ToolTips are disabled in App-Indicators by design
            pass

        elif TrayEngine == "Qt":
            self.tray.setToolTip(text)

    # -------------------------------------------------------------------------------------------

    #def removeAction(self, act_name_id):
        #index = self.get_act_index(act_name_id)
        #if index < 0: return
        #act_widget = self.act_indexes[index][1]
        #parent_menu_widget = self.get_parent_menu_widget(self.act_indexes[index][2])
        ##if TrayEngine == "KDE":
            ##parent_menu_widget.removeAction(act_widget)
        ##elif TrayEngine == "AppIndicator":
            ##act_widget.hide()
            ##parent_menu_widget.remove(act_widget)
        ##else:
        #if (1):
            #parent_menu_widget.removeAction(act_widget)
        #self.act_indexes.pop(index)

    #def removeSeparator(self, sep_name_id):
        #index = self.get_sep_index(sep_name_id)
        #if index < 0: return
        #sep_widget = self.sep_indexes[index][1]
        #parent_menu_widget = self.get_parent_menu_widget(self.sep_indexes[index][2])
        ##if TrayEngine == "KDE":
            ##parent_menu_widget.removeAction(sep_widget)
        ##elif TrayEngine == "AppIndicator":
            ##sep_widget.hide()
            ##parent_menu_widget.remove(sep_widget)
        ##else:
        #if (1):
            #parent_menu_widget.removeAction(sep_widget)
        #self.sep_indexes.pop(index)

    #def removeMenu(self, menu_name_id):
        #index = self.get_menu_index(menu_name_id)
        #if index < 0: return
        #menu_widget = self.menu_indexes[index][1]
        #parent_menu_widget = self.get_parent_menu_widget(self.menu_indexes[index][2])
        ##if TrayEngine == "KDE":
            ##parent_menu_widget.removeAction(menu_widget.menuAction())
        ##elif TrayEngine == "AppIndicator":
            ##menu_widget.hide()
            ##parent_menu_widget.remove(menu_widget.get_submenu())
        ##else:
        #if (1):
            #parent_menu_widget.removeAction(menu_widget.menuAction())
        #self.remove_actions_by_menu_name_id(menu_name_id)
        #self.remove_separators_by_menu_name_id(menu_name_id)
        #self.remove_submenus_by_menu_name_id(menu_name_id)

    # -------------------------------------------------------------------------------------------

    #def clearAll(self):
        ##if TrayEngine == "KDE":
            ##self.menu.clear()
        ##elif TrayEngine == "AppIndicator":
            ##for child in self.menu.get_children():
                ##self.menu.remove(child)
        ##else:
        #if (1):
            #self.menu.clear()

        #self.act_indexes = []
        #self.sep_indexes = []
        #self.menu_indexes = []

    #def clearMenu(self, menu_name_id):
        #menu_index = self.get_menu_index(menu_name_id)
        #if menu_index < 0: return
        #menu_widget = self.menu_indexes[menu_index][1]
        ##if TrayEngine == "KDE":
            ##menu_widget.clear()
        ##elif TrayEngine == "AppIndicator":
            ##for child in menu_widget.get_submenu().get_children():
                ##menu_widget.get_submenu().remove(child)
        ##else:
        #if (1):
            #menu_widget.clear()
        #list_of_submenus = [menu_name_id]
        #for x in range(0, 10): # 10x level deep, should cover all cases...
            #for this_menu_name_id, menu_widget, parent_menu_id in self.menu_indexes:
                #if parent_menu_id in list_of_submenus and this_menu_name_id not in list_of_submenus:
                    #list_of_submenus.append(this_menu_name_id)
        #for this_menu_name_id in list_of_submenus:
            #self.remove_actions_by_menu_name_id(this_menu_name_id)
            #self.remove_separators_by_menu_name_id(this_menu_name_id)
            #self.remove_submenus_by_menu_name_id(this_menu_name_id)

    # -------------------------------------------------------------------------------------------

    def getTrayEngine(self):
        return TrayEngine

    def isTrayAvailable(self):
        if TrayEngine in ("KDE", "Qt"):
            return QSystemTrayIcon.isSystemTrayAvailable()
        elif TrayEngine == "AppIndicator":
            # Ubuntu/Unity always has a systray
            return True
        else:
            return False

    # -------------------------------------------------------------------------------------------

    def show(self):
        if not self._quit_added:
            self._quit_added = True

            if TrayEngine != "KDE":
                self.addSeparator("_quit")
                self.addAction("show", self._parent.tr("Minimize"))
                self.addAction("quit", self._parent.tr("Quit"))
                self.setActionIcon("quit", "application-exit")
                self.connect("show", self.__hideShowCall)
                self.connect("quit", self.__quitCall)

        if TrayEngine == "KDE":
            self.tray.setStatus(KStatusNotifierItem.Active)
        elif TrayEngine == "AppIndicator":
            self.tray.set_status(AppIndicator3.IndicatorStatus.ACTIVE)
        elif TrayEngine == "Qt":
            self.tray.show()

    def hide(self):
        if TrayEngine == "KDE":
            self.tray.setStatus(KStatusNotifierItem.Passive)
        elif TrayEngine == "AppIndicator":
            self.tray.set_status(AppIndicator3.IndicatorStatus.PASSIVE)
        elif TrayEngine == "Qt":
            self.tray.hide()

    def close(self):
        if TrayEngine == "KDE":
            self.menu.close()
        elif TrayEngine == "AppIndicator":
            if self._gtk_running:
                self._gtk_running = False
                Gtk.main_quit()
        elif TrayEngine == "Qt":
            self.menu.close()

    def exec_(self, app):
        if TrayEngine == "AppIndicator":
            self._gtk_running = True
            return Gtk.main()
        else:
            return app.exec_()

    # -------------------------------------------------------------------------------------------

    def get_act_index(self, act_name_id):
        for i in range(len(self.act_indexes)):
            if self.act_indexes[i][iActNameId] == act_name_id:
                return i
        else:
            print("systray.py - Failed to get action index for %s" % act_name_id)
            return -1

    def get_sep_index(self, sep_name_id):
        for i in range(len(self.sep_indexes)):
            if self.sep_indexes[i][iSepNameId] == sep_name_id:
                return i
        else:
            print("systray.py - Failed to get separator index for %s" % sep_name_id)
            return -1

    def get_menu_index(self, menu_name_id):
        for i in range(len(self.menu_indexes)):
            if self.menu_indexes[i][iMenuNameId] == menu_name_id:
                return i
        else:
            print("systray.py - Failed to get menu index for %s" % menu_name_id)
            return -1

    #def get_parent_menu_widget(self, parent_menu_id):
        #if parent_menu_id != None:
            #menu_index = self.get_menu_index(parent_menu_id)
            #if menu_index >= 0:
                #return self.menu_indexes[menu_index][1]
            #else:
                #print("systray.py::Failed to get parent Menu widget for", parent_menu_id)
                #return None
        #else:
            #return self.menu

    #def remove_actions_by_menu_name_id(self, menu_name_id):
        #h = 0
        #for i in range(len(self.act_indexes)):
            #act_name_id, act_widget, parent_menu_id, act_func = self.act_indexes[i - h]
            #if parent_menu_id == menu_name_id:
                #self.act_indexes.pop(i - h)
                #h += 1

    #def remove_separators_by_menu_name_id(self, menu_name_id):
        #h = 0
        #for i in range(len(self.sep_indexes)):
            #sep_name_id, sep_widget, parent_menu_id = self.sep_indexes[i - h]
            #if parent_menu_id == menu_name_id:
                #self.sep_indexes.pop(i - h)
                #h += 1

    #def remove_submenus_by_menu_name_id(self, submenu_name_id):
        #h = 0
        #for i in range(len(self.menu_indexes)):
            #menu_name_id, menu_widget, parent_menu_id = self.menu_indexes[i - h]
            #if parent_menu_id == submenu_name_id:
                #self.menu_indexes.pop(i - h)
                #h += 1

    # -------------------------------------------------------------------------------------------

    def gtk_call_func(self, gtkmenu, act_name_id):
        i = self.get_act_index(act_name_id)
        if i < 0: return None

        act_func = self.act_indexes[i][iActFunc]
        return act_func()

    def qt_systray_clicked(self, reason):
        if reason in (QSystemTrayIcon.DoubleClick, QSystemTrayIcon.Trigger):
            self.__hideShowCall()

    # -------------------------------------------------------------------------------------------

    def __hideShowCall(self):
        if self._parent.isVisible():
            self.setActionText("show", self._parent.tr("Restore"))
            self._parent.hide()
        else:
            self.setActionText("show", self._parent.tr("Minimize"))
            if self._parent.isMaximized():
                self._parent.showMaximized()
            else:
                self._parent.showNormal()

    def __quitCall(self):
        self._parent.close()

#--------------- main ------------------
if __name__ == '__main__':
    import sys
    from PyQt4.QtGui import QApplication, QDialog, QMessageBox

    class ExampleGUI(QDialog):
        def __init__(self, parent=None):
            QDialog.__init__(self, parent)

            self.setWindowIcon(getIcon("audacity"))

            self.systray = GlobalSysTray(self, "Claudia", "claudia")
            self.systray.addAction("about", self.tr("About"))
            self.systray.setIcon("audacity")
            self.systray.setToolTip("Demo systray app")

            self.systray.connect("about", self.about)

            self.systray.show()

        def about(self):
            QMessageBox.about(self, self.tr("About"), self.tr("Systray Demo"))

        def done(self, r):
            QDialog.done(self, r)
            self.close()

        def closeEvent(self, event):
            self.systray.close()
            QDialog.closeEvent(self, event)

    app = QApplication(sys.argv)
    gui = ExampleGUI()
    gui.show()
    sys.exit(gui.systray.exec_(app))

#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Imports (Global)
from PyQt4.QtCore import Qt, QTimer, SIGNAL
from PyQt4.QtGui import QAbstractSpinBox, QComboBox, QCursor, QDialog, QInputDialog, QMenu, QPainter, QProgressBar, QValidator
from PyQt4.QtGui import QStyleFactory
from math import isnan

# Imports (Custom)
import ui_inputdialog_value

def fix_value(value, minimum, maximum):
  if (isnan(value)):
    print("Parameter is NaN! - %f" % (value))
    return minimum
  elif (value < minimum):
    print("Parameter too low! - %f/%f" % (value, minimum))
    return minimum
  elif (value > maximum):
    print("Parameter too high! - %f/%f" % (value, maximum))
    return maximum
  else:
    return value

QPlastiqueStyle = QStyleFactory.create("Plastique")

# Custom InputDialog with Scale Points support
class CustomInputDialog(QDialog, ui_inputdialog_value.Ui_Dialog):
    def __init__(self, parent=None, label="", current=0, minimum=0, maximum=0, step=0, scalepoints=None):
        super(CustomInputDialog, self).__init__(parent)
        self.setupUi(self)

        self.label.setText(label)
        self.doubleSpinBox.setMinimum(minimum)
        self.doubleSpinBox.setMaximum(maximum)
        self.doubleSpinBox.setValue(current)
        self.doubleSpinBox.setSingleStep(step)

        self.ret_value = current

        if (scalepoints == None):
          self.groupBox.setVisible(False)
          self.resize(200, 0)
        else:
          text = "<table>"
          for scalepoint in scalepoints:
            text += "<tr><td align='right'>%s</td><td align='left'> - %s</td></tr>" % (str(scalepoint['value']), scalepoint['label'])
          text += "</table>"
          self.textBrowser.setText(text)
          self.resize(200, 300)

        self.connect(self, SIGNAL("accepted()"), self.setReturnValue)

    def setReturnValue(self):
        self.ret_value = self.doubleSpinBox.value()

# Progress-Bar used for ParamSpinBox
class ParamProgressBar(QProgressBar):
    def __init__(self, parent=None):
        super(ParamProgressBar, self).__init__(parent)

        self._minimum = 0.0
        self._maximum = 1.0
        self._step_small = 0.0
        self._rvalue = 0.0

        self._label = ""
        self._pre_label = " "
        self._left_click_down = False

        self.setMinimum(0)
        self.setMaximum(1000)
        self.setValue(0)
        self.setFormat("(none)")

    def set_minimum(self, value):
        self._minimum = value

    def set_maximum(self, value):
        self._maximum = value

    def set_value(self, value):
        self._rvalue = value
        vper = (value-self._minimum)/(self._maximum-self._minimum)
        self.setValue(vper*1000)

    def set_label(self, label):
        self._label = label.strip()

        if (self._label == "(coef)"):
          self._label = ""
          self._pre_label = "*"

        self.update()

    def handleMouseEventPos(self, pos):
        xper  = float(pos.x())/self.width()
        value = xper*(self._maximum-self._minimum)+self._minimum

        if (value < self._minimum):
          value = self._minimum
        elif (value > self._maximum):
          value = self._maximum

        self.emit(SIGNAL("valueChangedFromBar(double)"), value)

    def mousePressEvent(self, event):
        if (event.button() == Qt.LeftButton):
          self.handleMouseEventPos(event.pos())
          self._left_click_down = True
        else:
          self._left_click_down = False
        return QProgressBar.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if (self._left_click_down):
          self.handleMouseEventPos(event.pos())
        return QProgressBar.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        self._left_click_down = False
        return QProgressBar.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        self.setFormat("%s %f %s" % (self._pre_label, self._rvalue, self._label))
        return QProgressBar.paintEvent(self, event)

# Special SpinBox used for parameters
class ParamSpinBox(QAbstractSpinBox):
    def __init__(self, parent=None):
        super(ParamSpinBox, self).__init__(parent)

        self._minimum = 0.0
        self._maximum = 1.0
        self._default = 0.0
        self._value = None
        self._step = 0.0
        self._step_small = 0.0
        self._step_large = 0.0

        self._read_only = False
        self._scalepoints = None
        self._have_scalepoints = False

        self.bar = ParamProgressBar(self)
        self.bar.setContextMenuPolicy(Qt.NoContextMenu)
        self.bar.show()

        self.lineEdit().setVisible(False)

        self.connect(self.bar, SIGNAL("valueChangedFromBar(double)"), self.handleValueChangedFromBar)
        self.connect(self, SIGNAL("customContextMenuRequested(QPoint)"), self.showCustomMenu)

        QTimer.singleShot(0, self.updateBarGeometry)

    def force_plastique_style(self):
        self.setStyle(QPlastiqueStyle)

    def set_minimum(self, value):
        self._minimum = value
        self.bar.set_minimum(value)

    def set_maximum(self, value):
        self._maximum = value
        self.bar.set_maximum(value)

    def set_default(self, value):
        value = fix_value(value, self._minimum, self._maximum)
        self._default = value

    def set_value(self, value, send=True):
        value = fix_value(value, self._minimum, self._maximum)
        if (self._value != value):
          self._value = value
          self.bar.set_value(value)

          if (self._have_scalepoints):
            self.set_scalepoint_value(value)

          if (send):
            self.emit(SIGNAL("valueChanged(double)"), value)

          self.update()

          return True

        else:
          return False

    def set_step(self, value):
        if (value == 0.0):
          self._step = 0.01
        else:
          self._step = value

    def set_step_small(self, value):
        if (value == 0.0):
          self._step_small = 0.001
        else:
          self._step_small = value

    def set_step_large(self, value):
        if (value == 0.0):
          self._step_large = 0.1
        else:
          self._step_large = value

    def set_label(self, label):
        self.bar.set_label(label)

    def set_read_only(self, yesno):
        self.setButtonSymbols(QAbstractSpinBox.UpDownArrows if (yesno) else QAbstractSpinBox.NoButtons)
        self._read_only = yesno
        self.setReadOnly(yesno)

    def set_scalepoints(self, scalepoints, use_scalepoints):
        if (len(scalepoints) > 0):
          self._scalepoints = scalepoints
          self._have_scalepoints = use_scalepoints

          if (use_scalepoints):
            # Hide ProgressBar and create a ComboBox
            self.bar.close()
            self.box = QComboBox(self)
            self.box.setContextMenuPolicy(Qt.NoContextMenu)
            self.box.show()
            self.updateBarGeometry()

            for scalepoint in scalepoints:
              self.box.addItem("%f - %s" % (scalepoint['value'], scalepoint['label']))

            self.set_scalepoint_value(self._value)

            self.connect(self.box, SIGNAL("currentIndexChanged(QString)"), self.handleValueChangedFromBox)

        else:
          self._scalepoints = None

    def set_scalepoint_value(self, value):
        value = self.get_nearest_scalepoint(value)
        for i in range(self.box.count()):
          if (float(self.box.itemText(i).split(" - ", 1)[0]) == value):
            self.box.setCurrentIndex(i)
            break

    def get_nearest_scalepoint(self, real_value):
        final_value = 0.0
        for i in range(len(self._scalepoints)):
          scale_value = self._scalepoints[i]['value']
          if (i == 0):
            final_value = scale_value
          else:
            srange1 = abs(real_value-scale_value)
            srange2 = abs(real_value-final_value)

            if (srange2 > srange1):
              final_value = scale_value

        return final_value

    def handleValueChangedFromBar(self, value):
        if (self._read_only): return

        step = int(0.5+((value-self._minimum)/self._step))
        real_value = self._minimum+(step*self._step)

        self.set_value(real_value)

    def handleValueChangedFromBox(self, box_text):
        if (self._read_only): return

        value = float(box_text.split(" - ", 1)[0])
        last_scale_value = self._scalepoints[len(self._scalepoints)-1]['value']

        if (value == last_scale_value):
          value = self._maximum

        self.set_value(value)

    def showCustomMenu(self, pos):
        menu = QMenu(self)
        act_x_reset = menu.addAction(self.tr("Reset (%f)" % (self._default)))
        menu.addSeparator()
        act_x_copy = menu.addAction(self.tr("Copy (%f)"% (self._value)))
        if (False and not self._read_only):
          act_x_paste = menu.addAction(self.tr("Paste (%s)" % ("TODO")))
        else:
          act_x_paste = menu.addAction(self.tr("Paste"))
          act_x_paste.setEnabled(False)
        menu.addSeparator()
        act_x_set = menu.addAction(self.tr("Set value..."))

        if (self._read_only):
          act_x_reset.setEnabled(False)
          act_x_paste.setEnabled(False)
          act_x_set.setEnabled(False)

        # TODO - NOT IMPLEMENTED YET
        act_x_copy.setEnabled(False)

        act_x_sel = menu.exec_(QCursor.pos())

        if (act_x_sel == act_x_set):
          dialog = CustomInputDialog(self, self.parent().label.text(), self._value, self._minimum, self._maximum, self._step, self._scalepoints)
          if (dialog.exec_()):
            value = dialog.ret_value
            self.set_value(value)

        elif (act_x_sel == act_x_copy):
          pass

        elif (act_x_sel == act_x_paste):
          pass

        elif (act_x_sel == act_x_reset):
          self.set_value(self._default)

    def stepBy(self, steps):
        if (steps == 0):
          return

        value = self._value+(steps*self._step)

        if (value < self._minimum):
          value = self._minimum
        elif (value > self._maximum):
          value = self._maximum

        self.set_value(value)

    def stepEnabled(self):
        if (self._read_only):
          return QAbstractSpinBox.StepNone
        elif (self._value <= self._minimum):
          return QAbstractSpinBox.StepUpEnabled
        elif (self._value >= self._maximum):
          return QAbstractSpinBox.StepDownEnabled
        else:
          return QAbstractSpinBox.StepUpEnabled|QAbstractSpinBox.StepDownEnabled

    def updateAll(self):
        self.update()
        self.bar.update()
        if (self._have_scalepoints):
          self.box.update()

    def updateBarGeometry(self):
        self.bar.setGeometry(self.lineEdit().geometry())
        if (self._have_scalepoints):
          self.box.setGeometry(self.lineEdit().geometry())

    def resizeEvent(self, event):
        QTimer.singleShot(0, self.updateBarGeometry)
        return QAbstractSpinBox.resizeEvent(self, event)

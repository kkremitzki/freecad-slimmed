# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import FreeCADGui
import PathScripts.PathGui as PathGui
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathSetupSheetOpPrototype as PathSetupSheetOpPrototype
import PathScripts.PathSetupSheetOpPrototypeGui as PathSetupSheetOpPrototypeGui
import PathScripts.PathUtil as PathUtil

from PySide import QtCore, QtGui

__title__ = "Setup Sheet Editor"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Task panel editor for a SetupSheet"

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

class ViewProvider:

    def __init__(self, vobj, name):
        PathLog.track(name)
        vobj.Proxy = self
        self.icon = name
        mode = 2
        #vobj.setEditorMode('BoundingBox', mode)
        #vobj.setEditorMode('DisplayMode', mode)
        #vobj.setEditorMode('Selectable', mode)
        #vobj.setEditorMode('ShapeColor', mode)
        #vobj.setEditorMode('Transparency', mode)

    def attach(self, vobj):
        PathLog.track()
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        return ":/icons/Path-SetupSheet.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getDisplayMode(self, mode):
        return 'Default'

    def setEdit(self, vobj, mode=0):
        PathLog.track()
        taskPanel = TaskPanel(vobj)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(taskPanel)
        taskPanel.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def claimChildren(self):
        return []

    def doubleClicked(self, vobj):
        self.setEdit(vobj)

class Delegate(QtGui.QStyledItemDelegate):
    PropertyRole = QtCore.Qt.UserRole + 1
    EditorRole   = QtCore.Qt.UserRole + 2


    #def paint(self, painter, option, index):
    #    #PathLog.track(index.column(), type(option))

    def createEditor(self, parent, option, index):
        if index.data(self.EditorRole) is None:
            editor = PathSetupSheetOpPrototypeGui.Editor(index.data(self.PropertyRole))
            index.model().setData(index, editor, self.EditorRole)
        return index.data(self.EditorRole).widget(parent)

    def setEditorData(self, widget, index):
        PathLog.track(index.row(), index.column())
        index.data(self.EditorRole).setEditorData(widget)

    def setModelData(self, widget, model, index):
        PathLog.track(index.row(), index.column())
        editor = index.data(self.EditorRole)
        editor.setModelData(widget)
        index.model().setData(index, editor.prop.displayString(), QtCore.Qt.DisplayRole)

    def updateEditorGeometry(self, widget, option, index):
	widget.setGeometry(option.rect)

class OpTaskPanel:

    def __init__(self, obj, name, op):
        self.name = name
        self.obj = obj
        self.op = op
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/SetupOp.ui")
        self.form.setWindowTitle("Op {}".format(self.name))
        self.props = sorted(op.properties())
        self.prototype = op.prototype(name)

    def updateData(self, topLeft, bottomRight):
        if 0 == topLeft.column():
            isset = self.model.item(topLeft.row(), 0).checkState() == QtCore.Qt.Checked
            self.model.item(topLeft.row(), 1).setEnabled(isset)
            self.model.item(topLeft.row(), 2).setEnabled(isset)

    def setupUi(self):
        PathLog.track()

        self.delegate = Delegate(self.form)
        self.model = QtGui.QStandardItemModel(len(self.props), 3, self.form)
        self.model.setHorizontalHeaderLabels(['Set', 'Property', 'Value'])

        for i,name in enumerate(self.props):
            prop = self.prototype.getProperty(name)
            isset = hasattr(self.obj, self.propertyName(name))
            if isset:
                prop.setValue(getattr(self.obj, self.propertyName(name)))

            self.model.setData(self.model.index(i, 0), isset, QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 1), name,  QtCore.Qt.EditRole)
            self.model.setData(self.model.index(i, 2), prop,  Delegate.PropertyRole)
            self.model.setData(self.model.index(i, 2), prop.displayString(),  QtCore.Qt.DisplayRole)

            self.model.item(i, 0).setCheckable(True)
            self.model.item(i, 0).setText('')
            self.model.item(i, 1).setEditable(False)
            self.model.item(i, 1).setToolTip(prop.info)
            self.model.item(i, 2).setToolTip(prop.info)

            if isset:
                self.model.item(i, 0).setCheckState(QtCore.Qt.Checked)
            else:
                self.model.item(i, 0).setCheckState(QtCore.Qt.Unchecked)
                self.model.item(i, 1).setEnabled(False)
                self.model.item(i, 2).setEnabled(False)

        self.form.table.setModel(self.model)
        self.form.table.setItemDelegateForColumn(2, self.delegate)
        self.form.table.resizeColumnsToContents()

        self.model.dataChanged.connect(self.updateData)

    def propertyName(self, prop):
        return PathSetupSheet.OpPropertyName(self.name, prop)

    def propertyGroup(self):
        return PathSetupSheet.OpPropertyGroup(self.name)

    def accept(self):
        propertiesCreatedRemoved = False
        for i,name in enumerate(self.props):
            prop = self.prototype.getProperty(name)
            propName = self.propertyName(name)
            enabled = self.model.item(i, 0).checkState() == QtCore.Qt.Checked
            if enabled and not prop.getValue() is None:
                if prop.setupProperty(self.obj, propName, self.propertyGroup(), prop.getValue()):
                    propertiesCreatedRemoved = True
            else:
                if hasattr(self.obj, propName):
                    self.obj.removeProperty(propName)
                    propertiesCreatedRemoved = True
        return propertiesCreatedRemoved


class OpsDefaultEditor:

    def __init__(self, obj, parent):
        self.obj = obj
        self.ops = sorted([OpTaskPanel(self.obj, name, op) for name, op in PathUtil.keyValueIter(PathSetupSheet._RegisteredOps)], key = lambda op: op.name)
        if parent:
            self.toolbox = QtGui.QToolBox(parent)
            for op in self.ops:
                self.toolbox.addItem(op.form, op.form.windowTitle())
            self.toolbox.setParent(parent)
            parent.layout().addWidget(self.toolbox)

    def reject(self):
        pass

    def accept(self):
        if any([op.accept() for op in self.ops]):
            PathLog.track()
            #sel = FreeCADGui.Selection.getSelection()
            #FreeCADGui.Selection.clearSelection()
            #for o in sel:
            #    FreeCADGui.Selection.addSelection(o)

    def getFields(self):
        pass

    def updateUI(self):
        pass

    def updateModel(self, recomp = True):
        PathLog.track()
        self.getFields()
        self.updateUI()
        if recomp:
            FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.updateUI()

    def setupUi(self):
        for op in self.ops:
            op.setupUi()

class GlobalEditor(object):

    def __init__(self, obj, form):
        self.form = form
        self.obj = obj

    def reject(self):
        pass

    def accept(self):
        self.getFields()

    def getFields(self):
        def updateExpression(name, widget):
            value = str(widget.text())
            val = PathGui.getProperty(self.obj, name)
            if val != value:
                PathGui.setProperty(self.obj, name, value)

        updateExpression('StartDepthExpression',        self.form.setupStartDepthExpr)
        updateExpression('FinalDepthExpression',        self.form.setupFinalDepthExpr)
        updateExpression('StepDownExpression',          self.form.setupStepDownExpr)
        updateExpression('ClearanceHeightExpression',   self.form.setupClearanceHeightExpr)
        updateExpression('SafeHeightExpression',        self.form.setupSafeHeightExpr)
        self.clearanceHeightOffs.updateProperty()
        self.safeHeightOffs.updateProperty()
        self.rapidVertical.updateProperty()
        self.rapidHorizontal.updateProperty()

    def updateUI(self):
        PathLog.track()
        self.form.setupStartDepthExpr.setText(       self.obj.StartDepthExpression)
        self.form.setupFinalDepthExpr.setText(       self.obj.FinalDepthExpression)
        self.form.setupStepDownExpr.setText(         self.obj.StepDownExpression)
        self.form.setupClearanceHeightExpr.setText(  self.obj.ClearanceHeightExpression)
        self.form.setupSafeHeightExpr.setText(       self.obj.SafeHeightExpression)
        self.clearanceHeightOffs.updateSpinBox()
        self.safeHeightOffs.updateSpinBox()
        self.rapidVertical.updateSpinBox()
        self.rapidHorizontal.updateSpinBox()

    def updateModel(self, recomp = True):
        PathLog.track()
        self.getFields()
        self.updateUI()
        if recomp:
            FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.updateUI()

    def setupUi(self):
        self.clearanceHeightOffs = PathGui.QuantitySpinBox(self.form.setupClearanceHeightOffs, self.obj, 'ClearanceHeightOffset')
        self.safeHeightOffs = PathGui.QuantitySpinBox(self.form.setupSafeHeightOffs, self.obj, 'SafeHeightOffset')
        self.rapidHorizontal = PathGui.QuantitySpinBox(self.form.setupRapidHorizontal, self.obj, 'HorizRapid')
        self.rapidVertical = PathGui.QuantitySpinBox(self.form.setupRapidVertical, self.obj, 'VertRapid')
        self.setFields()

class TaskPanel:

    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        PathLog.track(self.obj.Label)
        self.globalForm = FreeCADGui.PySideUic.loadUi(":/panels/SetupGlobal.ui")
        self.globalEditor = GlobalEditor(self.obj, self.globalForm)
        self.opsEditor = OpsDefaultEditor(self.obj, None)
        self.form = [op.form for op in self.opsEditor.ops] + [self.globalForm]
        FreeCAD.ActiveDocument.openTransaction(translate("Path_SetupSheet", "Edit SetupSheet"))

    def reject(self):
        self.globalEditor.reject()
        self.opsEditor.reject()
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.globalEditor.accept()
        self.opsEditor.accept()

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        #FreeCADGui.Selection.removeObserver(self.s)
        #FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.globalEditor.getFields()
        self.opsEditor.getFields()

    def updateUI(self):
        self.globalEditor.updateUI()
        self.opsEditor.updateUI()

    def updateModel(self):
        self.globalEditor.updateModel(False)
        self.opsEditor.updateModel(False)
        FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.globalEditor.setFields()
        self.opsEditor.setFields()

    def setupUi(self):
        self.globalEditor.setupUi()
        self.opsEditor.setupUi()

def Create(name = 'SetupSheet'):
    '''Create(name = 'SetupSheet') ... creates a new setup sheet'''
    FreeCAD.ActiveDocument.openTransaction(translate("Path_Job", "Create Job"))
    ssheet = PathSetupSheet.Create(name)
    PathIconViewProvider.Attach(ssheet, name)
    return ssheet

PathIconViewProvider.RegisterViewProvider('SetupSheet', ViewProvider)

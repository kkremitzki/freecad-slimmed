# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_ViewProviderFemElementGeometry2D"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementGeometry2D
#  \ingroup FEM

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from PySide import QtCore
from . import FemSelectionWidgets


class _ViewProviderFemElementGeometry2D:
    "A View Provider for the FemElementGeometry2D object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-shell-thickness.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Standard")

    def getDisplayModes(self, obj):
        return ["Standard"]

    def getDefaultDisplayMode(self):
        return "Standard"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show task panel
        taskd = _TaskPanelFemElementGeometry2D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode, https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before open a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemElementGeometry2D:
    '''The TaskPanel for editing References property of FemElementGeometry2D objects'''

    def __init__(self, obj):

        # parameter widget
        self.obj = obj
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry2D.ui")
        self.init_parameter_widget()
        QtCore.QObject.connect(self.parameterWidget.if_thickness, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.thickness_changed)
        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.FacesSelection(obj.References)
        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.obj.Thickness = self.thickness
        self.obj.References = self.selectionWidget.references
        FreeCAD.ActiveDocument.recompute()
        self.set_back_all()
        return True

    def reject(self):
        self.set_back_all()
        return True

    def init_parameter_widget(self):
        self.thickness = self.obj.Thickness
        self.parameterWidget.if_thickness.setText(self.thickness.UserString)

    def thickness_changed(self, base_quantity_value):
        self.thickness = base_quantity_value

    def set_back_all(self):
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()

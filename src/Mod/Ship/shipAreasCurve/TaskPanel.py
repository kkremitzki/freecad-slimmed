#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Qt library
from PyQt4 import QtGui,QtCore
# Module
import Preview
import Instance
from shipUtils import Paths, Translator
from surfUtils import Geometry

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/shipAreasCurve/TaskPanel.ui"
        self.preview = Preview.Preview()
        self.ship = None

    def accept(self):
        if not self.ship:
            return False
        self.save()
        self.preview.clean()
        return True

    def reject(self):
        self.preview.clean()
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return True

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return False

    def helpRequested(self):
        pass

    def setupUi(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.draft = form.findChild(QtGui.QDoubleSpinBox, "Draft")
        form.trim = form.findChild(QtGui.QDoubleSpinBox, "Trim")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.draft, QtCore.SIGNAL("valueChanged(double)"), self.onData)
        QtCore.QObject.connect(form.trim, QtCore.SIGNAL("valueChanged(double)"), self.onData)
        QtCore.QObject.connect(form.findChild(QtGui.QPushButton, "UpdateButton"),QtCore.SIGNAL("pressed()"),self.onUpdate)

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.qApp.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    def initValues(self):
        """ Set initial values for fields
        """
        # Get objects
        selObjs  = Geometry.getSelectedObjs()
        if not selObjs:
            msg = Translator.translate("Ship instance must be selected (any object selected)\n")
            App.Console.PrintError(msg)
            return True
        for i in range(0,len(selObjs)):
            obj = selObjs[i]
            # Test if is a ship instance
            props = obj.PropertiesList
            try:
                props.index("IsShip")
            except ValueError:
                continue
            if obj.IsShip:
                # Test if another ship already selected
                if self.ship:
                    msg = Translator.translate("More than one ship selected (extra ship will be neglected)\n")
                    App.Console.PrintWarning(msg)
                    break
                self.ship = obj
        # Test if any valid ship was selected
        if not self.ship:
            msg = Translator.translate("Ship instance must be selected (any valid ship found at selected objects)\n")
            App.Console.PrintError(msg)
            return True
        # Get bounds
        bbox = self.ship.Shape.BoundBox
        self.form.draft.setMaximum(bbox.ZMax)
        self.form.draft.setMinimum(bbox.ZMin)
        self.form.draft.setValue(self.ship.Draft)
        # Try to use saved values
        props = self.ship.PropertiesList
        flag = True
        try:
            props.index("AreaCurveDraft")
        except ValueError:
            flag = False
        if flag:
            self.form.draft.setValue(self.ship.AreaCurveDraft)
        flag = True
        try:
            props.index("AreaCurveTrim")
        except ValueError:
            flag = False
        if flag:
            self.form.trim.setValue(self.ship.AreaCurveTrim)
        # Update GUI
        self.preview.update(self.form.draft.value(), self.form.trim.value(), self.ship)
        msg = Translator.translate("Ready to work\n")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Plot transversal areas curve"))
        self.form.findChild(QtGui.QLabel, "DraftLabel").setText(Translator.translate("Draft"))
        self.form.findChild(QtGui.QLabel, "TrimLabel").setText(Translator.translate("Trim"))
        self.form.findChild(QtGui.QPushButton, "UpdateButton").setText(Translator.translate("Update Data"))

    def onData(self, value):
        """ Method called when input data is changed.
         @param value Changed value.
        """
        if not self.ship:
            return
        self.preview.update(self.form.draft.value(), self.form.trim.value(), self.ship)

    def onUpdate(self):
        """ Method called when update data request.
        """
        if not self.ship:
            return

    def save(self):
        """ Saves data into ship instance.
        """
        props = self.ship.PropertiesList
        try:
            props.index("AreaCurveDraft")
        except ValueError:
            self.ship.addProperty("App::PropertyFloat","AreaCurveDraft","Ship", str(Translator.translate("Areas curve draft selected [m]")))
            self.ship.AreaCurveDraft = self.form.draft.value()
        try:
            props.index("AreaCurveTrim")
        except ValueError:
            self.ship.addProperty("App::PropertyFloat","AreaCurveTrim","Ship", str(Translator.translate("Areas curve trim selected [m]")))
            self.ship.AreaCurveTrim = self.form.draft.value()

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel

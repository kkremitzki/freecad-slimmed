#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

import ccxFrdReader
import FreeCAD
import FemGui
import os
import sys
import tempfile
import time

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from PySide.QtCore import Qt
    from PySide.QtGui import QApplication

__title__ = "Mechanical Analysis managment"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"


def makeMechanicalAnalysis(name):
    '''makeFemAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    _FemAnalysis(obj)
    _ViewProviderFemAnalysis(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandNewMechanicalAnalysis:
    "the Fem Analysis command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_Analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "New mechanical analysis"),
                'Accel': "N, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Create a new mechanical analysis")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MechanicalAnalysis")
        #FreeCADGui.doCommand("FreeCADGui.ActiveDocument.ActiveView.setAxisCross(True)")
        FreeCADGui.doCommand("MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                #FreeCADGui.doCommand("Gui.activeDocument().hide('" + sel[0].Name + "')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        #FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is None


class _CommandFemFromShape:
    def GetResources(self):
        return {'Pixmap': 'Fem_FemMesh',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh from shape")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MechanicalAnalysis")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            return sel[0].isDerivedFrom("Part::Feature")
        return False


class _CommandMechanicalJobControl:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_NewAnalysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Start calculation"),
                'Accel': "S, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Dialog to start the calculation of the mechanical anlysis")}

    def Activated(self):
        taskd = _JobControlTaskPanel(FemGui.getActiveAnalysis())
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _CommandPurgeFemResults:
    def GetResources(self):
        return {'Pixmap': 'Fem_Purge_Results',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results"),
                'Accel': "S, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results from an analysis")}

    def Activated(self):
        purge_fem_results()
        reset_mesh_color()
        reset_mesh_deformation()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_Result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement", "Show result"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement", "Show result information of an analysis")}

    def Activated(self):
        DisplacementObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultVector"):
                if i.DataType == 'Displacement':
                    DisplacementObject = i
        StressObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'VonMisesStress':
                    StressObject = i

        if not DisplacementObject and not StressObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No result found in active Analysis")
            return

        taskd = _ResultControlTaskPanel(FemGui.getActiveAnalysis())
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


class _FemAnalysis:
    "The Material object"
    def __init__(self, obj):
        self.Type = "FemAnalysis"
        obj.Proxy = self
        #obj.Material = StartMat
        obj.addProperty("App::PropertyString", "OutputDir", "Base", "Directory where the jobs get generated")

    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class _ViewProviderFemAnalysis:
    "A View Provider for the Material object"

    def __init__(self, vobj):
        #vobj.addProperty("App::PropertyLength", "BubbleSize", "Base", str(translate("Fem", "The size of the axis bubbles")))
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Fem_Analysis.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        if not FemGui.getActiveAnalysis() == self.Object:
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench':
                FreeCADGui.activateWorkbench("FemWorkbench")
            FemGui.setActiveAnalysis(self.Object)
            return True
        else:
            taskd = _JobControlTaskPanel(self.Object)
            FreeCADGui.Control.showDialog(taskd)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _JobControlTaskPanel:
    '''The editmode TaskPanel for Material objects'''
    def __init__(self, object):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/MechanicalAnalysis.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        ccx_binary = self.fem_prefs.GetString("ccxBinaryPath", "")
        if ccx_binary:
            self.CalculixBinary = ccx_binary
            print "Using ccx binary path from FEM preferences: {}".format(ccx_binary)
        else:
            from platform import system
            if system() == 'Linux':
                self.CalculixBinary = 'ccx'
            elif system() == 'Windows':
                self.CalculixBinary = FreeCAD.getHomePath() + 'bin/ccx.exe'
            else:
                self.CalculixBinary = 'ccx'
        self.TempDir = FreeCAD.ActiveDocument.TransientDir.replace('\\', '/') + '/FemAnl_' + object.Uid[-4:]
        if not os.path.isdir(self.TempDir):
            os.mkdir(self.TempDir)

        self.obj = object
        #self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        self.Calculix = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)

        self.fem_console_message = ''

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.toolButton_chooseOutputDir, QtCore.SIGNAL("clicked()"), self.chooseOutputDir)
        QtCore.QObject.connect(self.form.pushButton_write, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        QtCore.QObject.connect(self.form.pushButton_edit, QtCore.SIGNAL("clicked()"), self.editCalculixInputFile)
        QtCore.QObject.connect(self.form.pushButton_generate, QtCore.SIGNAL("clicked()"), self.runCalculix)

        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("started()"), self.calculixStarted)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.calculixStateChanged)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("error(QProcess::ProcessError)"), self.calculixError)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("finished(int)"), self.calculixFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()

    def femConsoleMessage(self, message="", color="#000000"):
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.textEdit_Output.setText(self.fem_console_message)
        self.form.textEdit_Output.moveCursor(QtGui.QTextCursor.End)

    def printCalculiXstdout(self):
        #There is probably no need to show user output from CalculiX. It should be
        #written to a file in the calcs directory and shown to user upon request [BUTTON]
        out = self.Calculix.readAllStandardOutput()
        if out.isEmpty():
            self.femConsoleMessage("CalculiX stdout is empty", "#FF0000")
        else:
            try:
                out = unicode(out, 'utf-8', 'replace')
                rx = QtCore.QRegExp("\\*ERROR.*\\n\\n")
                rx.setMinimal(True)
                pos = rx.indexIn(out)
                while not pos < 0:
                    match = rx.cap(0)
                    FreeCAD.Console.PrintError(match.strip().replace('\n', ' ') + '\n')
                    pos = rx.indexIn(out, pos + 1)
                out = os.linesep.join([s for s in out.splitlines() if s])
                self.femConsoleMessage(out.replace('\n', '<br>'))
            except UnicodeDecodeError:
                self.femConsoleMessage("Error converting stdout from CalculiX", "#FF0000")

    def UpdateText(self):
        if(self.Calculix.state() == QtCore.QProcess.ProcessState.Running):
            self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def calculixError(self, error):
        print "Error()", error
        self.femConsoleMessage("CalculiX execute error: {}".format(error), "#FF0000")

    def calculixStarted(self):
        print "calculixStarted()"
        print self.Calculix.state()
        self.form.pushButton_generate.setText("Break Calculix")

    def calculixStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.femConsoleMessage("Starting CalculiX...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.femConsoleMessage("CalculiX is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.femConsoleMessage("CalculiX stopped.")

    def calculixFinished(self, exitCode):
        print "calculixFinished()", exitCode
        print self.Calculix.state()

        # Restore previous cwd
        QtCore.QDir.setCurrent(self.cwd)

        self.printCalculiXstdout()
        self.Timer.stop()

        self.femConsoleMessage("Calculix done!", "#00AA00")

        self.form.pushButton_generate.setText("Re-run Calculix")
        print "Loading results...."
        self.femConsoleMessage("Loading result sets...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        purge_fem_results()
        reset_mesh_color()
        reset_mesh_deformation()
        if os.path.isfile(self.base_name + '.frd'):
            QApplication.setOverrideCursor(Qt.WaitCursor)
            ccxFrdReader.importFrd(self.base_name + '.frd', FemGui.getActiveAnalysis())
            QApplication.restoreOverrideCursor()
            self.femConsoleMessage("Loading results done!", "#00AA00")
        else:
            self.femConsoleMessage("Loading results failed! Results file doesn\'t exist", "#FF0000")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the widgets'
        self.form.lineEdit_outputDir.setText(tempfile.gettempdir())
        return

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

    def chooseOutputDir(self):
        print "chooseOutputDir"
        dirname = QtGui.QFileDialog.getExistingDirectory(None, 'Choose material directory', self.params.GetString("JobDir", '/'))
        if(dirname):
            self.params.SetString("JobDir", str(dirname))
            self.form.lineEdit_outputDir.setText(dirname)

    def write_input_file_handler(self):
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            try:
                import ccxInpWriter as iw
                inp_writer = iw.inp_writer(self.TempDir, self.MeshObject, self.MaterialObjects,
                                           self.FixedObjects, self.ForceObjects, self.PressureObjects)
                self.base_name = inp_writer.write_calculix_input_file()
                if self.base_name != "":
                    self.femConsoleMessage("Write completed.")
                else:
                    self.femConsoleMessage("Write .inp file failed!", "#FF0000")
            except:
                print "Unexpected error when writing CalculiX input file:", sys.exc_info()[0]
                raise
            finally:
                QApplication.restoreOverrideCursor()
            if self.base_name:
                self.form.pushButton_edit.setEnabled(True)
                self.form.pushButton_generate.setEnabled(True)

    def check_prerequisites(self):
        self.Start = time.time()
        self.femConsoleMessage("Check dependencies...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.MeshObject = None
        # [{'Object':MaterialObject}, {}, ...]
        self.MaterialObjects = []
        # [{'Object':FixedObject, 'NodeSupports':bool}, {}, ...]
        self.FixedObjects = []
        # [{'Object':ForceObject, 'NodeLoad':value}, {}, ...
        self.ForceObjects = []
        # [{'Object':PressureObject, 'xxxxxxxx':value}, {}, ...]
        self.PressureObjects = []
        if not FemGui.getActiveAnalysis():
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No active Analysis")
            return False

        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                self.MeshObject = i
            elif i.isDerivedFrom("App::MaterialObjectPython"):
                MaterialObjectDict = {}
                MaterialObjectDict['Object'] = i
                self.MaterialObjects.append(MaterialObjectDict)
            elif i.isDerivedFrom("Fem::ConstraintFixed"):
                FixedObjectDict = {}
                FixedObjectDict['Object'] = i
                self.FixedObjects.append(FixedObjectDict)
            elif i.isDerivedFrom("Fem::ConstraintForce"):
                ForceObjectDict = {}
                ForceObjectDict['Object'] = i
                self.ForceObjects.append(ForceObjectDict)
            elif i.isDerivedFrom("Fem::ConstraintPressure"):
                PressureObjectDict = {}
                PressureObjectDict['Object'] = i
                self.PressureObjects.append(PressureObjectDict)

        if not self.MeshObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No mesh object in the Analysis")
            return False

        if not self.MaterialObjects:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No material object in the Analysis")
            return False

        if not self.FixedObjects:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No fixed-constraint nodes defined in the Analysis")
            return False

        if not (self.ForceObjects or self.PressureObjects):
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No force-constraint or pressure-constraint defined in the Analysis")
            return False
        return True

    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])

    def editCalculixInputFile(self):
        filename = self.base_name + '.inp'
        print 'editCalculixInputFile {}'.format(filename)
        if self.fem_prefs.GetBool("UseInternalEditor", True):
            FemGui.open(filename)
        else:
            ext_editor_path = self.fem_prefs.GetString("ExternalEditorPath", "")
            if ext_editor_path:
                self.start_ext_editor(ext_editor_path, filename)
            else:
                print "External editor is not defined in FEM preferences. Falling back to internal editor"
                FemGui.open(filename)

    def runCalculix(self):
        print 'runCalculix'
        self.Start = time.time()

        self.femConsoleMessage("CalculiX binary: {}".format(self.CalculixBinary))
        self.femConsoleMessage("Run Calculix...")

        # run Calculix
        print 'run Calculix at: ', self.CalculixBinary, '  with: ', self.base_name
        # change cwd because ccx may crash if directory has no write permission
        # there is also a limit of the length of file names so jump to the document directory
        self.cwd = QtCore.QDir.currentPath()
        fi = QtCore.QFileInfo(self.base_name)
        QtCore.QDir.setCurrent(fi.path())
        self.Calculix.start(self.CalculixBinary, ['-i', fi.baseName()])

        QApplication.restoreOverrideCursor()


class _ResultControlTaskPanel:
    '''The control for the displacement post-processing'''
    def __init__(self, object):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/ShowDisplacement.ui")

        self.obj = object

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.rb_none, QtCore.SIGNAL("toggled(bool)"), self.none_selected)
        QtCore.QObject.connect(self.form.rb_x_displacement, QtCore.SIGNAL("toggled(bool)"), self.x_displacement_selected)
        QtCore.QObject.connect(self.form.rb_y_displacement, QtCore.SIGNAL("toggled(bool)"), self.y_displacement_selected)
        QtCore.QObject.connect(self.form.rb_z_displacement, QtCore.SIGNAL("toggled(bool)"), self.z_displacement_selected)
        QtCore.QObject.connect(self.form.rb_abs_displacement, QtCore.SIGNAL("toggled(bool)"), self.abs_displacement_selected)
        QtCore.QObject.connect(self.form.rb_vm_stress, QtCore.SIGNAL("toggled(bool)"), self.vm_stress_selected)

        QtCore.QObject.connect(self.form.cb_show_displacement, QtCore.SIGNAL("clicked(bool)"), self.show_displacement)
        QtCore.QObject.connect(self.form.hsb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.hsb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor_max, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_max_changed)

        self.DisplacementObject = None
        self.StressObject = None
        self.update()
        self.restore_result_dialog()

    def restore_result_dialog(self):
        try:
            rt = FreeCAD.FEM_dialog["results_type"]
            if rt == "None":
                self.form.rb_none.setChecked(True)
                self.none_selected(True)
            elif rt == "Uabs":
                self.form.rb_abs_displacement.setChecked(True)
                self.abs_displacement_selected(True)
            elif rt == "U1":
                self.form.rb_x_displacement.setChecked(True)
                self.x_displacement_selected(True)
            elif rt == "U2":
                self.form.rb_y_displacement.setChecked(True)
                self.y_displacement_selected(True)
            elif rt == "U3":
                self.form.rb_z_displacement.setChecked(True)
                self.z_displacement_selected(True)
            elif rt == "Sabs":
                self.form.rb_vm_stress.setChecked(True)
                self.vm_stress_selected(True)

            sd = FreeCAD.FEM_dialog["show_disp"]
            self.form.cb_show_displacement.setChecked(sd)
            self.show_displacement(sd)

            df = FreeCAD.FEM_dialog["disp_factor"]
            dfm = FreeCAD.FEM_dialog["disp_factor_max"]
            self.form.hsb_displacement_factor.setMaximum(dfm)
            self.form.hsb_displacement_factor.setValue(df)
            self.form.sb_displacement_factor_max.setValue(dfm)
            self.form.sb_displacement_factor.setValue(df)
        except:
            FreeCAD.FEM_dialog = {"results_type": "None", "show_disp": False,
                                  "disp_factor": 0, "disp_factor_max": 100}

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def get_result_stats(self, type_name, analysis=None):
        if analysis is None:
            analysis = FemGui.getActiveAnalysis()
        for i in analysis.Member:
            if i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'AnalysisStats':
                    match_table = {"U1": (i.Values[0], i.Values[1], i.Values[2]),
                                   "U2": (i.Values[3], i.Values[4], i.Values[5]),
                                   "U3": (i.Values[6], i.Values[7], i.Values[8]),
                                   "Uabs": (i.Values[9], i.Values[10], i.Values[11]),
                                   "Sabs": (i.Values[12], i.Values[13], i.Values[14]),
                                   "None": (0.0, 0.0, 0.0)}
                    return match_table[type_name]
        return (0.0, 0.0, 0.0)

    def none_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.set_result_stats("mm", 0.0, 0.0, 0.0)
        reset_mesh_color()

    def abs_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Uabs"
        self.select_displacement_type("Uabs")

    def x_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U1"
        self.select_displacement_type("U1")

    def y_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U2"
        self.select_displacement_type("U2")

    def z_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U3"
        self.select_displacement_type("U3")

    def vm_stress_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Sabs"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        (minm, avg, maxm) = (0.0, 0.0, 0.0)
        if self.StressObject:
            self.MeshObject.ViewObject.setNodeColorByResult(self.StressObject)
            (minm, avg, maxm) = self.get_result_stats("Sabs")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def select_displacement_type(self, disp_type):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        (minm, avg, maxm) = (0.0, 0.0, 0.0)
        if self.DisplacementObject:
            match = {"Uabs": 0, "U1": 1, "U2": 2, "U3": 3}
            self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject, match[disp_type])
            (minm, avg, maxm) = self.get_result_stats(disp_type)
        self.set_result_stats("mm", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def set_result_stats(self, unit, minm, avg, maxm):
        self.form.le_min.setProperty("unit", unit)
        self.form.le_min.setText("{:.6} {}".format(minm, unit))
        self.form.le_avg.setProperty("unit", unit)
        self.form.le_avg.setText("{:.6} {}".format(avg, unit))
        self.form.le_max.setProperty("unit", unit)
        self.form.le_max.setText("{:.6} {}".format(maxm, unit))

    def show_displacement(self, checked):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        FreeCAD.FEM_dialog["show_disp"] = checked
        factor = 0.0
        if checked:
            factor = self.form.hsb_displacement_factor.value()
        self.MeshObject.ViewObject.setNodeDisplacementByResult(self.DisplacementObject)
        self.MeshObject.ViewObject.applyDisplacement(factor)
        QtGui.qApp.restoreOverrideCursor()

    def hsb_disp_factor_changed(self, value):
        self.MeshObject.ViewObject.applyDisplacement(value)
        self.form.sb_displacement_factor.setValue(value)

    def sb_disp_factor_max_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor_max"] = value
        self.form.hsb_displacement_factor.setMaximum(value)

    def sb_disp_factor_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor"] = value
        self.form.hsb_displacement_factor.setValue(value)

    def update(self):
        self.MeshObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                self.MeshObject = i
            elif i.isDerivedFrom("Fem::FemResultVector"):
                if i.DataType == 'Displacement':
                    self.DisplacementObject = i
            elif i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'VonMisesStress':
                    self.StressObject = i

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

# Helpers


def results_present():
    results = False
    analysis_members = FemGui.getActiveAnalysis().Member
    for o in analysis_members:
        if o.isDerivedFrom('Fem::FemResultVector'):
            results = True
        elif o.isDerivedFrom("Fem::FemResultValue") and o.DataType == 'VonMisesStress':
            results = True
    return results


def purge_fem_results(Analysis=None):
    if Analysis is None:
        analysis_members = FemGui.getActiveAnalysis().Member
    else:
        analysis_members = FemGui.Analysis().Member
    for o in analysis_members:
        if (o.isDerivedFrom('Fem::FemResultVector') or
           (o.isDerivedFrom("Fem::FemResultValue") and o.DataType == 'VonMisesStress') or
           (o.isDerivedFrom("Fem::FemResultValue") and o.DataType == 'AnalysisStats')):
            FreeCAD.ActiveDocument.removeObject(o.Name)


def reset_mesh_color(mesh=None):
    if mesh is None:
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                mesh = i
    mesh.ViewObject.NodeColor = {}
    mesh.ViewObject.ElementColor = {}
    mesh.ViewObject.setNodeColorByResult()


def reset_mesh_deformation(mesh=None):
    if mesh is None:
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                mesh = i
    mesh.ViewObject.applyDisplacement(0.0)

FreeCADGui.addCommand('Fem_NewMechanicalAnalysis', _CommandNewMechanicalAnalysis())
FreeCADGui.addCommand('Fem_CreateFromShape', _CommandFemFromShape())
FreeCADGui.addCommand('Fem_MechanicalJobControl', _CommandMechanicalJobControl())
FreeCADGui.addCommand('Fem_PurgeResults', _CommandPurgeFemResults())
FreeCADGui.addCommand('Fem_ShowResult', _CommandMechanicalShowResult())

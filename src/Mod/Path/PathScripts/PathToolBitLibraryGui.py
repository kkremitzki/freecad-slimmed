# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathToolBit as PathToolBit
import PathScripts.PathToolBitGui as PathToolBitGui
import PathScripts.PathToolBitEdit as PathToolBitEdit
import PathScripts.PathToolControllerGui as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
from PySide import QtCore, QtGui
import PySide
import json
import os
import glob
import traceback
import uuid as UUID
from functools import partial

# PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())

_UuidRole = PySide.QtCore.Qt.UserRole + 1
_PathRole = PySide.QtCore.Qt.UserRole + 2


def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)


class _TableView(PySide.QtGui.QTableView):
    '''Subclass of QTableView to support rearrange and copying of ToolBits'''

    def __init__(self, parent):
        PySide.QtGui.QTableView.__init__(self, parent)
        self.setDragEnabled(False)
        self.setAcceptDrops(False)
        self.setDropIndicatorShown(False)
        self.setDragDropMode(PySide.QtGui.QAbstractItemView.DragOnly)
        self.setDefaultDropAction(PySide.QtCore.Qt.IgnoreAction)
        self.setSortingEnabled(True)
        self.setSelectionBehavior(PySide.QtGui.QAbstractItemView.SelectRows)
        self.verticalHeader().hide()

    def supportedDropActions(self):
        return [PySide.QtCore.Qt.CopyAction, PySide.QtCore.Qt.MoveAction]

    def _uuidOfRow(self, row):
        model = self.toolModel()
        return model.data(model.index(row, 0), _UuidRole)

    def _rowWithUuid(self, uuid):
        model = self.toolModel()
        for row in range(model.rowCount()):
            if self._uuidOfRow(row) == uuid:
                return row
        return None

    def _copyTool(self, uuid_, dstRow):
        model = self.toolModel()
        model.insertRow(dstRow)
        srcRow = self._rowWithUuid(uuid_)
        for col in range(model.columnCount()):
            srcItem = model.item(srcRow, col)

            model.setData(model.index(dstRow, col), srcItem.data(PySide.QtCore.Qt.EditRole), PySide.QtCore.Qt.EditRole)
            if col == 0:
                model.setData(model.index(dstRow, col), srcItem.data(_PathRole), _PathRole)
                # Even a clone of a tool gets its own uuid so it can be identified when
                # rearranging the order or inserting/deleting rows
                model.setData(model.index(dstRow, col), UUID.uuid4(), _UuidRole)
            else:
                model.item(dstRow, col).setEditable(False)

    def _copyTools(self, uuids, dst):
        for i, uuid in enumerate(uuids):
            self._copyTool(uuid, dst + i)

    def dropEvent(self, event):
        PathLog.track()
        mime = event.mimeData()
        data = mime.data('application/x-qstandarditemmodeldatalist')
        stream = PySide.QtCore.QDataStream(data)
        srcRows = []
        while not stream.atEnd():
            # pylint: disable=unused-variable
            row = stream.readInt32()
            srcRows.append(row)

        # get the uuids of all srcRows
        model = self.toolModel()
        srcUuids = [self._uuidOfRow(row) for row in set(srcRows)]
        destRow = self.rowAt(event.pos().y())

        self._copyTools(srcUuids, destRow)
        if PySide.QtCore.Qt.DropAction.MoveAction == event.proposedAction():
            for uuid in srcUuids:
                model.removeRow(self._rowWithUuid(uuid))


class ModelFactory(object):
    ''' Helper class to generate qtdata models for toolbit libraries
    '''

    def __init__(self, path=None):
        PathLog.track()
        self.path = ""
        # self.currentLib = ""


    def __libraryLoad(self, path, datamodel):
        PathLog.track(path)
        PathPreferences.setLastFileToolLibrary(path)
        # self.currenLib = path

        with open(path) as fp:
            library = json.load(fp)

        for toolBit in library['tools']:
            nr = toolBit['nr']
            bit = PathToolBit.findBit(toolBit['path'])
            if bit:
                PathLog.track(bit)
                tool = PathToolBit.Declaration(bit)
                datamodel.appendRow(self._toolAdd(nr, tool, bit))
            else:
                PathLog.error("Could not find tool #{}: {}".format(nr, toolBit['path']))

    def _toolAdd(self, nr, tool, path):

        strShape = os.path.splitext(os.path.basename(tool['shape']))[0]
        strDiam = tool['parameter']['Diameter']
        tooltip = "{}: {}".format(strShape, strDiam)

        toolNr = PySide.QtGui.QStandardItem()
        toolNr.setData(nr, PySide.QtCore.Qt.EditRole)
        toolNr.setToolTip(tool['shape'])
        toolNr.setData(path, _PathRole)
        toolNr.setData(UUID.uuid4(), _UuidRole)
        toolNr.setToolTip(tooltip)

        toolName = PySide.QtGui.QStandardItem()
        toolName.setData(tool['name'], PySide.QtCore.Qt.EditRole)
        toolName.setEditable(False)
        toolName.setToolTip(tooltip)

        toolShape = PySide.QtGui.QStandardItem()
        toolShape.setData(strShape, PySide.QtCore.Qt.EditRole)
        toolShape.setEditable(False)

        toolDiameter = PySide.QtGui.QStandardItem()
        toolDiameter.setData(strDiam, PySide.QtCore.Qt.EditRole)
        toolDiameter.setEditable(False)

        return [toolNr, toolName, toolShape, toolDiameter]

    def newTool(self, datamodel, path):
        '''
        Adds a toolbit item to a model
        '''
        PathLog.track()

        try:
            nr = 0
            for row in range(datamodel.rowCount()):
                itemNr = int(datamodel.item(row, 0).data(PySide.QtCore.Qt.EditRole))
                nr = max(nr, itemNr)
            nr += 1
            tool = PathToolBit.Declaration(path)
        except Exception:
            PathLog.error(traceback.print_exc())

        datamodel.appendRow(self._toolAdd(nr, tool, path))

    def findLibraries(self, model):
        '''
        Finds all the fctl files in a location
        Returns a QStandardItemModel
        '''
        PathLog.track()
        path = PathPreferences.lastPathToolLibrary()

        if os.path.isdir(path):  # opening all tables in a directory
            libFiles = [f for f in glob.glob(path + '/*.fctl')]
            libFiles.sort()
            for libFile in libFiles:
                loc, fnlong = os.path.split(libFile)
                fn, ext = os.path.splitext(fnlong)
                libItem = QtGui.QStandardItem(fn)
                libItem.setToolTip(loc)
                libItem.setData(libFile, _PathRole)
                libItem.setIcon(QtGui.QPixmap(':/icons/Path-ToolTable.svg'))
                model.appendRow(libItem)

        PathLog.debug('model rows: {}'.format(model.rowCount()))
        return model

    def libraryOpen(self, model, lib=""):
        '''
        opens the tools in library
        Returns a QStandardItemModel
        '''
        PathLog.track(lib)

        if lib == "":
            lib = PathPreferences.lastFileToolLibrary()

        if os.path.isfile(lib):  # An individual library is wanted
            self.__libraryLoad(lib, model)

        PathLog.debug('model rows: {}'.format(model.rowCount()))
        return model


class ToolBitSelector(object):
    '''Controller for displaying a library and creating ToolControllers'''

    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/ToolBitSelector.ui')
        self.factory = ModelFactory()
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.setupUI()
        self.title = self.form.windowTitle()

    def columnNames(self):
        return ['#', 'Tool']

    def curLib(self):
        libfile = os.path.split(PathPreferences.lastFileToolLibrary())[1]
        libName = os.path.splitext(libfile)[0]
        return libName

    def loadData(self):
        PathLog.track()
        self.toolModel.clear()
        self.toolModel.setHorizontalHeaderLabels(self.columnNames())
        self.form.lblLibrary.setText(self.curLib())
        self.factory.libraryOpen(self.toolModel)
        self.toolModel.takeColumn(3)
        self.toolModel.takeColumn(2)

    def setupUI(self):
        PathLog.track()
        self.loadData()

        self.form.tools.setModel(self.toolModel)
        self.form.tools.selectionModel().selectionChanged.connect(self.enableButtons)
        self.form.tools.doubleClicked.connect(partial(self.selectedOrAllToolControllers))
        self.form.libraryEditorOpen.clicked.connect(self.libraryEditorOpen)
        self.form.addToolController.clicked.connect(self.selectedOrAllToolControllers)

    def enableButtons(self):
        selected = (len(self.form.tools.selectedIndexes()) >= 1)
        if selected:
            jobs = len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"]) >= 1
        self.form.addToolController.setEnabled(selected and jobs)

    def libraryEditorOpen(self):
        library = ToolBitLibrary()
        library.open()
        self.loadData()

    def selectedOrAllTools(self):
        '''
        Iterate the selection and add individual tools
        If a group is selected, iterate and add children
        '''

        itemsToProcess = []
        for index in self.form.tools.selectedIndexes():
            item = index.model().itemFromIndex(index)

            if item.hasChildren():
                for i in range(item.rowCount()-1):
                    if item.child(i).column() == 0:
                        itemsToProcess.append(item.child(i))

            elif item.column() == 0:
                itemsToProcess.append(item)

        tools = []
        for item in itemsToProcess:
            toolNr = int(item.data(PySide.QtCore.Qt.EditRole))
            toolPath = item.data(_PathRole)
            tools.append((toolNr, PathToolBit.Factory.CreateFrom(toolPath)))
        return tools

    def selectedOrAllToolControllers(self, index=None):
        '''
        if no jobs, don't do anything, otherwise all TCs for all
        selected toolbits
        '''
        jobs = PathUtilsGui.PathUtils.GetJobs()
        if len(jobs) == 0:
            return
        elif len(jobs) == 1:
            job = jobs[0]
        else:
            userinput = PathUtilsGui.PathUtilsUserInput()
            job = userinput.chooseJob(jobs)

        if job is None:  # user may have canceled
            return

        tools = self.selectedOrAllTools()

        for tool in tools:
            tc = PathToolControllerGui.Create(tool[1].Label, tool[1], tool[0])
            job.Proxy.addToolController(tc)
            FreeCAD.ActiveDocument.recompute()

    def open(self, path=None):
        ''' load library stored in path and bring up ui'''
        docs = FreeCADGui.getMainWindow().findChildren(QtGui.QDockWidget)
        for doc in docs:
            if doc.objectName() == "ToolSelector":
                if doc.isVisible():
                    doc.deleteLater()
                    return
                else:
                    doc.setVisible(True)

        mw = FreeCADGui.getMainWindow()
        mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.form,
                         PySide.QtCore.Qt.Orientation.Vertical)


class ToolBitLibrary(object):
    '''ToolBitLibrary is the controller for
    displaying/selecting/creating/editing a collection of ToolBits.'''

    def __init__(self):
        PathLog.track()
        self.factory = ModelFactory()
        self.temptool = None
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.listModel = PySide.QtGui.QStandardItemModel()
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/ToolBitLibraryEdit.ui')
        self.toolTableView = _TableView(self.form.toolTableGroup)
        self.form.toolTableGroup.layout().replaceWidget(self.form.toolTable, self.toolTableView)
        self.form.toolTable.hide()
        self.setupUI()
        self.title = self.form.windowTitle()

    def toolBitNew(self):
        PathLog.track()

        # select the shape file
        shapefile = PathToolBitGui.GetToolShapeFile()
        if shapefile is None:  # user canceled
            return

        filename = PathToolBitGui.GetNewToolFile()
        if filename == None:
            return

        # Parse out the name of the file and write the structure
        loc, fil = os.path.split(filename)
        fname = os.path.splitext(fil)[0]
        fullpath = "{}/{}.fctb".format(loc, fname)
        PathLog.debug(fullpath)

        self.temptool = PathToolBit.ToolBitFactory().Create(name=fname)
        self.temptool.BitShape = shapefile
        self.temptool.Proxy.unloadBitBody(self.temptool)
        self.temptool.Label = fname
        self.temptool.Proxy.saveToFile(self.temptool, fullpath)
        self.temptool.Document.removeObject(self.temptool.Name)
        self.temptool = None

        # add it to the model
        self.factory.newTool(self.toolModel, fullpath)

    def toolBitExisting(self):

        filenames = PathToolBitGui.GetToolFiles()

        if len(filenames) == 0:
            return

        for f in filenames:

            loc, fil = os.path.split(f)
            fname = os.path.splitext(fil)[0]
            fullpath = "{}/{}.fctb".format(loc, fname)

            self.factory.newTool(self.toolModel, fullpath)

    def toolDelete(self):
        PathLog.track()
        selectedRows = set([index.row() for index in self.toolTableView.selectedIndexes()])
        for row in sorted(list(selectedRows), key=lambda r: -r):
            self.toolModel.removeRows(row, 1)

    def toolSelect(self, selected, deselected):
        sel = len(self.toolTableView.selectedIndexes()) > 0
        self.form.toolDelete.setEnabled(sel)

    def tableSelected(self, index):
        ''' loads the tools for the selected tool table '''
        PathLog.track()
        item = index.model().itemFromIndex(index)
        libpath = item.data(_PathRole)
        self.loadData(libpath)
        self.path = libpath

    def open(self):
        PathLog.track()
        return self.form.exec_()

    def libraryPath(self):
        PathLog.track()
        path = PySide.QtGui.QFileDialog.getExistingDirectory(self.form, 'Tool Library Path', PathPreferences.lastPathToolLibrary())
        if len(path) == 0:
            return

        PathPreferences.setLastPathToolLibrary(path)
        self.loadData()

    def cleanupDocument(self):
        # This feels like a hack.  Remove the toolbit object
        # remove the editor from the dialog
        # re-enable all the controls
        self.temptool.Proxy.unloadBitBody(self.temptool)
        self.temptool.Document.removeObject(self.temptool.Name)
        self.temptool = None
        widget = self.form.toolTableGroup.children()[-1]
        widget.setParent(None)
        self.editor = None
        self.lockoff()

    def accept(self):
        self.temptool.Proxy.saveToFile(self.temptool, self.temptool.File)
        self.loadData()
        self.cleanupDocument()

    def reject(self):
        self.cleanupDocument()

    def lockon(self):
        self.toolTableView.setEnabled(False)
        self.form.toolCreate.setEnabled(False)
        self.form.toolDelete.setEnabled(False)
        self.form.toolAdd.setEnabled(False)
        self.form.TableList.setEnabled(False)
        self.form.libraryOpen.setEnabled(False)
        self.form.libraryExport.setEnabled(False)
        self.form.addToolTable.setEnabled(False)
        self.form.librarySave.setEnabled(False)

    def lockoff(self):
        self.toolTableView.setEnabled(True)
        self.form.toolCreate.setEnabled(True)
        self.form.toolDelete.setEnabled(True)
        self.form.toolAdd.setEnabled(True)
        self.form.toolTable.setEnabled(True)
        self.form.TableList.setEnabled(True)
        self.form.libraryOpen.setEnabled(True)
        self.form.libraryExport.setEnabled(True)
        self.form.addToolTable.setEnabled(True)
        self.form.librarySave.setEnabled(True)

    def toolEdit(self, selected):
        item = self.toolModel.item(selected.row(), 0)

        if self.temptool is not None:
            self.temptool.Document.removeObject(self.temptool.Name)

        if selected.column() == 0:  # editing Nr
            pass
        else:
            tbpath = item.data(_PathRole)
            self.temptool = PathToolBit.ToolBitFactory().CreateFrom(tbpath, 'temptool')
            self.editor = PathToolBitEdit.ToolBitEditor(self.temptool, self.form.toolTableGroup, loadBitBody=False)

            QBtn = QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel
            buttonBox = QtGui.QDialogButtonBox(QBtn)
            buttonBox.accepted.connect(self.accept)
            buttonBox.rejected.connect(self.reject)

            layout = self.editor.form.layout()
            layout.addWidget(buttonBox)
            self.lockon()
            self.editor.setupUI()

    def toolEditDone(self, success=True):
        FreeCAD.ActiveDocument.removeObject("temptool")
        print('all done')

    def libraryNew(self):
        TooltableTypeJSON = translate("PathToolLibraryManager", "Tooltable JSON (*.fctl)")

        filename = PySide.QtGui.QFileDialog.getSaveFileName(self.form,
                translate("TooltableEditor", "Save toolbit library", None),
                PathPreferences.lastPathToolLibrary(), "{}".format(TooltableTypeJSON))

        if not (filename and filename[0]):
            self.loadData()

        path = filename[0] if filename[0].endswith('.fctl') else "{}.fctl".format(filename[0])
        library = {}
        tools = []
        library['version'] = 1
        library['tools'] = tools
        with open(path, 'w') as fp:
            json.dump(library, fp, sort_keys=True, indent=2)

        self.loadData()

    def librarySave(self):
        library = {}
        tools = []
        library['version'] = 1
        library['tools'] = tools
        for row in range(self.toolModel.rowCount()):
            toolNr = self.toolModel.data(self.toolModel.index(row, 0), PySide.QtCore.Qt.EditRole)
            toolPath = self.toolModel.data(self.toolModel.index(row, 0), _PathRole)
            if PathPreferences.toolsStoreAbsolutePaths():
                tools.append({'nr': toolNr, 'path': toolPath})
            else:
                tools.append({'nr': toolNr, 'path': PathToolBit.findRelativePathTool(toolPath)})

        with open(self.path, 'w') as fp:
            json.dump(library, fp, sort_keys=True, indent=2)

    def libraryOk(self):
        self.librarySave()
        self.form.close()

    def libPaths(self):
        lib = PathPreferences.lastFileToolLibrary()
        loc = PathPreferences.lastPathToolLibrary()
        #loc = os.path.split(lib)[0]

        PathLog.track("lib: {} loc: {}".format(lib, loc))
        return lib, loc

    def columnNames(self):
        return ['Nr', 'Tool', 'Shape', 'Diameter']

    def loadData(self, path=None):
        PathLog.track(path)
        self.toolTableView.setUpdatesEnabled(False)
        self.form.TableList.setUpdatesEnabled(False)


        if path is None:
            path, loc = self.libPaths()

            self.toolModel.clear()
            self.listModel.clear()
            self.factory.libraryOpen(self.toolModel, lib=path)
            self.factory.findLibraries(self.listModel)

        else:
            self.toolModel.clear()
            self.factory.libraryOpen(self.toolModel, lib=path)

        self.path = path
        self.form.setWindowTitle("{}".format(PathPreferences.lastPathToolLibrary()))
        self.toolModel.setHorizontalHeaderLabels(self.columnNames())
        self.listModel.setHorizontalHeaderLabels(['Library'])


        # Select the current library in the list of tables
        curIndex = None
        for i in range(self.listModel.rowCount()):
            item = self.listModel.item(i)
            if item.data(_PathRole) == path:
                curIndex = self.listModel.indexFromItem(item)

        if curIndex:
            sm = self.form.TableList.selectionModel()
            sm.select(curIndex, QtCore.QItemSelectionModel.Select)

        self.toolTableView.setUpdatesEnabled(True)
        self.form.TableList.setUpdatesEnabled(True)

    def setupUI(self):
        PathLog.track()
        self.form.TableList.setModel(self.listModel)
        self.toolTableView.setModel(self.toolModel)

        self.loadData()

        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)
        self.toolTableView.doubleClicked.connect(self.toolEdit)

        self.form.TableList.clicked.connect(self.tableSelected)

        self.form.toolAdd.clicked.connect(self.toolBitExisting)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolCreate.clicked.connect(self.toolBitNew)

        self.form.addToolTable.clicked.connect(self.libraryNew)

        self.form.libraryOpen.clicked.connect(self.libraryPath)
        self.form.librarySave.clicked.connect(self.libraryOk)
        self.form.libraryExport.clicked.connect(self.librarySaveAs)

        self.toolSelect([], [])

    def librarySaveAs(self, path):

        TooltableTypeJSON = translate("PathToolLibraryManager", "Tooltable JSON (*.fctl)")
        TooltableTypeLinuxCNC = translate("PathToolLibraryManager", "LinuxCNC tooltable (*.tbl)")

        filename = PySide.QtGui.QFileDialog.getSaveFileName(self.form,
                translate("TooltableEditor", "Save toolbit library", None),
                PathPreferences.lastPathToolLibrary(), "{};;{}".format(TooltableTypeJSON,
                    TooltableTypeLinuxCNC))
        if filename and filename[0]:
            if filename[1] == TooltableTypeLinuxCNC:
                path = filename[0] if filename[0].endswith('.tbl') else "{}.tbl".format(filename[0])
                self.libararySaveLinuxCNC(path)
            else:
                path = filename[0] if filename[0].endswith('.fctl') else "{}.fctl".format(filename[0])
                self.path = path
                self.librarySave()
                self.updateToolbar()

    def libararySaveLinuxCNC(self, path):
        # linuxcnc line template
        LIN = "T{} P{} X{} Y{} Z{} A{} B{} C{} U{} V{} W{} D{} I{} J{} Q{}; {}"
        with open(path, 'w') as fp:
            fp.write(";\n")

            for row in range(self.toolModel.rowCount()):
                toolNr = self.toolModel.data(self.toolModel.index(row, 0), PySide.QtCore.Qt.EditRole)
                toolPath = self.toolModel.data(self.toolModel.index(row, 0), _PathRole)

                bit = PathToolBit.Factory.CreateFrom(toolPath)
                if bit:
                    PathLog.track(bit)

                    pocket = bit.Pocket if hasattr(bit, "Pocket") else ""
                    xoffset = bit.Xoffset if hasattr(bit, "Xoffset") else "0"
                    yoffset = bit.Yoffset if hasattr(bit, "Yoffset") else "0"
                    zoffset = bit.Zoffset if hasattr(bit, "Zoffset") else "0"
                    aoffset = bit.Aoffset if hasattr(bit, "Aoffset") else "0"
                    boffset = bit.Boffset if hasattr(bit, "Boffset") else "0"
                    coffset = bit.Coffset if hasattr(bit, "Coffset") else "0"
                    uoffset = bit.Uoffset if hasattr(bit, "Uoffset") else "0"
                    voffset = bit.Voffset if hasattr(bit, "Voffset") else "0"
                    woffset = bit.Woffset if hasattr(bit, "Woffset") else "0"

                    diameter = bit.Diameter.getUserPreferred()[0].split()[0] if hasattr(bit, "Diameter") else "0"
                    frontangle = bit.FrontAngle if hasattr(bit, "FrontAngle") else "0"
                    backangle = bit.BackAngle if hasattr(bit, "BackAngle") else "0"
                    orientation = bit.Orientation if hasattr(bit, "Orientation") else "0"
                    remark = bit.Label

                    fp.write(LIN.format(toolNr, pocket, xoffset, yoffset,
                             zoffset, aoffset, boffset, coffset, uoffset,
                             voffset, woffset, diameter, frontangle, backangle,
                             orientation, remark) + "\n")

                    FreeCAD.ActiveDocument.removeObject(bit.Name)

                else:
                    PathLog.error("Could not find tool #{} ".format(toolNr))

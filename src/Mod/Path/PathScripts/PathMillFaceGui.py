# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathAreaOpGui as PathAreaOpGui
import PathScripts.PathMillFace as PathMillFace
import PathScripts.PathPocketBaseGui as PathPocketBaseGui

from PySide import QtCore

class TaskPanelOpPage(PathPocketBaseGui.TaskPanelOpPage):

    def pocketFeatures(self):
        return PathPocketBaseGui.FeatureFacing

PathAreaOpGui.SetupOperation('MillFace',
        PathMillFace.Create,
        TaskPanelOpPage,
        'Path-Face',
        QtCore.QT_TRANSLATE_NOOP("PathFace", "Face"),
        "P, O",
        QtCore.QT_TRANSLATE_NOOP("PathFace", "Create a Facing Operation from a model or face"))

FreeCAD.Console.PrintLog("Loading PathMillFaceGui... done\n")


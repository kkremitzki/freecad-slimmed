# -*- coding: utf-8 -*-
# ***************************************************************************
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

import Path
import subprocess

LOGLEVEL = False

if LOGLEVEL:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

Processed = False


def Startup():
    global Processed
    if not Processed:
        Path.Log.debug("Initializing PathGui")
        from Path.Op.Gui import Adaptive
        from Path.Dressup.Gui import AxisMap
        from Path.Dressup.Gui import Dogbone
        from Path.Dressup.Gui import Dragknife
        from Path.Dressup.Gui import LeadInOut
        from Path.Dressup.Gui import PathBoundary
        from Path.Dressup.Gui import RampEntry
        from Path.Dressup.Gui import Tags
        from Path.Dressup.Gui import ZCorrect
        from Path.Op.Gui import Custom
        from Path.Op.Gui import Deburr
        from Path.Op.Gui import Drilling
        from Path.Op.Gui import Engrave
        from Path.Op.Gui import Helix
        from Path.Op.Gui import MillFace
        from Path.Op.Gui import Pocket
        from Path.Op.Gui import PocketShape
        from Path.Op.Gui import Probe
        from Path.Op.Gui import Profile
        from Path.Op.Gui import Slot
        from Path.Op.Gui import ThreadMilling
        from Path.Op.Gui import Vcarve
        from Path.Post import Command
        from Path.Tools import Controller
        from Path.Tools.Gui import Controller
        from PathScripts import PathArray
        from PathScripts import PathComment
        from PathScripts import PathFixture
        from PathScripts import PathHop
        from PathScripts import PathInspect
        from PathScripts import PathPropertyBagGui
        from PathScripts import PathSanity
        from PathScripts import PathSetupSheetGui
        from PathScripts import PathSimpleCopy
        from PathScripts import PathSimulatorGui
        from PathScripts import PathStop
        from PathScripts import PathToolLibraryEditor
        from PathScripts import PathToolLibraryManager
        from PathScripts import PathUtilsGui

        from packaging.version import Version, parse

        # If camotics is installed and current enough, import the GUI
        try:
            import camotics

            r = subprocess.run(
                ["camotics", "--version"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            ).stderr.strip()

            v = parse(r.decode("utf-8"))
            if v >= Version("1.2.2"):
                from PathScripts import PathCamoticsGui
        except (FileNotFoundError, ModuleNotFoundError):
            pass

        Processed = True
    else:
        Path.Log.debug("Skipping PathGui initialisation")

# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic shopinthewoods@gmail.com                 *
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

__title__ = "Path Machine State"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Dataclass to implement a machinestate tracker"
__contributors__ = ""

import PathScripts.PathLog as PathLog
import FreeCAD
from dataclasses import dataclass, field

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


@dataclass
class MachineState:
    WCSLIST = [
        "G53",
        "G54",
        "G55",
        "G56",
        "G57",
        "G58",
        "G59",
        "G59.1",
        "G59.2",
        "G59.3",
        "G59.4",
        "G59.5",
        "G59.6",
        "G59.7",
        "G59.8",
        "G59.9",
    ]

    X: float = field(default=None)
    Y: float = field(default=None)
    Z: float = field(default=None)
    A: float = field(default=None)
    B: float = field(default=None)
    C: float = field(default=None)
    F: float = field(default=None)
    Coolant: bool = field(default=False)
    WCS: str = field(default="G54")
    Spindle: str = field(default="off")
    S: int = field(default=0)
    T: str = field(default=None)

    def addCommand(self, command):
        if command.Name == "M6":
            self.T = command.Parameters["T"]
            return

        if command.Name in ["M3", "M4"]:
            self.S = command.Parameters["S"]
            self.SpindApple = "CW" if command.Name == "M3" else "CCW"
            return

        if command.Name in ["M2", "M5"]:
            self.S = 0
            self.Spindle = "off"
            return

        if command.Name in self.WCSLIST:
            self.WCS = command.Name
            return

        for p in command.Parameters:
            self.__setattr__(p, command.Parameters[p])

    def getPosition(self):
        """
        Returns an App.Vector of the current location
        """
        x = 0 if self.X is None else self.X
        y = 0 if self.Y is None else self.Y
        z = 0 if self.Z is None else self.Z

        return FreeCAD.Vector(x, y, z)


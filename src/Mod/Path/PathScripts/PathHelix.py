# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

from Generators import helix_generator
from PathScripts.PathUtils import fmt
from PathScripts.PathUtils import sort_locations
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Part
import Path
import PathScripts.PathCircularHoleBase as PathCircularHoleBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp

__title__ = "Path Helix Drill Operation"
__author__ = "Lorenz Hüdepohl"
__url__ = "https://www.freecadweb.org"
__doc__ = "Class and implementation of Helix Drill operation"
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2016"
__scriptVersion__ = "1b testing"
__lastModified__ = "2019-07-12 09:50 CST"


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


class ObjectHelix(PathCircularHoleBase.ObjectOp):
    """Proxy class for Helix operations."""

    @classmethod
    def helixOpPropertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("Path_Helix", "CW"), "CW"),
                (translate("Path_Helix", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "StartSide": [
                (translate("PathProfile", "Outside"), "Outside"),
                (translate("PathProfile", "Inside"), "Inside"),
            ],  # side of profile that cutter is on in relation to direction of profile
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        PathLog.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        PathLog.debug(data)

        return data

    def circularHoleFeatures(self, obj):
        """circularHoleFeatures(obj) ... enable features supported by Helix."""
        return (
            PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces
        )

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... create helix specific properties."""
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The direction of the circular cuts, ClockWise (CW), or CounterClockWise (CCW)",
            ),
        )
        # obj.Direction = ["CW", "CCW"]

        obj.addProperty(
            "App::PropertyEnumeration",
            "StartSide",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Start cutting from the inside or outside"
            ),
        )
        # obj.StartSide = ["Inside", "Outside"]

        obj.addProperty(
            "App::PropertyPercent",
            "StepOver",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property", "Percent of cutter diameter to step over on each pass"
            ),
        )
        obj.addProperty(
            "App::PropertyLength",
            "StartRadius",
            "Helix Drill",
            QT_TRANSLATE_NOOP("App::Property", "Starting Radius"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetExtra",
            "Helix Drill",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra value to stay away from final profile- good for roughing toolpath",
            ),
        )

        ENUMS = self.helixOpPropertyEnumerations()
        for n in ENUMS:
            setattr(obj, n[0], n[1])

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "StartRadius"):
            obj.addProperty(
                "App::PropertyLength",
                "StartRadius",
                "Helix Drill",
                QT_TRANSLATE_NOOP("App::Property", "Starting Radius"),
            )

        if not hasattr(obj, "OffsetExtra"):
            obj.addProperty(
                "App::PropertyDistance",
                "OffsetExtra",
                "Helix Drill",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra value to stay away from final profile- good for roughing toolpath",
                ),
            )
    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... generate helix commands for each hole in holes"""
        PathLog.track()
        self.commandlist.append(Path.Command("(helix cut operation)"))

        self.commandlist.append(
            Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
        )

        zsafe = (
            max(baseobj.Shape.BoundBox.ZMax for baseobj, features in obj.Base)
            + obj.ClearanceHeight.Value
        )
        output = ""
        output += "G0 Z" + fmt(zsafe)

        holes = sort_locations(holes, ["x", "y"])

        tool = obj.ToolController.Tool
        tooldiamter = (
            tool.Diameter.Value if hasattr(tool.Diameter, "Value") else tool.Diameter
        )

        args = {
            "edge": None,
            "hole_radius": None,
            "step_down": obj.StepDown.Value,
            "step_over": obj.StepOver / 100,
            "tool_diameter": tooldiamter,
            "inner_radius": obj.StartRadius.Value + obj.OffsetExtra.Value,
            "direction": obj.Direction,
            "startAt": obj.StartSide,
        }

        for hole in holes:
            args["hole_radius"] = (hole["r"] / 2) - (obj.OffsetExtra.Value)
            startPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.StartDepth.Value)
            endPoint = FreeCAD.Vector(hole["x"], hole["y"], obj.FinalDepth.Value)
            args["edge"] = Part.makeLine(startPoint, endPoint)

            results = helix_generator.generate(**args)

            for command in results:
                self.commandlist.append(command)

            # output += self.helix_cut(
            #     obj,
            #     hole["x"],
            #     hole["y"],
            #     hole["r"] / 2,
            #     float(obj.StartRadius.Value),
            #     (float(obj.StepOver.Value) / 50.0) * self.radius,
            # )
        PathLog.debug(output)

    # def helix_cut(self, obj, x0, y0, r_out, r_in, dr):
    #     '''helix_cut(obj, x0, y0, r_out, r_in, dr) ... generate helix commands for specified hole.
    #         x0, y0: coordinates of center
    #         r_out, r_in: outer and inner radius of the hole
    #         dr: step over radius value'''
    #     from numpy import ceil, linspace

    #     if (obj.StartDepth.Value <= obj.FinalDepth.Value):
    #         return ""

    #     out = "(helix_cut <{0}, {1}>, {2})".format(
    #         x0, y0, ", ".join(map(str, (r_out, r_in, dr, obj.StartDepth.Value,
    #                                     obj.FinalDepth.Value, obj.StepDown.Value, obj.SafeHeight.Value,
    #                                     self.radius, self.vertFeed, self.horizFeed, obj.Direction, obj.StartSide))))

    #     nz = max(int(ceil((obj.StartDepth.Value - obj.FinalDepth.Value) / obj.StepDown.Value)), 2)
    #     zi = linspace(obj.StartDepth.Value, obj.FinalDepth.Value, 2 * nz + 1)

    #     def xyz(x=None, y=None, z=None):
    #         out = ""
    #         if x is not None:
    #             out += " X" + fmt(x)
    #         if y is not None:
    #             out += " Y" + fmt(y)
    #         if z is not None:
    #             out += " Z" + fmt(z)
    #         return out

    #     def rapid(x=None, y=None, z=None):
    #         return "G0" + xyz(x, y, z) + "\n"

    #     def F(f=None):
    #         return (" F" + fmt(f) if f else "")

    #     def feed(x=None, y=None, z=None, f=None):
    #         return "G1" + xyz(x, y, z) + F(f) + "\n"

    #     def arc(x, y, i, j, z, f):
    #         if obj.Direction == "CW":
    #             code = "G2"
    #         elif obj.Direction == "CCW":
    #             code = "G3"
    #         return code + " I" + fmt(i) + " J" + fmt(j) + " X" + fmt(x) + " Y" + fmt(y) + " Z" + fmt(z) + F(f) + "\n"

    #     def helix_cut_r(r):
    #         arc_cmd = 'G2' if obj.Direction == 'CW' else 'G3'
    #         out = ""
    #         out += rapid(x=x0 + r, y=y0)
    #         self.commandlist.append(Path.Command('G0', {'X': x0 + r, 'Y': y0, 'F': self.horizRapid}))
    #         out += rapid(z=obj.StartDepth.Value + 2 * self.radius)
    #         self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
    #         out += feed(z=obj.StartDepth.Value, f=self.vertFeed)
    #         self.commandlist.append(Path.Command('G1', {'Z': obj.StartDepth.Value, 'F': self.vertFeed}))
    #         # z = obj.FinalDepth.Value
    #         for i in range(1, nz + 1):
    #             out += arc(x0 - r, y0, i=-r, j=0.0, z=zi[2 * i - 1], f=self.horizFeed)
    #             self.commandlist.append(Path.Command(arc_cmd, {'X': x0 - r, 'Y': y0, 'Z': zi[2 * i - 1], 'I': -r, 'J': 0.0, 'F': self.horizFeed}))
    #             out += arc(x0 + r, y0, i=r, j=0.0, z=zi[2 * i], f=self.horizFeed)
    #             self.commandlist.append(Path.Command(arc_cmd, {'X': x0 + r, 'Y': y0, 'Z': zi[2 * i], 'I': r, 'J': 0.0, 'F': self.horizFeed}))
    #         out += arc(x0 - r, y0, i=-r, j=0.0, z=obj.FinalDepth.Value, f=self.horizFeed)
    #         self.commandlist.append(Path.Command(arc_cmd, {'X': x0 - r, 'Y': y0, 'Z': obj.FinalDepth.Value, 'I': -r, 'J': 0.0, 'F': self.horizFeed}))
    #         out += arc(x0 + r, y0, i=r, j=0.0, z=obj.FinalDepth.Value, f=self.horizFeed)
    #         self.commandlist.append(Path.Command(arc_cmd, {'X': x0 + r, 'Y': y0, 'Z': obj.FinalDepth.Value, 'I': r, 'J': 0.0, 'F': self.horizFeed}))
    #         out += feed(z=obj.StartDepth.Value + 2 * self.radius, f=self.vertFeed)
    #         out += rapid(z=obj.SafeHeight.Value)
    #         self.commandlist.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
    #         return out

    #     msg = None
    #     if r_out < 0.0:
    #         msg = "r_out < 0"
    #     elif r_in > 0 and r_out - r_in < 2 * self.radius:
    #         msg = "r_out - r_in = {0} is < tool diameter of {1}".format(r_out - r_in, 2 * self.radius)
    #     elif r_in == 0.0 and not r_out > self.radius / 2.:
    #         msg = "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, 2 * self.radius)
    #     elif obj.StartSide not in ["Inside", "Outside"]:
    #         msg = "Invalid value for parameter 'obj.StartSide'"
    #     elif r_in > 0:
    #         out += "(annulus mode)\n"
    #         r_out = r_out - self.radius
    #         r_in = r_in + self.radius
    #         if abs((r_out - r_in) / dr) < 1e-5:
    #             radii = [(r_out + r_in) / 2]
    #         else:
    #             nr = max(int(ceil((r_out - r_in) / dr)), 2)
    #             radii = linspace(r_out, r_in, nr)
    #     elif r_out <= 2 * dr:
    #         out += "(single helix mode)\n"
    #         radii = [r_out - self.radius]
    #         if radii[0] <= 0:
    #             msg = "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, 2 * self.radius)
    #     else:
    #         out += "(full hole mode)\n"
    #         r_out = r_out - self.radius
    #         r_in = dr / 2

    #         nr = max(1 + int(ceil((r_out - r_in) / dr)), 2)
    #         radii = [r for r in linspace(r_out, r_in, nr) if r > 0]
    #         if not radii:
    #             msg = "Cannot helix a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, 2 * self.radius)

    #     if msg:
    #         out += "(ERROR: Hole at {0}: ".format((x0, y0, obj.StartDepth.Value)) + msg + ")\n"
    #         PathLog.error("{0} - ".format((x0, y0, obj.StartDepth.Value)) + msg)
    #         return out

    #     if obj.StartSide == "Inside":
    #         radii = radii[::-1]

    #     for r in radii:
    #         out += "(radius {0})\n".format(r)
    #         out += helix_cut_r(r)

    #     return out

    # def opSetDefaultValues(self, obj, job):
    #     obj.Direction = "CW"
    #     obj.StartSide = "Inside"
    #     obj.StepOver = 100


def SetupProperties():
    setup = []
    setup.append("Direction")
    setup.append("StartSide")
    setup.append("StepOver")
    setup.append("StartRadius")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Helix operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectHelix(obj, name, parentJob)
    if obj.Proxy:
        obj.Proxy.findAllHoles(obj)
    return obj

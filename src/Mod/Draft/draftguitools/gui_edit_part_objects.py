# ***************************************************************************
# *   Copyright (c) 2019 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides support functions to edit Part objects."""
## @package gui_edit_part_objects
# \ingroup draftguitools
# \brief Provides support functions to edit Part objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecadweb.org"

## \addtogroup draftguitools
# @{
import FreeCAD as App
import DraftVecUtils

def get_supported_part_objects():
    return ["Part", "Part::Line", "Part::Box", 
            "Part::Sphere", "Part::Cylinder", "Part::Cone"
            ]

# PART::LINE--------------------------------------------------------------

def getPartLinePts(obj):
    editpoints = []
    editpoints.append(App.Vector(obj.X1,obj.Y1,obj.Z1))
    editpoints.append(App.Vector(obj.X2,obj.Y2,obj.Z2))
    return editpoints

def updatePartLine(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.X1 = v.x
        obj.Y1 = v.y
        obj.Z1 = v.z
    elif nodeIndex == 1:
        obj.X2 = v.x
        obj.Y2 = v.y
        obj.Z2 = v.z


# PART::BOX---------------------------------------------------------------

def getPartBoxPts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.Length, 0, 0))
    editpoints.append(App.Vector(0, obj.Width, 0))
    editpoints.append(App.Vector(0, 0, obj.Height))
    return editpoints

def updatePartBox(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.Base + v
    elif nodeIndex == 1:
        _vector = DraftVecUtils.project(v, App.Vector(1, 0, 0))
        obj.Length = _vector.Length
    elif nodeIndex == 2:
        _vector = DraftVecUtils.project(v, App.Vector(0, 1, 0))
        obj.Width = _vector.Length
    elif nodeIndex == 3:
        _vector = DraftVecUtils.project(v, App.Vector(0, 0, 1))
        obj.Height = _vector.Length

# Part::Cylinder --------------------------------------------------------------

def getPartCylinderPts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.Radius, 0, 0))
    editpoints.append(App.Vector(0, 0, obj.Height))
    return editpoints

def updatePartCylinder(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.Base + v
    elif nodeIndex == 1:
        if v.Length > 0.0:
            obj.Radius = v.Length
    elif nodeIndex == 2:
        _vector = DraftVecUtils.project(v, App.Vector(0, 0, 1))
        obj.Height = _vector.Length


# Part::Cone --------------------------------------------------------------

def getPartConePts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.Radius1, 0, 0))
    editpoints.append(App.Vector(obj.Radius2, 0, obj.Height))
    editpoints.append(App.Vector(0, 0, obj.Height))
    return editpoints

def updatePartCone(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.Base + v
    elif nodeIndex == 1:
        obj.Radius1 = v.Length # TODO: Perhaps better to project on the face?
    elif nodeIndex == 2:
        v.z = 0
        obj.Radius2 = v.Length # TODO: Perhaps better to project on the face?
    elif nodeIndex == 3: # Height is last to have the priority on the radius
        _vector = DraftVecUtils.project(v, App.Vector(0, 0, 1))
        obj.Height = _vector.Length


# Part::Sphere --------------------------------------------------------------

def getPartSpherePts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.Radius, 0, 0))
    return editpoints

def updatePartSphere(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.Base + v
    elif nodeIndex == 1:
        if v.Length > 0.0:
            obj.Radius = v.Length # TODO: Perhaps better to project on the face?

## @}

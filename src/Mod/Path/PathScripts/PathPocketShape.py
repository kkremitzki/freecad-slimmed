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
import Part
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathPocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils
import TechDraw
import sys

from PathScripts.PathGeom import PathGeom
from PySide import QtCore

__title__ = "Path Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of shape based Pocket operation."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPocket(PathPocketBase.ObjectPocket):
    '''Proxy object for Pocket operation.'''

    def initPocketOp(self, obj):
        '''initPocketOp(obj) ... setup receiver'''
        pass

    def pocketInvertExtraOffset(self):
        return False

    def combineConnectedShapes(self, shapes):
        done = False
        while not done:
            done = True
            combined = []
            for shape in shapes:
                connected = [f for f in combined if PathGeom.isRoughly(shape.distToShape(f)[0], 0,0)]
                if connected:
                    combined = [f for f in combined if f not in connected]
                    connected.append(shape)
                    combined.append(Part.makeCompound(connected))
                    done = False
                else:
                    combined.append(shape)
            shapes = combined
        return shapes

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return shapes representing the solids to be removed.'''
        PathLog.track()

        if obj.Base:
            PathLog.debug("base items exist.  Processing...")
            removalshapes = []
            horizontal = []
            vertical = []
            for o in obj.Base:
                PathLog.debug("Base item: {}".format(o))
                base = o[0]
                for sub in o[1]:
                    if "Face" in sub:
                        face = base.Shape.getElement(sub)
                        if type(face.Surface) == Part.Plane and PathGeom.isVertical(face.Surface.Axis):
                            # it's a flat horizontal face
                            horizontal.append(face)
                        elif type(face.Surface) == Part.Cylinder and PathGeom.isVertical(face.Surface.Axis):
                            # vertical cylinder wall
                            if any(e.isClosed() for e in face.Edges):
                                # complete cylinder
                                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                                disk = Part.Face(Part.Wire(circle))
                                horizontal.append(disk)
                            else:
                                # partial cylinder wall
                                vertical.append(face)
                        elif type(face.Surface) == Part.Plane and PathGeom.isHorizontal(face.Surface.Axis):
                            vertical.append(face)
                        else:
                            PathLog.error(translate('PathPocket', "Pocket does not support shape %s.%s") % (base.Label, sub))

            vertical = self.combineConnectedShapes(vertical)
            vWires = [TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1)) for shape in vertical]
            for wire in vWires:
                face = Part.Face(wire)
                if PathGeom.isRoughly(face.Area, 0):
                    PathLog.error(translate('PathPocket', 'Vertical faces do not form a loop - ignoring'))
                else:
                    horizontal.append(face)


            # move all horizontal faces to FinalDepth
            for face in horizontal:
                face.translate(FreeCAD.Vector(0, 0, obj.FinalDepth.Value - face.BoundBox.ZMin))

            # check all faces and see if they are touching/overlapping and combine those into a compound
            horizontal = self.combineConnectedShapes(horizontal)

            # extrude all faces up to StartDepth and those are the removal shapes
            extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value)
            removalshapes = [(face.extrude(extent), False) for face in horizontal]

        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")
            self.outline = Part.Face(TechDraw.findShapeOutline(self.baseobject.Shape, 1, FreeCAD.Vector(0, 0, 1)))
            stockBB = self.stock.Shape.BoundBox

            self.outline.translate(FreeCAD.Vector(0, 0, stockBB.ZMin - 1))
            self.body  = self.outline.extrude(FreeCAD.Vector(0, 0, stockBB.ZLength + 2))
            obj.removalshape = self.stock.Shape.cut(self.body)
            obj.removalshape.tessellate(0.1)
            removalshapes = [(obj.removalshape, False)]
        return removalshapes

    def areaOpSetDefaultValues(self, obj):
        '''areaOpSetDefaultValues(obj) ... set default values'''
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        job = PathUtils.findParentJob(obj)
        if job and job.Stock:
            bb = job.Stock.Shape.BoundBox
            obj.FinalDepth = bb.ZMin
            obj.StartDepth = bb.ZMax

    def areaOpOnChanged(self, obj, prop):
        if 'Base' == prop and obj.Base and not 'Restore' in obj.State:
            PathLog.track(obj.Label, prop)
            zmin = -sys.maxint
            zmax = zmin
            for base, sublist in obj.Base:
                bb = base.Shape.BoundBox  # parent boundbox
                for  sub in sublist:
                    subobj = base.Shape.getElement(sub)
                    fbb = subobj.BoundBox  # feature boundbox

                    if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                        finalDepth = bb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
                        finalDepth = fbb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
                        finalDepth = fbb.ZMin
                    elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                        finalDepth = fbb.ZMin
                    else:  # catch all
                        finalDepth = bb.ZMin

                    zmin = max(zmin, finalDepth)
                    zmax = max(zmax, bb.ZMax)
                    PathLog.debug("%s: final=%.2f, max=%.2f" % (sub, zmin, zmax))

            PathLog.debug("zmin=%.2f, zmax=%.2f" % (zmin, zmax))
            if not PathGeom.isRoughly(zmin, obj.FinalDepth.Value):
                obj.FinalDepth = zmin
            if not PathGeom.isRoughly(zmax, obj.StartDepth.Value):
                obj.StartDepth = zmax
            clearance = zmax + 5.0
            safe = zmax + 3
            if not PathGeom.isRoughly(clearance, obj.ClearanceHeight.Value):
                obj.CearanceHeight = clearance
            if not PathGeom.isRoughly(safe, obj.SafeHeight.Value):
                obj.SafeHeight = safe

        if prop in ['Base', 'Stock'] and not obj.Base and not 'Restore' in obj.State:
            job = PathUtils.findParentJob(obj)
            if job and job.Stock:
                bb = job.Stock.Shape.BoundBox
                obj.FinalDepth = bb.ZMin
                obj.StartDepth = bb.ZMax


def Create(name):
    '''Create(name) ... Creates and returns a Pocket operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectPocket(obj)
    return obj

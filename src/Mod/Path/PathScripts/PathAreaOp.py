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
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

from PathScripts.PathUtils import depth_params
from PathScripts.PathUtils import makeWorkplane
from PathScripts.PathUtils import waiting_effects
from PySide import QtCore

__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

FeatureTool         = 0x0001
FeatureDepths       = 0x0002
FeatureHeights      = 0x0004
FeatureStartPoint   = 0x0008
FeatureFinishDepth  = 0x0010
FeatureBaseFaces    = 0x1001
FeatureBaseEdges    = 0x1002
FeatureBasePanels   = 0x1002

FeatureBaseGeometry = FeatureBaseFaces | FeatureBaseEdges | FeatureBasePanels

class ObjectOp(object):

    def __init__(self, obj):
        PathLog.track()

        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Operation"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        if FeatureBaseGeometry & self.opFeatures(obj):
            obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"))

        if FeatureTool & self.opFeatures(obj):
            obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        if FeatureDepths & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"))
            obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
            obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))

        if FeatureFinishDepth & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum material removed on final pass."))

        if FeatureHeights & self.opFeatures(obj):
            obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
            obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))

        if FeatureStartPoint & self.opFeatures(obj):
            obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
            obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

        # Debugging
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode('PathParams', 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode('removalshape', 2)  # hide

        self.initOperation(obj)
        obj.Proxy = self
        self.setDefaultValues(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def opFeatures(self, obj):
        return FeatureTool | FeatureDepths | FeatureHeights | FeatureStartPoint | FeatureBaseGeometry | FeatureFinishDepth
    def opOnChanged(self, obj, prop):
        pass
    def opSetDefaultValues(self, obj):
        pass
    def opShapeForDepths(self, obj):
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            PathLog.debug("job=%s base=%s shape=%s" % (job, job.Base, job.Base.Shape))
            return job.Base.Shape
        if job:
            PathLog.warning(translate("PathAreaOp", "job %s has no Base.") % job.Label)
        else:
            PathLog.warning(translate("PathAreaOp", "no job for op %s found.") % obj.Label)
        return None

     
    def onChanged(self, obj, prop):
        #PathLog.track(obj.Label, prop)
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

        if FeatureBaseGeometry & self.opFeatures(obj):
            if prop == 'Base' and len(obj.Base) == 1:
                try:
                    (base, sub) = obj.Base[0]
                    bb = base.Shape.BoundBox  # parent boundbox
                    subobj = base.Shape.getElement(sub[0])
                    fbb = subobj.BoundBox  # feature boundbox
                    obj.StartDepth = bb.ZMax
                    obj.ClearanceHeight = bb.ZMax + 5.0
                    obj.SafeHeight = bb.ZMax + 3.0

                    if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                        obj.FinalDepth = bb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
                        obj.FinalDepth = fbb.ZMin
                    elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
                        obj.FinalDepth = fbb.ZMin
                    elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                        obj.FinalDepth = fbb.ZMin
                    else:  # catch all
                        obj.FinalDepth = bb.ZMin

                    if hasattr(obj, 'Side'):
                        if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                            obj.Side = "Outside"
                        else:
                            obj.Side = "Inside"

                except Exception as e:
                    PathLog.error(translate("PatArea", "Error in calculating depths: %s" % e))
                    obj.StartDepth = 5.0
                    obj.ClearanceHeight = 10.0
                    obj.SafeHeight = 8.0
                    if hasattr(obj, 'Side'):
                        obj.Side = "Outside"

        self.opOnChanged(obj, prop)

    def setDefaultValues(self, obj):
        PathUtils.addToJob(obj)

        obj.Active = True

        if FeatureTool & self.opFeatures(obj):
            obj.ToolController = PathUtils.findToolController(obj)

        if FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.opShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.StartDepth      = bb.ZMax
                obj.FinalDepth      = bb.ZMin
                obj.StepDown        = 1.0
            else:
                obj.StartDepth      =  1.0
                obj.FinalDepth      =  0.0
                obj.StepDown        =  1.0

        if FeatureHeights & self.opFeatures(obj):
            try:
                shape = self.opShapeForDepths(obj)
            except:
                shape = None

            if shape:
                bb = shape.BoundBox
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight      = bb.ZMax + 3.0
            else:
                obj.ClearanceHeight = 10.0
                obj.SafeHeight      =  8.0

        if FeatureStartPoint & self.opFeatures(obj):
            obj.UseStartPoint = False

        self.opSetDefaultValues(obj)

    @waiting_effects
    def _buildPathArea(self, obj, baseobject, isHole, start, getsim):
        PathLog.track()
        area = Path.Area()
        area.setPlane(makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.opAreaParams(obj, isHole)

        heights = [i for i in self.depthparams]
        PathLog.debug('depths: {}'.format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(mode=0, project=self.opUseProjection(obj), heights=heights)
        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        pathParams = self.opPathParams(obj, isHole)
        pathParams['shapes'] = shapelist
        pathParams['feedrate'] = self.horizFeed
        pathParams['feedrate_v'] = self.vertFeed
        pathParams['verbose'] = True
        pathParams['resume_height'] = obj.StepDown.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True

        if self.endVector is not None:
            pathParams['start'] = self.endVector
        elif obj.UseStartPoint:
            pathParams['start'] = obj.StartPoint

        obj.PathParams = str({key: value for key, value in pathParams.items() if key != 'shapes'})
        PathLog.debug("Path with params: {}".format(obj.PathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug('pp: {}, end vector: {}'.format(pp, end_vector))
        self.endVector = end_vector

        simobj = None
        if getsim:
            areaParams['Thicken'] = True
            areaParams['ToolRadius'] = self.radius - self.radius * .005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0, project=False, heights=heights)[-1].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0, 0, baseobject.BoundBox.ZMax))

        return pp, simobj

    def execute(self, obj, getsim=False):
        PathLog.track()
        self.endVector = None

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            if obj.ViewObject:
                obj.ViewObject.Visibility = False
            return

        self.depthparams = depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:

            FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
            return
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                return
            else:
                self.radius = tool.Diameter/2

        if FeatureStartPoint and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = FreeCAD.Vector()

        commandlist = []
        commandlist.append(Path.Command("(" + obj.Label + ")"))

        shapes = self.opShapes(obj, commandlist)

        sims = []
        for (shape, isHole) in shapes:
            try:
                (pp, sim) = self._buildPathArea(obj, shape, isHole, start, getsim)
                commandlist.extend(pp.Commands)
                sims.append(sim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Check project and tool config.")


        # Let's finish by rapid to clearance...just for safety
        commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        PathLog.track()
        path = Path.Path(commandlist)
        obj.Path = path
        return sims

    def addBase(self, obj, base, sub=""):
        PathLog.track()
        baselist = obj.Base
        if baselist is None:
            baselist = []
        item = (base, sub)
        if item in baselist:
            PathLog.warning(translate("Path", "this object already in the list" + "\n"))
        else:
            baselist.append(item)
            obj.Base = baselist

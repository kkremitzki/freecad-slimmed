#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009, 2010                                              *  
#*   Yorik van Havre <yorik@uncreated.net>, Ken Cline <cline@frii.com>     *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

__title__="FreeCAD Draft Workbench GUI Tools"
__author__ = "Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, Dmitry Chigrin"
__url__ = "http://free-cad.sourceforge.net"

#---------------------------------------------------------------------------
# Generic stuff
#---------------------------------------------------------------------------

import os, FreeCAD, FreeCADGui, Part, WorkingPlane, math, re, importSVG, Draft, Draft_rc
from functools import partial
from draftlibs import fcvec,fcgeo
from FreeCAD import Vector
from draftGui import todo,QtCore,QtGui
from pivy import coin

# loads a translation engine
#locale = QtCore.QLocale(eval("QtCore.QLocale."+FreeCADGui.getLocale())).name()
#translator = QtCore.QTranslator()
#translator.load('Draft_'+locale+'.qm',':/translations/')
#QtGui.QApplication.installTranslator(translator)
FreeCADGui.updateLocale()

def translate(context,text):
    "convenience function for Qt translator"
    return QtGui.QApplication.translate(context, text, None, QtGui.QApplication.UnicodeUTF8).toUtf8()
		
def msg(text=None,mode=None):
    "prints the given message on the FreeCAD status bar"
    if not text: FreeCAD.Console.PrintMessage("")
    else:
        if mode == 'warning':
            FreeCAD.Console.PrintWarning(text)
        elif mode == 'error':
            FreeCAD.Console.PrintError(text)
        else:
            FreeCAD.Console.PrintMessage(text)

# loads the fill patterns
FreeCAD.svgpatterns = importSVG.getContents(Draft_rc.qt_resource_data,'pattern',True)
altpat = Draft.getParam("patternFile")
if os.path.isdir(altpat):
    for f in os.listdir(altpat):
        if '.svg' in f:
            p = importSVG.getContents(altpat+os.sep+f,'pattern')
            if p: FreeCAD.svgpatterns[p[0]]=p[1]

# sets the default working plane
plane = WorkingPlane.plane()
FreeCAD.DraftWorkingPlane = plane
defaultWP = Draft.getParam("defaultWP")
if defaultWP == 1: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), 0)
elif defaultWP == 2: plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,1,0), 0)
elif defaultWP == 3: plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), 0)

# last snapped objects, for quick intersection calculation
lastObj = [0,0]

# set modifier keys
MODS = ["shift","ctrl","alt"]
MODCONSTRAIN = MODS[Draft.getParam("modconstrain")]
MODSNAP = MODS[Draft.getParam("modsnap")]
MODALT = MODS[Draft.getParam("modalt")]

#---------------------------------------------------------------------------
# Snapping stuff
#---------------------------------------------------------------------------

def snapPoint(target,point,cursor,ctrl=False):
    '''
    Snap function used by the Draft tools
    
    Currently has two modes: passive and active. Pressing CTRL while 
    clicking puts you in active mode:
    
    - In passive mode (an open circle appears), your point is
    snapped to the nearest point on any underlying geometry.
    
    - In active mode (ctrl pressed, a filled circle appears), your point
    can currently be snapped to the following points:
        - Nodes and midpoints of all Part shapes
        - Nodes and midpoints of lines/wires
        - Centers and quadrant points of circles
        - Endpoints of arcs
        - Intersection between line, wires segments, arcs and circles
        - When constrained (SHIFT pressed), Intersections between
        constraining axis and lines/wires
    '''
        
    def getConstrainedPoint(edge,last,constrain):
        "check for constrained snappoint"
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        ar = []
        if (constrain == 0):
            if ((last.y > p1.y) and (last.y < p2.y) or (last.y > p2.y) and (last.y < p1.y)):
                pc = (last.y-p1.y)/(p2.y-p1.y)
                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                ar.append([cp,1,cp]) # constrainpoint
        if (constrain == 1):
            if ((last.x > p1.x) and (last.x < p2.x) or (last.x > p2.x) and (last.x < p1.x)):
                pc = (last.x-p1.x)/(p2.x-p1.x)
                cp = (Vector(p1.x+pc*(p2.x-p1.x),p1.y+pc*(p2.y-p1.y),p1.z+pc*(p2.z-p1.z)))
                ar.append([cp,1,cp]) # constrainpoint
        return ar

    def getPassivePoint(info):
        "returns a passive snap point"
        cur = Vector(info['x'],info['y'],info['z'])
        return [cur,2,cur]

    def getScreenDist(dist,cursor):
        "returns a 3D distance from a screen pixels distance"
        p1 = FreeCADGui.ActiveDocument.ActiveView.getPoint(cursor)
        p2 = FreeCADGui.ActiveDocument.ActiveView.getPoint((cursor[0]+dist,cursor[1]))
        return (p2.sub(p1)).Length

    def getGridSnap(target,point):
        "returns a grid snap point if available"
        if target.grid:
            return target.grid.getClosestNode(point)
        return None

    def getPerpendicular(edge,last):
        "returns a point on an edge, perpendicular to the given point"
        dv = last.sub(edge.Vertexes[0].Point)
        nv = fcvec.project(dv,fcgeo.vec(edge))
        np = (edge.Vertexes[0].Point).add(nv)
        return np

    # checking if alwaySnap setting is on
    extractrl = False
    if Draft.getParam("alwaysSnap"):
        extractrl = ctrl
        ctrl = True                

    # setting Radius
    radius =  getScreenDist(Draft.getParam("snapRange"),cursor)
	
    # checking if parallel to one of the edges of the last objects
    target.snap.off()
    target.extsnap.off()
    if (len(target.node) > 0):
        for o in [lastObj[1],lastObj[0]]:
            if o:
                ob = target.doc.getObject(o)
                if ob:
                    edges = ob.Shape.Edges
                    if len(edges)<10:
                        for e in edges:
                            if isinstance(e.Curve,Part.Line):
                                last = target.node[len(target.node)-1]
                                de = Part.Line(last,last.add(fcgeo.vec(e))).toShape()
                                np = getPerpendicular(e,point)
                                if (np.sub(point)).Length < radius:
                                    target.snap.coords.point.setValue((np.x,np.y,np.z))
                                    target.snap.setMarker("circle")
                                    target.snap.on()
                                    target.extsnap.p1(e.Vertexes[0].Point)
                                    target.extsnap.p2(np)
                                    target.extsnap.on()
                                    point = np
                                else:
                                    last = target.node[len(target.node)-1]
                                    de = Part.Line(last,last.add(fcgeo.vec(e))).toShape()  
                                    np = getPerpendicular(de,point)
                                    if (np.sub(point)).Length < radius:
                                        target.snap.coords.point.setValue((np.x,np.y,np.z))
                                        target.snap.setMarker("circle")
                                        target.snap.on()
                                        point = np

    # check if we snapped to something
    snapped=target.view.getObjectInfo((cursor[0],cursor[1]))

    if (snapped == None):
        # nothing has been snapped, check fro grid snap
        gpt = getGridSnap(target,point)
        if gpt:
            if radius != 0:
                dv = point.sub(gpt)
                if dv.Length <= radius:
                    target.snap.coords.point.setValue((gpt.x,gpt.y,gpt.z))
                    target.snap.setMarker("point")
                    target.snap.on()  
                    return gpt
        return point
    else:
        # we have something to snap
        obj = target.doc.getObject(snapped['Object'])
        if hasattr(obj.ViewObject,"Selectable"):
                        if not obj.ViewObject.Selectable:
                                return point
        if not ctrl:
                        # are we in passive snap?
                        snapArray = [getPassivePoint(snapped)]
        else:
            snapArray = []
            comp = snapped['Component']
            if obj.isDerivedFrom("Part::Feature"):
                if "Edge" in comp:
                    # get the stored objects to calculate intersections
                    intedges = []
                    if lastObj[0]:
                        lo = target.doc.getObject(lastObj[0])
                        if lo:
                            if lo.isDerivedFrom("Part::Feature"):
                                intedges = lo.Shape.Edges
                                                           
                    nr = int(comp[4:])-1
                    edge = obj.Shape.Edges[nr]
                    for v in edge.Vertexes:
                        snapArray.append([v.Point,0,v.Point])
                    if isinstance(edge.Curve,Part.Line):
                        # the edge is a line
                        midpoint = fcgeo.findMidpoint(edge)
                        snapArray.append([midpoint,1,midpoint])
                        if (len(target.node) > 0):
                            last = target.node[len(target.node)-1]
                            snapArray.extend(getConstrainedPoint(edge,last,target.constrain))
                            np = getPerpendicular(edge,last)
                            snapArray.append([np,1,np])

                    elif isinstance (edge.Curve,Part.Circle):
                        # the edge is an arc
                        rad = edge.Curve.Radius
                        pos = edge.Curve.Center
                        for i in [0,30,45,60,90,120,135,150,180,210,225,240,270,300,315,330]:
                            ang = math.radians(i)
                            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
                            snapArray.append([cur,1,cur])
                        for i in [15,37.5,52.5,75,105,127.5,142.5,165,195,217.5,232.5,255,285,307.5,322.5,345]:
                            ang = math.radians(i)
                            cur = Vector(math.sin(ang)*rad+pos.x,math.cos(ang)*rad+pos.y,pos.z)
                            snapArray.append([cur,0,pos])

                    for e in intedges:
                        # get the intersection points
                        pt = fcgeo.findIntersection(e,edge)
                        if pt:
                            for p in pt:
                                snapArray.append([p,3,p])
                elif "Vertex" in comp:
                    # directly snapped to a vertex
                    p = Vector(snapped['x'],snapped['y'],snapped['z'])
                    snapArray.append([p,0,p])
                elif comp == '':
                    # workaround for the new view provider
                    p = Vector(snapped['x'],snapped['y'],snapped['z'])
                    snapArray.append([p,2,p])
                else:
                    snapArray = [getPassivePoint(snapped)]
            elif Draft.getType(obj) == "Dimension":
                for pt in [obj.Start,obj.End,obj.Dimline]:
                    snapArray.append([pt,0,pt])
            elif Draft.getType(obj) == "Mesh":
                for v in obj.Mesh.Points:
                    snapArray.append([v.Vector,0,v.Vector])
        if not lastObj[0]:
            lastObj[0] = obj.Name
            lastObj[1] = obj.Name
        if (lastObj[1] != obj.Name):
            lastObj[0] = lastObj[1]
            lastObj[1] = obj.Name

        # calculating shortest distance
        shortest = 1000000000000000000
        spt = Vector(snapped['x'],snapped['y'],snapped['z'])
        newpoint = [Vector(0,0,0),0,Vector(0,0,0)]
        for pt in snapArray:
            if pt[0] == None: print "snapPoint: debug 'i[0]' is 'None'"
            di = pt[0].sub(spt)
            if di.Length < shortest:
                shortest = di.Length
                newpoint = pt
        if radius != 0:
            dv = point.sub(newpoint[2])
            if (not extractrl) and (dv.Length > radius):
                newpoint = getPassivePoint(snapped)
        target.snap.coords.point.setValue((newpoint[2].x,newpoint[2].y,newpoint[2].z))
        if (newpoint[1] == 1):
            target.snap.setMarker("square")
        elif (newpoint[1] == 0):
            target.snap.setMarker("point")
        elif (newpoint[1] == 3):
            target.snap.setMarker("square")
        else:
            target.snap.setMarker("circle")
        target.snap.on()                                
        return newpoint[2]

def constrainPoint (target,pt,mobile=False,sym=False):
    '''
    Constrain function used by the Draft tools
    On commands that need to enter several points (currently only line/wire),
    you can constrain the next point to be picked to the last drawn point by
    pressing SHIFT. The vertical or horizontal constraining depends on the
    position of your mouse in relation to last point at the moment you press
    SHIFT. if mobile=True, mobile behaviour applies. If sym=True, x alway = y
    '''
    point = Vector(pt)
    if len(target.node) > 0:
        last = target.node[-1]
        dvec = point.sub(last)
        affinity = plane.getClosestAxis(dvec)
        if ((target.constrain == None) or mobile):
            if affinity == "x":
                dv = fcvec.project(dvec,plane.u)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(plane.u) > 1:
                        l = -l
                    point = last.add(plane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 0 #x direction
                target.ui.xValue.setEnabled(True)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(False)
                target.ui.xValue.setFocus()
            elif affinity == "y":
                dv = fcvec.project(dvec,plane.v)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(plane.v) > 1:
                        l = -l
                    point = last.add(plane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 1 #y direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(True)
                target.ui.zValue.setEnabled(False)
                target.ui.yValue.setFocus()
            elif affinity == "z":
                dv = fcvec.project(dvec,plane.axis)
                point = last.add(dv)
                if sym:
                    l = dv.Length
                    if dv.getAngle(plane.axis) > 1:
                        l = -l
                    point = last.add(plane.getGlobalCoords(Vector(l,l,l)))
                target.constrain = 2 #z direction
                target.ui.xValue.setEnabled(False)
                target.ui.yValue.setEnabled(False)
                target.ui.zValue.setEnabled(True)
                target.ui.zValue.setFocus()
            else: target.constrain = 3
        elif (target.constrain == 0):
            dv = fcvec.project(dvec,plane.u)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(plane.u) > 1:
                    l = -l
                point = last.add(plane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 1):
            dv = fcvec.project(dvec,plane.v)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(plane.u) > 1:
                    l = -l
                point = last.add(plane.getGlobalCoords(Vector(l,l,l)))
        elif (target.constrain == 2):
            dv = fcvec.project(dvec,plane.axis)
            point = last.add(dv)
            if sym:
                l = dv.Length
                if dv.getAngle(plane.u) > 1:
                    l = -l
                point = last.add(plane.getGlobalCoords(Vector(l,l,l)))			
    return point

def selectObject(arg):
    '''this is a scene even handler, to be called from the Draft tools
    when they need to select an object'''
    if (arg["Type"] == "SoKeyboardEvent"):
        if (arg["Key"] == "ESCAPE"):
            FreeCAD.activeDraftCommand.finish()
            # TODO : this part raises a coin3D warning about scene traversal, to be fixed.
        if (arg["Type"] == "SoMouseButtonEvent"):
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                snapped = FreeCADGui.ActiveDocument.ActiveView.getObjectInfo((cursor[0],cursor[1]))
                if snapped:
                    obj = FreeCAD.ActiveDocument.getObject(snapped['Object'])
                    FreeCADGui.Selection.addSelection(obj)
                    FreeCAD.activeDraftCommand.component=snapped['Component']
                    FreeCAD.activeDraftCommand.proceed()

def getPoint(target,args,mobile=False,sym=False,workingplane=True):
    '''
    Function used by the Draft Tools.
    returns a constrained 3d point and its original point.
    if mobile=True, the constraining occurs from the location of
    mouse cursor when Shift is pressed, otherwise from last entered
    point. If sym=True, x and y values stay always equal. If workingplane=False,
    the point wont be projected on the Working Plane.
    '''
    ui = FreeCADGui.draftToolBar
    view = FreeCADGui.ActiveDocument.ActiveView
    point = view.getPoint(args["Position"][0],args["Position"][1])
    point = snapPoint(target,point,args["Position"],hasMod(args,MODSNAP))

    if (not plane.weak) and workingplane:
        # working plane was explicitely selected - project onto it
        viewDirection = view.getViewDirection()
        if FreeCADGui.ActiveDocument.ActiveView.getCameraType() == "Perspective":
            camera = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            p = camera.getField("position").getValue()
            # view is from camera to point:
            viewDirection = point.sub(Vector(p[0],p[1],p[2]))
        # if we are not snapping to anything, project along view axis,
        # otherwise perpendicularly
        if view.getObjectInfo((args["Position"][0],args["Position"][1])):
            pass
            # point = plane.projectPoint(point)
        else:
            point = plane.projectPoint(point, viewDirection)
    ctrlPoint = Vector(point.x,point.y,point.z)
    if (hasMod(args,MODCONSTRAIN)): # constraining
        if mobile and (target.constrain == None):
            target.node.append(point)
        point = constrainPoint(target,point,mobile=mobile,sym=sym)
    else:
        target.constrain = None
        ui.xValue.setEnabled(True)
        ui.yValue.setEnabled(True)
        ui.zValue.setEnabled(True)
    if target.node:
        if target.featureName == "Rectangle":
            ui.displayPoint(point, target.node[0], plane=plane)
        else:
            ui.displayPoint(point, target.node[-1], plane=plane)
    else: ui.displayPoint(point, plane=plane)
    return point,ctrlPoint

def getSupport(args):
    "returns the supporting object and sets the working plane"
    snapped = FreeCADGui.ActiveDocument.ActiveView.getObjectInfo((args["Position"][0],args["Position"][1]))
    if not snapped: return None
    obj = None
    plane.save()
    try:
        obj = FreeCAD.ActiveDocument.getObject(snapped['Object'])
        shape = obj.Shape
        component = getattr(shape,snapped["Component"])
        if plane.alignToFace(component, 0) \
                or plane.alignToCurve(component, 0):
            self.display(plane.axis)
    except:
        pass
    return obj

def hasMod(args,mod):
    "checks if args has a specific modifier"
    if mod == "shift":
        return args["ShiftDown"]
    elif mod == "ctrl":
        return args["CtrlDown"]
    elif mod == "alt":
        return args["AltDown"]

def setMod(args,mod,state):
    "sets a specific modifier state in args"
    if mod == "shift":
        args["ShiftDown"] = state
    elif mod == "ctrl":
        args["CtrlDown"] = state
    elif mod == "alt":
        args["AltDown"] = state
        
#---------------------------------------------------------------------------
# Trackers
#---------------------------------------------------------------------------

class Tracker:
    "A generic Draft Tracker, to be used by other specific trackers"
    def __init__(self,dotted=False,scolor=None,swidth=None,children=[],ontop=False):
        self.ontop = ontop
        color = coin.SoBaseColor()
        color.rgb = scolor or FreeCADGui.draftToolBar.getDefaultColor("ui")
        drawstyle = coin.SoDrawStyle()
        if swidth:
            drawstyle.lineWidth = swidth
        if dotted:
            drawstyle.style = coin.SoDrawStyle.LINES
            drawstyle.lineWeight = 3
            drawstyle.linePattern = 0x0f0f #0xaa
        node = coin.SoSeparator()
        for c in [drawstyle, color] + children:
            node.addChild(c)
        self.switch = coin.SoSwitch() # this is the on/off switch
        self.switch.addChild(node)
        self.switch.whichChild = -1
        self.Visible = False
        todo.delay(self._insertSwitch, self.switch)

    def finalize(self):
        todo.delay(self._removeSwitch, self.switch)
        self.switch = None

    def _insertSwitch(self, switch):
        '''insert self.switch into the scene graph.  Must not be called
        from an event handler (or other scene graph traversal).'''
        sg=FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
        if self.ontop:
            sg.insertChild(switch,0)
        else:
            sg.addChild(switch)

    def _removeSwitch(self, switch):
        '''remove self.switch from the scene graph.  As with _insertSwitch,
        must not be called during scene graph traversal).'''
        sg=FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
        sg.removeChild(switch)

    def on(self):
        self.switch.whichChild = 0
        self.Visible = True

    def off(self):
        self.switch.whichChild = -1
        self.Visible = False
				
class snapTracker(Tracker):
    "A Snap Mark tracker, used by tools that support snapping"
    def __init__(self):
        color = coin.SoBaseColor()
        color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet() # this is the marker symbol
        self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValue((0,0,0))
        node = coin.SoAnnotation()
        node.addChild(self.coords)
        node.addChild(color)
        node.addChild(self.marker)
        Tracker.__init__(self,children=[node])

    def setMarker(self,style):
        if (style == "point"):
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_FILLED_9_9
        elif (style == "square"):
            self.marker.markerIndex = coin.SoMarkerSet.DIAMOND_FILLED_9_9
        elif (style == "circle"):
            self.marker.markerIndex = coin.SoMarkerSet.CIRCLE_LINE_9_9

class lineTracker(Tracker):
    "A Line tracker, used by the tools that need to draw temporary lines"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        line = coin.SoLineSet()
        line.numVertices.setValue(2)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,2,[[0,0,0],[1,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])

    def p1(self,point=None):
        "sets or gets the first point of the line"
        if point:
            self.coords.point.set1Value(0,point.x,point.y,point.z)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self,point=None):
        "sets or gets the second point of the line"
        if point:
            self.coords.point.set1Value(1,point.x,point.y,point.z)
        else:
            return Vector(self.coords.point.getValues()[-1].getValue())
                        
    def getLength(self):
        "returns the length of the line"
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[-1].getValue())
        return (p2.sub(p1)).Length

class rectangleTracker(Tracker):
    "A Rectangle tracker, used by the rectangle tool"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        self.origin = Vector(0,0,0)
        line = coin.SoLineSet()
        line.numVertices.setValue(5)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,50,[[0,0,0],[2,0,0],[2,2,0],[0,2,0],[0,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])
        self.u = plane.u
        self.v = plane.v

    def setorigin(self,point):
        "sets the base point of the rectangle"
        self.coords.point.set1Value(0,point.x,point.y,point.z)
        self.coords.point.set1Value(4,point.x,point.y,point.z)
        self.origin = point

    def update(self,point):
        "sets the opposite (diagonal) point of the rectangle"
        diagonal = point.sub(self.origin)
        inpoint1 = self.origin.add(fcvec.project(diagonal,self.v))
        inpoint2 = self.origin.add(fcvec.project(diagonal,self.u))
        self.coords.point.set1Value(1,inpoint1.x,inpoint1.y,inpoint1.z)
        self.coords.point.set1Value(2,point.x,point.y,point.z)
        self.coords.point.set1Value(3,inpoint2.x,inpoint2.y,inpoint2.z)

    def setPlane(self,u,v=None):
        '''sets given (u,v) vectors as working plane. You can give only u
        and v will be deduced automatically given current workplane'''
        self.u = u
        if v:
            self.v = v
        else:
            norm = plane.u.cross(plane.v)
            self.v = self.u.cross(norm)

    def p1(self,point=None):
        "sets or gets the base point of the rectangle"
        if point:
            self.setorigin(point)
        else:
            return Vector(self.coords.point.getValues()[0].getValue())

    def p2(self):
        "gets the second point (on u axis) of the rectangle"
        return Vector(self.coords.point.getValues()[3].getValue())

    def p3(self,point=None):
        "sets or gets the opposite (diagonal) point of the rectangle"
        if point:
            self.update(point)
        else:
            return Vector(self.coords.point.getValues()[2].getValue())

    def p4(self):
        "gets the fourth point (on v axis) of the rectangle"
        return Vector(self.coords.point.getValues()[1].getValue())
                
    def getSize(self):
        "returns (length,width) of the rectangle"
        p1 = Vector(self.coords.point.getValues()[0].getValue())
        p2 = Vector(self.coords.point.getValues()[2].getValue())
        diag = p2.sub(p1)
        return ((fcvec.project(diag,self.u)).Length,(fcvec.project(diag,self.v)).Length)

    def getNormal(self):
        "returns the normal of the rectangle"
        return (self.u.cross(self.v)).normalize()
                
class dimTracker(Tracker):
    "A Dimension tracker, used by the dimension tool"
    def __init__(self,dotted=False,scolor=None,swidth=None):
        line = coin.SoLineSet()
        line.numVertices.setValue(4)
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValues(0,4,[[0,0,0],[0,0,0],[0,0,0],[0,0,0]])
        Tracker.__init__(self,dotted,scolor,swidth,[self.coords,line])
        self.p1 = self.p2 = self.p3 = None

    def update(self,pts):
        if len(pts) == 1:
            self.p3 = pts[0]
        else:
            self.p1 = pts[0]
            self.p2 = pts[1]
            if len(pts) > 2:
                self.p3 = pts[2]
        self.calc()
        
    def calc(self):
        if (self.p1 != None) and (self.p2 != None):
            points = [fcvec.tup(self.p1,True),fcvec.tup(self.p2,True),\
                          fcvec.tup(self.p1,True),fcvec.tup(self.p2,True)]
            if self.p3 != None:
                p1 = self.p1
                p4 = self.p2
                if fcvec.equals(p1,p4):
                    proj = None
                else:
                    base = Part.Line(p1,p4).toShape()
                    proj = fcgeo.findDistance(self.p3,base)
                if not proj:
                    p2 = p1
                    p3 = p4
                else:
                    p2 = p1.add(fcvec.neg(proj))
                    p3 = p4.add(fcvec.neg(proj))
                points = [fcvec.tup(p1),fcvec.tup(p2),fcvec.tup(p3),fcvec.tup(p4)]
            self.coords.point.setValues(0,4,points)

class bsplineTracker(Tracker):
    "A bspline tracker"
    def __init__(self,dotted=False,scolor=None,swidth=None,points = []):
        self.bspline = None
        self.points = points
        self.trans = coin.SoTransform()
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self,dotted,scolor,swidth,[self.trans,self.sep])
        
    def update(self, points):
        self.points = points
        self.recompute()
            
    def recompute(self):
        if (len(self.points) >= 2):
            if self.bspline: self.sep.removeChild(self.bspline)
            self.bspline = None
            c =  Part.BSplineCurve()
            # DNC: allows to close the curve by placing ends close to each other
            if ( len(self.points) >= 3 ) and ( (self.points[0] - self.points[-1]).Length < Draft.tolerance() ):
                # YVH: Added a try to bypass some hazardous situations
                try:
                    c.interpolate(self.points[:-1], True)
                except:
                    pass
            elif self.points:
                try:
                    c.interpolate(self.points, False)
                except:
                    pass
            c = c.toShape()
            buf=c.writeInventor(2,0.01)
            #fp=open("spline.iv","w")
            #fp.write(buf)
            #fp.close()
            ivin = coin.SoInput()
            ivin.setBuffer(buf)
            ivob = coin.SoDB.readAll(ivin)
            # In case reading from buffer failed
            if ivob and ivob.getNumChildren() > 1:
                self.bspline = ivob.getChild(1).getChild(0)
                self.bspline.removeChild(self.bspline.getChild(0))
                self.bspline.removeChild(self.bspline.getChild(0))
                self.sep.addChild(self.bspline)
            else:
                FreeCAD.Console.PrintWarning("bsplineTracker.recompute() failed to read-in Inventor string\n")

class arcTracker(Tracker):
    "An arc tracker"
    def __init__(self,dotted=False,scolor=None,swidth=None,start=0,end=math.pi*2):
        self.circle = None
        self.startangle = math.degrees(start)
        self.endangle = math.degrees(end)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        self.sep = coin.SoSeparator()
        self.recompute()
        Tracker.__init__(self,dotted,scolor,swidth,[self.trans, self.sep])

    def setCenter(self,cen):
        "sets the center point"
        self.trans.translation.setValue([cen.x,cen.y,cen.z])

    def setRadius(self,rad):
        "sets the radius"
        self.trans.scaleFactor.setValue([rad,rad,rad])

    def getRadius(self):
        "returns the current radius"
        return self.trans.scaleFactor.getValue()[0]

    def setStartAngle(self,ang):
        "sets the start angle"
        self.startangle = math.degrees(ang)
        self.recompute()

    def setEndAngle(self,ang):
        "sets the end angle"
        self.endangle = math.degrees(ang)
        self.recompute()

    def getAngle(self,pt):
        "returns the angle of a given vector"
        c = self.trans.translation.getValue()
        center = Vector(c[0],c[1],c[2])
        base = plane.u
        rad = pt.sub(center)
        return(fcvec.angle(rad,base,plane.axis))

    def getAngles(self):
        "returns the start and end angles"
        return(self.startangle,self.endangle)
                
    def setStartPoint(self,pt):
        "sets the start angle from a point"
        self.setStartAngle(-self.getAngle(pt))

    def setEndPoint(self,pt):
        "sets the end angle from a point"
        self.setEndAngle(self.getAngle(pt))
                
    def setApertureAngle(self,ang):
        "sets the end angle by giving the aperture angle"
        ap = math.degrees(ang)
        self.endangle = self.startangle + ap
        self.recompute()

    def recompute(self):
        if self.circle: self.sep.removeChild(self.circle)
        self.circle = None
        if self.endangle < self.startangle:
            c = Part.makeCircle(1,Vector(0,0,0),plane.axis,self.endangle,self.startangle)
        else:
            c = Part.makeCircle(1,Vector(0,0,0),plane.axis,self.startangle,self.endangle)
        buf=c.writeInventor(2,0.01)
        ivin = coin.SoInput()
        ivin.setBuffer(buf)
        ivob = coin.SoDB.readAll(ivin)
        # In case reading from buffer failed
        if ivob and ivob.getNumChildren() > 1:
            self.circle = ivob.getChild(1).getChild(0)
            self.circle.removeChild(self.circle.getChild(0))
            self.circle.removeChild(self.circle.getChild(0))
            self.sep.addChild(self.circle)
        else:
            FreeCAD.Console.PrintWarning("arcTracker.recompute() failed to read-in Inventor string\n")

class ghostTracker(Tracker):
    '''A Ghost tracker, that allows to copy whole object representations.
    You can pass it an object or a list of objects, or a shape.'''
    def __init__(self,sel):
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        self.children = [self.trans]
        self.ivsep = coin.SoSeparator()
        try:
            if isinstance(sel,Part.Shape):
                ivin = coin.SoInput()
                ivin.setBuffer(sel.writeInventor())
                ivob = coin.SoDB.readAll(ivin)
                self.ivsep.addChild(ivob.getChildren()[1])
            else:
                if not isinstance(sel,list):
                    sel = [sel]
                for obj in sel:
                    self.ivsep.addChild(obj.ViewObject.RootNode.copy())
        except:
            print "draft: Couldn't create ghost"
        self.children.append(self.ivsep)
        Tracker.__init__(self,children=self.children)

    def update(self,obj):
        obj.ViewObject.show()
        self.finalize()
        self.ivsep = coin.SoSeparator()
        self.ivsep.addChild(obj.ViewObject.RootNode.copy())
        Tracker.__init__(self,children=[self.ivsep])
        self.on()
        obj.ViewObject.hide()

class editTracker(Tracker):
    "A node edit tracker"
    def __init__(self,pos=Vector(0,0,0),name="None",idx=0,objcol=None):
        color = coin.SoBaseColor()
        if objcol:
            color.rgb = objcol[:3]
        else:
            color.rgb = FreeCADGui.draftToolBar.getDefaultColor("snap")
        self.marker = coin.SoMarkerSet() # this is the marker symbol
        self.marker.markerIndex = coin.SoMarkerSet.SQUARE_FILLED_9_9
        self.coords = coin.SoCoordinate3() # this is the coordinate
        self.coords.point.setValue((pos.x,pos.y,pos.z))
        selnode = coin.SoType.fromName("SoFCSelection").createInstance()
        selnode.documentName.setValue(FreeCAD.ActiveDocument.Name)
        selnode.objectName.setValue(name)
        selnode.subElementName.setValue("EditNode"+str(idx))
        node = coin.SoAnnotation()
        selnode.addChild(self.coords)
        selnode.addChild(color)
        selnode.addChild(self.marker)
        node.addChild(selnode)
        Tracker.__init__(self,children=[node],ontop=True)
        self.on()

    def set(self,pos):
        self.coords.point.setValue((pos.x,pos.y,pos.z))

    def get(self):
        p = self.coords.point.getValues()[0]
        return Vector(p[0],p[1],p[2])

    def move(self,delta):
        self.set(self.get().add(delta))

class PlaneTracker(Tracker):
    "A working plane tracker"
    def __init__(self):
        # getting screen distance
        p1 = FreeCADGui.ActiveDocument.ActiveView.getPoint((100,100))
        p2 = FreeCADGui.ActiveDocument.ActiveView.getPoint((110,100))
        bl = (p2.sub(p1)).Length * (Draft.getParam("snapRange")/2)
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
        m1 = coin.SoMaterial()
        m1.transparency.setValue(0.8)
        m1.diffuseColor.setValue([0.4,0.4,0.6])
        c1 = coin.SoCoordinate3()
        c1.point.setValues([[-bl,-bl,0],[bl,-bl,0],[bl,bl,0],[-bl,bl,0]])
        f = coin.SoIndexedFaceSet()
        f.coordIndex.setValues([0,1,2,3])
        m2 = coin.SoMaterial()
        m2.transparency.setValue(0.7)
        m2.diffuseColor.setValue([0.2,0.2,0.3])
        c2 = coin.SoCoordinate3()
        c2.point.setValues([[0,bl,0],[0,0,0],[bl,0,0],[-.05*bl,.95*bl,0],[0,bl,0],
                            [.05*bl,.95*bl,0],[.95*bl,.05*bl,0],[bl,0,0],[.95*bl,-.05*bl,0]])
        l = coin.SoLineSet()
        l.numVertices.setValues([3,3,3])
        s = coin.SoSeparator()
        s.addChild(self.trans)
        s.addChild(m1)
        s.addChild(c1)
        s.addChild(f)
        s.addChild(m2)
        s.addChild(c2)
        s.addChild(l)
        Tracker.__init__(self,children=[s])

    def set(self,pos=None):
        if pos:                        
            Q = plane.getRotation().Rotation.Q
        else:
            plm = plane.getPlacement()
            Q = plm.Rotation.Q
            pos = plm.Base
        self.trans.translation.setValue([pos.x,pos.y,pos.z])
        self.trans.rotation.setValue([Q[0],Q[1],Q[2],Q[3]])
        self.on()
                        
class wireTracker(Tracker):                
    "A wire tracker"
    def __init__(self,wire):
        self.line = coin.SoLineSet()
        self.closed = fcgeo.isReallyClosed(wire)
        if self.closed:
            self.line.numVertices.setValue(len(wire.Vertexes)+1)
        else:
            self.line.numVertices.setValue(len(wire.Vertexes))
        self.coords = coin.SoCoordinate3()
        self.update(wire)
        Tracker.__init__(self,children=[self.coords,self.line])

    def update(self,wire):
        if wire:
            self.line.numVertices.setValue(len(wire.Vertexes))
            for i in range(len(wire.Vertexes)):
                p=wire.Vertexes[i].Point
                self.coords.point.set1Value(i,[p.x,p.y,p.z])
            if self.closed:
                t = len(wire.Vertexes)
                p = wire.Vertexes[0].Point
                self.coords.point.set1Value(t,[p.x,p.y,p.z])

class gridTracker(Tracker):
    "A grid tracker"
    def __init__(self):
        # self.space = 1
        self.space = Draft.getParam("gridSpacing")
        # self.mainlines = 10
        self.mainlines = Draft.getParam("gridEvery")
        self.numlines = 100
        col = [0.2,0.2,0.3]
        
        self.trans = coin.SoTransform()
        self.trans.translation.setValue([0,0,0])
                
        bound = (self.numlines/2)*self.space
        pts = []
        mpts = []
        for i in range(self.numlines+1):
            curr = -bound + i*self.space
            z = 0
            if i/float(self.mainlines) == i/self.mainlines:
                mpts.extend([[-bound,curr,z],[bound,curr,z]])
                mpts.extend([[curr,-bound,z],[curr,bound,z]])
            else:
                pts.extend([[-bound,curr,z],[bound,curr,z]])
                pts.extend([[curr,-bound,z],[curr,bound,z]])
        idx = []
        midx = []
        for p in range(0,len(pts),2):
            idx.append(2)
        for mp in range(0,len(mpts),2):
            midx.append(2)

        mat1 = coin.SoMaterial()
        mat1.transparency.setValue(0.7)
        mat1.diffuseColor.setValue(col)
        self.coords1 = coin.SoCoordinate3()
        self.coords1.point.setValues(pts)
        lines1 = coin.SoLineSet()
        lines1.numVertices.setValues(idx)
        mat2 = coin.SoMaterial()
        mat2.transparency.setValue(0.3)
        mat2.diffuseColor.setValue(col)
        self.coords2 = coin.SoCoordinate3()
        self.coords2.point.setValues(mpts)
        lines2 = coin.SoLineSet()
        lines2.numVertices.setValues(midx)
        s = coin.SoSeparator()
        s.addChild(self.trans)
        s.addChild(mat1)
        s.addChild(self.coords1)
        s.addChild(lines1)
        s.addChild(mat2)
        s.addChild(self.coords2)
        s.addChild(lines2)
        Tracker.__init__(self,children=[s])
        self.update()

    def update(self):
        bound = (self.numlines/2)*self.space
        pts = []
        mpts = []
        for i in range(self.numlines+1):
            curr = -bound + i*self.space
            if i/float(self.mainlines) == i/self.mainlines:
                mpts.extend([[-bound,curr,0],[bound,curr,0]])
                mpts.extend([[curr,-bound,0],[curr,bound,0]])
            else:
                pts.extend([[-bound,curr,0],[bound,curr,0]])
                pts.extend([[curr,-bound,0],[curr,bound,0]])
        self.coords1.point.setValues(pts)
        self.coords2.point.setValues(mpts)

    def setSpacing(self,space):
        self.space = space
        self.update()

    def setMainlines(self,ml):
        self.mainlines = ml
        self.update()

    def set(self):
        Q = plane.getRotation().Rotation.Q
        self.trans.rotation.setValue([Q[0],Q[1],Q[2],Q[3]])
        self.on()

    def getClosestNode(self,point):
        "returns the closest node from the given point"
        # get the 2D coords.
        point = plane.projectPoint(point)
        u = fcvec.project(point,plane.u)
        lu = u.Length
        if u.getAngle(plane.u) > 1.5:
            lu  = -lu
        v = fcvec.project(point,plane.v)
        lv = v.Length
        if v.getAngle(plane.v) > 1.5:
            lv = -lv
        # print "u = ",u," v = ",v
        # find nearest grid node
        pu = (round(lu/self.space,0))*self.space
        pv = (round(lv/self.space,0))*self.space
        rot = FreeCAD.Rotation()
        rot.Q = self.trans.rotation.getValue().getValue()
        return rot.multVec(Vector(pu,pv,0))

                
#---------------------------------------------------------------------------
# Helper tools
#---------------------------------------------------------------------------
                	
class SelectPlane:
    "The Draft_SelectPlane FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SelectPlane',
                'Accel' : "W, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SelectPlane", "SelectPlane"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select a working plane for geometry creation")}

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False
        
    def Activated(self):
        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        self.offset = 0
        self.ui = None
        self.call = None
        self.doc = FreeCAD.ActiveDocument
        if self.doc:
            FreeCAD.activeDraftCommand = self
            self.view = FreeCADGui.ActiveDocument.ActiveView
            self.ui = FreeCADGui.draftToolBar
            self.ui.selectPlaneUi()
            msg(translate("draft", "Pick a face to define the drawing plane\n"))
            self.ui.sourceCmd = self
            if plane.alignToSelection(self.offset):
                FreeCADGui.Selection.clearSelection()
                self.display(plane.axis)
                self.finish()
            else:
                self.call = self.view.addEventCallback("SoEvent", self.action)

    def action(self, arg):
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        if arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                doc = FreeCADGui.ActiveDocument
                info = doc.ActiveView.getObjectInfo((cursor[0],cursor[1]))
                if info:
                    try:
                        shape = doc.getObject(info["Object"]).Object.Shape
                        component = getattr(shape, info["Component"])
                        if plane.alignToFace(component, self.offset) \
                                or plane.alignToCurve(component, self.offset):
                            self.display(plane.axis)
                            self.finish()
                    except:
                        pass

    def selectHandler(self, arg):
        try:
            self.offset = float(self.ui.offsetValue.text())
        except:
            self.offset = 0
        if arg == "XY":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,0,1), self.offset)
            self.display('top')
            self.finish()
        elif arg == "XZ":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(0,-1,0), self.offset)
            self.display('front')
            self.finish()
        elif arg == "YZ":
            plane.alignToPointAndAxis(Vector(0,0,0), Vector(1,0,0), self.offset)
            self.display('side')
            self.finish()
        elif arg == "currentView":
            viewDirection = fcvec.neg(self.view.getViewDirection())
            plane.alignToPointAndAxis(Vector(0,0,0), viewDirection, self.offset)
            self.display(viewDirection)
            self.finish()
        elif arg == "reset":
            plane.reset()
            self.display('None')
            self.finish()

    def offsetHandler(self, arg):
        self.offset = arg

    def display(self,arg):
        if self.offset:
            if self.offset > 0: suffix = ' + '+str(self.offset)
            else: suffix = ' - '+str(self.offset)
        else: suffix = ''
        if type(arg).__name__  == 'str':
            self.ui.wplabel.setText(arg+suffix)
        elif type(arg).__name__ == 'Vector':
            plv = 'd('+str(arg.x)+','+str(arg.y)+','+str(arg.z)+')'
            self.ui.wplabel.setText(plv+suffix)

    def finish(self):
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()


#---------------------------------------------------------------------------
# Geometry constructors
#---------------------------------------------------------------------------

class Creator:
    "A generic Draft Creator Tool used by creation tools such as line or arc"
    
    def __init__(self):
        self.commitList = []
        
    def Activated(self,name="None"):
        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        self.ui = None
        self.call = None
        self.doc = None
        self.support = None
        self.commitList = []
        self.doc = FreeCAD.ActiveDocument
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.featureName = name
        if not self.doc:
            self.finish()
        else:
            FreeCAD.activeDraftCommand = self
            self.ui = FreeCADGui.draftToolBar
            self.ui.cross(True)
            self.ui.sourceCmd = self
            self.ui.setTitle(name)
            self.ui.show()
            rot = self.view.getCameraNode().getField("orientation").getValue()
            upv = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
            plane.setup(fcvec.neg(self.view.getViewDirection()), Vector(0,0,0), upv)
            self.node = []
            self.pos = []
            self.constrain = None
            self.obj = None
            self.snap = snapTracker()
            self.extsnap = lineTracker(dotted=True)
            self.planetrack = PlaneTracker()
            if Draft.getParam("grid"):
                self.grid = gridTracker()
                self.grid.set()
            else:
                self.grid = None
                        
    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def finish(self):
        self.snap.finalize()
        self.extsnap.finalize()
        self.node=[]
        self.planetrack.finalize()
        if self.grid: self.grid.finalize()
        if self.support: plane.restore()
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()
            self.ui.cross(False)
            self.ui.sourceCmd = None
        msg("")
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
            self.call = None
        if self.commitList:
            todo.delayCommit(self.commitList)
        self.commitList = []

    def commit(self,name,func):
        "stores partial actions to be committed to the FreeCAD document"
        self.commitList.append((name,func))

class Line(Creator):
    "The Line FreeCAD command definition"

    def __init__(self, wiremode=False):
        self.isWire = wiremode

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Line',
                'Accel' : "L,I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Line", "Creates a 2-point line. CTRL to snap, SHIFT to constrain")}

    def Activated(self,name="Line"):
        Creator.Activated(self,name)
        if self.doc:
            self.obj = None
            self.ui.lineUi()
            self.linetrack = lineTracker()
            self.constraintrack = lineTracker(dotted=True)
            self.obj=self.doc.addObject("Part::Feature",self.featureName)
            # self.obj.ViewObject.Selectable = False
            Draft.formatObject(self.obj)
            if not Draft.getParam("UiMode"): self.makeDumbTask()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick first point:\n"))

    def makeDumbTask(self):
        "create a dumb taskdialog to prevent deleting the temp object"
        class TaskPanel:
            def __init__(self):
                pass
            def getStandardButtons(self):
                return 0
        panel = TaskPanel()
        FreeCADGui.Control.showDialog(panel)

    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        if not Draft.getParam("UiMode"):
            FreeCADGui.Control.closeDialog()
        if self.obj:
            old = self.obj.Name
            todo.delay(self.doc.removeObject,old)
        self.obj = None
        if (len(self.node) > 1):
            self.commit(translate("draft","Create Wire"),
                        partial(Draft.makeWire,self.node,closed,
                                face=self.ui.hasFill.isChecked(),support=self.support))
        if self.ui:
            self.linetrack.finalize()
            self.constraintrack.finalize()
        Creator.finish(self)
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg)
            self.ui.cross(True)
            self.linetrack.p2(point)
            # Draw constraint tracker line.
            if hasMod(arg,MODCONSTRAIN):
                self.constraintrack.p1(point)
                self.constraintrack.p2(ctrlPoint)
                self.constraintrack.on()
            else:
                self.constraintrack.off()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish(False,cont=True)
                else:
                    if not self.node: self.support = getSupport(arg)
                    point,ctrlPoint = getPoint(self,arg)
                    self.pos = arg["Position"]
                    self.node.append(point)
                    self.linetrack.p1(point)
                    self.drawSegment(point)
                    if (not self.isWire and len(self.node) == 2):
                        self.finish(False,cont=True)
                    if (len(self.node) > 2):
                        # DNC: allows to close the curve
                        # by placing ends close to each other
                        # with tol = Draft tolerance
                        # old code has been to insensitive
                        # if fcvec.equals(point,self.node[0]):
                        if ((point-self.node[0]).Length < Draft.tolerance()):
                            self.undolast()
                            self.finish(True,cont=True)
                            msg(translate("draft", "Wire has been closed\n"))

    def undolast(self):
        "undoes last line segment"
        if (len(self.node) > 1):
            self.node.pop()
            last = self.node[len(self.node)-1]
            self.linetrack.p1(last)
            if self.obj.Shape.Edges:
                edges = self.obj.Shape.Edges
                if len(edges) > 1:
                    edges.pop()
                    newshape = Part.Wire(edges)
                else:
                    newshape = Part.Shape()
                self.obj.Shape = newshape
                # DNC: report on removal
                msg(translate("draft", "Last point has been removed\n"))

    def drawSegment(self,point):
        "draws a new segment"
        if (len(self.node) == 1):
            self.linetrack.on()
            msg(translate("draft", "Pick next point:\n"))
            self.planetrack.set(self.node[0])
        elif (len(self.node) == 2):
            last = self.node[len(self.node)-2]
            newseg = Part.Line(last,point).toShape()
            self.obj.Shape = newseg
            self.obj.ViewObject.Visibility = True
            if self.isWire:
                msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))
        else:
            currentshape = self.obj.Shape
            last = self.node[len(self.node)-2]
            newseg = Part.Line(last,point).toShape()
            newshape=currentshape.fuse(newseg)
            self.obj.Shape = newshape
            msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))

    def wipe(self):
        "removes all previous segments and starts from last point"
        if len(self.node) > 1:
            print "nullifying"
            # self.obj.Shape.nullify() - for some reason this fails
            self.obj.ViewObject.Visibility = False
            self.node = [self.node[-1]]
            print "setting trackers"
            self.linetrack.p1(self.node[0])
            self.planetrack.set(self.node[0])
            msg(translate("draft", "Pick next point:\n"))
            print "done"
                        
    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        point = Vector(numx,numy,numz)
        self.node.append(point)
        self.linetrack.p1(point)
        self.drawSegment(point)
        if (not self.isWire and len(self.node) == 2):
            self.finish(False,cont=True)
        if self.ui.xValue.isEnabled():
            self.ui.xValue.setFocus()
            self.ui.xValue.selectAll()
        elif self.ui.yValue.isEnabled():
            self.ui.yValue.setFocus()
            self.ui.yValue.selectAll()
        else:
            self.ui.zValue.setFocus()
            self.ui.zValue.selectAll()

            
class Wire(Line):
    "a FreeCAD command for creating a wire"
    def __init__(self):
        Line.__init__(self,wiremode=True)
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Wire',
                'Accel' : "W, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "Wire"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Wire", "Creates a multiple-point wire. CTRL to snap, SHIFT to constrain")}

    
class BSpline(Line):
    "a FreeCAD command for creating a b-spline"
    
    def __init__(self):
        Line.__init__(self,wiremode=True)

    def GetResources(self):
        return {'Pixmap'  : 'Draft_BSpline',
                'Accel' : "B, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "B-Spline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_BSpline", "Creates a multiple-point b-spline. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Line.Activated(self,"BSpline")
        if self.doc:
            self.bsplinetrack = bsplineTracker()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg)
            self.ui.cross(True)
            self.bsplinetrack.update(self.node + [point])
            # Draw constraint tracker line.
            if hasMod(arg,MODCONSTRAIN):
                self.constraintrack.p1(point)
                self.constraintrack.p2(ctrlPoint)
                self.constraintrack.on()
            else: self.constraintrack.off()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish(False,cont=True)
                else:
                    if not self.node: self.support = getSupport(arg)
                    point,ctrlPoint = getPoint(self,arg)
                    self.pos = arg["Position"]
                    self.node.append(point)
                    self.drawUpdate(point)
                    if (not self.isWire and len(self.node) == 2):
                        self.finish(False,cont=True)
                    if (len(self.node) > 2):
                        # DNC: allows to close the curve
                        # by placing ends close to each other
                        # with tol = Draft tolerance
                        # old code has been to insensitive
                        if ((point-self.node[0]).Length < Draft.tolerance()):
                            self.undolast()
                            self.finish(True,cont=True)
                            msg(translate("draft", "Spline has been closed\n"))

    def undolast(self):
        "undoes last line segment"
        if (len(self.node) > 1):
            self.node.pop()
            self.bsplinetrack.update(self.node)
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            msg(translate("draft", "Last point has been removed\n"))

    def drawUpdate(self,point):
        if (len(self.node) == 1):
            self.bsplinetrack.on()
            self.planetrack.set(self.node[0])
            msg(translate("draft", "Pick next point:\n"))
        else:
            spline = Part.BSplineCurve()
            spline.interpolate(self.node, False)
            self.obj.Shape = spline.toShape()
            msg(translate("draft", "Pick next point, or (F)inish or (C)lose:\n"))
	
    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        if not Draft.getParam("UiMode"):
            FreeCADGui.Control.closeDialog()
        if (len(self.node) > 1):
            old = self.obj.Name
            self.doc.removeObject(old)
            self.commit(translate("draft","Create BSpline"),
                        partial(Draft.makeBSpline,self.node,closed,
                                face=self.ui.hasFill.isChecked(),support=self.support))
        if self.ui:
			self.bsplinetrack.finalize()
			self.constraintrack.finalize()
        Creator.finish(self)
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

                
class FinishLine:
    "a FreeCAD command to finish any running Line drawing operation"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(False)
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Finish',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finish line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_FinishLine", "Finishes a line without closing it")}
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False

    
class CloseLine:
    "a FreeCAD command to close any running Line drawing operation"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.finish(True)
                
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Lock',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Close Line"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_CloseLine", "Closes the line being drawn")}
    
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False


class UndoLine:
    "a FreeCAD command to undo last drawn segment of a line"
    
    def Activated(self):
        if (FreeCAD.activeDraftCommand != None):
            if (FreeCAD.activeDraftCommand.featureName == "Line"):
                FreeCAD.activeDraftCommand.undolast()
                
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undo last segment"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_UndoLine", "Undoes the last drawn segment of the line being drawn")}
    
    def IsActive(self):
        if FreeCAD.activeDraftCommand:
            if FreeCAD.activeDraftCommand.featureName == "Line":
                return True
        return False

    
class Rectangle(Creator):
    "the Draft_Rectangle FreeCAD command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rectangle',
                'Accel' : "R, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Rectangle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rectangle", "Creates a 2-point rectangle. CTRL to snap")}

    def Activated(self):
        Creator.Activated(self,"Rectangle")
        if self.ui:
            self.refpoint = None
            self.ui.pointUi()
            self.ui.extUi()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.rect = rectangleTracker()
            msg(translate("draft", "Pick first point:\n"))

    def finish(self,closed=False,cont=False):
        "terminates the operation and closes the poly if asked"
        Creator.finish(self) 
        if self.ui:
            self.rect.off()
            self.rect.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        "creates the final object in the current doc"
        p1 = self.node[0]
        p3 = self.node[-1]
        diagonal = p3.sub(p1)
        p2 = p1.add(fcvec.project(diagonal, plane.v))
        p4 = p1.add(fcvec.project(diagonal, plane.u))
        length = p4.sub(p1).Length
        if abs(fcvec.angle(p4.sub(p1),plane.u,plane.axis)) > 1: length = -length
        height = p2.sub(p1).Length
        if abs(fcvec.angle(p2.sub(p1),plane.v,plane.axis)) > 1: height = -height
        p = plane.getRotation()
        p.move(p1)
        self.commit(translate("draft","Create Rectangle"),
                    partial(Draft.makeRectangle,length,height,
                            p,self.ui.hasFill.isChecked(),support=self.support))
        self.finish(cont=True)

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg,mobile=True)
            self.rect.update(point)
            self.ui.cross(True)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if (arg["Position"] == self.pos):
                    self.finish()
                else:
                    if not self.node: self.support = getSupport(arg)
                    point,ctrlPoint = getPoint(self,arg)
                    self.appendPoint(point)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        point = Vector(numx,numy,numz)
        self.appendPoint(point)

    def appendPoint(self,point):
        self.node.append(point)
        if (len(self.node) > 1):
            self.rect.update(point)
            self.createObject()
        else:
            msg(translate("draft", "Pick opposite point:\n"))
            self.ui.isRelative.show()
            self.rect.setorigin(point)
            self.rect.on()
            self.planetrack.set(point)


class Arc(Creator):
    "the Draft_Arc FreeCAD command definition"
        
    def __init__(self):
        self.closedCircle=False
        self.featureName = "Arc"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Arc',
                'Accel' : "A, R",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Arc"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Arc", "Creates an arc. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Creator.Activated(self,self.featureName)
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.angle = 0 # angle inscribed by arc
            self.tangents = []
            self.tanpoints = []
            if self.featureName == "Arc": self.ui.arcUi()
            else: self.ui.circleUi()
            self.altdown = False
            self.ui.sourceCmd = self
            self.linetrack = lineTracker(dotted=True)
            self.constraintrack = lineTracker(dotted=True)
            self.arctrack = arcTracker()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick center point:\n"))

    def finish(self,closed=False,cont=False):
        "finishes the arc"
        Creator.finish(self)
        if self.ui:
            self.linetrack.finalize()
            self.constraintrack.finalize()
            self.arctrack.finalize()
            self.doc.recompute()
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

    def updateAngle(self, angle):
        # previous absolute angle
        lastangle = self.firstangle + self.angle
        if lastangle <= -2*math.pi: lastangle += 2*math.pi
        if lastangle >= 2*math.pi: lastangle -= 2*math.pi
        # compute delta = change in angle:
        d0 = angle-lastangle
        d1 = d0 + 2*math.pi
        d2 = d0 - 2*math.pi
        if abs(d0) < min(abs(d1), abs(d2)):
            delta = d0
        elif abs(d1) < abs(d2):
            delta = d1
        else:
            delta = d2
        newangle = self.angle + delta
        # normalize angle, preserving direction
        if newangle >= 2*math.pi: newangle -= 2*math.pi
        if newangle <= -2*math.pi: newangle += 2*math.pi
        self.angle = newangle

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            point,ctrlPoint = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            self.ui.cross(True)
            if self.center and fcvec.dist(point,self.center) > 0:
                viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                if not fcvec.isNull(viewdelta):
                    point = point.add(fcvec.neg(viewdelta))
            if (self.step == 0): # choose center
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.ui.cross(False)
                        self.altdown = True
                        self.ui.switchUi(True)
                    else:
                        if self.altdown:
                            self.ui.cross(True)
                            self.altdown = False
                            self.ui.switchUi(False)
            elif (self.step == 1): # choose radius
                if len(self.tangents) == 2:
                    cir = fcgeo.circleFrom2tan1pt(self.tangents[0], self.tangents[1], point)
                    self.center = fcgeo.findClosestCircle(point,cir).Center
                    self.arctrack.setCenter(self.center)
                elif self.tangents and self.tanpoints:
                    cir = fcgeo.circleFrom1tan2pt(self.tangents[0], self.tanpoints[0], point)
                    self.center = fcgeo.findClosestCircle(point,cir).Center
                    self.arctrack.setCenter(self.center)
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.ui.cross(False)
                        self.altdown = True
                    snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                    if snapped:
                        ob = self.doc.getObject(snapped['Object'])
                        num = int(snapped['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        if len(self.tangents) == 2:
                            cir = fcgeo.circleFrom3tan(self.tangents[0], self.tangents[1], ed)
                            cl = fcgeo.findClosestCircle(point,cir)
                            self.center = cl.Center
                            self.rad = cl.Radius
                            self.arctrack.setCenter(self.center)
                        else:
                            self.rad = self.center.add(fcgeo.findDistance(self.center,ed).sub(self.center)).Length
                    else:
                        self.rad = fcvec.dist(point,self.center)
                else:
                    if self.altdown:
                        self.ui.cross(True)
                        self.altdown = False
                    self.rad = fcvec.dist(point,self.center)
                self.ui.setRadiusValue(self.rad)
                self.arctrack.setRadius(self.rad)
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.linetrack.p1(self.center)
                self.linetrack.p2(point)
                self.linetrack.on()
            elif (self.step == 2): # choose first angle
                currentrad = fcvec.dist(point,self.center)
                if currentrad != 0:
                    angle = fcvec.angle(plane.u, point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(fcvec.scaleTo(point.sub(self.center),self.rad).add(self.center))
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.ui.setRadiusValue(math.degrees(angle))
                self.firstangle = angle
            else: # choose second angle
                currentrad = fcvec.dist(point,self.center)
                if currentrad != 0:
                    angle = fcvec.angle(plane.u, point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(fcvec.scaleTo(point.sub(self.center),self.rad).add(self.center))
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.ui.setRadiusValue(math.degrees(angle))
                self.updateAngle(angle)
                self.arctrack.setApertureAngle(self.angle)

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg)
                # this is to make sure radius is what you see on screen
                if self.center and fcvec.dist(point,self.center) > 0:
                    viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                    if not fcvec.isNull(viewdelta):
                        point = point.add(fcvec.neg(viewdelta))
                if (self.step == 0): # choose center
                    self.support = getSupport(arg)
                    if hasMod(arg,MODALT):
                        snapped=self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                        if snapped:
                            ob = self.doc.getObject(snapped['Object'])
                            num = int(snapped['Component'].lstrip('Edge'))-1
                            ed = ob.Shape.Edges[num]
                            self.tangents.append(ed)
                            if len(self.tangents) == 2:
                                self.arctrack.on()
                                self.ui.radiusUi()
                                self.step = 1
                                self.linetrack.on()
                                msg(translate("draft", "Pick radius:\n"))
                    else:
                        if len(self.tangents) == 1:
                            self.tanpoints.append(point)
                        else:
                            self.center = point
                            self.node = [point]
                            self.arctrack.setCenter(self.center)
                            self.linetrack.p1(self.center)
                            self.linetrack.p2(self.view.getPoint(arg["Position"][0],arg["Position"][1]))
                        self.arctrack.on()
                        self.ui.radiusUi()
                        self.step = 1
                        self.linetrack.on()
                        msg(translate("draft", "Pick radius:\n"))
                        self.planetrack.set(point)                        
                elif (self.step == 1): # choose radius
                    if self.closedCircle:
                        self.ui.cross(False)
                        self.drawArc()
                    else: 
                        self.ui.labelRadius.setText("Start angle")
                        self.linetrack.p1(self.center)
                        self.linetrack.on()
                        self.step = 2
                        msg(translate("draft", "Pick start angle:\n"))
                elif (self.step == 2): # choose first angle
                    self.ui.labelRadius.setText("Aperture")
                    self.step = 3
                    # scale center->point vector for proper display
                    u = fcvec.scaleTo(point.sub(self.center), self.rad)
                    self.arctrack.setStartAngle(self.firstangle)
                    msg(translate("draft", "Pick aperture:\n"))
                else: # choose second angle
                    self.step = 4
                    self.drawArc()

    def drawArc(self):
        "actually draws the FreeCAD object"
        p = plane.getRotation()
        p.move(self.center)
        if self.closedCircle:
            self.commit(translate("draft","Create Circle"),
                        partial(Draft.makeCircle,self.rad,p,
                                self.ui.hasFill.isChecked(),support=self.support))
        else:
            sta = math.degrees(self.firstangle)
            end = math.degrees(self.firstangle+self.angle)
            print "debug:",sta, end
            if end < sta: sta,end = end,sta
            self.commit(translate("draft","Create Arc"),
                        partial(Draft.makeCircle,self.rad,p,self.ui.hasFill.isChecked(),
                                sta,end,support=self.support))
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.radiusValue.setFocus()
        msg(translate("draft", "Pick radius:\n"))
		
    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
        if (self.step == 1):
            self.rad = rad
            if len(self.tangents) == 2:
                cir = fcgeo.circleFrom2tan1rad(self.tangents[0], self.tangents[1], rad)
                if self.center:
                    self.center = fcgeo.findClosestCircle(self.center,cir).Center
                else:
                    self.center = cir[-1].Center
            elif self.tangents and self.tanpoints:
                cir = fcgeo.circleFrom1tan1pt1rad(self.tangents[0],self.tanpoints[0],rad)
                if self.center:
                    self.center = fcgeo.findClosestCircle(self.center,cir).Center
                else:
                    self.center = cir[-1].Center
            if self.closedCircle:
                self.drawArc()
            else:
                self.step = 2
                self.arctrack.setCenter(self.center)
                self.ui.labelRadius.setText("Start angle")
                self.linetrack.p1(self.center)
                self.linetrack.on()
                self.ui.radiusValue.setText("")
                self.ui.radiusValue.setFocus()
                msg(translate("draft", "Pick start angle:\n"))
        elif (self.step == 2):
            self.ui.labelRadius.setText("Aperture")
            self.firstangle = math.radians(rad)
            if fcvec.equals(plane.axis, Vector(1,0,0)): u = Vector(0,self.rad,0)
            else: u = fcvec.scaleTo(Vector(1,0,0).cross(plane.axis), self.rad)
            urotated = fcvec.rotate(u, math.radians(rad), plane.axis)
            self.arctrack.setStartAngle(self.firstangle)
            self.step = 3
            self.ui.radiusValue.setText("")
            self.ui.radiusValue.setFocus()
            msg(translate("draft", "Aperture angle:\n"))
        else:
            self.updateAngle(rad)
            self.angle = math.radians(rad)
            self.step = 4
            self.drawArc()

            
class Circle(Arc):
    "The Draft_Circle FreeCAD command definition"
        
    def __init__(self):
        self.closedCircle=True
        self.featureName = "Circle"
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Circle',
                'Accel' : "C, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Circle"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Circle", "Creates a circle. CTRL to snap, ALT to select tangent objects")}


class Polygon(Creator):
    "the Draft_Polygon FreeCAD command definition"
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Polygon',
                'Accel' : "P, G",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Polygon"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Polygon", "Creates a regular polygon. CTRL to snap, SHIFT to constrain")}

    def Activated(self):
        Creator.Activated(self,"Polygon")
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.tangents = []
            self.tanpoints = []
            self.ui.pointUi()
            self.ui.extUi()
            self.ui.numFaces.show()
            self.altdown = False
            self.ui.sourceCmd = self
            self.linetrack = lineTracker(dotted=True)
            self.constraintrack = lineTracker(dotted=True)
            self.arctrack = arcTracker()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick center point:\n"))

    def finish(self,closed=False,cont=False):
        "finishes the arc"
        Creator.finish(self)
        if self.ui:
            self.linetrack.finalize()
            self.constraintrack.finalize()
            self.arctrack.finalize()
            self.doc.recompute()
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            point,ctrlPoint = getPoint(self,arg)
            # this is to make sure radius is what you see on screen
            self.ui.cross(True)
            if self.center and fcvec.dist(point,self.center) > 0:
                viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                if not fcvec.isNull(viewdelta):
                    point = point.add(fcvec.neg(viewdelta))
            if (self.step == 0): # choose center
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.ui.cross(False)
                        self.altdown = True
                        self.ui.switchUi(True)
                else:
                    if self.altdown:
                        self.ui.cross(True)
                        self.altdown = False
                        self.ui.switchUi(False)
            else: # choose radius
                if len(self.tangents) == 2:
                    cir = fcgeo.circleFrom2tan1pt(self.tangents[0], self.tangents[1], point)
                    self.center = fcgeo.findClosestCircle(point,cir).Center
                    self.arctrack.setCenter(self.center)
                elif self.tangents and self.tanpoints:
                    cir = fcgeo.circleFrom1tan2pt(self.tangents[0], self.tanpoints[0], point)
                    self.center = fcgeo.findClosestCircle(point,cir).Center
                    self.arctrack.setCenter(self.center)
                if hasMod(arg,MODALT):
                    if not self.altdown:
                        self.ui.cross(False)
                        self.altdown = True
                    snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                    if snapped:
                        ob = self.doc.getObject(snapped['Object'])
                        num = int(snapped['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        if len(self.tangents) == 2:
                            cir = fcgeo.circleFrom3tan(self.tangents[0], self.tangents[1], ed)
                            cl = fcgeo.findClosestCircle(point,cir)
                            self.center = cl.Center
                            self.rad = cl.Radius
                            self.arctrack.setCenter(self.center)
                        else:
                            self.rad = self.center.add(fcgeo.findDistance(self.center,ed).sub(self.center)).Length
                    else:
                        self.rad = fcvec.dist(point,self.center)
                else:
                    if self.altdown:
                        self.ui.cross(True)
                        self.altdown = False
                    self.rad = fcvec.dist(point,self.center)
                self.ui.setRadiusValue(self.rad)
                self.arctrack.setRadius(self.rad)
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else: self.constraintrack.off()
                self.linetrack.p1(self.center)
                self.linetrack.p2(point)
                self.linetrack.on()

        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg)
                # this is to make sure radius is what you see on screen
                if self.center and fcvec.dist(point,self.center) > 0:
                    viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                    if not fcvec.isNull(viewdelta):
                        point = point.add(fcvec.neg(viewdelta))
                if (self.step == 0): # choose center
                    if not self.node: self.support = getSupport(arg)
                    if hasMod(arg,MODALT):
                        snapped=self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                        if snapped:
                            ob = self.doc.getObject(snapped['Object'])
                            num = int(snapped['Component'].lstrip('Edge'))-1
                            ed = ob.Shape.Edges[num]
                            self.tangents.append(ed)
                            if len(self.tangents) == 2:
                                self.arctrack.on()
                                self.ui.radiusUi()
                                self.step = 1
                                self.linetrack.on()
                                msg(translate("draft", "Pick radius:\n"))
                    else:
                        if len(self.tangents) == 1:
                            self.tanpoints.append(point)
                        else:
                            self.center = point
                            self.node = [point]
                            self.arctrack.setCenter(self.center)
                            self.linetrack.p1(self.center)
                            self.linetrack.p2(self.view.getPoint(arg["Position"][0],arg["Position"][1]))
                        self.arctrack.on()
                        self.ui.radiusUi()
                        self.step = 1
                        self.linetrack.on()
                        msg(translate("draft", "Pick radius:\n"))
                        self.planetrack.set(point)
                elif (self.step == 1): # choose radius
                    self.ui.cross(False)
                    self.drawPolygon()

    def drawPolygon(self):
        "actually draws the FreeCAD object"
        p = plane.getRotation()
        p.move(self.center)
        self.commit(translate("draft","Create Polygon"),
                    partial(Draft.makePolygon,self.ui.numFaces.value(),self.rad,
                            True,p,face=self.ui.hasFill.isChecked(),support=self.support))
        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.radiusValue.setFocus()
        msg(translate("draft", "Pick radius:\n"))
		
    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
        self.rad = rad
        if len(self.tangents) == 2:
            cir = fcgeo.circleFrom2tan1rad(self.tangents[0], self.tangents[1], rad)
            if self.center:
                self.center = fcgeo.findClosestCircle(self.center,cir).Center
            else:
                self.center = cir[-1].Center
        elif self.tangents and self.tanpoints:
            cir = fcgeo.circleFrom1tan1pt1rad(self.tangents[0],self.tanpoints[0],rad)
            if self.center:
                self.center = fcgeo.findClosestCircle(self.center,cir).Center
            else:
                self.center = cir[-1].Center
        self.drawPolygon()

        
class Text(Creator):
    "This class creates an annotation feature."

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Text',
                'Accel' : "T, E",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Text"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Text", "Creates an annotation. CTRL to snap")}

    def Activated(self):
        Creator.Activated(self,"Text")
        if self.ui:
            self.dialog = None
            self.text = ''
            self.ui.sourceCmd = self
            self.ui.pointUi()
            self.call = self.view.addEventCallback("SoEvent",self.action)
            self.ui.xValue.setFocus()
            self.ui.xValue.selectAll()
            msg(translate("draft", "Pick location point:\n"))
            FreeCADGui.draftToolBar.show()

    def finish(self,closed=False,cont=False):
        "terminates the operation"
        Creator.finish(self)
        if self.ui:
            del self.dialog
        if cont and self.ui:
            if self.ui.continueMode:
                self.Activated()

    def createObject(self):
        "creates an object in the current doc"
        self.commit(translate("draft","Create Text"),
                    partial(Draft.makeText,self.text,self.node[0]))
        self.finish(cont=True)

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg)
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,dtrlPoint = getPoint(self,arg)
                self.node.append(point)
                self.ui.textUi()
                self.ui.textValue.setFocus()
                self.ui.cross(False)

    def numericInput(self,numx,numy,numz):
        '''this function gets called by the toolbar when valid
        x, y, and z have been entered there'''
        point = Vector(numx,numy,numz)
        self.node.append(point)
        self.ui.textUi()
        self.ui.textValue.setFocus()
        self.ui.cross(False)

        
class Dimension(Creator):
    "The Draft_Dimension FreeCAD command definition"
        
    def __init__(self):
        self.max=2
        self.cont = None
        self.dir = None

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Dimension',
                'Accel' : "D, I",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Dimension", "Dimension"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Dimension", "Creates a dimension. CTRL to snap, SHIFT to constrain, ALT to select a segment")}

    def Activated(self):
        if self.cont:
            self.finish()
        elif self.hasMeasures():
            Creator.Activated(self,"Dimension")
            self.dimtrack = dimTracker()
            self.arctrack = arcTracker()
            self.constraintrack = lineTracker(dotted=True)
            self.createOnMeasures()
            self.finish()
        else:
            Creator.Activated(self,"Dimension")
            if self.ui:
                self.ui.pointUi()
                self.ui.continueCmd.show()
                self.altdown = False
                self.call = self.view.addEventCallback("SoEvent",self.action)
                self.dimtrack = dimTracker()
                self.arctrack = arcTracker()
                self.link = None
                self.edges = []
                self.pts = []
                self.angledata = None
                self.indices = []
                self.center = None
                self.arcmode = False
                self.point2 = None
                self.constraintrack = lineTracker(dotted=True)
                msg(translate("draft", "Pick first point:\n"))
                FreeCADGui.draftToolBar.show()

    def hasMeasures(self):
        "checks if only measurements objects are selected"
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        for o in sel:
            if not o.isDerivedFrom("App::MeasureDistance"):
                return False
        return True

    def finish(self,closed=False):
        "terminates the operation"
        self.cont = None
        self.dir = None
        Creator.finish(self)
        if self.ui:
            self.dimtrack.finalize()
            self.arctrack.finalize()
            self.constraintrack.finalize()

    def createOnMeasures(self):
        for o in FreeCADGui.Selection.getSelection():
            p1 = o.P1
            p2 = o.P2
            pt = o.ViewObject.RootNode.getChildren()[1].getChildren()[0].getChildren()[0].getChildren()[3]
            p3 = Vector(pt.point.getValues()[2].getValue())
            self.commit(translate("draft","Create Dimension"),
                        partial(Draft.makeDimension,p1,p2,p3))
            self.commit(translate("draft","Delete Measurement"),
                        partial(FreeCAD.ActiveDocument.removeObject,o.Name))

    def createObject(self):
        "creates an object in the current doc"
        if self.angledata:
            self.commit(translate("draft","Create Dimension"),
                        partial(Draft.makeAngularDimension,self.center,
                                self.angledata,self.node[-1]))
        elif self.link and (not self.arcmode):
            self.commit(translate("draft","Create Dimension"),
                        partial(Draft.makeDimension,self.link[0],self.link[1],
                                self.link[2],self.node[2]))
        elif self.arcmode:
            self.commit(translate("draft","Create Dimension"),
                        partial(Draft.makeDimension,self.link[0],self.link[1],
                                self.arcmode,self.node[2]))
        else:
            self.commit(translate("draft","Create Dimension"),
                        partial(Draft.makeDimension,self.node[0],self.node[1],
                                self.node[2]))
        if self.ui.continueMode:
            self.cont = self.node[2]
            if not self.dir:
                if self.link:
                    v1 = self.link[0].Shape.Vertexes[self.link[1]].Point
                    v2 = self.link[0].Shape.Vertexes[self.link[2]].Point
                    self.dir = v2.sub(v1)
                else:
                    self.dir = self.node[1].sub(self.node[0])
            self.node = [self.node[1]]
        self.link = None

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            shift = hasMod(arg,MODCONSTRAIN)
            if self.arcmode or self.point2:
                setMod(arg,MODCONSTRAIN,False)
            point,ctrlPoint = getPoint(self,arg)
            self.ui.cross(True)
            if hasMod(arg,MODALT) and (len(self.node)<3):
                self.ui.cross(False)
                self.dimtrack.off()
                if not self.altdown:
                    self.altdown = True
                    self.ui.switchUi(True)
                snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                if snapped:
                    ob = self.doc.getObject(snapped['Object'])
                    if "Edge" in snapped['Component']:
                        num = int(snapped['Component'].lstrip('Edge'))-1
                        ed = ob.Shape.Edges[num]
                        v1 = ed.Vertexes[0].Point
                        v2 = ed.Vertexes[-1].Point
                        self.dimtrack.update([v1,v2,self.cont])
            else:
                self.ui.cross(True)
                if self.node and (len(self.edges) < 2):
                    self.dimtrack.on()
                if len(self.edges) == 2:
                    # angular dimension
                    self.dimtrack.off()
                    r = point.sub(self.center)
                    self.arctrack.setRadius(r.Length)
                    a = self.arctrack.getAngle(point)
                    pair = fcgeo.getBoundaryAngles(a,self.pts)
                    if not (pair[0] < a < pair[1]):
                        self.angledata = [4*math.pi-pair[0],2*math.pi-pair[1]]
                    else:
                        self.angledata = [2*math.pi-pair[0],2*math.pi-pair[1]]
                    self.arctrack.setStartAngle(self.angledata[0])
                    self.arctrack.setEndAngle(self.angledata[1])
                if self.altdown:
                    self.altdown = False
                    self.ui.switchUi(False)
                if self.dir:
                    point = self.node[0].add(fcvec.project(point.sub(self.node[0]),self.dir))
                if len(self.node) == 2:
                    if self.arcmode and self.edges:
                        cen = self.edges[0].Curve.Center
                        rad = self.edges[0].Curve.Radius
                        baseray = point.sub(cen)
                        v2 = fcvec.scaleTo(baseray,rad)
                        v1 = fcvec.neg(v2)
                        if shift:
                            self.node = [cen,cen.add(v2)]
                            self.arcmode = "radius"
                        else:
                            self.node = [cen.add(v1),cen.add(v2)]
                            self.arcmode = "diameter"
                        self.dimtrack.update(self.node)
                # Draw constraint tracker line.
                if shift and (not self.arcmode):
                    if len(self.node) == 2:
                        if not self.point2:
                            self.point2 = self.node[1]
                        else:
                            self.node[1] = self.point2
                        a=abs(point.sub(self.node[0]).getAngle(plane.u))
                        if (a > math.pi/4) and (a <= 0.75*math.pi):
                            self.node[1] = Vector(self.node[0].x,self.node[1].y,self.node[0].z)
                        else:
                            self.node[1] = Vector(self.node[1].x,self.node[0].y,self.node[0].z)
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    if self.point2:
                        self.node[1] = self.point2
                        self.point2 = None
                    self.constraintrack.off()
                # update the dimline
                if self.node and (not self.arcmode):
                    self.dimtrack.update(self.node+[point]+[self.cont])
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg)
                if not self.node: self.support = getSupport(arg)
                if hasMod(arg,MODALT) and (len(self.node)<3):
                    snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                    if snapped:
                        ob = self.doc.getObject(snapped['Object'])
                        if 'Edge' in snapped['Component']:
                            num = int(snapped['Component'].lstrip('Edge'))-1
                            ed = ob.Shape.Edges[num]
                            v1 = ed.Vertexes[0].Point
                            v2 = ed.Vertexes[-1].Point
                            i1 = i2 = None
                            for i in range(len(ob.Shape.Vertexes)):
                                if v1 == ob.Shape.Vertexes[i].Point:
                                    i1 = i
                                if v2 == ob.Shape.Vertexes[i].Point:
                                    i2 = i
                            if (i1 != None) and (i2 != None):
                                self.indices.append(num)
                                if not self.edges:
                                    # nothing snapped yet, we treat it as normal edge-snapped dimension
                                    self.node = [v1,v2]
                                    self.link = [ob,i1,i2]
                                    self.edges.append(ed)
                                    if isinstance(ed.Curve,Part.Circle):
                                        # snapped edge is an arc
                                        self.arcmode = "diameter"
                                        self.link = [ob,num]
                                else:
                                    # there is already a snapped edge, so we start angular dimension
                                    self.edges.append(ed)
                                    self.node.extend([v1,v2]) # self.node now has the 4 endpoints
                                    c = fcgeo.findIntersection(self.node[0],
                                                               self.node[1],
                                                               self.node[2],
                                                               self.node[3],
                                                               True,True)
                                    if c:
                                        self.center = c[0]
                                        self.arctrack.setCenter(self.center)
                                        self.arctrack.on()
                                        for e in self.edges:
                                            for v in e.Vertexes:
                                                self.pts.append(self.arctrack.getAngle(v.Point))
                                        self.link = [self.link[0],ob]
                                    else:
                                        msg(translate("draft", "Edges don't intersect!\n"))
                                        self.finish()                                
                            self.dimtrack.on()
                else:
                    if self.dir:
                        point = self.node[0].add(fcvec.project(point.sub(self.node[0]),self.dir))
                    self.node.append(point)
                self.dimtrack.update(self.node)
                if (len(self.node) == 2):
                    self.point2 = self.node[1]
                if (len(self.node) == 1):
                    self.dimtrack.on()
                    self.planetrack.set(self.node[0])
                elif (len(self.node) == 2) and self.cont:
                    self.node.append(self.cont)
                    self.createObject()
                    if not self.cont: self.finish()
                elif (len(self.node) == 3):
                    # for unlinked arc mode:
                    # if self.arcmode:
                    #        v = self.node[1].sub(self.node[0])
                    #        v = fcvec.scale(v,0.5)
                    #        cen = self.node[0].add(v)
                    #        self.node = [self.node[0],self.node[1],cen]
                    self.createObject()
                    if not self.cont: self.finish()
                elif self.angledata:
                    self.node.append(point)
                    self.createObject()
                    self.finish()

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        point = Vector(numx,numy,numz)
        self.node.append(point)
        self.dimtrack.update(self.node)
        if (len(self.node) == 1):
            self.dimtrack.on()
        elif (len(self.node) == 3):
            self.createObject()
            if not self.cont: self.finish()


#---------------------------------------------------------------------------
# Modifier functions
#---------------------------------------------------------------------------

class Modifier:
    "A generic Modifier Tool, used by modification tools such as move"
    
    def __init__(self):
        self.commitList = []
        
    def Activated(self,name="None"):
        if FreeCAD.activeDraftCommand:
            FreeCAD.activeDraftCommand.finish()
        self.ui = None
        self.call = None
        self.commitList = []
        self.doc = FreeCAD.ActiveDocument
        if not self.doc:
            self.finish()
        else:
            FreeCAD.activeDraftCommand = self
            self.view = FreeCADGui.ActiveDocument.ActiveView
            self.ui = FreeCADGui.draftToolBar
            FreeCADGui.draftToolBar.show()
            rot = self.view.getCameraNode().getField("orientation").getValue()
            upv = Vector(rot.multVec(coin.SbVec3f(0,1,0)).getValue())
            plane.setup(fcvec.neg(self.view.getViewDirection()), Vector(0,0,0), upv)
            self.node = []
            self.ui.sourceCmd = self
            self.constrain = None
            self.obj = None
            self.extendedCopy = False
            self.ui.setTitle(name)
            self.featureName = name
            self.snap = snapTracker()
            self.extsnap = lineTracker(dotted=True)
            self.planetrack = PlaneTracker()
            if Draft.getParam("grid"):
                self.grid = gridTracker()
                self.grid.set()
            else:
                self.grid = None

    def IsActive(self):
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False
		
    def finish(self):
        self.node = []
        self.snap.finalize()
        self.extsnap.finalize()
        FreeCAD.activeDraftCommand = None
        if self.ui:
            self.ui.offUi()
            self.ui.sourceCmd=None
            self.ui.cross(False)
        msg("")
        self.planetrack.finalize()
        if self.grid: self.grid.finalize()
        if self.call:
            self.view.removeEventCallback("SoEvent",self.call)
            self.call = None
        if self.commitList:
            todo.delayCommit(self.commitList)
        self.commitList = []

    def commit(self,name,func):
        "stores partial actions to be committed to the FreeCAD document"
        # print "committing"
        self.commitList.append((name,func))


class Move(Modifier):
    "The Draft_Move FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Move',
                'Accel' : "M, V",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Move"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Move", "Moves the selected objects between 2 points. CTRL to snap, SHIFT to constrain, ALT to copy")}
    
    def Activated(self):
        Modifier.Activated(self,"Move")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.linetrack = None
                self.constraintrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to move\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.ui.pointUi()
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.linetrack = lineTracker()
        self.constraintrack = lineTracker(dotted=True)
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick start point:\n"))
        self.ui.cross(True)

    def finish(self,closed=False,cont=False):
        if self.ui:
            self.ghost.finalize()
            self.linetrack.finalize()
            self.constraintrack.finalize()
        Modifier.finish(self)
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def move(self,delta,copy=False):
        "moving the real shapes"
        if copy:
            self.commit(translate("draft","Copy"),partial(Draft.move,self.sel,delta,copy))
        else:
            self.commit(translate("draft","Move"),partial(Draft.move,self.sel,delta,copy))
        self.doc.recompute()

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg)
            self.linetrack.p2(point)
            self.ui.cross(True)
            # Draw constraint tracker line.
            if hasMod(arg,MODCONSTRAIN):
                self.constraintrack.p1(point)
                self.constraintrack.p2(ctrlPoint)
                self.constraintrack.on()
            else: self.constraintrack.off()
            if (len(self.node) > 0):
                last = self.node[len(self.node)-1]
                delta = point.sub(last)
                self.ghost.trans.translation.setValue([delta.x,delta.y,delta.z])
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg)
                if (self.node == []):
                    self.node.append(point)
                    self.ui.isRelative.show()
                    self.linetrack.on()
                    self.ghost.on()
                    self.linetrack.p1(point)
                    msg(translate("draft", "Pick end point:\n"))
                    self.planetrack.set(point)
                else:
                    last = self.node[0]
                    if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                        self.move(point.sub(last),True)
                    else:
                        self.move(point.sub(last))
                    if hasMod(arg,MODALT):
                        self.extendedCopy = True
                    else:
                        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            self.linetrack.p1(point)
            self.linetrack.on()
            self.ghost.on()
            msg(translate("draft", "Pick end point:\n"))
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.move(point.sub(last),True)
            else:
                self.move(point.sub(last))
            self.finish()

			
class ApplyStyle(Modifier):
    "The Draft_ApplyStyle FreeCA command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Apply',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Apply Current Style"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ApplyStyle", "Applies current line width and color to selected objects")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False

    def Activated(self):
        Modifier.Activated(self)
        if self.ui:
            self.sel = Draft.getSelection()
            if (len(self.sel)>0):
                for ob in self.sel:
                    if (ob.Type == "App::DocumentObjectGroup"):
                        self.formatGroup(ob)
                    else:
                        self.commit(translate("draft","Change Style"),partial(Draft.formatObject,ob))

    def formatGroup(self,grpob):
        for ob in grpob.Group:
            if (ob.Type == "App::DocumentObjectGroup"):
                self.formatGroup(ob)
            else:
                self.commit(translate("draft","Change Style"),partial(Draft.formatObject,ob))

			
class Rotate(Modifier):
    "The Draft_Rotate FreeCAD command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Rotate',
                'Accel' : "R, O",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotate"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Rotate", "Rotates the selected objects. CTRL to snap, SHIFT to constrain, ALT creates a copy")}

    def Activated(self):
        Modifier.Activated(self,"Rotate")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.linetrack = None
                self.arctrack = None
                self.constraintrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to rotate\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.step = 0
        self.center = None
        self.ui.arcUi()
        self.ui.isCopy.show()
        self.ui.setTitle("Rotate")
        self.linetrack = lineTracker()
        self.constraintrack = lineTracker(dotted=True)
        self.arctrack = arcTracker()
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick rotation center:\n"))
        self.ui.cross(True)
				
    def finish(self,closed=False,cont=False):
        "finishes the arc"
        Modifier.finish(self)
        if self.ui:
            self.linetrack.finalize()
            self.constraintrack.finalize()
            self.arctrack.finalize()
            self.ghost.finalize()
            self.doc.recompute()
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def rot (self,angle,copy=False):
        "rotating the real shapes"
        if copy:
            self.commit(translate("draft","Copy"),
                        partial(Draft.rotate,self.sel,
                                math.degrees(angle),self.center,plane.axis,copy))
        else:
            self.commit(translate("draft","Rotate"),
                        partial(Draft.rotate,self.sel,
                                math.degrees(angle),self.center,plane.axis,copy))

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            point,ctrlPoint = getPoint(self,arg)
            self.ui.cross(True)
            # this is to make sure radius is what you see on screen
            if self.center and fcvec.dist(point,self.center):
                viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                if not fcvec.isNull(viewdelta):
                    point = point.add(fcvec.neg(viewdelta))
            if self.extendedCopy:
                if not hasMod(arg,MODALT):
                    self.step = 3
                    self.finish()
            if (self.step == 0):
                pass
            elif (self.step == 1):
                currentrad = fcvec.dist(point,self.center)
                if (currentrad != 0):
                    angle = fcvec.angle(plane.u, point.sub(self.center), plane.axis)
                else: angle = 0
                self.linetrack.p2(point)
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.ui.radiusValue.setText("%.2f" % math.degrees(angle))
                self.firstangle = angle
                self.ui.radiusValue.setFocus()
                self.ui.radiusValue.selectAll()
            elif (self.step == 2):
                currentrad = fcvec.dist(point,self.center)
                if (currentrad != 0):
                    angle = fcvec.angle(plane.u, point.sub(self.center), plane.axis)
                else: angle = 0
                if (angle < self.firstangle): 
                    sweep = (2*math.pi-self.firstangle)+angle
                else:
                    sweep = angle - self.firstangle
                self.arctrack.setApertureAngle(sweep)
                self.ghost.trans.rotation.setValue(coin.SbVec3f(fcvec.tup(plane.axis)),sweep)
                self.linetrack.p2(point)
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.ui.radiusValue.setText("%.2f" % math.degrees(sweep))
                self.ui.radiusValue.setFocus()
                self.ui.radiusValue.selectAll()
                
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg)
                if self.center and fcvec.dist(point,self.center):
                    viewdelta = fcvec.project(point.sub(self.center), plane.axis)
                    if not fcvec.isNull(viewdelta): point = point.add(fcvec.neg(viewdelta))
                if (self.step == 0):
                    self.center = point
                    self.node = [point]
                    self.ui.radiusUi()
                    self.ui.hasFill.hide()
                    self.ui.labelRadius.setText("Base angle")
                    self.linetrack.p1(self.center)
                    self.arctrack.setCenter(self.center)
                    self.ghost.trans.center.setValue(self.center.x,self.center.y,self.center.z)
                    self.linetrack.on()
                    self.step = 1
                    msg(translate("draft", "Pick base angle:\n"))
                    self.planetrack.set(point)
                elif (self.step == 1):
                    self.ui.labelRadius.setText("Rotation")
                    self.rad = fcvec.dist(point,self.center)
                    self.arctrack.on()
                    self.arctrack.setStartPoint(point)
                    self.ghost.on()
                    self.step = 2
                    msg(translate("draft", "Pick rotation angle:\n"))
                else:
                    currentrad = fcvec.dist(point,self.center)
                    angle = point.sub(self.center).getAngle(plane.u)
                    if fcvec.project(point.sub(self.center), plane.v).getAngle(plane.v) > 1:
                        angle = -angle
                    if (angle < self.firstangle): 
                        sweep = (2*math.pi-self.firstangle)+angle
                    else:
                        sweep = angle - self.firstangle
                    if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                        self.rot(sweep,True)
                    else:
                        self.rot(sweep)
                    if hasMod(arg,MODALT):
                        self.extendedCopy = True
                    else:
                        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        self.center = Vector(numx,numy,numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.ghost.trans.center.setValue(self.center.x,self.center.y,self.center.z)
        self.linetrack.p1(self.center)
        # self.arctrack.on()
        self.linetrack.on()
        self.ui.radiusUi()
        self.ui.hasFill.hide()
        self.ui.labelRadius.setText("Base angle")
        self.step = 1
        msg(translate("draft", "Pick base angle:\n"))

    def numericRadius(self,rad):
        "this function gets called by the toolbar when valid radius have been entered there"
        if (self.step == 1):
            self.ui.labelRadius.setText("Rotation")
            self.firstangle = math.radians(rad)
            self.arctrack.setStartAngle(self.firstangle)
            self.arctrack.on()
            self.ghost.on()
            self.step = 2
            msg(translate("draft", "Pick rotation angle:\n"))
        else:
            self.rot(math.radians(rad),self.ui.isCopy.isChecked())
            self.finish(cont=True)


class Offset(Modifier):
    "The Draft_Offset FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Offset',
                'Accel' : "O, S",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offset"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Offset", "Offsets the active object. CTRL to snap, SHIFT to constrain, ALT to copy")}

    def Activated(self):
        self.running = False
        Modifier.Activated(self,"Offset")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.linetrack = None
                self.arctrack = None
                self.constraintrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to offset\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            elif len(Draft.getSelection()) > 1:
                msg(translate("draft", "Offset only works on one object at a time\n"),"warning")
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()[0]
        if not self.sel.isDerivedFrom("Part::Feature"):
            msg(translate("draft", "Cannot offset this object type\n"),"warning")
            self.finish()
        else:
            self.step = 0
            self.dvec = None
            self.constrainSeg = None
            self.ui.offsetUi()
            self.linetrack = lineTracker()
            self.constraintrack = lineTracker(dotted=True)
            self.faces = False
            self.shape = self.sel.Shape
            self.mode = None
            if Draft.getType(self.sel) in ["Circle","Arc"]:
                self.ghost = arcTracker()
                self.mode = "Circle"
                self.center = self.shape.Edges[0].Curve.Center
                self.ghost.setCenter(self.center)
                self.ghost.setStartAngle(math.radians(self.sel.FirstAngle))
                self.ghost.setEndAngle(math.radians(self.sel.LastAngle))
            else:
                self.ghost = wireTracker(self.shape)
                self.mode = "Wire"
            self.call = self.view.addEventCallback("SoEvent",self.action)
            msg(translate("draft", "Pick distance:\n"))
            self.ui.cross(True)
            self.planetrack.set(self.shape.Vertexes[0].Point)
            self.running = True

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":
            self.ui.cross(True)
            point,ctrlPoint = getPoint(self,arg)
            if hasMod(arg,MODCONSTRAIN) and self.constrainSeg:
                dist = fcgeo.findPerpendicular(point,self.shape,self.constrainSeg[1])
                e = self.shape.Edges[self.constrainSeg[1]]
                self.constraintrack.p1(e.Vertexes[0].Point)
                self.constraintrack.p2(point.add(dist[0]))
                self.constraintrack.on()
            else:
                dist = fcgeo.findPerpendicular(point,self.shape.Edges)
                self.constraintrack.off()
            if dist:
                self.ghost.on()
                if self.mode == "Wire":
                    d = fcvec.neg(dist[0])
                    v1 = fcgeo.getTangent(self.shape.Edges[0],point)
                    v2 = fcgeo.getTangent(self.shape.Edges[dist[1]],point)
                    a = -fcvec.angle(v1,v2)
                    self.dvec = fcvec.rotate(d,a,plane.axis)
                    self.ghost.update(fcgeo.offsetWire(self.shape,self.dvec,occ=self.ui.occOffset.isChecked()))
                elif self.mode == "Circle":
                    self.dvec = point.sub(self.center).Length
                    self.ghost.setRadius(self.dvec)
                self.constrainSeg = dist
                self.linetrack.on()
                self.linetrack.p1(point)
                self.linetrack.p2(point.add(dist[0]))
                self.ui.radiusValue.setText("%.2f" % dist[0].Length)
            else:
                self.dvec = None
                self.ghost.off()
                self.constrainSeg = None
                self.linetrack.off()
                self.ui.radiusValue.setText("off")
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
                                
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                copymode = False
                occmode = self.ui.occOffset.isChecked()
                if hasMod(arg,MODALT) or self.ui.isCopy.isChecked(): copymode = True
                if self.dvec:
                    self.commit(translate("draft","Offset"),
                                partial(Draft.offset,self.sel,
                                        self.dvec,copymode,occ=occmode))
                if hasMod(arg,MODALT):
                    self.extendedCopy = True
                else:
                    self.finish()
                                        
    def finish(self,closed=False):
        Modifier.finish(self)
        if self.ui and self.running:
            self.linetrack.finalize()
            self.constraintrack.finalize()
            self.ghost.finalize()

    def numericRadius(self,rad):
        '''this function gets called by the toolbar when
        valid radius have been entered there'''
        if self.dvec:
            self.dvec.normalize()
            self.dvec.multiply(rad)
            copymode = False
            occmode = self.ui.occOffset.isChecked()
            if self.ui.isCopy.isChecked(): copymode = True
            self.commit(translate("draft","Offset"),
                        partial(Draft.offset,self.sel,
                                self.dvec,copymode,occ=occmode))
            self.finish()

            
class Upgrade(Modifier):
    '''The Draft_Upgrade FreeCAD command definition.
    This class upgrades selected objects in different ways,
    following this list (in order):
    - if there are more than one faces, the faces are merged (union)
    - if there is only one face, nothing is done
    - if there are closed wires, they are transformed in a face
    - otherwise join all edges into a wire (closed if applicable)
    - if nothing of the above is possible, a Compound is created
    '''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Upgrade',
                'Accel' : "U, P",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Upgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Upgrade", "Joins the selected objects into one, or converts closed wires to filled faces, or unite faces")}

    def Activated(self):
        Modifier.Activated(self,"Upgrade")
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to upgrade\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def compound(self):
        # shapeslist = []
        # for ob in self.sel: shapeslist.append(ob.Shape)
        # newob = self.doc.addObject("Part::Feature","Compound")
        # newob.Shape = Part.makeCompound(shapeslist)
        newob = Draft.makeBlock(self.sel)
        self.nodelete = True
        return newob
		
    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        newob = None
        self.nodelete = False
        edges = []
        wires = []
        openwires = []
        faces = []
        groups = []
        curves = []
        facewires = []
        
        # determining what we have in our selection
        for ob in self.sel:
            if ob.Type == "App::DocumentObjectGroup":
                groups.append(ob)
            else:
                if ob.Shape.ShapeType == 'Edge': openwires.append(ob.Shape)
                for f in ob.Shape.Faces:
                    faces.append(f)
                    facewires.extend(f.Wires)
                for w in ob.Shape.Wires:
                    if w.isClosed():
                        wires.append(w)
                    else:
                        openwires.append(w)
                for e in ob.Shape.Edges:
                    if not isinstance(e.Curve,Part.Line):
                        curves.append(e)
                lastob = ob
        # print "objects:",self.sel," edges:",edges," wires:",wires," openwires:",openwires," faces:",faces
        # print "groups:",groups," curves:",curves," facewires:",facewires
                
        # applying transformation
        self.doc.openTransaction("Upgrade")
        
        if groups:
            # if we have a group: turn each closed wire inside into a face
            msg(translate("draft", "Found groups: closing each open object inside\n"))
            for grp in groups: 
                for ob in grp.Group:
                    if not ob.Shape.Faces:
                        for w in ob.Shape.Wires:
                            newob = Draft.makeWire(w,closed=w.isClosed())
                            self.sel.append(ob)
                            grp.addObject(newob)
                            
        elif faces and (len(wires)+len(openwires)==len(facewires)):
            # we have only faces here, no lone edges
            
            if (len(self.sel) == 1) and (len(faces) > 1):
                # we have a shell: we try to make a solid
                sol = Part.makeSolid(self.sel[0].Shape)
                if sol.isClosed():
                    msg(translate("draft", "Found 1 solidificable object: solidifying it\n"))
                    newob = self.doc.addObject("Part::Feature","Solid")
                    newob.Shape = sol
                    Draft.formatObject(newob,lastob)
                        
            elif (len(self.sel) == 2) and (not curves):
                # we have exactly 2 objects: we fuse them
                msg(translate("draft", "Found 2 objects: fusing them\n"))
                newob = Draft.fuse(self.sel[0],self.sel[1])
                self.nodelete = True
                                
            elif (len(self.sel) > 2) and (len(faces) > 10):
                # we have many separate faces: we try to make a shell
                sh = Part.makeShell(faces)
                newob = self.doc.addObject("Part::Feature","Shell")
                newob.Shape = sh
                Draft.formatObject(newob,lastob)
                
            elif (len(self.sel) > 2) or (len(faces) > 1):
                # more than 2 objects or faces: we try the draft way: make one face out of them
                u = faces.pop(0)
                for f in faces:
                    u = u.fuse(f)
                if fcgeo.isCoplanar(faces):
                    if self.sel[0].ViewObject.DisplayMode == "Wireframe":
                        f = False
                    else:
                        f = True
                    u = fcgeo.concatenate(u)
                    if not curves:
                        msg(translate("draft", "Found several objects or faces: making a parametric face\n"))
                        newob = Draft.makeWire(u.Wires[0],closed=True,face=f)
                        Draft.formatObject(newob,lastob)
                    else:
                        # if not possible, we do a non-parametric union
                        msg(translate("draft", "Found objects containing curves: fusing them\n"))
                        newob = self.doc.addObject("Part::Feature","Union")
                        newob.Shape = u
                        Draft.formatObject(newob,lastob)
                else:
                    # if not possible, we do a non-parametric union
                    msg(translate("draft", "Found several objects: fusing them\n"))
                    # if we have a solid, make sure we really return a solid
                    if (len(u.Faces) > 1) and u.isClosed():
                        u = Part.makeSolid(u)
                    newob = self.doc.addObject("Part::Feature","Union")
                    newob.Shape = u
                    Draft.formatObject(newob,lastob)
            elif len(self.sel) == 1:
                # only one object: if not parametric, we "draftify" it
                self.nodelete = True
                if (not curves) and (Draft.getType(self.sel[0]) == "Part"):
                    msg(translate("draft", "Found 1 non-parametric objects: draftifying it\n"))
                    Draft.draftify(self.sel[0])
                                        
        elif wires and (not faces) and (not openwires):
            # we have only wires, no faces
            
            if (len(self.sel) == 1) and self.sel[0].isDerivedFrom("Sketcher::SketchObject") and (not curves):
                # we have a sketch
                msg(translate("draft", "Found 1 closed sketch object: making a face from it\n"))
                newob = Draft.makeWire(self.sel[0].Shape,closed=True)
                newob.Base = self.sel[0]
                self.sel[0].ViewObject.Visibility = False
                self.nodelete = True
                
            else:
                # only closed wires
                for w in wires:
                    f = Part.Face(w)
                    faces.append(f)
                for f in faces:
                    if not curves: 
                        newob = Draft.makeWire(f.Wire,closed=True)
                    else:
                        # if there are curved segments, we do a non-parametric face
                        msg(translate("draft", "Found closed wires: making faces\n"))
                        newob = self.doc.addObject("Part::Feature","Face")
                        newob.Shape = f
                        Draft.formatObject(newob,lastob)
                        
        elif (len(openwires) == 1) and (not faces) and (not wires):
            # special case, we have only one open wire. We close it!"
            p0 = openwires[0].Vertexes[0].Point
            p1 = openwires[0].Vertexes[-1].Point
            edges = openwires[0].Edges
            edges.append(Part.Line(p1,p0).toShape())
            w = Part.Wire(fcgeo.sortEdges(edges))
            msg(translate("draft", "Found 1 open wire: closing it\n"))
            if not curves:
                newob = Draft.makeWire(w,closed=True)
            else:
                # if not possible, we do a non-parametric union
                newob = self.doc.addObject("Part::Feature","Wire")
                newob.Shape = w
                Draft.formatObject(newob,lastob)
                
        elif openwires and (not wires) and (not faces):
            # only open wires and edges: we try to join their edges
            for ob in self.sel:
                for e in ob.Shape.Edges:
                    edges.append(e)
            newob = None
            nedges = fcgeo.sortEdges(edges[:])
            # for e in nedges: print "debug: ",e.Curve,e.Vertexes[0].Point,e.Vertexes[-1].Point
            w = Part.Wire(nedges)
            if len(w.Edges) == len(edges):
                msg(translate("draft", "Found several edges: wiring them\n"))
                if not curves:
                    newob = Draft.makeWire(w)
                else:
                    newob = self.doc.addObject("Part::Feature","Wire")
                    newob.Shape = w
                    Draft.formatObject(newob,lastob)
            if not newob:
                print "no new object found"
                msg(translate("draft", "Found several non-connected edges: making compound\n"))
                newob = self.compound()
                Draft.formatObject(newob,lastob)
        else:
            # all other cases
            msg(translate("draft", "Found several non-treatable objects: making compound\n"))
            newob = self.compound()
            Draft.formatObject(newob,lastob)
            
        if not self.nodelete:
            # deleting original objects, if needed
            for ob in self.sel:
                if not ob.Type == "App::DocumentObjectGroup":
                    self.doc.removeObject(ob.Name)
                    
        self.doc.commitTransaction()
        if newob: Draft.select(newob)
        Modifier.finish(self)

        
class Downgrade(Modifier):
    '''
    The Draft_Downgrade FreeCAD command definition.
    This class downgrades selected objects in different ways,
    following this list (in order):
    - if there are more than one faces, the subsequent
    faces are subtracted from the first one
    - if there is only one face, it gets converted to a wire
    - otherwise wires are exploded into single edges
    '''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Downgrade',
                'Accel' : "D, N",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Downgrade"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Downgrade", "Explodes the selected objects into simpler objects, or subtract faces")}

    def Activated(self):
        Modifier.Activated(self,"Downgrade")
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to upgrade\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        self.sel = Draft.getSelection()
        edges = []
        faces = []

        # scanning objects
        for ob in self.sel:
            for f in ob.Shape.Faces:
                faces.append(f)
        for ob in self.sel:
            for e in ob.Shape.Edges:
                edges.append(e)
            lastob = ob
                        
        # applying transformation
        self.doc.openTransaction("Downgrade")
        
        if (len(self.sel) == 1) and (Draft.getType(self.sel[0]) == "Block"):
            # a block, we explode it
            pl = self.sel[0].Placement
            newob = []
            for ob in self.sel[0].Components:
                ob.ViewObject.Visibility = True
                ob.Placement = ob.Placement.multiply(pl)
                newob.append(ob)
            self.doc.removeObject(self.sel[0].Name)
            
        elif (len(self.sel) == 1) and (self.sel[0].isDerivedFrom("Part::Feature")) and ("Base" in self.sel[0].PropertiesList):
            # special case, we have one parametric object: we "de-parametrize" it
            msg(translate("draft", "Found 1 parametric object: breaking its dependencies\n"))
            newob = Draft.shapify(self.sel[0])
            
        elif len(self.sel) == 2:
            # we have only 2 objects: cut 2nd from 1st
            msg(translate("draft", "Found 2 objects: subtracting them\n"))
            newob = Draft.cut(self.sel[0],self.sel[1])
            
        elif (len(faces) > 1):
            
            if len(self.sel) == 1:
                # one object with several faces: split it
                for f in faces:
                    msg(translate("draft", "Found several faces: splitting them\n"))
                    newob = self.doc.addObject("Part::Feature","Face")
                    newob.Shape = f
                    Draft.formatObject(newob,self.sel[0])
                self.doc.removeObject(ob.Name)
                
            else:
                # several objects: remove all the faces from the first one
                msg(translate("draft", "Found several objects: subtracting them from the first one\n"))
                u = faces.pop(0)
                for f in faces:
                    u = u.cut(f)
                newob = self.doc.addObject("Part::Feature","Subtraction")
                newob.Shape = u
                for ob in self.sel:
                    Draft.formatObject(newob,ob)
                    self.doc.removeObject(ob.Name)
                    
        elif (len(faces) > 0):
            # only one face: we extract its wires
            msg(translate("draft", "Found 1 face: extracting its wires\n"))
            for w in faces[0].Wires:
                newob = self.doc.addObject("Part::Feature","Wire")
                newob.Shape = w
                Draft.formatObject(newob,lastob)
            for ob in self.sel:
                self.doc.removeObject(ob.Name)
                
        else:
            # no faces: split wire into single edges
            msg(translate("draft", "Found only wires: extracting their edges\n"))
            for ob in self.sel:
                for e in edges:
                    newob = self.doc.addObject("Part::Feature","Edge")
                    newob.Shape = e
                    Draft.formatObject(newob,ob)
                self.doc.removeObject(ob.Name)
        self.doc.commitTransaction()
        Draft.select(newob)
        Modifier.finish(self)


class Trimex(Modifier):
    ''' The Draft_Trimex FreeCAD command definition.
    This tool trims or extends lines, wires and arcs,
    or extrudes single faces. SHIFT constrains to the last point
    or extrudes in direction to the face normal.'''

    def GetResources(self):
        return {'Pixmap' : 'Draft_Trimex',
                'Accel' : "T, R",
                'MenuText' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trimex"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_Trimex", "Trims or extends the selected object, or extrudes single faces. CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts")}

    def Activated(self):
        Modifier.Activated(self,"Trimex")
        self.edges = []
        self.placement = None
        self.ghost = None
        self.linetrack = None
        self.constraintrack = None
        if self.ui:
            if not Draft.getSelection():
                self.ui.selectUi()
                msg(translate("draft", "Select an object to trim/extend\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.obj = Draft.getSelection()[0]
        self.ui.radiusUi()
        self.ui.labelRadius.setText("Distance")
        self.ui.radiusValue.setFocus()
        self.ui.radiusValue.selectAll()
        self.linetrack = lineTracker()
        self.constraintrack = lineTracker(dotted=True)
        if not "Shape" in self.obj.PropertiesList: return
        if "Placement" in self.obj.PropertiesList:
            self.placement = self.obj.Placement
        if len(self.obj.Shape.Faces) == 1:
            # simple extrude mode, the object itself is extruded
            self.extrudeMode = True
            self.ghost = [ghostTracker([self.obj])]
            self.normal = self.obj.Shape.Faces[0].normalAt(.5,.5)
            for v in self.obj.Shape.Vertexes:
                self.ghost.append(lineTracker())
        elif len(self.obj.Shape.Faces) > 1:
            # face extrude mode, a new object is created
            ss =  FreeCADGui.Selection.getSelectionEx()[0]
            if len(ss.SubObjects) == 1:
                if ss.SubObjects[0].ShapeType == "Face":
                    self.obj = self.doc.addObject("Part::Feature","Face")
                    self.obj.Shape = ss.SubObjects[0]
                    self.extrudeMode = True
                    self.ghost = [ghostTracker([self.obj])]
                    self.normal = self.obj.Shape.Faces[0].normalAt(.5,.5)
                    for v in self.obj.Shape.Vertexes:
                        self.ghost.append(lineTracker())
        else:
            # normal wire trimex mode
            self.obj.ViewObject.Visibility = False
            self.extrudeMode = False
            if self.obj.Shape.Wires:
                self.edges = self.obj.Shape.Wires[0].Edges
                self.edges = fcgeo.sortEdges(self.edges)
            else:
                self.edges = self.obj.Shape.Edges	
            self.ghost = []
            lc = self.obj.ViewObject.LineColor
            sc = (lc[0],lc[1],lc[2])
            sw = self.obj.ViewObject.LineWidth
            for e in self.edges:
                if isinstance(e.Curve,Part.Line):
                    self.ghost.append(lineTracker(scolor=sc,swidth=sw))
                else:
                    self.ghost.append(arcTracker(scolor=sc,swidth=sw))
        if not self.ghost: self.finish()
        for g in self.ghost: g.on()
        self.activePoint = 0
        self.nodes = []
        self.shift = False
        self.alt = False
        self.force = None
        self.cv = None
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick distance:\n"))
        self.ui.cross(True)
				
    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            self.ui.cross(True)
            self.shift = hasMod(arg,MODCONSTRAIN)
            self.alt = hasMod(arg,MODALT)
            wp = not(self.extrudeMode and self.shift)
            self.point = getPoint(self,arg,workingplane=wp)[0]
            if hasMod(arg,MODSNAP): self.snapped = None
            else: self.snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
            if self.extrudeMode:
                dist = self.extrude(self.shift)
            else:
                dist = self.redraw(self.point,self.snapped,self.shift,self.alt)
            self.constraintrack.p1(self.point)
            self.constraintrack.p2(self.newpoint)
            self.constraintrack.on()
            self.ui.radiusValue.setText("%.2f" % dist)
            self.ui.radiusValue.setFocus()
            self.ui.radiusValue.selectAll()
			
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                cursor = arg["Position"]
                self.shift = hasMod(arg,MODCONSTRAIN)
                self.alt = hasMod(arg,MODALT)
                if hasMod(arg,MODSNAP): self.snapped = None
                else: self.snapped = self.view.getObjectInfo((cursor[0],cursor[1]))
                self.trimObject()
                self.finish()

    def extrude(self,shift=False,real=False):
        "redraws the ghost in extrude mode"
        self.newpoint = self.obj.Shape.Faces[0].CenterOfMass
        dvec = self.point.sub(self.newpoint)
        if shift: delta = fcvec.project(dvec,self.normal)
        else: delta = dvec
        if self.force:
            ratio = self.force/delta.Length
            delta.multiply(ratio)
        if real: return delta
        self.ghost[0].trans.translation.setValue([delta.x,delta.y,delta.z])
        for i in range(1,len(self.ghost)):
            base = self.obj.Shape.Vertexes[i-1].Point
            self.ghost[i].p1(base)
            self.ghost[i].p2(base.add(delta))
        return delta.Length
        
    def redraw(self,point,snapped=None,shift=False,alt=False,real=None):
        "redraws the ghost"
        
        # initializing
        reverse = False
        for g in self.ghost: g.off()
        if real: newedges = []
        
        # finding the active point
        vlist = []
        for e in self.edges: vlist.append(e.Vertexes[0].Point)
        vlist.append(self.edges[-1].Vertexes[-1].Point)
        if shift: npoint = self.activePoint
        else: npoint = fcgeo.findClosest(point,vlist)
        if npoint > len(self.edges)/2: reverse = True
        if alt: reverse = not reverse
        self.activePoint = npoint
		
        # sorting out directions
        if reverse and (npoint > 0): npoint = npoint-1
        if (npoint > len(self.edges)-1):
            edge = self.edges[-1]
            ghost = self.ghost[-1]
        else:
            edge = self.edges[npoint]
            ghost = self.ghost[npoint]
        if reverse:
            v1 = edge.Vertexes[-1].Point
            v2 = edge.Vertexes[0].Point
        else:
            v1 = edge.Vertexes[0].Point
            v2 = edge.Vertexes[-1].Point
                        
        # snapping
        if snapped:
            snapped = self.doc.getObject(snapped['Object'])
            pts = []
            for e in snapped.Shape.Edges:
                int = fcgeo.findIntersection(edge,e,True,True)
                if int: pts.extend(int)
            if pts:
                point = pts[fcgeo.findClosest(point,pts)]

        # modifying active edge
        if isinstance(edge.Curve,Part.Line):
            perp = fcgeo.vec(edge).cross(Vector(0,0,1))
            chord = v1.sub(point)
            proj = fcvec.project(chord,perp)
            self.newpoint = Vector.add(point,proj)
            dist = v1.sub(self.newpoint).Length
            ghost.p1(self.newpoint)
            ghost.p2(v2)
            self.ui.labelRadius.setText("Distance")
            if real:
                if self.force:
                    ray = self.newpoint.sub(v1)
                    ray = fcvec.scale(ray,self.force/ray.Length)
                    self.newpoint = Vector.add(v1,ray)
                newedges.append(Part.Line(self.newpoint,v2).toShape())
        else:
            center = edge.Curve.Center
            rad = edge.Curve.Radius
            ang1 = fcvec.angle(v2.sub(center))
            ang2 = fcvec.angle(point.sub(center))
            self.newpoint=Vector.add(center,fcvec.rotate(Vector(rad,0,0),-ang2))
            self.ui.labelRadius.setText("Angle")
            dist = math.degrees(-ang2)
            # if ang1 > ang2: ang1,ang2 = ang2,ang1
            print "last calculated:",math.degrees(-ang1),math.degrees(-ang2)
            ghost.setEndAngle(-ang2)
            ghost.setStartAngle(-ang1)
            ghost.setCenter(center)
            ghost.setRadius(rad)
            if real:
                if self.force:
                    angle = math.radians(self.force)
                    newray = fcvec.rotate(Vector(rad,0,0),-angle)
                    self.newpoint = Vector.add(center,newray)
                chord = self.newpoint.sub(v2)
                perp = chord.cross(Vector(0,0,1))
                scaledperp = fcvec.scaleTo(perp,rad)
                midpoint = Vector.add(center,scaledperp)
                newedges.append(Part.Arc(self.newpoint,midpoint,v2).toShape())
        ghost.on()

        # resetting the visible edges
        if not reverse: list = range(npoint+1,len(self.edges))
        else: list = range(npoint-1,-1,-1)
        for i in list:
            edge = self.edges[i]
            ghost = self.ghost[i]
            if isinstance(edge.Curve,Part.Line):
                ghost.p1(edge.Vertexes[0].Point)
                ghost.p2(edge.Vertexes[-1].Point)
            else:
                ang1 = fcvec.angle(edge.Vertexes[0].Point.sub(center))
                ang2 = fcvec.angle(edge.Vertexes[-1].Point.sub(center))
                # if ang1 > ang2: ang1,ang2 = ang2,ang1
                ghost.setEndAngle(-ang2)
                ghost.setStartAngle(-ang1)
                ghost.setCenter(edge.Curve.Center)
                ghost.setRadius(edge.Curve.Radius)
            if real: newedges.append(edge)
            ghost.on()
			
        # finishing
        if real: return newedges
        else: return dist

    def trimObject(self):
        "trims the actual object"
        if self.extrudeMode:
            delta = self.extrude(self.shift,real=True)
            print "delta",delta
            self.doc.openTransaction("Extrude")
            obj = Draft.extrude(self.obj,delta)
            self.doc.commitTransaction()
            self.obj = obj
        else:
            edges = self.redraw(self.point,self.snapped,self.shift,self.alt,real=True)
            newshape = Part.Wire(edges)
            self.doc.openTransaction("Trim/extend")
            if Draft.getType(self.obj) in ["Wire","BSpline"]:
                p = []
                if self.placement: invpl = self.placement.inverse()
                for v in newshape.Vertexes:
                    np = v.Point
                    if self.placement: np = invpl.multVec(np)
                    p.append(np)
                self.obj.Points = p
            elif Draft.getType(self.obj) == "Circle":
                angles = self.ghost[0].getAngles()
                print "original",self.obj.FirstAngle," ",self.obj.LastAngle
                print "new",angles
                if angles[0] > angles[1]: angles = (angles[1],angles[0])
                self.obj.FirstAngle = angles[0]
                self.obj.LastAngle = angles[1]
            else:
                self.obj.Shape = newshape
            self.doc.commitTransaction()
        for g in self.ghost: g.off()

    def finish(self,closed=False):		
        Modifier.finish(self)
        self.force = None
        if self.ui:
            self.ui.labelRadius.setText("Distance")
            self.linetrack.finalize()
            self.constraintrack.finalize()
            if self.ghost:
                for g in self.ghost:
                    g.finalize()
            self.obj.ViewObject.Visibility = True
            Draft.select(self.obj)

    def numericRadius(self,dist):
        "this function gets called by the toolbar when valid distance have been entered there"
        self.force = dist
        self.trimObject()
        self.finish()

        
class Scale(Modifier):
    '''The Draft_Scale FreeCAD command definition.
    This tool scales the selected objects from a base point.'''

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Scale',
                'Accel' : "S, C",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Scale", "Scale"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Scale", "Scales the selected objects from a base point. CTRL to snap, SHIFT to constrain, ALT to copy")}

    def Activated(self):
        Modifier.Activated(self,"Scale")
        if self.ui:
            if not Draft.getSelection():
                self.ghost = None
                self.linetrack = None
                self.constraintrack = None
                self.ui.selectUi()
                msg(translate("draft", "Select an object to scale\n"))
                self.call = self.view.addEventCallback("SoEvent",selectObject)
            else:
                self.proceed()

    def proceed(self):
        if self.call: self.view.removeEventCallback("SoEvent",self.call)
        self.sel = Draft.getSelection()
        self.sel = Draft.getGroupContents(self.sel)
        self.ui.pointUi()
        self.ui.modUi()
        self.ui.xValue.setFocus()
        self.ui.xValue.selectAll()
        self.linetrack = lineTracker()
        self.constraintrack = lineTracker(dotted=True)
        self.ghost = ghostTracker(self.sel)
        self.call = self.view.addEventCallback("SoEvent",self.action)
        msg(translate("draft", "Pick base point:\n"))
        self.ui.cross(True)

    def finish(self,closed=False,cont=False):
        Modifier.finish(self)
        if self.ui:
            self.ghost.finalize()
            self.linetrack.finalize()
            self.constraintrack.finalize()
        if cont and self.ui:
            if self.ui.continueMode:
                FreeCADGui.Selection.clearSelection()
                self.Activated()

    def scale(self,delta,copy=False):
        "moving the real shapes"
        if copy:
            self.commit(translate("draft","Copy"),
                        partial(Draft.scale,self.sel,delta,self.node[0],copy))
        else:
            self.commit(translate("draft","Scale"),
                        partial(Draft.scale,self.sel,delta,self.node[0],copy))

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            point,ctrlPoint = getPoint(self,arg,sym=True)
            self.linetrack.p2(point)
            self.ui.cross(True)
            # Draw constraint tracker line.
            if hasMod(arg,MODCONSTRAIN):
                self.constraintrack.p1(point)
                self.constraintrack.p2(ctrlPoint)
                self.constraintrack.on()
            else: self.constraintrack.off()
            if (len(self.node) > 0):
                last = self.node[len(self.node)-1]
                delta = point.sub(last)
                self.ghost.trans.scaleFactor.setValue([delta.x,delta.y,delta.z])
                corr = Vector(self.node[0].x,self.node[0].y,self.node[0].z)
                corr.scale(delta.x,delta.y,delta.z)
                corr = fcvec.neg(corr.sub(self.node[0]))
                self.ghost.trans.translation.setValue([corr.x,corr.y,corr.z])
            if self.extendedCopy:
                if not hasMod(arg,MODALT): self.finish()
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                point,ctrlPoint = getPoint(self,arg,sym=True)
                if (self.node == []):
                    self.node.append(point)
                    self.ui.isRelative.show()
                    self.ui.isCopy.show()
                    self.linetrack.on()
                    self.ghost.on()
                    self.linetrack.p1(point)
                    msg(translate("draft", "Pick scale factor:\n"))
                else:
                    last = self.node[0]
                    if self.ui.isCopy.isChecked() or hasMod(arg,MODALT):
                        self.scale(point.sub(last),True)
                    else:
                        self.scale(point.sub(last))
                    if hasMod(arg,MODALT):
                        self.extendedCopy = True
                    else:
                        self.finish(cont=True)

    def numericInput(self,numx,numy,numz):
        "this function gets called by the toolbar when valid x, y, and z have been entered there"
        point = Vector(numx,numy,numz)
        if not self.node:
            self.node.append(point)
            self.ui.isRelative.show()
            self.ui.isCopy.show()
            self.linetrack.p1(point)
            self.linetrack.on()
            self.ghost.on()
            msg(translate("draft", "Pick scale factor:\n"))
        else:
            last = self.node[-1]
            if self.ui.isCopy.isChecked():
                self.scale(point.sub(last),True)
            else:
                self.scale(point.sub(last))
            self.finish(cont=True)

            
class ToggleConstructionMode():
    "The Draft_ToggleConstructionMode FreeCAD command definition"

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggle construcion Mode"),
                'Accel' : "C, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleConstructionMode", "Toggles the Construction Mode for next objects.")}
    
    def Activated(self):
        FreeCADGui.draftToolBar.constrButton.toggle()

        
class ToggleContinueMode():
    "The Draft_ToggleContinueMode FreeCAD command definition"

    def GetResources(self):
        return {'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggle continue Mode"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleContinueMode", "Toggles the Continue Mode for next commands.")}

    def Activated(self):
        FreeCADGui.draftToolBar.continueCmd.toggle()

        
class Drawing(Modifier):
    "The Draft Drawing command definition"
    
    def GetResources(self):
        return {'Pixmap'  : 'Draft_Drawing',
                'Accel' : "D, D",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Drawing"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Drawing", "Puts the selected objects on a Drawing sheet.")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False

    def Activated(self):
        Modifier.Activated(self,"Drawing")
        sel = Draft.getSelection()
        if not sel:
            self.page = self.createDefaultPage()
        else:
            self.page = None
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeaturePage"):
                    self.page = obj
                    sel.pop(sel.index(obj))
            if not self.page:
                for obj in self.doc.Objects:
                    if obj.isDerivedFrom("Drawing::FeaturePage"):
                        self.page = obj
            if not self.page:
                self.page = self.createDefaultPage()
            sel.reverse() 
            for obj in sel:
                self.insertPattern(obj)
                if obj.ViewObject.isVisible():
                    name = 'View'+obj.Name
                    oldobj = self.page.getObject(name)
                    if oldobj: self.doc.removeObject(oldobj.Name)
                    Draft.makeDrawingView(obj,self.page)
            self.doc.recompute()

    def insertPattern(self,obj):
        "inserts a pattern object on the page"
        if 'FillStyle' in obj.ViewObject.PropertiesList:
            if obj.ViewObject.FillStyle != 'shape color':
                hatch = obj.ViewObject.FillStyle
                vobj = self.page.getObject('Pattern'+hatch)
                if not vobj:
                    if hatch in FreeCAD.svgpatterns:
                        view = self.doc.addObject('Drawing::FeatureView','Pattern'+hatch)
                        svg = FreeCAD.svgpatterns[hatch]
                        view.ViewResult = svg
                        view.X = 0
                        view.Y = 0
                        view.Scale = 1
                        self.page.addObject(view)

    def createDefaultPage(self):
        "created a default page"
        template = Draft.getParam("template")
        if not template:
            template = FreeCAD.getResourceDir()+'Mod/Drawing/Templates/A3_Landscape.svg'
        page = self.doc.addObject('Drawing::FeaturePage','Page')
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 20
        page.Template = template
        self.doc.recompute()
        return page

    
class ToggleDisplayMode():
    "The ToggleDisplayMode FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SwitchMode',
                'Accel' : "Shift+Space",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Toggle display mode"),
                'ToolTip' : QtCore.QT_TRANSLATE_NOOP("Draft_ToggleDisplayMode", "Swaps display mode of selected objects between wireframe and flatlines")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        for obj in Draft.getSelection():
            if obj.ViewObject.DisplayMode == "Flat Lines":
                if "Wireframe" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Wireframe"
            elif obj.ViewObject.DisplayMode == "Wireframe":
                if "Flat Lines" in obj.ViewObject.listDisplayModes():
                    obj.ViewObject.DisplayMode = "Flat Lines"


class Edit(Modifier):
    "The Draft_Edit FreeCAD command definition"

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_Edit',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edit"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Edit", "Edits the active object")}

    def IsActive(self):
        if Draft.getSelection():
            self.selection = Draft.getSelection()
            if "Proxy" in self.selection[0].PropertiesList:
                if hasattr(self.selection[0].Proxy,"Type"):
                    return True
        return False

    def Activated(self):
        if self.running:
            self.finish()
        else:
            Modifier.Activated(self,"Edit")
            self.ui.editUi()
            if self.doc:
                self.obj = Draft.getSelection()
                if self.obj:
                    self.obj = self.obj[0]
                    # store selectable state of the object
                    self.selectstate = self.obj.ViewObject.Selectable
                    self.obj.ViewObject.Selectable = False
                    if not Draft.getType(self.obj) in ["Wire","BSpline"]:
                        self.ui.addButton.setEnabled(False)
                        self.ui.delButton.setEnabled(False)
                    else:
                        self.ui.addButton.setEnabled(True)
                        self.ui.delButton.setEnabled(True)
                    # self.ui.addButton.setChecked(False)
                    # self.ui.delButton.setChecked(False)
                    self.editing = None
                    self.editpoints = []
                    self.pl = None
                    if "Placement" in self.obj.PropertiesList:
                        self.pl = self.obj.Placement
                        self.invpl = self.pl.inverse()
                    if Draft.getType(self.obj) in ["Wire","BSpline"]:
                        for p in self.obj.Points:
                            if self.pl: p = self.pl.multVec(p)
                            self.editpoints.append(p)
                    elif Draft.getType(self.obj) == "Circle":
                        self.editpoints.append(self.obj.Placement.Base)
                        if self.obj.FirstAngle == self.obj.LastAngle:
                            self.editpoints.append(self.obj.Shape.Vertexes[0].Point)
                    elif Draft.getType(self.obj) == "Rectangle":
                        self.editpoints.append(self.obj.Placement.Base)
                        self.editpoints.append(self.obj.Shape.Vertexes[2].Point)
                        v = self.obj.Shape.Vertexes
                        self.bx = v[1].Point.sub(v[0].Point)
                        if self.obj.Length < 0: self.bx = fcvec.neg(self.bx)
                        self.by = v[2].Point.sub(v[1].Point)
                        if self.obj.Height < 0: self.by = fcvec.neg(self.by)
                    elif Draft.getType(self.obj) == "Polygon":
                        self.editpoints.append(self.obj.Placement.Base)
                        self.editpoints.append(self.obj.Shape.Vertexes[0].Point)
                    elif Draft.getType(self.obj) == "Dimension":
                        p = self.obj.ViewObject.Proxy.textpos.translation.getValue()
                        self.editpoints.append(self.obj.Start)
                        self.editpoints.append(self.obj.End)
                        self.editpoints.append(self.obj.Dimline)
                        self.editpoints.append(Vector(p[0],p[1],p[2]))
                    self.trackers = []
                    self.constraintrack = None
                    if self.editpoints:
                        for ep in range(len(self.editpoints)):
                            self.trackers.append(editTracker(self.editpoints[ep],self.obj.Name,
                                                             ep,self.obj.ViewObject.LineColor))
                            self.constraintrack = lineTracker(dotted=True)
                            self.call = self.view.addEventCallback("SoEvent",self.action)
                            self.running = True
                            plane.save()
                            if "Shape" in self.obj.PropertiesList:
                                plane.alignToFace(self.obj.Shape)
                            self.planetrack.set(self.editpoints[0])
                    else:
                        msg(translate("draft", "This object type is not editable\n"),'warning')
                        self.finish()
                else:
                    self.finish()

    def finish(self,closed=False):
        "terminates the operation"
        if closed:
            if "Closed" in self.obj.PropertiesList:
                if not self.obj.Closed:
                    self.obj.Closed = True
        if self.ui:
            if self.trackers:
                for t in self.trackers:
                    t.finalize()
            if self.constraintrack:
                self.constraintrack.finalize()
        self.obj.ViewObject.Selectable = self.selectstate
        Modifier.finish(self)
        plane.restore()
        self.running = False

    def action(self,arg):
        "scene event handler"
        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event": #mouse movement detection
            if self.editing != None:
                point,ctrlPoint = getPoint(self,arg)
                # Draw constraint tracker line.
                if hasMod(arg,MODCONSTRAIN):
                    self.constraintrack.p1(point)
                    self.constraintrack.p2(ctrlPoint)
                    self.constraintrack.on()
                else:
                    self.constraintrack.off()
                self.trackers[self.editing].set(point)
                self.update(self.trackers[self.editing].get())
        elif arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                if self.editing == None:
                    snapped = self.view.getObjectInfo((arg["Position"][0],arg["Position"][1]))
                    if snapped:
                        if snapped['Object'] == self.obj.Name:
                            if self.ui.addButton.isChecked():
                                point,ctrlPoint = getPoint(self,arg)
                                self.pos = arg["Position"]
                                self.addPoint(point)
                            elif self.ui.delButton.isChecked():
                                if 'EditNode' in snapped['Component']:
                                    self.delPoint(int(snapped['Component'][8:]))
                            elif 'EditNode' in snapped['Component']:
                                self.ui.pointUi()
                                self.ui.isRelative.show()
                                self.editing = int(snapped['Component'][8:])
                                self.trackers[self.editing].off()
                                self.obj.ViewObject.Selectable = False
                                if "Points" in self.obj.PropertiesList:
                                    self.node.append(self.obj.Points[self.editing])
                else:
                    print "finishing edit"
                    self.trackers[self.editing].on()
                    self.obj.ViewObject.Selectable = True
                    self.numericInput(self.trackers[self.editing].get())

    def update(self,v):
        if Draft.getType(self.obj) in ["Wire","BSpline"]:
            pts = self.obj.Points
            editPnt = self.invpl.multVec(v)
            # DNC: allows to close the curve by placing ends close to each other
            tol = 0.001
            if ( ( self.editing == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( self.editing == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
                self.obj.Closed = True
            # DNC: fix error message if edited point coinsides with one of the existing points
            if ( editPnt in pts ) == False:
                pts[self.editing] = editPnt
                self.obj.Points = pts
                self.trackers[self.editing].set(v)
        elif Draft.getType(self.obj) == "Circle":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.trackers[0].set(self.obj.Placement.Base)
            elif self.editing == 1:
                self.obj.Radius = delta.Length
            self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        elif Draft.getType(self.obj) == "Rectangle":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
            elif self.editing == 1:
                diag = v.sub(self.obj.Placement.Base)
                nx = fcvec.project(diag,self.bx)
                ny = fcvec.project(diag,self.by)
                ax = nx.Length
                ay = ny.Length
                if ax and ay:
                    if abs(nx.getAngle(self.bx)) > 0.1:
                        ax = -ax
                    if abs(ny.getAngle(self.by)) > 0.1:
                        ay = -ay
                    self.obj.Length = ax
                    self.obj.Height = ay
            self.trackers[0].set(self.obj.Placement.Base)
            self.trackers[1].set(self.obj.Shape.Vertexes[2].Point)
        elif Draft.getType(self.obj) == "Polygon":
            delta = v.sub(self.obj.Placement.Base)
            if self.editing == 0:
                p = self.obj.Placement
                p.move(delta)
                self.obj.Placement = p
                self.trackers[0].set(self.obj.Placement.Base)
            elif self.editing == 1:
                if self.obj.DrawMode == 'inscribed':
                    self.obj.Radius = delta.Length
                else:
                    halfangle = ((math.pi*2)/self.obj.FacesNumber)/2
                    rad = math.cos(halfangle)*delta.Length
                    self.obj.Radius = rad
            self.trackers[1].set(self.obj.Shape.Vertexes[0].Point)
        elif Draft.getType(self.obj) == "Dimension":
            if self.editing == 0:
                self.obj.Start = v
            elif self.editing == 1:
                self.obj.End = v
            elif self.editing == 2:
                self.obj.Dimline = v
            elif self.editing == 3:
                self.obj.ViewObject.TextPosition = v        

    def numericInput(self,v,numy=None,numz=None):
        '''this function gets called by the toolbar
        when valid x, y, and z have been entered there'''
        if (numy != None):
            v = Vector(v,numy,numz)
        self.doc.openTransaction("Edit "+self.obj.Name)
        self.update(v)
        self.doc.commitTransaction()
        self.editing = None
        self.ui.editUi()
        self.node = []
       
    def addPoint(self,point):
        if not (Draft.getType(self.obj) in ["Wire","BSpline"]): return
        pts = self.obj.Points
        if ( Draft.getType(self.obj) == "Wire" ):
            if (self.obj.Closed == True):
                # DNC: work around.... seems there is a
                # bug in approximate method for closed wires...
                edges = self.obj.Shape.Wires[0].Edges
                e1 = edges[-1] # last edge
                v1 = e1.Vertexes[0].Point
                v2 = e1.Vertexes[1].Point
                v2.multiply(0.9999)
                edges[-1] = Part.makeLine(v1,v2)
                edges.reverse()
                wire = Part.Wire(edges)
                curve = wire.approximate(0.0001,0.0001,100,25)
            else:
                # DNC: this version is much more reliable near sharp edges!
                curve = self.obj.Shape.Wires[0].approximate(0.0001,0.0001,100,25)
        elif ( Draft.getType(self.obj) == "BSpline" ):
            if (self.obj.Closed == True):
                curve = self.obj.Shape.Edges[0].Curve
            else:
                curve = self.obj.Shape.Curve
        uNewPoint = curve.parameter(point)
        uPoints = []
        for p in self.obj.Points:
            uPoints.append(curve.parameter(p))
        for i in range(len(uPoints)-1):
            if ( uNewPoint > uPoints[i] ) and ( uNewPoint < uPoints[i+1] ):
                pts.insert(i+1, self.invpl.multVec(point))
                break
        # DNC: fix: add points to last segment if curve is closed 
        if ( self.obj.Closed ) and ( uNewPoint > uPoints[-1] ) :
            pts.append(self.invpl.multVec(point))
        self.doc.openTransaction("Edit "+self.obj.Name)
        self.obj.Points = pts
        self.doc.commitTransaction()
        self.resetTrackers()
        
    def delPoint(self,point):
        if not (Draft.getType(self.obj) in ["Wire","BSpline"]): return
        if len(self.obj.Points) <= 2:
            msg(translate("draft", "Active object must have more than two points/nodes\n"),'warning')
        else: 
            pts = self.obj.Points
            pts.pop(point)
            self.doc.openTransaction("Edit "+self.obj.Name)
            self.obj.Points = pts
            self.doc.commitTransaction()
            self.resetTrackers()

    def resetTrackers(self):
        for t in self.trackers:
            t.finalize()
        self.trackers = []
        for ep in range(len(self.obj.Points)):
            objPoints = self.obj.Points[ep]
            if self.pl: objPoints = self.pl.multVec(objPoints)
            self.trackers.append(editTracker(objPoints,self.obj.Name,ep,self.obj.ViewObject.LineColor))

            
class AddToGroup():
    "The AddToGroup FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddToGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Add to group..."),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddToGroup", "Adds the selected object(s) to an existing group")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        self.groups = ["Ungroup"]
        self.groups.extend(Draft.getGroupNames())
        self.labels = ["Ungroup"]
        for g in self.groups:
            o = FreeCAD.ActiveDocument.getObject(g)
            if o: self.labels.append(o.Label)
        self.ui = FreeCADGui.draftToolBar
        self.ui.sourceCmd = self
        self.ui.popupMenu(self.labels)

    def proceed(self,labelname):
        self.ui.sourceCmd = None
        if labelname == "Ungroup":
            for obj in Draft.getSelection():
                try:
                    Draft.ungroup(obj)
                except:
                    pass
        else:
            if labelname in self.labels:
                i = self.labels.index(labelname)
                g = FreeCAD.ActiveDocument.getObject(self.groups[i])
                for obj in Draft.getSelection():
                    try:
                        g.addObject(obj)
                    except:
                        pass

                    
class AddPoint(Modifier):
    "The Draft_AddPoint FreeCAD command definition"

    def __init__(self):
        self.running = False

    def GetResources(self):
        return {'Pixmap'  : 'Draft_AddPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Add Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_AddPoint", "Adds a point to an existing wire/bspline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        FreeCADGui.draftToolBar.addButton.setChecked(True)
        FreeCADGui.draftToolBar.delButton.setChecked(False)
        FreeCADGui.runCommand("Draft_Edit")

        
class DelPoint(Modifier):
    "The Draft_DelPoint FreeCAD command definition"

    def __init__(self):
        self.running = False
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_DelPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Remove Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_DelPoint", "Removes a point from an existing wire or bspline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        FreeCADGui.draftToolBar.addButton.setChecked(False)
        FreeCADGui.draftToolBar.delButton.setChecked(True)
        FreeCADGui.runCommand("Draft_Edit")

        
class WireToBSpline(Modifier):
    "The Draft_Wire2BSpline FreeCAD command definition"

    def __init__(self):
        self.running = False
        
    def GetResources(self):
        return {'Pixmap'  : 'Draft_WireToBSpline',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Wire to BSpline"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Converts between Wire and BSpline")}

    def IsActive(self):
        self.selection = Draft.getSelection()
        if (Draft.getType(self.selection[0]) in ['Wire','BSpline']):
            return True
        else:
            return False

    def Activated(self):
        if self.running:
            self.finish()
        else:
            Modifier.Activated(self,"Convert Curve Type")
            if self.doc:
                self.obj = Draft.getSelection()
                if self.obj:
                    self.obj = self.obj[0]
                    self.pl = None
                    if "Placement" in self.obj.PropertiesList:
                        self.pl = self.obj.Placement
                    self.Points = self.obj.Points
                    self.closed = self.obj.Closed
                    n = None
                    if (Draft.getType(self.selection[0]) == 'Wire'):
                        n = Draft.makeBSpline(self.Points, self.closed, self.pl)
                    elif (Draft.getType(self.selection[0]) == 'BSpline'):
                        n = Draft.makeWire(self.Points, self.closed, self.pl)
                    if n:
                        Draft.formatObject(n,self.selection[0])
                else:
                    self.finish()
	def finish(self):
		Modifier.finish(self)

                
class SelectGroup():
    "The SelectGroup FreeCAD command definition"

    def GetResources(self):
        return {'Pixmap'  : 'Draft_SelectGroup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Select group"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_SelectGroup", "Selects all objects with the same parents as this group")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sellist = []
        for ob in Draft.getSelection():
            for child in ob.OutList:
                FreeCADGui.Selection.addSelection(child)
                for parent in ob.InList:
                    FreeCADGui.Selection.addSelection(parent)
                    for child in parent.OutList:
                        FreeCADGui.Selection.addSelection(child)

                        
class Shape2DView():
    "The Shape2DView FreeCAD command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Draft_2DShapeView',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Shape 2D view"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Draft_Shape2DView", "Creates Shape 2D views of selected objects")}

    def IsActive(self):
        if Draft.getSelection():
            return True
        else:
            return False
        
    def Activated(self):
        sellist = []
        for ob in Draft.getSelection():
            Draft.makeShape2DView(ob)
        
#---------------------------------------------------------------------------
# Adds the icons & commands to the FreeCAD command manager, and sets defaults
#---------------------------------------------------------------------------
		
# drawing commands
FreeCADGui.addCommand('Draft_SelectPlane',SelectPlane())
FreeCADGui.addCommand('Draft_Line',Line())
FreeCADGui.addCommand('Draft_Wire',Wire())
FreeCADGui.addCommand('Draft_Circle',Circle())
FreeCADGui.addCommand('Draft_Arc',Arc())
FreeCADGui.addCommand('Draft_Text',Text())
FreeCADGui.addCommand('Draft_Rectangle',Rectangle())
FreeCADGui.addCommand('Draft_Dimension',Dimension())
FreeCADGui.addCommand('Draft_Polygon',Polygon())
FreeCADGui.addCommand('Draft_BSpline',BSpline())

# modification commands
FreeCADGui.addCommand('Draft_Move',Move())
FreeCADGui.addCommand('Draft_Rotate',Rotate())
FreeCADGui.addCommand('Draft_Offset',Offset())
FreeCADGui.addCommand('Draft_Upgrade',Upgrade())
FreeCADGui.addCommand('Draft_Downgrade',Downgrade())
FreeCADGui.addCommand('Draft_Trimex',Trimex())
FreeCADGui.addCommand('Draft_Scale',Scale())
FreeCADGui.addCommand('Draft_Drawing',Drawing())
FreeCADGui.addCommand('Draft_Edit',Edit())
FreeCADGui.addCommand('Draft_AddPoint',AddPoint())
FreeCADGui.addCommand('Draft_DelPoint',DelPoint())
FreeCADGui.addCommand('Draft_WireToBSpline',WireToBSpline())

# context commands
FreeCADGui.addCommand('Draft_FinishLine',FinishLine())
FreeCADGui.addCommand('Draft_CloseLine',CloseLine())
FreeCADGui.addCommand('Draft_UndoLine',UndoLine())
FreeCADGui.addCommand('Draft_ToggleConstructionMode',ToggleConstructionMode())
FreeCADGui.addCommand('Draft_ToggleContinueMode',ToggleContinueMode())
FreeCADGui.addCommand('Draft_ApplyStyle',ApplyStyle())
FreeCADGui.addCommand('Draft_ToggleDisplayMode',ToggleDisplayMode())
FreeCADGui.addCommand('Draft_AddToGroup',AddToGroup())
FreeCADGui.addCommand('Draft_SelectGroup',SelectGroup())
FreeCADGui.addCommand('Draft_Shape2DView',Shape2DView())

# a global place to look for active draft Command
FreeCAD.activeDraftCommand = None



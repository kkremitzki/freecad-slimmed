#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

import FreeCAD,FreeCADGui,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore
from DraftTools import translate

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

# Make some strings picked by the translator
QtCore.QT_TRANSLATE_NOOP("Arch","Wood")
QtCore.QT_TRANSLATE_NOOP("Arch","Steel")

# Presets in the form: Class, Name, Width, Height, [Web thickness, Flange thickness]
Presets = [None,
           ["Wood","1x2in",19,28],
           ["Wood","1x3in",19,64],
           ["Wood","1x4in",19,89],
           ["Wood","1x6in",19,89],
           ["Wood","1x8in",19,140],
           ["Wood","1x10in",19,184],
           ["Wood","1x12in",19,286],
           
           ["Wood","2x2in",38,38],
           ["Wood","2x3in",38,64],
           ["Wood","2x4in",38,89],
           ["Wood","2x6in",38,140],
           ["Wood","2x8in",38,184],
           ["Wood","2x10in",38,235],
           ["Wood","2x12in",38,286],
           
           ["Wood","4x4in",89,89],
           ["Wood","4x6in",89,140],
           ["Wood","6x6in",140,140],
           ["Wood","8x8in",184,184],
           
           ["Steel","IPE90",46,80,3.8,5.2],
           ["Steel","IPE100",55,100,4.1,5.7]
           ]

def makeStructure(baseobj=None,length=0,width=0,height=0,name=str(translate("Arch","Structure"))):
    '''makeStructure([obj],[length],[width],[heigth],[swap]): creates a
    structure element based on the given profile object and the given
    extrusion height. If no base object is given, you can also specify
    length and width for a cubic object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Structure(obj)
    _ViewProviderStructure(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    if height:
        obj.Height = height
    if length:
        obj.Length = length
    obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Structure")
    return obj

def makeStructuralSystem(objects,axes):
    '''makeStructuralSystem(objects,axes): makes a structural system
    based on the given objects and axes'''
    result = []
    if objects and axes:
        for o in objects:
            s = makeStructure(o)
            s.Axes = axes
            result.append(s)
        FreeCAD.ActiveDocument.recompute()
    return result
    
def makeProfile(W=46,H=80,tw=3.8,tf=5.2):
    '''makeProfile(W,H,tw,tf): returns a shape with one face describing 
    the profile of a steel beam (IPE, IPN, HE, etc...) based on the following
    dimensions: W = total width, H = total height, tw = web thickness
    tw = flange thickness (see http://en.wikipedia.org/wiki/I-beam for
    reference)'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Profile")
    _Profile(obj)
    obj.Width = W
    obj.Height = H
    obj.WebThickness = tw
    obj.FlangeThickness = tf
    Draft._ViewProviderDraft(obj.ViewObject)
    return obj

class _CommandStructure:
    "the Arch Structure command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}
        
    def Activated(self):
        
        global QtGui, QtCore
        from PyQt4 import QtGui, QtCore
        
        self.Length = 100
        self.Width = 100
        self.Height = 1000
        self.Profile = 0
        self.continueCmd = False
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            # direct creation
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
            FreeCADGui.doCommand("import Arch")
            # if selection contains structs and axes, make a system
            st = Draft.getObjectsOfType(sel,"Structure")
            ax = Draft.getObjectsOfType(sel,"Axis")
            if st and ax:
                FreeCADGui.doCommand("Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + "," + ArchCommands.getStringList(ax) + ")")
            else:
                # else, do normal structs
                for obj in sel:
                    FreeCADGui.doCommand("Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            # interactive mode
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                FreeCAD.DraftWorkingPlane.setup()
            import DraftTrackers
            self.points = []
            self.tracker = DraftTrackers.boxTracker()
            self.tracker.width(self.Width)
            self.tracker.height(self.Height)
            self.tracker.length(self.Length)
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
            
    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        self.tracker.finalize()
        if point == None:
            return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
        FreeCADGui.doCommand('import Arch')
        if self.Profile:
            pr = Presets[self.Profile]
            FreeCADGui.doCommand('p = Arch.makeProfile('+str(pr[2])+','+str(pr[3])+','+str(pr[4])+','+str(pr[5])+')')
            if self.Length == pr[2]:
                # vertical
                FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Height)+')')
            else:
                # horizontal
                FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Length)+')')
                FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
        else:
            FreeCADGui.doCommand('s = Arch.makeStructure(length='+str(self.Length)+',width='+str(self.Width)+',height='+str(self.Height)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if self.continueCmd:
            self.Activated()

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        w.setWindowTitle(str(translate("Arch","Structure options")))
        lay0 = QtGui.QVBoxLayout(w)
        
        # presets box
        layp = QtGui.QHBoxLayout()
        lay0.addLayout(layp)
        labelp = QtGui.QLabel(str(translate("Arch","Preset")))
        layp.addWidget(labelp)
        valuep = QtGui.QComboBox()
        fpresets = [" "]
        for p in Presets[1:]:
            fpresets.append(str(translate("Arch",p[0]))+" "+p[1]+" ("+str(p[2])+"x"+str(p[3])+"mm)")
        valuep.addItems(fpresets)
        layp.addWidget(valuep)
        
        # length
        lay1 = QtGui.QHBoxLayout()
        lay0.addLayout(lay1)
        label1 = QtGui.QLabel(str(translate("Arch","Length")))
        lay1.addWidget(label1)
        self.vLength = QtGui.QDoubleSpinBox()
        self.vLength.setDecimals(2)
        self.vLength.setMaximum(99999.99)
        self.vLength.setValue(self.Length)
        lay1.addWidget(self.vLength)
        
        # width
        lay2 = QtGui.QHBoxLayout()
        lay0.addLayout(lay2)
        label2 = QtGui.QLabel(str(translate("Arch","Width")))
        lay2.addWidget(label2)
        self.vWidth = QtGui.QDoubleSpinBox()
        self.vWidth.setDecimals(2)
        self.vWidth.setMaximum(99999.99)
        self.vWidth.setValue(self.Width)
        lay2.addWidget(self.vWidth)

        # height
        lay3 = QtGui.QHBoxLayout()
        lay0.addLayout(lay3)
        label3 = QtGui.QLabel(str(translate("Arch","Height")))
        lay3.addWidget(label3)
        self.vHeight = QtGui.QDoubleSpinBox()
        self.vHeight.setDecimals(2)
        self.vHeight.setMaximum(99999.99)
        self.vHeight.setValue(self.Height)
        lay3.addWidget(self.vHeight)
        
        # horizontal button
        value5 = QtGui.QPushButton(str(translate("Arch","Rotate")))
        lay0.addWidget(value5)

        # continue button
        value4 = QtGui.QCheckBox(str(translate("Arch","Continue")))
        lay0.addWidget(value4)
        
        QtCore.QObject.connect(valuep,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotate)
        return w
        
    def update(self,point):
        "this function is called by the Snapper when the mouse is moved"
        if self.Height >= self.Length:
            delta = Vector(0,0,self.Height/2)
        else:
            delta = Vector(self.Length/2,0,0)
        self.tracker.pos(point.add(delta))
        
    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setHeight(self,d):
        self.Height = d
        self.tracker.height(d)

    def setLength(self,d):
        self.Length = d
        self.tracker.length(d)

    def setContinue(self,i):
        self.continueCmd = bool(i)
        
    def setPreset(self,i):
        if i > 0:
            self.vLength.setValue(float(Presets[i][2]))
            self.vWidth.setValue(float(Presets[i][3]))
        if len(Presets[i]) == 6:
            self.Profile = i
        else:
            self.Profile = 0
            
    def rotate(self):
        l = self.Length
        w = self.Width
        h = self.Height
        self.vLength.setValue(h)
        self.vHeight.setValue(w)
        self.vWidth.setValue(l)
       
class _Structure(ArchComponent.Component):
    "The Structure object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Tool","Base",
                        "An optional extrusion path for this element")
        obj.addProperty("App::PropertyLength","Length","Base",
                        str(translate("Arch","The length of this element, if not based on a profile")))
        obj.addProperty("App::PropertyLength","Width","Base",
                        str(translate("Arch","The width of this element, if not based on a profile")))
        obj.addProperty("App::PropertyLength","Height","Base",
                        str(translate("Arch","The height or extrusion depth of this element. Keep 0 for automatic")))
        obj.addProperty("App::PropertyLinkList","Axes","Base",
                        str(translate("Arch","Axes systems this structure is built on")))
        obj.addProperty("App::PropertyVector","Normal","Base",
                        str(translate("Arch","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)")))
        obj.addProperty("App::PropertyIntegerList","Exclude","Base",
                        str(translate("Arch","The element numbers to exclude when this structure is based on axes")))
        self.Type = "Structure"
        obj.Length = 100
        obj.Width = 100
        obj.Height = 1000
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop in ["Base","Tool","Length","Width","Height","Normal","Additions","Subtractions","Axes"]:
            self.createGeometry(obj)

    def getAxisPoints(self,obj):
        "returns the gridpoints of linked axes"
        import DraftGeomUtils
        pts = []
        if len(obj.Axes) == 1:
            for e in obj.Axes[0].Shape.Edges:
                pts.append(e.Vertexes[0].Point)
        elif len(obj.Axes) >= 2:
            set1 = obj.Axes[0].Shape.Edges
            set2 = obj.Axes[1].Shape.Edges
            for e1 in set1:
                for e2 in set2: 
                    pts.extend(DraftGeomUtils.findIntersection(e1,e2))
        return pts

    def getAxisPlacement(self,obj):
        "returns an axis placement"
        if obj.Axes:
            return obj.Axes[0].Placement
        return None

    def createGeometry(self,obj):
        import Part, DraftGeomUtils
        
        # getting default values
        height = width = length = 100
        if hasattr(obj,"Length"):
            if obj.Length:
                length = obj.Length
        if hasattr(obj,"Width"):
            if obj.Width:
                width = obj.Width
        if hasattr(obj,"Height"):
            if obj.Height:
                height = obj.Height

        # creating base shape
        pl = obj.Placement
        base = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if hasattr(obj,"Tool"):
                    if obj.Tool:
                        try:
                            base = obj.Tool.Shape.copy().makePipe(obj.Base.Shape.copy())
                        except:
                            FreeCAD.Console.PrintError(str(translate("Arch","Error: The base shape couldn't be extruded along this tool object")))
                            return
                if not base:
                    if obj.Normal == Vector(0,0,0):
                        p = FreeCAD.Placement(obj.Base.Placement)
                        normal = p.Rotation.multVec(Vector(0,0,1))
                    else:
                        normal = Vector(obj.Normal)
                    normal = normal.multiply(height)
                    base = obj.Base.Shape.copy()
                    if base.Solids:
                        pass
                    elif base.Faces:
                        base = base.extrude(normal)
                    elif (len(base.Wires) == 1):
                        if base.Wires[0].isClosed():
                            base = Part.Face(base.Wires[0])
                            base = base.extrude(normal)
                            
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids:
                            base = sh
        else:
            if obj.Normal == Vector(0,0,0):
                normal = Vector(0,0,1)
            else:
                normal = Vector(obj.Normal)
            normal = normal.multiply(height)
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            base = Part.Face(base)
            base = base.extrude(normal)
            
        base = self.processSubShapes(obj,base)
            
        if base:
            # applying axes
            pts = self.getAxisPoints(obj)
            apl = self.getAxisPlacement(obj)
            if pts:
                fsh = []
                for i in range(len(pts)):
                    if hasattr(obj,"Exclude"):
                        if i in obj.Exclude:
                            continue
                    sh = base.copy()
                    if apl:
                        sh.Placement.Rotation = apl.Rotation
                    sh.translate(pts[i])
                    fsh.append(sh)
                    obj.Shape = Part.makeCompound(fsh)

            # finalizing
            
            else:
                if base:
                    if not base.isNull():
                        if base.isValid() and base.Solids:
                            if base.Volume < 0:
                                base.reverse()
                            if base.Volume < 0:
                                FreeCAD.Console.PrintError(str(translate("Arch","Couldn't compute a shape")))
                                return
                            base = base.removeSplitter()
                            obj.Shape = base
                if not DraftGeomUtils.isNull(pl):
                    obj.Placement = pl


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Structure_Tree.svg"


class _Profile(Draft._DraftObject):
    "A parametric beam profile object"
    
    def __init__(self,obj):
        obj.addProperty("App::PropertyDistance","Width","Base","Width of the beam").Width = 10
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the beam").Height = 30
        obj.addProperty("App::PropertyDistance","WebThickness","Base","Thickness of the webs").WebThickness = 3
        obj.addProperty("App::PropertyDistance","FlangeThickness","Base","Thickness of the flange").FlangeThickness = 2
        Draft._DraftObject.__init__(self,obj,"Profile")
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Width","Height","WebThickness","FlangeThickness"]:
            self.createGeometry(obj)
        
    def createGeometry(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width/2,-obj.Height/2,0)
        p2 = Vector(obj.Width/2,-obj.Height/2,0)
        p3 = Vector(obj.Width/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p4 = Vector(obj.WebThickness/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p5 = Vector(obj.WebThickness/2,obj.Height/2-obj.FlangeThickness,0)
        p6 = Vector(obj.Width/2,obj.Height/2-obj.FlangeThickness,0)
        p7 = Vector(obj.Width/2,obj.Height/2,0)
        p8 = Vector(-obj.Width/2,obj.Height/2,0)
        p9 = Vector(-obj.Width/2,obj.Height/2-obj.FlangeThickness,0)
        p10 = Vector(-obj.WebThickness/2,obj.Height/2-obj.FlangeThickness,0)
        p11 = Vector(-obj.WebThickness/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p12 = Vector(-obj.Width/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p1])
        p = Part.Face(p)
        obj.Shape = p
        obj.Placement = pl


FreeCADGui.addCommand('Arch_Structure',_CommandStructure())

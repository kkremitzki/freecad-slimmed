# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD,Draft,ArchCommands,DraftVecUtils,sys
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchBuildingPart
#  \ingroup ARCH
#  \brief The BuildingPart object and tools
#
#  This module provides tools to build BuildingPart objects.
#  BuildingParts are used to group different Arch objects

__title__="FreeCAD Arch BuildingPart"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"



def makeBuildingPart(objectslist=None):

    '''makeBuildingPart(objectslist): creates a buildingPart including the
    objects from the given list.'''

    obj = FreeCAD.ActiveDocument.addObject("App::GeometryPython","BuildingPart")
    #obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","BuildingPart")
    obj.Label = translate("Arch","BuildingPart")
    BuildingPart(obj)
    if FreeCAD.GuiUp:
        ViewProviderBuildingPart(obj.ViewObject)
    if objectslist:
        obj.addObjects(objectslist)
    return obj

def makeFloor(objectslist=None,baseobj=None,name="Floor"):
    
    """overwrites ArchFloor.makeFloor"""
    
    obj = makeBuildingPart(objectslist)
    obj.Label = name
    obj.IfcRole = "Building Storey"
    return obj

def convertFloors(floor=None):

    """convert the given Floor (or all Arch Floors from the active document if none is given) into BuildingParts"""

    todel = []
    if floor:
        objset = [floor]
    else:
        objset = FreeCAD.ActiveDocument.Objects
    for obj in objset:
        if Draft.getType(obj) == "Floor":
            nobj = makeBuildingPart(obj.Group)
            nobj.Role = "Storey"
            label = obj.Label
            for parent in obj.InList:
                if hasattr(parent,"Group"):
                    if obj in parent.Group:
                        parent.addObject(nobj)
                        #g = parent.Group
                        #g.append(nobj)
                        #parent.Group = g
                else:
                    print("Warning: couldn't add new object '"+label+"' to parent object '"+parent.Label+"'")
            todel.append(obj.Name)
            if obj.ViewObject:
                obj.ViewObject.Proxy.Object = None # some bug makes this trigger even efter the object has been deleted...
            obj.Label = obj.Label+" to delete" # in case FreeCAD doesn't allow 2 objs with same label
            nobj.Label = label
    for n in todel:
        from DraftGui import todo
        todo.delay(FreeCAD.ActiveDocument.removeObject,n)


class CommandBuildingPart:


    "the Arch BuildingPart command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_BuildingPart',
                'MenuText': QT_TRANSLATE_NOOP("Arch_BuildingPart","BuildingPart"),
                'Accel': "B, P",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_BuildingPart","Creates a BuildingPart object including selected objects")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        ss = "[ "
        for o in sel:
            ss += "FreeCAD.ActiveDocument." + o.Name + ", "
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create BuildingPart"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("obj = Arch.makeBuildingPart("+ss+")")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()



class BuildingPart:


    "The BuildingPart object"

    def __init__(self,obj):

        obj.Proxy = self
        obj.addExtension('App::GroupExtensionPython', self)
        #obj.addExtension('App::OriginGroupExtensionPython', self)
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The height of this object"))
        if not "LevelOffset" in pl:
            obj.addProperty("App::PropertyLength","LevelOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The level of the (0,0,0) point of this level"))
        if not "Area" in pl:
            obj.addProperty("App::PropertyArea","Area", "BuildingPart",QT_TRANSLATE_NOOP("App::Property","The computed floor area of this floor"))
        if not "IfcRole" in pl:
            obj.addProperty("App::PropertyEnumeration","IfcRole","Component",QT_TRANSLATE_NOOP("App::Property","The role of this object"))
            import ArchComponent
            obj.IfcRole = ArchComponent.IfcRoles
        if not "CloneOf" in pl:
            obj.addProperty("App::PropertyLink","CloneOf","Component",QT_TRANSLATE_NOOP("App::Property","The object this component is cloning"))
        if not "Description" in pl:
            obj.addProperty("App::PropertyString","Description","Component",QT_TRANSLATE_NOOP("App::Property","An optional description for this component"))
        if not "Tag" in pl:
            obj.addProperty("App::PropertyString","Tag","Component",QT_TRANSLATE_NOOP("App::Property","An optional tag for this component"))
        if not "IfcAttributes" in pl:
            obj.addProperty("App::PropertyMap","IfcAttributes","Component",QT_TRANSLATE_NOOP("App::Property","Custom IFC properties and attributes"))
        self.Type = "BuildingPart"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def onBeforeChange(self,obj,prop):

        if prop == "Placement":
            self.oldPlacement = FreeCAD.Placement(obj.Placement)

    def onChanged(self,obj,prop):

        if prop == "Height":
            for child in obj.Group:
                if Draft.getType(child) in ["Wall","Structure"]:
                    if not child.Height.Value:
                        child.Proxy.execute(child)
        elif prop == "Placement":
            if hasattr(self,"oldPlacement"):
                if self.oldPlacement:
                    v = FreeCAD.Vector(0,0,1)
                    deltap = obj.Placement.Base.sub(self.oldPlacement.Base)
                    if deltap.Length == 0:
                        deltap = None
                    deltar = FreeCAD.Rotation(self.oldPlacement.Rotation.multVec(v),obj.Placement.Rotation.multVec(v))
                    if deltar.Angle < 0.001:
                        deltar = None
                    for child in obj.Group:
                        if ((not hasattr(child,"MoveWithHost")) or child.MoveWithHost) and hasattr(child,"Placement"):
                            print "moving ",child.Label
                            if deltap:
                                child.Placement.move(deltap)
                            if deltar:
                                child.Placement.Rotation = child.Placement.Rotation.multiply(deltar)

    def execute(self,obj):

        pass

    def getSpaces(self,obj):

        "gets the list of Spaces that have this object as their Zone property"
        g = []
        for o in obj.OutList:
            if hasattr(o,"Zone"):
                if o.Zone == obj:
                    g.append(o)
        return g



class ViewProviderBuildingPart:


    "A View Provider for the BuildingPart object"

    def __init__(self,vobj):

        vobj.addExtension("Gui::ViewProviderGroupExtensionPython", self)
        #vobj.addExtension("Gui::ViewProviderGeoFeatureGroupExtensionPython", self)
        vobj.Proxy = self
        self.setProperties(vobj)
        vobj.ShapeColor = (0.13,0.15,0.37)

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "LineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","LineWidth","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The line width of this object"))
            vobj.LineWidth = 1
        if not "OverrideUnit" in pl:
            vobj.addProperty("App::PropertyString","OverrideUnit","BuildingPart",QT_TRANSLATE_NOOP("App::Property","An optional unit to express levels"))
        if not "DisplayOffset" in pl:
            vobj.addProperty("App::PropertyPlacement","DisplayOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","A transformation to apply to the level mark"))
        if not "ShowLevel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLevel","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, show the level"))
            vobj.ShowLevel = True
        if not "ShowUnit" in pl:
            vobj.addProperty("App::PropertyBool","ShowUnit","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, show the unit on the level tag"))
        if not "SetWorkingPlane" in pl:
            vobj.addProperty("App::PropertyBool","SetWorkingPlane","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, when activated, the working plane will automatically adapt to this level"))
            vobj.SetWorkingPlane = True
        if not "OriginOffset" in pl:
            vobj.addProperty("App::PropertyBool","OriginOffset","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, when activated, Display offset will affect the origin mark too"))
        if not "ShowLabel" in pl:
            vobj.addProperty("App::PropertyBool","ShowLabel","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If true, when activated, the object's label is displayed"))
            vobj.ShowLabel = True
        if not "FontName" in pl:
            vobj.addProperty("App::PropertyFont","FontName","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The font to be used for texts"))
            vobj.FontName = Draft.getParam("textfont","Arial")
        if not "FontSize" in pl:
            vobj.addProperty("App::PropertyLength","FontSize","BuildingPart",QT_TRANSLATE_NOOP("App::Property","The font size of texts"))
            vobj.FontSize = Draft.getParam("textheight",2.0)
        if not "ViewData" in pl:
            vobj.addProperty("App::PropertyFloatList","ViewData","BuildingPart",QT_TRANSLATE_NOOP("App::Property","Camera position data associated with this object"))
        if not "RestoreView" in pl:
            vobj.addProperty("App::PropertyBool","RestoreView","BuildingPart",QT_TRANSLATE_NOOP("App::Property","If set, the view stored in this object will be restored on double-click"))

    def onDocumentRestored(self,vobj):

        selt.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if self.Object.IfcRole == "Building Storey":
                return ":/icons/Arch_Floor_Tree.svg"
            elif self.Object.IfcRole == "Building":
                return ":/icons/Arch_Building_Tree.svg"
        return ":/icons/Arch_BuildingPart_Tree.svg"

    def attach(self,vobj):

        self.Object = vobj.Object
        from pivy import coin
        self.sep = coin.SoSeparator()
        self.mat = coin.SoMaterial()
        self.sep.addChild(self.mat)
        self.dst = coin.SoDrawStyle()
        self.sep.addChild(self.dst)
        self.lco = coin.SoCoordinate3()
        self.sep.addChild(self.lco)
        lin = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        lin.coordIndex.setValues([0,1,-1,2,3,-1,4,5,-1])
        self.sep.addChild(lin)
        self.tra = coin.SoTransform()
        self.tra.rotation.setValue(FreeCAD.Rotation(0,0,90).Q)
        self.sep.addChild(self.tra)
        self.fon = coin.SoFont()
        self.sep.addChild(self.fon)
        self.txt = coin.SoAsciiText()
        self.txt.justification = coin.SoText2.LEFT
        self.txt.string.setValue("level")
        self.sep.addChild(self.txt)
        self.onChanged(vobj,"ShapeColor")
        self.onChanged(vobj,"FontName")
        self.onChanged(vobj,"ShowLevel")
        self.onChanged(vobj,"FontSize")
        return

    def getDisplayModes(self,vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self,mode):

        return mode

    def isShow(self):

        return True

    def updateData(self,obj,prop):

        if prop in ["Placement","LevelOffset"]:
            self.onChanged(obj.ViewObject,"OverrideUnit")

    def onChanged(self,vobj,prop):

        if prop == "ShapeColor":
            if hasattr(vobj,"ShapeColor"):
                l = vobj.ShapeColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "LineWidth":
            if hasattr(vobj,"LineWidth"):
                self.dst.lineWidth = vobj.LineWidth
        elif prop == "FontName":
            if hasattr(vobj,"FontName"):
                if vobj.FontName:
                    if sys.version_info.major < 3:
                        self.fon.name = vobj.FontName.encode("utf8")
                    else:
                        self.fon.name = vobj.FontName
        elif prop in ["FontSize","DisplayOffset","OriginOffset"]:
            if hasattr(vobj,"FontSize") and hasattr(vobj,"DisplayOffset") and hasattr(vobj,"OriginOffset"):
                fs = vobj.FontSize.Value
                if fs:
                    self.fon.size = fs
                    b = vobj.DisplayOffset.Base
                    self.tra.translation.setValue([b.x+fs/8,b.y,b.z+fs/8])
                    if vobj.OriginOffset:
                        self.lco.point.setValues([[b.x-fs,b.y,b.z],[b.x+fs,b.y,b.z],[b.x,b.y-fs,b.z],[b.x,b.y+fs,b.z],[b.x,b.y,b.z-fs],[b.x,b.y,b.z+fs]])
                    else:
                        self.lco.point.setValues([[-fs,0,0],[fs,0,0],[0,-fs,0],[0,fs,0],[0,0,-fs],[0,0,fs]])
        elif prop in ["ShowLevel","ShowLabel"]:
            if hasattr(vobj,"ShowLevel") and hasattr(vobj,"ShowLabel"):
                rn = vobj.RootNode
                if vobj.ShowLevel or vobj.ShowLabel:
                    if rn.findChild(self.sep) == -1:
                        rn.addChild(self.sep)
                    self.onChanged(vobj,"ShowUnit")
                else:
                    if rn.findChild(self.sep) != -1:
                        rn.removeChild(self.sep)
        elif prop in ["OverrideUnit","ShowUnit"]:
            if hasattr(vobj,"OverrideUnit") and hasattr(vobj,"ShowUnit"):
                z = vobj.Object.Placement.Base.z + vobj.Object.LevelOffset.Value
                q = FreeCAD.Units.Quantity(z,FreeCAD.Units.Length)
                txt = ""
                if vobj.ShowLabel:
                    txt += vobj.Object.Label
                if vobj.ShowLevel:
                    if txt:
                        txt += " "
                    if z >= 0:
                        txt = "+"
                    if vobj.OverrideUnit:
                        u = vobj.OverrideUnit
                    else:
                        u = q.getUserPreferred()[2]
                    q = q.getValueAs(u)
                    d = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",0)
                    fmt = "{0:."+ str(d) + "f}"
                    if not vobj.ShowUnit:
                        u = ""
                    txt += fmt.format(float(q)) + str(u)
                if isinstance(txt,unicode):
                    txt = txt.encode("utf8")
                self.txt.string.setValue(txt)

    def doubleClicked(self,vobj):

        self.activate(vobj)
        FreeCADGui.Selection.clearSelection()
        return True

    def activate(self,vobj):

        if FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch") == vobj.Object:
            FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",None)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane(restore=True)
        else:
            FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",vobj.Object)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane()

    def setupContextMenu(self,vobj,menu):

        from PySide import QtCore,QtGui
        import Draft_rc
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Set working plane",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.setWorkingPlane)
        menu.addAction(action1)
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),"Write camera position",menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),self.writeCamera)
        menu.addAction(action2)

    def setWorkingPlane(self,restore=False):

        if hasattr(self,"Object") and hasattr(FreeCAD,"DraftWorkingPlane"):
            import FreeCADGui
            if restore:
                FreeCAD.DraftWorkingPlane.restore()
            else:
                FreeCAD.DraftWorkingPlane.save()
                FreeCAD.DraftWorkingPlane.setFromPlacement(self.Object.Placement,rebase=True)
                FreeCAD.DraftWorkingPlane.weak = False
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.setGrid()
            if hasattr(FreeCADGui,"draftToolBar"):
                if restore and hasattr(self,"wptext"):
                    FreeCADGui.draftToolBar.wplabel.setText(self.wptext)
                else:
                    self.wptext = FreeCADGui.draftToolBar.wplabel.text()
                    FreeCADGui.draftToolBar.wplabel.setText(self.Object.Label)

    def writeCamera(self):

        if hasattr(self,"Object"):
            from pivy import coin
            n = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            FreeCAD.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing camera position")+"\n")
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n,coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0) # orthograhic camera
            elif isinstance(n,coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0) # perspective camera
            self.Object.ViewObject.ViewData = cdata

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_BuildingPart',CommandBuildingPart())

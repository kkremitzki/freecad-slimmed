#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013                                                    *  
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

"FreeCAD webgl exporter"

import FreeCAD,Draft
from DraftTools import translate

if FreeCAD.GuiUp:
    import FreeCADGui
else:
    FreeCADGui = None

tab = "                "
addWireframe = False

if open.__module__ == '__builtin__':
    pythonopen = open
    
def export(exportList,filename):
    "exports the given objects to a .html file"

    html = getHTML(exportList)
    outfile = pythonopen(filename,"wb")
    outfile.write(html)
    outfile.close()
    FreeCAD.Console.PrintMessage(str(translate("Arch","successfully written "))+filename)
    
def getHTML(objectsList):
    "returns the complete HTML code of a viewer for the given objects"
    
    # get objects data
    objectsData = ''
    for obj in objectsList:
        objectsData += getObjectData(obj)
    template = getTemplate()
    template = template.replace("$CameraData",getCameraData())
    template = template.replace("$ObjectsData",objectsData)
    return template
    
def getCameraData():
    "returns the position and direction of the camera as three.js snippet"
    
    result = ""
    if FreeCADGui:
        # getting camera position
        pos = FreeCADGui.ActiveDocument.ActiveView.viewPosition().Base
        result += "camera.position.set( "
        result += str(pos.x) + ", "
        result += str(pos.y) + ", "
        result += str(pos.z) + " );\n"
    else:
        result += "camera.position.set(0,0,1000);\n"
    result += tab+"camera.lookAt( scene.position );\n"+tab
    # print result
    return result
    
def getObjectData(obj):
    "returns the geometry data of an object as three.js snippet"
    
    result = ""
    if obj.isDerivedFrom("Part::Feature"):
        fcmesh = obj.Shape.tessellate(0.1)
        result = "var geom = new THREE.Geometry();\n"
        # adding vertices data
        for i in range(len(fcmesh[0])):
            v = fcmesh[0][i]
            result += tab+"var v"+str(i)+" = new THREE.Vector3("+str(v.x)+","+str(v.y)+","+str(v.z)+");\n"
        result += tab+"console.log(geom.vertices)\n"
        for i in range(len(fcmesh[0])):
            result += tab+"geom.vertices.push(v"+str(i)+");\n"
        # adding facets data
        for f in fcmesh[1]:
            result += tab+"geom.faces.push( new THREE.Face3"+str(f)+" );\n"
                    
    elif obj.isDerivedFrom("Mesh::Feature"):
        mesh = obj.Mesh
        result = "var geom = new THREE.Geometry();\n"
        # adding vertices data 
        for p in mesh.Points:
            v = p.Vector
            i = p.Index
            result += tab+"var v"+str(i)+" = new THREE.Vector3("+str(v.x)+","+str(v.y)+","+str(v.z)+");\n"
        result += tab+"console.log(geom.vertices)\n"
        for p in mesh.Points:
            result += tab+"geom.vertices.push(v"+str(p.Index)+");\n"
        # adding facets data
        for f in mesh.Facets:
            result += tab+"geom.faces.push( new THREE.Face3"+str(f.PointIndices)+" );\n"
            
    if result:
        # adding a base material
        if FreeCADGui:
            col = obj.ViewObject.ShapeColor
            rgb = Draft.getrgb(col,testbw=False)
        else:
            rgb = "#888888" # test color
        result += tab+"var basematerial = new THREE.MeshBasicMaterial( { color: 0x"+str(rgb)[1:]+" } );\n"
        # adding a wireframe material
        result += tab+"var wireframe = new THREE.MeshBasicMaterial( { color: "
        result += "0x000000, wireframe: true, transparent: true } );\n"
        result += tab+"var material = [ basematerial, wireframe ];\n"
        # adding the mesh to the scene
        #result += tab+"var mesh = new THREE.Mesh( geom, basematerial );\n"
        result += tab+"var mesh = new THREE.SceneUtils.createMultiMaterialObject( geom, material );\n"
        result += tab+"scene.add( mesh );\n"+tab
        
    return result

def getTemplate():
    "returns a html template"
    
    result = """<!DOCTYPE html>
        <html>
        <head>
            <title>FreeCAD model</title>
            <script type="text/javascript" src="http://cdnjs.cloudflare.com/ajax/libs/three.js/r50/three.min.js"></script>
            
            <script>
            
            var camera, controls, scene, renderer;
            
            window.onload = function() {

                var SCREEN_WIDTH = window.innerWidth, SCREEN_HEIGHT = window.innerHeight;
                var VIEW_ANGLE = 35, ASPECT = SCREEN_WIDTH / SCREEN_HEIGHT, NEAR = 0.1, FAR = 20000;

                renderer = new THREE.WebGLRenderer();
                renderer.setSize( SCREEN_WIDTH, SCREEN_HEIGHT );
                document.body.appendChild( renderer.domElement );
        
                scene = new THREE.Scene();
        
                camera = new THREE.PerspectiveCamera(
                    VIEW_ANGLE,      // Field of view
                    ASPECT,          // Aspect ratio
                    NEAR,            // Near plane
                    FAR              // Far plane
                );
                $CameraData // placeholder for the FreeCAD camera
                
                controls = new THREE.TrackballControls( camera );
                controls.rotateSpeed = 1.0;
                controls.zoomSpeed = 1.2;
                controls.panSpeed = 0.8;
                controls.noZoom = false;
                controls.noPan = false;
                controls.staticMoving = true;
                controls.dynamicDampingFactor = 0.3;
                controls.keys = [ 65, 83, 68 ]; 
        
                $ObjectsData // placeholder for the FreeCAD objects
        
                var light = new THREE.PointLight( 0xFFFF00 );
                light.position.set( -10, -10, 10 );
                scene.add( light );
        
                renderer.render( scene, camera );
                
                animate();
            };
            
            function animate(){
                requestAnimationFrame( animate );
                render();
            };
            
            function render(){
                controls.update();
                renderer.render( scene, camera );
            };
            </script>
        </head>
        <body></body>
        </html>"""
    
    return result
        

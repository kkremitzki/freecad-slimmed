/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2005     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <BRep_Tool.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomLProp_SLProps.hxx>
# include <Poly_Triangulation.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <sstream>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Matrix.h>
#include <App/ComplexGeoData.h>
#include <boost/regex.hpp>


#include "PovTools.h"
#include "LuxTools.h"

using Base::Console;

using namespace Raytracing;
using namespace std;

std::string LuxTools::getCamera(const CamDef& Cam)
{
    std::stringstream out;
    out << "# declares position and view direction" << endl
        << "# Generated by FreeCAD (http://www.freecadweb.org/)" << endl

        // writing Camera positions
        << "LookAt " << Cam.CamPos.X() << " " << Cam.CamPos.Y() << " " << Cam.CamPos.Z() << " "
        // writing lookat
        << Cam.LookAt.X()  << " " << Cam.LookAt.Y() << " " << Cam.LookAt.Z() << " "
        // writing the Up Vector
        << Cam.Up.X() << " " << Cam.Up.Y() << " " << Cam.Up.Z() << endl;

    return out.str();
}

void LuxTools::writeShape(std::ostream &out, const char *PartName, const TopoDS_Shape& Shape, float fMeshDeviation)
{
    Base::Console().Log("Meshing with Deviation: %f\n",fMeshDeviation);

    TopExp_Explorer ex;
    BRepMesh_IncrementalMesh MESH(Shape,fMeshDeviation);

    // counting faces and start sequencer
    int l = 1;
    for (ex.Init(Shape, TopAbs_FACE); ex.More(); ex.Next(),l++) {}
    Base::SequencerLauncher seq("Writing file", l);
    
    // write object
    out << "AttributeBegin #  \"" << PartName << "\"" << endl;
    out << "Transform [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]" << endl;
    out << "NamedMaterial \"FreeCADMaterial_" << PartName << "\"" << endl;
    out << "Shape \"mesh\"" << endl;
    
    // gather vertices, normals and face indices
    std::stringstream triindices;
    std::stringstream N;
    std::stringstream P;
    l = 1;
    int vi = 0;
    for (ex.Init(Shape, TopAbs_FACE); ex.More(); ex.Next(),l++) {

        // get the shape and mesh it
        const TopoDS_Face& aFace = TopoDS::Face(ex.Current());

        // this block mesh the face and transfers it in a C array of vertices and face indexes
        Standard_Integer nbNodesInFace,nbTriInFace;
        gp_Vec* vertices=0;
        gp_Vec* vertexnormals=0;
        long* cons=0;

        PovTools::transferToArray(aFace,&vertices,&vertexnormals,&cons,nbNodesInFace,nbTriInFace);

        if (!vertices) break;
        // writing vertices
        for (int i=0; i < nbNodesInFace; i++) {
            P << vertices[i].X() << " " << vertices[i].Y() << " " << vertices[i].Z() << " ";
        }

        // writing per vertex normals
        for (int j=0; j < nbNodesInFace; j++) {
            N << vertexnormals[j].X() << " "  << vertexnormals[j].Y() << " " << vertexnormals[j].Z() << " ";
        }

        // writing triangle indices
        for (int k=0; k < nbTriInFace; k++) {
            triindices << cons[3*k]+vi << " " << cons[3*k+2]+vi << " " << cons[3*k+1]+vi << " ";
        }
        
        vi = vi + nbNodesInFace;
        
        delete [] vertexnormals;
        delete [] vertices;
        delete [] cons;

        seq.next();

    } // end of face loop

    // write mesh data
    out << "    \"integer triindices\" [" << triindices.str() << "]" << endl;
    out << "    \"point P\" [" << P.str() << "]" << endl;
    out << "    \"normal N\" [" << N.str() << "]" << endl;
    out << "    \"bool generatetangents\" [\"false\"]" << endl;
    out << "    \"string name\" [\"" << PartName << "\"]" << endl;
    out << "AttributeEnd # \"\"" << endl;
}

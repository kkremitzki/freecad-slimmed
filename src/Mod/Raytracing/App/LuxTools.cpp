/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2005     *
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
#include <App/ComplexGeoData.h>


#include "PovTools.h"
#include "LuxTools.h"

using Base::Console;

using namespace Raytracing;
using namespace std;

std::string LuxTools::getCamera(const CamDef& Cam)
{
    std::stringstream out;
    out << "# declares positon and view direction" << endl
        << "# Generated by FreeCAD (http://www.freecadweb.org/)" << endl

        // writing Camera positions
        << "LookAt " << Cam.CamPos.X() << " " << Cam.CamPos.Y() << " " << Cam.CamPos.Z() << " "
        // writing lookat
        << Cam.LookAt.X()  << " " << Cam.LookAt.Y() << " " << Cam.LookAt.Z() << " "
        // writing the Up Vector
        << Cam.Up.X() << " " << Cam.Up.Y() << " " << Cam.Up.Z() << endl;

    return out.str();
}

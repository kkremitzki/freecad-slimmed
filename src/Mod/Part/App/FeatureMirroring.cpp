/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp.hxx>
# include <gp_Ax2.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Trsf.hxx>
# include <BRepBuilderAPI_Transform.hxx>
#endif


#include "FeatureMirroring.h"


using namespace Part;

PROPERTY_SOURCE(Part::Mirroring, Part::Feature)

Mirroring::Mirroring()
{
    ADD_PROPERTY(Source,(0));
    ADD_PROPERTY_TYPE(Base,(Base::Vector3f()),"Plane",App::Prop_None,"The base point of the plane");
    ADD_PROPERTY_TYPE(Normal,(Base::Vector3f(0,0,1)),"Plane",App::Prop_None,"The normal of the plane");
}

short Mirroring::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    if (Base.isTouched())
        return 1;
    if (Normal.isTouched())
        return 1;
    return 0;
}

void Mirroring::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Base || prop == &Normal) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Mirroring::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *source = static_cast<Part::Feature*>(link);
    Base::Vector3f base = Base.getValue();
    Base::Vector3f norm = Normal.getValue();

    try {
        const TopoDS_Shape& shape = source->Shape.getValue();
        gp_Ax2 ax2(gp_Pnt(base.x,base.y,base.z), gp_Dir(norm.x,norm.y,norm.z));
        gp_Trsf mat;
        mat.SetMirror(ax2);
        TopLoc_Location loc = shape.Location();
        gp_Trsf placement = loc.Transformation();
        mat = placement * mat;
        BRepBuilderAPI_Transform mkTrf(shape, mat);
        this->Shape.setValue(mkTrf.Shape());
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

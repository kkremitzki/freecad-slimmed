/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepFill.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shell.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
#endif


#include "PartFeatures.h"


using namespace Part;

PROPERTY_SOURCE(Part::RuledSurface, Part::Feature)

RuledSurface::RuledSurface()
{
    ADD_PROPERTY_TYPE(Curve1,(0),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
    ADD_PROPERTY_TYPE(Curve2,(0),"Ruled Surface",App::Prop_None,"Curve of ruled surface");
}

short RuledSurface::mustExecute() const
{
    if (Curve1.isTouched())
        return 1;
    if (Curve2.isTouched())
        return 1;
    return 0;
}

void RuledSurface::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *RuledSurface::execute(void)
{
    try {
        App::DocumentObject* c1 = Curve1.getValue();
        if (!(c1 && c1->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn("No shape linked.");
        const std::vector<std::string>& element1 = Curve1.getSubValues();
        if (element1.size() != 1)
            return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");
        App::DocumentObject* c2 = Curve2.getValue();
        if (!(c2 && c2->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn("No shape linked.");
        const std::vector<std::string>& element2 = Curve2.getSubValues();
        if (element2.size() != 1)
            return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");

        TopoDS_Shape curve1;
        const Part::TopoShape& shape1 = static_cast<Part::Feature*>(c1)->Shape.getValue();
        if (!shape1._Shape.IsNull()) {
            if (!element1[0].empty()) {
                curve1 = shape1.getSubShape(element1[0].c_str());
            }
            else {
                if (shape1._Shape.ShapeType() == TopAbs_EDGE)
                    curve1 = shape1._Shape;
                else if (shape1._Shape.ShapeType() == TopAbs_WIRE)
                    curve1 = shape1._Shape;
            }
        }

        TopoDS_Shape curve2;
        const Part::TopoShape& shape2 = static_cast<Part::Feature*>(c2)->Shape.getValue();
        if (!shape2._Shape.IsNull()) {
            if (!element2[0].empty()) {
                curve2 = shape2.getSubShape(element2[0].c_str());
            }
            else {
                if (shape2._Shape.ShapeType() == TopAbs_EDGE)
                    curve2 = shape2._Shape;
                else if (shape2._Shape.ShapeType() == TopAbs_WIRE)
                    curve2 = shape2._Shape;
            }
        }

        if (curve1.IsNull() || curve2.IsNull())
            return new App::DocumentObjectExecReturn("Linked shapes are empty.");
        if (curve1.ShapeType() == TopAbs_EDGE && curve2.ShapeType() == TopAbs_EDGE) {
            TopoDS_Face face = BRepFill::Face(TopoDS::Edge(curve1), TopoDS::Edge(curve2));
            this->Shape.setValue(face);
        }
        else if (curve1.ShapeType() == TopAbs_WIRE && curve2.ShapeType() == TopAbs_WIRE) {
            TopoDS_Shell shell = BRepFill::Shell(TopoDS::Wire(curve1), TopoDS::Wire(curve2));
            this->Shape.setValue(shell);
        }
        else {
            return new App::DocumentObjectExecReturn("Curves must either be edges or wires.");
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("General error in RuledSurface::execute()");
    }
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Loft, Part::Feature)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Solid,(false),"Loft",App::Prop_None,"Create solid");
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Ruled surface");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Solid.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    return 0;
}

void Loft::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Loft::execute(void)
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            if (!(*it)->isDerivedFrom(Part::Feature::getClassTypeId()))
                return new App::DocumentObjectExecReturn("Linked object is not a shape.");
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");
            if (shape.ShapeType() == TopAbs_WIRE) {
                profiles.Append(shape);
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_VERTEX) {
                profiles.Append(shape);
            }
            else {
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge nor wire.");
            }
        }

        Standard_Boolean isSolid = Solid.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isRuled = Ruled.getValue() ? Standard_True : Standard_False;

        TopoShape myShape;
        this->Shape.setValue(myShape.makeLoft(profiles, isSolid, isRuled));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(Part::Sweep, Part::Feature)

Sweep::Sweep()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Sweep",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Spine,(0),"Sweep",App::Prop_None,"Path to sweep along");
    ADD_PROPERTY_TYPE(Solid,(false),"Sweep",App::Prop_None,"Create solid");
    ADD_PROPERTY_TYPE(Fresnet,(false),"Sweep",App::Prop_None,"Fresnet");
}

short Sweep::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Spine.isTouched())
        return 1;
    if (Solid.isTouched())
        return 1;
    if (Fresnet.isTouched())
        return 1;
    return 0;
}

void Sweep::onChanged(const App::Property* prop)
{
    Part::Feature::onChanged(prop);
}

App::DocumentObjectExecReturn *Sweep::execute(void)
{
    if (Sections.getSize() == 0)
        return new App::DocumentObjectExecReturn("No sections linked.");
    App::DocumentObject* spine = Spine.getValue();
    if (!(spine && spine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
        return new App::DocumentObjectExecReturn("No shape linked.");
    const std::vector<std::string>& subedge = Spine.getSubValues();
    if (subedge.size() != 1)
        return new App::DocumentObjectExecReturn("Not exactly one sub-shape linked.");

    TopoDS_Shape edge;
    const Part::TopoShape& shape = static_cast<Part::Feature*>(spine)->Shape.getValue();
    if (!shape._Shape.IsNull()) {
        if (!subedge[0].empty()) {
            edge = shape.getSubShape(subedge[0].c_str());
        }
        else {
            if (shape._Shape.ShapeType() == TopAbs_EDGE)
                edge = shape._Shape;
            else if (shape._Shape.ShapeType() == TopAbs_WIRE)
                edge = shape._Shape;
        }
    }

    try {
        TopTools_ListOfShape profiles;
        const std::vector<App::DocumentObject*>& shapes = Sections.getValues();
        std::vector<App::DocumentObject*>::const_iterator it;
        for (it = shapes.begin(); it != shapes.end(); ++it) {
            if (!(*it)->isDerivedFrom(Part::Feature::getClassTypeId()))
                return new App::DocumentObjectExecReturn("Linked object is not a shape.");
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(*it)->Shape.getValue();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Linked shape is invalid.");
            if (shape.ShapeType() == TopAbs_WIRE) {
                profiles.Append(shape);
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(shape));
                profiles.Append(mkWire.Wire());
            }
            else if (shape.ShapeType() == TopAbs_VERTEX) {
                profiles.Append(shape);
            }
            else {
                return new App::DocumentObjectExecReturn("Linked shape is not a vertex, edge nor wire.");
            }
        }

        Standard_Boolean isSolid = Solid.getValue() ? Standard_True : Standard_False;
        Standard_Boolean isFresnet = Fresnet.getValue() ? Standard_True : Standard_False;

        BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(edge));
        TopoShape myShape(mkWire.Wire());
        this->Shape.setValue(myShape.makePipeShell(profiles, isSolid, isFresnet));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#endif


#include "FeatureDressUp.h"
#include <Base/Exception.h>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Edge.hxx>


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::DressUp, PartDesign::Feature)

DressUp::DressUp()
{
    ADD_PROPERTY(Base,(0));
    Placement.StatusBits.set(2, true);
}

short DressUp::mustExecute() const
{
    if (Base.getValue() && Base.getValue()->isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}


void DressUp::positionByBaseFeature(void)
{
    Part::Feature *base = static_cast<Part::Feature*>(BaseFeature.getValue());
    if (base && base->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        this->Placement.setValue(base->Placement.getValue());
}

Part::TopoShape DressUp::getBaseShape()
{
    App::DocumentObject* link = BaseFeature.getValue();
    if (!link)
        link = this->Base.getValue(); // For legacy features
    if (!link)
        throw Base::Exception("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::Exception("Linked object is not a Part object");
    Part::Feature* base = static_cast<Part::Feature*>(link);
    const Part::TopoShape& shape = base->Shape.getShape();
    if (shape._Shape.IsNull())
        throw Base::Exception("Cannot draft invalid shape");
    return shape;
}

void DressUp::getContiniusEdges(Part::TopoShape TopShape, std::vector< std::string >& SubNames) {

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(TopShape._Shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(TopShape._Shape, TopAbs_EDGE, mapOfEdges);

    unsigned int i = 0;
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) == "Edge") {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if(los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        else if(aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));

            TopTools_IndexedMapOfShape mapOfFaces;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfFaces);

            for(int j = 1; j <= mapOfFaces.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfFaces.FindKey(j));

                int id = mapOfEdges.FindIndex(edge);

                std::stringstream buf;
                buf << "Edge";
                buf << id;

                if(std::find(SubNames.begin(),SubNames.end(),buf.str()) == SubNames.end())
                {
                    SubNames.push_back(buf.str());
                }

            }

            SubNames.erase(SubNames.begin()+i);
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }        
}


void DressUp::onChanged(const App::Property* prop)
{
    if (prop == &BaseFeature) {
        // if attached to a sketch then mark it as read-only
        this->Placement.setStatus(App::Property::ReadOnly, BaseFeature.getValue() != 0);
    }

    Feature::onChanged(prop);
}

}

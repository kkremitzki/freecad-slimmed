/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2015              *
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

#include "DraftDxf.h"

#include <gp_Pnt.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Elips.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>

#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/PartFeature.h>

using namespace DraftUtils;

DraftDxfRead::DraftDxfRead(std::string filepath, App::Document *pcDoc) : CDxfRead(filepath.c_str())
{
    document = pcDoc;
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Draft");
    optionGroupLayers = hGrp->GetBool("groupLayers",false);
}


void DraftDxfRead::OnReadLine(const double* s, const double* e, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Pnt p1(e[0], e[1], e[2]);
    if (p0.IsEqual(p1,0.00000001))
        return;
    BRepBuilderAPI_MakeEdge makeEdge(p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadPoint(const double* s)
{
    BRepBuilderAPI_MakeVertex makeVertex(gp_Pnt(s[0], s[1], s[2]));
    TopoDS_Vertex vertex = makeVertex.Vertex();
    AddObject(new Part::TopoShape(vertex));
}


void DraftDxfRead::OnReadArc(const double* s, const double* e, const double* c, bool dir, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Pnt p1(e[0], e[1], e[2]);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle, p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadCircle(const double* s, const double* c, bool dir, bool hidden)
{
    gp_Pnt p0(s[0], s[1], s[2]);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadSpline(struct SplineData& sd)
{
    // not yet implemented
}


void DraftDxfRead::OnReadEllipse(const double* c, double major_radius, double minor_radius, double rotation, double start_angle, double end_angle, bool dir)
{
    gp_Dir up(0, 0, 1);
    if(!dir)
        up = -up;
    gp_Pnt pc(c[0], c[1], c[2]);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius, minor_radius);
    ellipse.Rotate(gp_Ax1(pc,up),rotation);
    BRepBuilderAPI_MakeEdge makeEdge(ellipse);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void DraftDxfRead::OnReadText(const double *point, const double height, const std::string text)
{
    // not yet implemented
}


void DraftDxfRead::AddObject(Part::TopoShape *shape)
{
    if (optionGroupLayers) {
        std::cout << "layer:" << LayerName() << std::endl;
        std::vector <Part::TopoShape*> vec;
        if (layers.count(LayerName()))
            vec = layers[LayerName()];
        vec.push_back(shape);
        layers[LayerName()] = vec;
    } else {
        Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", "Shape");
        pcFeature->Shape.setValue(*shape);
    }
}


void DraftDxfRead::AddGraphics() const
{
    std::cout << "end of file" << std::endl;
    if (optionGroupLayers) {
        for(std::map<std::string,std::vector<Part::TopoShape*> > ::const_iterator i = layers.begin(); i != layers.end(); ++i) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            std::string k = i->first;
            std::vector<Part::TopoShape*> v = i->second;
            std::cout << "joining:" << k << " size " << v.size() << std::endl;
            for(std::vector<Part::TopoShape*>::const_iterator j = v.begin(); j != v.end(); ++j) { 
                const TopoDS_Shape& sh = (*j)->_Shape;
                if (!sh.IsNull())
                    builder.Add(comp, sh);
            }
            if (!comp.IsNull()) {
                std::cout << "valid shape" << std::endl;
                Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", k.c_str());
                pcFeature->Shape.setValue(comp);
            } 
            else std::cout << "invalid shape" << std::endl;
        }
    }
}



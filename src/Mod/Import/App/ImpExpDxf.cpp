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

#include "ImpExpDxf.h"

#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_HCurve.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Lin.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Annotation.h>
#include <Mod/Part/App/PartFeature.h>

using namespace Import;


//******************************************************************************
// reading
ImpExpDxfRead::ImpExpDxfRead(std::string filepath, App::Document *pcDoc) : CDxfRead(filepath.c_str())
{
    document = pcDoc;
    setOptionSource("User parameter:BaseApp/Preferences/Mod/Draft");
    setOptions();
}

void ImpExpDxfRead::setOptions(void)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(getOptionSource().c_str());
    optionGroupLayers = hGrp->GetBool("groupLayers",false);
    optionImportAnnotations = hGrp->GetBool("dxftext",false);
    optionScaling = hGrp->GetFloat("dxfScaling",1.0);
}

gp_Pnt ImpExpDxfRead::makePoint(const double* p)
{
    double sp1(p[0]);
    double sp2(p[1]);
    double sp3(p[2]);
    if (optionScaling != 1.0) {
        sp1 = sp1 * optionScaling;
        sp2 = sp2 * optionScaling;
        sp3 = sp3 * optionScaling;
    }
    return gp_Pnt(sp1,sp2,sp3);
}

void ImpExpDxfRead::OnReadLine(const double* s, const double* e, bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(s);
    gp_Pnt p1 = makePoint(e);
    if (p0.IsEqual(p1,0.00000001))
        return;
    BRepBuilderAPI_MakeEdge makeEdge(p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void ImpExpDxfRead::OnReadPoint(const double* s)
{
    BRepBuilderAPI_MakeVertex makeVertex(makePoint(s));
    TopoDS_Vertex vertex = makeVertex.Vertex();
    AddObject(new Part::TopoShape(vertex));
}


void ImpExpDxfRead::OnReadArc(const double* s, const double* e, const double* c, bool dir, bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(s);
    gp_Pnt p1 = makePoint(e);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc = makePoint(c);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle, p0, p1);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void ImpExpDxfRead::OnReadCircle(const double* s, const double* c, bool dir, bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(s);
    gp_Dir up(0, 0, 1);
    if (!dir)
        up = -up;
    gp_Pnt pc = makePoint(c);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    BRepBuilderAPI_MakeEdge makeEdge(circle);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void ImpExpDxfRead::OnReadSpline(struct SplineData& /*sd*/)
{
    // not yet implemented
}


void ImpExpDxfRead::OnReadEllipse(const double* c, double major_radius, double minor_radius, double rotation, double /*start_angle*/, double /*end_angle*/, bool dir)
{
    gp_Dir up(0, 0, 1);
    if(!dir)
        up = -up;
    gp_Pnt pc = makePoint(c);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius * optionScaling, minor_radius * optionScaling);
    ellipse.Rotate(gp_Ax1(pc,up),rotation);
    BRepBuilderAPI_MakeEdge makeEdge(ellipse);
    TopoDS_Edge edge = makeEdge.Edge();
    AddObject(new Part::TopoShape(edge));
}


void ImpExpDxfRead::OnReadText(const double *point, const double /*height*/, const char* text)
{
    if (optionImportAnnotations) {
        Base::Vector3d pt(point[0] * optionScaling, point[1] * optionScaling, point[2] * optionScaling);
        if(LayerName().substr(0, 6) != "BLOCKS") {
            App::Annotation *pcFeature = (App::Annotation *)document->addObject("App::Annotation", "Text");
            pcFeature->LabelText.setValue(Deformat(text));
            pcFeature->Position.setValue(pt);
        }
        //else std::cout << "skipped text in block: " << LayerName() << std::endl;
    }
}


void ImpExpDxfRead::OnReadInsert(const double* point, const double* scale, const char* name, double rotation)
{
    //std::cout << "Inserting block " << name << " rotation " << rotation << " pos " << point[0] << "," << point[1] << "," << point[2] << " scale " << scale[0] << "," << scale[1] << "," << scale[2] << std::endl;
    std::string prefix = "BLOCKS ";
    prefix += name;
    prefix += " ";
    for(std::map<std::string,std::vector<Part::TopoShape*> > ::const_iterator i = layers.begin(); i != layers.end(); ++i) {
        std::string k = i->first;
        if(k.substr(0, prefix.size()) == prefix) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            std::vector<Part::TopoShape*> v = i->second;
            for(std::vector<Part::TopoShape*>::const_iterator j = v.begin(); j != v.end(); ++j) { 
                const TopoDS_Shape& sh = (*j)->getShape();
                if (!sh.IsNull())
                    builder.Add(comp, sh);
            }
            if (!comp.IsNull()) {
                Part::TopoShape* pcomp = new Part::TopoShape(comp);
                Base::Matrix4D mat;
                mat.scale(scale[0],scale[1],scale[2]);
                mat.rotZ(rotation);
                mat.move(point[0]*optionScaling,point[1]*optionScaling,point[2]*optionScaling);
                pcomp->transformShape(mat,true);
                AddObject(pcomp);
            }
        }
    } 
}


void ImpExpDxfRead::OnReadDimension(const double* s, const double* e, const double* point, double /*rotation*/)
{
    if (optionImportAnnotations) {
        Base::Interpreter().runString("import Draft");
        Base::Interpreter().runStringArg("p1=FreeCAD.Vector(%f,%f,%f)",s[0]*optionScaling,s[1]*optionScaling,s[2]*optionScaling);
        Base::Interpreter().runStringArg("p2=FreeCAD.Vector(%f,%f,%f)",e[0]*optionScaling,e[1]*optionScaling,e[2]*optionScaling);
        Base::Interpreter().runStringArg("p3=FreeCAD.Vector(%f,%f,%f)",point[0]*optionScaling,point[1]*optionScaling,point[2]*optionScaling);
        Base::Interpreter().runString("Draft.makeDimension(p1,p2,p3)");
    }
}


void ImpExpDxfRead::AddObject(Part::TopoShape *shape)
{
    //std::cout << "layer:" << LayerName() << std::endl;
    std::vector <Part::TopoShape*> vec;
    if (layers.count(LayerName()))
        vec = layers[LayerName()];
    vec.push_back(shape);
    layers[LayerName()] = vec;
    if (!optionGroupLayers) {
        if(LayerName().substr(0, 6) != "BLOCKS") {
            Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", "Shape");
            pcFeature->Shape.setValue(shape->getShape());
        }
    }
}


std::string ImpExpDxfRead::Deformat(const char* text)
{
    // this function removes DXF formatting from texts
    std::stringstream ss;
    bool escape = false; // turned on when finding an escape character
    bool longescape = false; // turned on for certain escape codes that expect additional chars
    for(unsigned int i = 0; i<strlen(text); i++) {
        if (text[i] == '\\')
            escape = true;
        else if (escape) {
            if (longescape) {
                if (text[i] == ';') {
                    escape = false;
                    longescape = false;
                }
            } else {
                if ( (text[i] == 'H') || (text[i] == 'h') ||
                     (text[i] == 'Q') || (text[i] == 'q') ||
                     (text[i] == 'W') || (text[i] == 'w') ||
                     (text[i] == 'F') || (text[i] == 'f') ||
                     (text[i] == 'A') || (text[i] == 'a') ||
                     (text[i] == 'C') || (text[i] == 'c') ||
                     (text[i] == 'T') || (text[i] == 't') )
                    longescape = true;
                else {
                    if ( (text[i] == 'P') || (text[i] == 'p') )
                        ss << "\n";
                    escape = false;
                }
            }
        }
        else if ( (text[i] != '{') && (text[i] != '}') ) {
            ss << text[i];
        }
    }
    return ss.str();
}


void ImpExpDxfRead::AddGraphics() const
{
    if (optionGroupLayers) {
        for(std::map<std::string,std::vector<Part::TopoShape*> > ::const_iterator i = layers.begin(); i != layers.end(); ++i) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            std::string k = i->first;
            if (k == "0") // FreeCAD doesn't like an object name being '0'...
                k = "LAYER_0";
            std::vector<Part::TopoShape*> v = i->second;
            if(k.substr(0, 6) != "BLOCKS") {
                for(std::vector<Part::TopoShape*>::const_iterator j = v.begin(); j != v.end(); ++j) { 
                    const TopoDS_Shape& sh = (*j)->getShape();
                    if (!sh.IsNull())
                        builder.Add(comp, sh);
                }
                if (!comp.IsNull()) {
                    Part::Feature *pcFeature = (Part::Feature *)document->addObject("Part::Feature", k.c_str());
                    pcFeature->Shape.setValue(comp);
                } 
            }
        }
    }
}

//******************************************************************************
// writing

void gPntToTuple(double* result, gp_Pnt& p)
{
   result[0] = p.X();
   result[1] = p.Y();
   result[2] = p.Z();
}

point3D gPntTopoint3D(gp_Pnt& p)
{ 
   point3D result;
   result.x = p.X();
   result.y = p.Y();
   result.z = p.Z();
   return result;
}

ImpExpDxfWrite::ImpExpDxfWrite(std::string filepath) : 
    CDxfWrite(filepath.c_str()),
    m_layerName("none")
{
    setOptionSource("User parameter:BaseApp/Preferences/Mod/Draft");
    setOptions();
}

void ImpExpDxfWrite::setOptions(void)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(getOptionSource().c_str());
    optionMaxLength = hGrp->GetFloat("maxsegmentlength",5.0);
    optionPolyLine  = hGrp->GetBool("DiscretizeEllipses",true);
}

void ImpExpDxfWrite::exportShape(const TopoDS_Shape input)
{
    //export Edges
    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1 ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt s = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);
            if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                exportCircle(adapt);
            } else {
                exportArc(adapt);
            }
        } else if (adapt.GetType() == GeomAbs_Ellipse) {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt s = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);
            if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
                exportEllipse(adapt);
            } else {
                exportEllipseArc(adapt);
            }

        } else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            if (optionPolyLine) {
                exportLWPoly(adapt);
            } else {
                exportBSpline(adapt);
            }
        } else if (adapt.GetType() == GeomAbs_BezierCurve) {
            exportBCurve(adapt);
        } else if (adapt.GetType() == GeomAbs_Line) {
            exportLine(adapt);
        } else {
            Base::Console().Warning("ImpExpDxf - unknown curve type: %d\n",adapt.GetType());
        }
    }

    //export Vertices
//    TopExp_Explorer verts(input, TopAbs_VERTEX);
//    for (int i = 1 ; verts.More(); verts.Next(),i++) {
//        const TopoDS_Vertex& v = TopoDS::Vertex(verts.Current());

//    }
}

void ImpExpDxfWrite::exportCircle(BRepAdaptor_Curve c)
{
    gp_Circ circ = c.Circle();
    gp_Pnt p = circ.Location();
    double center[3] = {0,0,0};
    gPntToTuple(center, p);

    double  radius = circ.Radius();

    WriteCircle(center, radius, getLayerName().c_str());
}

void ImpExpDxfWrite::exportEllipse(BRepAdaptor_Curve c)
{
    gp_Elips ellp = c.Ellipse();
    gp_Pnt p = ellp.Location();
    double center[3] = {0,0,0};
    gPntToTuple(center, p);

    double major = ellp.MajorRadius();
    double minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();       //direction of major axis
    //rotation appears to be the clockwise(?) angle between major & +Y??
    double rotation = xaxis.AngleWithRef(gp_Dir(0, 1, 0), gp_Dir(0, 0, 1));
    
    //2*M_PI = 6.28319 is invalid(doesn't display in LibreCAD), but 2PI = 6.28318 is valid!
    //WriteEllipse(center, major, minor, rotation, 0.0, 2 * M_PI, true, getLayerName().c_str() );
    WriteEllipse(center, major, minor, rotation, 0.0, 6.28318, true, getLayerName().c_str() );
}

void ImpExpDxfWrite::exportArc(BRepAdaptor_Curve c)
{
    gp_Circ circ = c.Circle();
    gp_Pnt p = circ.Location();
    double center[3] = {0,0,0};
    gPntToTuple(center, p);

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    double start[3];
    gPntToTuple(start, s);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);
    double end[3] = {0,0,0};
    gPntToTuple(end, e);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    bool dir = (a < 0) ? true: false;
    WriteArc(start, end, center, dir, getLayerName().c_str() );
}

void ImpExpDxfWrite::exportEllipseArc(BRepAdaptor_Curve c)
{
    gp_Elips ellp = c.Ellipse();
    gp_Pnt p = ellp.Location();
    double center[3] = {0,0,0};
    gPntToTuple(center, p);

    double major = ellp.MajorRadius();
    double minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();       //direction of major axis
    //rotation appears to be the clockwise angle between major & +Y??
    double rotation = xaxis.AngleWithRef(gp_Dir(0, 1, 0), gp_Dir(0, 0, 1));

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    double startAngle = fmod(f,2.0*M_PI);
    double endAngle = fmod(l,2.0*M_PI);
    bool dir = (a < 0) ? true: false;

    WriteEllipse(center, major, minor, rotation, startAngle, endAngle, dir, getLayerName().c_str() );
}

void ImpExpDxfWrite::exportBSpline(BRepAdaptor_Curve c)
{
    SplineDataOut sd;
    Handle(Geom_BSplineCurve) spline;
    double f,l;
    gp_Pnt s,ePt;

    Standard_Real tol3D = 0.001;
    Standard_Integer maxDegree = 3, maxSegment = 100;
    Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
    Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
    if (approx.IsDone() && approx.HasResult()) {
        spline = approx.Curve();
    } else {
        if (approx.HasResult()) {                   //result, but not within tolerance
            spline = approx.Curve();
            Base::Console().Message("DxfWrite::exportBSpline - result not within tolerance\n");
        } else {
            f = c.FirstParameter();
            l = c.LastParameter();
            s = c.Value(f);
            ePt = c.Value(l);
            Base::Console().Message("DxfWrite::exportBSpline - no result- from:(%.3f,%.3f) to:(%.3f,%.3f) poles: %d\n",
                                 s.X(),s.Y(),ePt.X(),ePt.Y(),spline->NbPoles());
            TColgp_Array1OfPnt controlPoints(0,1);
            controlPoints.SetValue(0,s);
            controlPoints.SetValue(1,ePt);
            spline = GeomAPI_PointsToBSpline(controlPoints,1).Curve();
        }
    }
    //WF? norm of surface containing curve??
    sd.norm.x = 0.0;
    sd.norm.y = 0.0;
    sd.norm.z = 1.0;

    sd.flag = spline->IsClosed();
    sd.flag += spline->IsPeriodic()*2;
    sd.flag += spline->IsRational()*4;

    sd.degree = spline->Degree();
    sd.control_points = spline->NbPoles();
    sd.knots  = spline->NbKnots();
    gp_Pnt p;
    spline->D0(spline->FirstParameter(),p);
    sd.starttan = gPntTopoint3D(p);
    spline->D0(spline->LastParameter(),p);
    sd.endtan = gPntTopoint3D(p);

    TColStd_Array1OfReal knotsequence(1,sd.knots);
    spline->KnotSequence(knotsequence);
    for (int i = knotsequence.Lower() ; i <= knotsequence.Upper(); i++) {
        sd.knot.push_back(knotsequence(i));
    }
    TColgp_Array1OfPnt poles(1,spline->NbPoles());
    spline->Poles(poles);
    for (int i = poles.Lower(); i <= poles.Upper(); i++) {
        sd.control.push_back(gPntTopoint3D(poles(i)));
    }
    //OCC doesn't have separate lists for control points and fit points. 
    
    WriteSpline(sd,getLayerName().c_str());
}

void ImpExpDxfWrite::exportBCurve(BRepAdaptor_Curve c)
{
    (void) c;
    Base::Console().Message("BCurve dxf export not yet supported\n");
}

void ImpExpDxfWrite::exportLine(BRepAdaptor_Curve c)
{
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    double start[3] = {0,0,0};
    gPntToTuple(start, s);
    gp_Pnt e = c.Value(l);
    double end[3] = {0,0,0};
    gPntToTuple(end, e);
    WriteLine(start, end, getLayerName().c_str());
}

void ImpExpDxfWrite::exportLWPoly(BRepAdaptor_Curve c)
{
    LWPolyDataOut pd;
    pd.Flag = c.IsClosed();
    pd.Elev = 0.0;
    pd.Thick = 0.0;
    pd.Extr.x = 0.0;
    pd.Extr.y = 0.0;
    pd.Extr.z = 1.0;

    GCPnts_UniformAbscissa discretizer;
    discretizer.Initialize (c, optionMaxLength);
    std::vector<point3D> points;
    if (discretizer.IsDone () && discretizer.NbPoints () > 0) {
        int nbPoints = discretizer.NbPoints ();
        for (int i=1; i<=nbPoints; i++) {
            gp_Pnt p = c.Value (discretizer.Parameter (i));
            pd.Verts.push_back(gPntTopoint3D(p));
        }
        WriteLWPolyLine(pd,getLayerName().c_str());
    }
}



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
# include <sstream>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Circle.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
#endif

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <HLRBRep_Algo.hxx>
#include <TopoDS_Shape.hxx>
#include <HLRTopoBRep_OutLiner.hxx>
//#include <BRepAPI_MakeOutLine.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh.hxx>

#include <BRepAdaptor_CompCurve.hxx>
#include <Handle_BRepAdaptor_HCompCurve.hxx>
#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_HCurve.hxx>
#include <Handle_BRepAdaptor_HCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Handle_Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomConvert_BSplineCurveToBezierCurve.hxx>
#include <GeomConvert_BSplineCurveKnotSplitting.hxx>
#include <Geom2d_BSplineCurve.hxx>

#include "DrawingExport.h"
#include <Base/Tools.h>
#include <Base/Vector3D.h>

using namespace Drawing;

SVGOutput::SVGOutput()
{
}

std::string SVGOutput::exportEdges(const TopoDS_Shape& input)
{
    std::stringstream result;

    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1 ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            printCircle(adapt, result);
        }
        else if (adapt.GetType() == GeomAbs_Ellipse) {
            printEllipse(adapt, i, result);
        }
        else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            printBSpline(adapt, i, result);
        }
        // fallback
        else {
            printGeneric(adapt, i, result);
        }
    }

    return result.str();
}

void SVGOutput::printCircle(const BRepAdaptor_Curve& c, std::ostream& out)
{
    gp_Circ circ = c.Circle();
    const gp_Pnt& p= circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full circle
    if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
        out << "<circle cx =\"" << p.X() << "\" cy =\"" 
            << p.Y() << "\" r =\"" << r << "\" />";
    }
    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        char xar = '0'; // x-axis-rotation
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r << " " << r << " "
            << xar << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";
    }
}

void SVGOutput::printEllipse(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p= ellp.Location();
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full ellipse
    if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
        out << "<ellipse cx =\"" << p.X() << "\" cy =\"" 
            << p.Y() << "\" rx =\"" << r1 << "\"  ry =\"" << r2 << "\"/>";
    }
    // arc of ellipse
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        gp_Dir xaxis = ellp.XAxis().Direction();
        Standard_Real angle = xaxis.Angle(gp_Dir(1,0,0));
        angle = Base::toDegrees<double>(angle);
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r1 << " " << r2 << " "
            << angle << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";
    }
}

void SVGOutput::printBSpline(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    try {
        std::stringstream str;
        Handle_Geom_BSplineCurve spline = c.BSpline();
        if (spline->Degree() > 3) {
            Standard_Real tol3D = 0.001;
            Standard_Integer maxDegree = 3, maxSegment = 10;
            Handle_BRepAdaptor_HCurve hCurve = new BRepAdaptor_HCurve(c);
            // approximate the curve using a tolerance
            Approx_Curve3d approx(hCurve,tol3D,GeomAbs_C2,maxSegment,maxDegree);
            if (approx.IsDone() && approx.HasResult()) {
                // have the result
                spline = approx.Curve();
            }
        }

        GeomConvert_BSplineCurveToBezierCurve crt(spline);
        Standard_Integer arcs = crt.NbArcs();
        str << "<path d=\"M";
        for (Standard_Integer i=1; i<=arcs; i++) {
            Handle_Geom_BezierCurve bezier = crt.Arc(i);
            Standard_Integer poles = bezier->NbPoles();
            if (bezier->Degree() == 3) {
                if (poles != 4)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                gp_Pnt p4 = bezier->Pole(4);
                if (i == 1) {
                    str << p1.X() << "," << p1.Y() << " C"
                        << p2.X() << "," << p2.Y() << " "
                        << p3.X() << "," << p3.Y() << " "
                        << p4.X() << "," << p4.Y() << " ";
                }
                else {
                    str << "S"
                        << p3.X() << "," << p3.Y() << " "
                        << p4.X() << "," << p4.Y() << " ";
                }
            }
            else if (bezier->Degree() == 2) {
                if (poles != 3)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                if (i == 1) {
                    str << p1.X() << "," << p1.Y() << " Q"
                        << p2.X() << "," << p2.Y() << " "
                        << p3.X() << "," << p3.Y() << " ";
                }
                else {
                    str << "T"
                        << p3.X() << "," << p3.Y() << " ";
                }
            }
            else {
                Standard_Failure::Raise("do it the generic way");
            }
        }

        str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure) {
        printGeneric(c, id, out);
    }
}

void SVGOutput::printGeneric(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(c.Edge(), location);
    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt& nodes = polygon->Nodes();
        char c = 'M';
        out << "<path id= \"" /*<< ViewName*/ << id << "\" d=\" "; 
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++){
            out << c << " " << nodes(i).X() << " " << nodes(i).Y()<< " " ; 
            c = 'L';
        }
        out << "\" />" << endl;
    }
}

// ----------------------------------------------------------------------------

DXFOutput::DXFOutput()
{
}

std::string DXFOutput::exportEdges(const TopoDS_Shape& input)
{
    std::stringstream result;

    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1 ; edges.More(); edges.Next(),i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            printCircle(adapt, result);
        }
        else if (adapt.GetType() == GeomAbs_Ellipse) {
            printEllipse(adapt, i, result);
        }
        else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            printBSpline(adapt, i, result);
        }
        // fallback
        else {
            printGeneric(adapt, i, result);
        }
    }

    return result.str();
}

void DXFOutput::printHeader( std::ostream& out)
{
        out	 << 0          << endl;
        out << "SECTION"  << endl;
        out << 2          << endl;
        out << "ENTITIES" << endl;
}

void DXFOutput::printCircle(const BRepAdaptor_Curve& c, std::ostream& out)
{
    gp_Circ circ = c.Circle();
	//const gp_Ax1& axis = c->Axis();
    const gp_Pnt& p= circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full circle
    if (s.SquareDistance(e) < 0.001) {
        //out << "<circle cx =\"" << p.X() << "\" cy =\"" 
            //<< p.Y() << "\" r =\"" << r << "\" />";
	    out << 0			<< endl;
	    out << "CIRCLE"		<< endl;
	    out << 8			<< endl;	// Group code for layer name
	    out << "sheet_layer"	<< endl;	// Layer number
	    out << 10			<< endl;	// Centre X
	    out << p.X()		<< endl;	// X in WCS coordinates
	    out << 20			<< endl;
	    out << p.Y()		<< endl;	// Y in WCS coordinates
	    out << 30			<< endl;
	    out << 0		<< endl;	// Z in WCS coordinates-leaving flat
	    out << 40			<< endl;	//
	    out << r		<< endl;	// Radius
                                }



    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        /*char xar = '0'; // x-axis-rotation
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r << " " << r << " "
            << xar << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";*/
	double ax = s.X() - p.X();
	double ay = s.Y() - p.Y();
	double bx = e.X() - p.X();
	double by = e.Y() - p.Y();

	double start_angle = atan2(ay, ax) * 180/D_PI;
	double end_angle = atan2(by, bx) * 180/D_PI;


	if(a > 0){
		double temp = start_angle;
		start_angle = end_angle;
		end_angle = temp;}
	out << 0			<< endl;
	out << "ARC"		<< endl;
	out << 8			<< endl;	// Group code for layer name
	out << "sheet_layer"	<< endl;	// Layer number
	out << 10			<< endl;	// Centre X
	out << p.X()		<< endl;	// X in WCS coordinates
	out << 20			<< endl;
	out << p.Y()		<< endl;	// Y in WCS coordinates
	out << 30			<< endl;
	out << 0		<< endl;	// Z in WCS coordinates
	out << 40			<< endl;	//
	out << r		<< endl;	// Radius
	out << 50			<< endl;
	out << start_angle	<< endl;	// Start angle
	out << 51			<< endl;
	out << end_angle	<< endl;	// End angle



    }
}

void DXFOutput::printEllipse(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p= ellp.Location();
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1);
    double a = v3.DotCross(v1,v2);

    // a full ellipse
   /* if (s.SquareDistance(e) < 0.001) {
        out << "<ellipse cx =\"" << p.X() << "\" cy =\"" 
            << p.Y() << "\" rx =\"" << r1 << "\"  ry =\"" << r2 << "\"/>";
    }
    // arc of ellipse
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        gp_Dir xaxis = ellp.XAxis().Direction();
        Standard_Real angle = xaxis.Angle(gp_Dir(1,0,0));
        angle = Base::toDegrees<double>(angle);
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r1 << " " << r2 << " "
            << angle << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";
    }*/
        gp_Dir xaxis = ellp.XAxis().Direction();
        double angle = xaxis.Angle(gp_Dir(1,0,0));
        //double rotation = Base::toDegrees<double>(angle);


	double ax = s.X() - p.X();
	double ay = s.Y() - p.Y();
	double bx = e.X() - p.X();
	double by = e.Y() - p.Y();

	double start_angle = atan2(ay, ax) * 180/D_PI;
	double end_angle = atan2(by, bx) * 180/D_PI;

	double major_x;double major_y;
	
	major_x = r1 * sin(angle*90);
	major_y = r1 * cos(angle*90);

	double ratio = r2/r1;

	if(a > 0){
		double temp = start_angle;
		start_angle = end_angle;
		end_angle = temp;
	}
	out << 0			<< endl;
	out << "ELLIPSE"		<< endl;
	out << 8			<< endl;	// Group code for layer name
	out << "sheet_layer"	<< endl;	// Layer number
	out << 10			<< endl;	// Centre X
	out << p.X()		<< endl;	// X in WCS coordinates
	out << 20			<< endl;
	out << p.Y()		<< endl;	// Y in WCS coordinates
	out << 30			<< endl;
	out << 0		<< endl;	// Z in WCS coordinates
	out << 11			<< endl;	//
	out << major_x		<< endl;	// Major X
	out << 21			<< endl;
	out << major_y		<< endl;	// Major Y
	out << 31			<< endl;
	out << 0		<< endl;	// Major Z
	out << 40			<< endl;	//
	out << ratio		<< endl;	// Ratio
	out << 41		<< endl;
	out << start_angle	<< endl;	// Start angle
	out << 42		<< endl;
	out << end_angle	<< endl;	// End angle
}

void DXFOutput::printBSpline(const BRepAdaptor_Curve& c, int id, std::ostream& out) //Not even close yet- DF 
{
    try {
        std::stringstream str;
        Handle_Geom_BSplineCurve spline = c.BSpline();
        if (spline->Degree() > 3) {
            Standard_Real tol3D = 0.001;
            Standard_Integer maxDegree = 3, maxSegment = 10;
            Handle_BRepAdaptor_HCurve hCurve = new BRepAdaptor_HCurve(c);
            // approximate the curve using a tolerance
            Approx_Curve3d approx(hCurve,tol3D,GeomAbs_C2,maxSegment,maxDegree);
            if (approx.IsDone() && approx.HasResult()) {
                // have the result
                spline = approx.Curve();
            }
        }
		
        GeomConvert_BSplineCurveToBezierCurve crt(spline);
		//GeomConvert_BSplineCurveKnotSplitting crt(spline,0);
        Standard_Integer arcs = crt.NbArcs();
		//Standard_Integer arcs = crt.NbSplits()-1;
        	str << 0 << endl
				<< "SECTION" << endl
				<< 2 << endl
				<< "ENTITIES" << endl
				<< 0 << endl
				<< "SPLINE" << endl;
				//<< 8 << endl
				//<< 0 << endl
				//<< 66 << endl
				//<< 1 << endl
				//<< 0 << endl;

        for (Standard_Integer i=1; i<=arcs; i++) {
            Handle_Geom_BezierCurve bezier = crt.Arc(i);
            Standard_Integer poles = bezier->NbPoles();
			//Standard_Integer poles = bspline->NbPoles();
			//gp_Pnt p1 = bspline->Pole(1);

            if (bezier->Degree() == 3) {
                if (poles != 4)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                gp_Pnt p4 = bezier->Pole(4);
                if (i == 1) {
                    str 
						<< 10 << endl
						<< p1.X() << endl
						<< 20 << endl
						<< p1.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p2.X() << endl
						<< 20 << endl
						<< p2.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p3.X() << endl
						<< 20 << endl
						<< p3.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p4.X() << endl
						<< 20 << endl
						<< p4.Y() << endl
						<< 30 << endl
						<< 0 << endl

						<< 12 << endl
						<< p1.X() << endl
						<< 22 << endl
						<< p1.Y() << endl
						<< 32 << endl
						<< 0 << endl

						<< 13 << endl
						<< p4.X() << endl
						<< 23 << endl
						<< p4.Y() << endl
						<< 33 << endl
						<< 0 << endl;
                }
                else {
                    str 
						<< 10 << endl
                        << p3.X() << endl
						<< 20 << endl
						<< p3.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p4.X() << endl
						<< 20 << endl
						<< p4.Y() << endl
						<< 30 << endl
						<< 0 << endl

						<< 12 << endl
						<< p3.X() << endl
						<< 22 << endl
						<< p3.Y() << endl
						<< 32 << endl
						<< 0 << endl

						<< 13 << endl
						<< p4.X() << endl
						<< 23 << endl
						<< p4.Y() << endl
						<< 33 << endl
						<< 0 << endl;

                }
            }
            else if (bezier->Degree() == 2) {
                if (poles != 3)
                    Standard_Failure::Raise("do it the generic way");
                gp_Pnt p1 = bezier->Pole(1);
                gp_Pnt p2 = bezier->Pole(2);
                gp_Pnt p3 = bezier->Pole(3);
                if (i == 1) {
                    str 
						<< 10 << endl
						<< p1.X() << endl
						<< 20 << endl
						<< p1.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p2.X() << endl
						<< 20 << endl
						<< p2.Y() << endl
						<< 30 << endl
						<< 0 << endl
						
						<< 10 << endl
                        << p3.X() << endl
						<< 20 << endl
						<< p3.Y() << endl
						<< 30 << endl
						<< 0 << endl

						<< 12 << endl
						<< p1.X() << endl
						<< 22 << endl
						<< p1.Y() << endl
						<< 32 << endl
						<< 0 << endl

						<< 13 << endl
						<< p3.X() << endl
						<< 23 << endl
						<< p3.Y() << endl
						<< 33 << endl
						<< 0 << endl;
                }
                else {
                    str 
						<< 10 << endl
                        << p3.X() << endl
						<< 20 << endl
						<< p3.Y() << endl
						<< 30 << endl
						<< 0 << endl;
                }
            }
            else {
                Standard_Failure::Raise("do it the generic way");
            }
        }

        //str << "\" />";
        out << str.str();
    }
    catch (Standard_Failure) {
        printGeneric(c, id, out);
    }
}

void DXFOutput::printGeneric(const BRepAdaptor_Curve& c, int id, std::ostream& out)
{
    double uStart = c.FirstParameter();
    gp_Pnt PS;
    gp_Vec VS;
    c.D1(uStart, PS, VS);

    double uEnd = c.LastParameter();
    gp_Pnt PE;
    gp_Vec VE;
    c.D1(uEnd, PE, VE);

    out << "0"			<< endl;
    out << "LINE"		<< endl;
    out << "8"			<< endl;	// Group code for layer name
    out << "sheet_layer" << endl; // Layer name 
    out << "10"			<< endl;	// Start point of line
    out << PS.X()		<< endl;	// X in WCS coordinates
    out << "20"			<< endl;
    out << PS.Y()		<< endl;	// Y in WCS coordinates
    out << "30"			<< endl;
    out << "0"		<< endl;	// Z in WCS coordinates
    out << "11"			<< endl;	// End point of line
    out << PE.X()		<< endl;	// X in WCS coordinates
    out << "21"			<< endl;
    out << PE.Y()		<< endl;	// Y in WCS coordinates
    out << "31"			<< endl;
    out << "0"		<< endl;	// Z in WCS coordinates
}

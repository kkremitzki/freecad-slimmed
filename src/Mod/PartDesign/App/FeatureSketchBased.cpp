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
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRep_Tool.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepProj_Projection.hxx>
# include <Geom_Plane.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp_Explorer.hxx>
# include <gp_Ax1.hxx>
# include <gp_Pln.hxx>
# include <ShapeFix_Face.hxx>
# include <ShapeFix_Wire.hxx>
# include <ShapeAnalysis.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
# include <IntTools_FClass2d.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <ShapeFix_Shape.hxx>
#endif


#include "FeatureSketchBased.h"
#include <Mod/Part/App/Part2DObject.h>

using namespace PartDesign;

namespace PartDesign {

// sort bounding boxes according to diagonal length
struct Wire_Compare {
    bool operator() (const TopoDS_Wire& w1, const TopoDS_Wire& w2)
    {
        Bnd_Box box1, box2;
        BRepBndLib::Add(w1, box1);
        box1.SetGap(0.0);

        BRepBndLib::Add(w2, box2);
        box2.SetGap(0.0);

        return box1.SquareExtent() < box2.SquareExtent();
    }
};

PROPERTY_SOURCE(PartDesign::SketchBased, PartDesign::Feature)

SketchBased::SketchBased()
{
    ADD_PROPERTY(Sketch,(0));
    ADD_PROPERTY_TYPE(Midplane,(0),"SketchBased", App::Prop_None, "Extrude symmetric to sketch face");
    ADD_PROPERTY_TYPE(Reversed, (0),"SketchBased", App::Prop_None, "Reverse extrusion direction");
}

short SketchBased::mustExecute() const
{
    if (Sketch.isTouched() ||
        Midplane.isTouched() ||
        Reversed.isTouched())
        return 1;
    return 0; // PartDesign::Feature::mustExecute();
}

void SketchBased::positionBySketch(void)
{
    Part::Part2DObject *sketch = static_cast<Part::Part2DObject*>(Sketch.getValue());
    if (sketch && sketch->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Part::Feature *part = static_cast<Part::Feature*>(sketch->Support.getValue());
        if (part && part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            this->Placement.setValue(part->Placement.getValue());
        else
            this->Placement.setValue(sketch->Placement.getValue());
    }
}

Part::Part2DObject* SketchBased::getVerifiedSketch() const {
    App::DocumentObject* result = Sketch.getValue();
    if (!result)
        throw Base::Exception("No sketch linked");
    if (!result->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        throw Base::Exception("Linked object is not a Sketch or Part2DObject");
    return static_cast<Part::Part2DObject*>(result);
}

std::vector<TopoDS_Wire> SketchBased::getSketchWires() const {
    std::vector<TopoDS_Wire> result;

    TopoDS_Shape shape = getVerifiedSketch()->Shape.getShape()._Shape;
    if (shape.IsNull())
        throw Base::Exception("Linked shape object is empty");

    // this is a workaround for an obscure OCC bug which leads to empty tessellations
    // for some faces. Making an explicit copy of the linked shape seems to fix it.
    // The error almost happens when re-computing the shape but sometimes also for the
    // first time
    BRepBuilderAPI_Copy copy(shape);
    shape = copy.Shape();
    if (shape.IsNull())
        throw Base::Exception("Linked shape object is empty");

    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        result.push_back(TopoDS::Wire(ex.Current()));
    }
    if (result.empty()) // there can be several wires
        throw Base::Exception("Linked shape object is not a wire");

    return result;
}

// TODO: This code is taken from and duplicates code in Part2DObject::positionBySupport()
// Note: We cannot return a reference, because it will become Null.
// Not clear where, because we check for IsNull() here, but as soon as it is passed out of
// this method, it becomes null!
const TopoDS_Face SketchBased::getSupportFace() const {
    const App::PropertyLinkSub& Support = static_cast<Part::Part2DObject*>(Sketch.getValue())->Support;
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::Exception("Sketch has no support shape");

    const std::vector<std::string> &sub = Support.getSubValues();
    assert(sub.size()==1);
    // get the selected sub shape (a Face)
    const Part::TopoShape &shape = part->Shape.getShape();
    if (shape._Shape.IsNull())
        throw Base::Exception("Sketch support shape is empty!");

    TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
    if (sh.IsNull())
        throw Base::Exception("Null shape in SketchBased::getSupportFace()!");

    const TopoDS_Face face = TopoDS::Face(sh);
    if (face.IsNull())
        throw Base::Exception("Null face in SketchBased::getSupportFace()!");

    BRepAdaptor_Surface adapt(face);
    if (adapt.GetType() != GeomAbs_Plane)
        throw Base::Exception("No planar face in SketchBased::getSupportFace()!");

    return face;
}

Part::Feature* SketchBased::getSupport() const {
    // get the support of the Sketch if any
    App::DocumentObject* SupportLink = static_cast<Part::Part2DObject*>(Sketch.getValue())->Support.getValue();
    Part::Feature* SupportObject = NULL;
    if (SupportLink && SupportLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        SupportObject = static_cast<Part::Feature*>(SupportLink);

    return SupportObject;
}

const TopoDS_Shape& SketchBased::getSupportShape() const {
    Part::Feature* SupportObject = getSupport();
    if (SupportObject == NULL)
        throw Base::Exception("No support in Sketch!");

    const TopoDS_Shape& result = SupportObject->Shape.getValue();
    if (result.IsNull())
        throw Base::Exception("Support shape is invalid");
    TopExp_Explorer xp (result, TopAbs_SOLID);
    if (!xp.More())
        throw Base::Exception("Support shape is not a solid");

    return result;
}

int SketchBased::getSketchAxisCount(void) const
{
    Part::Part2DObject *sketch = static_cast<Part::Part2DObject*>(Sketch.getValue());
    return sketch->getAxisCount();
}

void SketchBased::onChanged(const App::Property* prop)
{
    if (prop == &Sketch) {
        // if attached to a sketch then mark it as read-only
        this->Placement.StatusBits.set(2, Sketch.getValue() != 0);
    }

    Feature::onChanged(prop);
}

bool SketchBased::isInside(const TopoDS_Wire& wire1, const TopoDS_Wire& wire2) const
{
    Bnd_Box box1;
    BRepBndLib::Add(wire1, box1);
    box1.SetGap(0.0);

    Bnd_Box box2;
    BRepBndLib::Add(wire2, box2);
    box2.SetGap(0.0);

    if (box1.IsOut(box2))
        return false;

    double prec = Precision::Confusion();

    BRepBuilderAPI_MakeFace mkFace(wire1);
    TopoDS_Face face = validateFace(mkFace.Face());
    BRepAdaptor_Surface adapt(face);
    IntTools_FClass2d class2d(face, prec);
    Handle_Geom_Surface surf = new Geom_Plane(adapt.Plane());
    ShapeAnalysis_Surface as(surf);

    TopExp_Explorer xp(wire2,TopAbs_VERTEX);
    while (xp.More())  {
        TopoDS_Vertex v = TopoDS::Vertex(xp.Current());
        gp_Pnt p = BRep_Tool::Pnt(v);
        gp_Pnt2d uv = as.ValueOfUV(p, prec);
        if (class2d.Perform(uv) == TopAbs_IN)
            return true;
        // TODO: We can make a check to see if all points are inside or all outside
        // because otherwise we have some intersections which is not allowed
        else
            return false;
        xp.Next();
    }

    return false;
}

TopoDS_Face SketchBased::validateFace(const TopoDS_Face& face) const
{
    BRepCheck_Analyzer aChecker(face);
    if (!aChecker.IsValid()) {
        TopoDS_Wire outerwire = ShapeAnalysis::OuterWire(face);
        TopTools_IndexedMapOfShape myMap;
        myMap.Add(outerwire);

        TopExp_Explorer xp(face,TopAbs_WIRE);
        ShapeFix_Wire fix;
        fix.SetFace(face);
        fix.Load(outerwire);
        fix.Perform();
        BRepBuilderAPI_MakeFace mkFace(fix.WireAPIMake());
        while (xp.More()) {
            if (!myMap.Contains(xp.Current())) {
                fix.Load(TopoDS::Wire(xp.Current()));
                fix.Perform();
                mkFace.Add(fix.WireAPIMake());
            }
            xp.Next();
        }

        aChecker.Init(mkFace.Face());
        if (!aChecker.IsValid()) {
            ShapeFix_Shape fix(mkFace.Face());
            fix.SetPrecision(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.Perform();
            fix.FixWireTool()->Perform();
            fix.FixFaceTool()->Perform();
            return TopoDS::Face(fix.Shape());
        }
        return mkFace.Face();
    }

    return face;
}

TopoDS_Shape SketchBased::makeFace(std::list<TopoDS_Wire>& wires) const
{
    BRepBuilderAPI_MakeFace mkFace(wires.front());
    const TopoDS_Face& face = mkFace.Face();
    if (face.IsNull())
        return face;
    gp_Dir axis(0,0,1);
    BRepAdaptor_Surface adapt(face);
    if (adapt.GetType() == GeomAbs_Plane) {
        axis = adapt.Plane().Axis().Direction();
    }

    wires.pop_front();
    for (std::list<TopoDS_Wire>::iterator it = wires.begin(); it != wires.end(); ++it) {
        BRepBuilderAPI_MakeFace mkInnerFace(*it);
        const TopoDS_Face& inner_face = mkInnerFace.Face();
        if (inner_face.IsNull())
            return inner_face; // failure
        gp_Dir inner_axis(0,0,1);
        BRepAdaptor_Surface adapt(inner_face);
        if (adapt.GetType() == GeomAbs_Plane) {
            inner_axis = adapt.Plane().Axis().Direction();
        }
        // It seems that orientation is always 'Forward' and we only have to reverse
        // if the underlying plane have opposite normals.
        if (axis.Dot(inner_axis) < 0)
            it->Reverse();
        mkFace.Add(*it);
    }
    return validateFace(mkFace.Face());
}

TopoDS_Shape SketchBased::makeFace(const std::vector<TopoDS_Wire>& w) const
{
    if (w.empty())
        return TopoDS_Shape();

    //FIXME: Need a safe method to sort wire that the outermost one comes last
    // Currently it's done with the diagonal lengths of the bounding boxes
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), Wire_Compare());
    std::list<TopoDS_Wire> wire_list;
    wire_list.insert(wire_list.begin(), wires.rbegin(), wires.rend());

    // separate the wires into several independent faces
    std::list< std::list<TopoDS_Wire> > sep_wire_list;
    while (!wire_list.empty()) {
        std::list<TopoDS_Wire> sep_list;
        TopoDS_Wire wire = wire_list.front();
        wire_list.pop_front();
        sep_list.push_back(wire);

        std::list<TopoDS_Wire>::iterator it = wire_list.begin();
        while (it != wire_list.end()) {
            if (isInside(wire, *it)) {
                sep_list.push_back(*it);
                it = wire_list.erase(it);
            }
            else {
                ++it;
            }
        }

        sep_wire_list.push_back(sep_list);
    }

    if (sep_wire_list.size() == 1) {
        std::list<TopoDS_Wire>& wires = sep_wire_list.front();
        return makeFace(wires);
    }
    else if (sep_wire_list.size() > 1) {
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        for (std::list< std::list<TopoDS_Wire> >::iterator it = sep_wire_list.begin(); it != sep_wire_list.end(); ++it) {
            TopoDS_Shape aFace = makeFace(*it);
            if (!aFace.IsNull())
                builder.Add(comp, aFace);
        }

        return comp;
    }
    else {
        return TopoDS_Shape(); // error
    }
}

void SketchBased::getUpToFaceFromLinkSub(TopoDS_Face& upToFace,
                                         const App::PropertyLinkSub& refFace)
{
    App::DocumentObject* ref = refFace.getValue();
    std::vector<std::string> subStrings = refFace.getSubValues();

    if (ref == NULL)
        throw Base::Exception("SketchBased: Up to face: No face selected");
    if (!ref->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::Exception("SketchBased: Up to face: Must be face of a feature");
    Part::TopoShape baseShape = static_cast<Part::Feature*>(ref)->Shape.getShape();

    if (subStrings.empty() || subStrings[0].empty())
        throw Base::Exception("SketchBased: Up to face: No face selected");
    // TODO: Check for multiple UpToFaces?

    upToFace = TopoDS::Face(baseShape.getSubShape(subStrings[0].c_str()));
    if (upToFace.IsNull())
        throw Base::Exception("SketchBased: Up to face: Failed to extract face");
}

void SketchBased::getUpToFace(TopoDS_Face& upToFace,
                              const TopoDS_Shape& support,
                              const TopoDS_Face& supportface,
                              const TopoDS_Shape& sketchshape,
                              const std::string& method,
                              const gp_Dir& dir)
{
    if ((method == "UpToLast") || (method == "UpToFirst")) {
        // Check for valid support object
        if (support.IsNull())
            throw Base::Exception("SketchBased: Up to face: No support in Sketch!");

        std::vector<Part::cutFaces> cfaces = Part::findAllFacesCutBy(support, sketchshape, dir);
        if (cfaces.empty())
            throw Base::Exception("SketchBased: Up to face: No faces found in this direction");

        // Find nearest/furthest face
        std::vector<Part::cutFaces>::const_iterator it, it_near, it_far;
        it_near = it_far = cfaces.begin();
        for (it = cfaces.begin(); it != cfaces.end(); it++)
            if (it->distsq > it_far->distsq)
                it_far = it;
            else if (it->distsq < it_near->distsq)
                it_near = it;
        upToFace = (method == "UpToLast" ? it_far->face : it_near->face);
    }

    // Remove the limits of the upToFace so that the extrusion works even if sketchshape is larger
    // than the upToFace
    bool remove_limits = false;
    TopExp_Explorer Ex;
    for (Ex.Init(sketchshape,TopAbs_FACE); Ex.More(); Ex.Next()) {
        // Get outermost wire of sketch face
        TopoDS_Face sketchface = TopoDS::Face(Ex.Current());
        TopoDS_Wire outerWire = ShapeAnalysis::OuterWire(sketchface);
        if (!checkWireInsideFace(outerWire, upToFace, dir)) {
            remove_limits = true;
            break;
        }
    }

    if (remove_limits) {
        // Note: Using an unlimited face every time gives unnecessary failures for concave faces
        BRepAdaptor_Surface adapt(upToFace, Standard_False);
        BRepBuilderAPI_MakeFace mkFace(adapt.Surface().Surface());
        if (!mkFace.IsDone())
            throw Base::Exception("SketchBased: Up To Face: Failed to create unlimited face");
        upToFace = TopoDS::Face(mkFace.Shape());
    }

    // Check that the upToFace does not intersect the sketch face and
    // is not parallel to the extrusion direction
    BRepAdaptor_Surface adapt1(TopoDS::Face(supportface));
    BRepAdaptor_Surface adapt2(TopoDS::Face(upToFace));

    if (adapt2.GetType() == GeomAbs_Plane) {
        if (adapt1.Plane().Axis().IsNormal(adapt2.Plane().Axis(), Precision::Confusion()))
            throw Base::Exception("SketchBased: Up to face: Must not be parallel to extrusion direction!");
    }

    BRepExtrema_DistShapeShape distSS(supportface, upToFace);
    if (distSS.Value() < Precision::Confusion())
        throw Base::Exception("SketchBased: Up to face: Must not intersect sketch!");

}

void SketchBased::generatePrism(TopoDS_Shape& prism,
                                const TopoDS_Shape& sketchshape,
                                const std::string& method,
                                const gp_Dir& dir,
                                const double L,
                                const double L2,
                                const bool midplane,
                                const bool reversed)
{
    if (method == "Length" || method == "TwoLengths" || method == "ThroughAll") {
        double Ltotal = L;
        double Loffset = 0.;
        if (method == "ThroughAll")
            // "ThroughAll" is modelled as a very long, but finite prism to avoid problems with pockets
            // Note: 1E6 created problems once...
            Ltotal = 1E4;


        if (method == "TwoLengths") {
            // midplane makes no sense here
            Loffset = -L2;
            Ltotal += L2;
        } else if (midplane)
            Loffset = -Ltotal/2;

        TopoDS_Shape from = sketchshape;
        if (method == "TwoLengths" || midplane) {
            gp_Trsf mov;
            mov.SetTranslation(Loffset * gp_Vec(dir));
            TopLoc_Location loc(mov);
            from = sketchshape.Moved(loc);
        } else if (reversed)
            Ltotal *= -1.0;

        // Its better not to use BRepFeat_MakePrism here even if we have a support because the
        // resulting shape creates problems with Pocket
        BRepPrimAPI_MakePrism PrismMaker(from, Ltotal*gp_Vec(dir), 0,1); // finite prism
        if (!PrismMaker.IsDone())
            throw Base::Exception("SketchBased: Length: Could not extrude the sketch!");
        prism = PrismMaker.Shape();
    } else {
        throw Base::Exception("SketchBased: Internal error: Unknown method for generatePrism()");
    }

}

const bool SketchBased::checkWireInsideFace(const TopoDS_Wire& wire, const TopoDS_Face& face,
                                            const gp_Dir& dir) {
    // Project wire onto the face (face, not surface! So limits of face apply)
    // FIXME: For a user-selected upToFace, sometimes this returns a non-closed wire for no apparent reason
    // Check again after introduction of "robust" reference for upToFace
    BRepProj_Projection proj(wire, face, dir);
    return (proj.More() && proj.Current().Closed());
}

void SketchBased::remapSupportShape(const TopoDS_Shape& newShape)
{
    std::vector<App::DocumentObject*> refs = this->getInList();
    for (std::vector<App::DocumentObject*>::iterator it = refs.begin(); it != refs.end(); ++it) {
        if ((*it)->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
            Part::Part2DObject* part = static_cast<Part::Part2DObject*>(*it);
            Part::TopoShape shape = this->Shape.getValue();
            // here we must reset the placement otherwise the geometric matching doesn't work
            shape._Shape.Location(TopLoc_Location());
            std::vector<std::string> subValues = part->Support.getSubValues();
            std::vector<std::string> newSubValues;
            TopTools_IndexedMapOfShape faceMap;
            TopExp::MapShapes(newShape, TopAbs_FACE, faceMap);

            for (std::vector<std::string>::iterator it = subValues.begin(); it != subValues.end(); ++it) {
                std::string shapetype;
                if (it->size() > 4 && it->substr(0,4) == "Face") {
                    shapetype = "Face";
                }
                else if (it->size() > 4 && it->substr(0,4) == "Edge") {
                    shapetype = "Edge";
                }
                else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
                    shapetype = "Vertex";
                }
                else {
                    continue;
                }

                TopoDS_Shape element = shape.getSubShape(it->c_str());
                bool success = false;
                // first try an exact matching
                for (int i=1; i<faceMap.Extent(); i++) {
                    if (isQuasiEqual(element, faceMap.FindKey(i))) {
                        std::stringstream str;
                        str << shapetype << i;
                        newSubValues.push_back(str.str());
                        success = true;
                        break;
                    }
                }
                // if an exact matching fails then try to compare only the geometries
                if (!success) {
                    for (int i=1; i<faceMap.Extent(); i++) {
                        if (isEqualGeometry(element, faceMap.FindKey(i))) {
                            std::stringstream str;
                            str << shapetype << i;
                            newSubValues.push_back(str.str());
                            success = true;
                            break;
                        }
                    }
                }

                // the new shape couldn't be found so keep the old sub-name
                if (!success)
                    newSubValues.push_back(*it);
            }

            part->Support.setValue(this, newSubValues);
        }
    }
}

struct gp_Pnt_Less  : public std::binary_function<const gp_Pnt&,
                                                  const gp_Pnt&, bool>
{
    bool operator()(const gp_Pnt& p1,
                    const gp_Pnt& p2) const
    {
        if (fabs(p1.X() - p2.X()) > Precision::Confusion())
            return p1.X() < p2.X();
        if (fabs(p1.Y() - p2.Y()) > Precision::Confusion())
            return p1.Y() < p2.Y();
        if (fabs(p1.Z() - p2.Z()) > Precision::Confusion())
            return p1.Z() < p2.Z();
        return false; // points are considered to be equal
    }
};

bool SketchBased::isQuasiEqual(const TopoDS_Shape& s1, const TopoDS_Shape& s2) const
{
    if (s1.ShapeType() != s2.ShapeType())
        return false;
    TopTools_IndexedMapOfShape map1, map2;
    TopExp::MapShapes(s1, TopAbs_VERTEX, map1);
    TopExp::MapShapes(s2, TopAbs_VERTEX, map2);
    if (map1.Extent() != map2.Extent())
        return false;

    std::vector<gp_Pnt> p1;
    for (int i=1; i<=map1.Extent(); i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(map1.FindKey(i));
        p1.push_back(BRep_Tool::Pnt(v));
    }
    std::vector<gp_Pnt> p2;
    for (int i=1; i<=map2.Extent(); i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(map2.FindKey(i));
        p2.push_back(BRep_Tool::Pnt(v));
    }

    std::sort(p1.begin(), p1.end(), gp_Pnt_Less());
    std::sort(p2.begin(), p2.end(), gp_Pnt_Less());

    if (p1.size() != p2.size())
        return false;

    std::vector<gp_Pnt>::iterator it = p1.begin(), jt = p2.begin();
    for (; it != p1.end(); ++it, ++jt) {
        if (!(*it).IsEqual(*jt, Precision::Confusion()))
            return false;
    }

    return true;
}

bool SketchBased::isEqualGeometry(const TopoDS_Shape& s1, const TopoDS_Shape& s2)
{
    if (s1.ShapeType() == TopAbs_FACE && s2.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface a1(TopoDS::Face(s1));
        BRepAdaptor_Surface a2(TopoDS::Face(s2));
        if (a1.GetType() == GeomAbs_Plane && a2.GetType() == GeomAbs_Plane) {
            gp_Pln p1 = a1.Plane();
            gp_Pln p2 = a2.Plane();
            if (p1.Distance(p2.Location()) < Precision::Confusion()) {
                const gp_Dir& d1 = p1.Axis().Direction();
                const gp_Dir& d2 = p2.Axis().Direction();
                if (d1.IsParallel(d2, Precision::Confusion()))
                    return true;
            }
        }
    }
    else if (s1.ShapeType() == TopAbs_EDGE && s2.ShapeType() == TopAbs_EDGE) {
    }
    else if (s1.ShapeType() == TopAbs_VERTEX && s2.ShapeType() == TopAbs_VERTEX) {
        gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(s1));
        gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(s2));
        return p1.Distance(p2) < Precision::Confusion();
    }

    return false;
}

}

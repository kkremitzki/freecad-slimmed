/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>

#include <Mod/Part/App/Geometry.h>

#include <vector>

#include "SketchObject.h"
#include "SketchObjectPy.h"
#include "Sketch.h"

using namespace Sketcher;
using namespace Base;


PROPERTY_SOURCE(Sketcher::SketchObject, Part::Part2DObject)


SketchObject::SketchObject()
{
    ADD_PROPERTY_TYPE(Geometry,           (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch geometry");
    ADD_PROPERTY_TYPE(Constraints,        (0)  ,"Sketch",(App::PropertyType)(App::Prop_None),"Sketch constraints");
    ADD_PROPERTY_TYPE(ExternalConstraints,(0,0),"Sketch",(App::PropertyType)(App::Prop_None),"Sketch external constraints");
}

App::DocumentObjectExecReturn *SketchObject::execute(void)
{
    // recalculate support:
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (part && part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
    {
        Base::Placement ObjectPos = part->Placement.getValue();
        const std::vector<std::string> &sub = Support.getSubValues();
        assert(sub.size()==1);
        // get the selected sub shape (a Face)
        const Part::TopoShape &shape = part->Shape.getShape();
        if (shape._Shape.IsNull())
            return new App::DocumentObjectExecReturn("Support shape is empty!");
        TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
        const TopoDS_Face &face = TopoDS::Face(sh);
        assert(!face.IsNull());

        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane)
            return new App::DocumentObjectExecReturn("Sketch has no planar support!");

        // set sketch position
        Base::Placement placement = Part2DObject::positionBySupport(face,ObjectPos);
        Placement.setValue(placement);
    }

    // setup and diagnose the sketch
    Sketch sketch;
    int dofs = sketch.setUpSketch(Geometry.getValues(), Constraints.getValues());
    if (dofs < 0) { // over-constrained sketch
        std::string msg="Over-constrained sketch\n";
        appendConflictMsg(sketch.getConflicting(), msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }
    if (sketch.hasConflicts()) { // conflicting constraints
        std::string msg="Sketch with conflicting constraints\n";
        appendConflictMsg(sketch.getConflicting(), msg);
        return new App::DocumentObjectExecReturn(msg.c_str(),this);
    }

    // solve the sketch
    if (sketch.solve() != 0)
        return new App::DocumentObjectExecReturn("Solving the sketch failed",this);

    std::vector<Part::Geometry *> geomlist = sketch.getGeometry();
    Geometry.setValues(geomlist);
    for (std::vector<Part::Geometry *>::iterator it = geomlist.begin(); it != geomlist.end(); ++it)
        if (*it) delete *it;

    Shape.setValue(sketch.toShape());

    return App::DocumentObject::StdReturn;
}

int SketchObject::hasConflicts(void) const
{
    // set up a sketch (including dofs counting and diagnosing of conflicts)
    Sketch sketch;
    int dofs = sketch.setUpSketch(Geometry.getValues(), Constraints.getValues());
    if (dofs < 0) // over-constrained sketch
        return -2;
    if (sketch.hasConflicts()) // conflicting constraints
        return -1;

    return 0;
}

int SketchObject::setDatum(int ConstrId, double Datum)
{
    // set the changed value for the constraint
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= (int)vals.size())
        return -1;
    ConstraintType type = vals[ConstrId]->Type;
    if (type != Distance &&
        type != DistanceX &&
        type != DistanceY &&
        type != Radius &&
        type != Angle)
        return -1;

    if ((type == Distance || type == Radius) && Datum <= 0)
        return (Datum == 0) ? -5 : -4;

    // copy the list
    std::vector<Constraint *> newVals(vals);
    // clone the changed Constraint
    Constraint *constNew = vals[ConstrId]->clone();
    constNew->Value = Datum;
    newVals[ConstrId] = constNew;
    this->Constraints.setValues(newVals);
    delete constNew;

    // set up a sketch (including dofs counting and diagnosing of conflicts)
    Sketch sketch;
    int dofs = sketch.setUpSketch(Geometry.getValues(), Constraints.getValues());
    int err=0;
    if (dofs < 0) // over-constrained sketch
        err = -3;
    else if (sketch.hasConflicts()) // conflicting constraints
        err = -3;
    else if (sketch.solve() != 0) // solving
        err = -2;

    if (err == 0) {
        // set the newly solved geometry
        std::vector<Part::Geometry *> geomlist = sketch.getGeometry();
        Geometry.setValues(geomlist);
        for (std::vector<Part::Geometry *>::iterator it = geomlist.begin(); it != geomlist.end(); ++it)
            if (*it) delete *it;
    }
    else
        this->Constraints.setValues(vals);

    return err;
}

int SketchObject::movePoint(int geoIndex, PointPos PosId, const Base::Vector3d& toPoint, bool relative)
{
    Sketch sketch;
    int dofs = sketch.setUpSketch(Geometry.getValues(), Constraints.getValues());
    if (dofs < 0) // over-constrained sketch
        return -1;
    if (sketch.hasConflicts()) // conflicting constraints
        return -1;

    // move the point and solve
    int ret = sketch.movePoint(geoIndex, PosId, toPoint, relative);
    if (ret == 0) {
        std::vector<Part::Geometry *> geomlist = sketch.getGeometry();
        Geometry.setValues(geomlist);
        for (std::vector<Part::Geometry *>::iterator it = geomlist.begin(); it != geomlist.end(); ++it) {
            if (*it) delete *it;
        }
    }

    return ret;
}

Base::Vector3d SketchObject::getPoint(int geoIndex, PointPos PosId)
{
    const std::vector< Part::Geometry * > &geomlist = this->Geometry.getValues();
    assert(geoIndex < (int)geomlist.size());
    Part::Geometry *geo = geomlist[geoIndex];
    if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geo);
        if (PosId == start)
            return lineSeg->getStartPoint();
        else if (PosId == end)
            return lineSeg->getEndPoint();
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle*>(geo);
        if (PosId == mid)
            return circle->getCenter();
    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = dynamic_cast<const Part::GeomArcOfCircle*>(geo);
        if (PosId == start)
            return aoc->getStartPoint();
        else if (PosId == end)
            return aoc->getEndPoint();
        else if (PosId == mid)
            return aoc->getCenter();
    }

    return Base::Vector3d();
}

int SketchObject::addGeometry(const std::vector<Part::Geometry *> &geoList)
{
    return -1;
}

int SketchObject::addGeometry(const Part::Geometry *geo)
{
    const std::vector< Part::Geometry * > &vals = Geometry.getValues();

    std::vector< Part::Geometry * > newVals(vals);
    Part::Geometry *geoNew = geo->clone();
    newVals.push_back(geoNew);
    Geometry.setValues(newVals);
    Constraints.acceptGeometry(Geometry.getValues());
    delete geoNew;
    rebuildVertexIndex();
    return Geometry.getSize()-1;
}

int SketchObject::delGeometry(int GeoNbr)
{
    const std::vector< Part::Geometry * > &vals = this->Geometry.getValues();
    if (GeoNbr < 0 || GeoNbr >= (int)vals.size())
        return -1;

    std::vector< Part::Geometry * > newVals(vals);
    newVals.erase(newVals.begin()+GeoNbr);

    const std::vector< Constraint * > &constraints = this->Constraints.getValues();
    std::vector< Constraint * > newConstraints(0);
    for (std::vector<Constraint *>::const_iterator it = constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->First != GeoNbr && (*it)->Second != GeoNbr) {
            Constraint *copiedConstr = (*it)->clone();
            if (copiedConstr->First > GeoNbr)
                copiedConstr->First -= 1;
            if (copiedConstr->Second > GeoNbr)
                copiedConstr->Second -= 1;
            newConstraints.push_back(copiedConstr);
        }
    }

    this->Geometry.setValues(newVals);
    this->Constraints.setValues(newConstraints);
    this->Constraints.acceptGeometry(this->Geometry.getValues());
    rebuildVertexIndex();
    return 0;
}

int SketchObject::toggleConstruction(int GeoNbr)
{
    const std::vector< Part::Geometry * > &vals = this->Geometry.getValues();
    if (GeoNbr < 0 || GeoNbr >= (int)vals.size())
        return -1;

    std::vector< Part::Geometry * > newVals(vals);

    Part::Geometry *geoNew = newVals[GeoNbr]->clone();
    geoNew->Construction = !geoNew->Construction;
    newVals[GeoNbr]=geoNew;

    this->Geometry.setValues(newVals);
    this->Constraints.acceptGeometry(this->Geometry.getValues());
    return 0;
}

int SketchObject::addConstraints(const std::vector<Constraint *> &ConstraintList)
{
    return -1;
}

int SketchObject::addConstraint(const Constraint *constraint)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();

    std::vector< Constraint * > newVals(vals);
    Constraint *constNew = constraint->clone();
    newVals.push_back(constNew);
    this->Constraints.setValues(newVals);
    delete constNew;
    return this->Constraints.getSize()-1;
}

int SketchObject::delConstraint(int ConstrId)
{
    const std::vector< Constraint * > &vals = this->Constraints.getValues();
    if (ConstrId < 0 || ConstrId >= (int)vals.size())
        return -1;

    std::vector< Constraint * > newVals(vals);
    newVals.erase(newVals.begin()+ConstrId);
    this->Constraints.setValues(newVals);
    return 0;
}

int SketchObject::delConstraintOnPoint(int VertexId, bool onlyCoincident)
{
    int GeoId;
    PointPos PosId;
    getGeoVertexIndex(VertexId, GeoId, PosId);
    return delConstraintOnPoint(GeoId, PosId, onlyCoincident);
}

int SketchObject::delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();

    // check if constraints can be redirected to some other point
    int replaceGeoId=Constraint::GeoUndef;
    PointPos replacePosId=Sketcher::none;
    if (!onlyCoincident) {
        for (std::vector<Constraint *>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
            if ((*it)->Type == Sketcher::Coincident) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    replaceGeoId = (*it)->Second;
                    replacePosId = (*it)->SecondPos;
                    break;
                }
                else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                    replaceGeoId = (*it)->First;
                    replacePosId = (*it)->FirstPos;
                    break;
                }
            }
        }
    }

    // remove or redirect any constraints associated with the given point
    std::vector<Constraint *> newVals(0);
    for (std::vector<Constraint *>::const_iterator it = vals.begin(); it != vals.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                if (replaceGeoId != Constraint::GeoUndef &&
                    (replaceGeoId != (*it)->Second || replacePosId != (*it)->SecondPos)) { // redirect this constraint
                    (*it)->First = replaceGeoId;
                    (*it)->FirstPos = replacePosId;
                }
                else
                    continue; // skip this constraint
            }
            else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                if (replaceGeoId != Constraint::GeoUndef &&
                    (replaceGeoId != (*it)->First || replacePosId != (*it)->FirstPos)) { // redirect this constraint
                    (*it)->Second = replaceGeoId;
                    (*it)->SecondPos = replacePosId;
                }
                else
                    continue; // skip this constraint
            }
        }
        else if (!onlyCoincident) {
            if ((*it)->Type == Sketcher::Distance ||
                (*it)->Type == Sketcher::DistanceX || (*it)->Type == Sketcher::DistanceY) {
                if ((*it)->First == GeoId && (*it)->FirstPos == none &&
                    (PosId == start || PosId ==end)) {
                    // remove the constraint even if it is not directly associated
                    // with the given point
                    continue; // skip this constraint
                }
                else if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
                else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->Second = replaceGeoId;
                        (*it)->SecondPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::PointOnObject) {
                if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                    if (replaceGeoId != Constraint::GeoUndef) { // redirect this constraint
                        (*it)->First = replaceGeoId;
                        (*it)->FirstPos = replacePosId;
                    }
                    else
                        continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Tangent) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId) ||
                    ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    // we could keep the tangency constraint by converting it
                    // to a simple one but it is not really worth
                    continue; // skip this constraint
                }
            }
            else if ((*it)->Type == Sketcher::Symmetric) {
                if (((*it)->First == GeoId && (*it)->FirstPos == PosId) ||
                    ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) {
                    continue; // skip this constraint
                }
            }
        }
        newVals.push_back(*it);
    }
    if (newVals.size() < vals.size()) {
        this->Constraints.setValues(newVals);
        return 0;
    }

    return -1; // no such constraint
}

int SketchObject::transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId)
{
    const std::vector<Constraint *> &vals = this->Constraints.getValues();
    std::vector<Constraint *> newVals(vals);
    for (int i=0; i < int(newVals.size()); i++) {
        if (vals[i]->First == fromGeoId && vals[i]->FirstPos == fromPosId) {
            Constraint *constNew = newVals[i]->clone();
            constNew->First = toGeoId;
            constNew->FirstPos = toPosId;
            newVals[i] = constNew;
        } else if (vals[i]->Second == fromGeoId && vals[i]->SecondPos == fromPosId) {
            Constraint *constNew = newVals[i]->clone();
            constNew->Second = toGeoId;
            constNew->SecondPos = toPosId;
            newVals[i] = constNew;
        }
    }
    this->Constraints.setValues(newVals);
    return 0;
}

int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim)
{
    const std::vector<Part::Geometry *> &geomlist = this->Geometry.getValues();
    assert(GeoId < int(geomlist.size()));
    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    // only coincident points between two edges can be filleted
    if (GeoIdList.size() == 2) {
        Part::Geometry *geo1 = geomlist[GeoIdList[0]];
        Part::Geometry *geo2 = geomlist[GeoIdList[1]];
        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
            geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment*>(geo1);
            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector3d midPnt1 = (lineSeg1->getStartPoint() + lineSeg1->getEndPoint()) / 2 ;
            Base::Vector3d midPnt2 = (lineSeg2->getStartPoint() + lineSeg2->getEndPoint()) / 2 ;
            return fillet(GeoIdList[0], GeoIdList[1], midPnt1, midPnt2, radius, trim);
        }
    }

    return -1;
}

int SketchObject::fillet(int GeoId1, int GeoId2,
                         const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2,
                         double radius, bool trim)
{
    const std::vector<Part::Geometry *> &geomlist = this->Geometry.getValues();
    assert(GeoId1 < int(geomlist.size()));
    assert(GeoId2 < int(geomlist.size()));
    Part::Geometry *geo1 = geomlist[GeoId1];
    Part::Geometry *geo2 = geomlist[GeoId2];
    if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
        geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
        const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment*>(geo1);
        const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment*>(geo2);

        Base::Vector3d filletCenter;
        if (!Part::findFilletCenter(lineSeg1, lineSeg2, radius, refPnt1, refPnt2, filletCenter))
            return -1;
        Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
        Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();

        // the intersection point will and two distances will be necessary later for trimming the lines
        Base::Vector3d intersection, dist1, dist2;

        // create arc from known parameters and lines
        int filletId;
        Part::GeomArcOfCircle *arc = Part::createFilletGeometry(lineSeg1, lineSeg2, filletCenter, radius);
        if (arc) {
            // calculate intersection and distances before we invalidate lineSeg1 and lineSeg2
            if (!find2DLinesIntersection(lineSeg1, lineSeg2, intersection)) {
                delete arc;
                return -1;
            }
            dist1.ProjToLine(arc->getStartPoint()-intersection, dir1);
            dist2.ProjToLine(arc->getStartPoint()-intersection, dir2);
            Part::Geometry *newgeo = dynamic_cast<Part::Geometry* >(arc);
            filletId = addGeometry(newgeo);
            if (filletId < 0) {
                delete arc;
                return -1;
            }
        }
        else
            return -1;

        if (trim) {
            PointPos PosId1 = (filletCenter-intersection)*dir1 > 0 ? start : end;
            PointPos PosId2 = (filletCenter-intersection)*dir2 > 0 ? start : end;

            delConstraintOnPoint(GeoId1, PosId1, false);
            delConstraintOnPoint(GeoId2, PosId2, false);
            Sketcher::Constraint *tangent1 = new Sketcher::Constraint();
            Sketcher::Constraint *tangent2 = new Sketcher::Constraint();

            tangent1->Type = Sketcher::Tangent;
            tangent1->First = GeoId1;
            tangent1->FirstPos = PosId1;
            tangent1->Second = filletId;

            tangent2->Type = Sketcher::Tangent;
            tangent2->First = GeoId2;
            tangent2->FirstPos = PosId2;
            tangent2->Second = filletId;

            if (dist1.Length() < dist2.Length()) {
                tangent1->SecondPos = start;
                tangent2->SecondPos = end;
                movePoint(GeoId1, PosId1, arc->getStartPoint());
                movePoint(GeoId2, PosId2, arc->getEndPoint());
            }
            else {
                tangent1->SecondPos = end;
                tangent2->SecondPos = start;
                movePoint(GeoId1, PosId1, arc->getEndPoint());
                movePoint(GeoId2, PosId2, arc->getStartPoint());
            }

            addConstraint(tangent1);
            addConstraint(tangent2);
            delete tangent1;
            delete tangent2;
        }
        delete arc;
        return 0;
    }
    return -1;
}

int SketchObject::trim(int GeoId, const Base::Vector3d& point)
{
    const std::vector<Part::Geometry *> &geomlist = this->Geometry.getValues();
    const std::vector<Constraint *> &constraints = this->Constraints.getValues();

    assert(GeoId < int(geomlist.size()));

    int GeoId1=Constraint::GeoUndef, GeoId2=Constraint::GeoUndef;
    Base::Vector3d point1, point2;
    Part2DObject::seekTrimPoints(geomlist, GeoId, point, GeoId1, point1, GeoId2, point2);
    if (GeoId1 < 0 && GeoId2 >= 0) {
        std::swap(GeoId1,GeoId2);
        std::swap(point1,point2);
    }

    Part::Geometry *geo = geomlist[GeoId];
    if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geo);
        Base::Vector3d startPnt = lineSeg->getStartPoint();
        Base::Vector3d endPnt = lineSeg->getEndPoint();
        Base::Vector3d dir = (endPnt - startPnt).Normalize();
        double length = (endPnt - startPnt)*dir;
        double x0 = (point - startPnt)*dir;
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double x1 = (point1 - startPnt)*dir;
            double x2 = (point2 - startPnt)*dir;
            if (x1 > x2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(x1,x2);
            }
            if (x1 >= 0.001*length && x2 <= 0.999*length) {
                if (x1 < x0 && x2 > x0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    movePoint(GeoId, end, point1);
                    movePoint(newGeoId, start, point2);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;
                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none && (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                        newConstr->SecondPos = secondPos1;
                        delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset the second pos
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    // Reset the second pos
                    newConstr->SecondPos = Sketcher::none;

                    // new line segments colinear
                    newConstr->Type = Sketcher::Tangent;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = none;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);

                    delete newConstr;
                    return 0;
                }
            } else if (x1 < 0.001*length) { // drop the first intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (x2 > 0.999*length) { // drop the second intersection point
            }
            else
              return -1;
        }

        if (GeoId1 >= 0) {
            double x1 = (point1 - startPnt)*dir;
            if (x1 >= 0.001*length && x1 <= 0.999*length) {

                ConstraintType constrType = Sketcher::PointOnObject;
                PointPos secondPos = Sketcher::none;
                for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                     it != constraints.end(); ++it) {
                    Constraint *constr = *(it);
                    if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                        constrType = Sketcher::Coincident;
                        secondPos = constr->FirstPos;
                        delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                        break;
                    }
                }

                if (x1 > x0) { // trim line start
                    delConstraintOnPoint(GeoId, start, false);
                    movePoint(GeoId, start, point1);

                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
                else if (x1 < x0) { // trim line end
                    delConstraintOnPoint(GeoId, end, false);
                    movePoint(GeoId, end, point1);
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
            }
        }
    } else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
        const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle*>(geo);
        Base::Vector3d center = circle->getCenter();
        double theta0 = Base::fmod(atan2(point.y - center.y,point.x - center.x), 2.f*M_PI);
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x), 2.f*M_PI);
            double theta2 = Base::fmod(atan2(point2.y - center.y, point2.x - center.x), 2.f*M_PI);
            if (Base::fmod(theta1 - theta0, 2.f*M_PI) > Base::fmod(theta2 - theta0, 2.f*M_PI)) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 == theta0 || theta1 == theta2)
                return -1;
            else if (theta1 > theta2)
                theta2 += 2.f*M_PI;

            // Trim Point between intersection points

            // Create a new arc to substitute Circle in geometry list and set parameters
            Part::GeomArcOfCircle *geoNew = new Part::GeomArcOfCircle();
            geoNew->setCenter(center);
            geoNew->setRadius(circle->getRadius());
            geoNew->setRange(theta1, theta2);

            std::vector< Part::Geometry * > newVals(geomlist);
            newVals[GeoId] = geoNew;
            Geometry.setValues(newVals);
            Constraints.acceptGeometry(Geometry.getValues());
            delete geoNew;
            rebuildVertexIndex();

            PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
            ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if (secondPos1 == Sketcher::none && (constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType1= Sketcher::Coincident;
                    secondPos1 = constr->FirstPos;
                } else if(secondPos2 == Sketcher::none && (constr->First == GeoId2  && constr->Second == GeoId)) {
                    constrType2 = Sketcher::Coincident;
                    secondPos2 = constr->FirstPos;
                }
            }

            // constrain the trimming points on the corresponding geometries
            Sketcher::Constraint *newConstr = new Sketcher::Constraint();
            newConstr->Type = constrType1;
            newConstr->First = GeoId;
            newConstr->FirstPos = start;
            newConstr->Second = GeoId1;

            if (constrType1 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos1;
                delConstraintOnPoint(GeoId1, secondPos1, false);
            }

            addConstraint(newConstr);

            // Reset secondpos in case it was set previously
            newConstr->SecondPos = Sketcher::none;

            // Add Second Constraint
            newConstr->First = GeoId;
            newConstr->FirstPos = end;
            newConstr->Second = GeoId2;

            if (constrType2 == Sketcher::Coincident) {
                newConstr->SecondPos = secondPos2;
                delConstraintOnPoint(GeoId2, secondPos2, false);
            }

            addConstraint(newConstr);

            delete newConstr;

            return 0;
        }

    } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        const Part::GeomArcOfCircle *aoc = dynamic_cast<const Part::GeomArcOfCircle*>(geo);
        Base::Vector3d center = aoc->getCenter();
        double startAngle, endAngle;
        aoc->getRange(startAngle, endAngle);
        double dir = (startAngle < endAngle) ? 1 : -1; // this is always == 1
        double arcLength = (endAngle - startAngle)*dir;
        double theta0 = Base::fmod(atan2(point.y - center.y, point.x - center.x) - startAngle, 2.f*M_PI); // x0
        if (GeoId1 >= 0 && GeoId2 >= 0) {
            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x) - startAngle, 2.f*M_PI) * dir; // x1
            double theta2 = Base::fmod(atan2(point2.y - center.y, point2.x - center.x) - startAngle, 2.f*M_PI) * dir; // x2
            if (theta1 > theta2) {
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
                std::swap(theta1,theta2);
            }
            if (theta1 >= 0.001*arcLength && theta2 <= 0.999*arcLength) {
                // Trim Point between intersection points
                if (theta1 < theta0 && theta2 > theta0) {
                    int newGeoId = addGeometry(geo);
                    // go through all constraints and replace the point (GeoId,end) with (newGeoId,end)
                    transferConstraints(GeoId, end, newGeoId, end);

                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    Part::GeomArcOfCircle *aoc2 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[newGeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1);
                    aoc2->setRange(startAngle + theta2, endAngle);

                    // constrain the trimming points on the corresponding geometries
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();

                    // Build Constraints associated with new pair of arcs
                    newConstr->Type = Sketcher::Equal;
                    newConstr->First = GeoId;
                    newConstr->Second = newGeoId;
                    addConstraint(newConstr);

                    PointPos secondPos1 = Sketcher::none, secondPos2 = Sketcher::none;
                    ConstraintType constrType1 = Sketcher::PointOnObject, constrType2 = Sketcher::PointOnObject;

                    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                         it != constraints.end(); ++it) {
                        Constraint *constr = *(it);
                        if (secondPos1 == Sketcher::none &&
                            (constr->First == GeoId1  && constr->Second == GeoId)) {
                            constrType1= Sketcher::Coincident;
                            secondPos1 = constr->FirstPos;
                        } else if (secondPos2 == Sketcher::none &&
                                   (constr->First == GeoId2  && constr->Second == GeoId)) {
                            constrType2 = Sketcher::Coincident;
                            secondPos2 = constr->FirstPos;
                        }
                    }

                    newConstr->Type = constrType1;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType1 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos1;
                      delConstraintOnPoint(GeoId1, secondPos1, false);
                    }

                    addConstraint(newConstr);

                    // Reset secondpos in case it was set previously
                    newConstr->SecondPos = Sketcher::none;

                    newConstr->Type = constrType2;
                    newConstr->First = newGeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId2;

                    if (constrType2 == Sketcher::Coincident) {
                      newConstr->SecondPos = secondPos2;
                      delConstraintOnPoint(GeoId2, secondPos2, false);
                    }

                    addConstraint(newConstr);

                    newConstr->Type = Sketcher::Coincident;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = Sketcher::mid;
                    newConstr->Second = newGeoId;
                    newConstr->SecondPos = Sketcher::mid;
                    addConstraint(newConstr);

                    delete newConstr;

                    return 0;
                } else
                    return -1;
            } else if (theta1 < 0.001*arcLength) { // drop the second intersection point
                std::swap(GeoId1,GeoId2);
                std::swap(point1,point2);
            } else if (theta2 > 0.999*arcLength) {
            }
            else
                return -1;
        }

        if (GeoId1 >= 0) {

            ConstraintType constrType = Sketcher::PointOnObject;
            PointPos secondPos = Sketcher::none;
            for (std::vector<Constraint *>::const_iterator it=constraints.begin();
                 it != constraints.end(); ++it) {
                Constraint *constr = *(it);
                if ((constr->First == GeoId1  && constr->Second == GeoId)) {
                    constrType = Sketcher::Coincident;
                    secondPos = constr->FirstPos;
                    delConstraintOnPoint(GeoId1, constr->FirstPos, false);
                    break;
                }
            }

            double theta1 = Base::fmod(atan2(point1.y - center.y, point1.x - center.x) - startAngle, 2.f*M_PI) * dir; // x1
            if (theta1 >= 0.001*arcLength && theta1 <= 0.999*arcLength) {
                if (theta1 > theta0) { // trim arc start
                    delConstraintOnPoint(GeoId, start, false);
                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle + theta1, endAngle);
                    // constrain the trimming point on the corresponding geometry
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = start;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
                else { // trim arc end
                    delConstraintOnPoint(GeoId, end, false);
                    Part::GeomArcOfCircle *aoc1 = dynamic_cast<Part::GeomArcOfCircle*>(geomlist[GeoId]);
                    aoc1->setRange(startAngle, startAngle + theta1);
                    Sketcher::Constraint *newConstr = new Sketcher::Constraint();
                    newConstr->Type = constrType;
                    newConstr->First = GeoId;
                    newConstr->FirstPos = end;
                    newConstr->Second = GeoId1;

                    if (constrType == Sketcher::Coincident)
                        newConstr->SecondPos = secondPos;

                    addConstraint(newConstr);
                    delete newConstr;
                    return 0;
                }
            }
        }
    }

    return -1;
}

int SketchObject::addExternal(App::DocumentObject *Obj, const char* SubName)
{
    // so far only externals to the support of the sketch
    assert(Support.getValue() == Obj);

    // get the actual lists of the externals
    std::vector<DocumentObject*> Objects     = ExternalConstraints.getValues();
    std::vector<std::string>     SubElements = ExternalConstraints.getSubValues();

    // add the new ones
    Objects.push_back(Obj);
    SubElements.push_back(std::string(SubName));

    // set the Link list.
    ExternalConstraints.setValues(Objects,SubElements);

    return ExternalConstraints.getValues().size()-1;
}

int SketchObject::delExternal(int ConstrId)
{
    // FIXME: still to implement
    return 0;

}

std::vector<Part::Geometry *> getExternalGeometry(void)
{
    std::vector<Part::Geometry *> ExtGeos;

    // add the root point (0,0) the the external geos(-1)
    ExtGeos.push_back(new Part::GeomPoint(Base::Vector3d(0,0,0)));

    // add the X,Y (V,H) axis (-2,-3)
    ExtGeos.push_back(new Part::GeomLine(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0)));
    ExtGeos.push_back(new Part::GeomLine(Base::Vector3d(0,0,0),Base::Vector3d(0,1,0)));

    // return the result set
    return ExtGeos;
}


void SketchObject::rebuildVertexIndex(void)
{
    VertexId2GeoId.resize(0);
    VertexId2PosId.resize(0);
    int i=0;
    const std::vector< Part::Geometry * > &geometry = this->Geometry.getValues();
    for (std::vector< Part::Geometry * >::const_iterator it = geometry.begin();
         it != geometry.end(); ++it,i++) {
        if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
        } else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(mid);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(end);
        }
    }
}

void SketchObject::getCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
                                       std::vector<PointPos> &PosIdList)
{
    const std::vector<Constraint *> &constraints = this->Constraints.getValues();

    GeoIdList.clear();
    PosIdList.clear();
    GeoIdList.push_back(GeoId);
    PosIdList.push_back(PosId);
    for (std::vector<Constraint *>::const_iterator it=constraints.begin();
         it != constraints.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            if ((*it)->First == GeoId && (*it)->FirstPos == PosId) {
                GeoIdList.push_back((*it)->Second);
                PosIdList.push_back((*it)->SecondPos);
            }
            else if ((*it)->Second == GeoId && (*it)->SecondPos == PosId) {
                GeoIdList.push_back((*it)->First);
                PosIdList.push_back((*it)->FirstPos);
            }
        }
    }
    if (GeoIdList.size() == 1) {
        GeoIdList.clear();
        PosIdList.clear();
    }
}

void SketchObject::getCoincidentPoints(int VertexId, std::vector<int> &GeoIdList,
                                       std::vector<PointPos> &PosIdList)
{
    int GeoId;
    PointPos PosId;
    getGeoVertexIndex(VertexId, GeoId, PosId);
    getCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
}

void SketchObject::appendConflictMsg(const std::vector<int> &conflicting, std::string &msg)
{
    std::stringstream ss;
    if (msg.length() > 0)
        ss << msg;
    if (conflicting.size() > 0) {
        ss << "Please remove at least one of the constraints (" << conflicting[0];
        for (unsigned int i=1; i < conflicting.size(); i++)
            ss << ", " << conflicting[i];
        ss << ")\n";
    }
    msg = ss.str();
}

PyObject *SketchObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new SketchObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int SketchObject::getMemSize(void) const
{
    return 0;
}

void SketchObject::Save(Writer &writer) const
{
    // save the father classes
    Part::Part2DObject::Save(writer);
}

void SketchObject::Restore(XMLReader &reader)
{
    // read the father classes
    Part::Part2DObject::Restore(reader);
    Constraints.acceptGeometry(Geometry.getValues());
    rebuildVertexIndex();
}

void SketchObject::onChanged(const App::Property* prop)
{
    if (prop == &Geometry || prop == &Constraints)
        Constraints.checkGeometry(Geometry.getValues());
    Part::Part2DObject::onChanged(prop);
}

void SketchObject::getGeoVertexIndex(int VertexId, int &GeoId, PointPos &PosId)
{
    if (VertexId < 0 || VertexId >= (int)VertexId2GeoId.size()) {
        GeoId = Constraint::GeoUndef;
        PosId = none;
        return;
    }
    GeoId = VertexId2GeoId[VertexId];
    PosId = VertexId2PosId[VertexId];
}

// Python Sketcher feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher::SketchObjectPython, Sketcher::SketchObject)
template<> const char* Sketcher::SketchObjectPython::getViewProviderName(void) const {
    return "SketcherGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class SketcherExport FeaturePythonT<Sketcher::SketchObject>;
}

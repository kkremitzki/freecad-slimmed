// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.6 (2006/11/05)

#ifndef WM4FOUNDATION_H
#define WM4FOUNDATION_H

// approximation
#include "Wm4ApprCircleFit2.h"
#include "Wm4ApprCylinderFit3.h"
#include "Wm4ApprEllipseFit2.h"
#include "Wm4ApprEllipsoidFit3.h"
#include "Wm4ApprGaussPointsFit2.h"
#include "Wm4ApprGaussPointsFit3.h"
#include "Wm4ApprLineFit2.h"
#include "Wm4ApprLineFit3.h"
#include "Wm4ApprParaboloidFit3.h"
#include "Wm4ApprPlaneFit3.h"
#include "Wm4ApprPolyFit2.h"
#include "Wm4ApprPolyFit3.h"
#include "Wm4ApprPolyFit4.h"
#include "Wm4ApprQuadraticFit2.h"
#include "Wm4ApprQuadraticFit3.h"
#include "Wm4ApprSphereFit3.h"

// computational geometry:  convex hull
#include "Wm4ConvexHull.h"
#include "Wm4ConvexHull1.h"
#include "Wm4ConvexHull2.h"
#include "Wm4ConvexHull3.h"
#include "Wm4HullEdge2.h"
#include "Wm4HullTriangle3.h"

// computational geometry:  Delaunay
#include "Wm4Delaunay.h"
#include "Wm4Delaunay1.h"
#include "Wm4Delaunay2.h"
#include "Wm4Delaunay3.h"
#include "Wm4DelPolygonEdge.h"
#include "Wm4DelPolyhedronFace.h"
#include "Wm4DelTetrahedron.h"
#include "Wm4DelTriangle.h"

// computational geometry:  query
#include "Wm4Query.h"
#include "Wm4Query2.h"
#include "Wm4Query2Filtered.h"
#include "Wm4Query2Int64.h"
#include "Wm4Query2TInteger.h"
#include "Wm4Query2TRational.h"
#include "Wm4Query3.h"
#include "Wm4Query3Filtered.h"
#include "Wm4Query3Int64.h"
#include "Wm4Query3TInteger.h"
#include "Wm4Query3TRational.h"

// computational geometry:  rational arithmetic
#include "Wm4IVector2.h"
#include "Wm4IVector3.h"
#include "Wm4RVector2.h"
#include "Wm4RVector3.h"
#include "Wm4TInteger.h"
#include "Wm4TIVector.h"
#include "Wm4TRational.h"
#include "Wm4TRVector.h"

// computational geometry:  triangulate
#include "Wm4TriangulateEC.h"

// containment
#include "Wm4ContBox2.h"
#include "Wm4ContBox3.h"
#include "Wm4ContCapsule3.h"
#include "Wm4ContCircle2.h"
#include "Wm4ContCylinder3.h"
#include "Wm4ContEllipse2.h"
#include "Wm4ContEllipsoid3.h"
#include "Wm4ContLozenge3.h"
#include "Wm4ContPointInPolygon2.h"
#include "Wm4ContScribeCircle2.h"
#include "Wm4ContScribeCircle3Sphere3.h"
#include "Wm4ContSeparatePoints2.h"
#include "Wm4ContSeparatePoints3.h"
#include "Wm4ContSphere3.h"

// curves
#include "Wm4BezierCurve2.h"
#include "Wm4BezierCurve3.h"
#include "Wm4BSplineBasis.h"
#include "Wm4BSplineCurve2.h"
#include "Wm4BSplineCurve3.h"
#include "Wm4BSplineFit.h"
#include "Wm4BSplineFitBasis.h"
#include "Wm4BSplineGeodesic.h"
#include "Wm4BSplineReduction.h"
#include "Wm4CubicPolynomialCurve2.h"
#include "Wm4CubicPolynomialCurve3.h"
#include "Wm4Curve2.h"
#include "Wm4Curve3.h"
#include "Wm4EllipsoidGeodesic.h"
#include "Wm4MultipleCurve2.h"
#include "Wm4MultipleCurve3.h"
#include "Wm4NaturalSpline2.h"
#include "Wm4NaturalSpline3.h"
#include "Wm4NURBSCurve2.h"
#include "Wm4NURBSCurve3.h"
#include "Wm4PolynomialCurve2.h"
#include "Wm4PolynomialCurve3.h"
#include "Wm4RiemannianGeodesic.h"
#include "Wm4SingleCurve2.h"
#include "Wm4SingleCurve3.h"
#include "Wm4TCBSpline2.h"
#include "Wm4TCBSpline3.h"

// distance
#include "Wm4Distance.h"
#include "Wm4DistCircle3Circle3.h"
#include "Wm4DistLine2Line2.h"
#include "Wm4DistLine2Ray2.h"
#include "Wm4DistLine2Segment2.h"
#include "Wm4DistLine3Box3.h"
#include "Wm4DistLine3Circle3.h"
#include "Wm4DistLine3Line3.h"
#include "Wm4DistLine3Ray3.h"
#include "Wm4DistLine3Rectangle3.h"
#include "Wm4DistLine3Segment3.h"
#include "Wm4DistLine3Triangle3.h"
#include "Wm4DistRay2Ray2.h"
#include "Wm4DistRay2Segment2.h"
#include "Wm4DistRay3Box3.h"
#include "Wm4DistRay3Ray3.h"
#include "Wm4DistRay3Rectangle3.h"
#include "Wm4DistRay3Segment3.h"
#include "Wm4DistRay3Triangle3.h"
#include "Wm4DistRectangle3Rectangle3.h"
#include "Wm4DistSegment2Segment2.h"
#include "Wm4DistSegment3Box3.h"
#include "Wm4DistSegment3Rectangle3.h"
#include "Wm4DistSegment3Segment3.h"
#include "Wm4DistSegment3Triangle3.h"
#include "Wm4DistTriangle3Rectangle3.h"
#include "Wm4DistTriangle3Triangle3.h"
#include "Wm4DistVector2Box2.h"
#include "Wm4DistVector2Ellipse2.h"
#include "Wm4DistVector2Line2.h"
#include "Wm4DistVector2Quadratic2.h"
#include "Wm4DistVector2Ray2.h"
#include "Wm4DistVector2Segment2.h"
#include "Wm4DistVector3Box3.h"
#include "Wm4DistVector3Circle3.h"
#include "Wm4DistVector3Ellipsoid3.h"
#include "Wm4DistVector3Frustum3.h"
#include "Wm4DistVector3Line3.h"
#include "Wm4DistVector3Plane3.h"
#include "Wm4DistVector3Quadratic3.h"
#include "Wm4DistVector3Ray3.h"
#include "Wm4DistVector3Rectangle3.h"
#include "Wm4DistVector3Segment3.h"
#include "Wm4DistVector3Tetrahedron3.h"
#include "Wm4DistVector3Triangle3.h"

// interpolation
#include "Wm4IntpAkima1.h"
#include "Wm4IntpAkimaNonuniform1.h"
#include "Wm4IntpAkimaUniform1.h"
#include "Wm4IntpAkimaUniform2.h"
#include "Wm4IntpAkimaUniform3.h"
#include "Wm4IntpBicubic2.h"
#include "Wm4IntpBilinear2.h"
#include "Wm4IntpBSplineUniform.h"
#include "Wm4IntpBSplineUniform1.h"
#include "Wm4IntpBSplineUniform2.h"
#include "Wm4IntpBSplineUniform3.h"
#include "Wm4IntpBSplineUniform4.h"
#include "Wm4IntpBSplineUniformN.h"
#include "Wm4IntpLinearNonuniform2.h"
#include "Wm4IntpLinearNonuniform3.h"
#include "Wm4IntpQdrNonuniform2.h"
#include "Wm4IntpSphere2.h"
#include "Wm4IntpThinPlateSpline2.h"
#include "Wm4IntpThinPlateSpline3.h"
#include "Wm4IntpTricubic3.h"
#include "Wm4IntpTrilinear3.h"
#include "Wm4IntpVectorField2.h"

// intersection
#include "Wm4Intersector.h"
#include "Wm4Intersector1.h"
#include "Wm4IntrArc2Arc2.h"
#include "Wm4IntrArc2Circle2.h"
#include "Wm4IntrBox2Box2.h"
#include "Wm4IntrBox2Circle2.h"
#include "Wm4IntrBox3Box3.h"
#include "Wm4IntrBox3Frustum3.h"
#include "Wm4IntrBox3Sphere3.h"
#include "Wm4IntrCapsule3Capsule3.h"
#include "Wm4IntrCircle2Circle2.h"
#include "Wm4IntrCircle3Plane3.h"
#include "Wm4IntrEllipse2Ellipse2.h"
#include "Wm4IntrLinComp2LinComp2.h"
#include "Wm4IntrLinComp2Triangle2.h"
#include "Wm4IntrLine2Arc2.h"
#include "Wm4IntrLine2Box2.h"
#include "Wm4IntrLine2Circle2.h"
#include "Wm4IntrLine3Box3.h"
#include "Wm4IntrLine3Capsule3.h"
#include "Wm4IntrLine3Cone3.h"
#include "Wm4IntrLine3Cylinder3.h"
#include "Wm4IntrLine3Ellipsoid3.h"
#include "Wm4IntrLine3Lozenge3.h"
#include "Wm4IntrLine3Plane3.h"
#include "Wm4IntrLine3Sphere3.h"
#include "Wm4IntrLine3Torus3.h"
#include "Wm4IntrLine3Triangle3.h"
#include "Wm4IntrLozenge3Lozenge3.h"
#include "Wm4IntrPlane3Box3.h"
#include "Wm4IntrPlane3Capsule3.h"
#include "Wm4IntrPlane3Cylinder3.h"
#include "Wm4IntrPlane3Ellipsoid3.h"
#include "Wm4IntrPlane3Lozenge3.h"
#include "Wm4IntrPlane3Plane3.h"
#include "Wm4IntrPlane3Sphere3.h"
#include "Wm4IntrPlane3Triangle3.h"
#include "Wm4IntrRay2Arc2.h"
#include "Wm4IntrRay2Box2.h"
#include "Wm4IntrRay2Circle2.h"
#include "Wm4IntrRay3Box3.h"
#include "Wm4IntrRay3Capsule3.h"
#include "Wm4IntrRay3Cylinder3.h"
#include "Wm4IntrRay3Ellipsoid3.h"
#include "Wm4IntrRay3Lozenge3.h"
#include "Wm4IntrRay3Plane3.h"
#include "Wm4IntrRay3Sphere3.h"
#include "Wm4IntrRay3Triangle3.h"
#include "Wm4IntrSegment2Arc2.h"
#include "Wm4IntrSegment2Box2.h"
#include "Wm4IntrSegment2Circle2.h"
#include "Wm4IntrSegment3Box3.h"
#include "Wm4IntrSegment3Capsule3.h"
#include "Wm4IntrSegment3Cylinder3.h"
#include "Wm4IntrSegment3Ellipsoid3.h"
#include "Wm4IntrSegment3Lozenge3.h"
#include "Wm4IntrSegment3Plane3.h"
#include "Wm4IntrSegment3Sphere3.h"
#include "Wm4IntrSegment3Triangle3.h"
#include "Wm4IntrSphere3Cone3.h"
#include "Wm4IntrSphere3Frustum3.h"
#include "Wm4IntrSphere3Sphere3.h"
#include "Wm4IntrTetrahedron3Tetrahedron3.h"
#include "Wm4IntrTriangle2Triangle2.h"
#include "Wm4IntrTriangle3Cone3.h"
#include "Wm4IntrTriangle3Triangle3.h"

// math
#include "Wm4Arc2.h"
#include "Wm4AxisAlignedBox2.h"
#include "Wm4AxisAlignedBox3.h"
#include "Wm4BandedMatrix.h"
#include "Wm4BitHacks.h"
#include "Wm4Box2.h"
#include "Wm4Box3.h"
#include "Wm4BSplineVolume.h"
#include "Wm4Capsule3.h"
#include "Wm4Circle2.h"
#include "Wm4Circle3.h"
#include "Wm4ColorRGB.h"
#include "Wm4ColorRGBA.h"
#include "Wm4Cone3.h"
#include "Wm4ConvexPolyhedron3.h"
#include "Wm4Cylinder3.h"
#include "Wm4Ellipse2.h"
#include "Wm4Ellipsoid3.h"
#include "Wm4Frustum3.h"
#include "Wm4GMatrix.h"
#include "Wm4GVector.h"
#include "Wm4LinComp.h"
#include "Wm4LinComp2.h"
#include "Wm4LinComp3.h"
#include "Wm4Line2.h"
#include "Wm4Line3.h"
#include "Wm4Lozenge3.h"
#include "Wm4Mapper2.h"
#include "Wm4Mapper3.h"
#include "Wm4Math.h"
#include "Wm4Matrix2.h"
#include "Wm4Matrix3.h"
#include "Wm4Matrix4.h"
#include "Wm4Plane3.h"
#include "Wm4Polyhedron3.h"
#include "Wm4Polynomial1.h"
#include "Wm4Quadratic2.h"
#include "Wm4Quadratic3.h"
#include "Wm4Quaternion.h"
#include "Wm4Ray2.h"
#include "Wm4Ray3.h"
#include "Wm4Rectangle3.h"
#include "Wm4Segment2.h"
#include "Wm4Segment3.h"
#include "Wm4Sphere3.h"
#include "Wm4Tetrahedron3.h"
#include "Wm4Torus3.h"
#include "Wm4Triangle2.h"
#include "Wm4Triangle3.h"
#include "Wm4Vector2.h"
#include "Wm4Vector3.h"
#include "Wm4Vector4.h"

// meshes
#include "Wm4BasicMesh.h"
#include "Wm4ConformalMap.h"
#include "Wm4EdgeKey.h"
#include "Wm4ETManifoldMesh.h"
#include "Wm4ETNonmanifoldMesh.h"
#include "Wm4MeshCurvature.h"
#include "Wm4MeshSmoother.h"
#include "Wm4PlanarGraph.h"
#include "Wm4TriangleKey.h"
#include "Wm4UniqueVerticesTriangles.h"
#include "Wm4VEManifoldMesh.h"

// miscellaneous
#include "Wm4GridGraph2.h"
#include "Wm4NormalCompression.h"
#include "Wm4PerspProjEllipsoid.h"
#include "Wm4QuadToQuadTransforms.h"
#include "Wm4RandomHypersphere.h"
#include "Wm4TangentsToCircles.h"

// numerical analysis
#include "Wm4Bisect1.h"
#include "Wm4Bisect2.h"
#include "Wm4Bisect3.h"
#include "Wm4Eigen.h"
#include "Wm4Integrate1.h"
#include "Wm4LinearSystem.h"
#include "Wm4Minimize1.h"
#include "Wm4MinimizeN.h"
#include "Wm4NoniterativeEigen3x3.h"
#include "Wm4OdeEuler.h"
#include "Wm4OdeImplicitEuler.h"
#include "Wm4OdeMidpoint.h"
#include "Wm4OdeRungeKutta4.h"
#include "Wm4OdeSolver.h"
#include "Wm4PolynomialRoots.h"
#include "Wm4PolynomialRootsR.h"

// surfaces
#include "Wm4BSplineRectangle.h"
#include "Wm4ImplicitSurface.h"
#include "Wm4NURBSRectangle.h"
#include "Wm4ParametricSurface.h"
#include "Wm4QuadricSurface.h"
#include "Wm4Surface.h"

// system
#include "Wm4Command.h"
#include "Wm4Memory.h"
#include "Wm4Platforms.h"
#include "Wm4System.h"
#include "Wm4THashSet.h"
#include "Wm4THashTable.h"
#include "Wm4TMinHeap.h"
#include "Wm4TSmallUnorderedSet.h"
#include "Wm4TStringHashTable.h"
#include "Wm4TTuple.h"

#endif

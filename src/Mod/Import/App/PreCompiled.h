/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#include <FCConfig.h>
#ifdef _PreComp_

/// here get the warnings of to long specifieres disabled (needet for VC6)
#ifdef _MSC_VER
#	pragma warning( disable : 4251 )
#	pragma warning( disable : 4503 )
#	pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif


// Importing of App classes

#ifdef FC_OS_WIN32
# define AppPartExport __declspec(dllimport)
#else // for Linux
# define AppPartExport
#endif


// standard
#include <list>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <map>

// Xerces
#include <xercesc/util/XercesDefs.hpp>

// OpenCasCade =====================================================================================
// Base
#include <Standard_Failure.hxx>
#include <Standard_GUID.hxx>
#include <Standard_AbortiveTransaction.hxx>
#include <Standard_Address.hxx>
#include <Standard_AncestorIterator.hxx>
#include <Standard_BasicMap.hxx>
#include <Standard_BasicMapIterator.hxx>
#include <Standard_Boolean.hxx>
#include <Standard_Byte.hxx>
#include <Standard_Character.hxx>
#include <Standard_ConstructionError.hxx>
#include <Standard_Container.hxx>
#include <Standard_CString.hxx>
#include <Standard_ctype.hxx>
#include <Standard_DBHandle.hxx>
#include <Standard_DefineHandle.hxx>
#include <Standard_DimensionError.hxx>
#include <Standard_DimensionMismatch.hxx>
#include <Standard_DivideByZero.hxx>
#include <Standard_DomainError.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_ExtCharacter.hxx>
#include <Standard_ExtString.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ForMapOfTypes.hxx>
#include <Standard_GUID.hxx>
#include <Standard_ImmutableObject.hxx>
#include <Standard_Integer.hxx>
#include <Standard_InternalType.hxx>
#include <Standard_IStream.hxx>
#include <Standard_KindOfType.hxx>
#include <Standard_LicenseError.hxx>
#include <Standard_LicenseNotFound.hxx>
#include <Standard_Macro.hxx>
#include <Standard_MapOfTypes.hxx>
#include <Standard_math.hxx>
#include <Standard_MultiplyDefined.hxx>
#include <Standard_MyMapOfStringsHasher.hxx>
#include <Standard_MyMapOfTypes.hxx>
#include <Standard_NegativeValue.hxx>
#include <Standard_NoMoreObject.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_NullObject.hxx>
#include <Standard_NullValue.hxx>
#include <Standard_NumericError.hxx>
#include <Standard_OId.hxx>
#include <Standard_OStream.hxx>
#include <Standard_OutOfMemory.hxx>
#include <Standard_OutOfRange.hxx>
#include <Standard_Overflow.hxx>
#include <Standard_Persistent.hxx>
#include <Standard_Persistent_proto.hxx>
#include <Standard_PForMapOfTypes.hxx>
#include <Standard_PrimitiveTypes.hxx>
#include <Standard_ProgramError.hxx>
#include <Standard_RangeError.hxx>
#include <Standard_Real.hxx>
#include <Standard_ShortReal.hxx>
#include <Standard_SStream.hxx>
#include <Standard_Static.hxx>
#include <Standard_Storable.hxx>
#include <Standard_Stream.hxx>
#include <Standard_String.hxx>
#include <Standard_theForMapOfTypes.hxx>
#include <Standard_TooManyUsers.hxx>
#include <Standard_Transient.hxx>
#include <Standard_Transient_proto.hxx>
#include <Standard_Type.hxx>
#include <Standard_TypeDef.hxx>
#include <Standard_TypeMismatch.hxx>
#include <Standard_Underflow.hxx>
#include <Standard_UUID.hxx>
#include <Standard_WayOfLife.hxx>
#include <Quantity_Date.hxx>

 
#include <TCollection_ExtendedString.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColStd_SequenceOfExtendedString.hxx>
// OCAF
#include <TDF_Label.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Tool.hxx>
#include <TDF_ListIteratorOfAttributeList.hxx>
#include <TDocStd_Application.hxx>
#include <TDataStd_Real.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_RealArray.hxx>
#include <TDataStd_IntegerArray.hxx>
#include <TDataStd_Comment.hxx>

#include <BRepBuilderAPI.hxx>

#include <BRepTools.hxx>
#include <Standard_DefineHandle.hxx> 
#include <DsgPrs_LengthPresentation.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Line.hxx>
#include <Geom_Surface.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomTools_Curve2dSet.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <gp_Vec2d.hxx>
#include <MMgt_TShared.hxx>
#include <OSD_Environment.hxx>
#include <Precision.hxx>
#include <Quantity_Factor.hxx>
#include <Quantity_Length.hxx>
#include <Quantity_NameOfColor.hxx>
#include <Quantity_PhysicalQuantity.hxx>
#include <Quantity_PlaneAngle.hxx>
#include <Quantity_TypeOfColor.hxx>
#include <ShapeSchema.hxx> 
#include <Standard_Boolean.hxx>
#include <Standard_CString.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Integer.hxx>
#include <Standard_IStream.hxx>
#include <Standard_Macro.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_OStream.hxx>
#include <Standard_Real.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_HArray1OfPnt2d.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColStd_HSequenceOfTransient.hxx>
#include <TColStd_MapIteratorOfMapOfTransient.hxx>
#include <TColStd_MapOfTransient.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_ListIteratorOfListOfShape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <UnitsAPI.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

// OCAF
#include <TDocStd_Document.hxx>
#include <TDF_Label.hxx>
#include <TDF_Reference.hxx>
#include <TDF_ChildIterator.hxx>
#include <TNaming_NamedShape.hxx>
#include <TNaming_Builder.hxx>
#include <TDataStd_Real.hxx>
#include <TFunction_Logbook.hxx>
#include <TDataStd_Name.hxx>
#include <TFunction_DriverTable.hxx>
#include <TFunction_Function.hxx>
#include <TDataStd_Integer.hxx>

// IO
#include <IGESControl_Controller.hxx>
#include <IGESControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <STEPControl_Reader.hxx>


#endif //_PreComp_


#endif

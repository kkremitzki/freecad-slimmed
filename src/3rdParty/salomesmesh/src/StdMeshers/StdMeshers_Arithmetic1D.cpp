//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : StdMeshers_Arithmetic1D.cxx
//  Author : Damien COQUERET, OCC
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/StdMeshers_Arithmetic1D.cxx,v 1.8.2.1 2008/11/27 13:03:49 abd Exp $
//
#include "StdMeshers_Arithmetic1D.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Mesh.hxx"

#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Arithmetic1D::StdMeshers_Arithmetic1D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen)
{
  _begLength = 1.;
  _endLength = 10.;
  _name = "Arithmetic1D";
  _param_algo_dim = 1; 
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Arithmetic1D::~StdMeshers_Arithmetic1D()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_Arithmetic1D::SetLength(double length, bool isStartLength)
     throw(SMESH_Exception)
{
  if ( (isStartLength ? _begLength : _endLength) != length ) {
    if (length <= 0)
      throw SMESH_Exception(LOCALIZED("length must be positive"));
    if ( isStartLength )
      _begLength = length;
    else
      _endLength = length;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

double StdMeshers_Arithmetic1D::GetLength(bool isStartLength) const
{
  return isStartLength ? _begLength : _endLength;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_Arithmetic1D::SaveTo(ostream & save)
{
  save << _begLength << " " << _endLength;
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_Arithmetic1D::LoadFrom(istream & load)
{
  bool isOK = true;
  isOK = !(load >> _begLength).bad();
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  isOK = !(load >> _endLength).bad();
  if (!isOK)
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_Arithmetic1D & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_Arithmetic1D & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize start and end length by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Arithmetic1D::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                  const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _begLength = _endLength = 0.;

  Standard_Real UMin, UMax;
  TopLoc_Location L;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  for ( int i = 1; i <= edgeMap.Extent(); ++i )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edgeMap( i ));
    Handle(Geom_Curve) C = BRep_Tool::Curve(edge, L, UMin, UMax);
    GeomAdaptor_Curve AdaptCurve(C);

    vector< double > params;
    SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();
    if ( SMESH_Algo::GetNodeParamOnEdge( aMeshDS, edge, params ))
    {
      nbEdges++;
      _begLength += GCPnts_AbscissaPoint::Length( AdaptCurve, params[0], params[1]);
      int nb = params.size();
      _endLength += GCPnts_AbscissaPoint::Length( AdaptCurve, params[nb-2], params[nb-1]);
    }
  }
  if ( nbEdges ) {
    _begLength /= nbEdges;
    _endLength /= nbEdges;
  }
  return nbEdges;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_Arithmetic1D::SetParametersByDefaults(const TDefaults&  dflts,
                                                      const SMESH_Mesh* /*mesh*/)
{
  return ( _begLength = _endLength = dflts._elemLength );
}

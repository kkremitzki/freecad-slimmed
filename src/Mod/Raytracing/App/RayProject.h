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

 


#ifndef _RayProject_h_
#define _RayProject_h_

#include <App/DocumentObjectGroup.h>
#include <App/PropertyStandard.h>
#include <App/PropertyFile.h>

namespace Raytracing
{

class Property;

/** Base class of all Feature classes in FreeCAD
 */
//class RayFeature: public Part::PartFeature
class AppRaytracingExport RayProject: public App::DocumentObjectGroup
{
    PROPERTY_HEADER(Raytracing::RayProject);

public:
    /// Constructor
    RayProject(void);

    App::PropertyFileIncluded PageResult;
    App::PropertyFile Template;


    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
  //@}


};


} //namespace Raytracing



#endif //_RayFeature_h_

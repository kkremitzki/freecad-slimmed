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


#ifndef PARTDESIGN_DressUp_H
#define PARTDESIGN_DressUp_H

#include <App/PropertyStandard.h>
#include "Feature.h"

namespace PartDesign
{

class PartDesignExport DressUp : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::DressUp);

public:
    DressUp();

    App::PropertyLinkSub Base;

    short mustExecute() const;
    /// updates the Placement property from the Placement of the BaseFeature
    void positionByBaseFeature(void);
    Part::TopoShape getBaseShape();
    /// extracts all edges from the subshapes (inkluding face edges) and furthermore adds
    /// all C0 continius edges to the vector
    void getContiniusEdges(Part::TopoShape, std::vector< std::string >&);

protected:
    void onChanged(const App::Property* prop);
};

} //namespace PartDesign


#endif // PARTDESIGN_DressUp_H

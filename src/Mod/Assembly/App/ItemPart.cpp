/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include <Base/Placement.h>

#include "ItemPart.h"
#include <Mod/Part/App/PartFeature.h>


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ItemPart, Assembly::Item)

ItemPart::ItemPart()
{
    ADD_PROPERTY(Model,     (0));
    ADD_PROPERTY(Annotation,(0));
}

short ItemPart::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ItemPart::execute(void)
{
     return App::DocumentObject::StdReturn;
}

TopoDS_Shape ItemPart::getShape(void) const 
{
    App::DocumentObject* obj = Model.getValue();

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return static_cast<Part::Feature*>(obj)->Shape.getValue();
    }

    return TopoDS_Shape();

}

}
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
#endif

#include <Base/Placement.h>

#include "ConstraintGroup.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ConstraintGroup, App::DocumentObject)

ConstraintGroup::ConstraintGroup()
{
    ADD_PROPERTY(Constraints,(0));
}

short ConstraintGroup::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ConstraintGroup::execute(void)
{
 
    return App::DocumentObject::StdReturn;
}

}
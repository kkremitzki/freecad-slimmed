/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef ROBOT_VIEWPROVIDEREDGE2TRACOBJECT_H
#define ROBOT_VIEWPROVIDEREDGE2TRACOBJECT_H

#include "ViewProviderTrajectory.h"

namespace RobotGui
{

class RobotGuiExport ViewProviderEdge2TracObject : public ViewProviderTrajectory
{
    PROPERTY_HEADER(RobotGui::ViewProviderEdge2TracObject);

public:
    virtual bool doubleClicked(void);
protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

};

} //namespace RobotGui


#endif // ROBOT_VIEWPROVIDEREDGE2TRACOBJECT_H

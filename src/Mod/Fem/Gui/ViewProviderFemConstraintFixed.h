/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_VIEWPROVIDERFEMCONSTRAINTFIXED_H
#define GUI_VIEWPROVIDERFEMCONSTRAINTFIXED_H

#include "ViewProviderFemConstraintOnBoundary.h"

namespace FemGui
{

class FemGuiExport ViewProviderFemConstraintFixed
    : public FemGui::ViewProviderFemConstraintOnBoundary
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemConstraintFixed);

public:
    /// Constructor
    ViewProviderFemConstraintFixed();
    ~ViewProviderFemConstraintFixed() override;

    void updateData(const App::Property*) override;

protected:
    bool setEdit(int ModNum) override;
};

}  // namespace FemGui


#endif  // GUI_VIEWPROVIDERFEMCONSTRAINTFIXED_H

/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _DrawViewDetail_h_
#define _DrawViewDetail_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/FeaturePython.h>
#include <Base/Vector3D.h>

#include "DrawViewPart.h"


class gp_Pln;
class gp_Ax2;
class TopoDS_Face;

namespace TechDraw
{
class Face;
}

namespace TechDraw
{


class TechDrawExport DrawViewDetail : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawViewDetail);

public:
    /// Constructor
    DrawViewDetail();
    ~DrawViewDetail() override;

    App::PropertyLink   BaseView;
    App::PropertyVector AnchorPoint;
    App::PropertyFloat  Radius;
    App::PropertyString Reference;

    short mustExecute() const override;
    App::DocumentObjectExecReturn *execute() override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewPart";
    }
    void unsetupObject() override;


    void detailExec(TopoDS_Shape s, 
                    DrawViewPart* baseView,
                    DrawViewSection* sectionAlias);
    double getFudgeRadius();
    TopoDS_Shape projectEdgesOntoFace(TopoDS_Shape edgeShape, TopoDS_Face projFace, gp_Dir projDir);

    std::vector<DrawViewDetail*> getDetailRefs() const override;

    void postHlrTasks(void) override;

protected:
    Base::Vector3d toR3(const gp_Ax2 fromSystem, const Base::Vector3d fromPoint);
    void getParameters();
    double m_fudge;
    bool debugDetail() const;

};

typedef App::FeaturePythonT<DrawViewDetail> DrawViewDetailPython;

} //namespace TechDraw

#endif

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <qobject.h>
#endif

#include "Workbench.h"
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

using namespace TechDrawGui;

TYPESYSTEM_SOURCE(TechDrawGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* draw = new Gui::MenuItem;
    root->insertItem(item, draw);
    draw->setCommand("TechDraw");
    *draw << "TechDraw_PageDefault";
    *draw << "TechDraw_PageTemplate";
    *draw << "TechDraw_Redraw";
    *draw << "Separator";
    *draw << "TechDraw_View";
    *draw << "TechDraw_ActiveView";
    *draw << "TechDraw_ProjectionGroup";
    *draw << "TechDraw_ViewSection";
    *draw << "TechDraw_ViewDetail";
    *draw << "Separator";
    *draw << "TechDraw_DraftView";
    *draw << "TechDraw_ArchView";
    *draw << "TechDraw_SpreadsheetView";
    *draw << "Separator";
    *draw << "TechDraw_ClipGroup";
    *draw << "TechDraw_ClipGroupAdd";
    *draw << "TechDraw_ClipGroupRemove";
    *draw << "Separator";
    *draw << "TechDraw_NewLengthDimension";
    *draw << "TechDraw_NewDistanceXDimension";
    *draw << "TechDraw_NewDistanceYDimension";
    *draw << "TechDraw_NewRadiusDimension";
    *draw << "TechDraw_NewDiameterDimension";
    *draw << "TechDraw_NewAngleDimension";
    *draw << "TechDraw_HorizontalExtent";
    *draw << "TechDraw_VerticalExtent";
    *draw << "TechDraw_LinkDimension";
    *draw << "TechDraw_Balloon";
    *draw << "Separator";
    *draw << "TechDraw_ExportPageSVG";
    *draw << "TechDraw_ExportPageDXF";
    *draw << "Separator";
    *draw << "TechDraw_NewHatch";
    *draw << "TechDraw_NewGeomHatch";
    *draw << "TechDraw_Symbol";
    *draw << "TechDraw_Image";
    *draw << "TechDraw_ToggleFrame";
    *draw << "Separator";
    *draw << "TechDraw_Annotation";
    *draw << "TechDraw_LeaderLine";
    *draw << "TechDraw_RichAnno";
    *draw << "TechDraw_CosmeticVertex";
    *draw << "TechDraw_Midpoints";
    *draw << "TechDraw_Quadrant";
    *draw << "TechDraw_FaceCenterLine";
    *draw << "TechDraw_2LineCenterLine";
    *draw << "TechDraw_2PointCenterLine";
    *draw << "TechDraw_CosmeticEraser";
    *draw << "TechDraw_DecorateLine";
    *draw << "TechDraw_ShowAll";
    *draw << "TechDraw_WeldSymbol";
    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_Redraw";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("TechDraw Views");
    *views << "TechDraw_View";
    *views << "TechDraw_ActiveView";
    *views << "TechDraw_ProjectionGroup";
    *views << "TechDraw_ViewSection";
    *views << "TechDraw_ViewDetail";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_ArchView";
    *views << "TechDraw_SpreadsheetView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_ClipGroup";
    *clips << "TechDraw_ClipGroupAdd";
    *clips << "TechDraw_ClipGroupRemove";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_NewLengthDimension";
    *dims << "TechDraw_NewDistanceXDimension";
    *dims << "TechDraw_NewDistanceYDimension";
    *dims << "TechDraw_NewRadiusDimension";
    *dims << "TechDraw_NewDiameterDimension";
    *dims << "TechDraw_NewAngleDimension";
    *dims << "TechDraw_NewAngle3PtDimension";
    *dims << "TechDraw_ExtentGrp";
//    *dims << "TechDraw_HorizontalExtent";
//    *dims << "TechDraw_VerticalExtent";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_Balloon";
//    *dims << "TechDraw_NewDimension"

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPageSVG";
    *file << "TechDraw_ExportPageDXF";

    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_NewGeomHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichAnno";
    *anno << "TechDraw_CosmeticVertexGrp";
    *anno << "TechDraw_CenterLineGrp";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem *pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_Redraw";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("Views");
    *views << "TechDraw_View";
    *views << "TechDraw_ActiveView";
//    *views << "TechDraw_NewMulti";    //deprecated
    *views << "TechDraw_ProjectionGroup";
    *views << "TechDraw_ViewSection";
    *views << "TechDraw_ViewDetail";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_SpreadsheetView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_ClipGroup";
    *clips << "TechDraw_ClipGroupAdd";
    *clips << "TechDraw_ClipGroupRemove";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_NewLengthDimension";
    *dims << "TechDraw_NewDistanceXDimension";
    *dims << "TechDraw_NewDistanceYDimension";
    *dims << "TechDraw_NewRadiusDimension";
    *dims << "TechDraw_NewDiameterDimension";
    *dims << "TechDraw_NewAngleDimension";
    *dims << "TechDraw_NewAngle3PtDimension";
    *dims << "TechDraw_ExtentGrp";
//    *dims << "TechDraw_HorizontalExtent";
//    *dims << "TechDraw_VerticalExtent";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_Balloon";
//    *dims << "TechDraw_NewDimension";

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPageSVG";
    *file << "TechDraw_ExportPageDXF";
 
    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_NewGeomHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichAnno";
    *anno << "TechDraw_CosmeticVertexGrp";
    *anno << "TechDraw_CenterLineGrp";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";

    return root;
}

# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""This module provides the code for Draft make_shapestring function.
"""
## @package make_shapestring
# \ingroup DRAFT
# \brief This module provides the code for Draft make_shapestring function.

import FreeCAD as App

from draftutils.gui_utils import format_object
from draftutils.gui_utils import select

from draftobjects.shapestring import ShapeString
if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraft


def make_shapestring(String, FontFile, Size=100, Tracking=0):
    """ShapeString(Text,FontFile,[Height],[Track])
    
    Turns a text string into a Compound Shape
    
    Parameters
    ----------
    majradius : 
        Major radius of the ellipse.

    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
        
    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython",
                                       "ShapeString")
    ShapeString(obj)
    obj.String = String
    obj.FontFile = FontFile
    obj.Size = Size
    obj.Tracking = Tracking

    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        format_object(obj)
        obrep = obj.ViewObject
        if "PointSize" in obrep.PropertiesList: obrep.PointSize = 1 # hide the segment end points
        select(obj)
    obj.recompute()
    return obj


makeShapeString = make_shapestring

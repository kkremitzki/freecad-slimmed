# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides the viewprovider code for the BSpline object.

At the moment this view provider subclasses the Wire view provider,
and behaves the same as it. In the future this could change
if another behavior is desired.
"""
## @package view_bspline
# \ingroup draftviewproviders
# \brief Provides the view provider code for BSpline objects.

## \addtogroup draftviewproviders
# @{
from draftviewproviders.view_wire import ViewProviderWire


class ViewProviderBSpline(ViewProviderWire):
    """The view provider for the BSpline object."""

    def __init__(self, vobj):
        super(ViewProviderBSpline, self).__init__(vobj)

    def setupContextMenu(self, vobj, menu):
        return


# Alias for compatibility with v0.18 and earlier
_ViewProviderBSpline = ViewProviderBSpline

## @}

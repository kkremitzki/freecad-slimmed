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
"""Provides the Fillet class for objects created with a prototype version.

The original Fillet object and Gui Command was introduced
in the development cycle of 0.19, in commit d5ca09c77b, 2019-08-22.

However, when this class was implemented, the reorganization
of the workbench was not advanced.

When the reorganization was on its way it was clear that this tool
also needed to be broken into different modules; however, this was done
only at the end of the reorganization.

In commit 01df7c0a63, 2020-02-10, the Gui Command was removed from
the graphical interface so that the user cannot create this object
graphically any more. The object class was still kept
so that previous objects created between August 2019 and February 2020
would open correctly.

Now in this module the older class is redirected to the new class
in order to migrate the object.

A new Gui Command in `draftguitools` and new make function in `draftmake`
are now used to create `Fillet` objects. Therefore, this module
is only required to migrate old objects created in that time
with the 0.19 development version.

Since this module is only used to migrate older objects, it is only temporary,
and will be removed after one year of the original introduction of the tool,
that is, in August 2020.
"""
## @package DraftFillet
# \ingroup DRAFT
# \brief Provides Fillet class for objects created with a prototype version.
#
# This module is only required to migrate old objects created
# from August 2019 to February 2020. It will be removed definitely
# in August 2020, as the new Fillet object should be available.

import FreeCAD as App
import draftobjects.fillet
import draftobjects.base as base
from draftutils.messages import _wrn

if App.GuiUp:
    import draftviewproviders.view_fillet as view_fillet

# -----------------------------------------------------------------------------
# Removed definitions
# def _extract_edges(objs):

# def makeFillet(objs, radius=100, chamfer=False, delete=False):

# class Fillet(Draft._DraftObject):

# class CommandFillet(DraftTools.Creator):
# -----------------------------------------------------------------------------


class Fillet(base.DraftObject):
    """The old Fillet object. DEPRECATED.

    This class is solely defined to migrate older objects.

    When an old object is opened it will reconstruct the object
    by searching for this class. So we implement `onDocumentRestored`
    to test that it is the old class and we migrate it,
    by assigning the new proxy class, and the new viewprovider.
    """

    def onDocumentRestored(self, obj):
        """Run when the document that is using this class is restored."""
        if hasattr(obj, "Proxy") and obj.Proxy.Type == "Fillet":
            _module = str(obj.Proxy.__class__)
            _module = _module.lstrip("<class '").rstrip("'>")

            if _module == "DraftFillet.Fillet":
                self._migrate(obj, _module)

    def _migrate(self, obj, _module):
        """Migrate the object to the new object."""
        _wrn("v0.19, {0}, '{1}' object ".format(obj.Label, _module)
             + "will be migrated to 'draftobjects.fillet.Fillet'")

        draftobjects.fillet.Fillet(obj)

        if App.GuiUp:
            vobj = obj.ViewObject
            view_fillet.ViewProviderFillet(vobj)

# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Unit test for the Draft Workbench, import tests."""

import unittest
import drafttests.auxiliary as aux


class DraftImport(unittest.TestCase):
    """Import the Draft modules."""

    # No document is needed to test 'import Draft' or other modules
    # thus 'setUp' just draws a line, and 'tearDown' isn't defined.
    def setUp(self):
        aux._draw_header()

    def test_import_draft(self):
        """Import the Draft module."""
        module = "Draft"
        imported = aux._import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_geomutils(self):
        """Import Draft geometrical utilities."""
        module = "DraftGeomUtils"
        imported = aux._import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_vecutils(self):
        """Import Draft vector utilities."""
        module = "DraftVecUtils"
        imported = aux._import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_svg(self):
        """Import Draft SVG utilities."""
        module = "getSVG"
        imported = aux._import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

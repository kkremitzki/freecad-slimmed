# Robot gui init module  
# (c) 2009 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2009                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2009                                                   *
#***************************************************************************/



class RobotWorkbench ( Workbench ):
	"Robot workbench object"
	Icon = """
                /* XPM */
                static char * robot_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #000300",
                "+	c #070905",
                "@	c #0E100C",
                "#	c #171816",
                "$	c #2D2E2C",
                "%	c #434542",
                "&	c #838582",
                "*	c #FEFFFC",
                "                ",
                "  $+$$$$$$$$$$  ",
                "  $**********$  ",
                "  $*+.****@@*$  ",
                "  $*+.****@@*$  ",
                "  $**********$  ",
                "  #$&&&&&&&&&+  ",
                "  $$$$$$$$$$$+  ",
                "       **&      ",
                "++++++++++++++..",
                ".%&&$&&&&&&&&&&.",
                "+&*************+",
                "+&*************+",
                "+&*************+",
                "+&*************+",
                "+.+++++++++++++."};
			"""
	MenuText = "Robot"
	ToolTip = "Robot workbench"

	def Initialize(self):
		# load the module
		import RobotGui
		import Robot
	def GetClassName(self):
		return "RobotGui::Workbench"

Gui.addWorkbench(RobotWorkbench())

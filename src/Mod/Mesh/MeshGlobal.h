/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCGlobal.h>

#ifndef MESH_GLOBAL_H
#define MESH_GLOBAL_H


// Mesh
#ifndef MeshExport
#ifdef Mesh_EXPORTS
#       define MeshExport   FREECAD_DECL_EXPORT
#else
#       define MeshExport   FREECAD_DECL_IMPORT
#endif
#endif

// MeshGui
#ifndef MeshGuiExport
#ifdef MeshGui_EXPORTS
#  define MeshGuiExport   FREECAD_DECL_EXPORT
#else
#  define MeshGuiExport   FREECAD_DECL_IMPORT
#endif
#endif

#endif //MESH_GLOBAL_H

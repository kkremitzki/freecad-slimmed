/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_MODULE3D_H
#define DCM_MODULE3D_H

#define DCM_USE_MODULE3D

#ifdef _WIN32
	//warning about to long decoraded names, won't affect the code correctness
	#pragma warning( disable : 4503 )
#endif

#include "module3d/defines.hpp"
#include "module3d/geometry.hpp"
#include "module3d/distance.hpp"
#include "module3d/parallel.hpp"
#include "module3d/angle.hpp"
#include "module3d/coincident.hpp"
#include "module3d/alignment.hpp"
#include "module3d/module.hpp"

#ifdef DCM_USE_MODULESTATE
#include "module3d/state.hpp"
#endif

#endif //DCM_MODULE3D_H


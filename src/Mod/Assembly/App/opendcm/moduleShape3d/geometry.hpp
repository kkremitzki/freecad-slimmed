/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

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

#ifndef DCM_GEOMETRY_SHAPE3D_H
#define DCM_GEOMETRY_SHAPE3D_H

#include "opendcm/core/geometry.hpp"
#include "opendcm/module3d/geometry.hpp"

namespace dcm {
namespace details {
  
template<typename T>
struct tag_traits {
  typedef void return_type;
  typedef void value_type;
  BOOST_MPL_ASSERT_MSG(false, NO_TAG_TRAITS_SPECIFIED_FOR_TYPE, (T));
};

} //details

namespace tag {
  
struct segment3D : details::stacked2_geometry<weight::segment, point3D, point3D> {};  
  
} //tag
} //dcm

#endif
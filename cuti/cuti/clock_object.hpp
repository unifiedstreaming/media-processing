/*
 * Copyright (C) 2021-2026 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
 * 2.1 of the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef CUTI_CLOCK_OBJECT_HPP_
#define CUTI_CLOCK_OBJECT_HPP_

#include "linkage.h"
#include "chrono_types.hpp"

namespace cuti
{

/*
 * Default clock object: delegates to the cuti clock when asked what
 * time it is.
 */
struct CUTI_ABI default_clock_object_t
{
  time_point_t now() const
  {
    return cuti_clock_t::now();
  }
};

/*
 * User clock object: returns the value of a time_point variable
 * when asked what time it is.  Useful for mocking.
 */
struct CUTI_ABI user_clock_object_t
{
  explicit user_clock_object_t(time_point_t& time_point)
  : time_point_(time_point)
  { }

  time_point_t now() const
  {
    return time_point_;
  }

private :
  time_point_t& time_point_;
};

} // cuti

#endif

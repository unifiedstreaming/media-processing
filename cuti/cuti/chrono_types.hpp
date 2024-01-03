/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_CHRONO_TYPES_HPP_
#define CUTI_CHRONO_TYPES_HPP_

#include <chrono>

namespace cuti
{

/*
 * Convenience aliases for some chrono types.
 */
using cuti_clock_t = std::chrono::system_clock;
using duration_t = cuti_clock_t::duration;
using milliseconds_t = std::chrono::milliseconds;
using minutes_t = std::chrono::minutes;
using seconds_t = std::chrono::seconds;
using time_point_t = cuti_clock_t::time_point;

using std::chrono::duration_cast;

} // namespace cuti

#endif

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

#ifndef CUTI_CALLBACK_HPP_
#define CUTI_CALLBACK_HPP_

#include "function.hpp"
#include "linkage.h"
#include "stack_marker.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace cuti
{

/*
 * Type-erased callback wrapper.
 */
using callback_t = function_t<void(stack_marker_t& base_marker)>;

} // cuti

#endif

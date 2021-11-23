/*
 * Copyright (C) 2021 CodeShop B.V.
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

#ifndef CUTI_STRUCTURE_READER_HPP_
#define CUTI_STRUCTURE_READER_HPP_

#include "reader_utils.hpp"

namespace cuti
{

using begin_structure_reader_t = detail::expected_reader_t<'{'>;

template<typename T>
using structure_element_reader_t = detail::element_reader_t<T>;

using end_structure_reader_t = detail::expected_reader_t<'}'>;

} // cuti

#endif

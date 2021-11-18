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

#ifndef CUTI_SEQUENCE_WRITER_HPP_
#define CUTI_SEQUENCE_WRITER_HPP_

#include "writer_utils.hpp"

namespace cuti
{

namespace detail
{

extern CUTI_ABI char const sequence_prefix[];
extern CUTI_ABI char const sequence_suffix[];

} // detail

using begin_sequence_writer_t =
  detail::literal_writer_t<detail::sequence_prefix>;

template<typename T>
using sequence_element_writer_t = detail::element_writer_t<T>;

using end_sequence_writer_t =
  detail::literal_writer_t<detail::sequence_suffix>;

} // cuti

#endif

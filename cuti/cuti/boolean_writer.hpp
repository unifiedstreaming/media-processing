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

#ifndef CUTI_BOOLEAN_WRITER_HPP_
#define CUTI_BOOLEAN_WRITER_HPP_

#include "flag.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

namespace cuti
{

template<>
struct writer_traits_t<bool>
{
  using type = detail::boolean_writer_t<bool>;
};

template<>
struct writer_traits_t<flag_t>
{
  using type = detail::boolean_writer_t<flag_t>;
};

} // cuti

#endif

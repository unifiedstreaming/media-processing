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

#ifndef CUTI_TUPLE_WRITER_HPP_
#define CUTI_TUPLE_WRITER_HPP_

#include "writer_traits.hpp"
#include "writer_utils.hpp"

namespace cuti
{

template<typename... Types>
struct writer_traits_t<std::tuple<Types...>>
{
  using type = detail::tuple_writer_t<std::tuple<Types...>>;
};

template<typename T1, typename T2>
struct writer_traits_t<std::pair<T1, T2>>
{
  using type = detail::tuple_writer_t<std::pair<T1, T2>>;
};

} // cuti

#endif

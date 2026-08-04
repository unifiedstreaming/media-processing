/*
 * Copyright (C) 2026 CodeShop B.V.
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

#ifndef CUTI_TYPE_TRAITS_HPP_
#define CUTI_TYPE_TRAITS_HPP_

#include <type_traits>

namespace cuti
{

/*
 * Grab bag of additional type traits not found in the standard's
 * <type_traits> header.
 */

template<typename T, typename U, typename = void>
struct equality_comparable : std::false_type { };

template<typename T, typename U>
struct equality_comparable<T, U, std::void_t<decltype(
  std::declval<T const&>() == std::declval<U const&>() &&
  std::declval<U const&>() == std::declval<T const&>() &&
  std::declval<T const&>() != std::declval<U const&>() &&
  std::declval<U const&>() != std::declval<T const&>()
)>> : std::true_type { };

template<typename T, typename U>
bool constexpr equality_comparable_v = equality_comparable<T, U>::value;
  
} // cuti

#endif

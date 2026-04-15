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

#ifndef CUTI_ALTERNATIVE_INDEX_HPP_
#define CUTI_ALTERNATIVE_INDEX_HPP_

#include <variant>

namespace cuti
{

template<typename Alternative, typename Variant>
struct has_alternative_t;

template<typename Alternative>
struct has_alternative_t<Alternative, std::variant<>>
{
  static bool constexpr value = false;
};

template<typename Alternative, typename... Ts>
struct has_alternative_t<Alternative, std::variant<Alternative, Ts...>>
{
  static bool constexpr value = true;
};

template<typename Alternative, typename T1, typename... Ts>
struct has_alternative_t<Alternative, std::variant<T1, Ts...>>
{
  static bool constexpr value =
    has_alternative_t<Alternative, std::variant<Ts...>>::value;
};

template<typename Alternative, typename Variant>
inline bool constexpr has_alternative_v = 
  has_alternative_t<Alternative, Variant>::value;  

template<typename Alternative, typename Variant>
struct alternative_index_t;

template<typename Alternative, typename... Ts>
struct alternative_index_t<Alternative, std::variant<Alternative, Ts...>>
{
  static_assert(!has_alternative_v<Alternative, std::variant<Ts...>>);
  static unsigned int constexpr value = 0;
};

template<typename Alternative, typename T1, typename... Ts>
struct alternative_index_t<Alternative, std::variant<T1, Ts...>>
{
  static unsigned int constexpr value =
    1 + alternative_index_t<Alternative, std::variant<Ts...>>::value;
};

template<typename Alternative, typename Variant>
inline unsigned int constexpr alternative_index_v =
  alternative_index_t<Alternative, Variant>::value;

} // cuti

#endif

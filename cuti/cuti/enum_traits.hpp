/*
 * Copyright (C) 2024 CodeShop B.V.
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

#ifndef CUTI_ENUM_TRAITS_HPP_
#define CUTI_ENUM_TRAITS_HPP_

#include "linkage.h"

#include <type_traits>

namespace cuti
{

namespace detail
{

/*
 * By default, we use an enum's underlying type for serialization
 * purposes.
 */
template<typename UnderlyingType,
         bool IsSigned = std::is_signed_v<UnderlyingType>>
struct serialized_underlying_type_t
{
  static_assert(IsSigned == std::is_signed_v<UnderlyingType>);
  using type = UnderlyingType;
};

/*
 * However, if the underlying type is one of the three char types we
 * use (unsigned) int, since the cuti protocol does not support
 * serialization of char types.
 */
template<bool IsSigned>
struct serialized_underlying_type_t<signed char, IsSigned>
{
  static_assert(IsSigned);
  using type = int;
};

template<bool IsSigned>
struct serialized_underlying_type_t<unsigned char, IsSigned>
{
  static_assert(!IsSigned);
  using type = unsigned int;
};

template<>
struct CUTI_ABI serialized_underlying_type_t<char, true>
{
  using type = int;
};

template<>
struct CUTI_ABI serialized_underlying_type_t<char, false>
{
  using type = unsigned int;
};

template<typename UnderlyingType>
using serialized_underlying_t =
  typename serialized_underlying_type_t<UnderlyingType>::type;

} // detail

template<typename EnumType>
struct serialized_enum_type_t
{
  static_assert(std::is_enum_v<EnumType>);
  using type =
    detail::serialized_underlying_t<std::underlying_type_t<EnumType>>;
};

template<typename EnumType>
using serialized_enum_t = typename serialized_enum_type_t<EnumType>::type;
    
} // cuti

#endif

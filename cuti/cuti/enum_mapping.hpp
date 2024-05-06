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

#ifndef CUTI_ENUM_MAPPING_HPP_
#define CUTI_ENUM_MAPPING_HPP_

#include "linkage.h"

#include <cstddef> // for std::byte
#include <type_traits>

namespace cuti
{

/*
 * Specialize this class template to enable cuti serialization for an
 * enum type T.
 *
 * This specialization must contain a static member function named
 * from_underlying, taking a std::underlying_type_t<T> and returning
 * a T.
 *
 * Please note that it is the user's responsibility to filter out any
 * unexpected underlying values by having from_underlying() throw
 * something derived from std::exception.
 *
 * C++ allows us to define a specialization of cuti::enum_mapping_t in
 * either the cuti namespace or the global namespace, but not in a
 * user namespace.
 */
template<typename T>
struct enum_mapping_t;

template<>
struct CUTI_ABI enum_mapping_t<std::byte>
{
  /*
   * This specialization is unusal because it accepts all underlying
   * values. This is OK for std::byte, but likely disastrous for
   * most other enum types. You have been warned.
   */
  static
  std::byte from_underlying(std::underlying_type_t<std::byte> underlying)
  {
    return std::byte{underlying};
  }
};

/*
 * to_underlying(): ripped from C++23.
 */
template<typename T,
         typename = std::enable_if_t<std::is_enum_v<T>>>
constexpr std::underlying_type_t<T> to_underlying(T value) noexcept
{
  return static_cast<std::underlying_type_t<T>>(value);
}

namespace detail
{

/*
 * By default, we use an enum's underlying type for serialization
 * purposes.
 */
template<typename UnderlyingType,
         bool IsSigned = std::is_signed_v<UnderlyingType>>
struct serialized_underlying_traits_t
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
struct serialized_underlying_traits_t<signed char, IsSigned>
{
  static_assert(IsSigned);
  using type = int;
};

template<bool IsSigned>
struct serialized_underlying_traits_t<unsigned char, IsSigned>
{
  static_assert(!IsSigned);
  using type = unsigned int;
};

template<>
struct serialized_underlying_traits_t<char, true>
{
  using type = int;
};

template<>
struct serialized_underlying_traits_t<char, false>
{
  using type = unsigned int;
};

template<typename EnumType>
struct serialized_enum_traits_t
{
  static_assert(std::is_enum_v<EnumType>);
  using type = typename detail::serialized_underlying_traits_t<
      std::underlying_type_t<EnumType>>::type;
};

} // detail

template<typename UnderlyingType>
using serialized_underlying_t =
  typename detail::serialized_underlying_traits_t<UnderlyingType>::type;

template<typename T, typename = std::enable_if_t<!std::is_enum_v<T>>>
constexpr serialized_underlying_t<T>
to_serialized_underlying(T value) noexcept
{
  return serialized_underlying_t<T>{value};
}

template<typename EnumType>
using serialized_enum_t =
  typename detail::serialized_enum_traits_t<EnumType>::type;
    
template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
constexpr serialized_enum_t<T>
to_serialized_enum(T value) noexcept
{
  return to_serialized_underlying(to_underlying(value));
}

} // cuti

#endif

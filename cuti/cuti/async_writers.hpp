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

#ifndef CUTI_ASYNC_WRITERS_HPP_
#define CUTI_ASYNC_WRITERS_HPP_

#include "flag.hpp"
#include "identifier.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <array>
#include <string>
#include <tuple>
#include <vector>

namespace cuti
{

/*
 * Built-async writers; use writer_t<T> for a writer writing a T.
 */
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

template<>
struct writer_traits_t<unsigned short>
{
  using type = detail::unsigned_writer_t<unsigned short>;
};

template<>
struct writer_traits_t<unsigned int>
{
  using type = detail::unsigned_writer_t<unsigned int>;
};

template<>
struct writer_traits_t<unsigned long>
{
  using type = detail::unsigned_writer_t<unsigned long>;
};

template<>
struct writer_traits_t<unsigned long long>
{
  using type = detail::unsigned_writer_t<unsigned long long>;
};

template<>
struct writer_traits_t<short>
{
  using type = detail::signed_writer_t<short>;
};

template<>
struct writer_traits_t<int>
{
  using type = detail::signed_writer_t<int>;
};

template<>
struct writer_traits_t<long>
{
  using type = detail::signed_writer_t<long>;
};

template<>
struct writer_traits_t<long long>
{
  using type = detail::signed_writer_t<long long>;
};

template<>
struct writer_traits_t<std::string>
{
  using type = detail::blob_writer_t<std::string>;
};

template<>
struct writer_traits_t<identifier_t>
{
  using type = detail::identifier_writer_t;
};

template<typename T>
struct writer_traits_t<std::vector<T>>
{
  using type = detail::vector_writer_t<T>;
};

template<>
struct writer_traits_t<std::vector<char>>
{
  using type = detail::blob_writer_t<std::vector<char>>;
};

template<>
struct writer_traits_t<std::vector<signed char>>
{
  using type = detail::blob_writer_t<std::vector<signed char>>;
};

template<>
struct writer_traits_t<std::vector<unsigned char>>
{
  using type = detail::blob_writer_t<std::vector<unsigned char>>;
};

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

template<typename T, std::size_t N>
struct writer_traits_t<std::array<T, N>>
{
  using type = detail::tuple_writer_t<std::array<T, N>>;
};

template<typename T>
struct writer_traits_t
{
  using type = detail::user_type_writer_t<T>;
};

/*
 * Helpers for streaming async writing and async writers for
 * user-defined types.
 */
using begin_sequence_writer_t = detail::begin_sequence_writer_t;
using end_sequence_writer_t = detail::end_sequence_writer_t;

using begin_structure_writer_t = detail::begin_structure_writer_t;
using end_structure_writer_t = detail::end_structure_writer_t;

} // cuti

#endif

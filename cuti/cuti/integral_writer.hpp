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

#ifndef CUTI_INTEGRAL_WRITER_HPP_
#define CUTI_INTEGRAL_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <exception>
#include <type_traits>

namespace cuti
{

namespace detail
{

template<typename T>
struct CUTI_ABI digits_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = void;

  digits_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  digits_writer_t(digits_writer_t const&) = delete;
  digits_writer_t& operator=(digits_writer_t const&) = delete;
  
  void start(T value);

private :
  void write_digits();

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  T value_;
  T divisor_;
};

extern template struct digits_writer_t<unsigned short>;
extern template struct digits_writer_t<unsigned int>;
extern template struct digits_writer_t<unsigned long>;
extern template struct digits_writer_t<unsigned long long>;

template<typename T>
struct CUTI_ABI unsigned_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = void;

  unsigned_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  unsigned_writer_t(unsigned_writer_t const&) = delete;
  unsigned_writer_t& operator=(unsigned_writer_t const&) = delete;

  void start(T value);

private :
  void on_prefix_written();
  void on_digits_written();
  void on_failure(std::exception_ptr ex);

private :
  result_t<void>& result_;
  subroutine_t<unsigned_writer_t, literal_writer_t> prefix_writer_;
  subroutine_t<unsigned_writer_t, digits_writer_t<T>> digits_writer_;
  T value_;
};

extern template struct unsigned_writer_t<unsigned short>;
extern template struct unsigned_writer_t<unsigned int>;
extern template struct unsigned_writer_t<unsigned long>;
extern template struct unsigned_writer_t<unsigned long long>;

template<typename T>
struct CUTI_ABI signed_writer_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  using result_value_t = void;

  signed_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  signed_writer_t(signed_writer_t const&) = delete;
  signed_writer_t& operator=(signed_writer_t const&) = delete;

  void start(T value);

private :
  using UT = std::make_unsigned_t<T>;

  void on_prefix_written();
  void on_digits_written();
  void on_failure(std::exception_ptr ex);

private :
  result_t<void>& result_;
  subroutine_t<signed_writer_t, literal_writer_t> prefix_writer_;
  subroutine_t<signed_writer_t, digits_writer_t<UT>> digits_writer_;

  UT unsigned_value_;
};

extern template struct signed_writer_t<short>;
extern template struct signed_writer_t<int>;
extern template struct signed_writer_t<long>;
extern template struct signed_writer_t<long long>;

} // detail

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

} // cuti

#endif

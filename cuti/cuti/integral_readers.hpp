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

#ifndef CUTI_INTEGRAL_READERS_HPP_
#define CUTI_INTEGRAL_READERS_HPP_

#include "bound_inbuf.hpp"
#include "linkage.h"
#include "reader_traits.hpp"
#include "reader_utils.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <exception>
#include <type_traits>

namespace cuti
{

namespace detail
{

template<typename T>
struct CUTI_ABI digits_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = T;

  digits_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  digits_reader_t(digits_reader_t const&) = delete;
  digits_reader_t& operator=(digits_reader_t const&) = delete;

  void start(T max);

private :
  void read_digits();

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  T max_;
  bool digit_seen_;
  T value_;
};

extern template struct digits_reader_t<unsigned short>;
extern template struct digits_reader_t<unsigned int>;
extern template struct digits_reader_t<unsigned long>;
extern template struct digits_reader_t<unsigned long long>;

template<typename T>
struct CUTI_ABI unsigned_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = T;

  unsigned_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  unsigned_reader_t(unsigned_reader_t const&) = delete;
  unsigned_reader_t& operator=(unsigned_reader_t const&) = delete;

  void start();

private :
  void on_begin_token(int c);
  void on_digits_read(T value);
  void on_failure(std::exception_ptr ex);

private :
  result_t<T>& result_;
  subroutine_t<unsigned_reader_t, token_finder_t> finder_;
  subroutine_t<unsigned_reader_t, digits_reader_t<T>> digits_reader_;
};

extern template struct unsigned_reader_t<unsigned short>;
extern template struct unsigned_reader_t<unsigned int>;
extern template struct unsigned_reader_t<unsigned long>;
extern template struct unsigned_reader_t<unsigned long long>;

template<typename T>
struct CUTI_ABI signed_reader_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  using value_t = T;

  signed_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  signed_reader_t(signed_reader_t const&) = delete;
  signed_reader_t& operator=(signed_reader_t const&) = delete;

  void start();

private :
  using UT = std::make_unsigned_t<T>;

  void on_begin_token(int c);
  void on_digits_read(UT unsigned_value);
  void on_failure(std::exception_ptr ex);
    
private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<signed_reader_t, token_finder_t> finder_;
  subroutine_t<signed_reader_t, digits_reader_t<UT>> digits_reader_;

  bool negative_;
};

extern template struct signed_reader_t<short>;
extern template struct signed_reader_t<int>;
extern template struct signed_reader_t<long>;
extern template struct signed_reader_t<long long>;

} // detail

template<>
struct reader_traits_t<unsigned short>
{
  using type = detail::unsigned_reader_t<unsigned short>;
};

template<>
struct reader_traits_t<unsigned int>
{
  using type = detail::unsigned_reader_t<unsigned int>;
};

template<>
struct reader_traits_t<unsigned long>
{
  using type = detail::unsigned_reader_t<unsigned long>;
};

template<>
struct reader_traits_t<unsigned long long>
{
  using type = detail::unsigned_reader_t<unsigned long long>;
};

template<>
struct reader_traits_t<short>
{
  using type = detail::signed_reader_t<short>;
};

template<>
struct reader_traits_t<int>
{
  using type = detail::signed_reader_t<int>;
};

template<>
struct reader_traits_t<long>
{
  using type = detail::signed_reader_t<long>;
};

template<>
struct reader_traits_t<long long>
{
  using type = detail::signed_reader_t<long long>;
};

} // cuti

#endif

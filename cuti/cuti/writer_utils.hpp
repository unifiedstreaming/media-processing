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

#ifndef CUTI_WRITER_UTILS_HPP_
#define CUTI_WRITER_UTILS_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"

#include <cstddef>
#include <exception>
#include <type_traits>

namespace cuti
{

namespace detail
{

struct CUTI_ABI literal_writer_t
{
  using result_value_t = void;

  literal_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  literal_writer_t(literal_writer_t const&) = delete;
  literal_writer_t& operator=(literal_writer_t const&) = delete;

  template<std::size_t N>
  void start(char const (&literal)[N])
  {
    static_assert(N > 0);

    first_ = literal;
    last_ = literal + N - 1;

    this->write_chars();
  }

private :
  void write_chars();

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;

  char const* first_;
  char const* last_;
};

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

} // detail

} // cuti

#endif

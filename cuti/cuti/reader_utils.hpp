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

#ifndef CUTI_READER_UTILS_HPP_
#define CUTI_READER_UTILS_HPP_

#include "bound_inbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

namespace detail
{

struct CUTI_ABI token_finder_t
{
  using result_value_t = int;

  token_finder_t(result_t<int>& result, bound_inbuf_t& buf);

  token_finder_t(token_finder_t const&) = delete;
  token_finder_t& operator=(token_finder_t const&) = delete;

  /*
   * Skips whitespace and eventually submits the first non-whitespace
   * character from buf (which could be eof).  At that position in
   * buf, buf.readable() will be true and buf.peek() will equal the
   * submitted value.
   */
  void start();

private :
  result_t<int>& result_;
  bound_inbuf_t& buf_;
};

template<typename T>
struct CUTI_ABI digits_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = T;

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
struct CUTI_ABI chunk_reader_t
{
  static_assert(std::is_same_v<T, char> ||
                std::is_same_v<T, signed char> ||
                std::is_same_v<T, unsigned char>);

  static std::size_t constexpr max_chunksize = 256 * 1024;
  using result_value_t = std::size_t;

  chunk_reader_t(result_t<std::size_t>& result, bound_inbuf_t& buf);

  chunk_reader_t(chunk_reader_t const&) = delete;
  chunk_reader_t& operator=(chunk_reader_t const&) = delete;

  /*
   * Starts appending to target, eventually submitting the number of
   * bytes appended.
   */
  void start(std::vector<T>& target);
  
private :
  void read_lt(int c);
  void on_chunksize(std::size_t chunksize);
  void read_gt();
  void read_data();
  void on_exception(std::exception_ptr ex);

private :
  result_t<std::size_t>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<chunk_reader_t, token_finder_t> finder_;
  subroutine_t<chunk_reader_t, digits_reader_t<std::size_t>> digits_reader_;

  std::vector<T>* target_;
  char* first_;
  char* next_;
  char* last_;
};
  
extern template struct chunk_reader_t<char>;
extern template struct chunk_reader_t<signed char>;
extern template struct chunk_reader_t<unsigned char>;

} // detail

} // cuti

#endif

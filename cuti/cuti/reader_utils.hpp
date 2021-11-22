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
#include "charclass.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

namespace detail
{

struct CUTI_ABI whitespace_skipper_t
{
  using result_value_t = int;

  whitespace_skipper_t(result_t<int>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  { }

  whitespace_skipper_t(whitespace_skipper_t const&) = delete;
  whitespace_skipper_t& operator=(whitespace_skipper_t const&) = delete;

  /*
   * Skips whitespace and eventually submits the first non-whitespace
   * character from buf (which could be eof).  At that position in
   * buf, buf.readable() will be true and buf.peek() will equal the
   * submitted value.
   */
  void start()
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->start(); });
      return;
    }

    result_.submit(c);
  }

private :
  result_t<int>& result_;
  bound_inbuf_t& buf_;
};

template<int C>
struct sensor_t
{
  using result_value_t = bool;

  sensor_t(result_t<bool>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , skipper_(*this, result_, buf_)
  { }

  sensor_t(sensor_t const&) = delete;
  sensor_t& operator=(sensor_t const&) = delete;

  /*
   * Skips whitespace, eventually submitting true if C is found (and
   * skipped) or false if something else is found (and not skipped).
   */
  void start()
  {
    skipper_.start(&sensor_t::on_whitespace_skipped);
  }

private :
  void on_whitespace_skipped(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != C)
    {
      result_.submit(false);
      return;
    }

    if constexpr(C != eof)
    {
      buf_.skip();
    }

    result_.submit(true);
  }

private :
  result_t<bool>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<sensor_t, whitespace_skipper_t> skipper_;
};

template<int C>
struct expected_reader_t
{
  using result_value_t = void;

  expected_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , sensor_(*this, result_, buf)
  { }

  expected_reader_t(expected_reader_t const&) = delete;
  expected_reader_t& operator=(expected_reader_t const&) = delete;

  /*
   * Skips whitespace, checks for C, then submits with C skipped, or
   * fails.
   */
  void start()
  {
    sensor_.start(&expected_reader_t::on_sensor);
  }

private :
  void on_sensor(bool match)
  {
    if(!match)
    {
      if constexpr(C == eof)
      {
        result_.fail(parse_error_t("eof expected"));
      }
      else if constexpr(is_printable(C))
      {
        result_.fail(parse_error_t(std::string("\'") +
          static_cast<char>(C) + "\' expected"));
      }
      else
      {
        result_.fail(parse_error_t("char (" +
          std::to_string(C) + ") expected"));
      }
      return;
    }
      
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<expected_reader_t, sensor_t<C>> sensor_;
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

struct CUTI_ABI hex_digits_reader_t
{
  using result_value_t = int;

  hex_digits_reader_t(result_t<int>& result, bound_inbuf_t& buf);

  hex_digits_reader_t(hex_digits_reader_t const&) = delete;
  hex_digits_reader_t& operator=(hex_digits_reader_t const&) = delete;

  void start();

private :
  void read_digits();

private :
  result_t<int>& result_;
  bound_inbuf_t& buf_;
  int shift_;
  int value_;
};
    
template<typename T>
struct CUTI_ABI blob_reader_t
{
  static_assert(std::is_same_v<T, std::string> ||
                std::is_same_v<T, std::vector<char>> ||
                std::is_same_v<T, std::vector<signed char>> ||
                std::is_same_v<T, std::vector<unsigned char>>);

  using result_value_t = T;

  blob_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  blob_reader_t(blob_reader_t const&) = delete;
  blob_reader_t& operator=(blob_reader_t const&) = delete;

  void start();

private :
  void read_contents();
  void read_escaped();
  void on_hex_digits(int c);
  
private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<blob_reader_t, expected_reader_t<'\"'>> leading_dq_reader_;
  subroutine_t<blob_reader_t, hex_digits_reader_t> hex_digits_reader_;

  T value_;
};

extern template struct blob_reader_t<std::string>;
extern template struct blob_reader_t<std::vector<char>>;
extern template struct blob_reader_t<std::vector<signed char>>;
extern template struct blob_reader_t<std::vector<unsigned char>>;

} // detail

} // cuti

#endif

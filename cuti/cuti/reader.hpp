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

#ifndef CUTI_READER_HPP_
#define CUTI_READER_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace cuti
{

template<typename T>
struct reader_traits_t;

template<typename T>
using reader_t = typename reader_traits_t<T>::type;

namespace detail
{

template<typename T>
struct digits_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = T;

  digits_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , max_()
  , digit_seen_()
  , value_()
  { }

  digits_reader_t(digits_reader_t const&) = delete;
  digits_reader_t& operator=(digits_reader_t const&) = delete;

  void start(T max)
  {
    max_ = max;
    digit_seen_ = false;
    value_ = 0;

    this->read_digits();
  }

private :
  void read_digits()
  {
    int dval{};
    while(buf_.readable() && (dval = digit_value(buf_.peek())) >= 0)
    {
      digit_seen_ = true;

      T udval = static_cast<T>(dval);
      if(value_ > max_ / 10 || udval > max_ - 10 * value_)
      {
        result_.fail(parse_error_t("integer overflow"));
        return;
      }

      value_ *= 10;
      value_ += udval;

      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->read_digits(); });
      return;
    }

    if(!digit_seen_)
    {
      result_.fail(parse_error_t("digit expected"));
      return;
    }
  
    result_.submit(value_);
  }

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  T max_;
  bool digit_seen_;
  T value_;
};

template<typename T>
struct unsigned_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = T;

  unsigned_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , digits_reader_(*this, &unsigned_reader_t::on_failure, buf_)
  { }

  unsigned_reader_t(unsigned_reader_t const&) = delete;
  unsigned_reader_t& operator=(unsigned_reader_t const&) = delete;

  void start()
  {
    this->skip_spaces();
  }

private :
  void skip_spaces()
  {
    while(buf_.readable() && is_whitespace(buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->skip_spaces(); });
      return;
    }

    digits_reader_.start(
      &unsigned_reader_t::on_digits_read, std::numeric_limits<T>::max());
  }    

  void on_digits_read(T value)
  {
    result_.submit(value);
  }

  void on_failure(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<unsigned_reader_t, digits_reader_t<T>> digits_reader_;
};

template<typename T>
struct signed_reader_t
{
  static_assert(std::is_signed_v<T>);
  static_assert(std::is_integral_v<T>);

  using value_t = T;

  signed_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , negative_()
  , digits_reader_(*this, &signed_reader_t::on_failure, buf_)
  { }

  void start()
  {
    negative_ = false;
    this->skip_spaces_and_check_minus();
  }

private :
  using UT = std::make_unsigned_t<T>;

  void skip_spaces_and_check_minus()
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->skip_spaces_and_check_minus(); });
      return;
    }

    UT max = std::numeric_limits<T>::max();
    if(c == '-')
    {
      negative_ = true;
      ++max;
      buf_.skip();
    }

    digits_reader_.start(&signed_reader_t::on_digits_read, max);
  }

  void on_digits_read(UT unsigned_value)
  {
    T signed_value;

    if(!negative_ || unsigned_value == 0)
    {
      signed_value = unsigned_value;
    }
    else
    {
      --unsigned_value;
      signed_value = unsigned_value;
      signed_value = -signed_value;
      --signed_value;
    }

    result_.submit(signed_value);
  }

  void on_failure(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }
    
private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  bool negative_;
  subroutine_t<signed_reader_t, digits_reader_t<UT>> digits_reader_;
};

struct CUTI_ABI string_reader_t
{
  using value_t = std::string;

  string_reader_t(result_t<std::string>& result, bound_inbuf_t& buf);

  string_reader_t(string_reader_t const&) = delete;
  string_reader_t& operator=(string_reader_t const&) = delete;

  void start();

private :
  struct CUTI_ABI hex_digits_reader_t
  {
    using value_t = char;

    hex_digits_reader_t(result_t<char>& result, bound_inbuf_t& buf);

    hex_digits_reader_t(hex_digits_reader_t const&) = delete;
    hex_digits_reader_t& operator=(hex_digits_reader_t const&) = delete;

    void start();

  private :
    void read_digits();

  private :
    result_t<char>& result_;
    bound_inbuf_t& buf_;
    int shift_;
    char value_;
  };
    
  void read_opening_dq();
  void read_contents();
  void read_escaped();
  void on_char_value(char c);
  void on_exception(std::exception_ptr ex);
  
private :
  result_t<std::string>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<string_reader_t, hex_digits_reader_t> hex_digits_reader_;

  std::string value_;
  int recursion_;
};

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

template<>
struct reader_traits_t<std::string>
{
  using type = detail::string_reader_t;
};

} // cuti

#endif

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
#include "eof.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

template<typename T>
struct reader_traits_t;

template<typename T>
using reader_t = typename reader_traits_t<T>::type;

namespace detail
{

struct CUTI_ABI token_finder_t
{
  using value_t = int;

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

struct CUTI_ABI hex_digits_reader_t
{
  using value_t = int;

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

struct CUTI_ABI string_reader_t
{
  using value_t = std::string;

  string_reader_t(result_t<std::string>& result, bound_inbuf_t& buf);

  string_reader_t(string_reader_t const&) = delete;
  string_reader_t& operator=(string_reader_t const&) = delete;

  void start();

private :
  void on_begin_token(int c);
  void read_contents();
  void read_escaped();
  void on_hex_digits(int c);
  void on_exception(std::exception_ptr ex);
  
private :
  result_t<std::string>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<string_reader_t, token_finder_t> finder_;
  subroutine_t<string_reader_t, hex_digits_reader_t> hex_digits_reader_;

  std::string value_;
  int recursion_;
};

template<typename T>
struct vector_reader_t
{
  using value_t = std::vector<T>;

  vector_reader_t(result_t<std::vector<T>>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , finder_(*this, &vector_reader_t::on_exception, buf)
  , element_reader_(*this, &vector_reader_t::on_exception, buf)
  , value_()
  { }

  void start()
  {
    value_.clear();
    finder_.start(&vector_reader_t::on_begin_token);
  }

private :
  void on_begin_token(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != '[')
    {
      result_.fail(parse_error_t("\'[\' expected"));
      return;
    }
    buf_.skip();

    this->read_elements();
  }

  void read_elements()
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      /*
       * Direct whitespace skipping (not using a token finder) is OK
       * here because the element reader should use a token finder to
       * check for inline exceptions in buf_.
       */
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable([this] { this->read_elements(); });
      return;
    }

    switch(c)
    {
    case ']' :
      break;
    case eof :
    case '\n' :
      result_.fail(parse_error_t("missing \']\' at end of list"));
      return;
    default :
      element_reader_.start(&vector_reader_t::on_element_read);
      return;
    }
      
    buf_.skip();
    result_.submit(std::move(value_));
  }
    
  void on_element_read(T element)
  {
    value_.push_back(std::move(element));
    buf_.call_when_readable([this] { this->read_elements(); });
  }

  void on_exception(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<std::vector<T>>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<vector_reader_t, token_finder_t> finder_;
  subroutine_t<vector_reader_t, reader_t<T>> element_reader_;

  std::vector<T> value_;
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

template<typename T>
struct reader_traits_t<std::vector<T>>
{
  using type = detail::vector_reader_t<T>;
};

} // cuti

#endif

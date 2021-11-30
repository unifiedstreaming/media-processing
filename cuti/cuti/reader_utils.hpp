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
#include "flag.hpp"
#include "identifier.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "result.hpp"
#include "reader_traits.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

namespace detail
{

/*
 * whitespace_skipper: skips whitespace and eventually submits the
 * first non-whitespace character from buf (which could be eof).  At
 * that position in buf, buf.readable() will be true and buf.peek()
 * will equal the submitted value.
 *
 * Please note: to prevent stack overflow as a result of unbounded
 * tail recursion, any token reader MUST use a whitespace skipper as
 * its first step.
 */
struct CUTI_ABI whitespace_skipper_t
{
  using result_value_t = int;

  whitespace_skipper_t(result_t<int>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  { }

  whitespace_skipper_t(whitespace_skipper_t const&) = delete;
  whitespace_skipper_t& operator=(whitespace_skipper_t const&) = delete;

  void start()
  {
    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      this->skip_spaces();
      return;
    }

    buf_.call_when_readable([this] { this->skip_spaces(); });
  }

private :
  void skip_spaces()
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
struct expected_checker_t
{
  using result_value_t = bool;

  expected_checker_t(result_t<bool>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , skipper_(*this, result_, buf_)
  { }

  expected_checker_t(expected_checker_t const&) = delete;
  expected_checker_t& operator=(expected_checker_t const&) = delete;

  /*
   * Skips whitespace, eventually submitting true if C is found, and
   * false otherwise.  C is skipped if found.
   */
  void start()
  {
    skipper_.start(&expected_checker_t::on_whitespace_skipped);
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
  subroutine_t<expected_checker_t, whitespace_skipper_t> skipper_;
};

template<int C>
struct expected_reader_t
{
  using result_value_t = void;

  expected_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , skipper_(*this, result_, buf_)
  { }

  expected_reader_t(expected_reader_t const&) = delete;
  expected_reader_t& operator=(expected_reader_t const&) = delete;

  /*
   * Skips whitespace, checks for C, then submits with C skipped, or
   * fails.
   */
  void start()
  {
    skipper_.start(&expected_reader_t::on_whitespace_skipped);
  }

private :
  void on_whitespace_skipped(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != C)
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
        result_.fail(parse_error_t("char(" +
          std::to_string(C) + ") expected"));
      }
      return;
    }

    if constexpr(C != eof)
    {
      buf_.skip();
    }
      
    result_.submit();
  }

private :
  result_t<void>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<expected_reader_t, whitespace_skipper_t> skipper_;
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
struct CUTI_ABI boolean_reader_t
{
  using result_value_t = T;

  boolean_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  boolean_reader_t(boolean_reader_t const&) = delete;
  boolean_reader_t& operator=(boolean_reader_t const&) = delete;
  
  void start();

private :
  void on_whitespace_skipped(int c);

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<boolean_reader_t, whitespace_skipper_t> skipper_;
};

extern template struct boolean_reader_t<bool>;
extern template struct boolean_reader_t<flag_t>;

template<typename T>
struct CUTI_ABI unsigned_reader_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = T;

  unsigned_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  unsigned_reader_t(unsigned_reader_t const&) = delete;
  unsigned_reader_t& operator=(unsigned_reader_t const&) = delete;

  void start();

private :
  void on_whitespace_skipped(int c);
  void on_digits_read(T value);

private :
  result_t<T>& result_;
  subroutine_t<unsigned_reader_t, whitespace_skipper_t> skipper_;
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

  using result_value_t = T;

  signed_reader_t(result_t<T>& result, bound_inbuf_t& buf);

  signed_reader_t(signed_reader_t const&) = delete;
  signed_reader_t& operator=(signed_reader_t const&) = delete;

  void start();

private :
  using UT = std::make_unsigned_t<T>;

  void on_whitespace_skipped(int c);
  void on_digits_read(UT unsigned_value);

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<signed_reader_t, whitespace_skipper_t> skipper_;
  subroutine_t<signed_reader_t, digits_reader_t<UT>> digits_reader_;

  bool negative_;
};

extern template struct signed_reader_t<short>;
extern template struct signed_reader_t<int>;
extern template struct signed_reader_t<long>;
extern template struct signed_reader_t<long long>;

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
  void read_leading_dq(int c);
  void read_contents();
  void read_escaped();
  void on_hex_digits(int c);
  
private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<blob_reader_t, whitespace_skipper_t> skipper_;
  subroutine_t<blob_reader_t, hex_digits_reader_t> hex_digits_reader_;

  T value_;
};

extern template struct blob_reader_t<std::string>;
extern template struct blob_reader_t<std::vector<char>>;
extern template struct blob_reader_t<std::vector<signed char>>;
extern template struct blob_reader_t<std::vector<unsigned char>>;

struct CUTI_ABI identifier_reader_t
{
  using result_value_t = identifier_t;

  identifier_reader_t(result_t<identifier_t>& result, bound_inbuf_t& buf);

  identifier_reader_t(identifier_reader_t const&) = delete;
  identifier_reader_t& operator=(identifier_reader_t const&) = delete;

  void start();

private :
  void read_leader(int c);
  void read_followers();

private :
  result_t<identifier_t>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<identifier_reader_t, whitespace_skipper_t> skipper_;

  std::string rep_;
};
  
using begin_sequence_reader_t = expected_reader_t<'['>;
using end_sequence_checker_t = expected_checker_t<']'>;

template<typename T>
struct vector_reader_t
{
  using result_value_t = std::vector<T>;

  vector_reader_t(result_t<std::vector<T>>& result, bound_inbuf_t& buf)
  : result_(result)
  , begin_reader_(*this, result_, buf)
  , end_checker_(*this, result_, buf)
  , element_reader_(*this, result_, buf)
  , value_()
  { }

  vector_reader_t(vector_reader_t const&) = delete;
  vector_reader_t& operator=(vector_reader_t const&) = delete;
  
  void start()
  {
    value_.clear();
    begin_reader_.start(&vector_reader_t::read_elements);
  }

private :
  void read_elements()
  {
    end_checker_.start(&vector_reader_t::on_end_checker);
  }

  void on_end_checker(bool at_end)
  {
    if(at_end)
    {
      result_.submit(std::move(value_));
      return;
    }
      
    element_reader_.start(&vector_reader_t::on_element);
  }
    
  void on_element(T element)
  {
    value_.push_back(std::move(element));
    this->read_elements();
  }

private :
  result_t<std::vector<T>>& result_;
  subroutine_t<vector_reader_t, begin_sequence_reader_t> begin_reader_;
  subroutine_t<vector_reader_t, end_sequence_checker_t> end_checker_;
  subroutine_t<vector_reader_t, reader_t<T>> element_reader_;

  std::vector<T> value_;
};

template<typename T,
         typename = std::make_index_sequence<std::tuple_size_v<T>>>
struct tuple_elements_reader_t;

template<typename T>
struct tuple_elements_reader_t<T, std::index_sequence<>>
{
  using result_value_t = void;

  tuple_elements_reader_t(result_t<void>& result, bound_inbuf_t&)
  : result_(result)
  { }

  tuple_elements_reader_t(tuple_elements_reader_t const&) = delete;
  tuple_elements_reader_t& operator=(tuple_elements_reader_t const&) = delete;
  
  void start(T&)
  {
    result_.submit();
  }
  
private :
  result_t<void>& result_;
};

template<typename T, std::size_t First, std::size_t... Rest>
struct tuple_elements_reader_t<T, std::index_sequence<First, Rest...>>
{
  using result_value_t = void;

  using element_t = std::tuple_element_t<First, T>;
  using delegate_t = tuple_elements_reader_t<T, std::index_sequence<Rest...>>;

  tuple_elements_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , element_reader_(*this, result_, buf)
  , delegate_(*this, result_, buf)
  , value_()
  { }

  tuple_elements_reader_t(tuple_elements_reader_t const&) = delete;
  tuple_elements_reader_t& operator=(tuple_elements_reader_t const&) = delete;
  
  void start(T& value)
  {
    value_ = &value;
    element_reader_.start(&tuple_elements_reader_t::on_element_read);
  }

private :
  void on_element_read(element_t element)
  {
    std::get<First>(*value_) = std::move(element);
    delegate_.start(&tuple_elements_reader_t::on_delegate_done, *value_);
  }

  void on_delegate_done()
  {
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<tuple_elements_reader_t, reader_t<element_t>> element_reader_;
  subroutine_t<tuple_elements_reader_t, delegate_t> delegate_;

  T* value_;
};

using begin_structure_reader_t = expected_reader_t<'{'>;
using end_structure_reader_t = expected_reader_t<'}'>;

template<typename T>
struct tuple_reader_t
{
  using result_value_t = T;

  tuple_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , begin_reader_(*this, result_, buf)
  , elements_reader_(*this, result_, buf)
  , end_reader_(*this, result_, buf)
  , value_()
  { }

  tuple_reader_t(tuple_reader_t const&) = delete;
  tuple_reader_t& operator=(tuple_reader_t const&) = delete;

  void start()
  {
    begin_reader_.start(&tuple_reader_t::on_begin_read);
  }

private :
  void on_begin_read()
  {
    elements_reader_.start(&tuple_reader_t::on_elements_read, value_);
  }

  void on_elements_read()
  {
    end_reader_.start(&tuple_reader_t::on_end_read);
  }

  void on_end_read()
  {
    result_.submit(std::move(value_));
  }

private :
  result_t<T>& result_;
  subroutine_t<tuple_reader_t, begin_structure_reader_t> begin_reader_;
  subroutine_t<tuple_reader_t, tuple_elements_reader_t<T>> elements_reader_;
  subroutine_t<tuple_reader_t, end_structure_reader_t> end_reader_;

  T value_;
};

} // detail

} // cuti

#endif

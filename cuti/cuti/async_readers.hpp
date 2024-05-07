/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_ASYNC_READERS_HPP_
#define CUTI_ASYNC_READERS_HPP_

#include "bound_inbuf.hpp"
#include "charclass.hpp"
#include "consumer.hpp"
#include "enum_mapping.hpp"
#include "exception_builder.hpp"
#include "flag.hpp"
#include "identifier.hpp"
#include "linkage.h"
#include "parse_error.hpp"
#include "quoted.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "tuple_mapping.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

template<typename T>
struct reader_traits_t;

/*
 * Async readers; use reader_t<T> for a reader reading a T.
 */
template<typename T>
using reader_t = typename reader_traits_t<T>::type;

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
  , exception_handler_()
  { }

  whitespace_skipper_t(whitespace_skipper_t const&) = delete;
  whitespace_skipper_t& operator=(whitespace_skipper_t const&) = delete;

  void start(stack_marker_t& base_marker)
  {
    if(base_marker.in_range())
    {
      this->skip_spaces(base_marker);
      return;
    }

    buf_.call_when_readable(
      [this](stack_marker_t& marker) { this->skip_spaces(marker); }
    );
  }

private :
  void skip_spaces(stack_marker_t& base_marker)
  {
    int c{};
    while(buf_.readable() && is_whitespace(c = buf_.peek()))
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable(
        [this](stack_marker_t& marker) { this->start(marker); }
      );
      return;
    }

    if(c == '!')
    {
      this->start_exception_handler(base_marker);
      return;
    }

    result_.submit(base_marker, c);
  }

private :
  void start_exception_handler(stack_marker_t& base_marker);

private :
  struct exception_handler_t;

  struct CUTI_ABI exception_handler_deleter_t
  {
    void operator()(exception_handler_t* handler) const noexcept;
  };

  result_t<int>& result_;
  bound_inbuf_t& buf_;
  std::unique_ptr<exception_handler_t, exception_handler_deleter_t>
    exception_handler_;
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
  void start(stack_marker_t& base_marker)
  {
    skipper_.start(base_marker, &expected_checker_t::on_whitespace_skipped);
  }

private :
  void on_whitespace_skipped(stack_marker_t& base_marker, int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != C)
    {
      result_.submit(base_marker, false);
      return;
    }

    if constexpr(C != eof)
    {
      buf_.skip();
    }

    result_.submit(base_marker, true);
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
  void start(stack_marker_t& base_marker)
  {
    skipper_.start(base_marker, &expected_reader_t::on_whitespace_skipped);
  }

private :
  void on_whitespace_skipped(stack_marker_t& base_marker, int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != C)
    {
      exception_builder_t<parse_error_t> builder;
      builder << quoted_char(C) << " expected, but got " << quoted_char(c);
      result_.fail(base_marker, builder.exception_object());
      return;
    }

    if constexpr(C != eof)
    {
      buf_.skip();
    }
      
    result_.submit(base_marker);
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

  void start(stack_marker_t& base_marker, T max);

private :
  void read_digits(stack_marker_t& base_marker);

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

  void start(stack_marker_t& base_marker);

private :
  void read_digits(stack_marker_t& base_marker);

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
  
  void start(stack_marker_t& base_marker);

private :
  void on_whitespace_skipped(stack_marker_t& base_marker, int c);

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

  void start(stack_marker_t& base_marker);

private :
  void on_whitespace_skipped(stack_marker_t& base_marker, int c);
  void on_digits_read(stack_marker_t& base_marker, T value);

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

  void start(stack_marker_t& base_marker);

private :
  using UT = std::make_unsigned_t<T>;

  void on_whitespace_skipped(stack_marker_t& base_marker, int c);
  void on_digits_read(stack_marker_t& base_marker, UT unsigned_value);

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

  void start(stack_marker_t& base_marker);

private :
  void read_leading_dq(stack_marker_t& base_marker, int c);
  void read_contents(stack_marker_t& base_marker);
  void read_escaped(stack_marker_t& base_marker);
  void on_hex_digits(stack_marker_t& base_marker, int c);
  
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

  void start(stack_marker_t& base_marker);

private :
  void read_leader(stack_marker_t& base_marker, int c);
  void read_followers(stack_marker_t& base_marker);

private :
  result_t<identifier_t>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<identifier_reader_t, whitespace_skipper_t> skipper_;

  std::string wrapped_;
};
  
using begin_sequence_reader_t = expected_reader_t<'['>;
using end_sequence_checker_t = expected_checker_t<']'>;

template<typename T>
struct sequence_reader_t
{
  using result_value_t = void;

  sequence_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , begin_reader_(*this, result_, buf)
  , end_checker_(*this, result_, buf)
  , element_reader_(*this, result_, buf)
  , consumer_(nullptr)
  { }

  sequence_reader_t(sequence_reader_t const&) = delete;
  sequence_reader_t& operator=(sequence_reader_t const&) = delete;
  
  void start(stack_marker_t& base_marker, consumer_t<T>& consumer)
  {
    consumer_ = &consumer;
    begin_reader_.start(base_marker, &sequence_reader_t::read_elements);
  }

private :
  void read_elements(stack_marker_t& base_marker)
  {
    end_checker_.start(base_marker, &sequence_reader_t::on_end_checker);
  }

  void on_end_checker(stack_marker_t& base_marker, bool at_end)
  {
    assert(consumer_ != nullptr);

    if(at_end)
    {
      try
      {
        consumer_->put(std::nullopt);
      }
      catch(std::exception const&)
      {
        result_.fail(base_marker, std::current_exception());
        return;
      }
        
      consumer_ = nullptr;
      result_.submit(base_marker);
      return;
    }
      
    element_reader_.start(base_marker, &sequence_reader_t::on_element);
  }
    
  void on_element(stack_marker_t& base_marker, T element)
  {
    assert(consumer_ != nullptr);

    try
    {
      consumer_->put(std::make_optional(std::move(element)));
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }
        
    this->read_elements(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<sequence_reader_t, begin_sequence_reader_t> begin_reader_;
  subroutine_t<sequence_reader_t, end_sequence_checker_t> end_checker_;
  subroutine_t<sequence_reader_t, reader_t<T>> element_reader_;

  consumer_t<T>* consumer_;
};

template<typename T>
struct optional_consumer_t : consumer_t<T>
{
  optional_consumer_t()
  : value_(std::nullopt)
  { }

  void put(std::optional<T> element) override
  {
    if(element.has_value())
    {
      if(value_.has_value())
      {
        throw parse_error_t("duplicate optional value");
      }
      value_ = std::move(element);
    }
  }

  std::optional<T>& value()
  {
    return value_;
  }

private :
  std::optional<T> value_;
};
    
template<typename T>
struct optional_reader_t
{
  using result_value_t = std::optional<T>;

  optional_reader_t(result_t<std::optional<T>>& result, bound_inbuf_t& buf)
  : result_(result)
  , sequence_reader_(*this, result_, buf)
  , consumer_(std::nullopt)
  { }

  optional_reader_t(optional_reader_t const&) = delete;
  optional_reader_t& operator=(optional_reader_t const&) = delete;
  
  void start(stack_marker_t& base_marker)
  {
    consumer_.emplace();
    sequence_reader_.start(
      base_marker, &optional_reader_t::on_sequence_read, *consumer_);
  }

private :
  void on_sequence_read(stack_marker_t& base_marker)
  {
    assert(consumer_ != std::nullopt);
    auto value = std::move(consumer_->value());
    consumer_.reset();
    result_.submit(base_marker, std::move(value));
  }

private :
  result_t<std::optional<T>>& result_;
  subroutine_t<optional_reader_t, sequence_reader_t<T>> sequence_reader_;

  std::optional<optional_consumer_t<T>> consumer_;
};

template<typename T>
struct vector_consumer_t : consumer_t<T>
{
  vector_consumer_t()
  : value_()
  { }

  void put(std::optional<T> element) override
  {
    if(element != std::nullopt)
    {
      value_.push_back(std::move(*element));
    }
  }

  std::vector<T>& value()
  {
    return value_;
  }

private :
  std::vector<T> value_;
};

template<typename T>
struct vector_reader_t
{
  using result_value_t = std::vector<T>;

  vector_reader_t(result_t<std::vector<T>>& result, bound_inbuf_t& buf)
  : result_(result)
  , sequence_reader_(*this, result_, buf)
  , consumer_(std::nullopt)
  { }

  vector_reader_t(vector_reader_t const&) = delete;
  vector_reader_t& operator=(vector_reader_t const&) = delete;
  
  void start(stack_marker_t& base_marker)
  {
    consumer_.emplace();
    sequence_reader_.start(
      base_marker, &vector_reader_t::on_sequence_read, *consumer_);
  }

private :
  void on_sequence_read(stack_marker_t& base_marker)
  {
    assert(consumer_ != std::nullopt);
    auto value = std::move(consumer_->value());
    consumer_.reset();
    result_.submit(base_marker, std::move(value));
  }

private :
  result_t<std::vector<T>>& result_;
  subroutine_t<vector_reader_t, sequence_reader_t<T>> sequence_reader_;

  std::optional<vector_consumer_t<T>> consumer_;
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
  
  void start(stack_marker_t& base_marker, T& /* ignored */)
  {
    result_.submit(base_marker);
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
  
  void start(stack_marker_t& base_marker, T& value)
  {
    value_ = &value;
    element_reader_.start(
      base_marker, &tuple_elements_reader_t::on_element_read);
  }

private :
  void on_element_read(stack_marker_t& base_marker, element_t element)
  {
    std::get<First>(*value_) = std::move(element);
    delegate_.start(
      base_marker, &tuple_elements_reader_t::on_delegate_done, *value_);
  }

  void on_delegate_done(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
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

  void start(stack_marker_t& base_marker)
  {
    begin_reader_.start(base_marker, &tuple_reader_t::on_begin_read);
  }

private :
  void on_begin_read(stack_marker_t& base_marker)
  {
    elements_reader_.start(
      base_marker, &tuple_reader_t::on_elements_read, value_);
  }

  void on_elements_read(stack_marker_t& base_marker)
  {
    end_reader_.start(base_marker, &tuple_reader_t::on_end_read);
  }

  void on_end_read(stack_marker_t& base_marker)
  {
    result_.submit(base_marker, std::move(value_));
  }

private :
  result_t<T>& result_;
  subroutine_t<tuple_reader_t, begin_structure_reader_t> begin_reader_;
  subroutine_t<tuple_reader_t, tuple_elements_reader_t<T>> elements_reader_;
  subroutine_t<tuple_reader_t, end_structure_reader_t> end_reader_;

  T value_;
};

template<typename T>
struct enum_reader_t
{
  using result_value_t = T;

  static_assert(std::is_enum_v<T>);
  using wire_t = serialized_type_t<T>;
  using underlying_t = std::underlying_type_t<T>;

  enum_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , wire_reader_(*this, result_, buf)
  { }

  void start(stack_marker_t& base_marker)
  {
    wire_reader_.start(base_marker, &enum_reader_t::on_wire_value);
  }

private :
  void on_wire_value(stack_marker_t& base_marker, wire_t wire_value)
  {
    if(wire_value < std::numeric_limits<underlying_t>::min() ||
       wire_value > std::numeric_limits<underlying_t>::max())
    {
      exception_builder_t<parse_error_t> builder;
      builder << "on-the-wire value " << wire_value <<
        " cannot be represented in underlying type \'" <<
	typeid(underlying_t).name() << "\' of enum type '" <<
	typeid(T).name() << "\'";
      result_.fail(base_marker, builder.exception_object());
      return;
    }

    auto underlying_value = static_cast<underlying_t>(wire_value);

    T enum_value;
    try
    {
      enum_value = enum_mapping_t<T>::from_underlying(underlying_value);
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }

    result_.submit(base_marker, enum_value);
  }

private :
  result_t<T>& result_;
  subroutine_t<enum_reader_t, reader_t<wire_t>> wire_reader_;
};
  
template<typename T>
struct default_reader_t
{
  using result_value_t = T;
  using mapping_t = tuple_mapping_t<T>;
  using tuple_t = typename mapping_t::tuple_t;

  default_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , tuple_reader_(*this, result_, buf)
  { }

  default_reader_t(default_reader_t const&) = delete;
  default_reader_t& operator=(default_reader_t const&) = delete;

  void start(stack_marker_t& base_marker)
  {
    tuple_reader_.start(base_marker, &default_reader_t::on_tuple);
  }

private :
  void on_tuple(stack_marker_t& base_marker, tuple_t t)
  {
    std::optional<T> opt_value;
    try
    {
      opt_value.emplace(mapping_t::from_tuple(std::move(t)));
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }
      
    result_.submit(base_marker, std::move(*opt_value));
  }
    
private :
  result_t<T>& result_;
  subroutine_t<default_reader_t, tuple_reader_t<tuple_t>> tuple_reader_;
};

template<typename T, bool IsEnum = std::is_enum_v<T>>
struct user_type_reader_traits_t;

template<typename T>
struct user_type_reader_traits_t<T, true>
{
  static_assert(std::is_enum_v<T>);
  using type = enum_reader_t<T>;
};

template<typename T>
struct user_type_reader_traits_t<T, false>
{
  static_assert(!std::is_enum_v<T>);
  using type = default_reader_t<T>;
};

template<typename T>
using user_type_reader_t = typename user_type_reader_traits_t<T>::type;

struct CUTI_ABI eom_checker_t
{
  using result_value_t = void;

  eom_checker_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , skipper_(*this, result_, buf_)
  { }

  void start(stack_marker_t& base_marker)
  {
    skipper_.start(base_marker, &eom_checker_t::on_whitespace_skipped);
  }

private :
  void on_whitespace_skipped(stack_marker_t& base_marker, int c)
  {
    if(c != '\n')
    {
      exception_builder_t<parse_error_t> builder;
      builder << "end of message (" << quoted_char('\n') <<
        ") expected, but got " << quoted_char(c);
      result_.fail(base_marker, builder.exception_object());
      return;
    }

    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<eom_checker_t, whitespace_skipper_t> skipper_;
};

struct CUTI_ABI message_drainer_t
{
  using result_value_t = void;

  message_drainer_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  { }

  void start(stack_marker_t& base_marker)
  {
    if(base_marker.in_range())
    {
      this->drain(base_marker);
      return;
    }

    buf_.call_when_readable(
      [this](stack_marker_t& marker) { this->drain(marker); }
    );
  }

private :
  void drain(stack_marker_t& base_marker)
  {
    int c{};
    while(buf_.readable() && (c = buf_.peek()) != '\n' && c != eof)
    {
      buf_.skip();
    }

    if(!buf_.readable())
    {
      buf_.call_when_readable(
        [this](stack_marker_t& marker) { this->drain(marker); }
      );
      return;
    }

    if(c != eof)
    {
      buf_.skip();
    }

    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  bound_inbuf_t& buf_;
};

} // detail

template<>
struct reader_traits_t<bool>
{
  using type = detail::boolean_reader_t<bool>;
};

template<>
struct reader_traits_t<flag_t>
{
  using type = detail::boolean_reader_t<flag_t>;
};

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
  using type = detail::blob_reader_t<std::string>;
};

template<>
struct reader_traits_t<identifier_t>
{
  using type = detail::identifier_reader_t;
};

template<typename T>
struct reader_traits_t<std::optional<T>>
{
  using type = detail::optional_reader_t<T>;
};

template<typename T>
struct reader_traits_t<std::vector<T>>
{
  using type = detail::vector_reader_t<T>;
};

template<>
struct reader_traits_t<std::vector<char>>
{
  using type = detail::blob_reader_t<std::vector<char>>;
};

template<>
struct reader_traits_t<std::vector<signed char>>
{
  using type = detail::blob_reader_t<std::vector<signed char>>;
};

template<>
struct reader_traits_t<std::vector<unsigned char>>
{
  using type = detail::blob_reader_t<std::vector<unsigned char>>;
};

template<typename... Types>
struct reader_traits_t<std::tuple<Types...>>
{
  using type = detail::tuple_reader_t<std::tuple<Types...>>;
};

template<typename T1, typename T2>
struct reader_traits_t<std::pair<T1, T2>>
{
  using type = detail::tuple_reader_t<std::pair<T1, T2>>;
};

template<typename T, std::size_t N>
struct reader_traits_t<std::array<T, N>>
{
  using type = detail::tuple_reader_t<std::array<T, N>>;
};

template<typename T>
struct reader_traits_t
{
  using type = detail::user_type_reader_t<T>;
};

/*
 * Helpers for streaming async reading.
 */
using begin_sequence_reader_t = detail::begin_sequence_reader_t;
using end_sequence_checker_t = detail::end_sequence_checker_t;

template<typename T>
using sequence_reader_t = detail::sequence_reader_t<T>;

using begin_structure_reader_t = detail::begin_structure_reader_t;
using end_structure_reader_t = detail::end_structure_reader_t;

using eof_checker_t = detail::expected_checker_t<eof>;
using eom_checker_t = detail::eom_checker_t;
using message_drainer_t = detail::message_drainer_t;

} // cuti

#endif

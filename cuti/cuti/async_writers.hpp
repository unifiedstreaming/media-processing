/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include "bound_outbuf.hpp"
#include "enum_mapping.hpp"
#include "flag.hpp"
#include "flusher.hpp"
#include "identifier.hpp"
#include "linkage.h"
#include "producer.hpp"
#include "remote_error.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "tuple_mapping.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cuti
{

template<typename T>
struct writer_traits_t;

/*
 * Async writers: use writer_t<T> for a writer writing a T.
 */
template<typename T>
using writer_t = typename writer_traits_t<T>::type;

namespace detail
{

/*
 * token_suffix_writer: writes some fixed string literal.
 *
 * Please note: to prevent stack overflow as a result of unbounded
 * tail recursion, any token writer MUST use a suffix writer as its
 * last step.
 */
template<char const* Literal>
struct token_suffix_writer_t
{
  using result_value_t = void;

  token_suffix_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , p_()
  { }

  token_suffix_writer_t(token_suffix_writer_t const&) = delete;
  token_suffix_writer_t& operator=(token_suffix_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker)
  {
    p_ = Literal;

    if(base_marker.in_range())
    {
      this->write_chars(base_marker);
      return;
    }

    buf_.call_when_writable(
      [this](stack_marker_t& marker) { this->write_chars(marker); }
    );
  }

private :
  void write_chars(stack_marker_t& base_marker)
  {
    while(*p_ != '\0' && buf_.writable())
    {
      buf_.put(*p_);
      ++p_;
    }

    if(*p_ != '\0')
    {
      buf_.call_when_writable(
        [this](stack_marker_t& marker) { this->write_chars(marker); }
      );
      return;
    }

    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  char const* p_;
};

extern CUTI_ABI char const space_suffix[];

using space_writer_t = token_suffix_writer_t<space_suffix>;

template<typename T>
struct CUTI_ABI digits_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = void;

  digits_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  digits_writer_t(digits_writer_t const&) = delete;
  digits_writer_t& operator=(digits_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T value);

private :
  void write_digits(stack_marker_t& base_marker);

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

extern CUTI_ABI char const true_literal[];
extern CUTI_ABI char const false_literal[];

template<typename T>
struct CUTI_ABI boolean_writer_t
{
  using result_value_t = void;

  boolean_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  boolean_writer_t(boolean_writer_t const&) = delete;
  boolean_writer_t& operator=(boolean_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T value);

private :
  void on_done(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  subroutine_t<boolean_writer_t, token_suffix_writer_t<true_literal>>
    true_writer_;
  subroutine_t<boolean_writer_t, token_suffix_writer_t<false_literal>>
    false_writer_;
};

extern template struct boolean_writer_t<bool>;
extern template struct boolean_writer_t<flag_t>;

template<typename T>
struct CUTI_ABI unsigned_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using result_value_t = void;

  unsigned_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  unsigned_writer_t(unsigned_writer_t const&) = delete;
  unsigned_writer_t& operator=(unsigned_writer_t const&) = delete;

  void start(stack_marker_t& base_marker, T value);

private :
  void on_digits_written(stack_marker_t& base_marker);
  void on_space_written(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  subroutine_t<unsigned_writer_t, digits_writer_t<T>> digits_writer_;
  subroutine_t<unsigned_writer_t, space_writer_t> space_writer_;
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

  void start(stack_marker_t& base_marker, T value);

private :
  using UT = std::make_unsigned_t<T>;

  void write_minus(stack_marker_t& base_marker);
  void on_digits_written(stack_marker_t& base_marker);
  void on_space_written(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<signed_writer_t, digits_writer_t<UT>> digits_writer_;
  subroutine_t<signed_writer_t, space_writer_t> space_writer_;

  UT unsigned_value_;
};

extern template struct signed_writer_t<short>;
extern template struct signed_writer_t<int>;
extern template struct signed_writer_t<long>;
extern template struct signed_writer_t<long long>;

extern CUTI_ABI char const blob_suffix[];

template<typename T>
struct CUTI_ABI blob_writer_t
{
  static_assert(std::is_same_v<T, std::string> ||
                std::is_same_v<T, std::vector<char>> ||
                std::is_same_v<T, std::vector<signed char>> ||
                std::is_same_v<T, std::vector<unsigned char>>);

  using result_value_t = void;

  blob_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  blob_writer_t(blob_writer_t const&) = delete;
  blob_writer_t& operator=(blob_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T value);

private :
  void write_opening_dq(stack_marker_t& base_marker);
  void write_contents(stack_marker_t& base_marker);
  void write_escaped(stack_marker_t& base_marker);
  void on_suffix_written(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<blob_writer_t, token_suffix_writer_t<blob_suffix>>
    suffix_writer_;
  
  T value_;
  typename T::const_iterator first_;
  typename T::const_iterator last_;
};

extern template struct blob_writer_t<std::string>;
extern template struct blob_writer_t<std::vector<char>>;
extern template struct blob_writer_t<std::vector<signed char>>;
extern template struct blob_writer_t<std::vector<unsigned char>>;

struct CUTI_ABI identifier_writer_t
{
  using result_value_t = void;

  identifier_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  identifier_writer_t(identifier_writer_t const&) = delete;
  identifier_writer_t& operator=(identifier_writer_t const&) = delete;

  void start(stack_marker_t& base_marker, identifier_t value);

private :
  void write_contents(stack_marker_t& base_marker);
  void on_space_written(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<identifier_writer_t, space_writer_t> space_writer_;

  identifier_t value_;
  std::string::const_iterator begin_;
  std::string::const_iterator end_;
};
  
extern CUTI_ABI char const sequence_prefix[];
extern CUTI_ABI char const sequence_suffix[];

using begin_sequence_writer_t = token_suffix_writer_t<sequence_prefix>;
using end_sequence_writer_t = token_suffix_writer_t<sequence_suffix>;

template<typename T>
struct sequence_writer_t
{
  using result_value_t = void;

  sequence_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , begin_writer_(*this, result_, buf)
  , element_writer_(*this, result_, buf)
  , end_writer_(*this, result_, buf)
  , producer_(nullptr)
  { }

  sequence_writer_t(sequence_writer_t const&) = delete;
  sequence_writer_t& operator=(sequence_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, producer_t<T>& producer)
  {
    producer_ = &producer;
    begin_writer_.start(base_marker, &sequence_writer_t::write_elements);
  }

private :
  void write_elements(stack_marker_t& base_marker)
  {
    assert(producer_ != nullptr);

    std::optional<T> element;
    try
    {
      element = producer_->get();
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }
   
    if(element != std::nullopt)
    {
      element_writer_.start(
        base_marker, &sequence_writer_t::write_elements, std::move(*element));
      return;
    }

    producer_ = nullptr;
    end_writer_.start(base_marker, &sequence_writer_t::on_end_written);
  }
      
  void on_end_written(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<sequence_writer_t, begin_sequence_writer_t> begin_writer_;
  subroutine_t<sequence_writer_t, writer_t<T>> element_writer_;
  subroutine_t<sequence_writer_t, end_sequence_writer_t> end_writer_;

  producer_t<T>* producer_;
};

template<typename T>
struct optional_producer_t : producer_t<T>
{
  explicit optional_producer_t(std::optional<T> value)
  : value_(std::move(value))
  { }

  std::optional<T> get() override
  {
    auto result = std::move(value_);
    value_.reset();
    return result;
  }

private :
  std::optional<T> value_;
};
    
template<typename T>
struct optional_writer_t
{
  using result_value_t = void;

  optional_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , sequence_writer_(*this, result_, buf)
  , producer_(std::nullopt)
  { }

  optional_writer_t(optional_writer_t const&) = delete;
  optional_writer_t& operator=(optional_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, std::optional<T> value)
  {
    producer_.emplace(std::move(value));
    sequence_writer_.start(
      base_marker, &optional_writer_t::on_sequence_written, *producer_);
  }

private :
  void on_sequence_written(stack_marker_t& base_marker)
  {
    producer_.reset();
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<optional_writer_t, sequence_writer_t<T>> sequence_writer_;

  std::optional<optional_producer_t<T>> producer_;
};

template<typename T>
struct vector_producer_t : producer_t<T>
{
  explicit vector_producer_t(std::vector<T> value)
  : value_(std::move(value))
  , first_(value_.begin())
  , last_(value_.end())
  { }

  std::optional<T> get() override
  {
    if(first_ == last_)
    {
      return std::nullopt;
    }

    auto pos = first_;
    ++first_;
    return std::optional<T>(std::move(*pos));
  }

private :
  std::vector<T> value_;
  typename std::vector<T>::iterator first_;
  typename std::vector<T>::iterator last_;
};
    
template<typename T>
struct vector_writer_t
{
  using result_value_t = void;

  vector_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , sequence_writer_(*this, result_, buf)
  , producer_(std::nullopt)
  { }

  vector_writer_t(vector_writer_t const&) = delete;
  vector_writer_t& operator=(vector_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, std::vector<T> value)
  {
    producer_.emplace(std::move(value));
    sequence_writer_.start(
      base_marker, &vector_writer_t::on_sequence_written, *producer_);
  }

private :
  void on_sequence_written(stack_marker_t& base_marker)
  {
    producer_.reset();
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<vector_writer_t, sequence_writer_t<T>> sequence_writer_;

  std::optional<vector_producer_t<T>> producer_;
};

template<typename T,
         typename = std::make_index_sequence<std::tuple_size_v<T>>>
struct tuple_elements_writer_t;

template<typename T>
struct tuple_elements_writer_t<T, std::index_sequence<>>
{
  using result_value_t = void;

  tuple_elements_writer_t(result_t<void>& result, bound_outbuf_t&)
  : result_(result)
  { }

  tuple_elements_writer_t(tuple_elements_writer_t const&) = delete;
  tuple_elements_writer_t& operator=(tuple_elements_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T& /* ignored */)
  {
    result_.submit(base_marker);
  }
  
private :
  result_t<void>& result_;
};

template<typename T, std::size_t First, std::size_t... Rest>
struct tuple_elements_writer_t<T, std::index_sequence<First, Rest...>>
{
  using result_value_t = void;

  using element_t = std::tuple_element_t<First, T>;
  using delegate_t = tuple_elements_writer_t<T, std::index_sequence<Rest...>>;

  tuple_elements_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , element_writer_(*this, result_, buf_)
  , delegate_(*this, result_, buf_)
  , value_()
  { }

  tuple_elements_writer_t(tuple_elements_writer_t const&) = delete;
  tuple_elements_writer_t& operator=(tuple_elements_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T& value)
  {
    value_ = &value;
    element_writer_.start(
      base_marker,
      &tuple_elements_writer_t::on_element_written,
      std::move(std::get<First>(*value_))
    );
  }

private :
  void on_element_written(stack_marker_t& base_marker)
  {
    delegate_.start(
      base_marker, &tuple_elements_writer_t::on_delegate_done, *value_);
  }

  void on_delegate_done(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<tuple_elements_writer_t, writer_t<element_t>> element_writer_;
  subroutine_t<tuple_elements_writer_t, delegate_t> delegate_;

  T* value_;
};

extern CUTI_ABI char const structure_prefix[];
extern CUTI_ABI char const structure_suffix[];

using begin_structure_writer_t = token_suffix_writer_t<structure_prefix>;
using end_structure_writer_t = token_suffix_writer_t<structure_suffix>;

template<typename T>
struct tuple_writer_t
{
  using result_value_t = void;

  tuple_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , prefix_writer_(*this, result_, buf)
  , elements_writer_(*this, result_, buf)
  , suffix_writer_(*this, result_, buf)
  , value_()
  { }

  tuple_writer_t(tuple_writer_t const&) = delete;
  tuple_writer_t& operator=(tuple_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, T value)
  {
    value_ = std::move(value);
    prefix_writer_.start(base_marker, &tuple_writer_t::on_prefix_written);
  }

private :
  void on_prefix_written(stack_marker_t& base_marker)
  {
    elements_writer_.start(
      base_marker, &tuple_writer_t::on_elements_written, value_);
  }
    
  void on_elements_written(stack_marker_t& base_marker)
  {
    suffix_writer_.start(base_marker, &tuple_writer_t::on_suffix_written);
  }

  void on_suffix_written(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<tuple_writer_t, begin_structure_writer_t> prefix_writer_;
  subroutine_t<tuple_writer_t, tuple_elements_writer_t<T>> elements_writer_;
  subroutine_t<tuple_writer_t, end_structure_writer_t> suffix_writer_;

  T value_;
};

template<typename T>
struct enum_writer_t
{
  using result_value_t = void;

  static_assert(std::is_enum_v<T>);
  using wire_t = serialized_type_t<T>;

  enum_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , wire_writer_(*this, result_, buf)
  { }

  void start(stack_marker_t& base_marker, T value)
  {
    wire_writer_.start(base_marker,
                       &enum_writer_t::on_wire_writer_done,
                       to_serialized(value));
  }

private :
  void on_wire_writer_done(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<enum_writer_t, writer_t<wire_t>> wire_writer_;
};
  
template<typename T>
struct default_writer_t
{
  using result_value_t = void;
  using mapping_t = tuple_mapping_t<T>;
  using tuple_t = typename mapping_t::tuple_t;

  default_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , tuple_writer_(*this, result_, buf)
  { }
  
  default_writer_t(default_writer_t const&) = delete;
  default_writer_t& operator=(default_writer_t const&) = delete;
  
  template<typename TT>
  void start(stack_marker_t& base_marker, TT&& value)
  {
    tuple_writer_.start(
      base_marker,
      &default_writer_t::on_tuple_writer_done,
      mapping_t::to_tuple(std::forward<TT>(value)));
  }

private :
  void on_tuple_writer_done(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }
    
private :
  result_t<void>& result_;
  subroutine_t<default_writer_t, tuple_writer_t<tuple_t>> tuple_writer_;
};

template<typename T, bool IsEnum = std::is_enum_v<T>>
struct user_type_writer_traits_t;

template<typename T>
struct user_type_writer_traits_t<T, true>
{
  static_assert(std::is_enum_v<T>);
  using type = enum_writer_t<T>;
};

template<typename T>
struct user_type_writer_traits_t<T, false>
{
  static_assert(!std::is_enum_v<T>);
  using type = default_writer_t<T>;
};

template<typename T>
using user_type_writer_t = typename user_type_writer_traits_t<T>::type;

struct CUTI_ABI exception_writer_t
{
  using result_value_t = void;

  exception_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  exception_writer_t(exception_writer_t const&) = delete;
  exception_writer_t& operator=(exception_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, remote_error_t error);

  ~exception_writer_t();

private :
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

extern CUTI_ABI char const newline[];

struct CUTI_ABI eom_writer_t
{
  using result_value_t = void;

  eom_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  eom_writer_t(eom_writer_t const&) = delete;
  eom_writer_t& operator=(eom_writer_t const&) = delete;

  void start(stack_marker_t& base_marker);

private :
  void on_newline_written(stack_marker_t& base_marker);
  void on_flushed(stack_marker_t& base_marker);

private :
  result_t<void>& result_;
  subroutine_t<eom_writer_t, token_suffix_writer_t<newline>> newline_writer_;
  subroutine_t<eom_writer_t, flusher_t> flusher_;
};
  
} // detail

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
struct writer_traits_t<std::optional<T>>
{
  using type = detail::optional_writer_t<T>;
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
 * Helpers for streaming async writing.
 */
using begin_sequence_writer_t = detail::begin_sequence_writer_t;
using end_sequence_writer_t = detail::end_sequence_writer_t;

template<typename T>
using sequence_writer_t = detail::sequence_writer_t<T>;

using begin_structure_writer_t = detail::begin_structure_writer_t;
using end_structure_writer_t = detail::end_structure_writer_t;

using exception_writer_t = detail::exception_writer_t;
using eom_writer_t = detail::eom_writer_t;

} // cuti

#endif

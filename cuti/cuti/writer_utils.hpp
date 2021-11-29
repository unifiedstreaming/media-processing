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
#include "flag.hpp"
#include "linkage.h"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <tuple>
#include <vector>

namespace cuti
{

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
  
  void start()
  {
    p_ = Literal;

    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      this->write_chars();
      return;
    }

    buf_.call_when_writable([this] { this->write_chars(); });
  }

private :
  void write_chars()
  {
    while(*p_ != '\0' && buf_.writable())
    {
      buf_.put(*p_);
      ++p_;
    }

    if(*p_ != '\0')
    {
      buf_.call_when_writable([this] { this->write_chars(); });
      return;
    }

    result_.submit();
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

extern CUTI_ABI char const true_literal[];
extern CUTI_ABI char const false_literal[];

template<typename T>
struct CUTI_ABI boolean_writer_t
{
  using result_value_t = void;

  boolean_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  boolean_writer_t(boolean_writer_t const&) = delete;
  boolean_writer_t& operator=(boolean_writer_t const&) = delete;
  
  void start(T value);

private :
  void on_done();

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

  void start(T value);

private :
  void on_digits_written();
  void on_space_written();

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

  void start(T value);

private :
  using UT = std::make_unsigned_t<T>;

  void write_minus();
  void on_digits_written();
  void on_space_written();

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
  
  void start(T value);

private :
  void write_opening_dq();
  void write_contents();
  void write_escaped();
  void on_suffix_written();

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

extern CUTI_ABI char const sequence_prefix[];
extern CUTI_ABI char const sequence_suffix[];

using begin_sequence_writer_t = token_suffix_writer_t<sequence_prefix>;
using end_sequence_writer_t = token_suffix_writer_t<sequence_suffix>;

template<typename T>
struct vector_writer_t
{
  using result_value_t = void;

  vector_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , begin_writer_(*this, result_, buf)
  , element_writer_(*this, result_, buf)
  , end_writer_(*this, result_, buf)
  , value_()
  , first_()
  , last_()
  { }

  vector_writer_t(vector_writer_t const&) = delete;
  vector_writer_t& operator=(vector_writer_t const&) = delete;
  
  void start(std::vector<T> value)
  {
    value_ = std::move(value);
    first_ = value_.begin();
    last_ = value_.end();

    begin_writer_.start(&vector_writer_t::write_elements);
  }

private :
  void write_elements()
  {
    if(first_ != last_)
    {
      auto pos = first_;
      ++first_;
      element_writer_.start(
        &vector_writer_t::write_elements, std::move(*pos));
      return;
    }

    end_writer_.start(&vector_writer_t::on_end_written);
  }
      
  void on_end_written()
  {
    value_.clear();
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<vector_writer_t, begin_sequence_writer_t> begin_writer_;
  subroutine_t<vector_writer_t, writer_t<T>> element_writer_;
  subroutine_t<vector_writer_t, end_sequence_writer_t> end_writer_;

  std::vector<T> value_;
  typename std::vector<T>::iterator first_;
  typename std::vector<T>::iterator last_;
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
  
  void start(T&)
  {
    result_.submit();
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
  
  void start(T& value)
  {
    value_ = &value;
    element_writer_.start(&tuple_elements_writer_t::on_element_written,
                          std::move(std::get<First>(*value_)));
  }

private :
  void on_element_written()
  {
    delegate_.start(&tuple_elements_writer_t::on_delegate_done, *value_);
  }

  void on_delegate_done()
  {
    result_.submit();
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
  
  void start(T value)
  {
    value_ = std::move(value);
    prefix_writer_.start(&tuple_writer_t::on_prefix_written);
  }

private :
  void on_prefix_written()
  {
    elements_writer_.start(&tuple_writer_t::on_elements_written, value_);
  }
    
  void on_elements_written()
  {
    suffix_writer_.start(&tuple_writer_t::on_suffix_written);
  }

  void on_suffix_written()
  {
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<tuple_writer_t, begin_structure_writer_t> prefix_writer_;
  subroutine_t<tuple_writer_t, tuple_elements_writer_t<T>> elements_writer_;
  subroutine_t<tuple_writer_t, end_structure_writer_t> suffix_writer_;

  T value_;
};
    
} // detail

} // cuti

#endif

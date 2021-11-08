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

#ifndef CUTI_WRITER_HPP_
#define CUTI_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "subroutine.hpp"

#include <type_traits>
#include <string>
#include <vector>
#include <utility>

namespace cuti
{

template<typename T>
struct writer_traits_t;

template<typename T>
using writer_t = typename writer_traits_t<T>::type;

namespace detail
{

template<typename T>
struct CUTI_ABI digits_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = void;

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

struct CUTI_ABI hex_digits_writer_t
{
  using value_t = void;

  hex_digits_writer_t(result_t<void>& result_, bound_outbuf_t& buf);

  hex_digits_writer_t(hex_digits_writer_t const&) = delete;
  hex_digits_writer_t& operator=(hex_digits_writer_t const&) = delete;

  void start(int value);

private :
  void write_digits();

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;

  int value_;
  int shift_;
};
    
template<typename T>
struct CUTI_ABI unsigned_writer_t
{
  static_assert(std::is_unsigned_v<T>);

  using value_t = void;

  unsigned_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  unsigned_writer_t(unsigned_writer_t const&) = delete;
  unsigned_writer_t& operator=(unsigned_writer_t const&) = delete;

  void start(T value);

private :
  void write_space();
  void on_digits_written();
  void on_failure(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<unsigned_writer_t, digits_writer_t<T>> digits_writer_;
  T value_;
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

  using value_t = void;

  signed_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  signed_writer_t(signed_writer_t const&) = delete;
  signed_writer_t& operator=(signed_writer_t const&) = delete;

  void start(T value);

private :
  using UT = std::make_unsigned_t<T>;

  void write_space();
  void write_minus();
  void on_digits_written();
  void on_failure(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<signed_writer_t, digits_writer_t<UT>> digits_writer_;
  T value_;
};

extern template struct signed_writer_t<short>;
extern template struct signed_writer_t<int>;
extern template struct signed_writer_t<long>;
extern template struct signed_writer_t<long long>;

struct CUTI_ABI string_writer_t
{
  using value_t = void;

  string_writer_t(result_t<void>& result, bound_outbuf_t& buf);

  string_writer_t(string_writer_t const&) = delete;
  string_writer_t& operator=(string_writer_t const&) = delete;
  
  void start(std::string value);

private :
  void write_space();
  void write_opening_dq();
  void write_contents();
  void write_escaped();
  void write_closing_dq();
  void on_hex_digits_written();
  void on_exception(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<string_writer_t, hex_digits_writer_t> hex_digits_writer_;
  
  std::string value_;
  char const* rp_;
  char const* ep_;
};

template<typename T>
struct vector_writer_t
{
  using value_t = void;

  vector_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , element_writer_(*this, &vector_writer_t::on_exception, buf_)
  , value_()
  , rp_()
  , ep_()
  { }

  vector_writer_t(vector_writer_t const&) = delete;
  vector_writer_t& operator=(vector_writer_t const&) = delete;
  
  void start(std::vector<T> value)
  {
    value_ = std::move(value);
    rp_ = value_.data();
    ep_ = rp_ + value_.size();

    this->write_opening_space();
  }

private :
  void write_opening_space()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_opening_space(); });
      return;
    }
    buf_.put(' ');

    this->write_opening_bracket();
  }

  void write_opening_bracket()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_opening_bracket(); });
      return;
    }
    buf_.put('[');

    this->write_elements();
  }

  void write_elements()
  {
    if(rp_ != ep_)
    {
      T& elem = *rp_;
      ++rp_;
      element_writer_.start(
        &vector_writer_t::on_element_written, std::move(elem));
      return;
    }

    this->write_closing_space();
  }
      
  void write_closing_space()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_closing_space(); });
      return;
    }
    buf_.put(' ');

    this->write_closing_bracket();
  }

  void write_closing_bracket()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_closing_bracket(); });
      return;
    }
    buf_.put(']');

    value_.clear();
    result_.submit();
  }

  void on_element_written()
  {
    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      this->write_elements();
      return;
    }

    buf_.call_when_writable([this] { this->write_elements(); });
  }

  void on_exception(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<vector_writer_t, writer_t<T>> element_writer_;

  std::vector<T> value_;
  T* rp_;
  T* ep_;
};

} // detail

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
  using type = detail::string_writer_t;
};

template<typename T>
struct writer_traits_t<std::vector<T>>
{
  using type = detail::vector_writer_t<T>;
};

} // cuti

#endif

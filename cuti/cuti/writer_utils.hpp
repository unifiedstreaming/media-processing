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
#include <string>
#include <type_traits>
#include <vector>

namespace cuti
{

namespace detail
{

template<char const* Literal>
struct literal_writer_t
{
  using result_value_t = void;

  literal_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , p_()
  { }

  literal_writer_t(literal_writer_t const&) = delete;
  literal_writer_t& operator=(literal_writer_t const&) = delete;
  
  void start()
  {
    p_ = Literal;
    this->write_chars();
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

extern CUTI_ABI char const blob_prefix[];
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
  void write_contents();
  void write_escaped();
  void on_suffix_written();
  void on_exception(std::exception_ptr ex);

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<blob_writer_t, literal_writer_t<blob_prefix>> prefix_writer_;
  subroutine_t<blob_writer_t, literal_writer_t<blob_suffix>> suffix_writer_;
  
  T value_;
  typename T::const_iterator first_;
  typename T::const_iterator last_;
};

extern template struct blob_writer_t<std::string>;
extern template struct blob_writer_t<std::vector<char>>;
extern template struct blob_writer_t<std::vector<signed char>>;
extern template struct blob_writer_t<std::vector<unsigned char>>;

} // detail

} // cuti

#endif
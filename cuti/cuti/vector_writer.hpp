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

#ifndef CUTI_VECTOR_WRITER_HPP_
#define CUTI_VECTOR_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "linkage.h"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <cstddef>
#include <exception>
#include <utility>
#include <vector>

namespace cuti
{

namespace detail
{

template<typename T>
struct vector_writer_t
{
  using result_value_t = void;

  vector_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , literal_writer_(*this, &vector_writer_t::on_exception, buf_)
  , element_writer_(*this, &vector_writer_t::on_exception, buf_)
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

    literal_writer_.start(&vector_writer_t::write_elements, " [");
  }

private :
  void write_elements()
  {
    if(first_ != last_)
    {
      T elem = std::move(*first_);
      ++first_;
      element_writer_.start(
        &vector_writer_t::on_element_written, std::move(elem));
      return;
    }

    literal_writer_.start(&vector_writer_t::on_suffix_written, " ]");
  }
      
  void on_suffix_written()
  {
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
  subroutine_t<vector_writer_t, literal_writer_t> literal_writer_;
  subroutine_t<vector_writer_t, writer_t<T>> element_writer_;

  std::vector<T> value_;
  typename std::vector<T>::iterator first_;
  typename std::vector<T>::iterator last_;
};

} // detail

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

} // cuti

#endif

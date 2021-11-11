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
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"

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
  using value_t = void;

  vector_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , element_writer_(*this, &vector_writer_t::on_exception, buf_)
  , value_()
  , ri_()
  , ei_()
  { }

  vector_writer_t(vector_writer_t const&) = delete;
  vector_writer_t& operator=(vector_writer_t const&) = delete;
  
  void start(std::vector<T> value)
  {
    value_ = std::move(value);
    ri_ = value_.begin();
    ei_ = value_.end();

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
    if(ri_ != ei_)
    {
      T elem = std::move(*ri_);
      ++ri_;
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
  typename std::vector<T>::iterator ri_;
  typename std::vector<T>::iterator ei_;
};

} // detail

template<typename T>
struct writer_traits_t<std::vector<T>>
{
  using type = detail::vector_writer_t<T>;
};

} // cuti

#endif

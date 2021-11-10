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

#ifndef CUTI_TUPLE_WRITER_HPP_
#define CUTI_TUPLE_WRITER_HPP_

#include "bound_outbuf.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "writer_traits.hpp"

#include <cstddef>
#include <exception>
#include <tuple>
#include <utility>

namespace cuti
{

namespace detail
{

template<typename T, std::size_t Remaining = std::tuple_size_v<T>>
struct tuple_elements_writer_t;

template<typename T>
struct tuple_elements_writer_t<T, 0>
{
  using value_t = void;

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

template<typename T, std::size_t Remaining>
struct tuple_elements_writer_t
{
  using value_t = void;

  static std::size_t constexpr index = std::tuple_size_v<T> - Remaining;
  using element_t = std::tuple_element_t<index, T>;
  using delegate_t = tuple_elements_writer_t<T, Remaining - 1>;

  tuple_elements_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , element_writer_(*this, &tuple_elements_writer_t::on_exception, buf_)
  , delegate_(*this, &tuple_elements_writer_t::on_exception, buf_)
  , value_()
  { }

  tuple_elements_writer_t(tuple_elements_writer_t const&) = delete;
  tuple_elements_writer_t& operator=(tuple_elements_writer_t const&) = delete;
  
  void start(T& value)
  {
    value_ = &value;
    element_writer_.start(&tuple_elements_writer_t::on_element_written,
                          std::move(std::get<index>(*value_)));
  }

private :
  void on_element_written()
  {
    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      delegate_.start(
        &tuple_elements_writer_t::on_delegate_done, *value_);
      return;
    }

    buf_.call_when_writable([this]
    {
      this->delegate_.start(
        &tuple_elements_writer_t::on_delegate_done, *this->value_);
    });
  }

  void on_delegate_done()
  {
    result_.submit();
  }

  void on_exception(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<tuple_elements_writer_t, writer_t<element_t>> element_writer_;
  subroutine_t<tuple_elements_writer_t, delegate_t> delegate_;

  T* value_;
};

template<typename T>
struct tuple_writer_t
{
  using value_t = void;

  tuple_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , elements_writer_(*this, &tuple_writer_t::on_exception, buf_)
  , value_()
  { }

  tuple_writer_t(tuple_writer_t const&) = delete;
  tuple_writer_t& operator=(tuple_writer_t const&) = delete;
  
  void start(T value)
  {
    value_ = std::move(value);
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

    this->write_opening_curly();
  }

  void write_opening_curly()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_opening_curly(); });
      return;
    }
    buf_.put('{');

    elements_writer_.start(&tuple_writer_t::write_closing_space, value_);
  }
    
  void write_closing_space()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_closing_space(); });
      return;
    }
    buf_.put(' ');

    this->write_closing_curly();
  }

  void write_closing_curly()
  {
    if(!buf_.writable())
    {
      buf_.call_when_writable([this] { this->write_closing_curly(); });
      return;
    }
    buf_.put('}');

    result_.submit();
  }

  void on_exception(std::exception_ptr ex)
  {
    result_.fail(std::move(ex));
  }

private :
  result_t<void>& result_;
  bound_outbuf_t& buf_;
  subroutine_t<tuple_writer_t, tuple_elements_writer_t<T>> elements_writer_;

  T value_;
};
    
} // detail

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

} // cuti

#endif

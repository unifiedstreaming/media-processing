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

#ifndef CUTI_TUPLE_READER_HPP_
#define CUTI_TUPLE_READER_HPP_

#include "bound_inbuf.hpp"
#include "parse_error.hpp"
#include "reader_traits.hpp"
#include "reader_utils.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <cstddef>
#include <tuple>
#include <utility>

namespace cuti
{

namespace detail
{

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
  , buf_(buf)
  , element_reader_(*this, result_, buf_)
  , delegate_(*this, result_, buf_)
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

    stack_marker_t marker;
    if(marker.in_range(buf_.base_marker()))
    {
      delegate_.start(
        &tuple_elements_reader_t::on_delegate_done, *value_);
      return;
    }

    buf_.call_when_readable([this]
    {
      this->delegate_.start(
        &tuple_elements_reader_t::on_delegate_done, *this->value_);
    });
  }

  void on_delegate_done()
  {
    result_.submit();
  }

private :
  result_t<void>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<tuple_elements_reader_t, reader_t<element_t>> element_reader_;
  subroutine_t<tuple_elements_reader_t, delegate_t> delegate_;

  T* value_;
};

template<typename T>
struct tuple_reader_t
{
  using result_value_t = T;

  tuple_reader_t(result_t<T>& result, bound_inbuf_t& buf)
  : result_(result)
  , buf_(buf)
  , finder_(*this, result_, buf_)
  , elements_reader_(*this, result_, buf_)
  , value_()
  { }

  tuple_reader_t(tuple_reader_t const&) = delete;
  tuple_reader_t& operator=(tuple_reader_t const&) = delete;

  void start()
  {
    finder_.start(&tuple_reader_t::on_first_token);
  }

private :
  void on_first_token(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != '{')
    {
      result_.fail(parse_error_t("\'{\' expected"));
      return;
    }
    buf_.skip();

    elements_reader_.start(&tuple_reader_t::on_elements_read, value_);
  }

  void on_elements_read()
  {
    finder_.start(&tuple_reader_t::on_last_token);
  }

  void on_last_token(int c)
  {
    assert(buf_.readable());
    assert(buf_.peek() == c);

    if(c != '}')
    {
      result_.fail(parse_error_t("\'}\' expected"));
      return;
    }
    buf_.skip();

    result_.submit(std::move(value_));
  }

private :
  result_t<T>& result_;
  bound_inbuf_t& buf_;
  subroutine_t<tuple_reader_t, token_finder_t> finder_;
  subroutine_t<tuple_reader_t, tuple_elements_reader_t<T>> elements_reader_;

  T value_;
};

} // detail

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

} // cuti

#endif

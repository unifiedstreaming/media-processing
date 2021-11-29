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
#include "subroutine.hpp"
#include "structure_writer.hpp"
#include "writer_traits.hpp"
#include "writer_utils.hpp"

#include <cstddef>
#include <tuple>
#include <utility>

namespace cuti
{

namespace detail
{

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

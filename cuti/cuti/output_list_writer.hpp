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

#ifndef CUTI_OUTPUT_LIST_WRITER_HPP_
#define CUTI_OUTPUT_LIST_WRITER_HPP_

#include "async_writers.hpp"
#include "bound_outbuf.hpp"
#include "linkage.h"
#include "output_list.hpp"
#include "result.hpp"
#include "sequence.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <exception>
#include <optional>

namespace cuti
{

template<typename Value>
struct output_writer_t
{
  using result_value_t = void;

  output_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , value_writer_(*this, result_, buf)
  { }

  output_writer_t(output_writer_t const&) = delete;
  output_writer_t& operator=(output_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, output_t<Value>& output)
  {
    std::optional<Value> value;
    
    try
    {
      value.emplace(output.get());
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }

    value_writer_.start(
      base_marker, &output_writer_t::on_value_written, std::move(*value));
  }

private :
  void on_value_written(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<output_writer_t, writer_t<Value>> value_writer_;
};

template<typename Value>
struct output_writer_t<sequence_t<Value>>
{
  using result_value_t = void;

  output_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , sequence_writer_(*this, result_, buf)
  { }

  output_writer_t(output_writer_t const&) = delete;
  output_writer_t& operator=(output_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker,
             output_t<sequence_t<Value>>& value)
  {
    sequence_writer_.start(
      base_marker, &output_writer_t::on_sequence_written, value);
  }

private :
  void on_sequence_written(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<output_writer_t, sequence_writer_t<Value>> sequence_writer_;
};

template<typename... Values>
struct output_list_writer_t;

template<>
struct CUTI_ABI output_list_writer_t<>
{
  using result_value_t = void;

  output_list_writer_t(result_t<void>& result, bound_outbuf_t& /* ignored */)
  : result_(result)
  { }

  output_list_writer_t(output_list_writer_t const&) = delete;
  output_list_writer_t& operator=(output_list_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker, output_list_t<>& /* ignored */)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
};

template<typename FirstValue, typename... OtherValues>
struct output_list_writer_t<FirstValue, OtherValues...>
{
  using result_value_t = void;

  output_list_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , first_writer_(*this, result_, buf)
  , others_writer_(*this, result_, buf)
  , outputs_(nullptr)
  { }

  output_list_writer_t(output_list_writer_t const&) = delete;
  output_list_writer_t& operator=(output_list_writer_t const&) = delete;
  
  void start(stack_marker_t& base_marker,
             output_list_t<FirstValue, OtherValues...>& outputs)
  {
    outputs_ = &outputs;
    first_writer_.start(
      base_marker,
      &output_list_writer_t::on_first_written, outputs_->first());
  }

private :
  void on_first_written(stack_marker_t& base_marker)
  {
    assert(outputs_ != nullptr);
    others_writer_.start(
      base_marker,
      &output_list_writer_t::on_others_written, outputs_->others());
  }

  void on_others_written(stack_marker_t& base_marker)
  {
    outputs_ = nullptr;
    result_.submit(base_marker);
  }
    
private :
  result_t<void>& result_;
  subroutine_t<output_list_writer_t, output_writer_t<FirstValue>>
    first_writer_;
  subroutine_t<output_list_writer_t, output_list_writer_t<OtherValues...>>
    others_writer_;

  output_list_t<FirstValue, OtherValues...>* outputs_;
};

} // cuti

#endif

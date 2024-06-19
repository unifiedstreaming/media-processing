/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_INPUT_LIST_READER_HPP_
#define CUTI_INPUT_LIST_READER_HPP_

#include "async_readers.hpp"
#include "bound_inbuf.hpp"
#include "input_list.hpp"
#include "linkage.h"
#include "result.hpp"
#include "sequence.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <exception>
#include <utility>

namespace cuti
{

template<typename Value>
struct input_reader_t
{
  using result_value_t = void;

  input_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , value_reader_(*this, result_, buf)
  , input_(nullptr)
  { }

  input_reader_t(input_reader_t const&) = delete;
  input_reader_t& operator=(input_reader_t const&) = delete;
  
  void start(stack_marker_t& base_marker, input_t<Value>& input)
  {
    input_ = &input;
    value_reader_.start(base_marker, &input_reader_t::on_value);
  }

private :
  void on_value(stack_marker_t& base_marker, Value value)
  {
    assert(input_ != nullptr);

    try
    {
      input_->put(std::move(value));
    }
    catch(std::exception const&)
    {
      result_.fail(base_marker, std::current_exception());
      return;
    }

    input_ = nullptr;
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<input_reader_t, reader_t<Value>> value_reader_;

  input_t<Value>* input_;
};

template<typename Value>
struct input_reader_t<sequence_t<Value>>
{
  using result_value_t = void;

  input_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , sequence_reader_(*this, result_, buf)
  { }

  void start(stack_marker_t& base_marker,
             input_t<sequence_t<Value>>& input)
  {
    sequence_reader_.start(
      base_marker, &input_reader_t::on_sequence_read, input);
  }

private :
  void on_sequence_read(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }
    
private :
  result_t<void>& result_;
  subroutine_t<input_reader_t, sequence_reader_t<Value>> sequence_reader_;
};

template<typename... Values>
struct input_list_reader_t;

template<>
struct CUTI_ABI input_list_reader_t<>
{
  using result_value_t = void;

  input_list_reader_t(result_t<void>& result, bound_inbuf_t& /* ignored */)
  : result_(result)
  { }

  input_list_reader_t(input_list_reader_t const&) = delete;
  input_list_reader_t& operator=(input_list_reader_t const&) = delete;

  void start(stack_marker_t& base_marker, input_list_t<>& /* ignored */)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
};

template<typename FirstValue, typename... OtherValues>
struct input_list_reader_t<FirstValue, OtherValues...>
{
  using result_value_t = void;

  input_list_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , first_reader_(*this, result_, buf)
  , others_reader_(*this, result_, buf)
  , inputs_(nullptr)
  { }

  input_list_reader_t(input_list_reader_t const&) = delete;
  input_list_reader_t& operator=(input_list_reader_t const&) = delete;

  void start(stack_marker_t& base_marker,
             input_list_t<FirstValue, OtherValues...>& inputs)
  {
    inputs_ = &inputs;
    first_reader_.start(
      base_marker, &input_list_reader_t::on_first_read, inputs_->first());
  }

private :
  void on_first_read(stack_marker_t& base_marker)
  {
    assert(inputs_ != nullptr);
    others_reader_.start(
      base_marker, &input_list_reader_t::on_others_read, inputs_->others());
  }

  void on_others_read(stack_marker_t& base_marker)
  {
    inputs_ = nullptr;
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<input_list_reader_t, input_reader_t<FirstValue>> first_reader_;
  subroutine_t<input_list_reader_t, input_list_reader_t<OtherValues...>>
    others_reader_;

  input_list_t<FirstValue, OtherValues...>* inputs_;
};
  
} // cuti

#endif

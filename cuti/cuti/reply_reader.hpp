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

#ifndef CUTI_REPLY_READER_HPP_
#define CUTI_REPLY_READER_HPP_

#include "async_readers.hpp"
#include "bound_inbuf.hpp"
#include "input_list.hpp"
#include "input_list_reader.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <utility>

namespace cuti
{

template<typename... Args>
struct reply_reader_t
{
  using result_value_t = void;

  reply_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , inputs_reader_(*this, result_, buf)
  , eom_checker_(*this, result_, buf)
  , inputs_(nullptr)
  { }

  reply_reader_t(reply_reader_t const&) = delete;
  reply_reader_t operator=(reply_reader_t const&) = delete;

  void start(stack_marker_t& base_marker,
             std::unique_ptr<input_list_t<Args...>> inputs)
  {
    assert(inputs != nullptr);
    inputs_ = std::move(inputs);
    inputs_reader_.start(
      base_marker, &reply_reader_t::on_inputs_read, *inputs_);
  }

private :
  void on_inputs_read(stack_marker_t& base_marker)
  {
    eom_checker_.start(base_marker, &reply_reader_t::on_eom_checked);
  }

  void on_eom_checked(stack_marker_t& base_marker)
  {
    result_.submit(base_marker);
  }

private :
  result_t<void>& result_;
  subroutine_t<reply_reader_t, input_list_reader_t<Args...>> inputs_reader_;
  subroutine_t<reply_reader_t, eom_checker_t> eom_checker_;
  std::unique_ptr<input_list_t<Args...>> inputs_; 
};

} // cuti

#endif

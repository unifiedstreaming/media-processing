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

#ifndef CUTI_REPLY_READER_HPP_
#define CUTI_REPLY_READER_HPP_

#include "async_readers.hpp"
#include "bound_inbuf.hpp"
#include "input_list.hpp"
#include "input_list_reader.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <exception>
#include <utility>

namespace cuti
{

template<typename... Args>
struct reply_reader_t
{
  using result_value_t = void;

  reply_reader_t(result_t<void>& result, bound_inbuf_t& buf)
  : result_(result)
  , args_reader_(*this, &reply_reader_t::on_child_failure, buf)
  , eom_checker_(*this, &reply_reader_t::on_child_failure, buf)
  , message_drainer_(*this, result_, buf)
  , ex_()
  { }

  reply_reader_t(reply_reader_t const&) = delete;
  reply_reader_t operator=(reply_reader_t const&) = delete;

  void start(input_list_t<Args...>& args)
  {
    ex_ = nullptr;
  
    args_reader_.start(&reply_reader_t::on_args_read, args);
  }

private :
  void on_args_read()
  {
    eom_checker_.start(&reply_reader_t::on_eom_checked);
  }

  void on_eom_checked()
  {
    message_drainer_.start(&reply_reader_t::on_message_drained);
  }

  void on_child_failure(std::exception_ptr ex)
  {
    assert(ex != nullptr);
    assert(ex_ == nullptr);
    ex_ = std::move(ex);

    message_drainer_.start(&reply_reader_t::on_message_drained);
  }

  void on_message_drained()
  {
    if(ex_ != nullptr)
    {
      result_.fail(std::move(ex_));
      return;
    }

    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<reply_reader_t, input_list_reader_t<Args...>,
    failure_mode_t::handle_in_parent> args_reader_;
  subroutine_t<reply_reader_t, eom_checker_t,
    failure_mode_t::handle_in_parent> eom_checker_;
  subroutine_t<reply_reader_t, message_drainer_t> message_drainer_;
  
  std::exception_ptr ex_;
};

} // cuti

#endif
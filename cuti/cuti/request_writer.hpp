/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#ifndef CUTI_REQUEST_WRITER_HPP_
#define CUTI_REQUEST_WRITER_HPP_

#include "async_writers.hpp"
#include "bound_outbuf.hpp"
#include "identifier.hpp"
#include "output_list.hpp"
#include "output_list_writer.hpp"
#include "result.hpp"
#include "subroutine.hpp"

#include <cassert>
#include <exception>
#include <utility>

namespace cuti
{

template<typename... Args>
struct request_writer_t
{
  using result_value_t = void;

  request_writer_t(result_t<void>& result, bound_outbuf_t& buf)
  : result_(result)
  , method_writer_(*this, result_, buf)
  , args_writer_(*this, result_, buf)
  , args_()
  { }

  request_writer_t(request_writer_t const&) = delete;
  request_writer_t& operator=(request_writer_t const&) = delete;
  
  void start(identifier_t method, output_list_t<Args...>& args)
  {
    args_ = &args;
    method_writer_.start(
      &request_writer_t::on_method_written, std::move(method));
  }

private :
  void on_method_written()
  {
    assert(args_ != nullptr);
    args_writer_.start(&request_writer_t::on_args_written, *args_);
  }

  void on_args_written()
  {
    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<request_writer_t, writer_t<identifier_t>> method_writer_;
  subroutine_t<request_writer_t, output_list_writer_t<Args...>> args_writer_;

  output_list_t<Args...>* args_;
};

} // cuti

#endif

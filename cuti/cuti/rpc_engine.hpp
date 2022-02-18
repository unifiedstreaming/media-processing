/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

#ifndef CUTI_RPC_ENGINE_HPP_
#define CUTI_RPC_ENGINE_HPP_

#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "default_scheduler.hpp"
#include "final_result.hpp"
#include "identifier.hpp"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "reply_reader.hpp"
#include "request_writer.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "system_error.hpp"
#include "throughput_checker.hpp"
#include "type_list.hpp"

#include <cassert>
#include <exception>
#include <utility>

namespace cuti
{

template<typename InputArgsList, typename OutputArgsList>
struct rpc_engine_t;

template<typename... InputArgs, typename... OutputArgs>
struct rpc_engine_t<type_list_t<InputArgs...>,
                    type_list_t<OutputArgs...>>
{
  rpc_engine_t(result_t<void>& result,
               bound_inbuf_t& inbuf,
               bound_outbuf_t& outbuf)
  : result_(result)
  , reply_reader_(*this, &rpc_engine_t::on_child_failure, inbuf)
  , request_writer_(*this, &rpc_engine_t::on_child_failure, outbuf)
  , pending_children_()
  , ex_()
  { }

  rpc_engine_t(rpc_engine_t const&) = delete;
  rpc_engine_t& operator=(rpc_engine_t const&) = delete;
  
  void start(identifier_t method,
             input_list_t<InputArgs...>& reply_args,
             output_list_t<OutputArgs...>& request_args)
  {
    pending_children_ = 2;
    ex_ = nullptr;

    reply_reader_.start(
      &rpc_engine_t::on_child_done, reply_args);
    request_writer_.start(
      &rpc_engine_t::on_child_done, std::move(method), request_args);
  }

private :
  void on_child_failure(std::exception_ptr ex)
  {
    assert(ex != nullptr);

    if(ex_ == nullptr)
    {
      ex_ = std::move(ex);
    }

    this->on_child_done();
  }
  
  void on_child_done()
  {
    assert(pending_children_ != 0);

    if(--pending_children_ != 0)
    {
      return;
    }

    if(ex_ != nullptr)
    {
      result_.fail(std::move(ex_));
      return;
    }

    result_.submit();
  }

private :
  result_t<void>& result_;
  subroutine_t<rpc_engine_t, reply_reader_t<InputArgs...>,
    failure_mode_t::handle_in_parent> reply_reader_;
  subroutine_t<rpc_engine_t, request_writer_t<OutputArgs...>,
    failure_mode_t::handle_in_parent> request_writer_;

  unsigned int pending_children_;
  std::exception_ptr ex_;
};

template<typename... InputArgs, typename... OutputArgs>
void perform_rpc(identifier_t method,
                 nb_inbuf_t& inbuf,
                 input_list_t<InputArgs...>& input_args,
                 nb_outbuf_t& outbuf,
                 output_list_t<OutputArgs...>& output_args,
                 throughput_settings_t settings)
{
  final_result_t<void> result;

  default_scheduler_t scheduler;
  stack_marker_t base_marker;

  bound_inbuf_t bound_inbuf(base_marker, inbuf, scheduler);
  bound_inbuf.enable_throughput_checking(settings);
  
  bound_outbuf_t bound_outbuf(base_marker, outbuf, scheduler);
  bound_outbuf.enable_throughput_checking(settings);
  
  rpc_engine_t<type_list_t<InputArgs...>, type_list_t<OutputArgs...>>
  rpc_engine(result, bound_inbuf, bound_outbuf);

  rpc_engine.start(std::move(method), input_args, output_args);

  while(!result.available())
  {
    auto callback = scheduler.wait();
    assert(callback != nullptr);
    callback();
  }
  assert(scheduler.wait() == nullptr);

  if(int status = bound_outbuf.error_status())
  {
    // Failed to write full request
    system_exception_builder_t builder;
    builder << bound_outbuf << ": output error";
    builder.explode(status);
  }

  if(int status = bound_inbuf.error_status())
  {
    // Failed to read full reply
    system_exception_builder_t builder;
    builder << bound_inbuf << ": input error";
    builder.explode(status);
  }

  // Throws on protocol (and remotely generated) errors
  result.value();
}
    
template<typename... InputArgs, typename... OutputArgs>
void perform_rpc(identifier_t method,
                 nb_inbuf_t& inbuf,
                 input_list_t<InputArgs...>& input_args,
                 nb_outbuf_t& outbuf,
                 output_list_t<OutputArgs...>& output_args)
{
  perform_rpc(std::move(method), inbuf, input_args, outbuf, output_args,
    throughput_settings_t());
}

} // cuti

#endif

/*
 * Copyright (C) 2021-2026 CodeShop B.V.
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

#include "async_readers.hpp"
#include "async_writers.hpp"
#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "identifier.hpp"
#include "input_list.hpp"
#include "output_list.hpp"
#include "reply_reader.hpp"
#include "request_writer.hpp"
#include "result.hpp"
#include "stack_marker.hpp"
#include "subroutine.hpp"
#include "type_list.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <utility>

namespace cuti
{

/*
 * Client side state machine for a single remote procedure call.
 */
template<typename InputArgsList, typename OutputArgsList>
struct rpc_engine_t;

template<typename... InputArgs, typename... OutputArgs>
struct rpc_engine_t<type_list_t<InputArgs...>,
                    type_list_t<OutputArgs...>>
{
  using result_value_t = void;

  rpc_engine_t(result_t<void>& result,
               scheduler_t& scheduler,
               nb_inbuf_t& nb_inbuf,
               nb_outbuf_t& nb_outbuf,
               throughput_settings_t settings = throughput_settings_t())
  : result_(result)
  , bound_inbuf_(nb_inbuf, scheduler)
  , bound_outbuf_(nb_outbuf, scheduler)
  , reply_reader_(*this, &rpc_engine_t::on_reply_error, bound_inbuf_)
  , message_drainer_(*this, result_, bound_inbuf_)
  , input_state_(input_not_started)
  , request_writer_(*this, &rpc_engine_t::on_request_error, bound_outbuf_)
  , eom_writer_(*this, result_, bound_outbuf_)
  , output_state_(output_not_started)
  , ex_(nullptr)
  {
    bound_inbuf_.enable_throughput_checking(settings);
    bound_outbuf_.enable_throughput_checking(settings);
  }

  rpc_engine_t(rpc_engine_t const&) = delete;
  rpc_engine_t& operator=(rpc_engine_t const&) = delete;
  
  void start(stack_marker_t& base_marker,
             identifier_t method,
             std::unique_ptr<input_list_t<InputArgs...>> inputs,
             std::unique_ptr<output_list_t<OutputArgs...>> outputs)
  {
    assert(method.is_valid());
    assert(inputs != nullptr);
    assert(outputs != nullptr);

    input_state_ = input_not_started;
    output_state_ = output_not_started;
    ex_ = nullptr;

    // This is obviously true...
    if(input_state_ == input_not_started)
    {
      input_state_ = reading_reply;
      reply_reader_.start(
        base_marker, &rpc_engine_t::on_reply_read, std::move(inputs));
    }

    // ...but be careful here: input engine may have changed output state
    if(output_state_ == output_not_started)
    {
      output_state_ = writing_request;
      request_writer_.start(base_marker,
                            &rpc_engine_t::on_request_written,
                            std::move(method),
                            std::move(outputs));
    }
  }

private :
  void on_reply_read(stack_marker_t& base_marker)
  {
    assert(input_state_ == reading_reply);
    input_state_ = draining_message;
    message_drainer_.start(base_marker, &rpc_engine_t::on_message_drained);
  }

  void on_reply_error(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);
    assert(input_state_ == reading_reply);

    if(ex_ == nullptr)
    {
      ex_ = std::move(ex);
    }

    if(output_state_ == output_not_started || output_state_ == writing_request)
    {
      // Skip or cancel request writing
      bound_outbuf_.cancel_when_writable();
      output_state_ = writing_eom;
      eom_writer_.start(base_marker, &rpc_engine_t::on_eom_written);
    }

    assert(input_state_ == reading_reply);
    input_state_ = draining_message;
    message_drainer_.start(base_marker, &rpc_engine_t::on_message_drained);
  }

  void on_message_drained(stack_marker_t& base_marker)
  {
    assert(input_state_ == draining_message);
    input_state_ = input_done;

    if(output_state_ == output_done)
    {
      this->on_done(base_marker);
    }
  }

  void on_request_written(stack_marker_t& base_marker)
  {
    assert(output_state_ == writing_request);
    output_state_ = writing_eom;
    eom_writer_.start(base_marker, &rpc_engine_t::on_eom_written);
  }

  void on_request_error(stack_marker_t& base_marker, std::exception_ptr ex)
  {
    assert(ex != nullptr);
    assert(output_state_ == writing_request);

    if(ex_ == nullptr)
    {
      ex_ = std::move(ex);
    }

    if(input_state_ == input_not_started || input_state_ == reading_reply)
    {
      // Skip or cancel reply reading
      bound_inbuf_.cancel_when_readable();
      input_state_ = draining_message;
      message_drainer_.start(base_marker, &rpc_engine_t::on_message_drained);
    }

    assert(output_state_ == writing_request);
    output_state_ = writing_eom;
    eom_writer_.start(base_marker, &rpc_engine_t::on_eom_written);
  }

  void on_eom_written(stack_marker_t& base_marker)
  {
    assert(output_state_ == writing_eom);
    output_state_ = output_done;

    if(input_state_ == input_done)
    {
      this->on_done(base_marker);
    }
  }

  void on_done(stack_marker_t& base_marker)
  {
    assert(input_state_ == input_done);
    assert(output_state_ == output_done);

    if(auto status = bound_outbuf_.error_status())
    {
      // Low-level I/O error on bound_outbuf_: report root cause
      system_exception_builder_t builder;
      builder <<
        "output error on " << bound_outbuf_ << ": " << error_status_t(status);
      result_.fail(base_marker, builder.exception_ptr());
    }
    else if(auto status = bound_inbuf_.error_status())
    {
      // Low-level I/O error on bound_inbuf_: report root cause
      system_exception_builder_t builder;
      builder <<
        "input error on " << bound_inbuf_ << ": " << error_status_t(status);
      result_.fail(base_marker, builder.exception_ptr());
    }
    else if(ex_ != nullptr)
    {
      result_.fail(base_marker, std::move(ex_));
    }
    else
    {
      result_.submit(base_marker);
    }
  }
    
private :
  result_t<void>& result_;
  bound_inbuf_t bound_inbuf_;
  bound_outbuf_t bound_outbuf_;

  subroutine_t<rpc_engine_t, reply_reader_t<InputArgs...>,
    failure_mode_t::handle_in_parent> reply_reader_;
  subroutine_t<rpc_engine_t, message_drainer_t> message_drainer_;
  enum { input_not_started, reading_reply, draining_message, input_done }
    input_state_;
  
  subroutine_t<rpc_engine_t, request_writer_t<OutputArgs...>,
    failure_mode_t::handle_in_parent> request_writer_;
  subroutine_t<rpc_engine_t, eom_writer_t> eom_writer_;
  enum { output_not_started, writing_request, writing_eom, output_done }
    output_state_;

  std::exception_ptr ex_;
};

} // cuti

#endif

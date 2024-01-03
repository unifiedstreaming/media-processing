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
#include "subroutine.hpp"
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
  using result_value_t = void;

  rpc_engine_t(result_t<void>& result,
               bound_inbuf_t& inbuf,
               bound_outbuf_t& outbuf)
  : result_(result)
  , inbuf_(inbuf)
  , outbuf_(outbuf)
  , reply_reader_(*this, &rpc_engine_t::on_reply_error, inbuf_)
  , message_drainer_(*this, result_, inbuf_)
  , input_state_(input_not_started)
  , request_writer_(*this, &rpc_engine_t::on_request_error, outbuf_)
  , eom_writer_(*this, result_, outbuf_)
  , output_state_(output_not_started)
  , ex_(nullptr)
  { }

  rpc_engine_t(rpc_engine_t const&) = delete;
  rpc_engine_t& operator=(rpc_engine_t const&) = delete;
  
  void start(identifier_t method,
             input_list_t<InputArgs...>& reply_args,
             output_list_t<OutputArgs...>& request_args)
  {
    assert(method.is_valid());

    input_state_ = input_not_started;
    output_state_ = output_not_started;
    ex_ = nullptr;

    // This is obviously true...
    if(input_state_ == input_not_started)
    {
      input_state_ = reading_reply;
      reply_reader_.start(
        &rpc_engine_t::on_reply_read, reply_args);
    }

    // ...but be careful here: input engine may have changed output state
    if(output_state_ == output_not_started)
    {
      output_state_ = writing_request;
      request_writer_.start(
        &rpc_engine_t::on_request_written, std::move(method), request_args);
    }
  }

private :
  void on_reply_read()
  {
    assert(input_state_ == reading_reply);
    input_state_ = draining_message;
    message_drainer_.start(&rpc_engine_t::on_message_drained);
  }

  void on_reply_error(std::exception_ptr ex)
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
      outbuf_.cancel_when_writable();
      output_state_ = writing_eom;
      eom_writer_.start(&rpc_engine_t::on_eom_written);
    }

    assert(input_state_ == reading_reply);
    input_state_ = draining_message;
    message_drainer_.start(&rpc_engine_t::on_message_drained);
  }

  void on_message_drained()
  {
    assert(input_state_ == draining_message);
    input_state_ = input_done;

    if(output_state_ == output_done)
    {
      if(ex_ != nullptr)
      {
        result_.fail(std::move(ex_));
      }
      else
      {
        result_.submit();
      }
    }
  }

  void on_request_written()
  {
    assert(output_state_ == writing_request);
    output_state_ = writing_eom;
    eom_writer_.start(&rpc_engine_t::on_eom_written);
  }

  void on_request_error(std::exception_ptr ex)
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
      inbuf_.cancel_when_readable();
      input_state_ = draining_message;
      message_drainer_.start(&rpc_engine_t::on_message_drained);
    }

    assert(output_state_ == writing_request);
    output_state_ = writing_eom;
    eom_writer_.start(&rpc_engine_t::on_eom_written);
  }

  void on_eom_written()
  {
    assert(output_state_ == writing_eom);
    output_state_ = output_done;

    if(input_state_ == input_done)
    {
      if(ex_ != nullptr)
      {
        result_.fail(std::move(ex_));
      }
      else
      {
        result_.submit();
      }
    }
  }

private :
  result_t<void>& result_;
  bound_inbuf_t& inbuf_;
  bound_outbuf_t& outbuf_;

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

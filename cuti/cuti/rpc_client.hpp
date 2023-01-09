/*
 * Copyright (C) 2022-2023 CodeShop B.V.
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

#ifndef CUTI_RPC_CLIENT_HPP_
#define CUTI_RPC_CLIENT_HPP_

#include "bound_inbuf.hpp"
#include "bound_outbuf.hpp"
#include "default_scheduler.hpp"
#include "endpoint.hpp"
#include "final_result.hpp"
#include "identifier.hpp"
#include "input_list.hpp"
#include "linkage.h"
#include "nb_inbuf.hpp"
#include "nb_outbuf.hpp"
#include "nb_tcp_buffers.hpp"
#include "output_list.hpp"
#include "rpc_engine.hpp"
#include "stack_marker.hpp"
#include "system_error.hpp"
#include "tcp_connection.hpp"
#include "throughput_checker.hpp"
#include "type_list.hpp"

#include <cassert>
#include <cstddef>
#include <ostream>
#include <memory>
#include <utility>

namespace cuti
{

struct CUTI_ABI rpc_client_t
{
  rpc_client_t(std::unique_ptr<nb_inbuf_t> inbuf,
               std::unique_ptr<nb_outbuf_t> outbuf,
               throughput_settings_t settings = throughput_settings_t())
  : inbuf_((assert(inbuf != nullptr), std::move(inbuf)))
  , outbuf_((assert(outbuf != nullptr), std::move(outbuf)))
  , settings_(std::move(settings))
  , scheduler_()
  { }

  explicit
  rpc_client_t(std::pair<std::unique_ptr<nb_inbuf_t>,
                         std::unique_ptr<nb_outbuf_t>> buffers, 
               throughput_settings_t settings = throughput_settings_t())
  : rpc_client_t(std::move(buffers.first),
                 std::move(buffers.second),
                 std::move(settings))
  { }

  rpc_client_t(endpoint_t const& server_address,
               std::size_t inbufsize,
               std::size_t outbufsize,
               throughput_settings_t settings = throughput_settings_t())
  : rpc_client_t(make_nb_tcp_buffers(
                   std::make_unique<tcp_connection_t>(server_address),
                   inbufsize,
                   outbufsize),
                 std::move(settings))
  { }

  explicit
  rpc_client_t(endpoint_t const& server_address,
               throughput_settings_t settings = throughput_settings_t())
  : rpc_client_t(make_nb_tcp_buffers(
                   std::make_unique<tcp_connection_t>(server_address)),
                 std::move(settings))
  { }

  rpc_client_t(rpc_client_t const&) = delete;
  rpc_client_t& operator=(rpc_client_t const&) = delete;
  
  template<typename... InputArgs, typename... OutputArgs>
  void operator()(identifier_t method,
                  input_list_t<InputArgs...>& input_args,
                  output_list_t<OutputArgs...>& output_args)
  {
    assert(method.is_valid());

    stack_marker_t base_marker;

    bound_inbuf_t bound_inbuf(base_marker, *inbuf_, scheduler_);
    bound_inbuf.enable_throughput_checking(settings_);
  
    bound_outbuf_t bound_outbuf(base_marker, *outbuf_, scheduler_);
    bound_outbuf.enable_throughput_checking(settings_);
  
    final_result_t<void> result;
    rpc_engine_t<type_list_t<InputArgs...>, type_list_t<OutputArgs...>>
      rpc_engine(result, bound_inbuf, bound_outbuf);

    rpc_engine.start(std::move(method), input_args, output_args);

    while(!result.available())
    {
      auto callback = scheduler_.wait();
      assert(callback != nullptr);
      callback();
    }
    assert(scheduler_.wait() == nullptr);

    if(auto status = bound_outbuf.error_status())
    {
      // Failed to write full request
      system_exception_builder_t builder;
      builder << "output error: " << error_status_t(status);
      builder.explode();
    }

    if(auto status = bound_inbuf.error_status())
    {
      // Failed to read full reply
      system_exception_builder_t builder;
      builder << "input error: " << error_status_t(status);
      builder.explode();
    }

    // Throws on protocol (and remotely generated) errors
    result.value();
  }

  friend std::ostream& operator<<(std::ostream& os, rpc_client_t const& client)
  {
    return os << *client.inbuf_;
  }
                   
private :
  std::unique_ptr<nb_inbuf_t> inbuf_;
  std::unique_ptr<nb_outbuf_t> outbuf_;
  throughput_settings_t const settings_;
  default_scheduler_t scheduler_;
};

} // cuti

#endif

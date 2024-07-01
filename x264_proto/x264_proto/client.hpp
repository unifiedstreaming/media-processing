/*
 * Copyright (C) 2022-2024 CodeShop B.V.
 *
 * This file is part of the x264 service protocol library.
 *
 * The x264 service protocol library is free software: you can
 * redistribute it and/or modify it under the terms of version 2.1 of
 * the GNU Lesser General Public License as published by the Free
 * Software Foundation.
 *
 * The x264 service protocol library is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See version 2.1 of the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of version 2.1 of the GNU Lesser
 * General Public License along with the x264 service protocol
 * library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef X264_PROTO_CLIENT_HPP_
#define X264_PROTO_CLIENT_HPP_

#include "linkage.h"
#include "types.hpp"

#include <cuti/endpoint.hpp>
#include <cuti/input_list.hpp>
#include <cuti/nb_client_cache.hpp>
#include <cuti/logging_context.hpp>
#include <cuti/output_list.hpp>
#include <cuti/rpc_client.hpp>
#include <cuti/throughput_checker.hpp>
#include <cuti/type_list.hpp>

#include <string>
#include <utility>
#include <vector>

namespace x264_proto
{

struct X264_PROTO_ABI client_t
{
  // 'add' is for testing purposes
  using add_reply_types_t =
    cuti::type_list_t<int>;
  using add_request_types_t =
    cuti::type_list_t<int, int>;

  // 'echo' is for testing purposes
  using echo_reply_types_t =
    cuti::type_list_t<cuti::sequence_t<std::string>>;
  using echo_request_types_t =
    cuti::type_list_t<cuti::sequence_t<std::string>>;

  using encode_reply_types_t =
    cuti::type_list_t<sample_headers_t, cuti::sequence_t<sample_t>>;
  using encode_request_types_t =
    cuti::type_list_t<session_params_t, cuti::sequence_t<frame_t>>;
    
  // 'subtract' is for testing purposes
  using subtract_reply_types_t =
    cuti::type_list_t<int>;
  using subtract_request_types_t =
    cuti::type_list_t<int, int>;
    
  client_t(cuti::logging_context_t const& context,
           cuti::nb_client_cache_t& client_cache,
           cuti::endpoint_t server_address,
           cuti::throughput_settings_t settings =
             cuti::throughput_settings_t());
    
  client_t(client_t const&) = delete;
  client_t& operator=(client_t const&) = delete;

  /*
   * Streaming interface
   */
  bool busy() const
  { return rpc_client_.busy(); }

  void step() // throws on RPC exceptions
  { rpc_client_.step(); }

  void complete_current_call()
  {
    rpc_client_.complete_current_call();
  }

  template<typename Result, typename Arg1, typename Arg2>
  void start_add(Result&& result, Arg1&& arg1, Arg2&& arg2)
  {
    auto inputs = cuti::make_input_list_ptr<add_reply_types_t>(
      std::forward<Result>(result));

    auto outputs = cuti::make_output_list_ptr<add_request_types_t>(
      std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));

    rpc_client_.start("add", std::move(inputs), std::move(outputs));
  }

  template<typename Consumer, typename Producer>
  void start_echo(Consumer&& consumer, Producer&& producer)
  {
    auto inputs = cuti::make_input_list_ptr<echo_reply_types_t>(
      std::forward<Consumer>(consumer));

    auto outputs = cuti::make_output_list_ptr<echo_request_types_t>(
      std::forward<Producer>(producer));

    rpc_client_.start("echo", std::move(inputs), std::move(outputs));
  }

  template<typename SampleHeadersConsumer, typename SampleConsumer,
           typename SessionParamsProducer, typename FrameProducer>
  void start_encode(SampleHeadersConsumer&& sample_headers_consumer,
                    SampleConsumer&& sample_consumer,
                    SessionParamsProducer&& session_params_producer,
                    FrameProducer&& frame_producer)
  {
    auto inputs = cuti::make_input_list_ptr<encode_reply_types_t>(
      std::forward<SampleHeadersConsumer>(sample_headers_consumer),
      std::forward<SampleConsumer>(sample_consumer));

    auto outputs = cuti::make_output_list_ptr<encode_request_types_t>(
      std::forward<SessionParamsProducer>(session_params_producer),
      std::forward<FrameProducer>(frame_producer));
    
    rpc_client_.start("encode", std::move(inputs), std::move(outputs));
  }

  template<typename Result, typename Arg1, typename Arg2>
  void start_subtract(Result&& result, Arg1&& arg1, Arg2&& arg2)
  {
    auto inputs = cuti::make_input_list_ptr<subtract_reply_types_t>(
      std::forward<Result>(result));

    auto outputs = cuti::make_output_list_ptr<subtract_request_types_t>(
      std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));

    rpc_client_.start("subtract", std::move(inputs), std::move(outputs));
  }

  /*
   * Convenience interface
   */
  int add(int arg1, int arg2);

  std::vector<std::string> echo(std::vector<std::string> strings);

  std::pair<sample_headers_t, std::vector<sample_t>>
  encode(session_params_t session_params, std::vector<frame_t> frames);
  
  int subtract(int arg1, int arg2);
   
private :
  cuti::rpc_client_t rpc_client_;
};

} // x264_proto

#endif

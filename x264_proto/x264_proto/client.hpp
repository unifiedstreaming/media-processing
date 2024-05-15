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
#include <cuti/rpc_client.hpp>

#include <utility>
#include <vector>

namespace x264_proto
{

struct X264_PROTO_ABI client_t
{
  using samples_t = std::vector<sample_t>;
  using frames_t = std::vector<frame_t>;

  explicit client_t(cuti::endpoint_t const& server_address);

  client_t(client_t const&) = delete;
  client_t& operator=(client_t const&) = delete;

  // for testing purposes
  int add(int arg1, int arg2);
  int subtract(int arg1, int arg2);

  void encode(sample_headers_t& sample_headers,
              samples_t& samples,
              session_params_t session_params,
              frames_t frames);

  template<typename SampleHeadersConsumer,
           typename SamplesConsumer,
           typename SessionParamsProducer,
           typename FramesProducer>
  void encode_streaming(SampleHeadersConsumer&& sample_headers_consumer,
                        SamplesConsumer&& samples_consumer,
                        SessionParamsProducer&& session_params_producer,
                        FramesProducer&& frames_producer)
  {
    auto inputs = cuti::make_input_list<
      sample_headers_t,
      cuti::streaming_tag_t<sample_t>
    >(
      std::forward<SampleHeadersConsumer>(sample_headers_consumer),
      std::forward<SamplesConsumer>(samples_consumer)
    );

    auto outputs = cuti::make_output_list<
      session_params_t,
      cuti::streaming_tag_t<frame_t>
    >(
      std::forward<SessionParamsProducer>(session_params_producer),
      std::forward<FramesProducer>(frames_producer)
    );

    rpc_client_("encode", inputs, outputs);
  }
   
private :
  cuti::rpc_client_t rpc_client_;
};

} // x264_proto

#endif

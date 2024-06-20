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

#include "client.hpp"

namespace x264_proto
{

client_t::client_t(cuti::endpoint_t const& server_address)
: rpc_client_(server_address)
{ }

int client_t::add(int arg1, int arg2)
{
  int result;

  auto inputs = cuti::make_input_list_ptr<int>(result);
  auto outputs = cuti::make_output_list_ptr<int, int>(arg1, arg2);
  rpc_client_("add", std::move(inputs), std::move(outputs));

  return result;
}

int client_t::subtract(int arg1, int arg2)
{
  int result;

  auto inputs = cuti::make_input_list_ptr<int>(result);
  auto outputs = cuti::make_output_list_ptr<int, int>(arg1, arg2);
  rpc_client_("subtract", std::move(inputs), std::move(outputs));

  return result;
}

void client_t::encode(sample_headers_t& sample_headers,
                      std::vector<sample_t>& samples,
                      session_params_t session_params,
                      std::vector<frame_t> frames)
{
  auto inputs = cuti::make_input_list_ptr<sample_headers_t, samples_t>(
    sample_headers, samples);
  auto outputs = cuti::make_output_list_ptr<session_params_t, frames_t>(
    std::move(session_params), std::move(frames));
  rpc_client_("encode", std::move(inputs), std::move(outputs));
}
  
} // x264_proto

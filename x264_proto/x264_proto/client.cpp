/*
 * Copyright (C) 2022-2025 CodeShop B.V.
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

client_t::client_t(cuti::logging_context_t const& context,
                   cuti::nb_client_cache_t& client_cache,
                   cuti::endpoint_t server_address,
                   cuti::throughput_settings_t settings)
: rpc_client_(
    context, client_cache, std::move(server_address), std::move(settings))
{ }

int client_t::add(int arg1, int arg2)
{
  int result;

  this->start_add(result, arg1, arg2);
  this->complete_current_call();

  return result;
}

std::vector<std::string> client_t::echo(std::vector<std::string> strings)
{
  std::vector<std::string> result;

  this->start_echo(result, std::move(strings));
  this->complete_current_call();

  return result;
}

std::pair<sample_headers_t, std::vector<sample_t>>
client_t::encode(session_params_t session_params, std::vector<frame_t> frames)
{
  std::pair<sample_headers_t, std::vector<sample_t>> result;

  this->start_encode(result.first, result.second,
    std::move(session_params), std::move(frames));
  this->complete_current_call();

  return result;
}    
  
int client_t::subtract(int arg1, int arg2)
{
  int result;

  this->start_subtract(result, arg1, arg2);
  this->complete_current_call();

  return result;
}

} // x264_proto

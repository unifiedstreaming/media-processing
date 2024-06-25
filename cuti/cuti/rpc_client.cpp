/*
 * Copyright (C) 2022-2024 CodeShop B.V.
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

#include "rpc_client.hpp"

#include "nb_tcp_buffers.hpp"
#include "scoped_guard.hpp"
#include "tcp_connection.hpp"

#include <ostream>
#include <tuple>

namespace cuti
{

rpc_client_t::rpc_client_t(endpoint_t server_address,
                           std::size_t inbufsize,
                           std::size_t outbufsize,
                           throughput_settings_t settings)
: scheduler_()
, nb_client_(std::move(server_address), inbufsize, outbufsize)
, settings_(std::move(settings))
, curr_call_(nullptr)
{ }
    
rpc_client_t::rpc_client_t(endpoint_t server_address,
                           throughput_settings_t settings)
: rpc_client_t(std::move(server_address),
               nb_inbuf_t::default_bufsize,
               nb_outbuf_t::default_bufsize,
               std::move(settings))
{ }
    
void rpc_client_t::step()
{
  assert(this->busy());
     
  auto deactivator =
    make_scoped_guard([this] { this->curr_call_.reset(); });

  curr_call_->step(); // this is where detected exceptions are thrown

  if(curr_call_->busy())
  {
    deactivator.dismiss(); // keep curr_call_ alive
  }
}

rpc_client_t::call_t::call_t(default_scheduler_t& scheduler)
: scheduler_(scheduler)
, result_()
, done_(false)
{ }

void rpc_client_t::call_t::step()
{
  assert(this->busy());

  if(result_.available())
  {
    done_ = true;
    result_.value(); // this is where detected exceptions are thrown
  }
  else
  {
    auto cb = scheduler_.wait();
    assert(cb != nullptr);

    stack_marker_t base_marker;
    cb(base_marker);
  }
}

rpc_client_t::call_t::~call_t()
{ }

std::ostream& operator<<(std::ostream& os, rpc_client_t const& client)
{
  return os << client.nb_client_.nb_inbuf();
}

} // namespace cuti

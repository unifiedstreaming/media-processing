/*
 * Copyright (C) 2022-2025 CodeShop B.V.
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

#include <cassert>
#include <ostream>
#include <tuple>

namespace cuti
{

rpc_client_t::rpc_client_t(
  logging_context_t const& context,
  nb_client_cache_t& client_cache,
  endpoint_t server_address,
  throughput_settings_t settings)
: context_(context)
, client_cache_(client_cache)
, server_address_((assert(!server_address.empty()), std::move(server_address)))
, settings_(std::move(settings))
, curr_call_(nullptr)
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

rpc_client_t::call_t::call_t(
  logging_context_t const& context,
  default_scheduler_t& scheduler,
  nb_client_cache_t& client_cache,
  endpoint_t const& server_address)
: context_(context)
, scheduler_(scheduler)
, result_()
, done_(false)
, client_cache_(client_cache)
, nb_client_(client_cache_.obtain(context_, server_address)) 
{
  assert(nb_client_ != nullptr);
}

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
{
  if(done_ && result_.exception() == nullptr)
  {
    // No RPC errors detected: connection reusable
    client_cache_.store(context_, std::move(nb_client_));
  }
  else
  {
    // Clear (possibly) bad cache entries
    client_cache_.invalidate_entries(context_, nb_client_->server_address());

    if (auto msg = context_.message_at(loglevel_t::info))
    {
      *msg << "rpc_client: closing connection " << *nb_client_;
    }
  }
}

} // namespace cuti

/*
 * Copyright (C) 2024-2025 CodeShop B.V.
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

#include "simple_nb_client_cache.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace cuti
{

simple_nb_client_cache_t::simple_nb_client_cache_t(
  socket_layer_t& sockets,
  std::size_t max_cachesize,
  std::size_t inbufsize,
  std::size_t outbufsize)
: sockets_(sockets)
, max_cachesize_(max_cachesize)
, inbufsize_(inbufsize)
, outbufsize_(outbufsize)
, mut_()
, clients_()
{ }

std::unique_ptr<nb_client_t>
simple_nb_client_cache_t::obtain(logging_context_t const& context,
                                 endpoint_t const& server_address)
{
  assert(!server_address.empty());

  std::unique_ptr<nb_client_t> result = nullptr;
  {
    std::lock_guard<std::mutex> guard(mut_);

    auto pos = std::find_if(clients_.begin(), clients_.end(),
      [&server_address](std::unique_ptr<nb_client_t> const& client)
      { return client->server_address() == server_address; });

    if(pos != clients_.end())
    {
      result = std::move(*pos);
      clients_.erase(pos);
    }
  }

  if(result != nullptr)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this << ": reusing connection " << *result;
    }
  }
  else
  {
    try
    {
      result = std::make_unique<nb_client_t>(
        sockets_, server_address, inbufsize_, outbufsize_);
    }
    catch(std::exception const&)
    {
      this->invalidate_entries(context, server_address);
      throw;
    }

    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this << ": created new connection " << *result;
    }
  }

  return result;
}

void simple_nb_client_cache_t::store(logging_context_t const& context,
                                     std::unique_ptr<nb_client_t> client)
{
  assert(client != nullptr);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this << ": storing connection " << *client;
  }

  std::unique_ptr<nb_client_t> evict = nullptr;
  {
    std::lock_guard<std::mutex> guard(mut_);
  
    std::size_t old_size = clients_.size();
    clients_.push_front(std::move(client));

    if(old_size == max_cachesize_)
    {
      evict = std::move(clients_.back());
      clients_.pop_back();
    }
  }

  if(evict != nullptr)
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this <<
        ": max cache size reached: closing connection " << *evict;
    }
  }
}

void
simple_nb_client_cache_t::invalidate_entries(logging_context_t const& context,
                                             endpoint_t const& server_address)
{
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this <<
        ": invalidating connections to " << server_address;
  }

  std::list<std::unique_ptr<nb_client_t>> invalidated;
  {
    std::lock_guard<std::mutex> guard(mut_);

    auto next = clients_.begin();
    while(next != clients_.end())
    {
      auto cur = next;
      ++next;
      
      assert(*cur != nullptr);
      if((*cur)->server_address() == server_address)
      {
        invalidated.splice(invalidated.end(), clients_, cur);
      }
    }
  }

  while(!invalidated.empty())
  {
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this <<
        ": closing invalidated connection " << *invalidated.front();
    }

    invalidated.pop_front();
  }
}

std::ostream& operator<<(std::ostream& os,
                         simple_nb_client_cache_t const& cache)
{
  return os << "simple_nb_client_cache@" << &cache;
}
    
} // cuti

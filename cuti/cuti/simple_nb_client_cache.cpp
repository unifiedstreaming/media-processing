/*
 * Copyright (C) 2024-2026 CodeShop B.V.
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
  socket_layer_t& sockets, settings_t settings)
: sockets_(sockets)
, settings_(std::move(settings))
, mut_()
, entries_()
{ }

socket_layer_t& simple_nb_client_cache_t::socket_layer()
{
  return sockets_;
}

std::unique_ptr<nb_client_t>
simple_nb_client_cache_t::obtain(logging_context_t const& context,
                                 endpoint_t const& server_address)
{
  assert(!server_address.empty());

  std::unique_ptr<nb_client_t> result = nullptr;
  entry_list_t expired_entries{};
  auto expiry_timestamp = cuti_clock_t::now() - settings_.max_age_;

  {
    std::lock_guard<std::mutex> guard(mut_);

    auto next = entries_.begin();

    while(next != entries_.end() &&
      next->timestamp_ > expiry_timestamp &&
      result == nullptr)
    {
      assert(next->client_ != nullptr);
      auto pos = next;
      ++next;
      
      if(pos->client_->server_address() == server_address)
      {
        result = std::move(pos->client_);
        entries_.erase(pos);
      }
    }

    while(next != entries_.end() &&
          next->timestamp_ > expiry_timestamp)
    {
      assert(next->client_ != nullptr);
      ++next;
    }

    expired_entries.splice(expired_entries.begin(),
      entries_, next, entries_.end());
  }

  while(!expired_entries.empty())
  {
    assert(expired_entries.front().client_ != nullptr);
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this << ": closing expired connection " <<
        *(expired_entries.front().client_);
    }
    expired_entries.pop_front();
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
        sockets_, server_address,
        settings_.inbufsize_, settings_.outbufsize_);
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
  
    std::size_t old_size = entries_.size();
    entries_.emplace_front(std::move(client));

    if(old_size == settings_.max_cachesize_)
    {
      evict = std::move(entries_.back().client_);
      entries_.pop_back();
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

void simple_nb_client_cache_t::invalidate_entries(
  logging_context_t const& context,
  endpoint_t const& server_address)
{
  assert(!server_address.empty());

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this <<
      ": invalidating connections to " << server_address;
  }

  entry_list_t invalidated_entries{};
  entry_list_t expired_entries{};
  auto expiry_timestamp = cuti_clock_t::now() - settings_.max_age_;

  {
    std::lock_guard<std::mutex> guard(mut_);

    auto next = entries_.begin();

    while(next != entries_.end() &&
      next->timestamp_ > expiry_timestamp)
    {
      assert(next->client_ != nullptr);
      auto pos = next;
      ++next;
      
      if(pos->client_->server_address() == server_address)
      {
        invalidated_entries.splice(invalidated_entries.end(),
          entries_, pos);
      }
    }

    expired_entries.splice(expired_entries.begin(),
      entries_, next, entries_.end());
  }

  while(!invalidated_entries.empty())
  {
    assert(invalidated_entries.front().client_ != nullptr);
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this << ": closing invalidated connection " <<
        *(invalidated_entries.front().client_);
    }
    invalidated_entries.pop_front();
  }
        
  while(!expired_entries.empty())
  {
    assert(expired_entries.front().client_ != nullptr);
    if(auto msg = context.message_at(loglevel_t::info))
    {
      *msg << *this << ": closing expired connection " <<
        *(expired_entries.front().client_);
    }
    expired_entries.pop_front();
  }
}

std::ostream& operator<<(std::ostream& os,
                         simple_nb_client_cache_t const& cache)
{
  return os << "simple_nb_client_cache@" << &cache;
}
    
} // cuti

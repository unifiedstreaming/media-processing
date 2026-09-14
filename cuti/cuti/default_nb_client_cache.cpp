/*
 * Copyright (C) 2026 CodeShop B.V.
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

#include "default_nb_client_cache.hpp"

#include <cassert>

namespace cuti
{

namespace // anonymous
{

auto make_ktor(
  socket_layer_t& sockets,
  std::size_t inbufsize,
  std::size_t outbufsize)
{
  return [&sockets, inbufsize, outbufsize](endpoint_t const& server_address)
  {
    return std::make_unique<nb_client_t>(
      sockets, server_address, inbufsize, outbufsize);
  };
}

auto make_rtok()
{
  return [](nb_client_t const& client)
  { return client.server_address(); };
}

} // anonymous

default_nb_client_cache_t::default_nb_client_cache_t(
  socket_layer_t& sockets, settings_t const& settings)
: nb_client_cache_t()
, sockets_(sockets)
, resource_cache_(
    make_ktor(sockets_, settings.inbufsize_, settings.outbufsize_),
    make_rtok(),
    settings.resource_cache_settings_)
{ }

socket_layer_t& default_nb_client_cache_t::socket_layer() const
{ return sockets_; }

std::unique_ptr<nb_client_t> default_nb_client_cache_t::obtain(
  logging_context_t const& context,
  endpoint_t const& server_address)
{
  assert(!server_address.empty());

  auto client = resource_cache_.obtain(server_address);

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this << ": obtained connection " << *client;
  }

  return client;
}

void default_nb_client_cache_t::store(
  logging_context_t const& context,
  std::unique_ptr<nb_client_t> client)
{
  assert(client != nullptr);
  
  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this << ": storing connection " << *client;
  }

  resource_cache_.store(std::move(client));
}

void default_nb_client_cache_t::invalidate_entries(
  logging_context_t const& context,
  endpoint_t const& server_address)
{
  assert(!server_address.empty());

  if(auto msg = context.message_at(loglevel_t::info))
  {
    *msg << *this <<
      ": invalidating connections to " << server_address;
  }

  resource_cache_.wipe(server_address);
}

default_nb_client_cache_t::~default_nb_client_cache_t() = default;

} // cuti

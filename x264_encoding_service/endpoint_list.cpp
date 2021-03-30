/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the x264_encoding_service.
 *
 * The x264_encoding_service is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The x264_encoding_service is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the x264_encoding_service.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "endpoint_list.hpp"
#include "system_error.hpp"

#include <cassert>
#include <cinttypes>
#include <cstring>
#include <limits>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#ifndef AI_IDN
#define AI_IDN 0
#endif

namespace xes
{

namespace // anonymous
{

std::shared_ptr<addrinfo const>
make_head(int flags, char const* host, unsigned int port)
{
  static const unsigned int max_port =
    std::numeric_limits<std::uint16_t>::max();
  if(port > max_port)
  {
    system_exception_builder_t builder;
    builder << "Port number " << port << " out of range";
    builder.explode();
  }

  addrinfo hints;
  std::memset(&hints, '\0', sizeof hints);
  hints.ai_flags = flags | AI_ADDRCONFIG | AI_NUMERICSERV | AI_IDN;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* head;
  int r = getaddrinfo(host, std::to_string(port).c_str(), &hints, &head);
  if(r != 0)
  {
#ifdef _WIN32
    int cause = last_system_error();
#endif
    system_exception_builder_t builder;
    builder << "Can't resolve";
    if(host != nullptr)
    {
      builder << " host " << host;
    }
    builder << " port " << port;
#ifdef _WIN32
    builder.explode(cause);
#else
    builder << ": " << gai_strerror(r);
    builder.explode();
#endif
  }

  return std::shared_ptr<const addrinfo>(head, freeaddrinfo);
}

} // anonymous

local_interfaces_t const local_interfaces = { };
all_interfaces_t const all_interfaces = { };

endpoint_list_iterator_t& endpoint_list_iterator_t::operator++()
{
  assert(node_ != nullptr);
  node_ = node_->ai_next;
  return *this;
}

endpoint_t const& endpoint_list_iterator_t::operator*() const
{
  assert(node_ != nullptr);
  return *node_->ai_addr;
}

endpoint_list_t::endpoint_list_t(local_interfaces_t const&, unsigned int port)
: head_(make_head(0, nullptr, port))
{ }

endpoint_list_t::endpoint_list_t(all_interfaces_t const&, unsigned int port)
: head_(make_head(AI_PASSIVE, nullptr, port))
{ }

endpoint_list_t::endpoint_list_t(char const* host, unsigned int port)
: head_(make_head(0, host, port))
{ }

endpoint_list_t::endpoint_list_t(std::string const& host, unsigned int port)
: endpoint_list_t(host.c_str(), port)
{ }

} // xes

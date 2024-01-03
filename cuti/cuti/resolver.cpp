/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#include "resolver.hpp"

#include "system_error.hpp"

#include <cassert>
#include <cstring>
#include <memory>
#include <utility>

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

namespace cuti
{

namespace // anonymous
{

std::shared_ptr<addrinfo const>
make_addrinfo(int flags, char const* host, unsigned int port)
{
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
    builder << " port " << port << ": ";
#ifdef _WIN32
    builder << error_status_t(cause);
#else
    builder << gai_strerror(r);
#endif
    builder.explode();
  }

  return std::shared_ptr<addrinfo const>(head, freeaddrinfo);
}

} // anonymous

/*
 * The functions in resolver_t have access to a dangerous, private
 * endpoint_t constructor; this is the only reason resolver_t exists.
 */
struct resolver_t
{
  static endpoint_t resolve_ip(char const* ip, unsigned int port)
  {
    assert(ip != nullptr);

    std::shared_ptr<addrinfo const> info =
      make_addrinfo(AI_NUMERICHOST, ip, port);
    assert(info != nullptr);
    assert(info->ai_next == nullptr);

    std::shared_ptr<sockaddr const> addr(std::move(info), info->ai_addr);
    return endpoint_t(std::move(addr));
  }

  static endpoints_t find_endpoints(
    int flags, char const* host, unsigned int port)
  {
    std::shared_ptr<addrinfo const> info = make_addrinfo(flags, host, port);
    assert(info != nullptr);

    endpoints_t result;
    for(addrinfo const* node = info.get();
        node != nullptr;
        node = node->ai_next)
    {
      std::shared_ptr<sockaddr const> addr(info, node->ai_addr);
      result.push_back(endpoint_t(std::move(addr)));
    }
    return result;
  }
};

endpoint_t resolve_ip(char const* ip, unsigned int port)
{
  return resolver_t::resolve_ip(ip, port);
}

endpoint_t resolve_ip(std::string const& ip, unsigned int port)
{
  return resolve_ip(ip.c_str(), port);
}

endpoints_t resolve_host(char const* host, unsigned int port)
{
  assert(host != nullptr);
  return resolver_t::find_endpoints(0, host, port);
}

endpoints_t resolve_host(std::string const& host, unsigned int port)
{
  return resolve_host(host.c_str(), port);
}

endpoints_t local_interfaces(unsigned int port)
{
  return resolver_t::find_endpoints(0, nullptr, port);
}

endpoints_t all_interfaces(unsigned int port)
{
  return resolver_t::find_endpoints(AI_PASSIVE, nullptr, port);
}

} // cuti

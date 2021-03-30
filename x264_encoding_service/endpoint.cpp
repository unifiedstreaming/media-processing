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

#include "endpoint.hpp"

#include "system_error.hpp"

#include <cassert>
#include <ostream>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

namespace xes
{

namespace // anonymous
{

union endpoint_storage_t
{
  sockaddr addr_;
  sockaddr_in addr_in_;
  sockaddr_in6 addr_in6_;
};

template<typename IPv4Handler, typename IPv6Handler>
void visit_endpoint(endpoint_t const& endpoint,
                    IPv4Handler ipv4_handler,
                    IPv6Handler ipv6_handler)
{
  switch(check_family(endpoint.sa_family))
  {
  case AF_INET :
    ipv4_handler(*reinterpret_cast<sockaddr_in const*>(&endpoint));
    break;
  case AF_INET6 :
    ipv6_handler(*reinterpret_cast<sockaddr_in6 const*>(&endpoint));
    break;
  default :
    assert(!"expected address family");
    break;
  }
}

} // anonymous

int check_family(int family)
{
  switch(family)
  {
  case AF_INET :
  case AF_INET6 :
    break;
  default :
    {
      system_exception_builder_t builder;
      builder << "Unsupported address family " << family;
      builder.explode();
    }
    break;
  }

  return family;
}

int endpoint_family(endpoint_t const& endpoint)
{
  int result = AF_UNSPEC;

  auto on_ipv4 = [&](sockaddr_in const&) { result = AF_INET; };
  auto on_ipv6 = [&](sockaddr_in6 const&) { result = AF_INET6; };
  visit_endpoint(endpoint, on_ipv4, on_ipv6);
  
  return result;
}

unsigned int endpoint_size(endpoint_t const& endpoint)
{
  unsigned int result = 0;

  auto on_ipv4 = [&](sockaddr_in const& addr) { result = sizeof addr; };
  auto on_ipv6 = [&](sockaddr_in6 const& addr) { result = sizeof addr; };
  visit_endpoint(endpoint, on_ipv4, on_ipv6);
  
  return result;
}

std::string ip_address(endpoint_t const& endpoint)
{
  static char const longest_expected[] =
    "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255";
  char result[sizeof longest_expected]; 

  int r = getnameinfo(
    &endpoint, static_cast<socklen_t>(endpoint_size(endpoint)),
    result, static_cast<socklen_t>(sizeof result),
    nullptr, 0,
    NI_NUMERICHOST);

  if(r != 0)
  {
#ifdef _WIN32
    int cause = last_system_error();
#endif
    system_exception_builder_t builder;
    builder << "Can't determine IP address";
#ifdef _WIN32
    builder.explode(cause);
#else
    builder << ": " << gai_strerror(r);
    builder.explode();
#endif
  }

  return result;
}

unsigned int port_number(endpoint_t const& endpoint)
{
  unsigned int result = 0;

  auto on_ipv4 = [&](sockaddr_in const& addr)
    { result = ntohs(addr.sin_port); };
  auto on_ipv6 = [&](sockaddr_in6 const& addr)
    { result = ntohs(addr.sin6_port); };
  visit_endpoint(endpoint, on_ipv4, on_ipv6);

  return result;
}

std::shared_ptr<endpoint_t const> local_endpoint(int fd)
{
  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getsockname(fd, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("getsockname() failure", cause);
  }
  
  check_family(storage->addr_.sa_family);

  return std::shared_ptr<endpoint_t const>(
    std::move(storage), &storage->addr_);
}
  
std::shared_ptr<endpoint_t const> remote_endpoint(int fd)
{
  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getpeername(fd, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("getpeername() failure", cause);
  }
  
  check_family(storage->addr_.sa_family);

  return std::shared_ptr<endpoint_t const>(
    std::move(storage), &storage->addr_);
}

std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint)
{
  os << '[' << port_number(endpoint) << '@' << ip_address(endpoint) << ']';
  return os;
}

} // namespace xes

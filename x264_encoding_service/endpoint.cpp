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

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <limits>
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

#ifndef AI_IDN
#define AI_IDN 0
#endif

namespace xes
{

namespace // anonymous
{

void check_family(int family)
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
}

template<typename IPv4Handler, typename IPv6Handler>
void visit_sockaddr(sockaddr const& addr,
                    IPv4Handler ipv4_handler,
                    IPv6Handler ipv6_handler)
{
  switch(addr.sa_family)
  {
  case AF_INET :
    ipv4_handler(*reinterpret_cast<sockaddr_in const*>(&addr));
    break;
  case AF_INET6 :
    ipv6_handler(*reinterpret_cast<sockaddr_in6 const*>(&addr));
    break;
  default :
    assert(!"expected address family");
    break;
  }
}

std::shared_ptr<addrinfo const>
make_addrinfo(int flags, char const* host, unsigned int port)
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

  return std::shared_ptr<addrinfo const>(head, freeaddrinfo);
}

endpoints_t find_endpoints(int flags, char const* host, unsigned int port)
{
  std::shared_ptr<addrinfo const> info = make_addrinfo(flags, host, port);
  assert(info != nullptr);

  endpoints_t result;
  for(addrinfo const* node = info.get(); node != nullptr; node = node->ai_next)
  {
    std::shared_ptr<sockaddr const> addr(info, node->ai_addr);
    result.emplace_back(std::move(addr));
  }
  return result;
}

} // anonymous

endpoint_t::endpoint_t(std::shared_ptr<sockaddr const> addr)
: addr_(std::move(addr))
{
  if(addr_ != nullptr)
  {
    check_family(addr_->sa_family);
  }
}

int endpoint_t::address_family() const
{
  assert(!empty());

  return addr_->sa_family;
}

sockaddr const& endpoint_t::socket_address() const
{
  assert(!empty());

  return *addr_;
}

unsigned int endpoint_t::socket_address_size() const
{
  assert(!empty());

  unsigned int result = 0;

  auto on_ipv4 = [&](sockaddr_in const& addr) { result = sizeof addr; };
  auto on_ipv6 = [&](sockaddr_in6 const& addr) { result = sizeof addr; };
  visit_sockaddr(*addr_, on_ipv4, on_ipv6);
  
  return result;
}

std::string endpoint_t::ip_address() const
{
  assert(!empty());

  static char const longest_expected[] =
    "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255";
  char buf[sizeof longest_expected]; 

  int r = getnameinfo(
    addr_.get(), this->socket_address_size(),
    buf, static_cast<socklen_t>(sizeof buf),
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

  char* end = std::find(buf, buf + sizeof buf, '\0');
  assert(end != buf + sizeof buf);

  return std::string(buf, end);
}

unsigned int endpoint_t::port() const
{
  assert(!empty());

  unsigned int result = 0;

  auto on_ipv4 = [&](sockaddr_in const& addr)
    { result = ntohs(addr.sin_port); };
  auto on_ipv6 = [&](sockaddr_in6 const& addr)
    { result = ntohs(addr.sin6_port); };
  visit_sockaddr(*addr_, on_ipv4, on_ipv6);

  return result;
}

endpoint_t resolve_ip(char const* ip, unsigned int port)
{
  assert(ip != nullptr);

  std::shared_ptr<addrinfo const> info =
    make_addrinfo(AI_NUMERICHOST, ip, port);
  assert(info != nullptr);
  assert(info->ai_next == nullptr);

  std::shared_ptr<sockaddr const> addr(std::move(info), info->ai_addr);
  return endpoint_t(std::move(addr));
}

endpoint_t resolve_ip(std::string const& ip, unsigned int port)
{
  return resolve_ip(ip.c_str(), port);
}

endpoints_t resolve_host(char const* host, unsigned int port)
{
  assert(host != nullptr);
  return find_endpoints(0, host, port);
}

endpoints_t resolve_host(std::string const& host, unsigned int port)
{
  return resolve_host(host.c_str(), port);
}

endpoints_t local_interfaces(unsigned int port)
{
  return find_endpoints(0, nullptr, port);
}

endpoints_t all_interfaces(unsigned int port)
{
  return find_endpoints(AI_PASSIVE, nullptr, port);
}

std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint)
{
  if(endpoint.empty())
  {
    os << "[EMPTY ENDPOINT]";
  }
  else
  {
    os << '[' << endpoint.port() << '@' << endpoint.ip_address() << ']';
  }

  return os;
}

} // namespace xes

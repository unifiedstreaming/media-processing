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

union endpoint_storage_t
{
  sockaddr addr_;
  sockaddr_in addr_in_;
  sockaddr_in6 addr_in6_;
};

template<typename IPv4Handler, typename IPv6Handler>
void visit_sockaddr(sockaddr const& addr,
                    IPv4Handler ipv4_handler,
                    IPv6Handler ipv6_handler)
{
  switch(endpoint_t::check_address_family(addr.sa_family))
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

  return std::shared_ptr<addrinfo const>(head, freeaddrinfo);
}

} // anonymous

endpoint_t::endpoint_t(char const* ip_address, unsigned int port)
: addr_(resolve_ip(ip_address, port))
{ }

endpoint_t::endpoint_t(std::string const& ip_address, unsigned int port)
: addr_(resolve_ip(ip_address.c_str(), port))
{ }

endpoint_t::endpoint_t(std::shared_ptr<sockaddr const> addr)
: addr_(std::move(addr))
{ }

std::vector<endpoint_t>
endpoint_t::resolve(char const* host, unsigned int port)
{
  assert(host != nullptr);
  return resolve_multiple(0, host, port);
}

std::vector<endpoint_t>
endpoint_t::resolve(std::string const& host, unsigned int port)
{
  return resolve_multiple(0, host.c_str(), port);
}

std::vector<endpoint_t>
endpoint_t::local_interfaces(unsigned int port)
{
  return resolve_multiple(0, nullptr, port);
}

std::vector<endpoint_t>
endpoint_t::all_interfaces(unsigned int port)
{
  return resolve_multiple(AI_PASSIVE, nullptr, port);
}

int endpoint_t::address_family() const
{
  assert(!empty());

  int result = AF_UNSPEC;

  auto on_ipv4 = [&](sockaddr_in const&) { result = AF_INET; };
  auto on_ipv6 = [&](sockaddr_in6 const&) { result = AF_INET6; };
  visit_sockaddr(*addr_, on_ipv4, on_ipv6);
  
  return result;
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
  char result[sizeof longest_expected]; 

  int r = getnameinfo(
    addr_.get(), this->socket_address_size(),
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

int endpoint_t::check_address_family(int family)
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

endpoint_t endpoint_t::local_endpoint(int fd)
{
  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getsockname(fd, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("getsockname() failure", cause);
  }
  
  check_address_family(storage->addr_.sa_family);

  std::shared_ptr<sockaddr const> addr(std::move(storage), &storage->addr_);
  return endpoint_t(std::move(addr));
}

endpoint_t endpoint_t::remote_endpoint(int fd)
{
  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getpeername(fd, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("getpeername() failure", cause);
  }
  
  check_address_family(storage->addr_.sa_family);

  std::shared_ptr<sockaddr const> addr(std::move(storage), &storage->addr_);
  return endpoint_t(std::move(addr));
}

std::shared_ptr<sockaddr const>
endpoint_t::resolve_ip(char const* ip, unsigned int port)
{
  assert(ip != nullptr);

  std::shared_ptr<addrinfo const> head = make_head(AI_NUMERICHOST, ip, port);
  assert(head != nullptr);
  assert(head->ai_next == nullptr);

  return std::shared_ptr<sockaddr const>(std::move(head), head->ai_addr);
}

std::vector<endpoint_t>
endpoint_t::resolve_multiple(int flags, char const* host, unsigned int port)
{
  std::shared_ptr<addrinfo const> head = make_head(flags, host, port);
  assert(head != nullptr);

  std::vector<endpoint_t> result;
  for(addrinfo const* node = head.get(); node != nullptr; node = node->ai_next)
  {
    std::shared_ptr<sockaddr const> addr(head, node->ai_addr);
    result.push_back(endpoint_t(std::move(addr)));
  }
  return result;
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

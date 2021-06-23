/*
 * Copyright (C) 2021 CodeShop B.V.
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

#include "endpoint.hpp"

#include "args_reader.hpp"
#include "resolver.hpp"
#include "system_error.hpp"

#include <algorithm>
#include <array>
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

namespace cuti
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

bool sockaddr_equals(sockaddr_in const& lhs, sockaddr_in const& rhs) noexcept
{
  return lhs.sin_port == rhs.sin_port &&
         lhs.sin_addr.s_addr == rhs.sin_addr.s_addr;
}

bool sockaddr_equals(sockaddr_in6 const& lhs, sockaddr_in6 const& rhs) noexcept
{
  return lhs.sin6_port == rhs.sin6_port &&
         std::equal(std::begin(lhs.sin6_addr.s6_addr),
                    std::end(lhs.sin6_addr.s6_addr),
                    std::begin(rhs.sin6_addr.s6_addr));
}

bool sockaddr_equals(sockaddr_in const& lhs, sockaddr const& rhs) noexcept
{
  bool result = false;

  auto on_ipv4 = [&](sockaddr_in const& rhs)
    { result = sockaddr_equals(lhs, rhs); };
  auto on_ipv6 = [&](sockaddr_in6 const&)
    { };
  visit_sockaddr(rhs, on_ipv4, on_ipv6);

  return result;
}

bool sockaddr_equals(sockaddr_in6 const& lhs, sockaddr const& rhs) noexcept
{
  bool result = false;

  auto on_ipv4 = [&](sockaddr_in const&)
    { };
  auto on_ipv6 = [&](sockaddr_in6 const& rhs)
    { result = sockaddr_equals(lhs, rhs); };
  visit_sockaddr(rhs, on_ipv4, on_ipv6);

  return result;
}

bool sockaddr_equals(sockaddr const& lhs, sockaddr const& rhs) noexcept
{
  bool result = false;

  auto on_ipv4 = [&](sockaddr_in const& lhs)
    { result = sockaddr_equals(lhs, rhs); };
  auto on_ipv6 = [&](sockaddr_in6 const& lhs)
    { result = sockaddr_equals(lhs, rhs); };
  visit_sockaddr(lhs, on_ipv4, on_ipv6);

  return result;
}

} // anonymous

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

bool endpoint_t::equals(endpoint_t const& that) const noexcept
{
  if(this->addr_ == that.addr_)
  {
    return true;
  }
  if(this->addr_ == nullptr || that.addr_ == nullptr)
  {
    return false;
  }

  return sockaddr_equals(*this->addr_, *that.addr_);
}

endpoint_t::endpoint_t(std::shared_ptr<sockaddr const> addr)
: addr_(std::move(addr))
{
  if(addr_ != nullptr)
  {
    check_family(addr_->sa_family);
  }
}

std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint)
{
  if(endpoint.empty())
  {
    os << "<EMPTY ENDPOINT>";
  }
  else
  {
    os << endpoint.port() << '@' << endpoint.ip_address();
  }

  return os;
}

void parse_optval(char const* name, args_reader_t const& reader,
                  char const* in, endpoint_t& out)
{
  unsigned int port = 0;
  do
  {
    if(*in < '0' || *in > '9')
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": digit expected in port number for option '" << name << "'";
      builder.explode();
    }

    unsigned int dval = *in - '0';
    if(port > max_port / 10 || 10 * port > max_port - dval)
    {
      system_exception_builder_t builder;
      builder << reader.current_origin() <<
        ": maximum port number (" << max_port <<
        ") exceeded for option '" << name << "'";
      builder.explode();
    }

    port *= 10;
    port += dval;

    ++in;
  } while(*in != '@');

  ++in;
  try
  {
    out = resolve_ip(in, port);
  }
  catch(std::exception const& ex)
  {
    system_exception_builder_t builder;
    builder << reader.current_origin() <<
      ": error resolving IP address '" << in <<
      "' for option '" << name << "': " << ex.what();
    builder.explode();
  }
}
    
} // cuti

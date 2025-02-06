/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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
#include "socket_layer.hpp"
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

struct endpoint_t::rep_t
{
  rep_t()
  { }

  rep_t(rep_t const&) = delete;
  rep_t& operator=(rep_t const&) = delete;

  virtual int address_family() const = 0;
  virtual sockaddr const& socket_address() const = 0;
  virtual unsigned int socket_address_size() const = 0;
  virtual std::string const& ip_address() const = 0;
  virtual unsigned int port() const = 0;

  virtual ~rep_t();
};
  
namespace // anonymous
{

std::string get_ip_address(socket_layer_t& sockets,
                           sockaddr const& addr, unsigned int addr_size)
{
  static char const longest_expected[] =
    "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255";
  char buf[sizeof longest_expected];

  int r = sockets.getnameinfo(
    &addr, addr_size, buf, sizeof buf,
    nullptr, 0, NI_NUMERICHOST);

  if(r != 0)
  {
#ifdef _WIN32
    int cause = last_system_error();
#endif
    system_exception_builder_t builder;
    builder << "Can't determine IP address: ";
#ifdef _WIN32
    builder << error_status_t(cause);
#else
    builder << sockets.gai_strerror(r);
#endif
    builder.explode();
  }

  char* end = std::find(buf, buf + sizeof buf, '\0');
  assert(end != buf + sizeof buf);

  return std::string(buf, end);
}

template<typename AddrT>
std::string get_ip_address(socket_layer_t& sockets, AddrT const& addr)
{
  return get_ip_address(
    sockets, *reinterpret_cast<sockaddr const*>(&addr), sizeof addr);
}

struct ipv4_rep_t : endpoint_t::rep_t
{
  ipv4_rep_t(socket_layer_t& sockets, sockaddr_in const& addr)
  : endpoint_t::rep_t()
  , addr_(addr)
  , ip_address_(get_ip_address(sockets, addr_))
  { }

  int address_family() const override
  { return AF_INET; }

  sockaddr const& socket_address() const override
  { return *reinterpret_cast<sockaddr const*>(&addr_); }

  unsigned int socket_address_size() const override
  { return sizeof addr_; }

  std::string const& ip_address() const override
  { return ip_address_; }

  unsigned int port() const override
  { return ntohs(addr_.sin_port); }

private :
  sockaddr_in addr_;
  std::string ip_address_;
};

struct ipv6_rep_t : endpoint_t::rep_t
{
  ipv6_rep_t(socket_layer_t& sockets, sockaddr_in6 const& addr)
  : endpoint_t::rep_t()
  , addr_(addr)
  , ip_address_(get_ip_address(sockets, addr_))
  { }

  int address_family() const override
  { return AF_INET6; }

  sockaddr const& socket_address() const override
  { return *reinterpret_cast<sockaddr const*>(&addr_); }

  unsigned int socket_address_size() const override
  { return sizeof addr_; }

  std::string const& ip_address() const override
  { return ip_address_; }

  unsigned int port() const override
  { return ntohs(addr_.sin6_port); }

private :
  sockaddr_in6 addr_;
  std::string ip_address_;
};

std::shared_ptr<endpoint_t::rep_t const>
make_rep(socket_layer_t& sockets, sockaddr const& addr, std::size_t addr_size)
{
  std::shared_ptr<endpoint_t::rep_t const> result;

  switch(addr.sa_family)
  {
  case AF_INET:
    if(addr_size != sizeof(sockaddr_in))
    {
      system_exception_builder_t builder;
      builder << "Bad sockaddr size " << addr_size <<
        " for address family AF_INET (" << sizeof(sockaddr_in) <<
        " expected)";
      builder.explode();
    }
    result = std::make_shared<ipv4_rep_t const>(sockets,
      *reinterpret_cast<sockaddr_in const*>(&addr));
    break;
  case AF_INET6:
    if(addr_size != sizeof(sockaddr_in6))
    {
      system_exception_builder_t builder;
      builder << "Bad sockaddr size " << addr_size <<
        " for address family AF_INET6 (" << sizeof(sockaddr_in6) <<
        " expected)";
      builder.explode();
    }
    result = std::make_shared<ipv6_rep_t const>(sockets,
      *reinterpret_cast<sockaddr_in6 const*>(&addr));
    break;
  default:
    {
      system_exception_builder_t builder;
      builder << "Unsupported address family " << addr.sa_family;
      builder.explode();
    }
    break;
  }

  return result;
}

} // anonymous

endpoint_t::rep_t::~rep_t()
{ }

int endpoint_t::address_family() const
{
  assert(!empty());
  return rep_->address_family();
}

sockaddr const& endpoint_t::socket_address() const
{
  assert(!empty());
  return rep_->socket_address();
}

unsigned int endpoint_t::socket_address_size() const
{
  assert(!empty());
  return rep_->socket_address_size();
}

std::string const& endpoint_t::ip_address() const
{
  assert(!empty());
  return rep_->ip_address();
}

unsigned int endpoint_t::port() const
{
  assert(!empty());
  return rep_->port();
}

bool endpoint_t::equals(endpoint_t const& that) const noexcept
{
  if(this->rep_ == that.rep_)
  {
    return true;
  }

  if(this->rep_ == nullptr || that.rep_ == nullptr)
  {
    return false;
  }

  return
     this->port() == that.port() &&
     this->ip_address() == that.ip_address() &&
     this->address_family() == that.address_family();
}

endpoint_t::endpoint_t(socket_layer_t& sockets,
                       sockaddr const& addr, std::size_t addr_size)
: rep_(make_rep(sockets, addr, addr_size))
{ }

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

void parse_endpoint(socket_layer_t& sockets,
  char const* name, args_reader_t const& reader,
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
    out = resolve_ip(sockets, in, port);
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

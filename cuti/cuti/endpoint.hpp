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

#ifndef CUTI_ENDPOINT_HPP_
#define CUTI_ENDPOINT_HPP_

#include "linkage.h"

#include <cstddef>
#include <memory>
#include <iosfwd>
#include <string>

struct sockaddr;

namespace cuti
{

struct args_reader_t;
struct resolver_t;
struct socket_layer_t;
struct tcp_socket_t;

struct CUTI_ABI endpoint_t
{
  struct rep_t;

  // Constructs an empty endpoint; access verboten. To obtain real,
  // non-empty endpoints, use the factory functions in resolver.hpp.
  endpoint_t()
  : rep_(nullptr)
  { }

  // Accessors: no properties exist when this->empty()
  bool empty() const
  { return rep_ == nullptr; }

  int address_family() const;
  sockaddr const& socket_address() const;
  unsigned int socket_address_size() const;
  std::string const& ip_address() const;
  unsigned int port() const;

  bool equals(endpoint_t const& that) const noexcept;

private :
  friend struct resolver_t;
  friend struct tcp_socket_t;

  // Constructs an endpoint from a socket address
  endpoint_t(socket_layer_t& sockets,
    sockaddr const& addr, std::size_t addr_size);

private :
  std::shared_ptr<rep_t const> rep_;
};

CUTI_ABI
inline bool operator==(endpoint_t const& lhs, endpoint_t const& rhs) noexcept
{ return lhs.equals(rhs); }

CUTI_ABI
inline bool operator!=(endpoint_t const& lhs, endpoint_t const& rhs) noexcept
{ return !(lhs == rhs); }

CUTI_ABI
std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint);

CUTI_ABI void parse_endpoint(socket_layer_t& sockets,
  char const* name, args_reader_t const& reader,
  char const* in, endpoint_t& out);

} // namespace cuti

#endif

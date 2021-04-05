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

#ifndef XES_ENDPOINT_HPP_
#define XES_ENDPOINT_HPP_

#include "socket_nifty.hpp"

#include <memory>
#include <iosfwd>
#include <string>

struct sockaddr;

namespace xes
{

struct resolver_t;
struct tcp_socket_t;

struct endpoint_t
{
  // Constructs an empty endpoint; access verboten. To obtain real,
  // non-empty endpoints, use the factory functions in resolver.hpp.
  endpoint_t()
  : addr_(nullptr)
  { }

  // Accessors: no properties exist when this->empty()
  bool empty() const
  { return addr_ == nullptr; }

  int address_family() const;
  sockaddr const& socket_address() const;
  unsigned int socket_address_size() const;
  std::string ip_address() const;
  unsigned int port() const;

private :
  friend struct resolver_t;
  friend struct tcp_socket_t;

  // Constructs an endpoint from a socket address
  explicit endpoint_t(std::shared_ptr<sockaddr const> addr);

private :
  std::shared_ptr<sockaddr const> addr_;
};

std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint);

} // namespace xes

#endif

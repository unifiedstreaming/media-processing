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
#include <utility>
#include <vector>

struct sockaddr;

namespace xes
{

struct tcp_socket_t;

unsigned int const any_port = 0;

struct endpoint_t
{
  // Constructs an empty endpoint; access verboten
  endpoint_t()
  : addr_(nullptr)
  { }

  // Constructs an endpoint for an ip address and port number
  endpoint_t(char const* ip_address, unsigned int port);
  endpoint_t(std::string const& ip_address, unsigned int port);

  // Finds endpoints for a host name and port number
  static std::vector<endpoint_t>
  resolve(char const* host, unsigned int port);

  static std::vector<endpoint_t>
  resolve(std::string const& host, unsigned int port);

  // Returns endpoints for binding to local interfaces
  static std::vector<endpoint_t>
  local_interfaces(unsigned int port);

  // Returns endpoints for binding to all interfaces
  static std::vector<endpoint_t>
  all_interfaces(unsigned int port);

  // Accessors: no properties exist when this->empty()
  bool empty() const
  { return addr_ == nullptr; }

  int address_family() const;
  sockaddr const& socket_address() const;
  unsigned int socket_address_size() const;
  std::string ip_address() const;
  unsigned int port() const;

  // Checks if family is supported and then returns it; throws if not
  static int check_address_family(int family);

private :
  friend struct tcp_socket_t;

  static endpoint_t local_endpoint(int fd);
  static endpoint_t remote_endpoint(int fd);
  
private :
  explicit endpoint_t(std::shared_ptr<sockaddr const> addr);

  static std::shared_ptr<sockaddr const>
  resolve_ip(char const* ip, unsigned int port);

  static std::vector<endpoint_t>
  resolve_multiple(int flags, char const* host, unsigned int port);

private :
  std::shared_ptr<sockaddr const> addr_;
};
  
std::ostream& operator<<(std::ostream& os, endpoint_t const& endpoint);

} // namespace xes

#endif

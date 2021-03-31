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

#ifndef XES_TCP_CONNECTION_HPP_
#define XES_TCP_CONNECTION_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"
#include "tcp_socket.hpp"

#include <iosfwd>
#include <memory>
#include <utility>

namespace xes
{

struct tcp_acceptor_t;

struct tcp_connection_t
{
  explicit tcp_connection_t(endpoint_t const& peer);

  tcp_connection_t(tcp_connection_t const&) = delete;
  tcp_connection_t& operator=(tcp_connection_t const&) = delete;

  endpoint_t const& local_endpoint() const
  { return *local_endpoint_; }
  
  endpoint_t const& remote_endpoint() const
  { return *remote_endpoint_; }

  /*
   * I/O functions
   */
  // In blocking mode, which is the default, I/O functions wait
  // until they can be completed.
  void set_blocking();

  // In non-blocking mode, I/O functions return some special
  // value if they cannot be completed immediately.
  void set_nonblocking();

  // Returns a pointer to the next byte to write.
  // In non-blocking mode, nullptr may be returned.
  char const* write_some(char const* first, char const* last);

  // Returns a pointer to the next byte to read; first on EOF.
  // In non-blocking mode, nullptr may be returned.
  char* read_some(char* first, char* last);

private :
  friend struct tcp_acceptor_t;
  explicit tcp_connection_t(tcp_socket_t&& socket); 
  
private :
  tcp_socket_t socket_;
  std::shared_ptr<endpoint_t const> local_endpoint_;
  std::shared_ptr<endpoint_t const> remote_endpoint_;
};

std::ostream& operator<<(std::ostream& os, tcp_connection_t const& connection);

std::pair<std::unique_ptr<tcp_connection_t>,
          std::unique_ptr<tcp_connection_t>>
make_connected_pair(endpoint_t const& interface);

std::pair<std::unique_ptr<tcp_connection_t>,
          std::unique_ptr<tcp_connection_t>>
make_connected_pair();

} // xes

#endif

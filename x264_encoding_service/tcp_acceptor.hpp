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

#ifndef XES_TCP_ACCEPTOR_HPP_
#define XES_TCP_ACCEPTOR_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"
#include "tcp_connection.hpp"
#include "tcp_socket.hpp"

#include <memory>
#include <iosfwd>

namespace xes
{

struct tcp_acceptor_t
{
  explicit tcp_acceptor_t(endpoint_t const& endpoint);

  tcp_acceptor_t(tcp_acceptor_t const&) = delete;
  tcp_acceptor_t& operator=(tcp_acceptor_t const&) = delete;
  
  endpoint_t const& local_endpoint() const
  { return local_endpoint_; }
  
  /*
   * I/O functions
   */
  // In blocking mode, which is the default, I/O functions wait
  // until they can be completed.
  void set_blocking();

  // In non-blocking mode, I/O functions return some special
  // value if they cannot be completed immediately.
  void set_nonblocking();

  // Returns an accepted connection.
  // In non-blocking mode, nullptr may be returned.
  std::unique_ptr<tcp_connection_t> accept();

private :
  tcp_socket_t socket_;
  endpoint_t local_endpoint_;
};

std::ostream& operator<<(std::ostream& os, tcp_acceptor_t const& acceptor);

} // xes

#endif

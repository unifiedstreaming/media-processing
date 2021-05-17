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

#ifndef CUTI_TCP_CONNECTION_HPP_
#define CUTI_TCP_CONNECTION_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "scheduler.hpp"
#include "socket_nifty.hpp"
#include "tcp_socket.hpp"

#include <iosfwd>
#include <memory>
#include <utility>

namespace cuti
{

struct tcp_acceptor_t;

struct CUTI_ABI tcp_connection_t
{
  explicit tcp_connection_t(endpoint_t const& peer);

  tcp_connection_t(tcp_connection_t const&) = delete;
  tcp_connection_t& operator=(tcp_connection_t const&) = delete;

  endpoint_t const& local_endpoint() const
  { return local_endpoint_; }

  endpoint_t const& remote_endpoint() const
  { return remote_endpoint_; }

  /*
   * In blocking mode, which is the default, I/O functions wait
   * until they can be completed.
   * In non-blocking mode, I/O functions return immediately; please
   * see the descriptions of write() and read().
   */
  void set_blocking();
  void set_nonblocking();

  /*
   * Tries to write the bytes in range [first, last>, setting next to
   * the next byte to write.  In non-blocking mode, next is set to
   * nullptr if the call would block.
   * Returns 0 on success; if the connection is broken, next is set to
   * last and a system error code is returned.  Please note that
   * refusing to block is not an error.
   */
  int write(char const* first, char const* last, char const*& next);

  /*
   * Closes the writing side of the connection, while leaving the
   * reading side open.  This should eventually result in an EOF at
   * the peer.
   * Returns 0 on success; if the connection is broken, a system error
   * code is returned.
   */
  int close_write_end();

  /*
   * Tries to read the bytes in range [first, last>, setting next to
   * the next byte to read, or first on EOF.  In non-blocking mode,
   * next is set to nullptr if the call would block.
   * Returns 0 on success; if the connection is broken, next is set to
   * first and a system error code is returned.  Please note that
   * hitting EOF or refusing to block is not an error.
   */
  int read(char* first, char const* last, char*& next);

  /*
   * Event reporting; see scheduler.hpp for detailed semantics.  A
   * callback can be canceled by calling cancel() directly on the
   * scheduler.
   */
  template<typename Callback>
  cancellation_ticket_t call_when_writable(scheduler_t& scheduler,
                                           Callback&& callback) const
  {
    return socket_.call_when_writable(scheduler,
      std::forward<Callback>(callback));
  }

  template<typename Callback>
  cancellation_ticket_t call_when_readable(scheduler_t& scheduler,
                                           Callback&& callback) const
  {
    return socket_.call_when_readable(scheduler,
      std::forward<Callback>(callback));
  }

private :
  friend struct tcp_acceptor_t;
  explicit tcp_connection_t(tcp_socket_t&& socket);

private :
  tcp_socket_t socket_;
  endpoint_t local_endpoint_;
  endpoint_t remote_endpoint_;
};

CUTI_ABI std::ostream& operator<<(std::ostream& os,
                                  tcp_connection_t const& connection);

CUTI_ABI std::pair<std::unique_ptr<tcp_connection_t>,
                   std::unique_ptr<tcp_connection_t>>
make_connected_pair(endpoint_t const& interface);

CUTI_ABI std::pair<std::unique_ptr<tcp_connection_t>,
                   std::unique_ptr<tcp_connection_t>>
make_connected_pair();

} // cuti

#endif

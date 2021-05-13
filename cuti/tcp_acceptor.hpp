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

#ifndef CUTI_TCP_ACCEPTOR_HPP_
#define CUTI_TCP_ACCEPTOR_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "scheduler.hpp"
#include "socket_nifty.hpp"
#include "tcp_connection.hpp"
#include "tcp_socket.hpp"

#include <memory>
#include <iosfwd>
#include <utility>

namespace cuti
{

struct CUTI_ABI tcp_acceptor_t
{
  explicit tcp_acceptor_t(endpoint_t const& endpoint);

  tcp_acceptor_t(tcp_acceptor_t const&) = delete;
  tcp_acceptor_t& operator=(tcp_acceptor_t const&) = delete;

  endpoint_t const& local_endpoint() const
  { return local_endpoint_; }

  /*
   * In blocking mode, which is the default, I/O functions wait
   * until they can be completed.
   * In non-blocking mode, I/O functions return immediately; please
   * see the description of accept().
   */
  void set_blocking();
  void set_nonblocking();

  /*
   * Tries to accept an incoming connnection to *this, returning
   * a pointer to the accepted connection.
   * If *this is in non-blocking mode and the call would block, or if
   * the incoming connection broke before it was accepted, nullptr is
   * returned.
   * Use last_accept_error() if you need to determine the cause of a
   * nullptr return value.
   */ 
  std::unique_ptr<tcp_connection_t> accept();

  /*
   * Returns the system error code of the last non-throwing call to
   * accept(), or 0 if there was no error.  Please note that refusing
   * to block is not an error.
   */
  int last_accept_error() const noexcept
  { return last_accept_error_; }

  /*
   * Event reporting; see scheduler.hpp for detailed semantics.  A
   * callback can be canceled by calling cancel() directly on the
   * scheduler.
   */
  template<typename Callback>
  cancellation_ticket_t call_when_ready(scheduler_t& scheduler,
                                        Callback&& callback) const
  {
    return socket_.call_when_readable(scheduler,
      std::forward<Callback>(callback));
  }

private :
  tcp_socket_t socket_;
  endpoint_t local_endpoint_;
  int last_accept_error_;
};

CUTI_ABI std::ostream& operator<<(std::ostream& os,
                                  tcp_acceptor_t const& acceptor);

} // cuti

#endif

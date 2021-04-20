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
#include "io_event_managers.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"
#include "tcp_connection.hpp"
#include "tcp_socket.hpp"

#include <memory>
#include <iosfwd>

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

  /*
   * Event reporting
   */
  // Schedule a single callback for when <scheduler> detects the
  // acceptor is ready to accept a connection, cancelling any
  // previously set callback.  Spurious callbacks are rare, but
  // possible.
  template<typename F>
  void call_when_ready(F&& callback, io_scheduler_t& scheduler)
  { ready_event_manager_.set(std::forward<F>(callback), scheduler); }

  // Cancel any previously set callback for when the acceptor is ready
  // to accept a connection.
  void cancel_when_ready()
  { ready_event_manager_.reset(); }

private :
  tcp_socket_t socket_;
  endpoint_t local_endpoint_;
  readable_event_manager_t ready_event_manager_;
};

CUTI_ABI std::ostream& operator<<(std::ostream& os,
                                  tcp_acceptor_t const& acceptor);

} // cuti

#endif

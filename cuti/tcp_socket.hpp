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

#ifndef CUTI_TCP_SOCKET_HPP_
#define CUTI_TCP_SOCKET_HPP_

#include "endpoint.hpp"
#include "linkage.h"
#include "scheduler.hpp"
#include "socket_nifty.hpp"

#include <memory>
#include <utility>

namespace cuti
{

/*
 * Low-level interface for TCP sockets.
 *
 * tcp_socket_t is a move-only type; its instances may be empty(),
 * that is, not holding an open file descriptor. Only re-assignment
 * and destruction make sense in that state.
 */
struct CUTI_ABI tcp_socket_t
{
  tcp_socket_t() noexcept
  : fd_(-1)
  { }

  explicit tcp_socket_t(int family);

  tcp_socket_t(tcp_socket_t const&) = delete;
  tcp_socket_t& operator=(tcp_socket_t const&) = delete;

  tcp_socket_t(tcp_socket_t&& rhs) noexcept
  : fd_(rhs.fd_)
  { rhs.fd_ = -1; }

  tcp_socket_t& operator=(tcp_socket_t&& rhs) noexcept
  {
    tcp_socket_t tmp(std::move(rhs));
    this->swap(tmp);
    return *this;
  }

  bool empty() const noexcept
  { return fd_ == -1; }

  void swap(tcp_socket_t& that) noexcept
  {
    using std::swap;
    swap(this->fd_, that.fd_);
  }

  ~tcp_socket_t()
  {
    if(fd_ != -1)
    {
      close_fd(fd_);
    }
  }

  /*
   * Socket setup
   */
  void bind(endpoint_t const& endpoint);
  void listen();
  void connect(endpoint_t const& peer);

  endpoint_t local_endpoint() const;
  endpoint_t remote_endpoint() const;

  /*
   * In blocking mode, which is the default, I/O functions wait
   * until they can be completed.
   * In non-blocking mode, I/O functions return immediately; please
   * see the descriptions of accept(), write() and read().
   */
  void set_blocking();
  void set_nonblocking();

  /*
   * Tries to set <accepted> to an incoming connection socket.
   * If *this is in non-blocking mode and the call would block, or if
   * the incoming connection broke before it was accepted, <accepted>
   * is set to the empty state.
   * Returns 0 on success, or a system error code if the incoming
   * connection was broken.  Please note that refusing to block is
   * not an error.
   */
  int accept(tcp_socket_t& accepted);

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
   * code might be returned.
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
    assert(!this->empty());
    return scheduler.call_when_writable(fd_,
      std::forward<Callback>(callback));
  }

  template<typename Callback>
  cancellation_ticket_t call_when_readable(scheduler_t& scheduler,
                                           Callback&& callback) const
  {
    assert(!this->empty());
    return scheduler.call_when_readable(fd_,
      std::forward<Callback>(callback));
  }

private :
  static void close_fd(int fd) noexcept;

private :
  friend struct socket_initializer_t;
  static bool stops_sigpipe();

private :
  int fd_;
};

CUTI_ABI
inline void swap(tcp_socket_t& s1, tcp_socket_t& s2) noexcept
{
  s1.swap(s2);
}

} // cuti

#endif

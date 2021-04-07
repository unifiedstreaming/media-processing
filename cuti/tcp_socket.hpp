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
#include "socket_nifty.hpp"

#include <memory>

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
  {
    rhs.fd_ = -1;
  }

  tcp_socket_t& operator=(tcp_socket_t&& rhs) noexcept
  {
    tcp_socket_t tmp(std::move(rhs));
    this->swap(tmp);
    return *this;
  }
    
  bool empty() const noexcept
  {
    return fd_ == -1;
  }

  void swap(tcp_socket_t& other) noexcept
  {
    int tmp = fd_;
    fd_ = other.fd_;
    other.fd_ = tmp;
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
   * I/O functions
   */
  // In blocking mode, which is the default, I/O functions wait
  // until they can be completed.
  void set_blocking();

  // In non-blocking mode, I/O functions return some special
  // value if they cannot be completed immediately.
  void set_nonblocking();

  // Returns an accepted socket.
  // In non-blocking mode, an empty socket may be returned.
  tcp_socket_t accept();

  // Returns a pointer to the next byte to write.
  // In non-blocking mode, nullptr may be returned.
  char const* write_some(char const* first, char const* last);

  // Closes the writing side of the connection, while leaving the
  // reading side open. This will eventually result in an EOF at the
  // peer.
  void close_write_end();
  
  // Returns a pointer to the next byte to read; first on EOF.
  // In non-blocking mode, nullptr may be returned.
  char* read_some(char* first, char* last);

private :
  static void close_fd(int fd) noexcept;

private :
  friend struct socket_initializer_t;
  static bool stops_sigpipe();
  
private :
  int fd_;
};

inline
void swap(tcp_socket_t& s1, tcp_socket_t& s2) noexcept
{
  s1.swap(s2);
}

} // cuti

#endif

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

#ifndef TCP_SOCKET_HPP_
#define TCP_SOCKET_HPP_

#include "endpoint.hpp"
#include "socket_nifty.hpp"

#include <memory>

namespace xes
{

/*
 * Low-level interface for TCP sockets.
 *
 * tcp_socket_t is a move-only type; its instances may be empty(),
 * that is, not holding an open file descriptor. Only re-assignment
 * and destruction make sense in that state.
 */
struct tcp_socket_t
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

  std::shared_ptr<endpoint_t const> local_endpoint() const;
  std::shared_ptr<endpoint_t const> remote_endpoint() const;

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

} // xes

#endif
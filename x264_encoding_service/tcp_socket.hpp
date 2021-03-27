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

  void set_nonblocking(bool enable);
  void bind(endpoint_t const& endpoint);
  void listen();

  // Note: may return an empty socket, even in blocking mode
  tcp_socket_t try_accept();

private :
  static void close_fd(int fd) noexcept;

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

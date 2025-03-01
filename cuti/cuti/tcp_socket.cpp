/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include "tcp_socket.hpp"

#include "endpoint.hpp"
#include "io_utils.hpp"
#include "system_error.hpp"

#include <cassert>
#include <limits>
#include <string>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

namespace cuti
{

namespace // anonymous
{

union sockaddr_buffer_t
{
  sockaddr addr_;
  sockaddr_in addr_in_;
  sockaddr_in6 addr_in6_;
};

void set_v6only(socket_layer_t&, int fd, bool enable)
{
  const int optval = enable;
  int r = ::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                       reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting IPV6_V6ONLY: " << error_status_t(cause);
    builder.explode();
  }
}

void set_nodelay(socket_layer_t&, int fd, bool enable)
{
  const int optval = enable;
  int r = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                       reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting TCP_NODELAY: " << error_status_t(cause);
    builder.explode();
  }
}

void set_keepalive(socket_layer_t&, int fd, bool enable)
{
  const int optval = enable;
  int r = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                       reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting SO_KEEPALIVE: " << error_status_t(cause);
    builder.explode();
  }
}

#ifdef _WIN32

int to_fd(SOCKET sock)
{
  if(sock == INVALID_SOCKET)
  {
    return -1;
  }

  static SOCKET constexpr max = std::numeric_limits<int>::max();
  assert(sock >= 0);
  assert(sock <= max);
  return static_cast<int>(sock);
}

#else // POSIX

int to_fd(int fd)
{
  return fd;
}

void set_reuseaddr(socket_layer_t&, int fd, bool enable)
{
  const int optval = enable;
  int r = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting SO_REUSEADDR: " << error_status_t(cause);
    builder.explode();
  }
}

#endif // POSIX

#ifdef SO_NOSIGPIPE

void set_nosigpipe(socket_layer_t&, int fd, bool enable)
{
  const int optval = enable;
  int r = ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE,
                       reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting SO_NOSIGPIPE: " << error_status_t(cause);
    builder.explode();
  }
}

#endif // SO_NOSIGPIPE

void set_initial_connection_flags(socket_layer_t& sockets, int fd)
{
  set_nonblocking(sockets, fd, false);
  set_nodelay(sockets, fd, true);
  set_keepalive(sockets, fd, true);

#if defined(SO_NOSIGPIPE)
  set_nosigpipe(sockets, fd, true);
#endif
}

} // anonymous

tcp_socket_t::tcp_socket_t(socket_layer_t& sockets, int family)
: tcp_socket_t() // -> exceptions will not leak fd_
{
#if defined(SOCK_CLOEXEC)
  fd_ = to_fd(::socket(family, SOCK_STREAM | SOCK_CLOEXEC, 0));
#else
  fd_ = to_fd(::socket(family, SOCK_STREAM, 0));
#endif

  if(fd_ == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can'\t create socket: " << error_status_t(cause);
    builder.explode();
  }

  sockets_ = &sockets;

#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
  set_cloexec(*sockets_, fd_, true);
#endif
}

void tcp_socket_t::bind(endpoint_t const& endpoint)
{
  assert(!empty());

  if(endpoint.address_family() == AF_INET6)
  {
    set_v6only(*sockets_, fd_, true);
  }

#ifndef _WIN32
  set_reuseaddr(*sockets_, fd_, true);
#endif

  int r = ::bind(fd_,
                 &endpoint.socket_address(),
                 endpoint.socket_address_size());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can\'t bind to endpoint " << endpoint << ": " <<
      error_status_t(cause);
    builder.explode();
  }
}

void tcp_socket_t::listen()
{
  assert(!empty());

  int r = ::listen(fd_, SOMAXCONN);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can\'t listen: " << error_status_t(cause);
    builder.explode();
  }
}

void tcp_socket_t::connect(endpoint_t const& peer)
{
  assert(!empty());

  int r = ::connect(fd_, &peer.socket_address(), peer.socket_address_size());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can\'t connect to endpoint " << peer << ": " <<
      error_status_t(cause);
    builder.explode();
  }

  set_initial_connection_flags(*sockets_, fd_);
}

endpoint_t tcp_socket_t::local_endpoint() const
{
  assert(!empty());

  sockaddr_buffer_t buffer;
  socklen_t size = static_cast<socklen_t>(sizeof buffer);

  int r = ::getsockname(fd_, &buffer.addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "getsockname() failure: " << error_status_t(cause);
    builder.explode();
  }

  return endpoint_t(*sockets_, buffer.addr_, size);
}

endpoint_t tcp_socket_t::remote_endpoint() const
{
  assert(!empty());

  sockaddr_buffer_t buffer;
  socklen_t size = static_cast<socklen_t>(sizeof buffer);

  int r = ::getpeername(fd_, &buffer.addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "getpeername() failure: " << error_status_t(cause);
    builder.explode();
  }

  return endpoint_t(*sockets_, buffer.addr_, size);
}

void tcp_socket_t::set_blocking()
{
  assert(!empty());

  cuti::set_nonblocking(*sockets_, fd_, false);
}

void tcp_socket_t::set_nonblocking()
{
  assert(!empty());

  cuti::set_nonblocking(*sockets_, fd_, true);
}

int tcp_socket_t::accept(tcp_socket_t& accepted)
{
  assert(!empty());

  int result = 0;
  tcp_socket_t tmp_socket;

#if defined(SOCK_CLOEXEC)
  tmp_socket.fd_ = to_fd(::accept4(fd_, nullptr, nullptr, SOCK_CLOEXEC));
#else
  tmp_socket.fd_ = to_fd(::accept(fd_, nullptr, nullptr));
#endif

  if(tmp_socket.fd_ == -1)
  {
    int cause = last_system_error();
    if(!is_wouldblock(*sockets_, cause))
    {
      if(is_fatal_io_error(*sockets_, cause))
      {
        system_exception_builder_t builder;
        builder << "accept() failure: " << error_status_t(cause);
        builder.explode();
      }
      result = cause;
    }
  }
  else
  {
    tmp_socket.sockets_ = this->sockets_;

#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
    set_cloexec(*tmp_socket.sockets_, tmp_socket.fd_, true);
#endif
    set_initial_connection_flags(*tmp_socket.sockets_, tmp_socket.fd_);
  }

  tmp_socket.swap(accepted);
  return result;
}

int tcp_socket_t::write(char const* first, char const* last, char const*& next)
{
  assert(!empty());
  assert(first < last);

  int count = std::numeric_limits<int>::max();
  if(count > last - first)
  {
    count = static_cast<int>(last - first);
  }

  int result = 0;

#if defined(_WIN32) || defined(SO_NOSIGPIPE)
  auto n = ::send(fd_, first, count, 0);
#else
  auto n = ::send(fd_, first, count, MSG_NOSIGNAL);
#endif

  if(n == -1)
  {
    int cause = last_system_error();
    if(is_wouldblock(*sockets_, cause))
    {
      next = nullptr;
    }
    else if(is_fatal_io_error(*sockets_, cause))
    {
      system_exception_builder_t builder;
      builder << "send() failure: " << error_status_t(cause);
      builder.explode();
    }
    else
    {
      result = cause;
      next = last;
    }
  }
  else
  {
    next = first + n;
  }

  return result;
}

int tcp_socket_t::close_write_end() noexcept
{
  int result = 0;

#ifdef _WIN32
  int r = ::shutdown(fd_, SD_SEND);
#else
  int r = ::shutdown(fd_, SHUT_WR);
#endif
  if(r == -1)
  {
    result = last_system_error();
  }

  return result;
}

int tcp_socket_t::read(char* first, char const* last, char*& next)
{
  assert(!empty());
  assert(first < last);

  int count = std::numeric_limits<int>::max();
  if(count > last - first)
  {
    count = static_cast<int>(last - first);
  }

  auto result = 0;
  auto n = ::recv(fd_, first, count, 0);

  if(n == -1)
  {
    int cause = last_system_error();
    if(is_wouldblock(*sockets_, cause))
    {
      next = nullptr;
    }
    else if(is_fatal_io_error(*sockets_, cause))
    {
      system_exception_builder_t builder;
      builder << "recv() failure: " << error_status_t(cause);
      builder.explode();
    }
    else
    {
      result = cause;
      next = first;
    }
  }
  else
  {
     next = first + n;
  }

  return result;
}

void tcp_socket_t::close_fd(socket_layer_t&, int fd) noexcept
{
  assert(fd != -1);

#ifdef _WIN32
  ::closesocket(fd);
#else
  ::close(fd);
#endif
}

} // cuti

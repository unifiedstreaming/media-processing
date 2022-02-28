/*
 * Copyright (C) 2021-2022 CodeShop B.V.
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

union endpoint_storage_t
{
  sockaddr addr_;
  sockaddr_in addr_in_;
  sockaddr_in6 addr_in6_;
};

void set_v6only(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting IPV6_V6ONLY: " << error_status_t(cause);
    builder.explode();
  }
}

void set_nodelay(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting TCP_NODELAY: " << error_status_t(cause);
    builder.explode();
  }
}

void set_keepalive(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting SO_KEEPALVE: " << error_status_t(cause);
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

  assert(sock >= 0);
  assert(sock <= std::numeric_limits<int>::max());
  return static_cast<int>(sock);
}

#else // POSIX

int to_fd(int fd)
{
  return fd;
}

void set_reuseaddr(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
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

void set_nosigpipe(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE,
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

void set_initial_connection_flags(int fd)
{
  set_nonblocking(fd, false);
  set_nodelay(fd, true);
  set_keepalive(fd, true);

#if defined(SO_NOSIGPIPE)
  set_nosigpipe(fd, true);
#endif
}

} // anonymous

tcp_socket_t::tcp_socket_t(int family)
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

#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
  set_cloexec(fd_, true);
#endif
}

void tcp_socket_t::bind(endpoint_t const& endpoint)
{
  assert(!empty());

  if(endpoint.address_family() == AF_INET6)
  {
    set_v6only(fd_, true);
  }

#ifndef _WIN32
  set_reuseaddr(fd_, true);
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

  set_initial_connection_flags(fd_);
}

endpoint_t tcp_socket_t::local_endpoint() const
{
  assert(!empty());

  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getsockname(fd_, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "getsockname() failure: " << error_status_t(cause);
    builder.explode();
  }

  std::shared_ptr<sockaddr const> addr(std::move(storage), &storage->addr_);
  return endpoint_t(std::move(addr));
}

endpoint_t tcp_socket_t::remote_endpoint() const
{
  assert(!empty());

  auto storage = std::make_shared<endpoint_storage_t>();
  socklen_t size = static_cast<socklen_t>(sizeof *storage);

  int r = getpeername(fd_, &storage->addr_, &size);
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "getpeername() failure: " << error_status_t(cause);
    builder.explode();
  }

  std::shared_ptr<sockaddr const> addr(std::move(storage), &storage->addr_);
  return endpoint_t(std::move(addr));
}

void tcp_socket_t::set_blocking()
{
  assert(!empty());

  cuti::set_nonblocking(fd_, false);
}

void tcp_socket_t::set_nonblocking()
{
  assert(!empty());

  cuti::set_nonblocking(fd_, true);
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
    if(!is_wouldblock(cause))
    {
      if(is_fatal_io_error(cause))
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
#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
    set_cloexec(tmp_socket.fd_, true);
#endif
    set_initial_connection_flags(tmp_socket.fd_);
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

#if defined(SO_NOSIGPIPE) || !defined(MSG_NOSIGNAL)
  auto n = ::send(fd_, first, count, 0);
#else
  auto n = ::send(fd_, first, count, MSG_NOSIGNAL);
#endif

  if(n == -1)
  {
    int cause = last_system_error();
    if(is_wouldblock(cause))
    {
      next = nullptr;
    }
    else if(is_fatal_io_error(cause))
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

int tcp_socket_t::close_write_end()
{
  int result = 0;

#ifdef _WIN32
  int r = ::shutdown(fd_, SD_SEND);
#else
  int r = ::shutdown(fd_, SHUT_WR);
#endif
  if(r == -1)
  {
    int cause = last_system_error();
    if(is_fatal_io_error(cause))
    {
      system_exception_builder_t builder;
      builder << "shutdown() failure: " << error_status_t(cause);
      builder.explode();
    }
    result = cause;
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
    if(is_wouldblock(cause))
    {
      next = nullptr;
    }
    else if(is_fatal_io_error(cause))
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

void tcp_socket_t::close_fd(int fd) noexcept
{
  assert(fd != -1);

#ifdef _WIN32
  ::closesocket(fd);
#else
  ::close(fd);
#endif
}

bool tcp_socket_t::stops_sigpipe()
{
#if defined(SO_NOSIGPIPE) || defined(MSG_NOSIGNAL)
  return true;
#else
  return false;
#endif
}

} // cuti

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

#include "tcp_socket.hpp"

#include "endpoint.hpp"
#include "system_error.hpp"

#include <cassert>
#include <limits>
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

namespace xes
{

namespace // anonymous
{

void set_v6only(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting IPV6_V6ONLY", cause);
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
    throw system_exception_t("Error setting TCP_NODELAY", cause);
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
    throw system_exception_t("Error setting SO_KEEPALIVE", cause);
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

void set_nonblocking(int fd, bool enable)
{
  u_long arg = enable;
  int r = ioctlsocket(fd, FIONBIO, &arg);
  if(r == SOCKET_ERROR)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting FIONBIO", cause);
  }
}

#else // POSIX

int to_fd(int fd)
{
  return fd;
}

void set_nonblocking(int fd, bool enable)
{
  int r = fcntl(fd, F_GETFL);
  if(r != -1)
  {
    if(enable)
    {
      r |= O_NONBLOCK;
    }
    else
    {
      r &= ~O_NONBLOCK;
    }
    r = fcntl(fd, F_SETFL, r);
  }

  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting O_NONBLOCK", cause);
  }
}

void set_reuseaddr(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting SO_REUSEADDR", cause);
  }
}
    
#ifndef SOCK_CLOEXEC

/*
 * In the absence of SOCK_CLOEXEC, a socket leak will occur when a
 * concurrent thread calls fork() before a fresh socket's
 * close-on-exec flag is set. All we can do here is call set_cloexec()
 * ASAP.
 */
void set_cloexec(int fd, bool enable)
{
  int r = fcntl(fd, F_GETFD);
  if(r != -1)
  {
    if(enable)
    {
      r |= FD_CLOEXEC;
    }
    else
    {
      r &= ~FD_CLOEXEC;
    }
    r = fcntl(fd, F_SETFD, r);
  }
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting FD_CLOEXEC", cause);
  }
}

#endif // !SOCK_CLOEXEC

#endif // POSIX

void set_default_connection_flags(int fd)
{
  set_nodelay(fd, true);
  set_keepalive(fd, true);
  set_nonblocking(fd, false);
}

} // anonymous

tcp_socket_t::tcp_socket_t(int family)
: tcp_socket_t() // -> exceptions will not leak fd_
{
#if defined(SOCK_CLOEXEC)
  fd_ = to_fd(::socket(check_family(family), SOCK_STREAM | SOCK_CLOEXEC, 0));
#else
  fd_ = to_fd(::socket(check_family(family), SOCK_STREAM, 0));
#endif

  if(fd_ == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't create socket", cause);
  }
    
#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
  set_cloexec(fd_, true);
#endif
}

void tcp_socket_t::set_nonblocking(bool enable)
{
  assert(!empty());

  xes::set_nonblocking(fd_, enable);
}

void tcp_socket_t::bind(endpoint_t const& endpoint)
{
  assert(!empty());

  if(endpoint_family(endpoint) == AF_INET6)
  {
    set_v6only(fd_, true);
  }

#ifndef _WIN32
  set_reuseaddr(fd_, true);
#endif

  int r = ::bind(fd_, &endpoint, endpoint_size(endpoint));
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t(
      "Can't bind to " + ip_address(endpoint) +
        " port " + std::to_string(port_number(endpoint)),
      cause);
  }
}

void tcp_socket_t::listen()
{
  assert(!empty());

  int r = ::listen(fd_, SOMAXCONN);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't listen", cause);
  }
}

tcp_socket_t tcp_socket_t::try_accept()
{
  assert(!empty());

  tcp_socket_t result;

#if defined(SOCK_CLOEXEC)
  result.fd_ = to_fd(::accept4(fd_, nullptr, nullptr, SOCK_CLOEXEC));
#else
  result.fd_ = to_fd(::accept(fd_, nullptr, nullptr));
#endif

  if(result.fd_ == -1)
  {
    /*
     * Even in blocking mode, accept() may fail spuriously when the
     * connection breaks before it is accepted by the application.
     * Don't panic; just return an empty tcp_socket.
     */
    return result;
  }

#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
  set_cloexec(result.fd_, true);
#endif

  set_default_connection_flags(result.fd_);
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

} // xes

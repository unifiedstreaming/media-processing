/*
 * Copyright (C) 2021 CodeShop B.V.
 *
 * This file is part of the cuti library.
 *
 * The cuti library is free software: you can redistribute it
 * and/or modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * The cuti library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See version 2 of the GNU General Public License for more details.
 *
 * You should have received a copy of version 2 of the GNU General
 * Public License along with the cuti library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "tcp_socket.hpp"

#include "endpoint.hpp"
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

#ifdef SO_NOSIGPIPE

void set_nosigpipe(int fd, bool enable)
{
  const int optval = enable;
  int r = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting SO_NOSIGPIPE", cause);
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
    throw system_exception_t("Can't create socket", cause);
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
    builder << "Can't bind to endpoint " << endpoint;
    builder.explode(cause);
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

void tcp_socket_t::connect(endpoint_t const& peer)
{
  assert(!empty());

  int r = ::connect(fd_, &peer.socket_address(), peer.socket_address_size());
  if(r == -1)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Can't connect to endpoint " << peer;
    builder.explode(cause);
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
    throw system_exception_t("getsockname() failure", cause);
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
    throw system_exception_t("getpeername() failure", cause);
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

tcp_socket_t tcp_socket_t::accept()
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
    int cause = last_system_error();
    if(is_wouldblock(cause))
    {
      return result;
    }
    throw system_exception_t("accept() failure", cause);
  }

#if !defined(_WIN32) && !defined(SOCK_CLOEXEC)
  set_cloexec(result.fd_, true);
#endif

  set_initial_connection_flags(result.fd_);

  return result;
}

char const* tcp_socket_t::write_some(char const* first, char const* last)
{
  assert(!empty());
  assert(first <= last);

  int count = std::numeric_limits<int>::max();
  if(count > last - first)
  {
    count = static_cast<int>(last - first);
  }
    
#if defined(SO_NOSIGPIPE) || !defined(MSG_NOSIGNAL)
  auto r = ::send(fd_, first, count, 0);
#else
  auto r = ::send(fd_, first, count, MSG_NOSIGNAL);
#endif

  if(r == -1)
  {
    int cause = last_system_error();
    if(is_wouldblock(cause))
    {
      return nullptr;
    }
    throw system_exception_t("send() failure", cause);
  }

  return first + r;
}

void tcp_socket_t::close_write_end()
{
#ifdef _WIN32
  int r = ::shutdown(fd_, SD_SEND);
#else
  int r = ::shutdown(fd_, SHUT_WR);
#endif
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("shutdown() failure", cause);
  }
}  

char* tcp_socket_t::read_some(char* first, char* last)
{
  assert(!empty());
  assert(first <= last);

  int count = std::numeric_limits<int>::max();
  if(count > last - first)
  {
    count = static_cast<int>(last - first);
  }
    
  auto r = ::recv(fd_, first, count, 0);

  if(r == -1)
  {
    int cause = last_system_error();
    if(is_wouldblock(cause))
    {
      return nullptr;
    }
    throw system_exception_t("recv() failure()", cause);
  }

  return first + r;
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

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

create_socket_t const create_socket = { };
consume_fd_t const consume_fd = { };

void tcp_socket_t::set_v6only(bool enable)
{
  assert(fd_ != -1);

  const int optval = enable;
  int r = setsockopt(fd_, IPPROTO_IPV6, IPV6_V6ONLY,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting IPV6_V6ONLY", cause);
  }
}
    
void tcp_socket_t::set_reuseaddr(bool enable)
{
  assert(fd_ != -1);

  const int optval = enable;
  int r = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting SO_REUSEADDR", cause);
  }
}
    
void tcp_socket_t::set_nodelay(bool enable)
{
  assert(fd_ != -1);

  const int optval = enable;
  int r = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting TCP_NODELAY", cause);
  }
}
    
void tcp_socket_t::set_keepalive(bool enable)
{
  assert(fd_ != -1);

  const int optval = enable;
  int r = setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,
                     reinterpret_cast<const char *>(&optval), sizeof optval);
  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting SO_KEEPALIVE", cause);
  }
}
    
#ifdef _WIN32

namespace // anonymous
{

int to_fd(SOCKET sock)
{
  assert(sock >= 0);
  assert(sock <= std::numeric_limits<int>::max());
  return static_cast<int>(sock);
}

} // anonymous

void tcp_socket_t::set_nonblocking(bool enable)
{
  u_long arg = enable;
  int r = ioctlsocket(fd_, FIONBIO, &arg);
  if(r == SOCKET_ERROR)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting FIONBIO", cause);
  }
}
	
int tcp_socket_t::open_fd(int family)
{
  SOCKET result = socket(check_family(family), SOCK_STREAM, 0);
  if(result == INVALID_SOCKET)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't create socket", cause);
  }
  return to_fd(result);
}

void tcp_socket_t::close_fd(int fd) noexcept
{
  closesocket(fd);
}

#else // POSIX

void tcp_socket_t::set_nonblocking(bool enable)
{
  int r = fcntl(fd_, F_GETFL);
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
    r = fcntl(fd_, F_SETFL, r);
  }

  if(r == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Error setting O_NONBLOCK", cause);
  }
}

int tcp_socket_t::open_fd(int family)
{
#ifdef SOCK_CLOEXEC
  
  /*
   * Close-on-exec set from the start; no socket leak.
   */
  int fd = socket(check_family(family), SOCK_STREAM | SOCK_CLOEXEC, 0);

#else

  /*
   * Potential socket leak when another thread concurrently does a
   * fork()/exec(). No proper fix exists; just close the window of
   * opportunity now.
   */
  int fd = socket(check_family(family), SOCK_STREAM, 0);
  if(fd != -1)
  {
    int r = fcntl(fd, F_GETFD);
    if(r != -1)
    {
      r |= FD_CLOEXEC;
      r = fcntl(fd, F_SETFD, r);
    }
    if(r == -1)
    {
      int cause = last_system_error();
      close_fd(fd);
      throw system_exception_t("Error setting FD_CLOEXEC", cause);
    }
  }

#endif
    
  if(fd == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't create socket", cause);
  }

  return fd;
}

void tcp_socket_t::close_fd(int fd) noexcept
{
  close(fd);
}
  
#endif // POSIX

} // xes

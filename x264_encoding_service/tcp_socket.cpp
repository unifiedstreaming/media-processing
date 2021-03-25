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
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace xes
{

create_socket_t const create_socket = { };
consume_fd_t const consume_fd = { };

#ifdef _WIN32

namespace // anonymous
{

int to_fd(SOCKET sock)
{
  assert(socket >= 0);
  assert(socket <= std::numeric_limits<int>::max());
  return static_cast<int>(sock);
}

} // anonymous

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

int tcp_socket_t::open_fd(int family)
{
  int result = socket(check_family(family), SOCK_STREAM | SOCK_CLOEXEC, 0);
  if(result == -1)
  {
    int cause = last_system_error();
    throw system_exception_t("Can't create socket", cause);
  }
  return result;
}

void tcp_socket_t::close_fd(int fd) noexcept
{
  close(fd);
}
  
#endif // POSIX

} // xes

/*
 * Copyright (C) 2022 CodeShop B.V.
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

#include "io_utils.hpp"
#include "system_error.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace cuti
{

#ifdef _WIN32

bool is_wouldblock(int error)
{
  return error == WSAEWOULDBLOCK;
}

bool is_fatal_io_error(int error)
{
  switch(error)
  {
  case WSAEACCES :
  case WSAEFAULT :
  case WSAEINPROGRESS :
  case WSAEINVAL :
  case WSAEINTR :
  case WSAEMFILE :
  case WSAEMSGSIZE :
  case WSAENOBUFS :
  case WSAENETDOWN :
  case WSAENOTSOCK :
  case WSANOTINITIALISED :
    return true;
  default :
    return false;
  }
}

void set_nonblocking(int fd, bool enable)
{
  u_long arg = enable;
  int r = ioctlsocket(fd, FIONBIO, &arg);
  if(r == SOCKET_ERROR)
  {
    int cause = last_system_error();
    system_exception_builder_t builder;
    builder << "Error setting FIONBIO: " << error_status_t(cause);
    builder.explode();
  }
}

#else // POSIX

bool is_wouldblock(int error)
{
  return error == EAGAIN || error == EWOULDBLOCK;
}

bool is_fatal_io_error(int error)
{
  switch(error)
  {
  case EACCES :
  case EBADF :
  case EFAULT :
  case EINVAL :
  case EMFILE :
  case ENFILE :
  case ENOBUFS :
  case ENOMEM :
  case ENOTSOCK :
    return true;
  default :
    return false;
  }
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
    system_exception_builder_t builder;
    builder << "Error setting O_NONBLOCK: " << error_status_t(cause);
    builder.explode();
  }
}

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
    system_exception_builder_t builder;
    builder << "Error setting FD_CLOEXEC: " << error_status_t(cause);
    builder.explode();
  }
}

#endif // POSIX

} // cuti

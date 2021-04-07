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

#include "system_error.hpp"

#include <utility>

#ifdef _WIN32

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace xes
{

int last_system_error()
{
  return GetLastError();
}

bool is_wouldblock(int error)
{
  return error == WSAEWOULDBLOCK;
}

std::string system_error_string(int error)
{
  static const int bufsize = 256;
  char buf[bufsize];

  DWORD result = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
      FORMAT_MESSAGE_MAX_WIDTH_MASK,
    nullptr, error, 0,
    buf, bufsize - 1, nullptr
  );

  if(result == 0)
  {
    return "System error number# " + std::to_string(error);
  }

  return std::string(buf);
}

} // namespace xes

#else // POSIX

#include <errno.h>
#include <string.h>

namespace xes
{

int last_system_error()
{
  return errno;
}

bool is_wouldblock(int error)
{
  return error == EAGAIN || error == EWOULDBLOCK;
}

std::string system_error_string(int error)
{
  static const int bufsize = 256;

  char buf[bufsize];

#if !__GLIBC__ || \
  ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)

  // Posix version
  int r = strerror_r(error, buf, bufsize - 1);
  if(r == 0)
  {
    return buf;
  }
  return "System error number #" + std::to_string(error);  

#else

  // GNU-specific version
  return strerror_r(error, buf, bufsize - 1);

#endif

}

} // namespace xes

#endif

namespace xes
{

system_exception_t::system_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

system_exception_t::system_exception_t(std::string complaint, int cause)
: std::runtime_error(std::move(complaint) + ": " + system_error_string(cause))
{ }

} // namespace xes

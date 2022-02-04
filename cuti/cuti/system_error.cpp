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

#include "system_error.hpp"

#include <utility>

#ifdef _WIN32

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace cuti
{

int last_system_error()
{
  return GetLastError();
}

int timeout_system_error()
{
  return WSAETIMEDOUT;
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

} // namespace cuti

#else // POSIX

#include <errno.h>
#include <stdio.h>
#include <string.h>

namespace cuti
{

int last_system_error()
{
  return errno;
}

int timeout_system_error()
{
  return ETIMEDOUT;
}

#if defined(__FreeBSD__)

std::string system_error_string(int error)
{
  // On (some versions of) FreeBSD, strerror_r confuses thread sanitizer
  if(error > 0 && error < sys_nerr)
  {
    return sys_errlist[error];
  }

  return "System error number #" + std::to_string(error);
}

#elif !__GLIBC__ || \
  ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)

std::string system_error_string(int error)
{
  // Generic POSIX
  static int constexpr bufsize = 256;
  char buf[bufsize];
  buf[bufsize - 1] = '\0';

  int r = strerror_r(error, buf, bufsize - 1);
  if(r == 0)
  {
    return buf;
  }

  return "System error number #" + std::to_string(error);
}

#else

std::string system_error_string(int error)
{
  // GNU-specific version
  static int constexpr bufsize = 256;
  char buf[bufsize];
  buf[bufsize - 1] = '\0';

  return strerror_r(error, buf, bufsize - 1);
}

#endif

} // namespace cuti

#endif // POSIX

namespace cuti
{

system_exception_t::system_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

system_exception_t::system_exception_t(std::string complaint, int cause)
: std::runtime_error(std::move(complaint) + ": " + system_error_string(cause))
{ }

system_exception_t::~system_exception_t()
{ }

} // namespace cuti

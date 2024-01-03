/*
 * Copyright (C) 2022-2024 CodeShop B.V.
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

#include "error_status.hpp"

#include <ostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#endif

namespace cuti
{

namespace // anonymous
{

#if defined(_WIN32)

void print_system_error(std::ostream& os, int error)
{
  static int constexpr bufsize = 256;
  char buf[bufsize];
  buf[bufsize - 1] = '\0';

  DWORD result = FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
      FORMAT_MESSAGE_MAX_WIDTH_MASK,
    nullptr, error, 0,
    buf, bufsize - 1, nullptr
  );

  if(result == 0)
  {
    os << "System error number #" << error;
  }
  else
  {
    os << buf;
  }
}

#elif !__GLIBC__ || \
  ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE)

void print_system_error(std::ostream& os, int error)
{
  // Generic POSIX
  static int constexpr bufsize = 256;
  char buf[bufsize];
  buf[bufsize - 1] = '\0';

  int r = strerror_r(error, buf, bufsize - 1);
  if(r == 0)
  {
    os << buf;
  }
  else
  {
    os << "System error number #" << error;
  }
}

#else

void print_system_error(std::ostream& os, int error)
{
  // GNU-specific version
  static int constexpr bufsize = 256;
  char buf[bufsize];
  buf[bufsize - 1] = '\0';

  os << strerror_r(error, buf, bufsize - 1);
}

#endif

} // anonymous

void error_status_t::print(std::ostream& os) const
{
  switch(cuti_error_code_)
  {
  case error_code_t::no_error :
    if(system_error_code_ == 0)
    {
      os << "no error";
    }
    else
    {
      print_system_error(os, system_error_code_);
    }
    break;
  case error_code_t::insufficient_throughput :
    os << "insufficient throughput";
    break;
  default :
    os << "unknown cuti error code " << static_cast<int>(cuti_error_code_);
    break;
  }
}

} // cuti

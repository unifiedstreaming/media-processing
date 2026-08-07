/*
 * Copyright (C) 2026 CodeShop B.V.
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

#include "stringprintf.hpp"

#include <cassert>
#include <vector>

namespace cuti
{

std::string stringprintf(char const* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  auto str = vstringprintf(fmt, args);
  va_end(args);
  return str;
}

std::string vstringprintf(char const* fmt, va_list args)
{
  /* From the C17 standard:
   *
   * The _vsnprintf_ function returns the number of characters that would have
   * been written had 'n' been sufficiently large, not counting the terminating
   * null character, or a negative value if an encoding error occurred. Thus,
   * the null-terminated output has been completely written if and only if the
   * returned value is nonnegative and less than 'n'.
   *
   * and:
   *
   * The object 'ap' may be passed as an argument to another function; if that
   * function invokes the va_arg macro with parameter 'ap', the value of 'ap' in
   * the calling function is indeterminate and shall be passed to the va_end
   * macro prior to any further reference to 'ap'.
   */

  va_list retry_args;
  va_copy(retry_args, args);

  std::vector<char> buffer(1024);
  int length = vsnprintf(buffer.data(), buffer.size(), fmt, args);

  if(length >= 0 && static_cast<size_t>(length) >= buffer.size())
  {
    buffer.resize(static_cast<size_t>(length) + 1);
    length = vsnprintf(buffer.data(), buffer.size(), fmt, retry_args);
  }

  va_end(retry_args);

  if(length >= 0 && static_cast<size_t>(length) < buffer.size())
  {
    return {buffer.data(), static_cast<size_t>(length)};
  }
  else
  {
    return "vsnprintf() encoding error";
  }
}

} // cuti

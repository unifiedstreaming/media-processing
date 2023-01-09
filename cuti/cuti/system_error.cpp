/*
 * Copyright (C) 2021-2023 CodeShop B.V.
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

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#endif

namespace cuti
{

#ifdef _WIN32

int last_system_error()
{
  return GetLastError();
}

#else

int last_system_error()
{
  return errno;
}

#endif

system_exception_t::system_exception_t(std::string complaint)
: std::runtime_error(std::move(complaint))
{ }

system_exception_t::~system_exception_t()
{ }

} // namespace cuti

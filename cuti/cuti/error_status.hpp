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

#ifndef CUTI_ERROR_STATUS_HPP_
#define CUTI_ERROR_STATUS_HPP_

#include "linkage.h"

#include <string>

namespace cuti
{

enum class error_code_t
{
  no_error,
  insufficient_throughput
};

struct CUTI_ABI error_status_t
{
  constexpr error_status_t()
  : system_error_code_(0)
  , cuti_error_code_(error_code_t::no_error)
  { }

  constexpr error_status_t(int system_error_code)
  : system_error_code_(system_error_code)
  , cuti_error_code_(error_code_t::no_error)
  { }

  constexpr error_status_t(error_code_t cuti_error_code)
  : system_error_code_(0)
  , cuti_error_code_(cuti_error_code)
  { }

  constexpr bool ok() const
  {
    return system_error_code_ != 0 ||
           cuti_error_code_ != error_code_t::no_error;
  }

  constexpr explicit operator bool() const
  {
    return !this->ok();
  }

  std::string to_string() const;
  
private :
  int system_error_code_;
  error_code_t cuti_error_code_;
};

} // cuti

#endif

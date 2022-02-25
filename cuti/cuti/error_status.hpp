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
#include "relational_ops.hpp"

#include <string>

namespace cuti
{

enum class error_code_t
{
  no_error,
  insufficient_throughput
};

struct CUTI_ABI error_status_t : relational_ops_t<error_status_t>
{
  constexpr error_status_t() noexcept
  : system_error_code_(0)
  , cuti_error_code_(error_code_t::no_error)
  { }

  constexpr error_status_t(int system_error_code) noexcept
  : system_error_code_(system_error_code)
  , cuti_error_code_(error_code_t::no_error)
  { }

  constexpr error_status_t(error_code_t cuti_error_code) noexcept
  : system_error_code_(0)
  , cuti_error_code_(cuti_error_code)
  { }

  constexpr bool ok() const noexcept
  {
    return system_error_code_ == 0 &&
           cuti_error_code_ == error_code_t::no_error;
  }

  constexpr explicit operator bool() const noexcept
  {
    return !this->ok();
  }

  std::string to_string() const;
  
  constexpr bool equal_to(error_status_t const& rhs) const noexcept
  {
    return this->system_error_code_ == rhs.system_error_code_ &&
      this->cuti_error_code_ == rhs.cuti_error_code_;
  }

  constexpr bool less_than(error_status_t const& rhs) const noexcept
  {
    return this->system_error_code_ < rhs.system_error_code_ ||
      (this->system_error_code_ == rhs.system_error_code_ &&
       this->cuti_error_code_ < rhs.cuti_error_code_);
  }

private :
  int system_error_code_;
  error_code_t cuti_error_code_;
};

} // cuti

#endif

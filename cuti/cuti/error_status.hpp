/*
 * Copyright (C) 2022-2023 CodeShop B.V.
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

#include <iosfwd>

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
  : cuti_error_code_(error_code_t::no_error)
  , system_error_code_(0)
  { }

  constexpr error_status_t(int system_error_code) noexcept
  : cuti_error_code_(error_code_t::no_error)
  , system_error_code_(system_error_code)
  { }

  constexpr error_status_t(error_code_t cuti_error_code) noexcept
  : cuti_error_code_(cuti_error_code)
  , system_error_code_(0)
  { }

  constexpr explicit operator bool() const noexcept
  {
    return cuti_error_code_ != error_code_t::no_error ||
      system_error_code_ != 0;
  }

  constexpr bool equal_to(error_status_t const& other) const noexcept
  {
    return this->cuti_error_code_ == other.cuti_error_code_ &&
      this->system_error_code_ == other.system_error_code_;
  }

  constexpr bool less_than(error_status_t const& other) const noexcept
  {
    if(this->cuti_error_code_ < other.cuti_error_code_)
    {
      return true;
    }
    else if(other.cuti_error_code_ < this->cuti_error_code_)
    {
      return false;
    }
    else
    {
      return this->system_error_code_ < other.system_error_code_;
    }
  }

  void print(std::ostream& os) const;

  friend std::ostream& operator<<(
    std::ostream& os, error_status_t const& status)
  {
    status.print(os);
    return os;
  }

private :
  error_code_t cuti_error_code_;
  int system_error_code_;
};

} // cuti

#endif

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

#include "error_status.hpp"
#include "system_error.hpp"

namespace cuti
{

std::string error_status_t::to_string() const
{
  if(system_error_code_ != 0)
  {
    return system_error_string(system_error_code_);
  }

  switch(cuti_error_code_)
  {
  case error_code_t::no_error :
    return "no error";
  case error_code_t::insufficient_throughput :
    return "insufficent throughput";
  default :
    return "unknown cuti error code " +
      std::to_string(static_cast<int>(cuti_error_code_));
  }
}

} // cuti

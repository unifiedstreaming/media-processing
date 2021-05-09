/*
 * Copyright (C) 2021 CodeShop B.V.
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

#include "selector.hpp"

namespace cuti
{

selector_t::~selector_t()
{ }

int selector_t::timeout_millis(timeout_t timeout)
{
  static timeout_t const zero = timeout_t::zero();
  static int const max_millis = 30000;

  int result;

  if(timeout < zero)
  {
    result = -1;
  }
  else if(timeout == zero)
  {
    result = 0;
  }
  else // timeout > zero
  {
    auto count = std::chrono::duration_cast<std::chrono::milliseconds>(
        timeout).count();
    if(count < 1)
    {
      result = 1; // prevent spinloop
    }
    else if(count < max_millis)
    {
      result = static_cast<int>(count);
    }
    else
    {
      result = max_millis;
    }
  }

  return result;
}

} // cuti

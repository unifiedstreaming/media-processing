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

#ifndef CUTI_SERIAL_GENERATOR_HPP_
#define CUTI_SERIAL_GENERATOR_HPP_

#include "linkage.h"
#include "mutex_wrapper.hpp"

namespace cuti
{

/*
 * A re-entrant serial number generator that may be useful for testing
 * and debugging purposes.
 */
struct CUTI_ABI serial_generator_t
{
  serial_generator_t()
  : next_wrapper_(0)
  { }

  serial_generator_t(serial_generator_t const&) = delete;
  serial_generator_t& operator=(serial_generator_t const&) = delete;

  unsigned int next()
  {
    unsigned int result;
    {
      auto locked_next = next_wrapper_.lock();
      result = *locked_next;
      ++(*locked_next);
    }
    return result;
  }

private :
  mutex_wrapper_t<unsigned int> next_wrapper_;
};

} // cuti

#endif

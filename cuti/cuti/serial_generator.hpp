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

#include <atomic>

namespace cuti
{

/*
 * A re-entrant serial number generator that may be useful for testing
 * and debugging purposes.
 */
struct CUTI_ABI serial_generator_t
{
  serial_generator_t()
  : next_(0)
  { }

  serial_generator_t(serial_generator_t const&) = delete;
  serial_generator_t& operator=(serial_generator_t const&) = delete;

  unsigned int next()
  {
    /*
     * https://en.cppreference.com/cpp/atomic/atomic/fetch_add:
     *
     * fetch_add() returns the value immediately preceding the effects
     * of this function in the modification order of *this.
     *
     * https://en.cppreference.com/cpp/atomic/memory_order:
     *
     * All modifications to any particular atomic variable occur in a
     * total order that is specific to this one atomic variable.
     */
    return next_.fetch_add(1, std::memory_order_relaxed);
  }

private :
  std::atomic<unsigned int> next_;
};

} // cuti

#endif

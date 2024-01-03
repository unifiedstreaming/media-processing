/*
 * Copyright (C) 2021-2024 CodeShop B.V.
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

#ifndef CUTI_STACK_MARKER_HPP_
#define CUTI_STACK_MARKER_HPP_

#include "linkage.h"

#include <cstdint>

namespace cuti
{

/*
 * A utility to measure runtime stack usage.  Useless when segmented
 * (non-contiguous) stacks are in action.
 * Stack markers must be local variables; be sure not to put stack
 * markers on the heap or in static storage.
 */
struct CUTI_ABI stack_marker_t
{
  /*
   * The default threshold is meant to support platforms with a small
   * runtime stack (musl: 80K).
   */
  static std::uintptr_t constexpr default_threshold = 32 * 1024;

  stack_marker_t()
  { }

  stack_marker_t(stack_marker_t const&) = delete;
  stack_marker_t& operator=(stack_marker_t const&) = delete;
  
  std::uintptr_t address() const;

  bool in_range(stack_marker_t const& other,
                std::uintptr_t threshold = default_threshold) const
  {
    auto addr1 = this->address();
    auto addr2 = other.address();
    return addr1 <= addr2 ? addr2 - addr1 < threshold
                          : addr1 - addr2 < threshold;
  }
};

} // cuti

#endif
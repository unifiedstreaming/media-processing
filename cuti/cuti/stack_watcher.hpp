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

#ifndef CUTI_STACK_WATCHER_HPP_
#define CUTI_STACK_WATCHER_HPP_

#include "linkage.h"

#include <cassert>
#include <cstdint>

namespace cuti
{

/*
 * Utility to check the use of the current thread's run-time stack.
 * Useless if segmented (non-contiguous) stacks are used.
 */
struct CUTI_ABI stack_watcher_t
{
  /*
   * The default threshold is meant to support platforms with a small
   * runtime stack (musl: 80K).
   */
  static std::uintptr_t constexpr default_threshold = 32 * 1024;

  explicit stack_watcher_t(std::uintptr_t threshold = default_threshold)
  : base_(approx_sp())
  , threshold_(threshold)
  { }

  stack_watcher_t(stack_watcher_t const&) = delete;
  stack_watcher_t& operator=(stack_watcher_t const&) = delete;
  
  bool could_overflow() const
  {
    auto sp = approx_sp();
    return sp <= base_ ?
      base_ - sp >= threshold_ :
      sp - base_ >= threshold_;
  }
                     
private :
  static std::uintptr_t approx_sp();

private :
  std::uintptr_t const base_;
  std::uintptr_t const threshold_;
};

} // cuti

#endif

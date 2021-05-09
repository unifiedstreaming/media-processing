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

#ifndef CUTI_DEFAULT_SCHEDULER_HPP_
#define CUTI_DEFAULT_SCHEDULER_HPP_

#include "linkage.h"
#include "scheduler.hpp"
#include "selector.hpp"

#include <chrono>
#include <memory>

namespace cuti
{

/*
 * Cuti's default scheduler implementation, providing has_work(),
 * poll() (for testing purposes) and wait() (for writing an event
 * loop).
 */
struct CUTI_ABI default_scheduler_t : scheduler_t
{
  using timeout_t = std::chrono::system_clock::duration;

  /*
   * Constructs a default scheduler using the specified selector
   * instance.  <selector> must be != nullptr.
   */
  explicit default_scheduler_t(std::unique_ptr<selector_t> selector);

  /*
   * Returns true if there are any pending callbacks, false otherwise.
   */
  bool has_work() const noexcept;

  /*
   * Checks if any of the registered events have occurred, without
   * blocking.  Returns the first event's corresponding callback if an
   * event was detected, and nullptr otherwise.
   * This function should only be used for testing purposes, to prove
   * that some event did not yet occur.
   *   
   * Precondition: this->has_work().
   */
  callback_t poll();

  /*
   * Waits for any of the registered events to occur, and returns the
   * first event's corresponding callback.
   *
   * Precondition: this->has_work().
   */
  callback_t wait();
  
  ~default_scheduler_t() override;

private :
  int do_call_when_writable(int fd, callback_t callback) override;
  void do_cancel_when_writable(int ticket) noexcept override;
  int do_call_when_readable(int fd, callback_t callback) override;
  void do_cancel_when_readable(int ticket) noexcept override;

private :
  std::unique_ptr<selector_t> selector_;
};

}

#endif

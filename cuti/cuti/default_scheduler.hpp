/*
 * Copyright (C) 2021-2025 CodeShop B.V.
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

#include "indexed_heap.hpp"
#include "linkage.h"
#include "scheduler.hpp"
#include "selector.hpp"
#include "selector_factory.hpp"

#include <functional>
#include <memory>

namespace cuti
{

/*
 * Cuti's default scheduler implementation.
 */
struct CUTI_ABI default_scheduler_t : scheduler_t
{
  /*
   * Constructs a default scheduler using the first of the available
   * selector factories.
   */
  default_scheduler_t();

  /*
   * Constructs a default scheduler using the specified selector
   * factory.
   */
  explicit default_scheduler_t(selector_factory_t const& factory);

  /*
   * Waits for any of the registered events to occur and returns the
   * first event's callback, or nullptr if the scheduler is out of
   * work.
   */
  callback_t wait();
  
private :
  int do_call_alarm(time_point_t time_point, callback_t callback) override;
  void do_cancel_alarm(int ticket) noexcept override;
  int do_call_when_writable(int fd, callback_t callback) override;
  void do_cancel_when_writable(int ticket) noexcept override;
  int do_call_when_readable(int fd, callback_t callback) override;
  void do_cancel_when_readable(int ticket) noexcept override;

private :
  indexed_heap_t<time_point_t, callback_t,
    std::greater<time_point_t>> alarms_; // std::greater results in minheap
  std::unique_ptr<selector_t> selector_;
  bool poll_first_;
};

}

#endif

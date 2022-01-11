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

#ifndef CUTI_WAKEUP_SIGNAL_HPP_
#define CUTI_WAKEUP_SIGNAL_HPP_

#include <memory>
#include <mutex>

#include "linkage.h"
#include "tcp_connection.hpp"

namespace cuti
{

/*
 * Object for delivering a wakeup signal to zero or more observers.
 */
struct CUTI_ABI wakeup_signal_t
{
  wakeup_signal_t();

  wakeup_signal_t(wakeup_signal_t const&) = delete;
  wakeup_signal_t& operator=(wakeup_signal_t const&) = delete;
  
  bool active() const noexcept;

  void activate() noexcept;
  void deactivate() noexcept;

  cancellation_ticket_t call_when_active(
    scheduler_t& scheduler, callback_t callback) const;
  
private :
  mutable std::mutex mutex_;
  unsigned int activation_count_;
  std::unique_ptr<tcp_connection_t> write_end_;
  std::unique_ptr<tcp_connection_t> read_end_;
};

/*
 * RAII object for observing a wakeup signal.  There can only be a
 * single watcher per signal/scheduler combination.
 */
struct CUTI_ABI wakeup_signal_watcher_t
{
  wakeup_signal_watcher_t(wakeup_signal_t const& wakeup_signal,
                          scheduler_t& scheduler);

  wakeup_signal_watcher_t(wakeup_signal_watcher_t const&) = delete;
  wakeup_signal_watcher_t& operator=(wakeup_signal_watcher_t const&) = delete;
  
  void call_when_active(callback_t callback);
  void cancel_when_active() noexcept;

  ~wakeup_signal_watcher_t();

private :
  void on_scheduler_callback();

private :
  wakeup_signal_t const& wakeup_signal_;
  scheduler_t& scheduler_;
  cancellation_ticket_t ticket_;
  callback_t callback_;
};
  
} // cuti

#endif

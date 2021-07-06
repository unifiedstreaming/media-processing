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

#ifndef CUTI_TICKET_HOLDER_HPP_
#define CUTI_TICKET_HOLDER_HPP_

#include "callback.hpp"
#include "scheduler.hpp"

namespace cuti
{

struct tcp_acceptor_t;
struct tcp_connection_t;

/*
 * RAII type managing at most one callback/cancellation ticket combo,
 * including the assocation with the tickets's scheduler.
 *
 * Any associated scheduler/acceptor/connection must remain alive
 * until either:
 * (1) cancel() is called on the holder
 * (2) the holder is pointed at some other
 *     scheduler/acceptor/connection
 * (3) the holder is destroyed.
 */
struct CUTI_ABI ticket_holder_t
{
  /*
   * Constructs an initially empty holder.
   */
  ticket_holder_t();

  ticket_holder_t(ticket_holder_t const&) = delete;
  ticket_holder_t& operator=(ticket_holder_t const&) = delete;
  
  /*
   * Returns whether the holder is empty (no pending callback).
   */
  bool empty() const noexcept
  {
    return ticket_.empty();
  }

  /*
   * Schedules a one-time callback at time_point when.
   */
  void call_alarm(scheduler_t& scheduler,
                  time_point_t when,
                  callback_t callback);

  /*
   * Schedules a one-time callback after timeout.
   */
  void call_alarm(scheduler_t& scheduler,
                  duration_t timeout,
                  callback_t callback);

  /*
   * Schedules a one-time callback for when acceptor is ready.
   */
  void call_when_ready(scheduler_t& scheduler,
                       tcp_acceptor_t& acceptor,
                       callback_t callback);

  /*
   * Schedules a one-time callback for when connection is writable.
   */
  void call_when_writable(scheduler_t& scheduler,
                          tcp_connection_t& connection,
                          callback_t callback);

  /*
   * Schedules a one-time callback for when connection is readable.
   */
  void call_when_readable(scheduler_t& scheduler,
                          tcp_connection_t& connection,
                          callback_t callback);

  /*
   * Cancels any pending callback; no effect if this->empty().
   */
  void cancel() noexcept;

  /*
   * Implicitly cancels any pending callback.
   */
  ~ticket_holder_t();

private :
  void on_scheduler_callback();

private :
  callback_t const scheduler_callback_;
  cancellation_ticket_t ticket_;
  scheduler_t* scheduler_;
  callback_t user_callback_;
};

} // cuti

#endif

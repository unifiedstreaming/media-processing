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

#ifndef CUTI_SCHEDULER_HPP_
#define CUTI_SCHEDULER_HPP_

#include "callback.hpp"
#include "chrono_types.hpp"
#include "linkage.h"

#include <cassert>
#include <utility>
#include <memory>

namespace cuti
{

struct scheduler_t;

/*
 * Cancellation tickets are used to cancel a previously scheduled
 * callback before it is invoked.
 * 
 * Please note: a cancellation ticket is only valid until the callback
 * is selected.  Any attempt to cancel a callback during or after its
 * invocation leads to undefined behavior.
 */
struct cancellation_ticket_t
{
  /*
   * Constructs an empty cancellation ticket.
   */
  cancellation_ticket_t() noexcept
  : type_(type_t::empty)
  , id_(-1)
  { }

  /*
   * Tells if the ticket is empty.  Scheduling a callback returns a
   * non-empty cancellation ticket.
   */
  bool empty() const noexcept
  { return type_ == type_t::empty; }

  /*
   * Sets the ticket to the empty state.
   */
  void clear() noexcept
  { *this = cancellation_ticket_t(); }

private :
  friend struct scheduler_t;

  enum class type_t { empty, alarm, writable, readable };

  explicit cancellation_ticket_t(type_t type, int id) noexcept
  : type_(type)
  , id_(id)
  { }

  type_t type() const noexcept
  { return type_; }

  int id() const noexcept
  { return id_; }

private :
  type_t type_;
  int id_;
};

/*
 * Abstract event scheduler interface.  The purpose of this interface
 * is to isolate cuti-based event handlers from the specifics of the
 * application's event loop(s).  Some implementations of this
 * interface, like cuti's default scheduler, can be used to write such
 * a loop; other implementations may serve as an adapter for an
 * existing event loop.
 */
struct CUTI_ABI scheduler_t
{
  scheduler_t();

  scheduler_t(scheduler_t const&) = delete;
  scheduler_t& operator=(scheduler_t const&) = delete;

  /*
   * Schedules a one-time callback at of after <when>.  Returns a
   * cancellation ticket that can be used to cancel the callback
   * before it is invoked.
   * Call this function again if you want another callback.
   */
  template<typename Callback>
  cancellation_ticket_t call_alarm(time_point_t when, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return cancellation_ticket_t(
      cancellation_ticket_t::type_t::alarm,
      this->do_call_alarm(when, std::move(callee)));
  }
    
  /*
   * Schedules a one-time callback at of after <timeout> from now.
   * Returns a cancellation ticket that can be used to cancel the
   * callback before it is invoked.
   * Call this function again if you want another callback.
   */
  template<typename Callback>
  cancellation_ticket_t call_alarm(duration_t timeout, Callback&& callback)
  {
    return this->call_alarm(
      clock_t::now() + timeout, std::forward<Callback>(callback));
  }
    
  /*
   * Schedules a one-time callback for when <fd> is ready for writing.
   * Returns a cancellation ticket that can be used to cancel the
   * callback before it is invoked.
   * Call this function again if you want another callback.
   */
  template<typename Callback>
  cancellation_ticket_t call_when_writable(int fd, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return cancellation_ticket_t(
      cancellation_ticket_t::type_t::writable,
      this->do_call_when_writable(fd, std::move(callee)));
  }

  /*
   * Schedules a one-time callback for when <fd> is ready for reading.
   * Returns a cancellation ticket that can be used to cancel the
   * callback before it is invoked.
   * Call this function again if you want another callback.
   */
  template<typename Callback>
  cancellation_ticket_t call_when_readable(int fd, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return cancellation_ticket_t(
      cancellation_ticket_t::type_t::readable,
      this->do_call_when_readable(fd, std::move(callee)));
  }

  /*
   * Cancels a callback before it is invoked.
   */
  void cancel(cancellation_ticket_t ticket) noexcept;
    
  virtual ~scheduler_t();

private :
  virtual int do_call_alarm(time_point_t when, callback_t callback) = 0;
  virtual void do_cancel_alarm(int ticket) noexcept = 0;
  virtual int do_call_when_writable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_writable(int ticket) noexcept = 0;
  virtual int do_call_when_readable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_readable(int ticket) noexcept = 0;
};

// SSTS: static start takes shared
template<typename EventHandler, typename... Args>
std::shared_ptr<EventHandler>
start_event_handler(scheduler_t& scheduler, Args&&... args)
{
  auto handler = std::make_shared<EventHandler>(std::forward<Args>(args)...);
  EventHandler::start(handler, scheduler);
  return handler;
}

} // cuti

#endif

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
  struct alarm_tag_t { };
  struct writable_tag_t { };
  struct readable_tag_t { };

  /*
   * Cancellation tickets may be used to cancel a previously scheduled
   * callback before it is invoked.
   *   
   * Please note: the ticket is only valid until the callback is
   * invoked.  Any attempt to cancel a callback during or after its
   * invocation leads to undefined behavior.
   */
  template<typename Tag>
  struct cancellation_ticket_t
  {
    /*
     * Constructs an empty cancellation ticket.
     */
    cancellation_ticket_t() noexcept
    : id_(-1)
    { }

    /*
     * Tells if the ticket is empty.  Scheduling a callback returns a
     * non-empty cancellation ticket.
     */
    bool empty() const noexcept
    { return id_ == -1; }

    /*
     * Sets the ticket to the empty state.
     */
    void clear() noexcept
    { id_ = -1; }

  private :
    friend struct scheduler_t;

    explicit cancellation_ticket_t(int id) noexcept
    : id_(id)
    { }

    int id() const noexcept
    { return id_; }

  private :
    int id_;
  };

  using alarm_ticket_t = cancellation_ticket_t<alarm_tag_t>;
  using writable_ticket_t = cancellation_ticket_t<writable_tag_t>;
  using readable_ticket_t = cancellation_ticket_t<readable_tag_t>;

  scheduler_t()
  { }

  scheduler_t(scheduler_t const&) = delete;
  scheduler_t& operator=(scheduler_t const&) = delete;

  /*
   * Schedules a one-time callback at of after <when>.  Returns a
   * cancellation ticket that is only valid until the callback is
   * invoked.  Call this function again if you want another callback.
   */
  template<typename Callback>
  alarm_ticket_t call_at(time_point_t when, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return alarm_ticket_t(this->do_call_at(when, std::move(callee)));
  }
    
  /*
   * Schedules a one-time callback at of after <timeout> from now.
   * Returns a cancellation ticket that is only valid until the
   * callback is invoked.  Call this function again if you want
   * another callback.
   */
  template<typename Callback>
  alarm_ticket_t call_in(duration_t timeout, Callback&& callback)
  {
    return this->call_at(
      clock_t::now() + timeout, std::forward<Callback>(callback));
  }
    
  /*
   * Schedules a one-time callback for when <fd> is ready for writing.
   * Returns a cancellation ticket that is only valid until the
   * callback is invoked.  Call this function again if you want
   * another callback.
   */
  template<typename Callback>
  writable_ticket_t call_when_writable(int fd, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return writable_ticket_t(this->do_call_when_writable(
      fd, std::move(callee)));
  }

  /*
   * Schedules a one-time callback for when <fd> is ready for reading.
   * Returns a cancellation ticket that is only valid until the
   * callback is invoked.  Call this function again if you want
   * another callback.
   */
  template<typename Callback>
  readable_ticket_t call_when_readable(int fd, Callback&& callback)
  {
    callback_t callee(std::forward<Callback>(callback));
    assert(callee != nullptr);
    return readable_ticket_t(this->do_call_when_readable(
      fd, std::move(callee)));
  }

  /*
   * Cancels a callback before it is invoked.
   */
  void cancel_callback(alarm_ticket_t ticket) noexcept
  {
    assert(!ticket.empty());
    this->do_cancel_at(ticket.id());
  }
    
  void cancel_callback(writable_ticket_t ticket) noexcept
  {
    assert(!ticket.empty());
    this->do_cancel_when_writable(ticket.id());
  }

  void cancel_callback(readable_ticket_t ticket) noexcept
  {
    assert(!ticket.empty());
    this->do_cancel_when_readable(ticket.id());
  }

  virtual ~scheduler_t();

private :
  virtual int do_call_at(time_point_t when, callback_t callback) = 0;
  virtual void do_cancel_at(int ticket) noexcept = 0;
  virtual int do_call_when_writable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_writable(int ticket) noexcept = 0;
  virtual int do_call_when_readable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_readable(int ticket) noexcept = 0;
};

using alarm_ticket_t = scheduler_t::alarm_ticket_t;
using writable_ticket_t = scheduler_t::writable_ticket_t;
using readable_ticket_t = scheduler_t::readable_ticket_t;

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

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

#ifndef CUTI_IO_SCHEDULER_HPP_
#define CUTI_IO_SCHEDULER_HPP_

#include "callback.hpp"
#include "cancellation_ticket.hpp"
#include "linkage.h"
#include "socket_nifty.hpp"

#include <utility>
#include <memory>

namespace cuti
{

/*
 * Abstract I/O event scheduler interface
 */
struct CUTI_ABI io_scheduler_t
{
  struct writable_tag_t { };
  struct readable_tag_t { };

  using writable_ticket_t =
    cancellation_ticket_t<io_scheduler_t, writable_tag_t>;
  using readable_ticket_t =
    cancellation_ticket_t<io_scheduler_t, readable_tag_t>;

  io_scheduler_t()
  { }

  io_scheduler_t(io_scheduler_t const&) = delete;
  io_scheduler_t& operator=(io_scheduler_t const&) = delete;

  /*
   * Schedules a one-time callback for when fd is ready for writing.
   * Returns a cancellation ticket that may be used to cancel the
   * callback before it is invoked.  Call this function again if you
   * want another callback.
   * Please note: when the callback is invoked, the ticket has already
   * lost its purpose; any further use leads to undefined behavior.
   */
  template<typename Callback>
  writable_ticket_t call_when_writable(int fd, Callback&& callback)
  {
    return writable_ticket_t(this->do_call_when_writable(
      fd, std::forward<Callback>(callback)));
  }

  /*
   * Schedules a one-time callback for when fd is ready for reading.
   * Returns a cancellation ticket that may be used to cancel the
   * callback before it is invoked.  Call this function again if you
   * want another callback.
   * Please note: when the callback is invoked, the ticket has already
   * lost its purpose; any further use leads to undefined behavior.
   */
  template<typename Callback>
  readable_ticket_t call_when_readable(int fd, Callback&& callback)
  {
    return readable_ticket_t(this->do_call_when_readable(
      fd, std::forward<Callback>(callback)));
  }

  /*
   * Cancels a callback before the callback is invoked.
   */
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

  virtual ~io_scheduler_t();

protected :
  enum class event_t { writable, readable };

private :
  virtual int do_call_when_writable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_writable(int ticket) noexcept = 0;
  virtual int do_call_when_readable(int fd, callback_t callback) = 0;
  virtual void do_cancel_when_readable(int ticket) noexcept = 0;
};

using writable_ticket_t = io_scheduler_t::writable_ticket_t;
using readable_ticket_t = io_scheduler_t::readable_ticket_t;

// SSTS: static start takes shared
template<typename IOHandler, typename... Args>
auto start_io_handler(io_scheduler_t& scheduler, Args&&... args)
{
  auto handler = std::make_shared<IOHandler>(std::forward<Args>(args)...);
  return IOHandler::start(handler, scheduler);
}

} // cuti

#endif
